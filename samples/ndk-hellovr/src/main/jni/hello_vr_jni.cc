/* Copyright 2018 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <android/log.h>
#include <jni.h>

#include <memory>

#include "hello_vr_app.h"  // NOLINT
#include "vr/gvr/capi/include/gvr.h"
#include "vr/gvr/capi/include/gvr_audio.h"

#define JNI_METHOD(return_type, method_name) \
  JNIEXPORT return_type JNICALL              \
      Java_com_google_vr_ndk_samples_hellovr_HelloVrActivity_##method_name

namespace {

inline jlong jptr(ndk_hello_vr::HelloVrApp *native_app) {
  return reinterpret_cast<intptr_t>(native_app);
}

inline ndk_hello_vr::HelloVrApp *native(jlong ptr) {
  return reinterpret_cast<ndk_hello_vr::HelloVrApp *>(ptr);
}

}  // anonymous namespace

extern "C" {

JNI_METHOD(jlong, nativeOnCreate)
(JNIEnv *env, jclass clazz, jobject class_loader, jobject android_context,
 jobject asset_mgr, jlong native_gvr_api) {
  std::unique_ptr<gvr::AudioApi> audio_context(new gvr::AudioApi);
  audio_context->Init(env, android_context, class_loader,
                      GVR_AUDIO_RENDERING_BINAURAL_HIGH_QUALITY);

  return jptr(new ndk_hello_vr::HelloVrApp(
      env, asset_mgr, reinterpret_cast<gvr_context *>(native_gvr_api),
      std::move(audio_context)));
}

JNI_METHOD(void, nativeOnDestroy)
(JNIEnv *env, jclass clazz, jlong native_app) { delete native(native_app); }

JNI_METHOD(void, nativeOnSurfaceCreated)
(JNIEnv *env, jobject obj, jlong native_app) {
  native(native_app)->OnSurfaceCreated(env);
}

JNI_METHOD(void, nativeOnDrawFrame)
(JNIEnv *env, jobject obj, jlong native_app) {
  native(native_app)->OnDrawFrame();
}

JNI_METHOD(void, nativeOnTriggerEvent)
(JNIEnv *env, jobject obj, jlong native_app) {
  native(native_app)->OnTriggerEvent();
}

JNI_METHOD(void, nativeOnPause)
(JNIEnv *env, jobject obj, jlong native_app) { native(native_app)->OnPause(); }

JNI_METHOD(void, nativeOnResume)
(JNIEnv *env, jobject obj, jlong native_app) { native(native_app)->OnResume(); }

}  // extern "C"
