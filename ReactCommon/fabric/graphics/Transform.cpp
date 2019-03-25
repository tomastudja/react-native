// Copyright (c) Facebook, Inc. and its affiliates.

// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#include "Transform.h"

#include <cmath>

namespace facebook {
namespace react {

Transform Transform::Identity() {
  return {};
}

Transform Transform::Perspective(Float perspective) {
  auto transform = Transform{};
  transform.matrix[11] = -1.0 / perspective;
  return transform;
}

Transform Transform::Scale(Float factorX, Float factorY, Float factorZ) {
  auto transform = Transform{};
  transform.matrix[0] = factorX;
  transform.matrix[5] = factorY;
  transform.matrix[10] = factorZ;
  return transform;
}

Transform Transform::Translate(Float x, Float y, Float z) {
  auto transform = Transform{};
  transform.matrix[12] = x;
  transform.matrix[13] = y;
  transform.matrix[14] = z;
  return transform;
}

Transform Transform::Skew(Float x, Float y) {
  auto transform = Transform{};
  transform.matrix[4] = std::tan(x);
  transform.matrix[1] = std::tan(y);
  return transform;
}

Transform Transform::RotateX(Float radians) {
  auto transform = Transform{};
  transform.matrix[5] = std::cos(radians);
  transform.matrix[6] = std::sin(radians);
  transform.matrix[9] = -std::sin(radians);
  transform.matrix[10] = std::cos(radians);
  return transform;
}

Transform Transform::RotateY(Float radians) {
  auto transform = Transform{};
  transform.matrix[0] = std::cos(radians);
  transform.matrix[2] = -std::sin(radians);
  transform.matrix[8] = std::sin(radians);
  transform.matrix[10] = std::cos(radians);
  return transform;
}

Transform Transform::RotateZ(Float radians) {
  auto transform = Transform{};
  transform.matrix[0] = std::cos(radians);
  transform.matrix[1] = std::sin(radians);
  transform.matrix[4] = -std::sin(radians);
  transform.matrix[5] = std::cos(radians);
  return transform;
}

Transform Transform::Rotate(Float x, Float y, Float z) {
  auto transform = Transform{};
  if (x != 0) {
    transform = transform * Transform::RotateX(x);
  }
  if (y != 0) {
    transform = transform * Transform::RotateY(y);
  }
  if (z != 0) {
    transform = transform * Transform::RotateZ(z);
  }
  return transform;
}

bool Transform::operator==(Transform const &rhs) const {
  for (auto i = 0; i < 16; i++) {
    if (matrix[i] != rhs.matrix[i]) {
      return false;
    }
  }
  return true;
}

bool Transform::operator!=(Transform const &rhs) const {
  return !(*this == rhs);
}

Transform Transform::operator*(Transform const &rhs) const {
  if (*this == Transform::Identity()) {
    return rhs;
  }

  const auto &lhs = *this;
  auto result = Transform{};

  auto lhs00 = lhs.matrix[0], lhs01 = lhs.matrix[1], lhs02 = lhs.matrix[2],
       lhs03 = lhs.matrix[3], lhs10 = lhs.matrix[4], lhs11 = lhs.matrix[5],
       lhs12 = lhs.matrix[6], lhs13 = lhs.matrix[7], lhs20 = lhs.matrix[8],
       lhs21 = lhs.matrix[9], lhs22 = lhs.matrix[10], lhs23 = lhs.matrix[11],
       lhs30 = lhs.matrix[12], lhs31 = lhs.matrix[13], lhs32 = lhs.matrix[14],
       lhs33 = lhs.matrix[15];

  auto rhs0 = rhs.matrix[0], rhs1 = rhs.matrix[1], rhs2 = rhs.matrix[2],
       rhs3 = rhs.matrix[3];
  result.matrix[0] = rhs0 * lhs00 + rhs1 * lhs10 + rhs2 * lhs20 + rhs3 * lhs30;
  result.matrix[1] = rhs0 * lhs01 + rhs1 * lhs11 + rhs2 * lhs21 + rhs3 * lhs31;
  result.matrix[2] = rhs0 * lhs02 + rhs1 * lhs12 + rhs2 * lhs22 + rhs3 * lhs32;
  result.matrix[3] = rhs0 * lhs03 + rhs1 * lhs13 + rhs2 * lhs23 + rhs3 * lhs33;

  rhs0 = rhs.matrix[4];
  rhs1 = rhs.matrix[5];
  rhs2 = rhs.matrix[6];
  rhs3 = rhs.matrix[7];
  result.matrix[4] = rhs0 * lhs00 + rhs1 * lhs10 + rhs2 * lhs20 + rhs3 * lhs30;
  result.matrix[5] = rhs0 * lhs01 + rhs1 * lhs11 + rhs2 * lhs21 + rhs3 * lhs31;
  result.matrix[6] = rhs0 * lhs02 + rhs1 * lhs12 + rhs2 * lhs22 + rhs3 * lhs32;
  result.matrix[7] = rhs0 * lhs03 + rhs1 * lhs13 + rhs2 * lhs23 + rhs3 * lhs33;

  rhs0 = rhs.matrix[8];
  rhs1 = rhs.matrix[9];
  rhs2 = rhs.matrix[10];
  rhs3 = rhs.matrix[11];
  result.matrix[8] = rhs0 * lhs00 + rhs1 * lhs10 + rhs2 * lhs20 + rhs3 * lhs30;
  result.matrix[9] = rhs0 * lhs01 + rhs1 * lhs11 + rhs2 * lhs21 + rhs3 * lhs31;
  result.matrix[10] = rhs0 * lhs02 + rhs1 * lhs12 + rhs2 * lhs22 + rhs3 * lhs32;
  result.matrix[11] = rhs0 * lhs03 + rhs1 * lhs13 + rhs2 * lhs23 + rhs3 * lhs33;

  rhs0 = rhs.matrix[12];
  rhs1 = rhs.matrix[13];
  rhs2 = rhs.matrix[14];
  rhs3 = rhs.matrix[15];
  result.matrix[12] = rhs0 * lhs00 + rhs1 * lhs10 + rhs2 * lhs20 + rhs3 * lhs30;
  result.matrix[13] = rhs0 * lhs01 + rhs1 * lhs11 + rhs2 * lhs21 + rhs3 * lhs31;
  result.matrix[14] = rhs0 * lhs02 + rhs1 * lhs12 + rhs2 * lhs22 + rhs3 * lhs32;
  result.matrix[15] = rhs0 * lhs03 + rhs1 * lhs13 + rhs2 * lhs23 + rhs3 * lhs33;

  return result;
}

} // namespace react
} // namespace facebook
