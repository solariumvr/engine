// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMMON_PLATFORM_VIEW_H_
#define COMMON_PLATFORM_VIEW_H_

#include <memory>

#include "flutter/lib/ui/semantics/semantics_node.h"
#include "flutter/shell/common/engine.h"
#include "flutter/shell/common/shell.h"
#include "flutter/shell/common/surface.h"
#include "flutter/shell/common/vsync_waiter.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "lib/ftl/synchronization/waitable_event.h"
#include "third_party/skia/include/core/SkSize.h"
#include "third_party/skia/include/gpu/GrContext.h"

namespace shell {

class Rasterizer;

class PlatformView {
 public:
  struct SurfaceConfig {
    uint8_t red_bits = 8;
    uint8_t green_bits = 8;
    uint8_t blue_bits = 8;
    uint8_t alpha_bits = 8;
    uint8_t depth_bits = 0;
    uint8_t stencil_bits = 8;
  };

  void SetupResourceContextOnIOThread();

  virtual ~PlatformView();

  void DispatchPlatformMessage(ftl::RefPtr<blink::PlatformMessage> message);
  void DispatchSemanticsAction(int32_t id, blink::SemanticsAction action);
  void SetSemanticsEnabled(bool enabled);

  void NotifyCreated(std::unique_ptr<Surface> surface);

  void NotifyCreated(std::unique_ptr<Surface> surface,
                     ftl::Closure continuation);

  void NotifyDestroyed();

  base::WeakPtr<PlatformView> GetWeakPtr();

  // The VsyncWaiter will live at least as long as the PlatformView.
  virtual VsyncWaiter* GetVsyncWaiter();

  virtual bool ResourceContextMakeCurrent() = 0;

  virtual void UpdateSemantics(std::vector<blink::SemanticsNode> update);
  virtual void HandlePlatformMessage(
      ftl::RefPtr<blink::PlatformMessage> message);

  Rasterizer& rasterizer() { return *rasterizer_; }
  Engine& engine() { return *engine_; }

  virtual void RunFromSource(const std::string& assets_directory,
                             const std::string& main,
                             const std::string& packages) = 0;

 protected:
  explicit PlatformView(std::unique_ptr<Rasterizer> rasterizer);

  void CreateEngine();
  void PostAddToShellTask();

  void SetupResourceContextOnIOThreadPerform(
      ftl::AutoResetWaitableEvent* event);

  SurfaceConfig surface_config_;
  std::unique_ptr<Rasterizer> rasterizer_;
  std::unique_ptr<Engine> engine_;
  std::unique_ptr<VsyncWaiter> vsync_waiter_;
  SkISize size_;

 private:
  base::WeakPtrFactory<PlatformView> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PlatformView);
};

}  // namespace shell

#endif  // COMMON_PLATFORM_VIEW_H_
