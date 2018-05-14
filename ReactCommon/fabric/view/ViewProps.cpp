/**
 * Copyright (c) 2015-present, Facebook, Inc.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ViewProps.h"

#include <fabric/core/propsConversions.h>
#include <fabric/debug/debugStringConvertibleUtils.h>
#include <fabric/graphics/debugStringConvertibleUtils.h>
#include <fabric/graphics/graphicValuesConversions.h>

namespace facebook {
namespace react {

ViewProps::ViewProps(const YGStyle &yogaStyle):
  YogaStylableProps(yogaStyle) {}

ViewProps::ViewProps(const ViewProps &sourceProps, const RawProps &rawProps):
  Props(sourceProps, rawProps),
  YogaStylableProps(sourceProps, rawProps),
  zIndex(convertRawProp(rawProps, "zIndex", sourceProps.zIndex)),
  opacity(convertRawProp(rawProps, "opacity", sourceProps.opacity)),
  foregroundColor(convertRawProp(rawProps, "color", sourceProps.foregroundColor)),
  backgroundColor(convertRawProp(rawProps, "backgroundColor", sourceProps.backgroundColor)) {};

#pragma mark - DebugStringConvertible

SharedDebugStringConvertibleList ViewProps::getDebugProps() const {
  return
    AccessibilityProps::getDebugProps() +
    YogaStylableProps::getDebugProps() +
    SharedDebugStringConvertibleList {
      debugStringConvertibleItem("zIndex", zIndex),
      debugStringConvertibleItem("opacity", opacity),
      debugStringConvertibleItem("foregroundColor", foregroundColor),
      debugStringConvertibleItem("backgroundColor", backgroundColor),
    };
}

} // namespace react
} // namespace facebook
