/*
 * Copyright 2016 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "vr/gvr/demos/controller_paint/jni/app_jni.h"

#include <memory>

#include "vr/gvr/demos/controller_paint/jni/demoapp.h"
#include "vr/gvr/demos/controller_paint/jni/utils.h"

namespace {

std::unique_ptr<DemoApp> demo_app_;

}  // namespace

NATIVE_METHOD(void, nativeOnCreate)(JNIEnv* env, jobject obj,
                                    jobject asset_mgr, jlong gvr_context_ptr) {
  CHECK(!demo_app_);
  CHECK(gvr_context_ptr);
  demo_app_.reset(new DemoApp(env, asset_mgr, gvr_context_ptr));
}

NATIVE_METHOD(void, nativeOnResume)(JNIEnv* env, jobject obj) {
  CHECK(demo_app_);
  demo_app_->OnResume();
}

NATIVE_METHOD(void, nativeOnPause)(JNIEnv* env, jobject obj) {
  CHECK(demo_app_);
  demo_app_->OnPause();
}

NATIVE_METHOD(void, nativeOnSurfaceCreated)(JNIEnv* env, jobject obj) {
  CHECK(demo_app_);
  demo_app_->OnSurfaceCreated();
}

NATIVE_METHOD(void, nativeOnSurfaceChanged)(JNIEnv* env, jobject obj,
                                            jint width, jint height) {
  CHECK(demo_app_);
  demo_app_->OnSurfaceChanged(static_cast<int>(width),
                              static_cast<int>(height));
}

NATIVE_METHOD(void, nativeOnDrawFrame)(JNIEnv* env, jobject obj) {
  CHECK(demo_app_);
  demo_app_->OnDrawFrame();
}

NATIVE_METHOD(void, nativeOnDestroy)(JNIEnv* env, jobject obj) {
  CHECK(demo_app_);
  demo_app_.reset(nullptr);
}

