# Android standalone app (CPU benchmark)

The Android application embeds the C++ pressure solver using **JNI**.
ADB, root, an internet connection and a separate Python process are
**not required to run the installed app**.

## Getting the APK

1. Go to [Actions](../.github/workflows/android-apk.yml) and open a successful
   **Android standalone APK** run.
2. Download the artifact `hexagon-fluid-lab-debug-apk`.
3. Unzip and install `app-debug.apk` on an arm64 Android phone.
   Android may ask you to allow installing an app from your file manager.
4. Launch **Hexagon Fluid Lab**, select grid / iteration cap / repeat count,
   and tap **CPU性能を測定**.
5. The results include median, minimum and maximum solver time, number of
   iterations, residual, pressure error and convergence status.
6. Tap **JSON形式で結果を共有** to send the complete measurements to another app.

This is a **debug-signed experimental APK** built in GitHub Actions, not
a production release with a stable signing key. Reinstalling an APK signed
by a different build key may require uninstalling first. Do not put
personal information into the shared JSON: it includes device model,
Android version and a device build identifier.

## Building locally

Open the repo in Android Studio with Android SDK 36, NDK 27.2.12479018,
CMake 3.22.1 and JDK 17, or run with Gradle 8.13:

```sh
gradle :app:assembleDebug
```

On the first run Gradle downloads Android and Compose dependencies.
It produces `app/build/outputs/apk/debug/app-debug.apk`.

## What the app actually measures

- Uniform 2D grid, zero Dirichlet boundary.
- Manufactured pressure, discrete RHS, matrix-free five-point stencil.
- Jacobi FP32 with a maximum-iteration limit and tolerance 1e-4.
- One warm-up followed by repeated **in-process** native calls.
- `steady_clock` native time from the beginning to the end of the
  Jacobi solve, including its residual checks but **not** RHS generation.
- An **accuracy warning** is shown if convergence is not achieved.

The app does not display moving water or use HVX, HMX, QNN or Vulkan yet.
This baseline is essential for fair future hardware comparisons.
