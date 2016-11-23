// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_FLOW_LAYERS_CLIP_PATH_LAYER_H_
#define FLUTTER_FLOW_LAYERS_CLIP_PATH_LAYER_H_

#include "flutter/flow/layers/container_layer.h"

namespace flow {

class ClipPathLayer : public ContainerLayer {
 public:
  ClipPathLayer();
  ~ClipPathLayer() override;

  void set_clip_path(const SkPath& clip_path) { clip_path_ = clip_path; }

 protected:
  void Preroll(PrerollContext* context, const SkMatrix& matrix) override;
  void Paint(PaintContext& context) override;

 private:
  SkPath clip_path_;

  DISALLOW_COPY_AND_ASSIGN(ClipPathLayer);
};

}  // namespace flow

#endif  // FLUTTER_FLOW_LAYERS_CLIP_PATH_LAYER_H_
