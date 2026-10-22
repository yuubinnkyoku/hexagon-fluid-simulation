#pragma once
#include "hfs/poisson.hpp"
#include <cstddef>
#include <span>

namespace hfs {
struct JacobiOptions {
    std::size_t max_iterations{5000};
    double tolerance{1e-5};
    std::size_t check_every{10};
};
struct SolveResult {
    std::size_t iterations{};
    double residual{};
    bool converged{};
};
// Pressure is an in/out buffer, allowing a warm-start from another solver.
[[nodiscard]] SolveResult solve_jacobi(
    const Grid& grid, std::span<const float> rhs,
    std::span<float> pressure, const JacobiOptions& options = {});
}  // namespace hfs
