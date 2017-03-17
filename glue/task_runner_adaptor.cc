// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/glue/task_runner_adaptor.h"

#include <utility>

#include "base/bind.h"
#include "base/location.h"
#include "base/task_runner.h"

namespace glue {
namespace {

void RunClosure(ftl::Closure task) {
  task();
}

}  // namespace

TaskRunnerAdaptor::TaskRunnerAdaptor(scoped_refptr<base::TaskRunner> runner)
    : runner_(std::move(runner)) {
  DCHECK(runner_);
}

TaskRunnerAdaptor::~TaskRunnerAdaptor() {}

void TaskRunnerAdaptor::PostTask(ftl::Closure task) {
  runner_->PostTask(FROM_HERE, base::Bind(RunClosure, task));
}

void TaskRunnerAdaptor::PostTaskForTime(ftl::Closure task,
                                        ftl::TimePoint target_time) {
  ftl::TimePoint now = ftl::TimePoint::Now();
  runner_->PostDelayedTask(FROM_HERE, base::Bind(RunClosure, task),
                           target_time <= now
                               ? base::TimeDelta()
                               : base::TimeDelta::FromMicroseconds(
                                     (target_time - now).ToMicroseconds()));
}

void TaskRunnerAdaptor::PostDelayedTask(ftl::Closure task,
                                        ftl::TimeDelta delay) {
  runner_->PostDelayedTask(
      FROM_HERE, base::Bind(RunClosure, task),
      base::TimeDelta::FromMicroseconds(delay.ToMicroseconds()));
}

void TaskRunnerAdaptor::PostTaskAndReply(ftl::Closure task, ftl::Closure reply)
{
	runner_->PostTaskAndReply(FROM_HERE, base::Bind(RunClosure, task), base::Bind(RunClosure, reply));
}

bool TaskRunnerAdaptor::RunsTasksOnCurrentThread() {
  return runner_->RunsTasksOnCurrentThread();
}

}  // namespace glue
