#include "hfs/jacobi.hpp"
#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
template <typename Fn>
void expect_invalid_argument(Fn fn) {
    try { fn(); }
    catch (const std::invalid_argument&) { return; }
    throw std::runtime_error("expected invalid_argument");
}
void test_stencil() {
    const hfs::Grid grid{3, 3, 1.0f};
    std::vector<float> p(9, 0), out(9);
    p[4] = 1;
    hfs::apply_negative_laplacian(grid, p, out);
    const std::vector<float> expected{0,-1,0,-1,4,-1,0,-1,0};
    require(out == expected, "stencil signs or neighbors are incorrect");
}
void test_manufactured_solution() {
    const hfs::Grid grid{16, 12, 1.0f / 17.0f};
    const auto n = grid.cell_count();
    std::vector<float> exact(n), rhs(n), pressure(n, 0);
    for (std::size_t y = 0; y < grid.height; ++y) {
        for (std::size_t x = 0; x < grid.width; ++x) {
            const double u = std::sin(std::numbers::pi * (x + 1) / (grid.width + 1));
            const double v = std::sin(std::numbers::pi * (y + 1) / (grid.height + 1));
            exact[y * grid.width + x] = static_cast<float>(u * v);
        }
    }
    hfs::apply_negative_laplacian(grid, exact, rhs);
    const auto result = hfs::solve_jacobi(grid, rhs, pressure,
                            {.max_iterations=5000, .tolerance=1e-5, .check_every=10});
    require(result.converged, "manufactured solution did not converge");
    require(result.residual <= 1e-5, "residual above tolerance");
    double max_error = 0;
    for (std::size_t i = 0; i < n; ++i) {
        max_error = std::max(max_error, std::abs(double(exact[i]) - pressure[i]));
    }
    require(max_error < 2e-4, "pressure error above expected bound");
}
void test_zero_rhs() {
    const hfs::Grid grid{8, 8, 1.0f};
    std::vector<float> rhs(grid.cell_count(), 0), pressure(grid.cell_count(), 0);
    const auto initial = hfs::solve_jacobi(grid, rhs, pressure);
    require(initial.converged && initial.iterations == 0 && initial.residual == 0,
            "zero pressure/RHS should converge immediately");
    std::fill(pressure.begin(), pressure.end(), 1.0f);
    const auto solved = hfs::solve_jacobi(grid, rhs, pressure,
                            {.max_iterations=2000, .tolerance=1e-5, .check_every=5});
    require(solved.converged, "zero RHS with nonzero initial guess did not converge");
    require(hfs::pressure_residual(grid, pressure, rhs) <= 1e-5,
            "zero RHS must use absolute residual");
}
void test_iteration_limit() {
    const hfs::Grid grid{8, 8, 1.0f};
    std::vector<float> rhs(grid.cell_count(), 1), pressure(grid.cell_count(), 0);
    const auto result = hfs::solve_jacobi(grid, rhs, pressure,
                           {.max_iterations=3, .tolerance=1e-10, .check_every=10});
    require(result.iterations == 3 && !result.converged, "iteration limit ignored");
    require(result.residual < 1.0, "residual failed to decrease");
    require(pressure[4 * grid.width + 4] > 0, "positive RHS must yield positive pressure");
}
void test_validation() {
    expect_invalid_argument([] { hfs::Grid{0, 1, 1}.validate(); });
    expect_invalid_argument([] { hfs::Grid{1, 1, 0}.validate(); });
    const hfs::Grid grid{4, 4, 1};
    std::vector<float> rhs(16, 1), p(16, 0), short_p(15, 0);
    expect_invalid_argument([&] { (void)hfs::solve_jacobi(grid, rhs, short_p); });
    expect_invalid_argument([&] { (void)hfs::solve_jacobi(grid, rhs, p,
                            {.max_iterations=5, .tolerance=0, .check_every=10}); });
    expect_invalid_argument([&] { hfs::apply_negative_laplacian(grid, p, p); });
}
}  // namespace

int main() {
    const std::vector<std::pair<std::string, std::function<void()>>> tests{
        {"stencil", test_stencil},
        {"manufactured_solution", test_manufactured_solution},
        {"zero_rhs", test_zero_rhs},
        {"iteration_limit", test_iteration_limit},
        {"validation", test_validation},
    };
    std::size_t failed = 0;
    for (const auto& [name, test] : tests) {
        try { test(); std::cout << "[PASS] " << name << '\n'; }
        catch (const std::exception& e) {
            ++failed;
            std::cerr << "[FAIL] " << name << ": " << e.what() << '\n';
        }
    }
    return failed ? 1 : 0;
}
