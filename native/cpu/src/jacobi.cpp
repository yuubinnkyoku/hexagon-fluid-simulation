#include "hfs/jacobi.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace hfs {
SolveResult solve_jacobi(
    const Grid& grid, std::span<const float> rhs,
    std::span<float> pressure, const JacobiOptions& options) {
    if (rhs.size() != grid.cell_count() || pressure.size() != rhs.size()) {
        throw std::invalid_argument("field size does not match grid");
    }
    if (!std::isfinite(options.tolerance) || options.tolerance <= 0.0 ||
        options.check_every == 0) {
        throw std::invalid_argument("invalid tolerance or check_every");
    }
    double residual = pressure_residual(grid, pressure, rhs);
    if (!std::isfinite(residual)) {
        throw std::invalid_argument("non-finite input field");
    }
    if (residual <= options.tolerance) return {0, residual, true};
    std::vector<float> current(pressure.begin(), pressure.end());
    std::vector<float> next(pressure.size(), 0.0f);
    const float h2 = grid.spacing * grid.spacing;
    std::size_t performed = 0;

    for (std::size_t iter = 1; iter <= options.max_iterations; ++iter) {
        for (std::size_t y = 0; y < grid.height; ++y) {
            for (std::size_t x = 0; x < grid.width; ++x) {
                const auto i = y * grid.width + x;
                float neighbors = 0.0f;
                if (x > 0) neighbors += current[i - 1];
                if (x + 1 < grid.width) neighbors += current[i + 1];
                if (y > 0) neighbors += current[i - grid.width];
                if (y + 1 < grid.height) neighbors += current[i + grid.width];
                next[i] = 0.25f * (neighbors + h2 * rhs[i]);
            }
        }
        current.swap(next);
        performed = iter;
        if (iter % options.check_every == 0 ||
            iter == options.max_iterations) {
            residual = pressure_residual(grid, current, rhs);
            if (!std::isfinite(residual) || residual <= options.tolerance) {
                break;
            }
        }
    }
    std::copy(current.begin(), current.end(), pressure.begin());
    return {performed, residual,
            std::isfinite(residual) && residual <= options.tolerance};
}
}  // namespace hfs
