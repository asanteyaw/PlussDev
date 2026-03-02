#pragma once

#include <pluss/nn_detail.h>

#include <span>
#include <string>
#include <utility>
#include <vector>

namespace pluss::ai::rnn {

struct NetWeight {
  Series weight_ih{};
  Series weight_hh{};
  Series bias_ih{};
  Series bias_hh{};
  Series weight_hr{};

  [[nodiscard]] bool has_bias() const noexcept { return bias_ih.defined() || bias_hh.defined(); }
  [[nodiscard]] bool has_projection() const noexcept { return weight_hr.defined(); }
};

struct NetConfig {
  bool training = true;
  bool batch_first = false;
  bool bidirectional = false;
  double dropout = 0.0;
  std::int64_t num_layers = 1;
  bool use_bias = true;
  bool split_bias = false;
  bool pre_compute_input = true;
  std::int64_t proj_size = 0;
  std::string nonlinearity = "tanh";
  Config cfg = ::pluss::detail::nn::default_cfg_();
  std::uint64_t seed = 0;
};

inline void validate_config(const NetConfig& cfg) {
  PLUSS_CHECK(cfg.num_layers > 0, "pluss::rnn: num_layers must be > 0");
  PLUSS_CHECK(cfg.dropout >= 0.0 && cfg.dropout < 1.0, "pluss::rnn: dropout must be in [0, 1)");
  PLUSS_CHECK(cfg.nonlinearity == "tanh" || cfg.nonlinearity == "relu",
              "pluss::rnn: nonlinearity must be 'tanh' or 'relu'");
  PLUSS_CHECK(cfg.proj_size >= 0, "pluss::rnn: proj_size must be >= 0");
}

inline Series unified_bias(const NetWeight& params, const Series& like) {
  if (params.bias_ih.defined() && params.bias_hh.defined()) {
    return ::pluss::detail::nn::cast_like_(params.bias_ih, like).add(::pluss::detail::nn::cast_like_(params.bias_hh, like));
  }
  if (params.bias_ih.defined()) return ::pluss::detail::nn::cast_like_(params.bias_ih, like);
  if (params.bias_hh.defined()) return ::pluss::detail::nn::cast_like_(params.bias_hh, like);
  return Series();
}

inline Series gate_chunk(const Series& gates, std::int64_t chunk, std::int64_t chunks) {
  const std::int64_t hidden = gates.sizes()[1] / chunks;
  return ::pluss::detail::nn::narrow_dim_(gates, 1, chunk * hidden, hidden);
}

namespace detail {

inline Series apply_nonlinearity_(const Series& x, std::string_view nonlinearity) {
  return ::pluss::detail::nn::apply_nonlinearity_(x, nonlinearity);
}

inline void require_rank_at_least_(const Series& x, std::int64_t rank, const char* what) {
  ::pluss::detail::nn::require_rank_at_least_(x, rank, what);
}

inline Series reverse_time_(const Series& x) {
  return ::pluss::detail::nn::reverse_time_(x);
}

inline Series select_dim_(const Series& x, std::int64_t dim, std::int64_t index) {
  return ::pluss::detail::nn::select_dim_(x, dim, index);
}

inline Series zeros_state_(std::span<const std::int64_t> sizes, const Series& like) {
  return ::pluss::detail::nn::zeros_state_(sizes, like);
}

inline Series zeros_state_(std::initializer_list<std::int64_t> sizes, const Series& like) {
  return ::pluss::detail::nn::zeros_state_(sizes, like);
}

inline Series maybe_batch_first_to_time_major(const Series& input, const NetConfig& cfg) {
  if (cfg.batch_first && input.dim() >= 3) {
    return input.transpose(0, 1);
  }
  return input;
}

inline Series maybe_time_major_to_batch_first(const Series& output, const NetConfig& cfg) {
  if (cfg.batch_first && output.dim() >= 3) {
    return output.transpose(0, 1);
  }
  return output;
}

inline void validate_param_count(const std::vector<NetWeight>& params, const NetConfig& cfg, const char* what) {
  const std::int64_t expected = cfg.num_layers * (cfg.bidirectional ? 2 : 1);
  PLUSS_CHECK(static_cast<std::int64_t>(params.size()) == expected,
              std::string("pluss::") + what + ": expected " + std::to_string(expected) + " parameter blocks");
}

} // namespace detail

} // namespace pluss::ai::rnn
