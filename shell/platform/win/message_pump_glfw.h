// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_PLATFORM_GLFW_MESSAGE_PUMP_GLFW_H_
#define SHELL_PLATFORM_GLFW_MESSAGE_PUMP_GLFW_H_

#include "base/macros.h"
#include "base/message_loop/message_pump.h"
#include "base/time/time.h"
#include <memory>

namespace shell {

    class MessagePumpGLFW : public base::MessagePump {
    public:
        MessagePumpGLFW();

        ~MessagePumpGLFW() override;

        static std::unique_ptr<base::MessagePump> Create();

        void Run(Delegate* delegate) override;

        void Quit() override;

        void ScheduleWork() override;

        void ScheduleDelayedWork(const base::TimeTicks& delayed_work_time) override;

    private:
        bool in_run_;
        bool should_quit_;
        base::TimeTicks delayed_work_time_;

        DISALLOW_COPY_AND_ASSIGN(MessagePumpGLFW);
    };

}  // namespace shell

#endif  // SHELL_PLATFORM_GLFW_MESSAGE_PUMP_GLFW_H_
