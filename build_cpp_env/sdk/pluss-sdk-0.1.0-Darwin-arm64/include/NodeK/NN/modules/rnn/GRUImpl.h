#pragma once

#include "RNNBase.h"

namespace pluss::ai::nn {

struct GRUOptions {
  std::int64_t input_size = 1;
  std::int64_t hidden_size = 1;
  std::int64_t num_layers = 1;
  bool bias = true;
  bool batch_first = false;
  double dropout = 0.0;
  bool bidirectional = false;
  bool split_bias = false;
  Config cfg = ::pluss::detail::nn::default_cfg_();
  std::uint64_t seed = 0;
  bool training = true;
};

namespace detail {

inline pluss::ai::rnn::NetConfig as_run_config_(const GRUOptions& options) {
  const bool pre_compute_input = options.cfg.platform != ::pluss::pMPS;
  return pluss::ai::rnn::NetConfig{
      .training = options.training,
      .batch_first = options.batch_first,
      .bidirectional = options.bidirectional,
      .dropout = options.dropout,
      .num_layers = options.num_layers,
      .use_bias = options.bias,
      .split_bias = options.split_bias,
      .pre_compute_input = pre_compute_input,
      .proj_size = 0,
      .nonlinearity = "tanh",
      .cfg = options.cfg,
      .seed = options.seed,
  };
}

} // namespace detail

class PLUSS_API GRUImpl final : public detail::RNNBase<GRUOptions> {
public:
  using Base = detail::RNNBase<GRUOptions>;
  using Base::options;
  using Base::params;
  using Base::to;
  using Base::train;
  using Base::eval;
  using Base::module_record;

  explicit GRUImpl(GRUOptions options) : Base(std::move(options)) { reset_parameters(); }
  GRUImpl(std::int64_t input_size,
          std::int64_t hidden_size,
          std::int64_t num_layers = 1,
          bool bias = true,
          bool batch_first = false,
          double dropout = 0.0,
          bool bidirectional = false,
          bool split_bias = false)
      : GRUImpl(GRUOptions{input_size, hidden_size, num_layers, bias, batch_first, dropout, bidirectional, split_bias}) {}

  void reset_parameters() {
    this->reset_parameter_blocks_(
        [this](std::int64_t layer, std::int64_t dir, std::int64_t layer_input, std::int64_t dirs) {
          const std::uint64_t seed = this->options_.seed + static_cast<std::uint64_t>(29 * (layer * dirs + dir));
          return detail::make_weight_block_(
              3 * this->options_.hidden_size,
              layer_input,
              this->options_.hidden_size,
              this->options_.bias,
              this->options_.split_bias,
              0,
              this->options_.cfg,
              seed);
        },
        this->options_.num_layers,
        this->options_.bidirectional,
        this->options_.input_size,
        this->options_.hidden_size);
  }

  std::tuple<Series, Series> forward(const Series& input, const Series& hx = pluss::series::undefined()) const {
    return pluss::ai::rnn::gru(input, hx, this->params_, detail::as_run_config_(this->options_));
  }

  std::tuple<PackedSequence, Series> forward(const PackedSequence& input, const Series& hx = pluss::series::undefined()) const {
    return this->forward_packed_(input, hx, this->options_.batch_first, [this](const Series& padded, const Series& state) {
      return forward(padded, state);
    });
  }

  auto operator()(const Series& input, const Series& hx = pluss::series::undefined()) const { return forward(input, hx); }
  GRUImpl* operator->() noexcept { return this; }
  const GRUImpl* operator->() const noexcept { return this; }
};

PLUSS_MODULE(GRU);

} // namespace pluss::ai::nn
