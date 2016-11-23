// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_PLATFORM_MAC_PLATFORM_VIEW_MAC_H_
#define SHELL_PLATFORM_MAC_PLATFORM_VIEW_MAC_H_

#include "base/mac/scoped_nsobject.h"
#include "flutter/shell/common/platform_view.h"
#include "flutter/shell/gpu/gpu_surface_gl.h"
#include "base/memory/weak_ptr.h"

@class NSOpenGLView;
@class NSOpenGLContext;

namespace shell {

class PlatformViewMac : public PlatformView, public GPUSurfaceGLDelegate {
 public:
  PlatformViewMac(NSOpenGLView* gl_view);

  ~PlatformViewMac() override;

  void SetupAndLoadDart();

  bool GLContextMakeCurrent() override;

  bool GLContextClearCurrent() override;

  bool GLContextPresent() override;

  intptr_t GLContextFBO() const override;

  VsyncWaiter* GetVsyncWaiter() override;

  bool ResourceContextMakeCurrent() override;

  void RunFromSource(const std::string& assets_directory,
                     const std::string& main,
                     const std::string& packages) override;

 private:
  base::scoped_nsobject<NSOpenGLView> opengl_view_;
  base::scoped_nsobject<NSOpenGLContext> resource_loading_context_;

  bool IsValid() const;

  void SetupAndLoadFromSource(const std::string& assets_directory,
                              const std::string& main,
                              const std::string& packages);

  DISALLOW_COPY_AND_ASSIGN(PlatformViewMac);
};

}  // namespace shell

#endif  // SHELL_PLATFORM_MAC_PLATFORM_VIEW_MAC_H_
