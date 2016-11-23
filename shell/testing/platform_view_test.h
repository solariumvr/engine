// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_TESTING_PLATFORM_VIEW_TEST_H_
#define SHELL_TESTING_PLATFORM_VIEW_TEST_H_

#include "flutter/shell/common/platform_view.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"

namespace shell {

class Shell;

class PlatformViewTest : public PlatformView {
 public:
  PlatformViewTest();

  ~PlatformViewTest();

  bool ResourceContextMakeCurrent() override;

  void RunFromSource(const std::string& assets_directory,
                     const std::string& main,
                     const std::string& packages) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(PlatformViewTest);
};

}  // namespace shell

#endif  // SHELL_TESTING_PLATFORM_VIEW_TEST_H_
