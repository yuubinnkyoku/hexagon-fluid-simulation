# HVX implementation plan (not yet implemented)

The CPU baseline is intentionally independent from Qualcomm tooling.
This document records the first proposed HVX kernel, not a claim that
the code already runs on Hexagon.

## Numerical contract

- Input: FP32 or FP16 pressure p, RHS b, grid size, spacing.
- Operator: A p = (4*p - four neighbors) / h^2.
- Ghost boundary: p=0 (homogeneous Dirichlet).
- Initial p=0 for cold solves; warm-start is supported by the CPU API.
- Output: pressure p plus residual accuracy checked by the **CPU**
  double-accumulation reference.
- Solver stop: compare end-to-end time to a fixed relative residual.

## First HVX kernel

1. Pack contiguous X rows with aligned strides and halo/ghost values.
2. Read the up/center/down rows and left/right neighbors with vector
   loads and lane shifts; avoid scalar gathers where possible.
3. Fuse Jacobi's neighbor sum and RHS update in one pass.
4. Ping-pong two pressure buffers and keep intermediate steps on the
   Hexagon side; avoid launching a QNN graph per Jacobi iteration.
5. Use a tile working set sized to **measured available** VTCM/TCM,
   not an assumed advertised full-cache capacity.
6. Vectorize the residual operation and periodically compute its norm;
   confirm that reduction costs do not dominate at small grid sizes.
7. Compare FP16 and FP32 where supported by the deployed HVX toolchain.

## Evidence needed before describing it as accelerated

- Identified Qualcomm Hexagon SDK / HVX target toolchain and deployment.
- A traced kernel truly executing on HVX (not an ARM NEON fallback).
- Numerical agreement with CPU baseline on manufactured pressure fields.
- Benchmark including host/Hexagon transfers and synchronization.
- Comparable tests across 16^2, 32^2, 64^2 and larger grids.

HMX/QNN convolutional Jacobi is a separate experiment: a convolution
operation does **not** guarantee HMX allocation. The correct mapping
must be established through profiling.
