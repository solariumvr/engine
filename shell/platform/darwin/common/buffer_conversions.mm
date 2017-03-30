// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/darwin/common/buffer_conversions.h"

namespace shell {

std::vector<uint8_t> GetVectorFromNSData(NSData* data) {
  if (!data.length)
    return std::vector<uint8_t>();
  const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.bytes);
  return std::vector<uint8_t>(bytes, bytes + data.length);
}

NSData* GetNSDataFromVector(const std::vector<uint8_t>& buffer) {
  return [NSData dataWithBytes:buffer.data() length:buffer.size()];
}

}  // namespace shell
