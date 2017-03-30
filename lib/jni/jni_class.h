// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_LIB_JNI_JNI_CLASS_H_
#define FLUTTER_LIB_JNI_JNI_CLASS_H_

#include <jni.h>

#include "flutter/lib/jni/jni_object.h"
#include "lib/tonic/dart_wrappable.h"

namespace blink {

// Wrapper that exposes a JNI jclass to Dart
class JniClass : public JniObject {
  DEFINE_WRAPPERTYPEINFO();
  friend class JniObject;
  FRIEND_MAKE_REF_COUNTED(JniClass);

 public:
  ~JniClass() override;

  static ftl::RefPtr<JniClass> FromName(const char* className);
  static ftl::RefPtr<JniClass> FromClassObject(const JniObject* clazz);

  jclass java_class() const { return static_cast<jclass>(object_.obj()); }

  intptr_t GetFieldId(const char* name, const char* sig);
  intptr_t GetStaticFieldId(const char* name, const char* sig);
  intptr_t GetMethodId(const char* name, const char* sig);
  intptr_t GetStaticMethodId(const char* name, const char* sig);

  ftl::RefPtr<JniObject> NewObject(jmethodID methodId,
                                   const std::vector<Dart_Handle>& args);

  bool IsAssignable(const JniClass* clazz);

  ftl::RefPtr<JniObject> GetStaticObjectField(jfieldID fieldId);
  bool GetStaticBooleanField(jfieldID fieldId);
  int64_t GetStaticByteField(jfieldID fieldId);
  int64_t GetStaticCharField(jfieldID fieldId);
  int64_t GetStaticShortField(jfieldID fieldId);
  int64_t GetStaticIntField(jfieldID fieldId);
  int64_t GetStaticLongField(jfieldID fieldId);
  double GetStaticFloatField(jfieldID fieldId);
  double GetStaticDoubleField(jfieldID fieldId);

  void SetStaticObjectField(jfieldID fieldId, const JniObject* value);
  void SetStaticBooleanField(jfieldID fieldId, bool value);
  void SetStaticByteField(jfieldID fieldId, int64_t value);
  void SetStaticCharField(jfieldID fieldId, int64_t value);
  void SetStaticShortField(jfieldID fieldId, int64_t value);
  void SetStaticIntField(jfieldID fieldId, int64_t value);
  void SetStaticLongField(jfieldID fieldId, int64_t value);
  void SetStaticFloatField(jfieldID fieldId, double value);
  void SetStaticDoubleField(jfieldID fieldId, double value);

  ftl::RefPtr<JniObject> CallStaticObjectMethod(
      jmethodID methodId,
      const std::vector<Dart_Handle>& args);
  bool CallStaticBooleanMethod(jmethodID methodId,
                               const std::vector<Dart_Handle>& args);
  int64_t CallStaticByteMethod(jmethodID methodId,
                               const std::vector<Dart_Handle>& args);
  int64_t CallStaticCharMethod(jmethodID methodId,
                               const std::vector<Dart_Handle>& args);
  int64_t CallStaticShortMethod(jmethodID methodId,
                                const std::vector<Dart_Handle>& args);
  int64_t CallStaticIntMethod(jmethodID methodId,
                              const std::vector<Dart_Handle>& args);
  int64_t CallStaticLongMethod(jmethodID methodId,
                               const std::vector<Dart_Handle>& args);
  double CallStaticFloatMethod(jmethodID methodId,
                               const std::vector<Dart_Handle>& args);
  double CallStaticDoubleMethod(jmethodID methodId,
                                const std::vector<Dart_Handle>& args);
  void CallStaticVoidMethod(jmethodID methodId,
                            const std::vector<Dart_Handle>& args);

 private:
  JniClass(JNIEnv* env, jclass clazz);
};

}  // namespace blink

#endif  // FLUTTER_LIB_JNI_JNI_CLASS_H_
