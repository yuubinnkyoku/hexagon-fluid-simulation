# Build the CPU reference for Android

This builds the standalone C++ CPU baseline for an arm64 Android device.
It does **not** run any work on HVX or HMX.

Prerequisites: Android NDK with C++20 support, CMake >= 3.22, adb.

Set ANDROID_NDK_HOME to your installed Android NDK path.

    cmake -S . -B build/android-arm64 \
      -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
      -DANDROID_ABI=arm64-v8a \
      -DANDROID_PLATFORM=android-29 \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_TESTING=OFF
    cmake --build build/android-arm64 -j

Then run:

    adb push build/android-arm64/hfs_bench /data/local/tmp/hfs_bench
    adb shell chmod 755 /data/local/tmp/hfs_bench
    adb shell /data/local/tmp/hfs_bench 64 64 1000 1e-4

The numbers are: width, height, maximum iterations, relative tolerance.
The CLI measures native solver execution only; it excludes renderer,
Android application startup and Kotlin/JNI overhead.

Hexagon-specific SDK/toolchains, runtime deployment and a Kotlin app
will be added in later phases. Do not commit proprietary SDK binaries.
