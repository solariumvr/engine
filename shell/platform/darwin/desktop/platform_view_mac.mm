// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/darwin/desktop/platform_view_mac.h"

#include <AppKit/AppKit.h>
#include <Foundation/Foundation.h>

#include "base/command_line.h"
#include "base/trace_event/trace_event.h"
#include "flutter/common/threads.h"
#include "flutter/shell/common/switches.h"
#include "flutter/shell/gpu/gpu_rasterizer.h"
#include "flutter/shell/platform/darwin/common/platform_mac.h"
#include "flutter/shell/platform/darwin/desktop/vsync_waiter_mac.h"
#include "lib/ftl/synchronization/waitable_event.h"


namespace shell {

PlatformViewMac::PlatformViewMac(NSOpenGLView* gl_view)
    : PlatformView(std::make_unique<GPURasterizer>()),
      opengl_view_([gl_view retain]),
      resource_loading_context_([[NSOpenGLContext alloc]
          initWithFormat:gl_view.pixelFormat
            shareContext:gl_view.openGLContext]) {
  CreateEngine();

  NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,
                                                       NSUserDomainMask, YES);
  if (paths.count > 0) {
    shell::Shell::Shared().tracing_controller().set_traces_base_path(
        [[paths objectAtIndex:0] UTF8String]);
  }
}

PlatformViewMac::~PlatformViewMac() = default;

void PlatformViewMac::SetupAndLoadDart() {
  if (AttemptLaunchFromCommandLineSwitches(&engine())) {
    // This attempts launching from an FLX bundle that does not contain a
    // dart snapshot.
    return;
  }

  base::CommandLine& command_line = *base::CommandLine::ForCurrentProcess();

  std::string bundle_path = command_line.GetSwitchValueASCII(switches::kFLX);
  if (!bundle_path.empty()) {
    blink::Threads::UI()->PostTask(
        [ engine = engine().GetWeakPtr(), bundle_path ] {
          if (engine)
            engine->RunBundle(bundle_path);
        });
    return;
  }

  auto args = command_line.GetArgs();
  if (args.size() > 0) {
    std::string main = args[0];
    std::string packages =
        command_line.GetSwitchValueASCII(switches::kPackages);
    blink::Threads::UI()->PostTask(
        [ engine = engine().GetWeakPtr(), main, packages ] {
          if (engine)
            engine->RunBundleAndSource(std::string(), main, packages);
        });
    return;
  }
}

void PlatformViewMac::SetupAndLoadFromSource(
    const std::string& assets_directory,
    const std::string& main,
    const std::string& packages) {
  blink::Threads::UI()->PostTask(
      [ engine = engine().GetWeakPtr(), assets_directory, main, packages ] {
        if (engine)
          engine->RunBundleAndSource(assets_directory, main, packages);
      });
}

intptr_t PlatformViewMac::GLContextFBO() const {
  // Default window bound framebuffer FBO 0.
  return 0;
}

bool PlatformViewMac::GLContextMakeCurrent() {
  TRACE_EVENT0("flutter", "PlatformViewMac::GLContextMakeCurrent");
  if (!IsValid()) {
    return false;
  }

  [opengl_view_.get().openGLContext makeCurrentContext];
  return true;
}

bool PlatformViewMac::GLContextClearCurrent() {
  TRACE_EVENT0("flutter", "PlatformViewMac::GLContextClearCurrent");
  if (!IsValid()) {
    return false;
  }

  [NSOpenGLContext clearCurrentContext];
  return true;
}

bool PlatformViewMac::GLContextPresent() {
  TRACE_EVENT0("flutter", "PlatformViewMac::GLContextPresent");
  if (!IsValid()) {
    return false;
  }

  [opengl_view_.get().openGLContext flushBuffer];
  return true;
}

VsyncWaiter* PlatformViewMac::GetVsyncWaiter() {
  if (!vsync_waiter_)
    vsync_waiter_ = std::make_unique<VsyncWaiterMac>();
  return vsync_waiter_.get();
}

bool PlatformViewMac::ResourceContextMakeCurrent() {
  NSOpenGLContext* context = resource_loading_context_.get();

  if (context == nullptr) {
    return false;
  }

  [context makeCurrentContext];
  return true;
}

bool PlatformViewMac::IsValid() const {
  if (opengl_view_ == nullptr) {
    return false;
  }

  auto context = opengl_view_.get().openGLContext;

  if (context == nullptr) {
    return false;
  }

  return true;
}

void PlatformViewMac::RunFromSource(const std::string& assets_directory,
                                    const std::string& main,
                                    const std::string& packages) {
  auto latch = new ftl::ManualResetWaitableEvent();

  dispatch_async(dispatch_get_main_queue(), ^{
    SetupAndLoadFromSource(assets_directory, main, packages);
    latch->Signal();
  });

  latch->Wait();
  delete latch;
}

}  // namespace shell
