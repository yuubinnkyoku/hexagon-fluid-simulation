# Hexagon Fluid Simulation

Research prototype for accelerating the **pressure Poisson solve** of incompressible fluid simulation on Android, with a focus on Snapdragon Hexagon (HSA / HVX / HMX).

> Status: **Phase 0 — portable CPU reference**. No HVX, HMX, QNN, Vulkan or Android application implementation is included yet. Performance figures must come from reproducible measurements, not theoretical TOPS.

## Research question

Can a smartphone's Hexagon NPU solve the pressure Poisson equation faster and/or more energy-efficiently than a CPU or GPU **at comparable numerical accuracy**?

We use a **positive-definite**, five-point, 2D discrete operator:

```text
A p = b
(Ap)[x,y] = (4 p[x,y] - p[x-1,y] - p[x+1,y]
             - p[x,y-1] - p[x,y+1]) / h^2
```

The grid holds **interior** cells only; all outside neighbors use a zero Dirichlet boundary. This is a controlled research benchmark, **not yet a full fluid simulator**. For a projection method written as `laplacian(p) = (rho / dt) * div(u*)`, our `b` is the **negative** of that right-hand side.

## Build and run (desktop)

Requirements: CMake >= 3.22 and a C++20 compiler (GCC, Clang, or MSVC).

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
ctest --test-dir build -C Release --output-on-failure
./build/hfs_bench 64 64 1000 1e-4
```

On Windows with a multi-config generator, run `build/Release/hfs_bench.exe` instead. Arguments are: `width height max_iterations relative_tolerance`. The benchmark prints timing, iterations, relative residual, and pressure error against a manufactured reference solution.

## Layout

- `native/core/`: grid definition, matrix-free `A p`, and residual calculation (FP32).
- `native/cpu/`: deterministic CPU Jacobi reference with configurable stopping criteria and optional initial pressure.
- `tests/`: correctness tests, manufactured solutions, zero input, error handling.
- `benchmarks/`: repeatable command-line baseline benchmark.
- `docs/architecture.md`: proposed Android / Hexagon backends and fair comparison methodology.
- `docs/android-build.md`: preparing an Android NDK build.
- `.github/workflows/ci.yml`: automatic build and test checks.

## Research roadmap

1. Validate CPU reference and residual/error metrics.
2. Implement **HVX** Jacobi and multigrid, preferably keeping iterations and intermediate buffers on-device.
3. Compare with a QNN/HTP convolution implementation (the actual HVX/HMX mapping needs profiling).
4. Compare GPU (Vulkan compute), CPU, HVX, HMX/QNN at **matched residual** and measure full-path latency, transfers, power and thermal stability.
5. Explore HMX neural initial guesses plus HVX corrections; add an interactive Kotlin application.

## Numerical accuracy & fairness

Report grid dimensions, operator convention, boundary conditions, dtype, stopping tolerance, `||A p - b||_2 / ||b||_2` (or absolute residual if `b = 0`), iterations, and wall-clock time. A faster solver that produces a worse pressure field is **not** a fair speedup. All timing reported by the reference CLI includes the solver and its convergence checks, but **excludes** synthetic input generation.

## License

No license has been selected yet. Do not assume permission to redistribute or reuse this code outside the terms established by the repository owner.
