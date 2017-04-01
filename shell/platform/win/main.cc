// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "dart/runtime/bin/embedded_dart_io.h"
#include "base/at_exit.h"
#include "base/basictypes.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "flutter/common/threads.h"
#include "flutter/shell/common/shell.h"
#include "flutter/shell/common/switches.h"
#include "flutter/shell/gpu/gpu_surface_gl.h"
#include "flutter/shell/platform/win/message_pump_glfw.h"
#include "flutter/shell/platform/win/platform_view_glfw.h"
#include "flutter/shell/testing/testing.h"

namespace {

int RunNonInteractive(ftl::CommandLine initial_command_line) {
  base::MessageLoop message_loop;

  shell::Shell::InitStandalone(initial_command_line);

  if (!shell::InitForTesting(initial_command_line)) {
    shell::PrintUsage("sky_shell");
    return 1;
  }

  base::RunLoop().Run();
  // message_loop.Run();
  return 0;
}

static bool IsDartFile(const std::string& path) {
  std::string dart_extension = ".dart";
  return path.rfind(dart_extension) == (path.size() - dart_extension.size());
}

int RunInteractive(ftl::CommandLine initial_command_line) {
  base::CommandLine& command_line = *base::CommandLine::ForCurrentProcess();

  base::MessageLoop message_loop(shell::MessagePumpGLFW::Create());

  shell::Shell::InitStandalone(initial_command_line);

  std::string bundle =
          "C:/Projects/VR/solarium-chromium/src/solarium/lib/navigator/package.zip";

  blink::Threads::Platform()->PostTask([&]() {
    std::string target = command_line.GetSwitchValueASCII(
        shell::FlagForSwitch(shell::Switch::FLX));

    if (target.empty()) {
      // Alternatively, use the first positional argument.
      auto args = command_line.GetArgs();
      if (args.empty())
        return 1;
      target = std::string(args[0].begin(), args[0].end());
    }

    if (target.empty())
      return 1;

    std::unique_ptr<shell::PlatformViewGLFW> platform_view(
        new shell::PlatformViewGLFW());

    platform_view->NotifyCreated(
        std::make_unique<shell::GPUSurfaceGL>(platform_view.get()));

    blink::Threads::UI()->PostTask(
        [ engine = platform_view->engine().GetWeakPtr(), target, bundle ] {
          if (engine) {
            if (IsDartFile(target)) {
              engine->RunBundleAndSource(bundle, target, std::string());

            } else {
              engine->RunBundle(target);
            }
          }
        });

    base::RunLoop().Run();
    platform_view->NotifyDestroyed();

  });
  // blink::Threads::Platform()->PostDelayedTask([]() {
  //	base::MessageLoop::current()->QuitNow();
  //}, ftl::TimeDelta::FromSeconds(4));
  base::RunLoop().Run();

  return 0;
}

}  // namespace

int main(int argc, char* argv[]) {
  dart::bin::SetExecutableName(argv[0]);
  dart::bin::SetExecutableArguments(argc - 1, argv);

  auto command_line = ftl::CommandLineFromArgcArgv(argc, argv);

  base::AtExitManager exit_manager;
  base::CommandLine::Init(argc, argv);


  logging::SetLogItems(true,  // Process ID
                       true,  // Thread ID
                       true,  // Timestamp
                       true   // Tick ount
                       );

  if (command_line.HasOption(shell::FlagForSwitch(shell::Switch::Help))) {
    shell::PrintUsage("sky_shell");
    return 0;
  }

  if (command_line.HasOption(
          shell::FlagForSwitch(shell::Switch::NonInteractive))) {
    return RunNonInteractive(std::move(command_line));
  }

  return RunInteractive(std::move(command_line));
}