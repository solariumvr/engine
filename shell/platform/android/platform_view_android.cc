// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/android/platform_view_android.h"

#include <android/native_window_jni.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>

#include <utility>

#include "flutter/common/threads.h"
#include "flutter/fml/platform/android/jni_util.h"
#include "flutter/fml/platform/android/scoped_java_ref.h"
#include "flutter/runtime/dart_service_isolate.h"
#include "flutter/shell/gpu/gpu_rasterizer.h"
#include "flutter/shell/platform/android/android_surface_gl.h"
#include "flutter/shell/platform/android/platform_view_android_jni.h"
#include "flutter/shell/platform/android/vsync_waiter_android.h"
#include "lib/ftl/functional/make_copyable.h"

#if SHELL_ENABLE_VULKAN
#include "flutter/shell/platform/android/android_surface_vulkan.h"
#endif  // SHELL_ENABLE_VULKAN

namespace shell {

class PlatformMessageResponseAndroid : public blink::PlatformMessageResponse {
  FRIEND_MAKE_REF_COUNTED(PlatformMessageResponseAndroid);

 public:
  void Complete(std::vector<uint8_t> data) override {
    ftl::RefPtr<PlatformMessageResponseAndroid> self(this);
    blink::Threads::Platform()->PostTask(
        ftl::MakeCopyable([ self, data = std::move(data) ]() mutable {
          if (!self->view_)
            return;
          static_cast<PlatformViewAndroid*>(self->view_.get())
              ->HandlePlatformMessageResponse(self->response_id_,
                                              std::move(data));
        }));
  }

  void CompleteWithError() override { Complete(std::vector<uint8_t>()); }

 private:
  PlatformMessageResponseAndroid(int response_id,
                                 ftl::WeakPtr<PlatformView> view)
      : response_id_(response_id), view_(view) {}

  int response_id_;
  ftl::WeakPtr<PlatformView> view_;
};

static std::unique_ptr<AndroidSurface> InitializePlatformSurfaceGL() {
  const PlatformView::SurfaceConfig offscreen_config = {
      .red_bits = 8,
      .green_bits = 8,
      .blue_bits = 8,
      .alpha_bits = 8,
      .depth_bits = 0,
      .stencil_bits = 0,
  };
  auto surface = std::make_unique<AndroidSurfaceGL>(offscreen_config);
  return surface->IsOffscreenContextValid() ? std::move(surface) : nullptr;
}

static std::unique_ptr<AndroidSurface> InitializePlatformSurfaceVulkan() {
#if SHELL_ENABLE_VULKAN
  auto surface = std::make_unique<AndroidSurfaceVulkan>();
  return surface->IsValid() ? std::move(surface) : nullptr;
#else   // SHELL_ENABLE_VULKAN
  return nullptr;
#endif  // SHELL_ENABLE_VULKAN
}

static std::unique_ptr<AndroidSurface> InitializePlatformSurface() {
  if (auto surface = InitializePlatformSurfaceVulkan()) {
    FTL_DLOG(INFO) << "Vulkan surface initialized.";
    return surface;
  }

  FTL_DLOG(INFO)
      << "Could not initialize Vulkan surface. Falling back to OpenGL.";

  if (auto surface = InitializePlatformSurfaceGL()) {
    FTL_DLOG(INFO) << "GL surface initialized.";
    return surface;
  }

  FTL_CHECK(false) << "Could not initialize either the Vulkan or OpenGL "
                      "surface backends. Flutter requires a GPU to render.";
  return nullptr;
}

PlatformViewAndroid::PlatformViewAndroid()
    : PlatformView(std::make_unique<GPURasterizer>(nullptr)),
      android_surface_(InitializePlatformSurface()) {
  CreateEngine();

  // Eagerly setup the IO thread context. We have already setup the surface.
  SetupResourceContextOnIOThread();

  UpdateThreadPriorities();

  PostAddToShellTask();
}

PlatformViewAndroid::~PlatformViewAndroid() = default;

void PlatformViewAndroid::Detach() {
  ReleaseSurface();
  delete this;
}

void PlatformViewAndroid::SurfaceCreated(JNIEnv* env,
                                         jobject jsurface,
                                         jint backgroundColor) {
  // Note: This frame ensures that any local references used by
  // ANativeWindow_fromSurface are released immediately. This is needed as a
  // workaround for https://code.google.com/p/android/issues/detail?id=68174
  fml::jni::ScopedJavaLocalFrame scoped_local_reference_frame(env);

  auto native_window = ftl::MakeRefCounted<AndroidNativeWindow>(
      ANativeWindow_fromSurface(env, jsurface));

  if (!native_window->IsValid()) {
    return;
  }

  if (!android_surface_->SetNativeWindow(native_window)) {
    return;
  }

  std::unique_ptr<Surface> gpu_surface = android_surface_->CreateGPUSurface();

  if (gpu_surface == nullptr || !gpu_surface->IsValid()) {
    return;
  }

  NotifyCreated(std::move(gpu_surface), [
    this, backgroundColor, native_window_size = native_window->GetSize()
  ] { rasterizer().Clear(backgroundColor, native_window_size); });
}

void PlatformViewAndroid::SurfaceChanged(jint width, jint height) {
  blink::Threads::Gpu()->PostTask([this, width, height]() {
    if (android_surface_) {
      android_surface_->OnScreenSurfaceResize(SkISize::Make(width, height));
    }
  });
}

void PlatformViewAndroid::UpdateThreadPriorities() {
  blink::Threads::Gpu()->PostTask(
      []() { ::setpriority(PRIO_PROCESS, gettid(), -2); });

  blink::Threads::UI()->PostTask(
      []() { ::setpriority(PRIO_PROCESS, gettid(), -1); });
}

void PlatformViewAndroid::SurfaceDestroyed() {
  ReleaseSurface();
}

void PlatformViewAndroid::RunBundleAndSnapshot(std::string bundle_path,
                                               std::string snapshot_override) {
  blink::Threads::UI()->PostTask([
    engine = engine_->GetWeakPtr(), bundle_path = std::move(bundle_path),
    snapshot_override = std::move(snapshot_override)
  ] {
    if (engine)
      engine->RunBundleAndSnapshot(std::move(bundle_path),
                                   std::move(snapshot_override));
  });
}

void PlatformViewAndroid::RunBundleAndSource(std::string bundle_path,
                                             std::string main,
                                             std::string packages) {
  blink::Threads::UI()->PostTask([
    engine = engine_->GetWeakPtr(), bundle_path = std::move(bundle_path),
    main = std::move(main), packages = std::move(packages)
  ] {
    if (engine)
      engine->RunBundleAndSource(std::move(bundle_path), std::move(main),
                                 std::move(packages));
  });
}

void PlatformViewAndroid::SetViewportMetrics(jfloat device_pixel_ratio,
                                             jint physical_width,
                                             jint physical_height,
                                             jint physical_padding_top,
                                             jint physical_padding_right,
                                             jint physical_padding_bottom,
                                             jint physical_padding_left) {
  blink::ViewportMetrics metrics;
  metrics.device_pixel_ratio = device_pixel_ratio;
  metrics.physical_width = physical_width;
  metrics.physical_height = physical_height;
  metrics.physical_padding_top = physical_padding_top;
  metrics.physical_padding_right = physical_padding_right;
  metrics.physical_padding_bottom = physical_padding_bottom;
  metrics.physical_padding_left = physical_padding_left;

  blink::Threads::UI()->PostTask([ engine = engine_->GetWeakPtr(), metrics ] {
    if (engine)
      engine->SetViewportMetrics(metrics);
  });
}

void PlatformViewAndroid::DispatchPlatformMessage(JNIEnv* env,
                                                  std::string name,
                                                  jobject java_message_data,
                                                  jint java_message_position,
                                                  jint response_id) {
  std::vector<uint8_t> message;
  if (java_message_data) {
    uint8_t* message_data =
        static_cast<uint8_t*>(env->GetDirectBufferAddress(java_message_data));
    message = std::vector<uint8_t>(message_data,
                                   message_data + java_message_position);
  }

  ftl::RefPtr<blink::PlatformMessageResponse> response;
  if (response_id) {
    response = ftl::MakeRefCounted<PlatformMessageResponseAndroid>(
        response_id, GetWeakPtr());
  }

  PlatformView::DispatchPlatformMessage(
      ftl::MakeRefCounted<blink::PlatformMessage>(
          std::move(name), std::move(message), std::move(response)));
}

void PlatformViewAndroid::DispatchPointerDataPacket(JNIEnv* env,
                                                    jobject obj,
                                                    jobject buffer,
                                                    jint position) {
  uint8_t* data = static_cast<uint8_t*>(env->GetDirectBufferAddress(buffer));

  blink::Threads::UI()->PostTask(ftl::MakeCopyable([
    engine = engine_->GetWeakPtr(),
    packet = std::make_unique<PointerDataPacket>(data, position)
  ] {
    if (engine.get())
      engine->DispatchPointerDataPacket(*packet);
  }));
}

void PlatformViewAndroid::InvokePlatformMessageResponseCallback(
    JNIEnv* env,
    jint response_id,
    jobject java_response_data,
    jint java_response_position) {
  if (!response_id)
    return;
  auto it = pending_responses_.find(response_id);
  if (it == pending_responses_.end())
    return;
  std::vector<uint8_t> response;
  if (java_response_data) {
    uint8_t* response_data =
        static_cast<uint8_t*>(env->GetDirectBufferAddress(java_response_data));
    response = std::vector<uint8_t>(response_data,
                                    response_data + java_response_position);
  }
  auto message_response = std::move(it->second);
  pending_responses_.erase(it);
  message_response->Complete(std::move(response));
}

void PlatformViewAndroid::HandlePlatformMessage(
    ftl::RefPtr<blink::PlatformMessage> message) {
  JNIEnv* env = fml::jni::AttachCurrentThread();
  fml::jni::ScopedJavaLocalRef<jobject> view = flutter_view_.get(env);
  if (view.is_null())
    return;

  int response_id = 0;
  if (auto response = message->response()) {
    response_id = next_response_id_++;
    pending_responses_[response_id] = response;
  }

  fml::jni::ScopedJavaLocalRef<jbyteArray> message_array(env,
      env->NewByteArray(message->data().size()));
  env->SetByteArrayRegion(
      message_array.obj(), 0, message->data().size(),
      reinterpret_cast<const jbyte*>(message->data().data()));
  auto java_channel = fml::jni::StringToJavaString(env, message->channel());
  message = nullptr;

  // This call can re-enter in InvokePlatformMessageResponseCallback.
  FlutterViewHandlePlatformMessage(
      env, view.obj(), java_channel.obj(), message_array.obj(), response_id);
}

void PlatformViewAndroid::HandlePlatformMessageResponse(
    int response_id,
    std::vector<uint8_t> data) {
  JNIEnv* env = fml::jni::AttachCurrentThread();

  fml::jni::ScopedJavaLocalRef<jobject> view = flutter_view_.get(env);

  if (view.is_null())
    return;

  fml::jni::ScopedJavaLocalRef<jbyteArray> data_array(env,
      env->NewByteArray(data.size()));
  env->SetByteArrayRegion(data_array.obj(), 0, data.size(),
                          reinterpret_cast<const jbyte*>(data.data()));

  FlutterViewHandlePlatformMessageResponse(env, view.obj(), response_id,
                                           data_array.obj());
}

void PlatformViewAndroid::DispatchSemanticsAction(jint id, jint action) {
  PlatformView::DispatchSemanticsAction(
      id, static_cast<blink::SemanticsAction>(action));
}

void PlatformViewAndroid::SetSemanticsEnabled(jboolean enabled) {
  PlatformView::SetSemanticsEnabled(enabled);
}

void PlatformViewAndroid::ReleaseSurface() {
  NotifyDestroyed();
  android_surface_->TeardownOnScreenContext();
}

VsyncWaiter* PlatformViewAndroid::GetVsyncWaiter() {
  if (!vsync_waiter_)
    vsync_waiter_ = std::make_unique<VsyncWaiterAndroid>();
  return vsync_waiter_.get();
}

bool PlatformViewAndroid::ResourceContextMakeCurrent() {
  return android_surface_->ResourceContextMakeCurrent();
}

void PlatformViewAndroid::UpdateSemantics(
    std::vector<blink::SemanticsNode> update) {
  constexpr size_t kBytesPerNode = 25 * sizeof(int32_t);
  constexpr size_t kBytesPerChild = sizeof(int32_t);

  JNIEnv* env = fml::jni::AttachCurrentThread();
  {
    fml::jni::ScopedJavaLocalRef<jobject> view = flutter_view_.get(env);
    if (view.is_null())
      return;

    size_t num_bytes = 0;
    for (const blink::SemanticsNode& node : update) {
      num_bytes += kBytesPerNode;
      num_bytes += node.children.size() * kBytesPerChild;
    }

    std::vector<uint8_t> buffer(num_bytes);
    int32_t* buffer_int32 = reinterpret_cast<int32_t*>(&buffer[0]);
    float* buffer_float32 = reinterpret_cast<float*>(&buffer[0]);

    std::vector<std::string> strings;
    size_t position = 0;
    for (const blink::SemanticsNode& node : update) {
      buffer_int32[position++] = node.id;
      buffer_int32[position++] = node.flags;
      buffer_int32[position++] = node.actions;
      if (node.label.empty()) {
        buffer_int32[position++] = -1;
      } else {
        buffer_int32[position++] = strings.size();
        strings.push_back(node.label);
      }
      buffer_float32[position++] = node.rect.left();
      buffer_float32[position++] = node.rect.top();
      buffer_float32[position++] = node.rect.right();
      buffer_float32[position++] = node.rect.bottom();
      node.transform.asColMajorf(&buffer_float32[position]);
      position += 16;
      buffer_int32[position++] = node.children.size();
      for (int32_t child : node.children)
        buffer_int32[position++] = child;
    }

    fml::jni::ScopedJavaLocalRef<jobject> direct_buffer(env,
        env->NewDirectByteBuffer(buffer.data(), buffer.size()));

    FlutterViewUpdateSemantics(
        env, view.obj(), direct_buffer.obj(),
        fml::jni::VectorToStringArray(env, strings).obj());
  }
}

void PlatformViewAndroid::RunFromSource(const std::string& assets_directory,
                                        const std::string& main,
                                        const std::string& packages) {
  JNIEnv* env = fml::jni::AttachCurrentThread();
  FTL_CHECK(env);

  {
    fml::jni::ScopedJavaLocalRef<jobject> local_flutter_view =
        flutter_view_.get(env);
    if (local_flutter_view.is_null()) {
      // Collected.
      return;
    }

    // Grab the class of the flutter view.
    jclass flutter_view_class = env->GetObjectClass(local_flutter_view.obj());
    FTL_CHECK(flutter_view_class);

    // Grab the runFromSource method id.
    jmethodID run_from_source_method_id = env->GetMethodID(
        flutter_view_class, "runFromSource",
        "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
    FTL_CHECK(run_from_source_method_id);

    // Invoke runFromSource on the Android UI thread.
    jstring java_assets_directory = env->NewStringUTF(assets_directory.c_str());
    FTL_CHECK(java_assets_directory);
    jstring java_main = env->NewStringUTF(main.c_str());
    FTL_CHECK(java_main);
    jstring java_packages = env->NewStringUTF(packages.c_str());
    FTL_CHECK(java_packages);
    env->CallVoidMethod(local_flutter_view.obj(), run_from_source_method_id,
                        java_assets_directory, java_main, java_packages);
  }

  // Detaching from the VM deletes any stray local references.
  fml::jni::DetachFromVM();
}

fml::jni::ScopedJavaLocalRef<jobject> PlatformViewAndroid::GetBitmap(
    JNIEnv* env) {
  // Render the last frame to an array of pixels on the GPU thread.
  // The pixels will be returned as a global JNI reference to an int array.
  ftl::AutoResetWaitableEvent latch;
  jobject pixels_ref = nullptr;
  SkISize frame_size;
  blink::Threads::Gpu()->PostTask([this, &latch, &pixels_ref, &frame_size]() {
    GetBitmapGpuTask(&pixels_ref, &frame_size);
    latch.Signal();
  });

  latch.Wait();

  // Convert the pixel array to an Android bitmap.
  if (pixels_ref == nullptr)
    return fml::jni::ScopedJavaLocalRef<jobject>();

  fml::jni::ScopedJavaGlobalRef<jobject> pixels(env, pixels_ref);

  jclass bitmap_class = env->FindClass("android/graphics/Bitmap");
  FTL_CHECK(bitmap_class);

  jmethodID create_bitmap = env->GetStaticMethodID(
      bitmap_class, "createBitmap",
      "([IIILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");
  FTL_CHECK(create_bitmap);

  jclass bitmap_config_class = env->FindClass("android/graphics/Bitmap$Config");
  FTL_CHECK(bitmap_config_class);

  jmethodID bitmap_config_value_of = env->GetStaticMethodID(
      bitmap_config_class, "valueOf",
      "(Ljava/lang/String;)Landroid/graphics/Bitmap$Config;");
  FTL_CHECK(bitmap_config_value_of);

  jstring argb = env->NewStringUTF("ARGB_8888");
  FTL_CHECK(argb);

  jobject bitmap_config = env->CallStaticObjectMethod(
      bitmap_config_class, bitmap_config_value_of, argb);
  FTL_CHECK(bitmap_config);

  jobject bitmap = env->CallStaticObjectMethod(
      bitmap_class, create_bitmap, pixels.obj(), frame_size.width(),
      frame_size.height(), bitmap_config);

  return fml::jni::ScopedJavaLocalRef<jobject>(env, bitmap);
}

void PlatformViewAndroid::GetBitmapGpuTask(jobject* pixels_out,
                                           SkISize* size_out) {
  flow::LayerTree* layer_tree = rasterizer_->GetLastLayerTree();
  if (layer_tree == nullptr)
    return;

  JNIEnv* env = fml::jni::AttachCurrentThread();
  FTL_CHECK(env);

  const SkISize& frame_size = layer_tree->frame_size();
  jsize pixels_size = frame_size.width() * frame_size.height();
  jintArray pixels_array = env->NewIntArray(pixels_size);
  FTL_CHECK(pixels_array);

  jint* pixels = env->GetIntArrayElements(pixels_array, nullptr);
  FTL_CHECK(pixels);

  SkImageInfo image_info =
      SkImageInfo::Make(frame_size.width(), frame_size.height(),
                        kRGBA_8888_SkColorType, kPremul_SkAlphaType);

  sk_sp<SkSurface> surface = SkSurface::MakeRasterDirect(
      image_info, pixels, frame_size.width() * sizeof(jint));

  flow::CompositorContext compositor_context(nullptr);
  SkCanvas* canvas = surface->getCanvas();
  flow::CompositorContext::ScopedFrame frame =
      compositor_context.AcquireFrame(nullptr, canvas, false);

  canvas->clear(SK_ColorBLACK);
  layer_tree->Raster(frame);
  canvas->flush();

  // Our configuration of Skia does not support rendering to the
  // BitmapConfig.ARGB_8888 format expected by android.graphics.Bitmap.
  // Convert from kRGBA_8888 to kBGRA_8888 (equivalent to ARGB_8888).
  for (int i = 0; i < pixels_size; i++) {
    uint8_t* bytes = reinterpret_cast<uint8_t*>(pixels + i);
    std::swap(bytes[0], bytes[2]);
  }

  env->ReleaseIntArrayElements(pixels_array, pixels, 0);

  *pixels_out = env->NewGlobalRef(pixels_array);
  *size_out = frame_size;

  fml::jni::DetachFromVM();
}

}  // namespace shell
