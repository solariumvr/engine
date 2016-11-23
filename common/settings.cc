// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/common/settings.h"

#include <memory>

#include "base/logging.h"

namespace blink {
namespace {

Settings* g_settings = nullptr;

}  // namespace

const Settings& Settings::Get() {
  CHECK(g_settings);
  return *g_settings;
}

void Settings::Set(const Settings& settings) {
  CHECK(!g_settings);
  g_settings = new Settings();
  *g_settings = settings;
}

}  // namespace blink
