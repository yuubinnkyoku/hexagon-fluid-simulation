#include "hfs/jacobi.hpp"

#include <jni.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <numbers>
#include <sstream>
#include <stdexcept>
#include <vector>

extern "C" JNIEXPORT jstring JNICALL
Java_io_github_yuubinnkyoku_hexagonfluid_FluidNative_run(
        JNIEnv* env, jobject /* receiver */, jint size,
        jint max_iterations, jdouble tolerance) {
    try {
        if (size < 1 || size > 512 || max_iterations < 1 ||
            max_iterations > 200000 || !std::isfinite(tolerance) ||
            tolerance <= 0.0 || tolerance >= 1.0) {
            throw std::invalid_argument("invalid benchmark parameters");
        }

        const auto width = static_cast<std::size_t>(size);
        const hfs::Grid grid{width, width, 1.0f / static_cast<float>(width + 1)};
        const auto count = grid.cell_count();
        std::vector<float> exact(count), rhs(count), pressure(count, 0.0f);

        // Create an exactly consistent discrete RHS outside the timed region.
        for (std::size_t y = 0; y < width; ++y) {
            for (std::size_t x = 0; x < width; ++x) {
                const double sx = std::sin(std::numbers::pi *
                                           static_cast<double>(x + 1) / (width + 1));
                const double sy = std::sin(std::numbers::pi *
                                           static_cast<double>(y + 1) / (width + 1));
                exact[y * width + x] = static_cast<float>(sx * sy);
            }
        }
        hfs::apply_negative_laplacian(grid, exact, rhs);

        const auto started = std::chrono::steady_clock::now();
        const auto solved = hfs::solve_jacobi(
            grid, rhs, pressure,
            {.max_iterations = static_cast<std::size_t>(max_iterations),
             .tolerance = tolerance, .check_every = 10});
        const auto finished = std::chrono::steady_clock::now();

        double max_error = 0.0;
        for (std::size_t i = 0; i < count; ++i) {
            max_error = std::max(max_error,
                                 std::abs(static_cast<double>(pressure[i]) - exact[i]));
        }
        const double elapsed_ms =
            std::chrono::duration<double, std::milli>(finished - started).count();

        std::ostringstream output;
        output << std::setprecision(12)
               << "{\"backend\":\"cpu_jacobi\","
               << "\"grid\":" << size << ","
               << "\"max_iterations\":" << max_iterations << ","
               << "\"iterations\":" << solved.iterations << ","
               << "\"converged\":" << (solved.converged ? "true" : "false") << ","
               << "\"relative_residual\":" << solved.residual << ","
               << "\"max_abs_pressure_error\":" << max_error << ","
               << "\"solver_wall_time_ms\":" << elapsed_ms
               << "}";
        return env->NewStringUTF(output.str().c_str());
    } catch (const std::exception& e) {
        const jclass exception = env->FindClass("java/lang/IllegalArgumentException");
        if (exception != nullptr) {
            env->ThrowNew(exception, e.what());
        }
        return nullptr;
    }
}
