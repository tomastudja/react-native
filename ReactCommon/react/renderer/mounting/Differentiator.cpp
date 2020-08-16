/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "Differentiator.h"

#include <better/map.h>
#include <better/small_vector.h>
#include <react/renderer/core/LayoutableShadowNode.h>
#include <react/renderer/debug/SystraceSection.h>
#include <algorithm>
#include "ShadowView.h"

// Uncomment this to enable verbose diffing logs, which can be useful for
// debugging. #define DEBUG_LOGS_DIFFER

#ifdef DEBUG_LOGS_DIFFER
#include <glog/logging.h>
#define DEBUG_LOGS(code) code
#else
#define DEBUG_LOGS(code)
#endif

namespace facebook {
namespace react {

/*
 * Extremely simple and naive implementation of a map.
 * The map is simple but it's optimized for particular constraints that we have
 * here.
 *
 * A regular map implementation (e.g. `std::unordered_map`) has some basic
 * performance guarantees like constant average insertion and lookup complexity.
 * This is nice, but it's *average* complexity measured on a non-trivial amount
 * of data. The regular map is a very complex data structure that using hashing,
 * buckets, multiple comprising operations, multiple allocations and so on.
 *
 * In our particular case, we need a map for `int` to `void *` with a dozen
 * values. In these conditions, nothing can beat a naive implementation using a
 * stack-allocated vector. And this implementation is exactly this: no
 * allocation, no hashing, no complex branching, no buckets, no iterators, no
 * rehashing, no other guarantees. It's crazy limited, unsafe, and performant on
 * a trivial amount of data.
 *
 * Besides that, we also need to optimize for insertion performance (the case
 * where a bunch of views appears on the screen first time); in this
 * implementation, this is as performant as vector `push_back`.
 */
template <typename KeyT, typename ValueT, int DefaultSize = 16>
class TinyMap final {
 public:
  using Pair = std::pair<KeyT, ValueT>;
  using Iterator = Pair *;

  /**
   * This must strictly only be called from outside of this class.
   */
  inline Iterator begin() {
    // Force a clean so that iterating over this TinyMap doesn't iterate over
    // erased elements. If all elements erased are at the front of the vector,
    // then we don't need to clean.
    cleanVector(erasedAtFront_ != numErased_);

    Iterator it = begin_();

    if (it != nullptr) {
      return it + erasedAtFront_;
    }

    return nullptr;
  }

  inline Iterator end() {
    // `back()` asserts on the vector being non-empty
    if (vector_.empty() || numErased_ == vector_.size()) {
      return nullptr;
    }

    return &vector_.back() + 1;
  }

  inline Iterator find(KeyT key) {
    cleanVector();

    assert(key != 0);

    if (begin_() == nullptr) {
      return end();
    }

    for (auto it = begin_() + erasedAtFront_; it != end(); it++) {
      if (it->first == key) {
        return it;
      }
    }

    return end();
  }

  inline void insert(Pair pair) {
    assert(pair.first != 0);
    vector_.push_back(pair);
  }

  inline void erase(Iterator iterator) {
    // Invalidate tag.
    iterator->first = 0;

    if (iterator == begin_() + erasedAtFront_) {
      erasedAtFront_++;
    }

    numErased_++;
  }

 private:
  /**
   * Same as begin() but doesn't call cleanVector at the beginning.
   */
  inline Iterator begin_() {
    // `front()` asserts on the vector being non-empty
    if (vector_.empty() || vector_.size() == numErased_) {
      return nullptr;
    }

    return &vector_.front();
  }

  /**
   * Remove erased elements from internal vector.
   * We only modify the vector if erased elements are at least half of the
   * vector.
   */
  inline void cleanVector(bool forceClean = false) {
    if ((numErased_ < (vector_.size() / 2) && !forceClean) || vector_.empty() ||
        numErased_ == 0 || numErased_ == erasedAtFront_) {
      return;
    }

    if (numErased_ == vector_.size()) {
      vector_.clear();
    } else {
      vector_.erase(
          std::remove_if(
              vector_.begin(),
              vector_.end(),
              [](auto const &item) { return item.first == 0; }),
          vector_.end());
    }
    numErased_ = 0;
    erasedAtFront_ = 0;
  }

  better::small_vector<Pair, DefaultSize> vector_;
  int numErased_{0};
  int erasedAtFront_{0};
};

struct OperationsOnTag final {
  int shouldEraseOp = 0;
  int opExists = 0;

  int removeInsertIndex = -1;
  Tag parentTag =
      -1; // the parent tag of Remove or Insert, whichever comes first

  ShadowNode const *oldNode;
  ShadowNode const *newNode;
};

class ReparentingMetadata final {
 public:
  ReparentingMetadata(bool enabled) : enabled_(enabled) {}

  /**
   * Returns a triple: (ShouldRemove, ShouldDelete, ShadowNode pointer if
   * updating)
   *
   * @param mutation
   * @return
   */
  std::tuple<bool, bool, ShadowNode const *> shouldRemoveDeleteUpdate(
      Tag parentTag,
      ShadowNode const *shadowNode,
      int index) {
    if (!enabled_) {
      return std::tuple<bool, bool, ShadowNode const *>(true, true, nullptr);
    }

    Tag tag = shadowNode->getTag();

    auto it = tagsToOperations_.find(tag);
    if (it == tagsToOperations_.end()) {
      auto tagOperations = OperationsOnTag{};
      tagOperations.removeInsertIndex = index;
      tagOperations.parentTag = parentTag;
      tagOperations.opExists |=
          ShadowViewMutation::Type::Remove | ShadowViewMutation::Type::Delete;
      tagOperations.oldNode = shadowNode;
      tagsToOperations_[tag] = tagOperations;
      return std::tuple<bool, bool, ShadowNode const *>(true, true, nullptr);
    }

    assert(it->second.shouldEraseOp == 0);
    bool shouldRemove =
        !((it->second.opExists & ShadowViewMutation::Type::Insert) &&
          it->second.removeInsertIndex == index &&
          it->second.parentTag == parentTag);
    it->second.shouldEraseOp |=
        (it->second.opExists & ShadowViewMutation::Type::Create);
    it->second.shouldEraseOp |= shouldRemove
        ? 0
        : (it->second.opExists & ShadowViewMutation::Type::Insert);

    if (it->second.shouldEraseOp != 0) {
      reparentingOperations_++;
    }

    // TODO: At this point we are *done* with this record in the map besides
    // postprocessing, and we know we've reparented. There is a potential
    // optimization here.

    return std::tuple<bool, bool, ShadowNode const *>(
        shouldRemove, false, it->second.newNode);
  }

  /**
   * Returns a triple: (ShouldCreate, ShouldInsert, ShadowNode pointer if
   * updating)
   *
   * @param mutation
   * @return
   */
  std::tuple<bool, bool, ShadowNode const *> shouldCreateInsertUpdate(
      Tag parentTag,
      ShadowNode const *shadowNode,
      int index) {
    if (!enabled_) {
      return std::tuple<bool, bool, ShadowNode const *>(true, true, nullptr);
    }

    Tag tag = shadowNode->getTag();

    auto it = tagsToOperations_.find(tag);
    if (it == tagsToOperations_.end()) {
      auto tagOperations = OperationsOnTag{};
      tagOperations.removeInsertIndex = index;
      tagOperations.opExists |=
          ShadowViewMutation::Type::Create | ShadowViewMutation::Type::Insert;
      tagOperations.newNode = shadowNode;
      tagsToOperations_[tag] = tagOperations;
      return std::tuple<bool, bool, ShadowNode const *>(true, true, nullptr);
    }

    assert(it->second.shouldEraseOp == 0);
    bool shouldInsert =
        !((it->second.opExists & ShadowViewMutation::Type::Remove) &&
          it->second.removeInsertIndex == index &&
          it->second.parentTag == parentTag);
    it->second.shouldEraseOp |=
        (it->second.opExists & ShadowViewMutation::Type::Delete);
    it->second.shouldEraseOp |= shouldInsert
        ? 0
        : (it->second.opExists & ShadowViewMutation::Type::Remove);

    if (it->second.shouldEraseOp != 0) {
      reparentingOperations_++;
    }

    // TODO: At this point we are *done* with this record in the map besides
    // postprocessing, and we know we've reparented. There is a potential
    // optimization here.

    return std::tuple<bool, bool, ShadowNode const *>(
        shouldInsert, false, it->second.oldNode);
  }

  /**
   * Returns a pair: (ShouldCreate, ShadowNode pointer if updating)
   * This is called in a case where a node has *already* been inserted.
   *
   * @param mutation
   * @return
   */
  std::pair<bool, ShadowNode const *> shouldCreateUpdate(
      ShadowNode const *shadowNode) {
    if (!enabled_) {
      return std::pair<bool, ShadowNode const *>(true, nullptr);
    }

    Tag tag = shadowNode->getTag();

    auto it = tagsToOperations_.find(tag);
    assert(it != tagsToOperations_.end());

    if (it->second.opExists & ShadowViewMutation::Type::Delete) {
      reparentingOperations_++;
      it->second.shouldEraseOp |= ShadowViewMutation::Type::Delete;
      it->second.newNode = shadowNode; // Is this necessary?
      return std::pair<bool, ShadowNode const *>(false, it->second.oldNode);
    }

    it->second.opExists |= ShadowViewMutation::Type::Create;
    return std::pair<bool, ShadowNode const *>(true, nullptr);
  }

  /**
   * Insertion is happening due to reordering and likely cannot be canceled.
   *
   * @param shadowNode
   * @param index
   */
  void markInserted(Tag parentTag, ShadowNode const *shadowNode, int index) {
    if (!enabled_) {
      return;
    }

    Tag tag = shadowNode->getTag();

    auto it = tagsToOperations_.find(tag);
    if (it == tagsToOperations_.end()) {
      auto tagOperations = OperationsOnTag{};
      tagOperations.removeInsertIndex = index;
      tagOperations.parentTag = parentTag;
      it->second.opExists |= ShadowViewMutation::Type::Insert;
      tagsToOperations_[tag] = tagOperations;
      return;
    }

    // Element was moved from somewhere else in the hierarchy and inserted
    // in a new position - we can't cancel this operation.
    it->second.opExists |= ShadowViewMutation::Type::Insert;
  }

  /**
   * Use this to prepare for iterating over records for ShadowViewMutation
   * removal. This removes unnecessary information from the map.
   */
  void removeUselessRecords() {
    if (!enabled_) {
      return;
    }

    for (auto it = tagsToOperations_.begin(); it != tagsToOperations_.end();) {
      OperationsOnTag &op = it->second;
      if (op.shouldEraseOp == 0) {
        it = tagsToOperations_.erase(it);
      } else {
        it++;
      }
    }
  }

  bool enabled_{false};
  int reparentingOperations_ = 0;
  std::map<Tag, OperationsOnTag> tagsToOperations_;
};

/*
 * Sorting comparator for `reorderInPlaceIfNeeded`.
 */
static bool shouldFirstPairComesBeforeSecondOne(
    ShadowViewNodePair const &lhs,
    ShadowViewNodePair const &rhs) noexcept {
  return lhs.shadowNode->getOrderIndex() < rhs.shadowNode->getOrderIndex();
}

/*
 * Reorders pairs in-place based on `orderIndex` using a stable sort algorithm.
 */
static void reorderInPlaceIfNeeded(ShadowViewNodePair::List &pairs) noexcept {
  if (pairs.size() < 2) {
    return;
  }

  auto isReorderNeeded = false;
  for (auto const &pair : pairs) {
    if (pair.shadowNode->getOrderIndex() != 0) {
      isReorderNeeded = true;
      break;
    }
  }

  if (!isReorderNeeded) {
    return;
  }

  std::stable_sort(
      pairs.begin(), pairs.end(), &shouldFirstPairComesBeforeSecondOne);
}

static void sliceChildShadowNodeViewPairsRecursively(
    ShadowViewNodePair::List &pairList,
    Point layoutOffset,
    ShadowNode const &shadowNode) {
  for (auto const &sharedChildShadowNode : shadowNode.getChildren()) {
    auto &childShadowNode = *sharedChildShadowNode;

#ifndef ANDROID
    // Temporary disabled on Android because the mounting infrastructure
    // is not fully ready yet.
    if (childShadowNode.getTraits().check(ShadowNodeTraits::Trait::Hidden)) {
      continue;
    }
#endif

    auto shadowView = ShadowView(childShadowNode);
    auto origin = layoutOffset;
    if (shadowView.layoutMetrics != EmptyLayoutMetrics) {
      origin += shadowView.layoutMetrics.frame.origin;
      shadowView.layoutMetrics.frame.origin += layoutOffset;
    }

    if (childShadowNode.getTraits().check(
            ShadowNodeTraits::Trait::FormsStackingContext)) {
      pairList.push_back({shadowView, &childShadowNode});
    } else {
      if (childShadowNode.getTraits().check(
              ShadowNodeTraits::Trait::FormsView)) {
        pairList.push_back({shadowView, &childShadowNode});
      }

      sliceChildShadowNodeViewPairsRecursively(
          pairList, origin, childShadowNode);
    }
  }
}

ShadowViewNodePair::List sliceChildShadowNodeViewPairs(
    ShadowNode const &shadowNode) {
  auto pairList = ShadowViewNodePair::List{};

  if (!shadowNode.getTraits().check(
          ShadowNodeTraits::Trait::FormsStackingContext) &&
      shadowNode.getTraits().check(ShadowNodeTraits::Trait::FormsView)) {
    return pairList;
  }

  sliceChildShadowNodeViewPairsRecursively(pairList, {0, 0}, shadowNode);

  return pairList;
}

/*
 * Before we start to diff, let's make sure all our core data structures are in
 * good shape to deliver the best performance.
 */
static_assert(
    std::is_move_constructible<ShadowViewMutation>::value,
    "`ShadowViewMutation` must be `move constructible`.");
static_assert(
    std::is_move_constructible<ShadowView>::value,
    "`ShadowView` must be `move constructible`.");
static_assert(
    std::is_move_constructible<ShadowViewNodePair>::value,
    "`ShadowViewNodePair` must be `move constructible`.");
static_assert(
    std::is_move_constructible<ShadowViewNodePair::List>::value,
    "`ShadowViewNodePair::List` must be `move constructible`.");

static_assert(
    std::is_move_assignable<ShadowViewMutation>::value,
    "`ShadowViewMutation` must be `move assignable`.");
static_assert(
    std::is_move_assignable<ShadowView>::value,
    "`ShadowView` must be `move assignable`.");
static_assert(
    std::is_move_assignable<ShadowViewNodePair>::value,
    "`ShadowViewNodePair` must be `move assignable`.");
static_assert(
    std::is_move_assignable<ShadowViewNodePair::List>::value,
    "`ShadowViewNodePair::List` must be `move assignable`.");

static void calculateShadowViewMutations(
    ShadowViewMutation::List &mutations,
    ReparentingMetadata &reparentingMetadata,
    ShadowView const &parentShadowView,
    ShadowViewNodePair::List &&oldChildPairs,
    ShadowViewNodePair::List &&newChildPairs) {
  if (oldChildPairs.empty() && newChildPairs.empty()) {
    return;
  }

  // Sorting pairs based on `orderIndex` if needed.
  reorderInPlaceIfNeeded(oldChildPairs);
  reorderInPlaceIfNeeded(newChildPairs);

  auto index = int{0};

  // Lists of mutations
  auto createMutations = ShadowViewMutation::List{};
  auto deleteMutations = ShadowViewMutation::List{};
  auto insertMutations = ShadowViewMutation::List{};
  auto removeMutations = ShadowViewMutation::List{};
  auto updateMutations = ShadowViewMutation::List{};
  auto downwardMutations = ShadowViewMutation::List{};
  auto destructiveDownwardMutations = ShadowViewMutation::List{};

  // Stage 1: Collecting `Update` mutations
  for (index = 0; index < oldChildPairs.size() && index < newChildPairs.size();
       index++) {
    auto const &oldChildPair = oldChildPairs[index];
    auto const &newChildPair = newChildPairs[index];

    if (oldChildPair.shadowView.tag != newChildPair.shadowView.tag) {
      DEBUG_LOGS({
        LOG(ERROR) << "Differ Branch 1.1: Tags Different: ["
                   << oldChildPair.shadowView.tag << "] ["
                   << newChildPair.shadowView.tag << "]";
      });

      // Totally different nodes, updating is impossible.
      break;
    }

    DEBUG_LOGS({
      LOG(ERROR) << "Differ Branch 1.2: Same tags, update and recurse: ["
                 << oldChildPair.shadowView.tag << "] ["
                 << newChildPair.shadowView.tag << "]";
    });

    if (oldChildPair.shadowView != newChildPair.shadowView) {
      updateMutations.push_back(ShadowViewMutation::UpdateMutation(
          parentShadowView,
          oldChildPair.shadowView,
          newChildPair.shadowView,
          index));
    }

    auto oldGrandChildPairs =
        sliceChildShadowNodeViewPairs(*oldChildPair.shadowNode);
    auto newGrandChildPairs =
        sliceChildShadowNodeViewPairs(*newChildPair.shadowNode);
    calculateShadowViewMutations(
        *(newGrandChildPairs.size() ? &downwardMutations
                                    : &destructiveDownwardMutations),
        reparentingMetadata,
        oldChildPair.shadowView,
        std::move(oldGrandChildPairs),
        std::move(newGrandChildPairs));
  }

  int lastIndexAfterFirstStage = index;

  if (index == newChildPairs.size()) {
    // We've reached the end of the new children. We can delete+remove the
    // rest.
    for (; index < oldChildPairs.size(); index++) {
      auto const &oldChildPair = oldChildPairs[index];

      DEBUG_LOGS({
        LOG(ERROR)
            << "Differ Branch 2: Deleting Tag/Tree (may be reparented): ["
            << oldChildPair.shadowView.tag << "]";
      });

      bool shouldRemove;
      bool shouldDelete;
      ShadowNode const *newTreeNode;
      std::tie(shouldRemove, shouldDelete, newTreeNode) =
          reparentingMetadata.shouldRemoveDeleteUpdate(
              parentShadowView.tag, oldChildPair.shadowNode, index);

      if (shouldDelete) {
        deleteMutations.push_back(
            ShadowViewMutation::DeleteMutation(oldChildPair.shadowView));
      }
      if (shouldRemove) {
        removeMutations.push_back(ShadowViewMutation::RemoveMutation(
            parentShadowView, oldChildPair.shadowView, index));
      }
      if (newTreeNode != nullptr) {
        ShadowView newTreeNodeView = ShadowView(*newTreeNode);
        if (newTreeNodeView != oldChildPair.shadowView) {
          updateMutations.push_back(ShadowViewMutation::UpdateMutation(
              parentShadowView, oldChildPair.shadowView, newTreeNodeView, -1));
        }
      }

      // We also have to call the algorithm recursively to clean up the entire
      // subtree starting from the removed view.
      calculateShadowViewMutations(
          destructiveDownwardMutations,
          reparentingMetadata,
          oldChildPair.shadowView,
          sliceChildShadowNodeViewPairs(*oldChildPair.shadowNode),
          {});
    }
  } else if (index == oldChildPairs.size()) {
    // If we don't have any more existing children we can choose a fast path
    // since the rest will all be create+insert.
    for (; index < newChildPairs.size(); index++) {
      auto const &newChildPair = newChildPairs[index];

      DEBUG_LOGS({
        LOG(ERROR)
            << "Differ Branch 3: Creating Tag/Tree (may be reparented): ["
            << newChildPair.shadowView.tag << "]";
      });

      bool shouldInsert;
      bool shouldCreate;
      ShadowNode const *oldTreeNode;
      std::tie(shouldInsert, shouldCreate, oldTreeNode) =
          reparentingMetadata.shouldCreateInsertUpdate(
              parentShadowView.tag, newChildPair.shadowNode, index);

      if (shouldInsert) {
        insertMutations.push_back(ShadowViewMutation::InsertMutation(
            parentShadowView, newChildPair.shadowView, index));
      }
      if (shouldCreate) {
        createMutations.push_back(
            ShadowViewMutation::CreateMutation(newChildPair.shadowView));
      }
      if (oldTreeNode != nullptr) {
        ShadowView oldTreeNodeView = ShadowView(*oldTreeNode);
        if (oldTreeNodeView != newChildPair.shadowView) {
          updateMutations.push_back(ShadowViewMutation::UpdateMutation(
              parentShadowView, oldTreeNodeView, newChildPair.shadowView, -1));
        }
      }

      calculateShadowViewMutations(
          downwardMutations,
          reparentingMetadata,
          newChildPair.shadowView,
          {},
          sliceChildShadowNodeViewPairs(*newChildPair.shadowNode));
    }
  } else {
    // Collect map of tags in the new list
    // In the future it would be nice to use TinyMap for newInsertedPairs, but
    // it's challenging to build an iterator that will work for our use-case
    // here.
    auto newRemainingPairs = TinyMap<Tag, ShadowViewNodePair const *>{};
    auto newInsertedPairs = TinyMap<Tag, ShadowViewNodePair const *>{};
    for (; index < newChildPairs.size(); index++) {
      auto const &newChildPair = newChildPairs[index];
      newRemainingPairs.insert({newChildPair.shadowView.tag, &newChildPair});
    }

    // Walk through both lists at the same time
    // We will perform updates, create+insert, remove+delete, remove+insert
    // (move) here.
    int oldIndex = lastIndexAfterFirstStage,
        newIndex = lastIndexAfterFirstStage, newSize = newChildPairs.size(),
        oldSize = oldChildPairs.size();
    while (newIndex < newSize || oldIndex < oldSize) {
      bool haveNewPair = newIndex < newSize;
      bool haveOldPair = oldIndex < oldSize;

      // Advance both pointers if pointing to the same element
      if (haveNewPair && haveOldPair) {
        auto const &newChildPair = newChildPairs[newIndex];
        auto const &oldChildPair = oldChildPairs[oldIndex];

        int newTag = newChildPair.shadowView.tag;
        int oldTag = oldChildPair.shadowView.tag;

        if (newTag == oldTag) {
          DEBUG_LOGS({
            LOG(ERROR) << "Differ Branch 5: Matched Tags at indices: "
                       << oldIndex << " " << newIndex << ": ["
                       << oldChildPair.shadowView.tag << "]["
                       << newChildPair.shadowView.tag << "]";
          });

          // Generate Update instructions
          if (oldChildPair.shadowView != newChildPair.shadowView) {
            updateMutations.push_back(ShadowViewMutation::UpdateMutation(
                parentShadowView,
                oldChildPair.shadowView,
                newChildPair.shadowView,
                index));
          }

          // Remove from newRemainingPairs
          auto newRemainingPairIt = newRemainingPairs.find(oldTag);
          if (newRemainingPairIt != newRemainingPairs.end()) {
            newRemainingPairs.erase(newRemainingPairIt);
          }

          // Update subtrees
          auto oldGrandChildPairs =
              sliceChildShadowNodeViewPairs(*oldChildPair.shadowNode);
          auto newGrandChildPairs =
              sliceChildShadowNodeViewPairs(*newChildPair.shadowNode);
          calculateShadowViewMutations(
              *(newGrandChildPairs.size() ? &downwardMutations
                                          : &destructiveDownwardMutations),
              reparentingMetadata,
              oldChildPair.shadowView,
              std::move(oldGrandChildPairs),
              std::move(newGrandChildPairs));

          newIndex++;
          oldIndex++;
          continue;
        }
      }

      if (haveOldPair) {
        auto const &oldChildPair = oldChildPairs[oldIndex];
        int oldTag = oldChildPair.shadowView.tag;

        // Was oldTag already inserted? This indicates a reordering, not just
        // a move. The new node has already been inserted, we just need to
        // remove the node from its old position now.
        auto const insertedIt = newInsertedPairs.find(oldTag);
        if (insertedIt != newInsertedPairs.end()) {
          DEBUG_LOGS({
            LOG(ERROR)
                << "Differ Branch 6: Removing tag that was already inserted: "
                << oldIndex << ": [" << oldChildPair.shadowView.tag << "]";
          });

          removeMutations.push_back(ShadowViewMutation::RemoveMutation(
              parentShadowView, oldChildPair.shadowView, oldIndex));

          // Generate update instruction since we have an iterator ref to the
          // new node
          auto const &newChildPair = *insertedIt->second;
          if (oldChildPair.shadowView != newChildPair.shadowView) {
            updateMutations.push_back(ShadowViewMutation::UpdateMutation(
                parentShadowView,
                oldChildPair.shadowView,
                newChildPair.shadowView,
                index));
          }

          // Update subtrees
          auto oldGrandChildPairs =
              sliceChildShadowNodeViewPairs(*oldChildPair.shadowNode);
          auto newGrandChildPairs =
              sliceChildShadowNodeViewPairs(*newChildPair.shadowNode);
          calculateShadowViewMutations(
              *(newGrandChildPairs.size() ? &downwardMutations
                                          : &destructiveDownwardMutations),
              reparentingMetadata,
              oldChildPair.shadowView,
              std::move(oldGrandChildPairs),
              std::move(newGrandChildPairs));

          newInsertedPairs.erase(insertedIt);
          oldIndex++;
          continue;
        }

        // Should we generate a delete+remove instruction for the old node?
        // If there's an old node and it's not found in the "new" list, we
        // generate remove+delete for this node and its subtree.
        auto const newIt = newRemainingPairs.find(oldTag);
        if (newIt == newRemainingPairs.end()) {
          DEBUG_LOGS({
            LOG(ERROR)
                << "Differ Branch 8: Removing tag/tree that was not reinserted (may be reparented): "
                << oldIndex << ": [" << oldChildPair.shadowView.tag << "]";
          });

          bool shouldRemove;
          bool shouldDelete;
          ShadowNode const *newTreeNode;
          // Indices and parentTag don't matter here because we will always be
          // executing this Remove operation, since it's in the context of
          // reordering of views and the View was not already in this hierarchy.
          std::tie(shouldRemove, shouldDelete, newTreeNode) =
              reparentingMetadata.shouldRemoveDeleteUpdate(
                  -1, oldChildPair.shadowNode, -1);

          removeMutations.push_back(ShadowViewMutation::RemoveMutation(
              parentShadowView, oldChildPair.shadowView, oldIndex));

          if (shouldDelete) {
            deleteMutations.push_back(
                ShadowViewMutation::DeleteMutation(oldChildPair.shadowView));
          }
          if (newTreeNode != nullptr) {
            ShadowView newTreeNodeView = ShadowView(*newTreeNode);
            if (newTreeNodeView != oldChildPair.shadowView) {
              updateMutations.push_back(ShadowViewMutation::UpdateMutation(
                  parentShadowView,
                  oldChildPair.shadowView,
                  newTreeNodeView,
                  -1));
            }
          }

          // We also have to call the algorithm recursively to clean up the
          // entire subtree starting from the removed view.
          calculateShadowViewMutations(
              destructiveDownwardMutations,
              reparentingMetadata,
              oldChildPair.shadowView,
              sliceChildShadowNodeViewPairs(*oldChildPair.shadowNode),
              {});

          oldIndex++;
          continue;
        }
      }

      // At this point, oldTag is -1 or is in the new list, and hasn't been
      // inserted or matched yet. We're not sure yet if the new node is in the
      // old list - generate an insert instruction for the new node.
      auto const &newChildPair = newChildPairs[newIndex];
      DEBUG_LOGS({
        LOG(ERROR)
            << "Differ Branch 9: Inserting tag/tree that was not yet removed from hierarchy (may be reparented): "
            << newIndex << ": [" << newChildPair.shadowView.tag << "]";
      });
      reparentingMetadata.markInserted(
          parentShadowView.tag, newChildPair.shadowNode, newIndex);
      insertMutations.push_back(ShadowViewMutation::InsertMutation(
          parentShadowView, newChildPair.shadowView, newIndex));
      newInsertedPairs.insert({newChildPair.shadowView.tag, &newChildPair});
      newIndex++;
    }

    // Final step: generate Create instructions for new nodes
    for (auto it = newInsertedPairs.begin(); it != newInsertedPairs.end();
         it++) {
      // Erased elements of a TinyMap will have a Tag/key of 0 - skip those
      // These *should* be removed by the map; there are currently no KNOWN
      // cases where TinyMap will do the wrong thing, but there are not yet
      // any unit tests explicitly for TinyMap, so this is safer for now.
      if (it->first == 0) {
        continue;
      }

      auto const &newChildPair = *it->second;

      DEBUG_LOGS({
        LOG(ERROR)
            << "Differ Branch 9: Inserting tag/tree that was not yet removed from hierarchy (may be reparented): "
            << newIndex << ": [" << newChildPair.shadowView.tag << "]";
      });

      bool shouldCreate;
      ShadowNode const *updateNode;
      std::tie(shouldCreate, updateNode) =
          reparentingMetadata.shouldCreateUpdate(newChildPair.shadowNode);

      if (shouldCreate) {
        createMutations.push_back(
            ShadowViewMutation::CreateMutation(newChildPair.shadowView));
      }

      if (updateNode != nullptr) {
        ShadowView updateNodeView = ShadowView(*updateNode);
        if (updateNodeView != newChildPair.shadowView) {
          updateMutations.push_back(ShadowViewMutation::UpdateMutation(
              parentShadowView, updateNodeView, newChildPair.shadowView, -1));
        }
      }

      calculateShadowViewMutations(
          downwardMutations,
          reparentingMetadata,
          newChildPair.shadowView,
          {},
          sliceChildShadowNodeViewPairs(*newChildPair.shadowNode));
    }
  }

  // All mutations in an optimal order:
  std::move(
      destructiveDownwardMutations.begin(),
      destructiveDownwardMutations.end(),
      std::back_inserter(mutations));
  std::move(
      updateMutations.begin(),
      updateMutations.end(),
      std::back_inserter(mutations));
  std::move(
      removeMutations.rbegin(),
      removeMutations.rend(),
      std::back_inserter(mutations));
  std::move(
      deleteMutations.begin(),
      deleteMutations.end(),
      std::back_inserter(mutations));
  std::move(
      createMutations.begin(),
      createMutations.end(),
      std::back_inserter(mutations));
  std::move(
      downwardMutations.begin(),
      downwardMutations.end(),
      std::back_inserter(mutations));
  std::move(
      insertMutations.begin(),
      insertMutations.end(),
      std::back_inserter(mutations));
}

ShadowViewMutation::List calculateShadowViewMutations(
    ShadowNode const &oldRootShadowNode,
    ShadowNode const &newRootShadowNode,
    bool enableReparentingDetection) {
  SystraceSection s("calculateShadowViewMutations");

  // Root shadow nodes must be belong the same family.
  assert(ShadowNode::sameFamily(oldRootShadowNode, newRootShadowNode));

  auto mutations = ShadowViewMutation::List{};
  mutations.reserve(256);

  auto reparentingMetadata = ReparentingMetadata(enableReparentingDetection);

  auto oldRootShadowView = ShadowView(oldRootShadowNode);
  auto newRootShadowView = ShadowView(newRootShadowNode);

  if (oldRootShadowView != newRootShadowView) {
    mutations.push_back(ShadowViewMutation::UpdateMutation(
        ShadowView(), oldRootShadowView, newRootShadowView, -1));
  }

  calculateShadowViewMutations(
      mutations,
      reparentingMetadata,
      ShadowView(oldRootShadowNode),
      sliceChildShadowNodeViewPairs(oldRootShadowNode),
      sliceChildShadowNodeViewPairs(newRootShadowNode));

  // Remove instructions obviated by reparenting
  if (reparentingMetadata.reparentingOperations_ > 0 &&
      enableReparentingDetection) {
    reparentingMetadata.removeUselessRecords();

    mutations.erase(
        std::remove_if(
            mutations.begin(),
            mutations.end(),
            [&](ShadowViewMutation &mutation) {
              if (reparentingMetadata.reparentingOperations_ == 0) {
                return false;
              }

              ShadowViewMutation::Type type = mutation.type;
              Tag tag = type == ShadowViewMutation::Type::Insert ||
                      type == ShadowViewMutation::Type::Create
                  ? mutation.newChildShadowView.tag
                  : mutation.oldChildShadowView.tag;
              auto it = reparentingMetadata.tagsToOperations_.find(tag);
              if (it != reparentingMetadata.tagsToOperations_.end()) {
                bool shouldDelete = it->second.shouldEraseOp & type;
                it->second.shouldEraseOp &= ~type;

                // We've done everything we need to with this record; delete it.
                if (it->second.shouldEraseOp == 0) {
                  reparentingMetadata.tagsToOperations_.erase(it);
                  reparentingMetadata.reparentingOperations_--;
                }

                return shouldDelete;
              }

              return false;
            }),
        mutations.end());
  }

  return mutations;
}

} // namespace react
} // namespace facebook
