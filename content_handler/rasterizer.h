// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_CONTENT_HANDLER_RASTERIZER_H_
#define FLUTTER_CONTENT_HANDLER_RASTERIZER_H_

#include <memory>

#include "apps/mozart/services/buffers/cpp/buffer_producer.h"
#include "apps/mozart/services/composition/scenes.fidl.h"
#include "flutter/flow/compositor_context.h"
#include "flutter/flow/layers/layer_tree.h"
#include "lib/ftl/functional/closure.h"
#include "base/macros.h"

namespace flutter_runner {

class Rasterizer {
 public:
  Rasterizer();
  ~Rasterizer();

  void SetScene(fidl::InterfaceHandle<mozart::Scene> scene);

  void Draw(std::unique_ptr<flow::LayerTree> layer_tree, ftl::Closure callback);

 private:
  mozart::ScenePtr scene_;
  std::unique_ptr<mozart::BufferProducer> buffer_producer_;
  flow::CompositorContext compositor_context_;

  DISALLOW_COPY_AND_ASSIGN(Rasterizer);
};

}  // namespace flutter_runner

#endif  // FLUTTER_CONTENT_HANDLER_RASTERIZER_H_
