# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.4.1)

# Configure the sample code.
file(GLOB native_srcs "src/main/jni/*.cc")
add_library(controllerpaint_jni
    SHARED
    ${native_srcs})

# Include the GVR headers & libraries.
include_directories(${GVR_INCLUDE})

add_library(gvr-lib SHARED IMPORTED)
set_target_properties(
    gvr-lib
    PROPERTIES IMPORTED_LOCATION ${GVR_LIBPATH}/${ANDROID_ABI}/libgvr.so)

# Include general Android libraries.
find_library(android-lib android)
find_library(EGL-lib EGL)
find_library(GLESv2-lib GLESv2)
find_library(log-lib log)

# Build final libcontrollerpaint_jni.so
target_link_libraries(controllerpaint_jni
    gvr-lib

    ${android-lib}
    ${EGL-lib}
    ${GLESv2-lib}
    ${log-lib} )
