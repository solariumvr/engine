// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_FLOW_LAYERS_PICTURE_LAYER_H_
#define FLUTTER_FLOW_LAYERS_PICTURE_LAYER_H_

#include "flutter/flow/layers/layer.h"

namespace flow {

class PictureLayer : public Layer {
 public:
  PictureLayer();
  ~PictureLayer() override;

  void set_offset(const SkPoint& offset) { offset_ = offset; }
  void set_picture(sk_sp<SkPicture> picture) { picture_ = std::move(picture); }

  void set_is_complex(bool value) { is_complex_ = value; }
  void set_will_change(bool value) { will_change_ = value; }

  SkPicture* picture() const { return picture_.get(); }

  void Preroll(PrerollContext* frame, const SkMatrix& matrix) override;
  void Paint(PaintContext& context) override;

 private:
  SkPoint offset_;
  sk_sp<SkPicture> picture_;
  bool is_complex_ = false;
  bool will_change_ = false;

  // If we rasterized the picture separately, image_ holds the pixels.
  sk_sp<SkImage> image_;

  FTL_DISALLOW_COPY_AND_ASSIGN(PictureLayer);
};

}  // namespace flow

#endif  // FLUTTER_FLOW_LAYERS_PICTURE_LAYER_H_
