// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_COMMON_ANIMATOR_H_
#define FLUTTER_SHELL_COMMON_ANIMATOR_H_

#include "flutter/shell/common/engine.h"
#include "flutter/shell/common/rasterizer.h"
#include "flutter/shell/common/vsync_waiter.h"
#include "flutter/synchronization/pipeline.h"
#include "flutter/synchronization/semaphore.h"
#include "lib/ftl/memory/ref_ptr.h"
#include "base/memory/weak_ptr.h"
#include "lib/ftl/time/time_point.h"

namespace shell {

class Animator {
 public:
  Animator(base::WeakPtr<Rasterizer> rasterizer,
           VsyncWaiter* waiter,
           Engine* engine);

  ~Animator();

  void RequestFrame();

  void Render(std::unique_ptr<flow::LayerTree> layer_tree);

  void Start();

  void Stop();

 private:
  using LayerTreePipeline = flutter::Pipeline<flow::LayerTree>;

  void BeginFrame(ftl::TimePoint frame_time);

  void AwaitVSync();

  base::WeakPtr<Rasterizer> rasterizer_;
  VsyncWaiter* waiter_;
  Engine* engine_;

  ftl::TimePoint last_begin_frame_time_;
  ftl::RefPtr<LayerTreePipeline> layer_tree_pipeline_;
  flutter::Semaphore pending_frame_semaphore_;
  LayerTreePipeline::ProducerContinuation producer_continuation_;
  bool paused_;

  base::WeakPtrFactory<Animator> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(Animator);
};

}  // namespace shell

#endif  // FLUTTER_SHELL_COMMON_ANIMATOR_H_
