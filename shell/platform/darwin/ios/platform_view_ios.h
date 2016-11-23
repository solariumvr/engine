// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_PLATFORM_IOS_PLATFORM_VIEW_IOS_H_
#define SHELL_PLATFORM_IOS_PLATFORM_VIEW_IOS_H_

#include <memory>

#include "base/mac/scoped_nsobject.h"
#include "flutter/shell/common/platform_view.h"
#include "flutter/shell/gpu/gpu_surface_gl.h"
#include "flutter/shell/platform/darwin/ios/framework/Source/accessibility_bridge.h"
#include "flutter/shell/platform/darwin/ios/framework/Source/platform_message_router.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"

@class CAEAGLLayer;
@class UIView;

namespace shell {

class IOSGLContext;

class PlatformViewIOS : public PlatformView, public GPUSurfaceGLDelegate {
 public:
  explicit PlatformViewIOS(CAEAGLLayer* layer);

  ~PlatformViewIOS() override;

  void ToggleAccessibility(UIView* view, bool enabled);

  PlatformMessageRouter& platform_message_router() {
    return platform_message_router_;
  }

  base::WeakPtr<PlatformViewIOS> GetWeakPtr();

  void UpdateSurfaceSize();

  VsyncWaiter* GetVsyncWaiter() override;

  bool ResourceContextMakeCurrent() override;

  bool GLContextMakeCurrent() override;

  bool GLContextClearCurrent() override;

  bool GLContextPresent() override;

  intptr_t GLContextFBO() const override;

  void HandlePlatformMessage(
      ftl::RefPtr<blink::PlatformMessage> message) override;

  void UpdateSemantics(std::vector<blink::SemanticsNode> update) override;

  void RunFromSource(const std::string& assets_directory,
                     const std::string& main,
                     const std::string& packages) override;

 private:
  std::unique_ptr<IOSGLContext> context_;
  PlatformMessageRouter platform_message_router_;
  std::unique_ptr<AccessibilityBridge> accessibility_bridge_;
  base::WeakPtrFactory<PlatformViewIOS> weak_factory_;

  void SetupAndLoadFromSource(const std::string& assets_directory,
                              const std::string& main,
                              const std::string& packages);

  DISALLOW_COPY_AND_ASSIGN(PlatformViewIOS);
};

}  // namespace shell

#endif  // SHELL_PLATFORM_IOS_PLATFORM_VIEW_IOS_H_
