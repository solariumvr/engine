// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/darwin/desktop/vsync_waiter_mac.h"

#include <CoreVideo/CoreVideo.h>

#include "flutter/common/threads.h"
#include "base/logging.h"

namespace shell {

#define link_ (reinterpret_cast<CVDisplayLinkRef>(opaque_))

VsyncWaiterMac::VsyncWaiterMac() : opaque_(nullptr) {
  // Create the link.
  CVDisplayLinkRef link = nullptr;
  CVDisplayLinkCreateWithActiveCGDisplays(&link);
  opaque_ = link;

  // Set the output callback.
  CVDisplayLinkSetOutputCallback(
      link_,
      [](CVDisplayLinkRef link, const CVTimeStamp* now,
         const CVTimeStamp* output, CVOptionFlags flags_in,
         CVOptionFlags* flags_out, void* context) -> CVReturn {
        OnDisplayLink(context);
        return kCVReturnSuccess;
      },
      this);
}

VsyncWaiterMac::~VsyncWaiterMac() {
  CVDisplayLinkRelease(link_);
}

void VsyncWaiterMac::OnDisplayLink(void* context) {
  reinterpret_cast<VsyncWaiterMac*>(context)->OnDisplayLink();
}

void VsyncWaiterMac::OnDisplayLink() {
  ftl::TimePoint frame_time = ftl::TimePoint::Now();
  CVDisplayLinkStop(link_);
  auto callback = std::move(callback_);
  callback_ = Callback();

  blink::Threads::UI()->PostTask(
      [callback, frame_time] { callback(frame_time); });
}

void VsyncWaiterMac::AsyncWaitForVsync(Callback callback) {
  DCHECK(!callback_);
  callback_ = std::move(callback);
  CVDisplayLinkStart(link_);
}

}  // namespace shell
