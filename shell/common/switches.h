// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_SWITCHES_H_
#define SHELL_COMMON_SWITCHES_H_

namespace shell {

// clang-format off
#ifndef DEF_SWITCHES_START
#define DEF_SWITCHES_START enum class Switch {
#endif
#ifndef DEF_SWITCH
#define DEF_SWITCH(swtch, flag, help) swtch,
#endif
#ifndef DEF_SWITCHES_END
#define DEF_SWITCHES_END Sentinel, } ;
#endif
// clang-format on

DEF_SWITCHES_START
DEF_SWITCH(AotSnapshotPath, "aot-snapshot-path", "Path to the AOT snapshot.")
DEF_SWITCH(AotVmSnapshotData, "vm-snapshot-data", "")
DEF_SWITCH(AotVmSnapshotInstructions, "vm-snapshot-instr", "")
DEF_SWITCH(AotIsolateSnapshotData, "isolate-snapshot-data", "")
DEF_SWITCH(AotIsolateSnapshotInstructions, "isolate-snapshot-instr", "")
DEF_SWITCH(CacheDirPath, "cache-dir-path", "Path to the cache directory.")
DEF_SWITCH(DartFlags,
           "dart-flags",
           "Flags passed directly to the Dart VM without being interpreted "
           "by the Flutter shell.")
DEF_SWITCH(DeviceObservatoryPort,
           "observatory-port",
           "A custom Dart Observatory port. The default is 8181.")
DEF_SWITCH(DisableObservatory,
           "disable-observatory",
           "Disable the Dart Observatory. The observatory is never available "
           "in release mode.")
DEF_SWITCH(DeviceDiagnosticPort,
           "diagnostic-port",
           "A custom diagnostic server port.")
DEF_SWITCH(DisableDiagnostic,
           "disable-diagnostic",
           "Disable the diagnostic server. The diagnostic server is never "
           "available in release mode.")
DEF_SWITCH(EnableDartProfiling,
           "enable-dart-profiling",
           "Enable Dart profiling. Profiling information can be viewed from "
           "the observatory.")
DEF_SWITCH(EndlessTraceBuffer,
           "endless-trace-buffer",
           "Enable an endless trace buffer. The default is a ring buffer. "
           "This is useful when very old events need to viewed. For example, "
           "during application launch. Memory usage will continue to grow "
           "indefinitely however.")
DEF_SWITCH(FLX, "flx", "Specify the the FLX path.")
DEF_SWITCH(Help, "help", "Display this help text.")
DEF_SWITCH(LogTag, "log-tag", "Tag associated with log messages.")
DEF_SWITCH(MainDartFile, "dart-main", "The path to the main Dart file.")
DEF_SWITCH(NonInteractive,
           "non-interactive",
           "Make the shell non-interactive. By default, the shell attempts "
           "to setup a window and create an OpenGL context.")
DEF_SWITCH(NoRedirectToSyslog,
           "no-redirect-to-syslog",
           "On iOS: Don't redirect stdout and stderr to syslog by default. "
           "This is used by the tools to read device logs. However, this can "
           "cause logs to not show up when launched from Xcode.")
DEF_SWITCH(Packages, "packages", "Specify the path to the packages.")
DEF_SWITCH(StartPaused,
           "start-paused",
           "Start the application paused in the Dart debugger.")
DEF_SWITCH(TraceStartup,
           "trace-startup",
           "Trace early application lifecycle. Automatically switches to an "
           "endless trace buffer.")
DEF_SWITCH(UseTestFonts,
           "use-test-fonts",
           "Running tests that layout and measure text will not yield "
           "consistent results across various platforms. Enabling this option "
           "will make font resolution default to the Ahem test font on all "
           "platforms (See https://www.w3.org/Style/CSS/Test/Fonts/Ahem/). "
           "This option is only available on the desktop test shells.")
DEF_SWITCH(RunForever,
           "run-forever",
           "In non-interactive mode, keep the shell running after the Dart "
           "script has completed.")
DEF_SWITCH(DartNonCheckedMode,
           "dart-non-checked-mode",
           "Dart code runs in checked mode when the runtime mode is debug. In "
           "profile and release product modes, the application code is "
           "precompiled and checked mode is unsupported. However, this flag "
           "may be specified if the user wishes to run in the debug product "
           "mode (i.e. with JIT or DBC) with checked mode off.")
DEF_SWITCHES_END

void PrintUsage(const std::string& executable_name);

const char* FlagForSwitch(Switch sw);

}  // namespace shell

#endif  // SHELL_COMMON_SWITCHES_H_
