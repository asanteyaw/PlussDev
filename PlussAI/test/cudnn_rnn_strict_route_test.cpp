#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

class ScopedEnvVar final {
public:
  ScopedEnvVar(const char* key, const char* value) : key_(key) {
    if (const char* old = std::getenv(key_); old != nullptr) {
      old_ = std::string(old);
    }
#if defined(_WIN32)
    _putenv_s(key_, value);
#else
    setenv(key_, value, 1);
#endif
  }

  ~ScopedEnvVar() {
    if (old_.has_value()) {
#if defined(_WIN32)
      _putenv_s(key_, old_->c_str());
#else
      setenv(key_, old_->c_str(), 1);
#endif
    } else {
#if defined(_WIN32)
      _putenv_s(key_, "");
#else
      unsetenv(key_);
#endif
    }
  }

private:
  const char* key_{nullptr};
  std::optional<std::string> old_{};
};

double max_abs_diff_(const std::vector<float>& a, const std::vector<float>& b) {
  const std::size_t n = std::min(a.size(), b.size());
  double m = 0.0;
  for (std::size_t i = 0; i < n; ++i) {
    m = std::max(m, std::fabs(static_cast<double>(a[i]) - static_cast<double>(b[i])));
  }
  return m;
}

std::vector<pluss::ai::rnn::NetWeight> to_cuda_(
    const std::vector<pluss::ai::rnn::NetWeight>& w) {
  std::vector<pluss::ai::rnn::NetWeight> out{};
  out.reserve(w.size());
  for (const auto& p : w) {
    out.push_back(pluss::ai::rnn::NetWeight{
        .weight_ih = p.weight_ih.to(pluss::pCUDA),
        .weight_hh = p.weight_hh.to(pluss::pCUDA),
        .bias_ih = p.bias_ih.defined() ? p.bias_ih.to(pluss::pCUDA) : pluss::Series(),
        .bias_hh = p.bias_hh.defined() ? p.bias_hh.to(pluss::pCUDA) : pluss::Series(),
        .weight_hr = p.weight_hr.defined() ? p.weight_hr.to(pluss::pCUDA) : pluss::Series(),
    });
  }
  return out;
}

std::vector<pluss::ai::rnn::NetWeight> make_weights_(
    std::int64_t gates,
    std::int64_t input_size,
    std::int64_t hidden_size,
    std::int64_t num_layers,
    bool bidirectional,
    const pluss::Config& cfg,
    std::uint64_t seed_base) {
  const std::int64_t dirs = bidirectional ? 2 : 1;
  std::vector<pluss::ai::rnn::NetWeight> out{};
  out.reserve(static_cast<std::size_t>(num_layers * dirs));
  for (std::int64_t layer = 0; layer < num_layers; ++layer) {
    const std::int64_t layer_input = (layer == 0) ? input_size : hidden_size * dirs;
    for (std::int64_t dir = 0; dir < dirs; ++dir) {
      const std::uint64_t seed = seed_base + static_cast<std::uint64_t>(17 * (layer * dirs + dir));
      out.push_back(pluss::ai::rnn::NetWeight{
          .weight_ih = pluss::rand({gates * hidden_size, layer_input}, cfg, seed + 1ULL),
          .weight_hh = pluss::rand({gates * hidden_size, hidden_size}, cfg, seed + 2ULL),
          .bias_ih = pluss::rand({gates * hidden_size}, cfg, seed + 3ULL),
          .bias_hh = pluss::rand({gates * hidden_size}, cfg, seed + 4ULL),
          .weight_hr = pluss::Series(),
      });
    }
  }
  return out;
}

} // namespace

bool run_cudnn_rnn_strict_route_checks(std::ostream* err = nullptr) {
  const ScopedEnvVar bridge_off("PLUSS_CUDA_ALLOW_CPU_BRIDGE", "0");
  const ScopedEnvVar rnn_on("PLUSS_CUDA_NATIVE_CUDNN_RNN", "1");
  const ScopedEnvVar rnn_strict("PLUSS_CUDA_NATIVE_CUDNN_RNN_STRICT", "1");

  if (!pluss::cuda::is_ready() || !pluss::cuda::cudnn_ready()) {
    if (err) *err << "CUDA/cuDNN unavailable on this host\n";
    return true;
  }
  if (!pluss::cudnn_rnn_ready()) {
    if (err) *err << "cuDNN strict RNN route unavailable on this host\n";
    return true;
  }

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU);

  pluss::ai::rnn::NetConfig run_cfg{};
  run_cfg.training = false;
  run_cfg.batch_first = false;
  run_cfg.bidirectional = true;
  run_cfg.num_layers = 2;
  run_cfg.dropout = 0.2;
  run_cfg.split_bias = true;
  run_cfg.cfg = cfg;

  const auto x = pluss::rand({6, 3, 5}, cfg, 7201ULL);
  const auto h0 = pluss::rand({4, 3, 4}, cfg, 7203ULL);
  const auto c0 = pluss::rand({4, 3, 4}, cfg, 7205ULL);

  const auto rnn_w_cpu = make_weights_(1, 5, 4, 2, true, cfg, 7301ULL);
  const auto gru_w_cpu = make_weights_(3, 5, 4, 2, true, cfg, 7401ULL);
  const auto lstm_w_cpu = make_weights_(4, 5, 4, 2, true, cfg, 7501ULL);

  const auto rnn_w_gpu = to_cuda_(rnn_w_cpu);
  const auto gru_w_gpu = to_cuda_(gru_w_cpu);
  const auto lstm_w_gpu = to_cuda_(lstm_w_cpu);

  const auto [rnn_cpu_y, rnn_cpu_h] = pluss::rnn(x, h0, rnn_w_cpu, run_cfg);
  const auto [rnn_gpu_y, rnn_gpu_h] = pluss::rnn(x.to(pluss::pCUDA), h0.to(pluss::pCUDA), rnn_w_gpu, run_cfg);
  if (max_abs_diff_(rnn_cpu_y.to(pluss::pCPU, pluss::ScalarType::Float32).to_vec<float>(),
                    rnn_gpu_y.to(pluss::pCPU, pluss::ScalarType::Float32).to_vec<float>()) > 1e-2) {
    if (err) *err << "strict RNN output parity exceeded tolerance\n";
    return false;
  }
  if (max_abs_diff_(rnn_cpu_h.to(pluss::pCPU, pluss::ScalarType::Float32).to_vec<float>(),
                    rnn_gpu_h.to(pluss::pCPU, pluss::ScalarType::Float32).to_vec<float>()) > 1e-2) {
    if (err) *err << "strict RNN state parity exceeded tolerance\n";
    return false;
  }

  const auto [gru_cpu_y, gru_cpu_h] = pluss::gru(x, h0, gru_w_cpu, run_cfg);
  const auto [gru_gpu_y, gru_gpu_h] = pluss::gru(x.to(pluss::pCUDA), h0.to(pluss::pCUDA), gru_w_gpu, run_cfg);
  if (max_abs_diff_(gru_cpu_y.to(pluss::pCPU, pluss::ScalarType::Float32).to_vec<float>(),
                    gru_gpu_y.to(pluss::pCPU, pluss::ScalarType::Float32).to_vec<float>()) > 2e-2) {
    if (err) *err << "strict GRU output parity exceeded tolerance\n";
    return false;
  }
  if (max_abs_diff_(gru_cpu_h.to(pluss::pCPU, pluss::ScalarType::Float32).to_vec<float>(),
                    gru_gpu_h.to(pluss::pCPU, pluss::ScalarType::Float32).to_vec<float>()) > 2e-2) {
    if (err) *err << "strict GRU state parity exceeded tolerance\n";
    return false;
  }

  const auto [lstm_cpu_y, lstm_cpu_state] = pluss::lstm(x, h0, c0, lstm_w_cpu, run_cfg);
  const auto [lstm_gpu_y, lstm_gpu_state] = pluss::lstm(x.to(pluss::pCUDA), h0.to(pluss::pCUDA), c0.to(pluss::pCUDA), lstm_w_gpu, run_cfg);
  if (max_abs_diff_(lstm_cpu_y.to(pluss::pCPU, pluss::ScalarType::Float32).to_vec<float>(),
                    lstm_gpu_y.to(pluss::pCPU, pluss::ScalarType::Float32).to_vec<float>()) > 2e-2) {
    if (err) *err << "strict LSTM output parity exceeded tolerance\n";
    return false;
  }
  if (max_abs_diff_(std::get<0>(lstm_cpu_state).to(pluss::pCPU, pluss::ScalarType::Float32).to_vec<float>(),
                    std::get<0>(lstm_gpu_state).to(pluss::pCPU, pluss::ScalarType::Float32).to_vec<float>()) > 2e-2) {
    if (err) *err << "strict LSTM hidden-state parity exceeded tolerance\n";
    return false;
  }
  if (max_abs_diff_(std::get<1>(lstm_cpu_state).to(pluss::pCPU, pluss::ScalarType::Float32).to_vec<float>(),
                    std::get<1>(lstm_gpu_state).to(pluss::pCPU, pluss::ScalarType::Float32).to_vec<float>()) > 2e-2) {
    if (err) *err << "strict LSTM cell-state parity exceeded tolerance\n";
    return false;
  }
  return true;
}

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("cuDNN strict rnn/gru/lstm routes execute without bridge fallback", "[cudnn][strict][rnn]") {
  REQUIRE(run_cudnn_rnn_strict_route_checks(&std::cerr));
}
#else
int main() {
  if (!run_cudnn_rnn_strict_route_checks(&std::cerr)) return 1;
  std::cout << "cudnn_rnn_strict_route_test: ok\n";
  return 0;
}
#endif
