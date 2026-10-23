# Measure the Android CPU reference

This project is not yet an Android GUI application. The Android build
is a standalone **arm64 CPU executable**, not an NPU/HVX/HMX kernel.

## Option A: download the GitHub Actions Android artifact (no NDK needed)

1. Open the repository's **Actions** tab.
2. Open a successful **Android arm64 CPU executable** run.
3. Download the **hfs-bench-android-arm64** artifact (sign in if necessary).
4. Extract the zip: it contains the `hfs_bench` Android executable.
5. Connect an arm64 Android phone by USB, enable USB debugging, and verify
   `adb devices` lists it as `device`.
6. Run the repeatable measurement script:

```sh
python3 scripts/bench_android.py --binary ./hfs_bench --sizes 16,32,64 --runs 5
```

Windows PowerShell is also supported using `python` instead of `python3`.
The tool only needs Python 3.9+ and adb in PATH; it uses the standard library.

Run with `--help` to see options, including `--adb adb.exe`, `--serial`,
`--max-iterations`, `--tolerance`, and `--output-dir`.
Keep the screen and power conditions comparable when benchmarking.

The script pushes the executable to `/data/local/tmp/hfs_bench`, performs
a warm-up, records five runs at each size, and saves a JSON report under
`benchmark-results/`. It does not require root access. Reported
`solver_wall_time_ms` is measured on the phone and **excludes adb latency**.

Important: the low-frequency manufactured test case converges slowly.
Each output has `converged` and `relative_residual` fields.
If `converged=false`, the run is a **fixed-iteration microbenchmark only**
and must not be presented as a matched-accuracy solver comparison.
Increase `--max-iterations` when studying time-to-tolerance.

The GitHub Actions artifact expires after 14 days; rerun the workflow
to create a new one.

## Option B: local Android NDK build

Requirements: Android NDK with C++20 support, CMake >= 3.22, adb.

Set `ANDROID_NDK_HOME` to the NDK path, then:

```sh
cmake -S . -B build/android-arm64 \
  -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-29 \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=OFF
cmake --build build/android-arm64 -j
python3 scripts/bench_android.py --binary build/android-arm64/hfs_bench
```

For one manual run:

```sh
adb push build/android-arm64/hfs_bench /data/local/tmp/hfs_bench
adb shell chmod 700 /data/local/tmp/hfs_bench
adb shell /data/local/tmp/hfs_bench 64 64 10000 1e-4
```

## What has not been measured

The tool does not measure phone power draw, energy per solve, GPU rendering,
GPU synchronization or the NPU. It cannot infer actual HVX/HMX use.
Android NDK builds run on the **CPU**.

Do not publish benchmark speedups without device name, Android version,
number of repeated trials, residual target, convergence status, and
same-accuracy comparisons.
