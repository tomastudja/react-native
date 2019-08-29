/**
* Copyright (c) Facebook, Inc. and its affiliates.
*
* This source code is licensed under the MIT license found in the
* LICENSE file in the root directory of this source tree.
*
* @generated by codegen project: GeneratePropsJavaDelegate.js
*/

package com.facebook.react.viewmanagers;

import android.view.View;
import androidx.annotation.Nullable;
import com.facebook.react.bridge.ReadableArray;
import com.facebook.react.uimanager.BaseViewManager;
import com.facebook.react.uimanager.BaseViewManagerDelegate;
import com.facebook.react.uimanager.LayoutShadowNode;

public class AndroidSwipeRefreshLayoutManagerDelegate<T extends View, U extends BaseViewManager<T, ? extends LayoutShadowNode> & AndroidSwipeRefreshLayoutManagerInterface<T>> extends BaseViewManagerDelegate<T, U> {
  public AndroidSwipeRefreshLayoutManagerDelegate(U viewManager) {
    super(viewManager);
  }
  @Override
  public void setProperty(T view, String propName, @Nullable Object value) {
    switch (propName) {
      case "enabled":
        mViewManager.setEnabled(view, value == null ? false : (boolean) value);
        break;
      case "colors":
        mViewManager.setColors(view, (ReadableArray) value);
        break;
      case "progressBackgroundColor":
        mViewManager.setProgressBackgroundColor(view, value == null ? null : ((Double) value).intValue());
        break;
      case "size":
        mViewManager.setSize(view, value == null ? 1 : ((Double) value).intValue());
        break;
      case "progressViewOffset":
        mViewManager.setProgressViewOffset(view, value == null ? 0f : ((Double) value).floatValue());
        break;
      case "refreshing":
        mViewManager.setRefreshing(view, value == null ? false : (boolean) value);
        break;
      default:
        super.setProperty(view, propName, value);
    }
  }
}
