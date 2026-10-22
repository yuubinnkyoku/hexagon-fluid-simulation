#include "hfs/poisson.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>

namespace hfs {
void Grid::validate() const {
    if (width == 0 || height == 0 ||
        width > std::numeric_limits<std::size_t>::max() / height) {
        throw std::invalid_argument("invalid grid dimensions");
    }
    if (!std::isfinite(spacing) || spacing <= 0.0f ||
        !std::isfinite(1.0f / (spacing * spacing))) {
        throw std::invalid_argument("invalid grid spacing");
    }
}
std::size_t Grid::cell_count() const {
    validate();
    return width * height;
}
namespace {
void check_size(const Grid& grid, std::span<const float> field) {
    if (field.size() != grid.cell_count()) {
        throw std::invalid_argument("field size does not match grid");
    }
}
double stencil(const Grid& grid, std::span<const float> p,
               std::size_t x, std::size_t y) {
    const auto i = y * grid.width + x;
    double sum = 4.0 * p[i];
    if (x > 0) sum -= p[i - 1];
    if (x + 1 < grid.width) sum -= p[i + 1];
    if (y > 0) sum -= p[i - grid.width];
    if (y + 1 < grid.height) sum -= p[i + grid.width];
    const double h = grid.spacing;
    return sum / (h * h);
}
}  // namespace
void apply_negative_laplacian(
    const Grid& grid, std::span<const float> pressure,
    std::span<float> result) {
    check_size(grid, pressure);
    if (result.size() != pressure.size()) {
        throw std::invalid_argument("invalid result size");
    }
    if (pressure.data() == result.data()) {
        throw std::invalid_argument("in-place stencil not supported");
    }
    for (std::size_t y = 0; y < grid.height; ++y) {
        for (std::size_t x = 0; x < grid.width; ++x) {
            result[y * grid.width + x] =
                static_cast<float>(stencil(grid, pressure, x, y));
        }
    }
}
double pressure_residual(
    const Grid& grid, std::span<const float> pressure,
    std::span<const float> rhs) {
    check_size(grid, pressure);
    check_size(grid, rhs);
    double err2 = 0.0, rhs2 = 0.0;
    for (std::size_t y = 0; y < grid.height; ++y) {
        for (std::size_t x = 0; x < grid.width; ++x) {
            const auto i = y * grid.width + x;
            const double err = stencil(grid, pressure, x, y) - rhs[i];
            err2 += err * err;
            rhs2 += static_cast<double>(rhs[i]) * rhs[i];
        }
    }
    const double error = std::sqrt(err2);
    return rhs2 > 0.0 ? error / std::sqrt(rhs2) : error;
}
}  // namespace hfs
