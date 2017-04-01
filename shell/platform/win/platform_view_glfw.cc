// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/win/platform_view_glfw.h"

#include "third_party/glad/include/glad/glad.h"
#include <GLFW/glfw3.h>

#include "flutter/common/threads.h"
#include "flutter/shell/gpu/gpu_rasterizer.h"
//#include "solarium/engine/shader.h"
//#include "solarium/engine/mesh.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace shell {

    inline PlatformViewGLFW* ToPlatformView(GLFWwindow* window) {
        return static_cast<PlatformViewGLFW*>(glfwGetWindowUserPointer(window));
    }

    PlatformViewGLFW::PlatformViewGLFW()
            : PlatformView(std::make_unique<GPURasterizer>(nullptr)),
              valid_(false),
              glfw_window_(nullptr),
              buttons_(0) {
        CreateEngine();

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        //glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
        static const int kStencilBits = 8;  // Skia needs 8 stencil bits

        glfwWindowHint(GLFW_STENCIL_BITS, kStencilBits);
        glfwWindowHint(GLFW_RED_BITS, kStencilBits);
        glfwWindowHint(GLFW_GREEN_BITS, kStencilBits);
        glfwWindowHint(GLFW_BLUE_BITS, kStencilBits);
        glfwWindowHint(GLFW_DOUBLEBUFFER, 1);
        glfwWindowHint(GLFW_DEPTH_BITS, 1);

        if (!glfwInit()) {
            return;
        }

        glfw_window_ = glfwCreateWindow(640, 480, "Flutter", NULL, NULL);
        if (glfw_window_ == nullptr) {
            return;
        }


        glfwSetWindowUserPointer(glfw_window_, this);

        glfwSetWindowSizeCallback(
                glfw_window_, [](GLFWwindow* window, int width, int height) {
                    ToPlatformView(window)->OnWindowSizeChanged(width, height);
                });

        glfwSetMouseButtonCallback(
                glfw_window_, [](GLFWwindow* window, int button, int action, int mods) {
                    ToPlatformView(window)->OnMouseButtonChanged(button, action, mods);
                });

        glfwSetKeyCallback(glfw_window_, [](GLFWwindow* window, int key, int scancode,
                                            int action, int mods) {
            ToPlatformView(window)->OnKeyEvent(key, scancode, action, mods);
        });


        blink::ViewportMetrics metrics;
        glfwGetWindowSize(glfw_window_, &metrics.physical_width, &metrics.physical_height);

        blink::Threads::UI()->PostTask([ engine = engine().GetWeakPtr(), metrics ] {
            if (engine.get())
                engine->SetViewportMetrics(metrics);
        });

        valid_ = true;
    }

    PlatformViewGLFW::~PlatformViewGLFW() {
        if (glfw_window_ != nullptr) {
            glfwSetWindowUserPointer(glfw_window_, nullptr);
            glfwDestroyWindow(glfw_window_);
            glfw_window_ = nullptr;
        }

        glfwTerminate();
    }

    bool PlatformViewGLFW::IsValid() const {
        return valid_;
    }

    intptr_t PlatformViewGLFW::GLContextFBO() const {
        // The default window bound FBO.
        return 0;
    }

    bool PlatformViewGLFW::GLContextMakeCurrent() {
        glfwMakeContextCurrent(glfw_window_);
        return true;
    }

    bool PlatformViewGLFW::GLContextClearCurrent() {
        glfwMakeContextCurrent(nullptr);
        return true;
    }

    bool PlatformViewGLFW::ResourceContextMakeCurrent() {
        // Resource loading contexts are not supported on this platform.
        return false;
    }

    bool PlatformViewGLFW::GLContextPresent() {
        //glDisable(GL_STENCIL_TEST);
        //glDisable(GL_DEPTH_TEST);
        //glClear(GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //auto s = engine::Shader::GetByKey("simple");
        //glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
        //glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
        //glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
        //auto projection = glm::perspective(45.0f, (GLfloat)(1024 / 768), 0.1f, 100.0f);
        //auto view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        //auto model = glm::mat4();
        //s.Use();
        //s.SetUniform("projection", projection);
        //s.SetUniform("view", view);
        //s.SetUniform("model", model);
        //glBindVertexArray(m->VAO);
        //glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        //glBindVertexArray(0);
        //s.End();
        glfwSwapBuffers(glfw_window_);
        return true;
    }

    void PlatformViewGLFW::RunFromSource(const std::string& assets_directory,
                                         const std::string& main,
                                         const std::string& packages) {}

    void PlatformViewGLFW::NotifyCreated(std::unique_ptr<Surface> surface)
    {
        PlatformView::NotifyCreated(std::move(surface), [&]() {
            gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
//            engine::Shader::Init();
        });
    }

    void PlatformViewGLFW::OnWindowSizeChanged(int width, int height) {
        blink::ViewportMetrics metrics;
        metrics.physical_width = width;
        metrics.physical_height = height;

        blink::Threads::UI()->PostTask([ engine = engine().GetWeakPtr(), metrics ] {
            if (engine.get())
                engine->SetViewportMetrics(metrics);
        });
    }

    void PlatformViewGLFW::OnMouseButtonChanged(int button, int action, int mods) {
        blink::PointerData::Change change = blink::PointerData::Change::kCancel;
        if (action == GLFW_PRESS) {
            if (!buttons_) {
                change = blink::PointerData::Change::kDown;
                glfwSetCursorPosCallback(
                        glfw_window_, [](GLFWwindow* window, double x, double y) {
                            ToPlatformView(window)->OnCursorPosChanged(x, y);
                        });
            } else {
                change = blink::PointerData::Change::kMove;
            }
            // GLFW's button order matches what we want:
            // https://github.com/flutter/engine/blob/master/sky/specs/pointer.md
            // http://www.glfw.org/docs/3.2/group__buttons.html
            buttons_ |= 1 << button;
        } else if (action == GLFW_RELEASE) {
            buttons_ &= ~(1 << button);
            if (!buttons_) {
                change = blink::PointerData::Change::kUp;
                glfwSetCursorPosCallback(glfw_window_, nullptr);
            } else {
                change = blink::PointerData::Change::kMove;
            }
        } else {
            DLOG(INFO) << "Unknown mouse action: " << action;
            return;
        }

        double x = 0.0;
        double y = 0.0;
        glfwGetCursorPos(glfw_window_, &x, &y);

        base::TimeDelta time_stamp = base::TimeTicks::Now() - base::TimeTicks();

        blink::PointerData pointer_data;
        pointer_data.Clear();
        pointer_data.time_stamp = time_stamp.InMicroseconds();
        pointer_data.change = change;
        pointer_data.kind = blink::PointerData::DeviceKind::kMouse;
        pointer_data.physical_x = x;
        pointer_data.physical_y = y;
        pointer_data.buttons = buttons_;
        pointer_data.pressure = 1.0;
        pointer_data.pressure_max = 1.0;

        blink::Threads::UI()->PostTask(
        [ engine = engine().GetWeakPtr(), pointer_data ] {
            if (engine.get()) {
                blink::PointerDataPacket packet(1);
                packet.SetPointerData(0, pointer_data);
                engine->DispatchPointerDataPacket(packet);
            }
        });
    }

    void PlatformViewGLFW::OnCursorPosChanged(double x, double y) {
        base::TimeDelta time_stamp = base::TimeTicks::Now() - base::TimeTicks();

        blink::PointerData pointer_data;
        pointer_data.Clear();
        pointer_data.time_stamp = time_stamp.InMicroseconds();
        pointer_data.change = blink::PointerData::Change::kMove;
        pointer_data.kind = blink::PointerData::DeviceKind::kMouse;
        pointer_data.physical_x = x;
        pointer_data.physical_y = y;
        pointer_data.buttons = buttons_;
        pointer_data.pressure = 1.0;
        pointer_data.pressure_max = 1.0;

        blink::Threads::UI()->PostTask(
        [ engine = engine().GetWeakPtr(), pointer_data ] {
            if (engine.get()) {
                blink::PointerDataPacket packet(1);
                packet.SetPointerData(0, pointer_data);
                engine->DispatchPointerDataPacket(packet);
            }
        });
    }

    void PlatformViewGLFW::OnKeyEvent(int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(glfw_window_, GLFW_TRUE);
            base::MessageLoop::current()->QuitNow();
        }

        ftl::RefPtr<blink::PlatformMessageResponse> response;
        if (action == GLFW_PRESS) {
            DispatchPlatformMessage(ftl::MakeRefCounted<blink::PlatformMessage>("flutter/keyevent", static_cast<std::vector<uint8_t>>(scancode), response ));
        }
    }

}  // namespace shell
