// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fcntl.h>

#include <iostream>
#include <set>
#include <string>

#include "dart/runtime/include/dart_api.h"
#include "dart/runtime/include/dart_native_api.h"
#include "dart/runtime/include/dart_tools_api.h"
#include "lib/ftl/arraysize.h"
#include "lib/ftl/command_line.h"
#include "lib/ftl/files/directory.h"
#include "lib/ftl/files/eintr_wrapper.h"
#include "lib/ftl/files/file_descriptor.h"
#include "lib/ftl/files/file.h"
#include "lib/ftl/files/symlink.h"
#include "lib/ftl/files/unique_fd.h"
#include "base/logging.h"
#include "lib/tonic/converter/dart_converter.h"
#include "lib/tonic/file_loader/file_loader.h"

extern "C" {
extern const uint8_t kVmIsolateSnapshot[];
extern const uint8_t kIsolateSnapshot[];
}

namespace {

using tonic::ToDart;

constexpr char kHelp[] = "help";
constexpr char kPackages[] = "packages";
constexpr char kSnapshot[] = "snapshot";
constexpr char kDepfile[] = "depfile";
constexpr char kBuildOutput[] = "build-output";
constexpr char kPrintDeps[] = "print-deps";
constexpr char kCompileAll[] = "compile-all";

const char* kDartArgs[] = {
    // clang-format off
    "--enable_mirrors=false",
    "--load_deferred_eagerly=true",
    "--conditional_directives",
    // TODO(chinmaygarde): The experimental interpreter for iOS device targets
    // does not support all these flags. The build process uses its own version
    // of this snapshotter. Till support for all these flags is added, make
    // sure the snapshotter does not error out on unrecognized flags.
    "--ignore-unrecognized-flags",
    // clang-format on
};

void Usage() {
  std::cerr
      << "Usage: sky_snapshot --" << kPackages << "=PACKAGES" << std::endl
      << "                  [ --" << kSnapshot << "=OUTPUT_SNAPSHOT ]"
      << std::endl
      << "                  [ --" << kDepfile << "=DEPFILE ]" << std::endl
      << "                  [ --" << kBuildOutput << "=BUILD_OUTPUT ]"
      << std::endl
      << std::endl
      << "                        MAIN_DART" << std::endl
      << " * PACKAGES is the '.packages' file that defines where to find Dart "
         "packages."
      << std::endl
      << " * OUTPUT_SNAPSHOT is the file to write the snapshot into."
      << std::endl
      << " * DEPFILE is the file into which to write the '.d' depedendency "
         "information into."
      << std::endl
      << " * BUILD_OUTPUT determines the target name used in the " << std::endl
      << "   DEPFILE. (Required if DEPFILE is provided.) " << std::endl;
}

class DartScope {
 public:
  DartScope(Dart_Isolate isolate) {
    Dart_EnterIsolate(isolate);
    Dart_EnterScope();
  }

  ~DartScope() {
    Dart_ExitScope();
    Dart_ExitIsolate();
  }
};

void InitDartVM() {
  CHECK(Dart_SetVMFlags(arraysize(kDartArgs), kDartArgs));
  Dart_InitializeParams params = {};
  params.version = DART_INITIALIZE_PARAMS_CURRENT_VERSION;
  params.vm_isolate_snapshot = kVmIsolateSnapshot;
  char* error = Dart_Initialize(&params);
  if (error)
    LOG(FATAL) << error;
}

Dart_Isolate CreateDartIsolate() {
  char* error = nullptr;
  Dart_Isolate isolate =
      Dart_CreateIsolate("dart:snapshot", "main", kIsolateSnapshot,
                         nullptr, nullptr, &error);
  CHECK(isolate) << error;
  Dart_ExitIsolate();
  return isolate;
}

tonic::FileLoader* g_loader = nullptr;

tonic::FileLoader& GetLoader() {
  if (!g_loader)
    g_loader = new tonic::FileLoader();
  return *g_loader;
}

Dart_Handle HandleLibraryTag(Dart_LibraryTag tag,
                             Dart_Handle library,
                             Dart_Handle url) {
  return GetLoader().HandleLibraryTag(tag, library, url);
}

static const char StubNativeFunctionName[] = "StubNativeFunction";

void StubNativeFunction(Dart_NativeArguments arguments) {
  // This is a stub function for the resolver
  Dart_SetReturnValue(
      arguments, Dart_NewApiError("<EMBEDDER DID NOT SETUP NATIVE RESOLVER>"));
}

static Dart_NativeFunction StubNativeLookup(Dart_Handle name,
                                            int argument_count,
                                            bool* auto_setup_scope) {
  return &StubNativeFunction;
}

static const uint8_t* StubNativeSymbol(Dart_NativeFunction nf) {
  return reinterpret_cast<const uint8_t*>(StubNativeFunctionName);
}

// Registers a dummy native symbol resolver on all loaded libraries that do not
// already have a native symbol resolver. This is necessary because
// `--compile-all` will try and resolve all native functions. The function
// returned by the dummy native symbol resolver will never be invoked.
static void SetupStubNativeResolvers() {
  Dart_Handle libraries = Dart_GetLoadedLibraries();
  DART_CHECK_VALID(libraries);
  intptr_t libraries_length;
  DART_CHECK_VALID(Dart_ListLength(libraries, &libraries_length));
  for (intptr_t i = 0; i < libraries_length; i++) {
    Dart_Handle library = Dart_ListGetAt(libraries, i);
    DART_CHECK_VALID(library);
    Dart_NativeEntryResolver old_resolver = NULL;
    DART_CHECK_VALID(Dart_GetNativeResolver(library, &old_resolver));
    if (old_resolver == NULL) {
      Dart_Handle result =
          Dart_SetNativeResolver(library, &StubNativeLookup, &StubNativeSymbol);
      DART_CHECK_VALID(result);
    }
  }
}

std::vector<char> CreateSnapshot() {
  uint8_t* buffer = nullptr;
  intptr_t size = 0;
  DART_CHECK_VALID(Dart_CreateScriptSnapshot(&buffer, &size));
  const char* begin = reinterpret_cast<const char*>(buffer);
  return std::vector<char>(begin, begin + size);
}

bool WriteDepfile(const std::string& path,
                  const std::string& build_output,
                  const std::set<std::string>& deps) {
  std::string current_directory = files::GetCurrentDirectory();
  std::string output = build_output + ":";
  for (const auto& dep : deps) {
    std::string file = dep;
    DCHECK(!file.empty());
    if (file[0] != '/')
      file = current_directory + "/" + file;

    std::string resolved_file;
    if (files::ReadSymbolicLink(file, &resolved_file)) {
      output += " " + resolved_file;
    } else {
      output += " " + file;
    }
  }
  return files::WriteFile(path, output.data(), output.size());
}

int CreateSnapshot(const ftl::CommandLine& command_line) {
  if (command_line.HasOption(kHelp, nullptr)) {
    Usage();
    return 0;
  }

  if (command_line.positional_args().empty()) {
    Usage();
    return 1;
  }

  std::string packages;
  if (!command_line.GetOptionValue(kPackages, &packages)) {
    std::cerr << "error: Need --" << kPackages << std::endl;
    return 1;
  }

  std::vector<std::string> args = command_line.positional_args();
  if (args.size() != 1) {
    std::cerr << "error: Need one position argument. Got " << args.size() << "."
              << std::endl;
    return 1;
  }

  std::string main_dart = args[0];
  const bool print_deps_mode = command_line.HasOption(kPrintDeps, nullptr);
  std::string snapshot;
  std::string depfile;
  std::string build_output;
  if (!print_deps_mode) {
    if (!command_line.GetOptionValue(kSnapshot, &snapshot)) {
      std::cerr << "error: Need --" << kSnapshot << "." << std::endl;
      return 1;
    }
    if (command_line.GetOptionValue(kDepfile, &depfile) &&
        !command_line.GetOptionValue(kBuildOutput, &build_output)) {
      std::cerr << "error: Need --" << kBuildOutput << " if --" << kDepfile
                << " is specified." << std::endl;
      return 1;
    }
  }

  InitDartVM();

  tonic::FileLoader& loader = GetLoader();
  if (!loader.LoadPackagesMap(packages))
    return 1;

  Dart_Isolate isolate = CreateDartIsolate();
  CHECK(isolate) << "Failed to create isolate.";

  DartScope scope(isolate);

  DART_CHECK_VALID(Dart_SetLibraryTagHandler(HandleLibraryTag));
  Dart_Handle load_result =
      Dart_LoadScript(ToDart(main_dart), Dart_Null(),
                      ToDart(loader.Fetch(main_dart)), 0, 0);

  if (Dart_IsError(load_result)) {
    std::cerr << "error: Failed to load main script:"
              << std::endl
              << Dart_GetError(load_result)
              << std::endl;
    return 1;
  }

  SetupStubNativeResolvers();

  if (print_deps_mode) {
    if (Dart_IsError(load_result)) {
      // Loading / parsing the source resulted in an error. Report the error
      // to stderr and exit.
      std::cerr << Dart_GetError(load_result) << std::endl;
      return 1;
    }
    // The script has been loaded, print out the minimal dependencies to run.
    for (const auto& dep : loader.url_dependencies()) {
      std::string file = dep;
      DCHECK(!file.empty());
      std::cout << file << "\n";
    }
  } else {
    DART_CHECK_VALID(load_result);
    DART_CHECK_VALID(Dart_FinalizeLoading(false));

    // The script has been loaded, generate a snapshot.
    std::vector<char> snapshot_blob = CreateSnapshot();

    if (!snapshot.empty() &&
        !files::WriteFile(snapshot, snapshot_blob.data(), snapshot_blob.size())) {
      std::cerr << "error: Failed to write snapshot to '" << snapshot << "'."
                << std::endl;
      return 1;
    }

    if (!depfile.empty() &&
        !WriteDepfile(depfile, build_output, loader.dependencies())) {
      std::cerr << "error: Failed to write depfile to '" << depfile << "'."
                << std::endl;
      return 1;
    }

    // Run compile-all *after* generating the snapshot to avoid adding
    // unnecessary compilation related artifacts to the snapshot.
    const bool compile_all_mode = command_line.HasOption(kCompileAll, nullptr);
    if (compile_all_mode) {
      // Compile all the code loaded into the isolate. This will eagerly detect
      // syntax errors.
      Dart_Handle compile_all_results = Dart_CompileAll();
      if (Dart_IsError(compile_all_results)) {
        std::cerr << "error: Compilation errors detected:"
                  << std::endl
                  << Dart_GetError(compile_all_results)
                  << std::endl;
        return 1;
      }
    }
  }

  return 0;
}

}  // namespace

int main(int argc, const char* argv[]) {
  return CreateSnapshot(ftl::CommandLineFromArgcArgv(argc, argv));
}
