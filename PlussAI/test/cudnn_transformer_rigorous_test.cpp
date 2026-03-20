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
  if (const char* env = std::getenv("PLUSS_AI_CUDNN_TRANSFORMER_LOG_PATH"); env != nullptr && *env != '\0') {
    return std::filesystem::path(env);
  }
  return std::filesystem::current_path() / "plussai_cudnn_transformer_rigorous.log";
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

std::vector<float> as_f32_vec(const pluss::Series& s) {
  return s.to(pluss::pCPU, pluss::ScalarType::Float32).to_vec<float>();
}

bool run_rigorous_cudnn_transformer_parity(std::ostream* err = nullptr) {
  RigorLogger log(resolve_log_path());
  if (!log.good()) {
    if (err) *err << "failed to open cudnn transformer rigorous log file\n";
    return false;
  }

  log.line("=== plussai cuDNN transformer rigorous parity test ===");
  log.line("log_file=" + log.path().string());
  log.line(std::string("cuda_ready=") + (pluss::cuda::is_ready() ? "true" : "false"));
  log.line(std::string("cudnn_ready=") + (pluss::cuda::cudnn_ready() ? "true" : "false"));
  log.line(std::string("cudnn_attention_ready=") + (pluss::cudnn_attention_ready() ? "true" : "false"));
  log.line(std::string("cudnn_transformer_ready=") + (pluss::cudnn_transformer_ready() ? "true" : "false"));
  log.line(std::string("cudnn_attention_enabled=") + (pluss::cudnn_attention_enabled() ? "true" : "false"));
  log.line(std::string("cudnn_attention_strict=") + (pluss::cudnn_attention_strict() ? "true" : "false"));
  log.line("env.PLUSS_CUDNN_ATTENTION=" + env_or("PLUSS_CUDNN_ATTENTION"));
  log.line("env.PLUSS_CUDNN_ATTENTION_STRICT=" + env_or("PLUSS_CUDNN_ATTENTION_STRICT"));
  log.line("cuda.device_count=" + std::to_string(pluss::cuda::device_count()));
  log.line("cuda.runtime_version=" + std::to_string(pluss::cuda::runtime_version()));
  log.line("cuda.driver_version=" + std::to_string(pluss::cuda::driver_version()));
  log.line(std::string("cuda.device_name=") + pluss::cuda::primary_device_name());
  log.line(std::string("cudnn_error=") + pluss::cuda::cudnn_error());

  if (!pluss::cuda::is_ready() || !pluss::cuda::cudnn_ready() || !pluss::cudnn_attention_ready()) {
    log.line("SKIP: CUDA/cuDNN/attention route not ready on this host. Test marked pass-by-skip.");
    return true;
  }

  const auto cfg_cpu = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU);

  // 1) scaled_attention forward + backward parity
  {
    const pluss::Series q = pluss::rand({2, 3, 7, 16}, cfg_cpu, 1101ULL);
    const pluss::Series k = pluss::rand({2, 3, 7, 16}, cfg_cpu, 1103ULL);
    const pluss::Series v = pluss::rand({2, 3, 7, 16}, cfg_cpu, 1107ULL);
    const pluss::Series mask = pluss::square_subsequent_mask(7, q);
    const auto [out_cpu, w_cpu] = pluss::scaled_attention(q, k, v, mask);
    const auto [out_gpu, w_gpu] = pluss::scaled_attention(
        q.to(pluss::pCUDA),
        k.to(pluss::pCUDA),
        v.to(pluss::pCUDA),
        mask.to(pluss::pCUDA));
    const double out_max = max_abs_diff(as_f32_vec(out_cpu), as_f32_vec(out_gpu));
    const double out_mean = mean_abs_diff(as_f32_vec(out_cpu), as_f32_vec(out_gpu));
    const double w_max = max_abs_diff(as_f32_vec(w_cpu), as_f32_vec(w_gpu));
    const double w_mean = mean_abs_diff(as_f32_vec(w_cpu), as_f32_vec(w_gpu));
    log.line(
        "scaled_attention forward: out(max=" + std::to_string(out_max) +
        ",mean=" + std::to_string(out_mean) +
        ") w(max=" + std::to_string(w_max) +
        ",mean=" + std::to_string(w_mean) + ")");
    if (out_max > 6e-3 || out_mean > 8e-4 || w_max > 6e-3 || w_mean > 8e-4) {
      if (err) *err << "scaled_attention forward parity failure\n";
      return false;
    }

    auto q_cpu_g = q.clone();
    auto k_cpu_g = k.clone();
    auto v_cpu_g = v.clone();
    q_cpu_g.tracks_slope_(true);
    k_cpu_g.tracks_slope_(true);
    v_cpu_g.tracks_slope_(true);
    q_cpu_g.clear_sensitivity();
    k_cpu_g.clear_sensitivity();
    v_cpu_g.clear_sensitivity();
    std::get<0>(pluss::scaled_attention(q_cpu_g, k_cpu_g, v_cpu_g, mask)).sum().reverse();
    const auto gq_cpu = as_f32_vec(q_cpu_g.sensitivity());
    const auto gk_cpu = as_f32_vec(k_cpu_g.sensitivity());
    const auto gv_cpu = as_f32_vec(v_cpu_g.sensitivity());

    auto q_gpu_g = q.to(pluss::pCUDA);
    auto k_gpu_g = k.to(pluss::pCUDA);
    auto v_gpu_g = v.to(pluss::pCUDA);
    q_gpu_g.tracks_slope_(true);
    k_gpu_g.tracks_slope_(true);
    v_gpu_g.tracks_slope_(true);
    q_gpu_g.clear_sensitivity();
    k_gpu_g.clear_sensitivity();
    v_gpu_g.clear_sensitivity();
    std::get<0>(pluss::scaled_attention(q_gpu_g, k_gpu_g, v_gpu_g, mask.to(pluss::pCUDA))).sum().reverse();
    const auto gq_gpu = as_f32_vec(q_gpu_g.sensitivity());
    const auto gk_gpu = as_f32_vec(k_gpu_g.sensitivity());
    const auto gv_gpu = as_f32_vec(v_gpu_g.sensitivity());

    const double gq_max = max_abs_diff(gq_cpu, gq_gpu);
    const double gk_max = max_abs_diff(gk_cpu, gk_gpu);
    const double gv_max = max_abs_diff(gv_cpu, gv_gpu);
    log.line(
        "scaled_attention backward: gq_max=" + std::to_string(gq_max) +
        " gk_max=" + std::to_string(gk_max) +
        " gv_max=" + std::to_string(gv_max));
    if (gq_max > 3e-2 || gk_max > 3e-2 || gv_max > 3e-2) {
      if (err) *err << "scaled_attention backward parity failure\n";
      return false;
    }
  }

  // 2) transformer forward + backward(input) parity
  {
    const auto cfg_gpu = cfg_cpu.with_platform(pluss::pCUDA);
    pluss::nn::Transformer cpu_model(pluss::nn::TransformerOptions{
        .d_model = 16,
        .nhead = 4,
        .num_encoder_layers = 2,
        .num_decoder_layers = 2,
        .dim_feedforward = 64,
        .dropout = 0.0,
        .activation = pluss::nn::TransformerActivation::GELU,
        .layer_norm_eps = 1e-5,
        .batch_first = true,
        .norm_first = true,
        .bias = true,
        .cfg = cfg_cpu,
        .seed = 1201ULL,
        .training = true,
    });
    pluss::nn::Transformer gpu_model(pluss::nn::TransformerOptions{
        .d_model = 16,
        .nhead = 4,
        .num_encoder_layers = 2,
        .num_decoder_layers = 2,
        .dim_feedforward = 64,
        .dropout = 0.0,
        .activation = pluss::nn::TransformerActivation::GELU,
        .layer_norm_eps = 1e-5,
        .batch_first = true,
        .norm_first = true,
        .bias = true,
        .cfg = cfg_gpu,
        .seed = 1201ULL,
        .training = true,
    });
    const auto snap = cpu_model->snapshot();
    gpu_model->restore(snap, true);

    for (auto* p : cpu_model->parameters()) {
      if (p != nullptr && p->defined()) p->tracks_slope_(false);
    }
    for (auto* p : gpu_model->parameters()) {
      if (p != nullptr && p->defined()) p->tracks_slope_(false);
    }

    const pluss::Series src = pluss::rand({2, 10, 16}, cfg_cpu, 1211ULL);
    const pluss::Series tgt = pluss::rand({2, 10, 16}, cfg_cpu, 1213ULL);
    const pluss::Series tgt_mask = pluss::nn::generate_square_subsequent_mask(tgt.sizes()[1], tgt);

    auto src_cpu_g = src.clone();
    auto tgt_cpu_g = tgt.clone();
    src_cpu_g.tracks_slope_(true);
    tgt_cpu_g.tracks_slope_(true);
    src_cpu_g.clear_sensitivity();
    tgt_cpu_g.clear_sensitivity();
    const pluss::Series out_cpu = cpu_model->forward(
        src_cpu_g,
        tgt_cpu_g,
        pluss::Series(),
        tgt_mask,
        pluss::Series(),
        pluss::Series(),
        pluss::Series(),
        pluss::Series(),
        false,
        true,
        false);
    out_cpu.sum().reverse();
    const auto src_grad_cpu = as_f32_vec(src_cpu_g.sensitivity());
    const auto tgt_grad_cpu = as_f32_vec(tgt_cpu_g.sensitivity());

    auto src_gpu_g = src.to(pluss::pCUDA);
    auto tgt_gpu_g = tgt.to(pluss::pCUDA);
    src_gpu_g.tracks_slope_(true);
    tgt_gpu_g.tracks_slope_(true);
    src_gpu_g.clear_sensitivity();
    tgt_gpu_g.clear_sensitivity();
    const pluss::Series out_gpu = gpu_model->forward(
        src_gpu_g,
        tgt_gpu_g,
        pluss::Series(),
        tgt_mask.to(pluss::pCUDA),
        pluss::Series(),
        pluss::Series(),
        pluss::Series(),
        pluss::Series(),
        false,
        true,
        false);
    out_gpu.sum().reverse();
    const auto src_grad_gpu = as_f32_vec(src_gpu_g.sensitivity());
    const auto tgt_grad_gpu = as_f32_vec(tgt_gpu_g.sensitivity());

    const double out_max = max_abs_diff(as_f32_vec(out_cpu), as_f32_vec(out_gpu));
    const double out_mean = mean_abs_diff(as_f32_vec(out_cpu), as_f32_vec(out_gpu));
    const double src_g_max = max_abs_diff(src_grad_cpu, src_grad_gpu);
    const double tgt_g_max = max_abs_diff(tgt_grad_cpu, tgt_grad_gpu);
    log.line(
        "transformer parity: out(max=" + std::to_string(out_max) +
        ",mean=" + std::to_string(out_mean) +
        ") src_grad_max=" + std::to_string(src_g_max) +
        " tgt_grad_max=" + std::to_string(tgt_g_max));
    if (out_max > 2e-2 || out_mean > 4e-3 || src_g_max > 5e-2 || tgt_g_max > 5e-2) {
      if (err) *err << "transformer cudnn parity failure\n";
      return false;
    }
  }

  log.line("ALL CASES PASS");
  log.line("=== end ===");
  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("cuDNN transformer rigorous parity writes diagnostic log") {
  REQUIRE(run_rigorous_cudnn_transformer_parity(&std::cerr));
}
#else
int main() {
  if (!run_rigorous_cudnn_transformer_parity(&std::cerr)) return 1;
  return 0;
}
#endif
