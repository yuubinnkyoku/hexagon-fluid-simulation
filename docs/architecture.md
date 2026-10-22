# Architecture and benchmarking rules

## Numerical definition

Phase 0 solves A p = b where A is the **negative** five-point Laplacian
on a uniform 2D grid with *zero Dirichlet ghost cells*. The unknowns
represent interior cells. The CPU baseline stores FP32 fields and uses
double accumulation for residual norms.

This is a **pressure solver benchmark**, not yet a free-surface fluid engine.

## Planned modules

| Role | Technology | Status |
| --- | --- | --- |
| Numeric operator | C++20 | Implemented |
| CPU Jacobi | C++20 | Implemented |
| Native benchmark and tests | CMake / CTest | Implemented |
| Execution management | Android Kotlin / JNI | Planned |
| HSA | Hexagon-side scheduling/control | Planned |
| HVX | Fused stencils, residual, multigrid | Planned |
| HMX | QNN convolution and learned initial guesses | Planned |
| GPU | Vulkan compute and rendering | Planned |

HTP may assign a graph operation to HVX or HMX internally.
Confirm the real mapping using device profiling; do not infer it from
the name of the QNN operation.

## Fair evaluation

- Use identical A, b, spacing and boundary conditions across all backends.
- Report pressure error and relative residual (or absolute for zero RHS).
- For end-to-end comparison, report **time to fixed residual**, not merely
  time for a fixed number of Jacobi steps.
- Measure warm-up, graph compilation, sustained latency, memory transfers,
  thermal behavior and energy per solve separately.
- Device-to-device synchronization costs count toward system-level time.
- Learned predictions require held-out boundary conditions and resolution
  tests; verify fluid divergence after projection in a later simulation.

## Roadmap

1. CPU correctness, manufactured solution, residual tests (Phase 0).
2. HVX Jacobi with fused updates and device-resident intermediate buffers.
3. QNN convolution Jacobi, with profiling of actual HVX/HMX placement.
4. Multigrid restriction, smoothing, prolongation and correction.
5. HMX neural initial pressure plus HVX iterative correction.
6. Vulkan GPU reference and interactive Kotlin Android application.

The adaptive-grid and free-surface boundary problem is separate work:
results on this uniform Dirichlet grid do not automatically transfer.
