#include <pluss/pluss.h>
#include <pluss/ai.h>

#include "NodeK/kernels/cudnn_rnn.h"

#include <iostream>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

bool has_shape(const pluss::Series& x, std::initializer_list<std::int64_t> expected) {
  if (x.dim() != static_cast<std::int64_t>(expected.size())) return false;
  std::size_t i = 0;
  for (const auto d : expected) {
    if (x.sizes()[i++] != d) return false;
  }
  return true;
}

std::vector<pluss::ai::rnn::NetWeight> make_weights(std::int64_t gates,
                                                    std::int64_t input_size,
                                                    std::int64_t hidden_size,
                                                    std::int64_t num_layers,
                                                    bool bidirectional,
                                                    const pluss::Config& cfg,
                                                    std::uint64_t seed_base,
                                                    std::int64_t proj_size = 0,
                                                    bool use_bias = true,
                                                    bool split_bias = true) {
  const std::int64_t dirs = bidirectional ? 2 : 1;
  const bool projected_lstm = gates == 4 && proj_size > 0;
  const std::int64_t hidden_out = projected_lstm ? proj_size : hidden_size;
  std::vector<pluss::ai::rnn::NetWeight> out;
  out.reserve(static_cast<std::size_t>(num_layers * dirs));
  for (std::int64_t layer = 0; layer < num_layers; ++layer) {
    const std::int64_t layer_input = (layer == 0) ? input_size : hidden_out * dirs;
    for (std::int64_t dir = 0; dir < dirs; ++dir) {
      const std::uint64_t seed = seed_base + static_cast<std::uint64_t>(19 * (layer * dirs + dir));
      out.push_back(pluss::ai::rnn::NetWeight{
          .weight_ih = pluss::series::rand({gates * hidden_size, layer_input}, cfg, seed + 1ULL),
          .weight_hh = pluss::series::rand({gates * hidden_size, hidden_out}, cfg, seed + 2ULL),
          .bias = (use_bias && !split_bias) ? pluss::series::rand({gates * hidden_size}, cfg, seed + 3ULL) : pluss::Series(),
          .bias_ih = (use_bias && split_bias) ? pluss::series::rand({gates * hidden_size}, cfg, seed + 3ULL) : pluss::Series(),
          .bias_hh = (use_bias && split_bias) ? pluss::series::rand({gates * hidden_size}, cfg, seed + 4ULL) : pluss::Series(),
          .weight_hr = projected_lstm ? pluss::series::rand({hidden_out, hidden_size}, cfg, seed + 5ULL) : pluss::Series(),
      });
    }
  }
  return out;
}

bool run_cudnn_rnn_route_checks(std::ostream* err = nullptr) {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::series::rand({5, 3, 4}, cfg, 1001ULL);

  pluss::ai::rnn::NetConfig run_cfg{};
  run_cfg.batch_first = false;
  run_cfg.num_layers = 2;
  run_cfg.bidirectional = true;
  run_cfg.dropout = 0.2;
  run_cfg.split_bias = true;
  run_cfg.training = false;
  run_cfg.cfg = cfg;

  const pluss::Series h0 = pluss::series::zeros({4, 3, 2}, cfg);
  const pluss::Series c0 = pluss::series::zeros({4, 3, 2}, cfg);

  const auto rnn_w = make_weights(1, 4, 2, 2, true, cfg, 11ULL);
  const auto gru_w = make_weights(3, 4, 2, 2, true, cfg, 21ULL);
  const auto lstm_w = make_weights(4, 4, 2, 2, true, cfg, 31ULL);
  const auto lstm_proj_w = make_weights(4, 4, 2, 2, true, cfg, 41ULL, 1);
  pluss::ai::rnn::NetConfig proj_cfg = run_cfg;
  proj_cfg.proj_size = 1;
  const pluss::Series h0_proj = pluss::series::zeros({4, 3, 1}, cfg);
  const pluss::Series c0_proj = pluss::series::zeros({4, 3, 2}, cfg);

  std::string why{};
  if (pluss::ai::kernels::cudnn::supports_rnn(x, h0, rnn_w, run_cfg, false, &why)) {
    if (err) *err << "CPU tensors should not report cuDNN RNN support\n";
    return false;
  }
  if (why.empty()) {
    if (err) *err << "supports_rnn should report an unsupported reason\n";
    return false;
  }

  why.clear();
  if (pluss::ai::kernels::cudnn::supports_gru(x, h0, gru_w, run_cfg, false, &why)) {
    if (err) *err << "CPU tensors should not report cuDNN GRU support\n";
    return false;
  }
  if (why.empty()) {
    if (err) *err << "supports_gru should report an unsupported reason\n";
    return false;
  }

  why.clear();
  if (pluss::ai::kernels::cudnn::supports_lstm(x, h0, c0, lstm_w, run_cfg, false, &why)) {
    if (err) *err << "CPU tensors should not report cuDNN LSTM support\n";
    return false;
  }
  if (why.empty()) {
    if (err) *err << "supports_lstm should report an unsupported reason\n";
    return false;
  }

  if (pluss::ai::kernels::cudnn::try_rnn(x, h0, rnn_w, run_cfg, false).has_value()) {
    if (err) *err << "CPU tensors should not take cuDNN RNN route\n";
    return false;
  }
  if (pluss::ai::kernels::cudnn::try_gru(x, h0, gru_w, run_cfg, false).has_value()) {
    if (err) *err << "CPU tensors should not take cuDNN GRU route\n";
    return false;
  }
  if (pluss::ai::kernels::cudnn::try_lstm(x, h0, c0, lstm_w, run_cfg, false).has_value()) {
    if (err) *err << "CPU tensors should not take cuDNN LSTM route\n";
    return false;
  }

  const auto [rnn_out, rnn_hn] = pluss::nn::rnn(x, h0, rnn_w, run_cfg);
  const auto [gru_out, gru_hn] = pluss::nn::gru(x, h0, gru_w, run_cfg);
  const auto [lstm_out, lstm_state] = pluss::nn::lstm(x, h0, c0, lstm_w, run_cfg);
  const auto [lstm_proj_out, lstm_proj_state] = pluss::nn::lstm(x, h0_proj, c0_proj, lstm_proj_w, proj_cfg);
  auto gru_bf_cfg = run_cfg;
  gru_bf_cfg.batch_first = true;
  gru_bf_cfg.bidirectional = false;
  const auto gru_bf_w = make_weights(3, 4, 2, 2, false, cfg, 51ULL);
  const auto [gru_bf_out, gru_bf_hn] = pluss::nn::gru(x.transpose(0, 1), pluss::series::zeros({2, 3, 2}, cfg), gru_bf_w, gru_bf_cfg);
  auto gru_no_bias_cfg = run_cfg;
  gru_no_bias_cfg.batch_first = false;
  gru_no_bias_cfg.use_bias = false;
  gru_no_bias_cfg.pre_compute_input = false;
  const auto gru_no_bias_w = make_weights(3, 4, 2, 2, true, cfg, 61ULL, 0, false);
  const auto [gru_no_bias_out, gru_no_bias_hn] = pluss::nn::gru(x, h0, gru_no_bias_w, gru_no_bias_cfg);

  if (!has_shape(rnn_out, {5, 3, 4}) || !has_shape(rnn_hn, {4, 3, 2})) {
    if (err) *err << "rnn fallback output shape mismatch\n";
    return false;
  }
  if (!has_shape(gru_out, {5, 3, 4}) || !has_shape(gru_hn, {4, 3, 2})) {
    if (err) *err << "gru fallback output shape mismatch\n";
    return false;
  }
  if (!has_shape(lstm_out, {5, 3, 4}) || !has_shape(std::get<0>(lstm_state), {4, 3, 2}) || !has_shape(std::get<1>(lstm_state), {4, 3, 2})) {
    if (err) *err << "lstm fallback output/state shape mismatch\n";
    return false;
  }
  if (!has_shape(lstm_proj_out, {5, 3, 2}) || !has_shape(std::get<0>(lstm_proj_state), {4, 3, 1}) ||
      !has_shape(std::get<1>(lstm_proj_state), {4, 3, 2})) {
    if (err) *err << "lstm projection fallback output/state shape mismatch\n";
    return false;
  }
  if (!has_shape(gru_bf_out, {3, 5, 2}) || !has_shape(gru_bf_hn, {2, 3, 2})) {
    if (err) *err << "gru batch_first fallback output/state shape mismatch\n";
    return false;
  }
  if (!has_shape(gru_no_bias_out, {5, 3, 4}) || !has_shape(gru_no_bias_hn, {4, 3, 2})) {
    if (err) *err << "gru no-bias fallback output/state shape mismatch\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("cuDNN rnn route preserves CPU fallback behavior") {
  REQUIRE(run_cudnn_rnn_route_checks(&std::cerr));
}
#else
int main() {
  if (!run_cudnn_rnn_route_checks(&std::cerr)) return 1;
  std::cout << "cudnn_rnn_route_test: ok\n";
  return 0;
}
#endif
