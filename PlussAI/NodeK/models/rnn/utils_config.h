#pragma once

#include <pluss/nn_detail.h>

#include <string>
#include <utility>
#include <vector>

namespace pluss::ai::rnn {

struct NetWeight {
  Series weight_ih{};
  Series weight_hh{};
  Series bias{};
  Series bias_ih{};
  Series bias_hh{};
  Series weight_hr{};

  [[nodiscard]] bool has_bias() const noexcept { return bias.defined() || bias_ih.defined() || bias_hh.defined(); }
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
  PLUSS_CHECK(cfg.dropout >= 0.0 && cfg.dropout <= 1.0, "pluss::rnn: dropout must be in [0, 1]");
  PLUSS_CHECK(cfg.nonlinearity == "tanh" || cfg.nonlinearity == "relu",
              "pluss::rnn: nonlinearity must be 'tanh' or 'relu'");
  PLUSS_CHECK(cfg.proj_size >= 0, "pluss::rnn: proj_size must be >= 0");
}

inline Series unified_bias(const NetWeight& params, const Series& like) {
  if (params.bias.defined()) return ::pluss::detail::nn::cast_like_(params.bias, like);
  return Series();
}

inline Series gate_chunk(const Series& gates, std::int64_t chunk, std::int64_t chunks) {
  PLUSS_CHECK(chunks > 0, "pluss::rnn: gate chunk expects chunks > 0");
  PLUSS_CHECK(gates.dim() >= 2, "pluss::rnn: gate chunk expects rank >= 2");
  PLUSS_CHECK(gates.sizes()[1] % chunks == 0, "pluss::rnn: gate chunk expects divisible feature axis");
  const std::int64_t hidden = gates.sizes()[1] / chunks;
  PLUSS_CHECK(chunk >= 0 && chunk < chunks, "pluss::rnn: gate chunk index out of range");
  return ::pluss::detail::nn::narrow_dim_(gates, 1, chunk * hidden, hidden).contiguous();
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

inline Series zeros_state_(pluss::msharp::RangeView::Int sizes, const Series& like) {
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

inline void validate_param_count(pluss::msharp::RangeView::Of<NetWeight> params,
                                 const NetConfig& cfg,
                                 const char* what) {
  const std::int64_t expected = cfg.num_layers * (cfg.bidirectional ? 2 : 1);
  PLUSS_CHECK(static_cast<std::int64_t>(params.size()) == expected,
              std::string("pluss::") + what + ": expected " + std::to_string(expected) + " parameter blocks");
}

inline void validate_bias_layout(pluss::msharp::RangeView::Of<NetWeight> params,
                                 const NetConfig& cfg,
                                 const char* what) {
  for (std::size_t i = 0; i < params.size(); ++i) {
    const auto& p = params[i];
    const bool has_b = p.bias.defined();
    const bool has_bih = p.bias_ih.defined();
    const bool has_bhh = p.bias_hh.defined();

    if (!cfg.use_bias) {
      PLUSS_CHECK(!has_b && !has_bih && !has_bhh,
                  std::string("pluss::") + what +
                      ": use_bias=false but a bias tensor is defined in parameter block " + std::to_string(i));
      continue;
    }

    if (cfg.split_bias) {
      PLUSS_CHECK(!has_b,
                  std::string("pluss::") + what +
                      ": split_bias=true does not allow unified bias tensor `bias` in block " + std::to_string(i));
      PLUSS_CHECK(has_bih == has_bhh,
                  std::string("pluss::") + what +
                      ": split_bias=true requires bias_ih and bias_hh to be both defined or both undefined in block " +
                      std::to_string(i));
      PLUSS_CHECK(has_bih && has_bhh,
                  std::string("pluss::") + what +
                      ": use_bias=true and split_bias=true requires both bias_ih and bias_hh in block " +
                      std::to_string(i) + "; set use_bias=false to disable bias");
      continue;
    }

    PLUSS_CHECK(!has_bih && !has_bhh,
                std::string("pluss::") + what +
                    ": split_bias=false does not allow bias_ih/bias_hh in block " + std::to_string(i) +
                    "; define unified `bias` instead or set split_bias=true");
    PLUSS_CHECK(has_b,
                std::string("pluss::") + what +
                    ": use_bias=true and split_bias=false requires unified bias tensor `bias` in block " +
                    std::to_string(i) + "; set use_bias=false to disable bias");
  }
}

inline void validate_impl_bias_layout(const NetWeight& params, bool split_bias, const char* what) {
  const bool has_b = params.bias.defined();
  const bool has_bih = params.bias_ih.defined();
  const bool has_bhh = params.bias_hh.defined();
  if (split_bias) {
    PLUSS_CHECK(!has_b, std::string("pluss::") + what + ": split_bias=true does not allow unified `bias`");
    PLUSS_CHECK(has_bih == has_bhh,
                std::string("pluss::") + what +
                    ": split_bias=true requires bias_ih and bias_hh to be both defined or both undefined");
    return;
  }
  PLUSS_CHECK(!has_bih && !has_bhh,
              std::string("pluss::") + what +
                  ": split_bias=false does not allow bias_ih/bias_hh; define unified `bias` instead");
}

} // namespace detail

} // namespace pluss::ai::rnn
