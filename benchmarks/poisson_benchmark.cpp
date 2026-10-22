#include "hfs/jacobi.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <vector>

int main(int argc, char** argv) {
    try {
        if (argc != 1 && argc != 5) {
            std::cerr << "Usage: hfs_bench [width height max_iterations tolerance]\n";
            return 2;
        }
        const std::size_t width = argc == 5 ? std::stoull(argv[1]) : 64;
        const std::size_t height = argc == 5 ? std::stoull(argv[2]) : 64;
        const std::size_t limit = argc == 5 ? std::stoull(argv[3]) : 1000;
        const double tolerance = argc == 5 ? std::stod(argv[4]) : 1e-4;
        const hfs::Grid grid{width, height,
                1.0f / static_cast<float>(std::max(width, height) + 1)};
        const auto n = grid.cell_count();
        std::vector<float> exact(n), rhs(n), pressure(n, 0.0f);
        for (std::size_t y = 0; y < height; ++y) {
            for (std::size_t x = 0; x < width; ++x) {
                const double a = std::sin(std::numbers::pi * (x + 1) / (width + 1));
                const double b = std::sin(std::numbers::pi * (y + 1) / (height + 1));
                exact[y * width + x] = static_cast<float>(a * b);
            }
        }
        hfs::apply_negative_laplacian(grid, exact, rhs);
        const auto t0 = std::chrono::steady_clock::now();
        const auto result = hfs::solve_jacobi(grid, rhs, pressure,
                {.max_iterations=limit, .tolerance=tolerance, .check_every=10});
        const auto t1 = std::chrono::steady_clock::now();
        double max_error = 0.0;
        for (std::size_t i = 0; i < n; ++i) {
            max_error = std::max(max_error, std::abs(double(pressure[i]) - exact[i]));
        }
        const auto milliseconds =
            std::chrono::duration<double, std::milli>(t1 - t0).count();
        std::cout << std::fixed << std::setprecision(8)
                  << "backend=cpu_jacobi\n"
                  << "grid=" << width << 'x' << height << '\n'
                  << "spacing=" << grid.spacing << '\n'
                  << "iterations=" << result.iterations << '\n'
                  << "converged=" << (result.converged ? "true" : "false") << '\n'
                  << "relative_residual=" << result.residual << '\n'
                  << "max_abs_pressure_error=" << max_error << '\n'
                  << "solver_wall_time_ms=" << milliseconds << '\n';
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
        return 2;
    }
}
