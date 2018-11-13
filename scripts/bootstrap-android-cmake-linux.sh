#!/bin/bash

# Edit path to Android NDK
ANDROID_NDK_ROOT=$HOME/local/android-ndk-r16b/

# CMake 3.6 or later required.
CMAKE_BIN=$HOME/local/cmake-3.12.4-Linux-x86_64/bin/cmake

rm -rf android-build

$CMAKE_BIN -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_NATIVE_API_LEVEL=24 \
  -DANDROID_ARM_MODE=arm \
  -DANDROID_ARM_NEON=TRUE \
  -DANDROID_STL=c++_shared \
  -DEMBREE_ISPC_SUPPORT=Off \
  -DEMBREE_TASKING_SYSTEM=Internal \
  -DEMBREE_TUTORIALS=Off \
  -DEMBREE_MAX_ISA=SSE2 \
  -Bandroid-build -H.

