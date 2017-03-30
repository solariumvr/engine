// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_SHELL_H_
#define SHELL_COMMON_SHELL_H_

#include "flutter/fml/thread.h"
#include "flutter/fml/thread_checker.h"
#include "flutter/shell/common/tracing_controller.h"
#include "lib/ftl/command_line.h"
#include "lib/ftl/macros.h"
#include "lib/ftl/memory/ref_ptr.h"
#include "lib/ftl/memory/weak_ptr.h"
#include "lib/ftl/synchronization/waitable_event.h"
#include "lib/ftl/tasks/task_runner.h"

namespace shell {

class PlatformView;
class Rasterizer;

class Shell {
 public:
  ~Shell();

  static void InitStandalone(ftl::CommandLine command_line,
                             std::string icu_data_path = "",
                             std::string application_library_path = "");

  static Shell& Shared();

  const ftl::CommandLine& GetCommandLine() const;

  TracingController& tracing_controller();

  // Maintain a list of rasterizers.
  // These APIs must only be accessed on the GPU thread.
  void AddRasterizer(const ftl::WeakPtr<Rasterizer>& rasterizer);
  void PurgeRasterizers();
  void GetRasterizers(std::vector<ftl::WeakPtr<Rasterizer>>* rasterizer);

  // List of PlatformViews.

  // These APIs must only be accessed on UI thread.
  void AddPlatformView(const ftl::WeakPtr<PlatformView>& platform_view);
  void PurgePlatformViews();
  void GetPlatformViews(
      std::vector<ftl::WeakPtr<PlatformView>>* platform_views);

  struct PlatformViewInfo {
    uintptr_t view_id;
    int64_t isolate_id;
    std::string isolate_name;
  };

  // These APIs can be called from any thread.
  // Return the list of platform view ids at the time of this call.
  void WaitForPlatformViewIds(std::vector<PlatformViewInfo>* platform_view_ids);

  // Attempt to run a script inside a flutter view indicated by |view_id|.
  // Will set |view_existed| to true if the view was found and false otherwise.
  void RunInPlatformView(uintptr_t view_id,
                         const char* main_script,
                         const char* packages_file,
                         const char* asset_directory,
                         bool* view_existed,
                         int64_t* dart_isolate_id,
                         std::string* isolate_name);

 private:
  static void Init(ftl::CommandLine command_line);

  Shell(ftl::CommandLine command_line);

  void InitGpuThread();
  void InitUIThread();

  void WaitForPlatformViewsIdsUIThread(
      std::vector<PlatformViewInfo>* platform_views,
      ftl::AutoResetWaitableEvent* latch);

  void RunInPlatformViewUIThread(uintptr_t view_id,
                                 const std::string& main,
                                 const std::string& packages,
                                 const std::string& assets_directory,
                                 bool* view_existed,
                                 int64_t* dart_isolate_id,
                                 std::string* isolate_name,
                                 ftl::AutoResetWaitableEvent* latch);

  ftl::CommandLine command_line_;

  std::unique_ptr<fml::Thread> gpu_thread_;
  std::unique_ptr<fml::Thread> ui_thread_;
  std::unique_ptr<fml::Thread> io_thread_;

  std::unique_ptr<fml::ThreadChecker> gpu_thread_checker_;
  std::unique_ptr<fml::ThreadChecker> ui_thread_checker_;

  TracingController tracing_controller_;

  std::vector<ftl::WeakPtr<Rasterizer>> rasterizers_;
  std::vector<ftl::WeakPtr<PlatformView>> platform_views_;

  FTL_DISALLOW_COPY_AND_ASSIGN(Shell);
};

}  // namespace shell

#endif  // SHELL_COMMON_SHELL_H_
