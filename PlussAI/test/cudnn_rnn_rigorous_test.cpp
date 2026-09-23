#include <pluss/pluss.h>
#include <pluss/ai.h>

#include "NodeK/kernels/cudnn_rnn.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <tuple>
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
  if (const char* env = std::getenv("PLUSS_AI_CUDNN_RNN_LOG_PATH"); env != nullptr && *env != '\0') {
    return std::filesystem::path(env);
  }
  return std::filesystem::current_path() / "plussai_cudnn_rnn_rigorous.log";
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

bool all_finite(const std::vector<float>& xs) {
  for (const auto v : xs) {
    if (!std::isfinite(static_cast<double>(v))) return false;
  }
  return true;
}

bool has_shape(const pluss::Series& x, std::initializer_list<std::int64_t> expected) {
  if (x.dim() != static_cast<std::int64_t>(expected.size())) return false;
  std::size_t i = 0;
  for (const auto d : expected) {
    if (x.sizes()[i++] != d) return false;
  }
  return true;
}

enum class Kind : std::uint8_t {
  RNN,
  GRU,
  LSTM,
};

struct RnnCase {
  std::string name{};
  Kind kind{Kind::RNN};
  std::int64_t seq{8};
  std::int64_t batch{4};
  std::int64_t input_size{3};
  std::int64_t hidden_size{5};
  std::int64_t proj_size{0};
  std::int64_t num_layers{1};
  bool bidirectional{false};
  double dropout{0.0};
  bool training{false};
  bool batch_first{false};
  bool use_bias{true};
  bool split_bias{true};
  bool pre_compute_input{true};
  std::string nonlinearity{"tanh"};
  std::uint64_t seed{1};
  bool parity_expected{true};
};

std::vector<pluss::ai::rnn::NetWeight> make_weights(const RnnCase& tc, const pluss::Config& cfg) {
  const std::int64_t gates = tc.kind == Kind::RNN ? 1 : (tc.kind == Kind::GRU ? 3 : 4);
  const std::int64_t dirs = tc.bidirectional ? 2 : 1;
  const bool projected_lstm = tc.kind == Kind::LSTM && tc.proj_size > 0;
  const std::int64_t hidden_out = projected_lstm ? tc.proj_size : tc.hidden_size;
  std::vector<pluss::ai::rnn::NetWeight> out;
  out.reserve(static_cast<std::size_t>(tc.num_layers * dirs));
  for (std::int64_t layer = 0; layer < tc.num_layers; ++layer) {
    const std::int64_t layer_input = (layer == 0) ? tc.input_size : hidden_out * dirs;
    for (std::int64_t dir = 0; dir < dirs; ++dir) {
      const std::uint64_t seed = tc.seed + static_cast<std::uint64_t>(29 * (layer * dirs + dir));
      out.push_back(pluss::ai::rnn::NetWeight{
          .weight_ih = pluss::series::rand({gates * tc.hidden_size, layer_input}, cfg, seed + 1ULL),
          .weight_hh = pluss::series::rand({gates * tc.hidden_size, hidden_out}, cfg, seed + 2ULL),
          .bias = (tc.use_bias && !tc.split_bias) ? pluss::series::rand({gates * tc.hidden_size}, cfg, seed + 3ULL) : pluss::Series(),
          .bias_ih = (tc.use_bias && tc.split_bias) ? pluss::series::rand({gates * tc.hidden_size}, cfg, seed + 3ULL) : pluss::Series(),
          .bias_hh = (tc.use_bias && tc.split_bias) ? pluss::series::rand({gates * tc.hidden_size}, cfg, seed + 4ULL) : pluss::Series(),
          .weight_hr = projected_lstm ? pluss::series::rand({hidden_out, tc.hidden_size}, cfg, seed + 5ULL) : pluss::Series(),
      });
    }
  }
  return out;
}

std::vector<pluss::ai::rnn::NetWeight> to_cuda_weights(const std::vector<pluss::ai::rnn::NetWeight>& ws) {
  std::vector<pluss::ai::rnn::NetWeight> out;
  out.reserve(ws.size());
  for (const auto& w : ws) {
    out.push_back(pluss::ai::rnn::NetWeight{
        .weight_ih = w.weight_ih.to(pluss::pCUDA),
        .weight_hh = w.weight_hh.to(pluss::pCUDA),
        .bias = w.bias.defined() ? w.bias.to(pluss::pCUDA) : pluss::Series(),
        .bias_ih = w.bias_ih.defined() ? w.bias_ih.to(pluss::pCUDA) : pluss::Series(),
        .bias_hh = w.bias_hh.defined() ? w.bias_hh.to(pluss::pCUDA) : pluss::Series(),
        .weight_hr = w.weight_hr.defined() ? w.weight_hr.to(pluss::pCUDA) : pluss::Series(),
    });
  }
  return out;
}

std::vector<float> as_f32_vec(const pluss::Series& s) {
  return s.to(pluss::pCPU, pluss::ScalarType::Float32).to_vec<float>();
}

bool run_rigorous_cudnn_rnn_parity(std::ostream* err = nullptr) {
  RigorLogger log(resolve_log_path());
  if (!log.good()) {
    if (err) *err << "failed to open cudnn rnn rigorous log file\n";
    return false;
  }

  log.line("=== plussai cuDNN rnn rigorous parity test ===");
  log.line("log_file=" + log.path().string());
  log.line(std::string("cuda_ready=") + (pluss::cuda::is_ready() ? "true" : "false"));
  log.line(std::string("cudnn_ready=") + (pluss::cuda::cudnn_ready() ? "true" : "false"));
  log.line(std::string("cudnn_rnn_ready=") + (pluss::nn::cudnn_rnn_ready() ? "true" : "false"));
  log.line(std::string("cudnn_rnn_enabled=") + (pluss::nn::cudnn_rnn_enabled() ? "true" : "false"));
  log.line(std::string("cudnn_rnn_strict=") + (pluss::nn::cudnn_rnn_strict() ? "true" : "false"));
  log.line("env.PLUSS_CUDA_NATIVE_CUDNN_RNN=" + env_or("PLUSS_CUDA_NATIVE_CUDNN_RNN"));
  log.line("env.PLUSS_CUDA_NATIVE_CUDNN_RNN_STRICT=" + env_or("PLUSS_CUDA_NATIVE_CUDNN_RNN_STRICT"));
  log.line("cuda.device_count=" + std::to_string(pluss::cuda::device_count()));
  log.line("cuda.runtime_version=" + std::to_string(pluss::cuda::runtime_version()));
  log.line("cuda.driver_version=" + std::to_string(pluss::cuda::driver_version()));
  log.line(std::string("cuda.device_name=") + pluss::cuda::primary_device_name());
  log.line(std::string("cudnn_error=") + pluss::cuda::cudnn_error());

  if (!pluss::cuda::is_ready() || !pluss::cuda::cudnn_ready() || !pluss::nn::cudnn_rnn_ready()) {
    log.line("SKIP: CUDA/cuDNN/RNN route not ready on this host. Test marked pass-by-skip.");
    return true;
  }

  const auto cfg_cpu = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU);
  const std::vector<RnnCase> cases{
      RnnCase{
          .name = "rnn_train_dropout_smoke",
          .kind = Kind::RNN,
          .seq = 6,
          .batch = 4,
          .input_size = 4,
          .hidden_size = 5,
          .num_layers = 2,
          .bidirectional = true,
          .dropout = 0.35,
          .training = true,
          .split_bias = true,
          .nonlinearity = "tanh",
          .seed = 9013ULL,
          .parity_expected = false,
      },
      RnnCase{
          .name = "gru_train_dropout_smoke",
          .kind = Kind::GRU,
          .seq = 6,
          .batch = 4,
          .input_size = 4,
          .hidden_size = 5,
          .num_layers = 2,
          .bidirectional = true,
          .dropout = 0.30,
          .training = true,
          .split_bias = true,
          .nonlinearity = "tanh",
          .seed = 9017ULL,
          .parity_expected = false,
      },
      RnnCase{
          .name = "lstm_train_dropout_smoke",
          .kind = Kind::LSTM,
          .seq = 6,
          .batch = 3,
          .input_size = 3,
          .hidden_size = 4,
          .num_layers = 2,
          .bidirectional = true,
          .dropout = 0.25,
          .training = true,
          .split_bias = true,
          .nonlinearity = "tanh",
          .seed = 9019ULL,
          .parity_expected = false,
      },
      RnnCase{
          .name = "lstm_proj_train_dropout_smoke",
          .kind = Kind::LSTM,
          .seq = 7,
          .batch = 3,
          .input_size = 4,
          .hidden_size = 6,
          .proj_size = 4,
          .num_layers = 2,
          .bidirectional = false,
          .dropout = 0.2,
          .training = true,
          .split_bias = true,
          .nonlinearity = "tanh",
          .seed = 9021ULL,
          .parity_expected = false,
      },
      RnnCase{
          .name = "rnn_tanh_split",
          .kind = Kind::RNN,
          .seq = 9,
          .batch = 5,
          .input_size = 4,
          .hidden_size = 6,
          .num_layers = 2,
          .bidirectional = true,
          .dropout = 0.25,
          .training = false,
          .split_bias = true,
          .nonlinearity = "tanh",
          .seed = 9001ULL,
          .parity_expected = true,
      },
      RnnCase{
          .name = "rnn_relu_deep",
          .kind = Kind::RNN,
          .seq = 7,
          .batch = 3,
          .input_size = 5,
          .hidden_size = 4,
          .num_layers = 3,
          .bidirectional = false,
          .dropout = 0.1,
          .training = false,
          .split_bias = false,
          .nonlinearity = "relu",
          .seed = 9003ULL,
          .parity_expected = true,
      },
      RnnCase{
          .name = "gru_bidir_split",
          .kind = Kind::GRU,
          .seq = 10,
          .batch = 4,
          .input_size = 3,
          .hidden_size = 5,
          .num_layers = 2,
          .bidirectional = true,
          .dropout = 0.2,
          .training = false,
          .split_bias = true,
          .nonlinearity = "tanh",
          .seed = 9007ULL,
          .parity_expected = true,
      },
      RnnCase{
          .name = "lstm_proj_inference_parity",
          .kind = Kind::LSTM,
          .seq = 9,
          .batch = 4,
          .input_size = 3,
          .hidden_size = 6,
          .proj_size = 4,
          .num_layers = 2,
          .bidirectional = false,
          .dropout = 0.0,
          .training = false,
          .split_bias = true,
          .nonlinearity = "tanh",
          .seed = 9023ULL,
          .parity_expected = true,
      },
      RnnCase{
          .name = "gru_batch_first_parity",
          .kind = Kind::GRU,
          .seq = 11,
          .batch = 3,
          .input_size = 4,
          .hidden_size = 5,
          .num_layers = 2,
          .bidirectional = true,
          .dropout = 0.05,
          .training = false,
          .batch_first = true,
          .use_bias = true,
          .split_bias = true,
          .pre_compute_input = true,
          .nonlinearity = "tanh",
          .seed = 9025ULL,
          .parity_expected = true,
      },
      RnnCase{
          .name = "gru_unified_bias_parity",
          .kind = Kind::GRU,
          .seq = 7,
          .batch = 4,
          .input_size = 5,
          .hidden_size = 4,
          .num_layers = 2,
          .bidirectional = false,
          .dropout = 0.0,
          .training = false,
          .batch_first = false,
          .use_bias = true,
          .split_bias = false,
          .pre_compute_input = true,
          .nonlinearity = "tanh",
          .seed = 9027ULL,
          .parity_expected = true,
      },
      RnnCase{
          .name = "gru_no_bias_precompute_off_parity",
          .kind = Kind::GRU,
          .seq = 8,
          .batch = 3,
          .input_size = 4,
          .hidden_size = 6,
          .num_layers = 2,
          .bidirectional = true,
          .dropout = 0.0,
          .training = false,
          .batch_first = false,
          .use_bias = false,
          .split_bias = true,
          .pre_compute_input = false,
          .nonlinearity = "tanh",
          .seed = 9029ULL,
          .parity_expected = true,
      },
      RnnCase{
          .name = "lstm_bidir_unified",
          .kind = Kind::LSTM,
          .seq = 8,
          .batch = 3,
          .input_size = 2,
          .hidden_size = 4,
          .num_layers = 2,
          .bidirectional = true,
          .dropout = 0.15,
          .training = false,
          .split_bias = false,
          .nonlinearity = "tanh",
          .seed = 9011ULL,
          .parity_expected = true,
      },
  };

  constexpr double out_max_tol = 8e-3;
  constexpr double out_mean_tol = 1e-3;

  for (const auto& tc : cases) {
    try {
      log.line("CASE begin: " + tc.name);
      const std::int64_t dirs = tc.bidirectional ? 2 : 1;
      const std::int64_t stacked = tc.num_layers * dirs;
      const std::int64_t hidden_out_single = (tc.kind == Kind::LSTM && tc.proj_size > 0) ? tc.proj_size : tc.hidden_size;
      const std::int64_t out_hidden = hidden_out_single * dirs;
      const pluss::Series x_cpu = tc.batch_first
          ? pluss::series::rand({tc.batch, tc.seq, tc.input_size}, cfg_cpu, tc.seed)
          : pluss::series::rand({tc.seq, tc.batch, tc.input_size}, cfg_cpu, tc.seed);
      const pluss::Series h0_cpu = pluss::series::rand({stacked, tc.batch, hidden_out_single}, cfg_cpu, tc.seed + 31ULL);
      const pluss::Series c0_cpu = pluss::series::rand({stacked, tc.batch, tc.hidden_size}, cfg_cpu, tc.seed + 47ULL);
      const auto w_cpu = make_weights(tc, cfg_cpu);
      const auto expect_func_output = [&](const pluss::Series& y) {
        return tc.batch_first ? has_shape(y, {tc.batch, tc.seq, out_hidden}) : has_shape(y, {tc.seq, tc.batch, out_hidden});
      };

      pluss::ai::rnn::NetConfig run_cfg{};
      run_cfg.training = tc.training;
      run_cfg.batch_first = tc.batch_first;
      run_cfg.bidirectional = tc.bidirectional;
      run_cfg.dropout = tc.dropout;
      run_cfg.num_layers = tc.num_layers;
      run_cfg.use_bias = tc.use_bias;
      run_cfg.split_bias = tc.split_bias;
      run_cfg.pre_compute_input = tc.pre_compute_input;
      run_cfg.proj_size = tc.proj_size;
      run_cfg.nonlinearity = tc.nonlinearity;
      run_cfg.cfg = cfg_cpu;
      run_cfg.seed = tc.seed + 99ULL;

      const auto w_gpu = to_cuda_weights(w_cpu);
      const pluss::Series x_gpu = x_cpu.to(pluss::pCUDA);
      const pluss::Series x_gpu_route = tc.batch_first ? x_gpu.transpose(0, 1) : x_gpu;
      const pluss::Series h0_gpu = h0_cpu.to(pluss::pCUDA);
      const pluss::Series c0_gpu = c0_cpu.to(pluss::pCUDA);

      if (!tc.parity_expected) {
        std::string why{};
        if (tc.kind == Kind::RNN) {
          if (!pluss::ai::kernels::cudnn::supports_rnn(x_gpu_route, h0_gpu, w_gpu, run_cfg, false, &why)) {
            log.line("ERROR route support failure in case " + tc.name + ": " + why);
            if (err) *err << "route support failure in " << tc.name << ": " << why << '\n';
            return false;
          }
          const auto routed = pluss::ai::kernels::cudnn::try_rnn(x_gpu_route, h0_gpu, w_gpu, run_cfg, false);
          if (!routed.has_value()) {
            log.line("ERROR route did not execute in case " + tc.name);
            if (err) *err << "route did not execute in " << tc.name << '\n';
            return false;
          }
          const auto y = std::get<0>(*routed);
          const auto h = std::get<1>(*routed);
          if (!has_shape(y, {tc.seq, tc.batch, out_hidden}) || !has_shape(h, {stacked, tc.batch, hidden_out_single})) {
            log.line("ERROR route shape mismatch in case " + tc.name);
            if (err) *err << "route shape mismatch in " << tc.name << '\n';
            return false;
          }
          if (!all_finite(as_f32_vec(y)) || !all_finite(as_f32_vec(h))) {
            log.line("ERROR non-finite values in case " + tc.name);
            if (err) *err << "non-finite values in " << tc.name << '\n';
            return false;
          }
        } else if (tc.kind == Kind::GRU) {
          if (!pluss::ai::kernels::cudnn::supports_gru(x_gpu_route, h0_gpu, w_gpu, run_cfg, false, &why)) {
            log.line("ERROR route support failure in case " + tc.name + ": " + why);
            if (err) *err << "route support failure in " << tc.name << ": " << why << '\n';
            return false;
          }
          const auto routed = pluss::ai::kernels::cudnn::try_gru(x_gpu_route, h0_gpu, w_gpu, run_cfg, false);
          if (!routed.has_value()) {
            log.line("ERROR route did not execute in case " + tc.name);
            if (err) *err << "route did not execute in " << tc.name << '\n';
            return false;
          }
          const auto y = std::get<0>(*routed);
          const auto h = std::get<1>(*routed);
          if (!has_shape(y, {tc.seq, tc.batch, out_hidden}) || !has_shape(h, {stacked, tc.batch, hidden_out_single})) {
            log.line("ERROR route shape mismatch in case " + tc.name);
            if (err) *err << "route shape mismatch in " << tc.name << '\n';
            return false;
          }
          if (!all_finite(as_f32_vec(y)) || !all_finite(as_f32_vec(h))) {
            log.line("ERROR non-finite values in case " + tc.name);
            if (err) *err << "non-finite values in " << tc.name << '\n';
            return false;
          }
        } else {
          if (!pluss::ai::kernels::cudnn::supports_lstm(x_gpu_route, h0_gpu, c0_gpu, w_gpu, run_cfg, false, &why)) {
            log.line("ERROR route support failure in case " + tc.name + ": " + why);
            if (err) *err << "route support failure in " << tc.name << ": " << why << '\n';
            return false;
          }
          const auto routed = pluss::ai::kernels::cudnn::try_lstm(x_gpu_route, h0_gpu, c0_gpu, w_gpu, run_cfg, false);
          if (!routed.has_value()) {
            log.line("ERROR route did not execute in case " + tc.name);
            if (err) *err << "route did not execute in " << tc.name << '\n';
            return false;
          }
          const auto y = std::get<0>(*routed);
          const auto h = std::get<0>(std::get<1>(*routed));
          const auto c = std::get<1>(std::get<1>(*routed));
          if (!has_shape(y, {tc.seq, tc.batch, out_hidden}) || !has_shape(h, {stacked, tc.batch, hidden_out_single}) ||
              !has_shape(c, {stacked, tc.batch, tc.hidden_size})) {
            log.line("ERROR route shape mismatch in case " + tc.name);
            if (err) *err << "route shape mismatch in " << tc.name << '\n';
            return false;
          }
          if (!all_finite(as_f32_vec(y)) || !all_finite(as_f32_vec(h)) || !all_finite(as_f32_vec(c))) {
            log.line("ERROR non-finite values in case " + tc.name);
            if (err) *err << "non-finite values in " << tc.name << '\n';
            return false;
          }
        }
        log.line("CASE route-smoke pass: " + tc.name);
        continue;
      }

      if (tc.kind == Kind::RNN) {
        const auto [y_cpu, h_cpu] = pluss::nn::rnn(x_cpu, h0_cpu, w_cpu, run_cfg);
        const auto [y_gpu, h_gpu] = pluss::nn::rnn(x_gpu, h0_gpu, w_gpu, run_cfg);
        if (!expect_func_output(y_cpu) || !expect_func_output(y_gpu) || h_cpu.sizes()[0] != stacked) {
          log.line("ERROR shape mismatch in CPU RNN reference case " + tc.name);
          if (err) *err << "shape mismatch in RNN reference " << tc.name << '\n';
          return false;
        }
        const auto y_ref = as_f32_vec(y_cpu);
        const auto y_val = as_f32_vec(y_gpu);
        const auto h_ref = as_f32_vec(h_cpu);
        const auto h_val = as_f32_vec(h_gpu);
        const double y_max = max_abs_diff(y_ref, y_val);
        const double y_mean = mean_abs_diff(y_ref, y_val);
        const double h_max = max_abs_diff(h_ref, h_val);
        const double h_mean = mean_abs_diff(h_ref, h_val);
        log.line(
            "CASE metrics: " + tc.name +
            " y(max=" + std::to_string(y_max) + ",mean=" + std::to_string(y_mean) + ")" +
            " h(max=" + std::to_string(h_max) + ",mean=" + std::to_string(h_mean) + ")");
        if (y_max > out_max_tol || y_mean > out_mean_tol || h_max > out_max_tol || h_mean > out_mean_tol) {
          log.line("ERROR parity failure in case " + tc.name);
          if (err) *err << "rnn parity failure in " << tc.name << '\n';
          return false;
        }
      } else if (tc.kind == Kind::GRU) {
        const auto [y_cpu, h_cpu] = pluss::nn::gru(x_cpu, h0_cpu, w_cpu, run_cfg);
        const auto [y_gpu, h_gpu] = pluss::nn::gru(x_gpu, h0_gpu, w_gpu, run_cfg);
        if (!expect_func_output(y_cpu) || !expect_func_output(y_gpu) || h_cpu.sizes()[0] != stacked) {
          log.line("ERROR shape mismatch in CPU GRU reference case " + tc.name);
          if (err) *err << "shape mismatch in GRU reference " << tc.name << '\n';
          return false;
        }
        const auto y_ref = as_f32_vec(y_cpu);
        const auto y_val = as_f32_vec(y_gpu);
        const auto h_ref = as_f32_vec(h_cpu);
        const auto h_val = as_f32_vec(h_gpu);
        const double y_max = max_abs_diff(y_ref, y_val);
        const double y_mean = mean_abs_diff(y_ref, y_val);
        const double h_max = max_abs_diff(h_ref, h_val);
        const double h_mean = mean_abs_diff(h_ref, h_val);
        log.line(
            "CASE metrics: " + tc.name +
            " y(max=" + std::to_string(y_max) + ",mean=" + std::to_string(y_mean) + ")" +
            " h(max=" + std::to_string(h_max) + ",mean=" + std::to_string(h_mean) + ")");
        if (y_max > out_max_tol || y_mean > out_mean_tol || h_max > out_max_tol || h_mean > out_mean_tol) {
          log.line("ERROR parity failure in case " + tc.name);
          if (err) *err << "gru parity failure in " << tc.name << '\n';
          return false;
        }
      } else {
        const auto [y_cpu, state_cpu] = pluss::nn::lstm(x_cpu, h0_cpu, c0_cpu, w_cpu, run_cfg);
        const auto [y_gpu, state_gpu] = pluss::nn::lstm(x_gpu, h0_gpu, c0_gpu, w_gpu, run_cfg);
        if (!expect_func_output(y_cpu) || !expect_func_output(y_gpu) ||
            std::get<0>(state_cpu).sizes()[0] != stacked || std::get<1>(state_cpu).sizes()[0] != stacked) {
          log.line("ERROR shape mismatch in CPU LSTM reference case " + tc.name);
          if (err) *err << "shape mismatch in LSTM reference " << tc.name << '\n';
          return false;
        }
        const auto y_ref = as_f32_vec(y_cpu);
        const auto y_val = as_f32_vec(y_gpu);
        const auto h_ref = as_f32_vec(std::get<0>(state_cpu));
        const auto h_val = as_f32_vec(std::get<0>(state_gpu));
        const auto c_ref = as_f32_vec(std::get<1>(state_cpu));
        const auto c_val = as_f32_vec(std::get<1>(state_gpu));
        const double y_max = max_abs_diff(y_ref, y_val);
        const double y_mean = mean_abs_diff(y_ref, y_val);
        const double h_max = max_abs_diff(h_ref, h_val);
        const double h_mean = mean_abs_diff(h_ref, h_val);
        const double c_max = max_abs_diff(c_ref, c_val);
        const double c_mean = mean_abs_diff(c_ref, c_val);
        log.line(
            "CASE metrics: " + tc.name +
            " y(max=" + std::to_string(y_max) + ",mean=" + std::to_string(y_mean) + ")" +
            " h(max=" + std::to_string(h_max) + ",mean=" + std::to_string(h_mean) + ")" +
            " c(max=" + std::to_string(c_max) + ",mean=" + std::to_string(c_mean) + ")");
        if (y_max > out_max_tol || y_mean > out_mean_tol ||
            h_max > out_max_tol || h_mean > out_mean_tol ||
            c_max > out_max_tol || c_mean > out_mean_tol) {
          log.line("ERROR parity failure in case " + tc.name);
          if (err) *err << "lstm parity failure in " << tc.name << '\n';
          return false;
        }
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
TEST_CASE("cuDNN rnn rigorous parity writes diagnostic log") {
  REQUIRE(run_rigorous_cudnn_rnn_parity(&std::cerr));
}
#else
int main() {
  if (!run_rigorous_cudnn_rnn_parity(&std::cerr)) return 1;
  return 0;
}
#endif
