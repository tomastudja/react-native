/**
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ShadowViewMutation.h"

namespace facebook {
namespace react {

ShadowViewMutation ShadowViewMutation::CreateMutation(ShadowView shadowView) {
  return {
      /* .type = */ Create,
      /* .parentShadowView = */ {},
      /* .newChildShadowView = */ shadowView,
      /* .oldChildShadowView = */ {},
      /* .index = */ -1,
  };
}

ShadowViewMutation ShadowViewMutation::DeleteMutation(ShadowView shadowView) {
  return {
      /* .type = */ Delete,
      /* .parentShadowView = */ {},
      /* .oldChildShadowView = */ shadowView,
      /* .newChildShadowView = */ {},
      /* .index = */ -1,
  };
}

ShadowViewMutation ShadowViewMutation::InsertMutation(
    ShadowView parentShadowView,
    ShadowView childShadowView,
    int index) {
  return {
      /* .type = */ Insert,
      /* .parentShadowView = */ parentShadowView,
      /* .oldChildShadowView = */ {},
      /* .newChildShadowView = */ childShadowView,
      /* .index = */ index,
  };
}

ShadowViewMutation ShadowViewMutation::RemoveMutation(
    ShadowView parentShadowView,
    ShadowView childShadowView,
    int index) {
  return {
      /* .type = */ Remove,
      /* .parentShadowView = */ parentShadowView,
      /* .oldChildShadowView = */ childShadowView,
      /* .newChildShadowView = */ {},
      /* .index = */ index,
  };
}

ShadowViewMutation ShadowViewMutation::UpdateMutation(
    ShadowView parentShadowView,
    ShadowView oldChildShadowView,
    ShadowView newChildShadowView,
    int index) {
  return {
      /* .type = */ Update,
      /* .parentShadowView = */ parentShadowView,
      /* .oldChildShadowView = */ oldChildShadowView,
      /* .newChildShadowView = */ newChildShadowView,
      /* .index = */ index,
  };
}

} // namespace react
} // namespace facebook
