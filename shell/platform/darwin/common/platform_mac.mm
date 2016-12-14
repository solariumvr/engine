// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/darwin/common/platform_mac.h"

#include <Foundation/Foundation.h>

#include <asl.h>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/i18n/icu_util.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/mac/scoped_nsautorelease_pool.h"
#include "base/message_loop/message_loop.h"
#include "base/trace_event/trace_event.h"
#include "dart/runtime/include/dart_tools_api.h"
#include "flutter/common/threads.h"
#include "flutter/runtime/start_up.h"
#include "flutter/shell/common/shell.h"
#include "flutter/shell/common/switches.h"
#include "flutter/shell/common/tracing_controller.h"
#include "flutter/sky/engine/wtf/MakeUnique.h"

namespace shell {

static void InitializeLogging() {
  logging::LoggingSettings settings;
  settings.logging_dest = logging::LOG_TO_SYSTEM_DEBUG_LOG;
  logging::InitLogging(settings);
  logging::SetLogItems(false,   // Process ID
                       false,   // Thread ID
                       false,   // Timestamp
                       false);  // Tick count
}

static void RedirectIOConnectionsToSyslog() {
#if TARGET_OS_IPHONE
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          FlagForSwitch(Switch::NoRedirectToSyslog))) {
    return;
  }

  asl_log_descriptor(NULL, NULL, ASL_LEVEL_INFO, STDOUT_FILENO,
                     ASL_LOG_DESCRIPTOR_WRITE);
  asl_log_descriptor(NULL, NULL, ASL_LEVEL_NOTICE, STDERR_FILENO,
                     ASL_LOG_DESCRIPTOR_WRITE);
#endif
}

class EmbedderState {
 public:
  EmbedderState(int argc, const char* argv[], std::string icu_data_path) {
#if TARGET_OS_IPHONE
    // This calls crashes on MacOS because we haven't run Dart_Initialize yet.
    // See https://github.com/flutter/flutter/issues/4006
    blink::engine_main_enter_ts = Dart_TimelineGetMicros();
#endif
    CHECK([NSThread isMainThread])
        << "Embedder initialization must occur on the main platform thread";

    base::CommandLine::Init(argc, argv);

    RedirectIOConnectionsToSyslog();

    InitializeLogging();

    base::CommandLine& command_line = *base::CommandLine::ForCurrentProcess();
    if (command_line.HasSwitch(FlagForSwitch(Switch::TraceStartup))) {
      // Usually, all tracing within flutter is managed via the tracing
      // controller
      // The tracing controller is accessed via the shell instance. This means
      // that tracing can only be enabled once that instance is created. Traces
      // early in startup are lost. This enables tracing only in base manually
      // till the tracing controller takes over.
      shell::TracingController::StartBaseTracing();
    }

    // This is about as early as tracing of any kind can start. Add an instant
    // marker that can be used as a reference for startup.
    TRACE_EVENT_INSTANT0("flutter", "main", TRACE_EVENT_SCOPE_PROCESS);

    embedder_message_loop_ = WTF::MakeUnique<base::MessageLoopForUI>();

#if TARGET_OS_IPHONE
    // One cannot start the message loop on the platform main thread. Instead,
    // we attach to the CFRunLoop
    embedder_message_loop_->Attach();
#endif

    shell::Shell::InitStandalone(icu_data_path);
  }

  ~EmbedderState() {
#if !TARGET_OS_IPHONE
    embedder_message_loop_.release();
#endif
  }

 private:
  base::AtExitManager exit_manager_;
  std::unique_ptr<base::MessageLoopForUI> embedder_message_loop_;

  DISALLOW_COPY_AND_ASSIGN(EmbedderState);
};

void PlatformMacMain(int argc, const char* argv[], std::string icu_data_path) {
  static std::unique_ptr<EmbedderState> g_embedder;
  static std::once_flag once_main;

  std::call_once(once_main, [&]() {
    g_embedder = WTF::MakeUnique<EmbedderState>(argc, argv, icu_data_path);
  });
}

static bool FlagsValidForCommandLineLaunch(const std::string& bundle_path,
                                           const std::string& main,
                                           const std::string& packages) {
  if (main.empty() || packages.empty() || bundle_path.empty()) {
    return false;
  }

  // Ensure that the paths exists. This catches cases where the user has
  // successfully launched the application from the tooling but has since moved
  // the source files on disk and is launching again directly.

  NSFileManager* manager = [NSFileManager defaultManager];

  if (![manager fileExistsAtPath:@(main.c_str())]) {
    return false;
  }

  if (![manager fileExistsAtPath:@(packages.c_str())]) {
    return false;
  }

  if (![manager fileExistsAtPath:@(bundle_path.c_str())]) {
    return false;
  }

  return true;
}

static std::string ResolveCommandLineLaunchFlag(const char* name) {
  auto command_line = *base::CommandLine::ForCurrentProcess();

  if (command_line.HasSwitch(name)) {
    return command_line.GetSwitchValueASCII(name);
  }

  const char* saved_default =
      [[NSUserDefaults standardUserDefaults] stringForKey:@(name)].UTF8String;

  if (saved_default != NULL) {
    return saved_default;
  }

  return "";
}

bool AttemptLaunchFromCommandLineSwitches(Engine* engine) {
  base::mac::ScopedNSAutoreleasePool pool;

  NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];

  auto command_line = *base::CommandLine::ForCurrentProcess();

  if (command_line.HasSwitch(FlagForSwitch(Switch::FLX)) ||
      command_line.HasSwitch(FlagForSwitch(Switch::MainDartFile)) ||
      command_line.HasSwitch(FlagForSwitch(Switch::Packages))) {
    // The main dart file, flx bundle and the package root must be specified in
    // one go. We dont want to end up in a situation where we take one value
    // from the command line and the others from user defaults. In case, any
    // new flags are specified, forget about all the old ones.
    [defaults removeObjectForKey:@(FlagForSwitch(Switch::FLX))];
    [defaults removeObjectForKey:@(FlagForSwitch(Switch::MainDartFile))];
    [defaults removeObjectForKey:@(FlagForSwitch(Switch::Packages))];

    [defaults synchronize];
  }

  std::string bundle_path =
      ResolveCommandLineLaunchFlag(FlagForSwitch(Switch::FLX));
  std::string main =
      ResolveCommandLineLaunchFlag(FlagForSwitch(Switch::MainDartFile));
  std::string packages =
      ResolveCommandLineLaunchFlag(FlagForSwitch(Switch::Packages));

  if (!FlagsValidForCommandLineLaunch(bundle_path, main, packages)) {
    return false;
  }

  // Save the newly resolved dart main file and the package root to user
  // defaults so that the next time the user launches the application in the
  // simulator without the tooling, the application boots up.
  [defaults setObject:@(bundle_path.c_str())
               forKey:@(FlagForSwitch(Switch::FLX))];
  [defaults setObject:@(main.c_str())
               forKey:@(FlagForSwitch(Switch::MainDartFile))];
  [defaults setObject:@(packages.c_str())
               forKey:@(FlagForSwitch(Switch::Packages))];

  [defaults synchronize];

  blink::Threads::UI()->PostTask(
      [ engine = engine->GetWeakPtr(), bundle_path, main, packages ] {
        if (engine)
          engine->RunBundleAndSource(bundle_path, main, packages);
      });

  return true;
}

}  // namespace shell
