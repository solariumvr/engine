#ifndef SOLARIUM_CHROMIUM_VSYNC_WAITER_H
#define SOLARIUM_CHROMIUM_VSYNC_WAITER_H

#include "flutter/shell/common/vsync_waiter.h"

namespace shell {

class VsyncWaiterGlfw : public VsyncWaiter {
  VsyncWaiterGlfw();

  void AsyncWaitForVsync(Callback callback) override;
};
}

#endif  // SOLARIUM_CHROMIUM_VSYNC_WAITER_H
