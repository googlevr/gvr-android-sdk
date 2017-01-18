#
# Copyright (C) The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

cmake_minimum_required(VERSION 3.4.1)

# now build app's shared lib
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")

add_library(controllerpaint_jni SHARED
            app_jni.cc
            demoapp.cc
            utils.cc)

target_include_directories(controllerpaint_jni
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../libraries/headers)

# add lib dependencies
target_link_libraries(controllerpaint_jni
                      android
                      EGL
                      GLESv2
                      log
                      ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../libraries/jni/${ANDROID_ABI}/libgvr.so)
