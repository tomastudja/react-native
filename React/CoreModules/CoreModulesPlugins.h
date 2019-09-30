/**
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * @generated by an internal plugin build system
 */

#ifdef RN_DISABLE_OSS_PLUGIN_HEADER

// FB Internal: Plugins.h is autogenerated by the build system.
#import "Plugins.h"

#else

// OSS-compatibility layer

#import <Foundation/Foundation.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type-c-linkage"

#ifdef __cplusplus
extern "C" {
#endif

// RCTTurboModuleManagerDelegate should call this to resolve module classes.
Class RCTCoreModulesClassProvider(const char *name);

// Lookup functions
Class RCTAccessibilityManagerCls(void);
Class RCTAppearanceCls(void);
Class RCTDeviceInfoCls(void);
Class RCTExceptionsManagerCls(void);
Class RCTImageLoaderCls(void);
Class RCTPlatformCls(void);
Class RCTClipboardCls(void);
Class RCTI18nManagerCls(void);
Class RCTSourceCodeCls(void);
Class RCTActionSheetManagerCls(void);

#ifdef __cplusplus
}
#endif

#pragma GCC diagnostic pop

#endif // RN_DISABLE_OSS_PLUGIN_HEADER
