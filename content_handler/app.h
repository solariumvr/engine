// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_CONTENT_HANDLER_APP_H_
#define FLUTTER_CONTENT_HANDLER_APP_H_

#include <memory>
#include <unordered_set>

#include "application/lib/app/application_context.h"
#include "application/services/application_runner.fidl.h"
#include "flutter/content_handler/application_controller_impl.h"
#include "flutter/content_handler/content_handler_thread.h"
#include "lib/ftl/macros.h"
#include "lib/ftl/synchronization/waitable_event.h"

namespace flutter_runner {

class App : public app::ApplicationRunner {
 public:
  App();
  ~App();

  static App& Shared();

  // |app::ApplicationRunner| implementation:

  void StartApplication(app::ApplicationPackagePtr application,
                        app::ApplicationStartupInfoPtr startup_info,
                        fidl::InterfaceRequest<app::ApplicationController>
                            controller) override;

  void Destroy(ApplicationControllerImpl* controller);

  struct PlatformViewInfo {
    uintptr_t view_id;
    int64_t isolate_id;
    std::string isolate_name;
  };

  void WaitForPlatformViewIds(std::vector<PlatformViewInfo>* platform_view_ids);

 private:
  void WaitForPlatformViewsIdsUIThread(
    std::vector<PlatformViewInfo>* platform_view_ids,
    ftl::AutoResetWaitableEvent* latch);

  std::unique_ptr<app::ApplicationContext> context_;
  std::unique_ptr<Thread> gpu_thread_;
  std::unique_ptr<Thread> io_thread_;
  fidl::BindingSet<app::ApplicationRunner> runner_bindings_;
  std::unordered_map<ApplicationControllerImpl*,
                     std::unique_ptr<ApplicationControllerImpl>>
      controllers_;

  FTL_DISALLOW_COPY_AND_ASSIGN(App);
};

}  // namespace flutter_runner

#endif  // FLUTTER_CONTENT_HANDLER_APP_H_
