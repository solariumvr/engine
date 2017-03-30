// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_LIB_JNI_JNI_OBJECT_H_
#define FLUTTER_LIB_JNI_JNI_OBJECT_H_

#include <jni.h>

#include "flutter/fml/platform/android/scoped_java_ref.h"
#include "lib/tonic/dart_wrappable.h"

namespace blink {

class JniClass;

// Wrapper that exposes a JNI jobject to Dart
class JniObject : public ftl::RefCountedThreadSafe<JniObject>,
                  public tonic::DartWrappable {
  DEFINE_WRAPPERTYPEINFO();
  FRIEND_MAKE_REF_COUNTED(JniObject);

 public:
  ~JniObject() override;

  static ftl::RefPtr<JniObject> Create(JNIEnv* env, jobject object);

  jobject java_object() const { return object_.obj(); }

  ftl::RefPtr<JniClass> GetObjectClass();

  ftl::RefPtr<JniObject> GetObjectField(jfieldID fieldId);
  bool GetBooleanField(jfieldID fieldId);
  int64_t GetByteField(jfieldID fieldId);
  int64_t GetCharField(jfieldID fieldId);
  int64_t GetShortField(jfieldID fieldId);
  int64_t GetIntField(jfieldID fieldId);
  int64_t GetLongField(jfieldID fieldId);
  double GetFloatField(jfieldID fieldId);
  double GetDoubleField(jfieldID fieldId);

  void SetObjectField(jfieldID fieldId, const JniObject* value);
  void SetBooleanField(jfieldID fieldId, bool value);
  void SetByteField(jfieldID fieldId, int64_t value);
  void SetCharField(jfieldID fieldId, int64_t value);
  void SetShortField(jfieldID fieldId, int64_t value);
  void SetIntField(jfieldID fieldId, int64_t value);
  void SetLongField(jfieldID fieldId, int64_t value);
  void SetFloatField(jfieldID fieldId, double value);
  void SetDoubleField(jfieldID fieldId, double value);

  ftl::RefPtr<JniObject> CallObjectMethod(jmethodID methodId,
                                          const std::vector<Dart_Handle>& args);
  bool CallBooleanMethod(jmethodID methodId,
                         const std::vector<Dart_Handle>& args);
  int64_t CallByteMethod(jmethodID methodId,
                         const std::vector<Dart_Handle>& args);
  int64_t CallCharMethod(jmethodID methodId,
                         const std::vector<Dart_Handle>& args);
  int64_t CallShortMethod(jmethodID methodId,
                          const std::vector<Dart_Handle>& args);
  int64_t CallIntMethod(jmethodID methodId,
                        const std::vector<Dart_Handle>& args);
  int64_t CallLongMethod(jmethodID methodId,
                         const std::vector<Dart_Handle>& args);
  double CallFloatMethod(jmethodID methodId,
                         const std::vector<Dart_Handle>& args);
  double CallDoubleMethod(jmethodID methodId,
                          const std::vector<Dart_Handle>& args);
  void CallVoidMethod(jmethodID methodId, const std::vector<Dart_Handle>& args);

 protected:
  JniObject(JNIEnv* env, jobject object);

  fml::jni::ScopedJavaGlobalRef<jobject> object_;
};

}  // namespace blink

#endif  // FLUTTER_LIB_JNI_JNI_OBJECT_H_
