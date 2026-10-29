#pragma once

#include "RNNBase.h"

#include <tuple>

namespace pluss::ai::nn {

struct LSTMOptions {
  std::int64_t input_size = 1;
  std::int64_t hidden_size = 1;
  std::int64_t num_layers = 1;
  bool bias = true;
  bool batch_first = false;
  double dropout = 0.0;
  bool bidirectional = false;
  bool split_bias = false;
  std::int64_t proj_size = 0;
  Config cfg = ::pluss::detail::nn::default_cfg_();
  std::uint64_t seed = 0;
  bool training = true;
};

namespace detail {

inline pluss::ai::rnn::NetConfig as_run_config_(const LSTMOptions& options) {
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
      .proj_size = options.proj_size,
      .nonlinearity = "tanh",
      .cfg = options.cfg,
      .seed = options.seed,
  };
}

} // namespace detail

class PLUSS_API LSTMImpl final : public detail::RNNBase<LSTMOptions> {
public:
  using Base = detail::RNNBase<LSTMOptions>;
  using Base::options;
  using Base::params;
  using Base::to;
  using Base::train;
  using Base::eval;
  using Base::module_record;

  explicit LSTMImpl(LSTMOptions options) : Base(std::move(options)) { reset_parameters(); }
  LSTMImpl(std::int64_t input_size,
           std::int64_t hidden_size,
           std::int64_t num_layers = 1,
           bool bias = true,
           bool batch_first = false,
           double dropout = 0.0,
           bool bidirectional = false,
           bool split_bias = false,
           std::int64_t proj_size = 0)
      : LSTMImpl(LSTMOptions{input_size, hidden_size, num_layers, bias, batch_first, dropout, bidirectional, split_bias, proj_size}) {}

  void reset_parameters() {
    const std::int64_t hidden_out = this->options_.proj_size > 0 ? this->options_.proj_size : this->options_.hidden_size;
    this->reset_parameter_blocks_(
        [this, hidden_out](std::int64_t layer, std::int64_t dir, std::int64_t layer_input, std::int64_t dirs) {
          const std::uint64_t seed = this->options_.seed + static_cast<std::uint64_t>(43 * (layer * dirs + dir));
          return detail::make_weight_block_(
              4 * this->options_.hidden_size,
              layer_input,
              hidden_out,
              this->options_.bias,
              this->options_.split_bias,
              this->options_.proj_size,
              this->options_.cfg,
              seed,
              this->options_.hidden_size);
        },
        this->options_.num_layers,
        this->options_.bidirectional,
        this->options_.input_size,
        hidden_out);
  }

  std::tuple<Series, std::tuple<Series, Series>> forward(const Series& input,
                                                         const std::tuple<Series, Series>& state = {
                                                             pluss::series::undefined(),
                                                             pluss::series::undefined()}) const {
    return pluss::ai::rnn::lstm(input, state, this->params_, detail::as_run_config_(this->options_));
  }

  std::tuple<PackedSequence, std::tuple<Series, Series>> forward(
      const PackedSequence& input,
      const std::tuple<Series, Series>& state = {
          pluss::series::undefined(),
          pluss::series::undefined()}) const {
    return this->forward_packed_(input, state, this->options_.batch_first, [this](const Series& padded, const std::tuple<Series, Series>& packed_state) {
      return forward(padded, packed_state);
    });
  }

  auto operator()(
      const Series& input,
      const std::tuple<Series, Series>& state = {
          pluss::series::undefined(),
          pluss::series::undefined()}) const {
    return forward(input, state);
  }
  LSTMImpl* operator->() noexcept { return this; }
  const LSTMImpl* operator->() const noexcept { return this; }
};

PLUSS_MODULE(LSTM);

} // namespace pluss::ai::nn
