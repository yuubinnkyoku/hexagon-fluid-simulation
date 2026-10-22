#pragma once
#include <cstddef>
#include <span>

namespace hfs {
// Interior cells only. Ghost cells outside the grid have pressure = 0.
struct Grid {
    std::size_t width{};
    std::size_t height{};
    float spacing{1.0f};
    void validate() const;
    [[nodiscard]] std::size_t cell_count() const;
};
// Positive-definite negative Laplacian: (4*p-left-right-up-down)/h^2.
// Projection-method RHS: -(density/dt) * divergence(u_star).
void apply_negative_laplacian(
    const Grid& grid, std::span<const float> pressure,
    std::span<float> result);
// ||A*p-rhs||_2 / ||rhs||_2, absolute if rhs == 0.
[[nodiscard]] double pressure_residual(
    const Grid& grid, std::span<const float> pressure,
    std::span<const float> rhs);
}  // namespace hfs
