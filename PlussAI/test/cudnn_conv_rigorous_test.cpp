#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

struct RigorLogger {
  explicit RigorLogger(std::filesystem::path path) : path_(std::move(path)), out_(path_, std::ios::out | std::ios::trunc) {}
  ~RigorLogger() { flush(); }

  void line(const std::string& msg) {
    const auto now = std::chrono::system_clock::now();
    const auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    out_ << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] " << msg << '\n';
    out_.flush();
  }

  void flush() { out_.flush(); }
  const std::filesystem::path& path() const noexcept { return path_; }
  bool good() const noexcept { return out_.good(); }

private:
  std::filesystem::path path_{};
  std::ofstream out_{};
};

std::filesystem::path resolve_log_path() {
  if (const char* env = std::getenv("PLUSS_AI_CUDNN_CONV_LOG_PATH"); env != nullptr && *env != '\0') {
    return std::filesystem::path(env);
  }
  return std::filesystem::current_path() / "plussai_cudnn_conv_rigorous.log";
}

std::string env_or(const char* name, const char* fallback = "<unset>") {
  const char* v = std::getenv(name);
  return (v != nullptr && *v != '\0') ? std::string(v) : std::string(fallback);
}

double max_abs_diff(const std::vector<float>& a, const std::vector<float>& b) {
  const std::size_t n = std::min(a.size(), b.size());
  double m = 0.0;
  for (std::size_t i = 0; i < n; ++i) {
    m = std::max(m, std::abs(static_cast<double>(a[i]) - static_cast<double>(b[i])));
  }
  return m;
}

double mean_abs_diff(const std::vector<float>& a, const std::vector<float>& b) {
  const std::size_t n = std::min(a.size(), b.size());
  if (n == 0) return 0.0;
  double acc = 0.0;
  for (std::size_t i = 0; i < n; ++i) {
    acc += std::abs(static_cast<double>(a[i]) - static_cast<double>(b[i]));
  }
  return acc / static_cast<double>(n);
}

struct ConvCase {
  std::string name{};
  std::vector<std::int64_t> x_shape{};
  std::vector<std::int64_t> w_shape{};
  std::vector<std::int64_t> b_shape{};
  std::array<std::int64_t, 2> stride{1, 1};
  std::array<std::int64_t, 2> padding{0, 0};
  std::array<std::int64_t, 2> dilation{1, 1};
  std::int64_t groups{1};
  std::uint64_t seed{1};
};

bool run_rigorous_cudnn_conv_parity(std::ostream* err = nullptr) {
  RigorLogger log(resolve_log_path());
  if (!log.good()) {
    if (err) *err << "failed to open cudnn rigorous log file\n";
    return false;
  }

  log.line("=== plussai cuDNN conv rigorous parity test ===");
  log.line("log_file=" + log.path().string());
  log.line(std::string("cuda_ready=") + (pluss::cuda::is_ready() ? "true" : "false"));
  log.line(std::string("cudnn_ready=") + (pluss::cuda::cudnn_ready() ? "true" : "false"));
  log.line(std::string("cudnn_conv_ready=") + (pluss::nn::cudnn_conv_ready() ? "true" : "false"));
  log.line(std::string("cudnn_conv_enabled=") + (pluss::nn::cudnn_conv_enabled() ? "true" : "false"));
  log.line(std::string("cudnn_conv_strict=") + (pluss::nn::cudnn_conv_strict() ? "true" : "false"));
  log.line("env.PLUSS_CUDA_NATIVE_CUDNN_CONV=" + env_or("PLUSS_CUDA_NATIVE_CUDNN_CONV"));
  log.line("env.PLUSS_CUDA_NATIVE_CUDNN_CONV_STRICT=" + env_or("PLUSS_CUDA_NATIVE_CUDNN_CONV_STRICT"));
  log.line("env.PLUSS_CUDA_ALLOW_CPU_BRIDGE=" + env_or("PLUSS_CUDA_ALLOW_CPU_BRIDGE"));
  log.line("cuda.device_count=" + std::to_string(pluss::cuda::device_count()));
  log.line("cuda.runtime_version=" + std::to_string(pluss::cuda::runtime_version()));
  log.line("cuda.driver_version=" + std::to_string(pluss::cuda::driver_version()));
  log.line(std::string("cuda.device_name=") + pluss::cuda::primary_device_name());
  log.line(std::string("cudnn_error=") + pluss::cuda::cudnn_error());

  if (!pluss::cuda::is_ready() || !pluss::cuda::cudnn_ready() || !pluss::nn::cudnn_conv_ready()) {
    log.line("SKIP: CUDA/cuDNN/route not ready on this host. Test marked pass-by-skip.");
    return true;
  }

  const auto cfg_cpu = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU);
  const std::vector<ConvCase> cases{
      ConvCase{
          .name = "base_groups2",
          .x_shape = {2, 4, 16, 16},
          .w_shape = {8, 2, 3, 3},
          .b_shape = {8},
          .stride = {1, 1},
          .padding = {1, 1},
          .dilation = {1, 1},
          .groups = 2,
          .seed = 7001ULL,
      },
      ConvCase{
          .name = "depthwise_like",
          .x_shape = {2, 4, 15, 15},
          .w_shape = {4, 1, 5, 5},
          .b_shape = {4},
          .stride = {1, 1},
          .padding = {2, 2},
          .dilation = {1, 1},
          .groups = 4,
          .seed = 7003ULL,
      },
      ConvCase{
          .name = "strided_dilated",
          .x_shape = {1, 6, 21, 19},
          .w_shape = {12, 3, 3, 5},
          .b_shape = {12},
          .stride = {2, 1},
          .padding = {2, 3},
          .dilation = {2, 1},
          .groups = 2,
          .seed = 7007ULL,
      },
  };

  constexpr double out_max_tol = 5e-3;
  constexpr double out_mean_tol = 8e-4;
  constexpr double grad_max_tol = 2e-2;
  constexpr double grad_mean_tol = 3e-3;

  for (const auto& tc : cases) {
    try {
      log.line("CASE begin: " + tc.name);
      const pluss::Series x_cpu = pluss::series::rand(tc.x_shape, cfg_cpu, tc.seed);
      const pluss::Series w_cpu = pluss::series::rand(tc.w_shape, cfg_cpu, tc.seed + 1ULL);
      const pluss::Series b_cpu = pluss::series::rand(tc.b_shape, cfg_cpu, tc.seed + 2ULL);

      const pluss::Series y_cpu = pluss::nn::conv(x_cpu, w_cpu, b_cpu, tc.stride, tc.padding, tc.dilation, tc.groups);
      const pluss::Series y_gpu = pluss::nn::conv(
          x_cpu.to(pluss::pCUDA),
          w_cpu.to(pluss::pCUDA),
          b_cpu.to(pluss::pCUDA),
          tc.stride,
          tc.padding,
          tc.dilation,
          tc.groups);

      const std::vector<float> out_ref = y_cpu.to(pluss::pCPU, pluss::ScalarType::Float32).to_vec<float>();
      const std::vector<float> out_val = y_gpu.to(pluss::pCPU, pluss::ScalarType::Float32).to_vec<float>();
      const double out_max = max_abs_diff(out_ref, out_val);
      const double out_mean = mean_abs_diff(out_ref, out_val);
      log.line(
          "CASE output: " + tc.name +
          " max_abs_diff=" + std::to_string(out_max) +
          " mean_abs_diff=" + std::to_string(out_mean));
      if (out_max > out_max_tol || out_mean > out_mean_tol) {
        log.line("ERROR output parity failure in case " + tc.name);
        if (err) *err << "output parity failure in " << tc.name << '\n';
        return false;
      }

      auto x_cpu_g = x_cpu.clone();
      auto w_cpu_g = w_cpu.clone();
      auto b_cpu_g = b_cpu.clone();
      x_cpu_g.tracks_slope_(true);
      w_cpu_g.tracks_slope_(true);
      b_cpu_g.tracks_slope_(true);
      x_cpu_g.clear_sensitivity();
      w_cpu_g.clear_sensitivity();
      b_cpu_g.clear_sensitivity();
      pluss::nn::conv(x_cpu_g, w_cpu_g, b_cpu_g, tc.stride, tc.padding, tc.dilation, tc.groups).sum().reverse();
      const std::vector<float> gx_ref = x_cpu_g.sensitivity().to(pluss::pCPU, pluss::ScalarType::Float32).to_vec<float>();
      const std::vector<float> gw_ref = w_cpu_g.sensitivity().to(pluss::pCPU, pluss::ScalarType::Float32).to_vec<float>();
      const std::vector<float> gb_ref = b_cpu_g.sensitivity().to(pluss::pCPU, pluss::ScalarType::Float32).to_vec<float>();

      auto x_gpu_g = x_cpu.to(pluss::pCUDA);
      auto w_gpu_g = w_cpu.to(pluss::pCUDA);
      auto b_gpu_g = b_cpu.to(pluss::pCUDA);
      x_gpu_g.tracks_slope_(true);
      w_gpu_g.tracks_slope_(true);
      b_gpu_g.tracks_slope_(true);
      x_gpu_g.clear_sensitivity();
      w_gpu_g.clear_sensitivity();
      b_gpu_g.clear_sensitivity();
      pluss::nn::conv(x_gpu_g, w_gpu_g, b_gpu_g, tc.stride, tc.padding, tc.dilation, tc.groups).sum().reverse();
      const std::vector<float> gx_val = x_gpu_g.sensitivity().to(pluss::pCPU, pluss::ScalarType::Float32).to_vec<float>();
      const std::vector<float> gw_val = w_gpu_g.sensitivity().to(pluss::pCPU, pluss::ScalarType::Float32).to_vec<float>();
      const std::vector<float> gb_val = b_gpu_g.sensitivity().to(pluss::pCPU, pluss::ScalarType::Float32).to_vec<float>();

      const double gx_max = max_abs_diff(gx_ref, gx_val);
      const double gx_mean = mean_abs_diff(gx_ref, gx_val);
      const double gw_max = max_abs_diff(gw_ref, gw_val);
      const double gw_mean = mean_abs_diff(gw_ref, gw_val);
      const double gb_max = max_abs_diff(gb_ref, gb_val);
      const double gb_mean = mean_abs_diff(gb_ref, gb_val);
      log.line(
          "CASE grad: " + tc.name +
          " gx(max=" + std::to_string(gx_max) + ",mean=" + std::to_string(gx_mean) + ")" +
          " gw(max=" + std::to_string(gw_max) + ",mean=" + std::to_string(gw_mean) + ")" +
          " gb(max=" + std::to_string(gb_max) + ",mean=" + std::to_string(gb_mean) + ")");

      if (gx_max > grad_max_tol || gx_mean > grad_mean_tol ||
          gw_max > grad_max_tol || gw_mean > grad_mean_tol ||
          gb_max > grad_max_tol || gb_mean > grad_mean_tol) {
        log.line("ERROR gradient parity failure in case " + tc.name);
        if (err) *err << "gradient parity failure in " << tc.name << '\n';
        return false;
      }

      log.line("CASE pass: " + tc.name);
    } catch (const std::exception& ex) {
      log.line(std::string("EXCEPTION in case ") + tc.name + ": " + ex.what());
      if (err) *err << "exception in " << tc.name << ": " << ex.what() << '\n';
      return false;
    } catch (...) {
      log.line(std::string("EXCEPTION in case ") + tc.name + ": unknown");
      if (err) *err << "unknown exception in " << tc.name << '\n';
      return false;
    }
  }

  log.line("ALL CASES PASS");
  log.line("=== end ===");
  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("cuDNN conv rigorous parity writes diagnostic log") {
  REQUIRE(run_rigorous_cudnn_conv_parity(&std::cerr));
}
#else
int main() {
  if (!run_rigorous_cudnn_conv_parity(&std::cerr)) return 1;
  return 0;
}
#endif
