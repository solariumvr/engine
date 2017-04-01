// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/common/shell.h"

#include <fcntl.h>
#include <memory>
#include <sstream>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/i18n/icu_util.h"
#include "base/lazy_instance.h"
#include "base/memory/discardable_memory.h"
#include "base/memory/discardable_memory_allocator.h"
#include "base/posix/eintr_wrapper.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread.h"
#include "base/trace_event/trace_event.h"
#include "dart/runtime/include/dart_tools_api.h"
#include "flutter/common/settings.h"
#include "flutter/common/threads.h"
#include "flutter/glue/task_runner_adaptor.h"
#include "flutter/runtime/dart_init.h"
#include "flutter/shell/common/diagnostic/diagnostic_server.h"
#include "flutter/shell/common/engine.h"
#include "flutter/shell/common/platform_view_service_protocol.h"
#include "flutter/shell/common/skia_event_tracer_impl.h"
#include "flutter/shell/common/switches.h"
#include "flutter/shell/platform/win/message_pump_glfw.h"
#include "lib/ftl/files/unique_fd.h"

namespace shell {
namespace {

static Shell* g_shell = nullptr;

bool IsInvalid(const base::WeakPtr<Rasterizer>& rasterizer) {
  return !rasterizer;
}

bool IsViewInvalid(const base::WeakPtr<PlatformView>& platform_view) {
  return !platform_view;
}

template <typename T>
bool GetSwitchValue(const ftl::CommandLine& command_line,
                    Switch sw,
                    T* result) {
  std::string switch_string;

  if (!command_line.GetOptionValue(FlagForSwitch(sw), &switch_string)) {
    return false;
  }

  std::stringstream stream(switch_string);
  T value = 0;
  if (stream >> value) {
    *result = value;
    return true;
  }

  return false;
}

void ServiceIsolateHook(bool running_precompiled) {
  if (!running_precompiled) {
    const blink::Settings& settings = blink::Settings::Get();
    if (settings.enable_diagnostic)
      DiagnosticServer::Start(settings.diagnostic_port);
  }
}

}  // namespace

Shell::Shell(ftl::CommandLine command_line)
    : command_line_(std::move(command_line)) {
  FTL_DCHECK(!g_shell);

  base::Thread::Options options;
  base::Thread::Options platform_options;
  platform_options.message_pump_factory = base::Bind(shell::MessagePumpGLFW::Create);

  platform_thread_.reset(new base::Thread("platform_thread"));
  platform_thread_->StartWithOptions(platform_options);

  gpu_thread_.reset(new base::Thread("gpu_thread"));
  gpu_thread_->StartWithOptions(options);

  ui_thread_.reset(new base::Thread("ui_thread"));
  ui_thread_->StartWithOptions(options);

  io_thread_.reset(new base::Thread("io_thread"));
  io_thread_->StartWithOptions(options);

	blink::Threads threads(ftl::MakeRefCounted<glue::TaskRunnerAdaptor>(
														base::MessageLoop::current()->task_runner()),
                         ftl::MakeRefCounted<glue::TaskRunnerAdaptor>(
                             platform_thread_->message_loop()->task_runner()),
                         ftl::MakeRefCounted<glue::TaskRunnerAdaptor>(
                             gpu_thread_->message_loop()->task_runner()),
                         ftl::MakeRefCounted<glue::TaskRunnerAdaptor>(
                             ui_thread_->message_loop()->task_runner()),
                         ftl::MakeRefCounted<glue::TaskRunnerAdaptor>(
                             io_thread_->message_loop()->task_runner()));
  blink::Threads::Set(threads);

  blink::Threads::Gpu()->PostTask([this]() { InitGpuThread(); });
  blink::Threads::UI()->PostTask([this]() { InitUIThread(); });

  blink::SetServiceIsolateHook(ServiceIsolateHook);
  blink::SetRegisterNativeServiceProtocolExtensionHook(
      PlatformViewServiceProtocol::RegisterHook);
}

Shell::~Shell() {}

void Shell::InitStandalone(ftl::CommandLine command_line,
                           std::string icu_data_path,
                           std::string application_library_path) {
  TRACE_EVENT0("flutter", "Shell::InitStandalone");

  //TODO:SOLARIUM We aren't using this right now.
//  fml::icu::InitializeICU(icu_data_path);

  blink::Settings settings;
  settings.application_library_path = application_library_path;

  // Enable Observatory
  settings.enable_observatory =
      !command_line.HasOption(FlagForSwitch(Switch::DisableObservatory));

  // Set Observatory Port
  if (command_line.HasOption(FlagForSwitch(Switch::DeviceObservatoryPort))) {
    if (!GetSwitchValue(command_line, Switch::DeviceObservatoryPort,
                        &settings.observatory_port)) {
      FTL_LOG(INFO)
          << "Observatory port specified was malformed. Will default to "
          << settings.observatory_port;
    }
  }

  // Checked mode overrides.
  settings.dart_non_checked_mode =
      command_line.HasOption(FlagForSwitch(Switch::DartNonCheckedMode));

  settings.enable_diagnostic =
      !command_line.HasOption(FlagForSwitch(Switch::DisableDiagnostic));

  if (command_line.HasOption(FlagForSwitch(Switch::DeviceDiagnosticPort))) {
    if (!GetSwitchValue(command_line, Switch::DeviceDiagnosticPort,
                        &settings.diagnostic_port)) {
      FTL_LOG(INFO)
          << "Diagnostic port specified was malformed. Will default to "
          << settings.diagnostic_port;
    }
  }

  settings.start_paused =
      command_line.HasOption(FlagForSwitch(Switch::StartPaused));

  settings.enable_dart_profiling =
      command_line.HasOption(FlagForSwitch(Switch::EnableDartProfiling));

  settings.endless_trace_buffer =
      command_line.HasOption(FlagForSwitch(Switch::EndlessTraceBuffer));

  settings.trace_startup =
      command_line.HasOption(FlagForSwitch(Switch::TraceStartup));

  command_line.GetOptionValue(FlagForSwitch(Switch::AotSnapshotPath),
                              &settings.aot_snapshot_path);

  command_line.GetOptionValue(FlagForSwitch(Switch::AotVmSnapshotData),
                              &settings.aot_vm_snapshot_data_filename);

  command_line.GetOptionValue(FlagForSwitch(Switch::AotVmSnapshotInstructions),
                              &settings.aot_vm_snapshot_instr_filename);

  command_line.GetOptionValue(FlagForSwitch(Switch::AotIsolateSnapshotData),
                              &settings.aot_isolate_snapshot_data_filename);

  command_line.GetOptionValue(
      FlagForSwitch(Switch::AotIsolateSnapshotInstructions),
      &settings.aot_isolate_snapshot_instr_filename);

  command_line.GetOptionValue(FlagForSwitch(Switch::CacheDirPath),
                              &settings.temp_directory_path);

  settings.use_test_fonts =
      command_line.HasOption(FlagForSwitch(Switch::UseTestFonts));

  std::string all_dart_flags;
  if (command_line.GetOptionValue(FlagForSwitch(Switch::DartFlags),
                                  &all_dart_flags)) {
    std::stringstream stream(all_dart_flags);
    std::istream_iterator<std::string> end;
    for (std::istream_iterator<std::string> it(stream); it != end; ++it)
      settings.dart_flags.push_back(*it);
  }

  command_line.GetOptionValue(FlagForSwitch(Switch::LogTag), &settings.log_tag);

  blink::Settings::Set(settings);

  Init(std::move(command_line));
}

void Shell::Init(ftl::CommandLine command_line) {
#if FLUTTER_RUNTIME_MODE != FLUTTER_RUNTIME_MODE_RELEASE
  InitSkiaEventTracer();
#endif

  FTL_DCHECK(!g_shell);
  g_shell = new Shell(std::move(command_line));
  blink::Threads::UI()->PostTask(Engine::Init);
}

Shell& Shell::Shared() {
  FTL_DCHECK(g_shell);
  return *g_shell;
}

const ftl::CommandLine& Shell::GetCommandLine() const {
  return command_line_;
}

TracingController& Shell::tracing_controller() {
  return tracing_controller_;
}

void Shell::InitGpuThread() {
  gpu_thread_checker_.reset(new base::ThreadChecker());
}

void Shell::InitUIThread() {
  ui_thread_checker_.reset(new base::ThreadChecker());
}

void Shell::AddRasterizer(const base::WeakPtr<Rasterizer>& rasterizer) {
  FTL_DCHECK(gpu_thread_checker_ &&
             gpu_thread_checker_->CalledOnValidThread());
  rasterizers_.push_back(rasterizer);
}

void Shell::PurgeRasterizers() {
  FTL_DCHECK(gpu_thread_checker_ &&
             gpu_thread_checker_->CalledOnValidThread());
  rasterizers_.erase(
      std::remove_if(rasterizers_.begin(), rasterizers_.end(), IsInvalid),
      rasterizers_.end());
}

void Shell::GetRasterizers(std::vector<base::WeakPtr<Rasterizer>>* rasterizers) {
  FTL_DCHECK(gpu_thread_checker_ &&
             gpu_thread_checker_->CalledOnValidThread());
  *rasterizers = rasterizers_;
}

void Shell::AddPlatformView(const base::WeakPtr<PlatformView>& platform_view) {
  FTL_DCHECK(ui_thread_checker_ && ui_thread_checker_->CalledOnValidThread());
  if (platform_view) {
    platform_views_.push_back(platform_view);
  }
}

void Shell::PurgePlatformViews() {
  FTL_DCHECK(ui_thread_checker_ && ui_thread_checker_->CalledOnValidThread());
  platform_views_.erase(std::remove_if(platform_views_.begin(),
                                       platform_views_.end(), IsViewInvalid),
                        platform_views_.end());
}

void Shell::GetPlatformViews(
    std::vector<base::WeakPtr<PlatformView>>* platform_views) {
  FTL_DCHECK(ui_thread_checker_ && ui_thread_checker_->CalledOnValidThread());
  *platform_views = platform_views_;
}

void Shell::WaitForPlatformViewIds(
    std::vector<PlatformViewInfo>* platform_view_ids) {
  ftl::AutoResetWaitableEvent latch;

  blink::Threads::UI()->PostTask([this, platform_view_ids, &latch]() {
    WaitForPlatformViewsIdsUIThread(platform_view_ids, &latch);
  });

  latch.Wait();
}

void Shell::WaitForPlatformViewsIdsUIThread(
    std::vector<PlatformViewInfo>* platform_view_ids,
    ftl::AutoResetWaitableEvent* latch) {
  std::vector<base::WeakPtr<PlatformView>> platform_views;
  GetPlatformViews(&platform_views);
  for (auto it = platform_views.begin(); it != platform_views.end(); it++) {
    PlatformView* view = it->get();
    if (!view) {
      // Skip dead views.
      continue;
    }
    PlatformViewInfo info;
    info.view_id = reinterpret_cast<uintptr_t>(view);
    info.isolate_id = view->engine().GetUIIsolateMainPort();
    info.isolate_name = view->engine().GetUIIsolateName();
    platform_view_ids->push_back(info);
  }
  latch->Signal();
}

void Shell::RunInPlatformView(uintptr_t view_id,
                              const char* main_script,
                              const char* packages_file,
                              const char* asset_directory,
                              bool* view_existed,
                              int64_t* dart_isolate_id,
                              std::string* isolate_name) {
  ftl::AutoResetWaitableEvent latch;
  FTL_DCHECK(view_id != 0);
  FTL_DCHECK(main_script);
  FTL_DCHECK(packages_file);
  FTL_DCHECK(asset_directory);
  FTL_DCHECK(view_existed);

  blink::Threads::UI()->PostTask([this, view_id, main_script, packages_file,
                                  asset_directory, view_existed,
                                  dart_isolate_id, isolate_name, &latch]() {
    RunInPlatformViewUIThread(view_id, main_script, packages_file,
                              asset_directory, view_existed, dart_isolate_id,
                              isolate_name, &latch);
  });
  latch.Wait();
}

void Shell::RunInPlatformViewUIThread(uintptr_t view_id,
                                      const std::string& main,
                                      const std::string& packages,
                                      const std::string& assets_directory,
                                      bool* view_existed,
                                      int64_t* dart_isolate_id,
                                      std::string* isolate_name,
                                      ftl::AutoResetWaitableEvent* latch) {
  FTL_DCHECK(ui_thread_checker_ && ui_thread_checker_->CalledOnValidThread());

  *view_existed = false;

  for (auto it = platform_views_.begin(); it != platform_views_.end(); it++) {
    PlatformView* view = it->get();
    if (reinterpret_cast<uintptr_t>(view) == view_id) {
      *view_existed = true;
      view->RunFromSource(assets_directory, main, packages);
      *dart_isolate_id = view->engine().GetUIIsolateMainPort();
      *isolate_name = view->engine().GetUIIsolateName();
      break;
    }
  }

  latch->Signal();
}

}  // namespace shell
