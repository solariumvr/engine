# This file is automatically processed to create .DEPS.git which is the file
# that gclient uses under git.
#
# See http://code.google.com/p/chromium/wiki/UsingGit
#
# To test manually, run:
#   python tools/deps2git/deps2git.py -o .DEPS.git -w <gclientdir>
# where <gcliendir> is the absolute path to the directory containing the
# .gclient file (the parent of 'src').
#
# Then commit .DEPS.git locally (gclient doesn't like dirty trees) and run
#   gclient sync
# Verify the thing happened you wanted. Then revert your .DEPS.git change
# DO NOT CHECK IN CHANGES TO .DEPS.git upstream. It will be automatically
# updated by a bot when you modify this one.
#
# When adding a new dependency, please update the top-level .gitignore file
# to list the dependency's destination directory.

vars = {
  'chromium_git': 'https://chromium.googlesource.com',
  'fuchsia_git': 'https://fuchsia.googlesource.com',
  'github_git': 'https://github.com',
  'mojo_sdk_revision': '6b5fb1227c742f5ecc077486ebc029f2711c61fa',
  'base_revision': 'b2412302ed4e45bfb47d7b5c0c3418077009e1ce',
  'skia_revision': 'bbe17a66705aff6f34a22adc0c12883dcb6161b3',

  # Note: When updating the Dart revision, ensure that all entries that are
  # dependencies of dart are also updated
  'dart_revision': '4d81fee396081f68ad33b59845fead0b905bc69d',
  'dart_boringssl_gen_revision': 'cc6668818f096351db9b591af0d7d0e9dec04451',
  'dart_boringssl_revision': '8d343b44bbab829d1a28fdef650ca95f7db4412e',
  'dart_observatory_packages_revision': '26aad88f1c1915d39bbcbff3cad589e2402fdcf1',
  'dart_root_certificates_revision': 'aed07942ce98507d2be28cbd29e879525410c7fc',

  'buildtools_revision': '1f4c1c3bd3bd4c991e6565a0dc509c8d8a3f90b4',
}

# Only these hosts are allowed for dependencies in this DEPS file.
# If you need to add a new host, contact chrome infrastructure team.
allowed_hosts = [
  'chromium.googlesource.com',
  'fuchsia.googlesource.com',
  'github.com',
]

deps = {
  'src': 'https://github.com/flutter/buildroot.git' + '@' + 'a69330303690bbc10953ed1a662f37f1f3c7b2b9',

   # Fuchsia compatibility
   #
   # The dependencies in this section should match the layout in the Fuchsia gn
   # build. Eventually, we'll manage these dependencies together with Fuchsia
   # and not have to specific specific hashes.

  'src/lib/ftl':
   Var('fuchsia_git') + '/ftl' + '@' + '9f51f24056554352b045fda338e9101c0e5af272',

  'src/lib/tonic':
   Var('fuchsia_git') + '/tonic' + '@' + '36142e27c6765301a4122dd85045c565b2d8b33f',

  'src/lib/zip':
   Var('fuchsia_git') + '/zip' + '@' + '92dc87ca645fe8e9f5151ef6dac86d8311a7222f',

  'src/mojo/public':
   Var('fuchsia_git') + '/mojo/public' + '@' + Var('mojo_sdk_revision'),

  'src/third_party/gtest':
   Var('fuchsia_git') + '/third_party/gtest' + '@' + 'c00f82917331efbbd27124b537e4ccc915a02b72',

  'src/third_party/rapidjson':
   Var('fuchsia_git') + '/third_party/rapidjson' + '@' + '9defbb0209a534ffeb3a2b79d5ee440a77407292',

   # Chromium-style
   #
   # As part of integrating with Fuchsia, we should eventually remove all these
   # Chromium-style dependencies.

  'src/base':
   Var('github_git') + '/flutter/base.git' + '@' +  Var('base_revision'),

  'src/buildtools':
   Var('fuchsia_git') + '/buildtools' + '@' +  Var('buildtools_revision'),

  # TODO(abarth): Remove in favor of //third_party/gtest
  'src/testing/gtest':
   Var('chromium_git') + '/external/googletest.git' + '@' + '23574bf2333f834ff665f894c97bef8a5b33a0a9',

  'src/testing/gmock':
   Var('chromium_git') + '/external/googlemock.git' + '@' + '29763965ab52f24565299976b936d1265cb6a271',

  'src/third_party/icu':
   Var('chromium_git') + '/chromium/deps/icu.git' + '@' + 'c3f79166089e5360c09e3053fce50e6e296c3204',

  'src/dart':
    Var('chromium_git') + '/external/github.com/dart-lang/sdk.git' + '@' + Var('dart_revision'),

  'src/third_party/boringssl':
    Var('github_git') + '/dart-lang/boringssl_gen.git' + '@' + Var('dart_boringssl_gen_revision'),

  'src/third_party/boringssl/src':
   'https://boringssl.googlesource.com/boringssl.git' + '@' + Var('dart_boringssl_revision'),

  'src/dart/third_party/observatory_pub_packages':
   Var('chromium_git') +
   '/external/github.com/dart-lang/observatory_pub_packages' + '@' +
   Var('dart_observatory_packages_revision'),

  'src/dart/third_party/root_certificates':
   Var('chromium_git') +
   '/external/github.com/dart-lang/root_certificates' + '@' +
   Var('dart_root_certificates_revision'),

  'src/third_party/skia':
   Var('chromium_git') + '/skia.git' + '@' +  Var('skia_revision'),

  'src/third_party/yasm/source/patched-yasm':
   Var('chromium_git') + '/chromium/deps/yasm/patched-yasm.git' + '@' + '4671120cd8558ce62ee8672ebf3eb6f5216f909b',

  'src/third_party/libjpeg_turbo':
   Var('chromium_git') + '/chromium/deps/libjpeg_turbo.git' + '@' + '7260e4d8b8e1e40b17f03fafdf1cd83296900f76',

   # Headers for Vulkan 1.0
   'src/third_party/vulkan':
   Var('github_git') + '/KhronosGroup/Vulkan-Docs.git' + '@' + 'e29c2489e238509c41aeb8c7bce9d669a496344b',
}

recursedeps = [
  'src/buildtools',
]

deps_os = {
  'android': {
    'src/third_party/colorama/src':
     Var('chromium_git') + '/external/colorama.git' + '@' + '799604a1041e9b3bc5d2789ecbd7e8db2e18e6b8',

    'src/third_party/jsr-305/src':
        Var('chromium_git') + '/external/jsr-305.git' + '@' + '642c508235471f7220af6d5df2d3210e3bfc0919',

    'src/third_party/junit/src':
      Var('chromium_git') + '/external/junit.git' + '@' + '45a44647e7306262162e1346b750c3209019f2e1',

    'src/third_party/mockito/src':
      Var('chromium_git') + '/external/mockito/mockito.git' + '@' + 'ed99a52e94a84bd7c467f2443b475a22fcc6ba8e',

    'src/third_party/robolectric/lib':
      Var('chromium_git') + '/chromium/third_party/robolectric.git' + '@' + '6b63c99a8b6967acdb42cbed0adb067c80efc810',

    'src/third_party/freetype-android/src':
       Var('chromium_git') + '/chromium/src/third_party/freetype2.git' + '@' + 'e186230678ee8e4ea4ac4797ece8125761e3225a',
  },
}


hooks = [
  {
    # This clobbers when necessary (based on get_landmines.py). It must be the
    # first hook so that other things that get/generate into the output
    # directory will not subsequently be clobbered.
    'name': 'landmines',
    'pattern': '.',
    'action': [
        'python',
        'src/build/landmines.py',
    ],
  },
  {
    'name': 'download_android_tools',
    'pattern': '.',
    'action': [
        'python',
        'src/tools/android/download_android_tools.py',
    ],
  },
  {
    'name': 'clang',
    'pattern': '.',
    'action': ['/bin/bash', 'src/buildtools/update.sh', '--toolchain', '--ninja', '--gn'],
  },
  {
    # Pull dart sdk if needed
    'name': 'dart',
    'pattern': '.',
    'action': ['python', 'src/tools/dart/update.py'],
  },
  {
    # Update LASTCHANGE. This is also run by export_tarball.py in
    # src/tools/export_tarball - please keep them in sync.
    'name': 'lastchange',
    'pattern': '.',
    'action': ['python', 'src/build/util/lastchange.py',
               '-o', 'src/build/util/LASTCHANGE'],
  },
  # Pull binutils for linux, enabled debug fission for faster linking /
  # debugging when used with clang on Ubuntu Precise.
  # https://code.google.com/p/chromium/issues/detail?id=352046
  {
    'name': 'binutils',
    'pattern': 'src/third_party/binutils',
    'action': [
        'python',
        'src/third_party/binutils/download.py',
    ],
  },
  # Pull eu-strip binaries using checked-in hashes.
  {
    'name': 'eu-strip',
    'pattern': '.',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--quiet',
                '--platform=linux*',
                '--no_auth',
                '--bucket', 'chromium-eu-strip',
                '-s', 'src/build/linux/bin/eu-strip.sha1',
    ],
  },
  # Pull the mojom parser binaries using checked-in hashes.
  {
    'name': 'mojom_tool',
    'pattern': '',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--quiet',
                '--platform=linux*',
                '--no_auth',
                '--bucket', 'mojo/mojom_parser/linux64',
                '-s', 'src/mojo/public/tools/bindings/mojom_tool/bin/linux64/mojom.sha1',
    ],
  },
  {
    'name': 'mojom_tool',
    'pattern': '',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--quiet',
                '--platform=darwin',
                '--no_auth',
                '--bucket', 'mojo/mojom_parser/mac64',
                '-s', 'src/mojo/public/tools/bindings/mojom_tool/bin/mac64/mojom.sha1',
    ],
  },
  {
    # Ensure that we don't accidentally reference any .pyc files whose
    # corresponding .py files have already been deleted.
    'name': 'remove_stale_pyc_files',
    'pattern': 'src/tools/.*\\.py',
    'action': [
        'python',
        'src/tools/remove_stale_pyc_files.py',
        'src/tools',
    ],
  },
  # Pull the mojom generator binaries using checked-in hashes.
  {
    'name': 'mojom_generators',
    'pattern': '',
    'action': [ 'download_from_google_storage.py',
                '--no_resume',
                '--quiet',
                '--platform=linux*',
                '--no_auth',
                '--bucket', 'mojo/mojom_parser/linux64/generators',
                '-d', 'src/mojo/public/tools/bindings/mojom_tool/bin/linux64/generators',
    ],
  },
  {
    'name': 'mojom_generators',
    'pattern': '',
    'action': [ 'download_from_google_storage.py',
                '--no_resume',
                '--quiet',
                '--platform=darwin',
                '--no_auth',
                '--bucket', 'mojo/mojom_parser/mac64/generators',
                '-d', 'src/mojo/public/tools/bindings/mojom_tool/bin/mac64/generators',
    ],
  },
]
