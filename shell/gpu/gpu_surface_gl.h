// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_GPU_GPU_SURFACE_GL_H_
#define SHELL_GPU_GPU_SURFACE_GL_H_

#include "flutter/shell/common/surface.h"
#include "flutter/synchronization/debug_thread_checker.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"

namespace shell {

class GPUSurfaceGLDelegate {
 public:
  virtual bool GLContextMakeCurrent() = 0;

  virtual bool GLContextClearCurrent() = 0;

  virtual bool GLContextPresent() = 0;

  virtual intptr_t GLContextFBO() const = 0;
};

class GPUSurfaceGL : public Surface {
 public:
  GPUSurfaceGL(GPUSurfaceGLDelegate* delegate);

  ~GPUSurfaceGL() override;

  bool Setup() override;

  bool IsValid() override;

  std::unique_ptr<SurfaceFrame> AcquireFrame(const SkISize& size) override;

  GrContext* GetContext() override;

 private:
  GPUSurfaceGLDelegate* delegate_;
  sk_sp<GrContext> context_;
  sk_sp<SkSurface> cached_surface_;
  base::WeakPtrFactory<GPUSurfaceGL> weak_factory_;

  sk_sp<SkSurface> CreateSurface(const SkISize& size);

  sk_sp<SkSurface> AcquireSurface(const SkISize& size);

  bool PresentSurface(SkCanvas* canvas);

  bool SelectPixelConfig(GrPixelConfig* config);

  DISALLOW_COPY_AND_ASSIGN(GPUSurfaceGL);
};

class GPUSurfaceFrameGL : public SurfaceFrame {
 public:
  using SubmitCallback = std::function<bool(SkCanvas* canvas)>;

  GPUSurfaceFrameGL(sk_sp<SkSurface> surface, SubmitCallback submit_callback);

  ~GPUSurfaceFrameGL();

  SkCanvas* SkiaCanvas() override;

 private:
  FLUTTER_THREAD_CHECKER_DECLARE(checker_);
  sk_sp<SkSurface> surface_;
  SubmitCallback submit_callback_;

  bool PerformSubmit() override;

  DISALLOW_COPY_AND_ASSIGN(GPUSurfaceFrameGL);
};

}  // namespace shell

#endif  // SHELL_GPU_GPU_SURFACE_GL_H_
