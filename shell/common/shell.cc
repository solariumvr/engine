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
#include "lib/ftl/files/unique_fd.h"
#include "flutter/shell/platform/linux/message_pump_glfw.h"

namespace shell {
namespace {

static Shell* g_shell = nullptr;

bool IsInvalid(const base::WeakPtr<Rasterizer>& rasterizer) {
  return !rasterizer;
}

bool IsViewInvalid(const base::WeakPtr<PlatformView>& platform_view) {
  return !platform_view;
}
//
//class NonDiscardableMemory : public base::DiscardableMemory {
// public:
//  explicit NonDiscardableMemory(size_t size) : data_(new uint8_t[size]) {}
//  bool Lock() override { return false; }
//  void Unlock() override {}
//  void* data() const override { return data_.get(); }
//
// private:
//  std::unique_ptr<uint8_t[]> data_;
//};
//
//class NonDiscardableMemoryAllocator : public base::DiscardableMemoryAllocator {
// public:
//  std::unique_ptr<base::DiscardableMemory> AllocateLockedDiscardableMemory(
//      size_t size) override {
//    return std::make_unique<NonDiscardableMemory>(size);
//  }
//};

//base::LazyInstance<NonDiscardableMemoryAllocator> g_discardable;

void ServiceIsolateHook(bool running_precompiled) {
  if (!running_precompiled) {
    const blink::Settings& settings = blink::Settings::Get();
    if (settings.enable_observatory)
      DiagnosticServer::Start();
  }
}

}  // namespace

Shell::Shell() {
  DCHECK(!g_shell);

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

Shell::~Shell() {
}

void Shell::InitStandalone(std::string icu_data_path) {
  TRACE_EVENT0("flutter", "Shell::InitStandalone");

  //ftl::UniqueFD icu_fd(
  //    icu_data_path.empty() ? -1 : HANDLE_EINTR(::open(icu_data_path.c_str(),
  //                                                     O_RDONLY)));
  //if (icu_fd.get() == -1) {
  //  // If the embedder did not specify a valid file, fallback to looking through
  //  // internal search paths.
  //  FTL_CHECK(base::i18n::InitializeICU());
  //} else {
  //  FTL_CHECK(base::i18n::InitializeICUWithFileDescriptor(
  //      icu_fd.get(), base::MemoryMappedFile::Region::kWholeFile));
  //  icu_fd.reset();
  //}

  base::CommandLine& command_line = *base::CommandLine::ForCurrentProcess();

  blink::Settings settings;

  // Enable Observatory
  settings.enable_observatory =
      !command_line.HasSwitch(FlagForSwitch(Switch::DisableObservatory));

  // Set Observatory Port
  if (command_line.HasSwitch(FlagForSwitch(Switch::DeviceObservatoryPort))) {
    auto port_string = command_line.GetSwitchValueASCII(
        FlagForSwitch(Switch::DeviceObservatoryPort));
    std::stringstream stream(port_string);
    uint32_t port = 0;
    if (stream >> port) {
      settings.observatory_port = port;
    } else {
      LOG(INFO)
          << "Observatory port specified was malformed. Will default to "
          << settings.observatory_port;
    }
  }

  settings.start_paused =
      command_line.HasSwitch(FlagForSwitch(Switch::StartPaused));

  settings.enable_dart_profiling =
      command_line.HasSwitch(FlagForSwitch(Switch::EnableDartProfiling));

  settings.endless_trace_buffer =
      command_line.HasSwitch(FlagForSwitch(Switch::EndlessTraceBuffer));

  settings.trace_startup =
      command_line.HasSwitch(FlagForSwitch(Switch::TraceStartup));

  settings.aot_snapshot_path =
      command_line.GetSwitchValueASCII(FlagForSwitch(Switch::AotSnapshotPath));

  settings.aot_isolate_snapshot_file_name = command_line.GetSwitchValueASCII(
      FlagForSwitch(Switch::AotIsolateSnapshot));

  settings.aot_vm_isolate_snapshot_file_name = command_line.GetSwitchValueASCII(
      FlagForSwitch(Switch::AotVmIsolateSnapshot));

  settings.aot_instructions_blob_file_name = command_line.GetSwitchValueASCII(
      FlagForSwitch(Switch::AotInstructionsBlob));

  settings.aot_rodata_blob_file_name =
      command_line.GetSwitchValueASCII(FlagForSwitch(Switch::AotRodataBlob));

  settings.temp_directory_path =
      command_line.GetSwitchValueASCII(FlagForSwitch(Switch::CacheDirPath));

  if (command_line.HasSwitch(FlagForSwitch(Switch::DartFlags))) {
		auto dart_flags = command_line.GetSwitchValueNative(FlagForSwitch(Switch::DartFlags));
		std::stringstream stream(
			std::string(dart_flags.begin(), dart_flags.end()));
    std::istream_iterator<std::string> end;
    for (std::istream_iterator<std::string> it(stream); it != end; ++it)
      settings.dart_flags.push_back(*it);
  }

  blink::Settings::Set(settings);

  Init();
}

void Shell::Init() {
  //base::DiscardableMemoryAllocator::SetInstance(&g_discardable.Get());

#if FLUTTER_RUNTIME_MODE != FLUTTER_RUNTIME_MODE_RELEASE
  InitSkiaEventTracer();
#endif

  DCHECK(!g_shell);
  g_shell = new Shell();
  blink::Threads::UI()->PostTask(Engine::Init);
}

Shell& Shell::Shared() {
  DCHECK(g_shell);
  return *g_shell;
}

//TracingController& Shell::tracing_controller() {
//  return tracing_controller_;
//}

void Shell::InitGpuThread() {
  gpu_thread_checker_.reset(new base::ThreadChecker());
}

void Shell::InitUIThread() {
  ui_thread_checker_.reset(new base::ThreadChecker());
}

void Shell::AddRasterizer(const base::WeakPtr<Rasterizer>& rasterizer) {
  DCHECK(gpu_thread_checker_ && gpu_thread_checker_->CalledOnValidThread());
  rasterizers_.push_back(rasterizer);
}

void Shell::PurgeRasterizers() {
  DCHECK(gpu_thread_checker_ && gpu_thread_checker_->CalledOnValidThread());
  rasterizers_.erase(
      std::remove_if(rasterizers_.begin(), rasterizers_.end(), IsInvalid),
      rasterizers_.end());
}

void Shell::GetRasterizers(std::vector<base::WeakPtr<Rasterizer>>* rasterizers) {
  DCHECK(gpu_thread_checker_ && gpu_thread_checker_->CalledOnValidThread());
  *rasterizers = rasterizers_;
}

void Shell::AddPlatformView(const base::WeakPtr<PlatformView>& platform_view) {
  DCHECK(ui_thread_checker_ && ui_thread_checker_->CalledOnValidThread());
  if (platform_view) {
    platform_views_.push_back(platform_view);
  }
}

void Shell::PurgePlatformViews() {
  DCHECK(ui_thread_checker_ && ui_thread_checker_->CalledOnValidThread());
  platform_views_.erase(std::remove_if(platform_views_.begin(),
                                       platform_views_.end(), IsViewInvalid),
                        platform_views_.end());
}

void Shell::GetPlatformViews(
    std::vector<base::WeakPtr<PlatformView>>* platform_views) {
  DCHECK(ui_thread_checker_ && ui_thread_checker_->CalledOnValidThread());
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
  DCHECK(view_id != 0);
  DCHECK(main_script);
  DCHECK(packages_file);
  DCHECK(asset_directory);
  DCHECK(view_existed);

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
  DCHECK(ui_thread_checker_ && ui_thread_checker_->CalledOnValidThread());

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
