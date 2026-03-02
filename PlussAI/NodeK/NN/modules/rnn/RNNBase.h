#pragma once

#include "../Module.h"
#include <pluss/ai/rnn.h>

#include <algorithm>
#include <cmath>
#include <tuple>
#include <utility>
#include <vector>

namespace pluss::ai::nn {

struct PackedSequence {
  Series data{};
  Series batch_sizes{};
  Series sorted_indices{};
  Series unsorted_indices{};
};

inline PackedSequence pack_padded_sequence(const Series& input, const Series& lengths, bool batch_first = false) {
  ::pluss::detail::nn::require_rank_at_least_(input, 2, "nn::pack_padded_sequence");
  ::pluss::detail::nn::require_rank_at_least_(lengths, 1, "nn::pack_padded_sequence");
  const auto lengths_vec = lengths.to(pluss::pCPU, ScalarType::Int64).to_vec<std::int64_t>();
  const auto sorted = ::pluss::detail::nn::sorted_batch_indices_(lengths_vec);
  const auto unsorted = ::pluss::detail::nn::inverse_permutation_(sorted);
  const Config icfg = Config{}.with_dtype(ScalarType::Int64).with_platform(input.platform());
  const Series sorted_idx = pluss::make_series(sorted, {static_cast<std::int64_t>(sorted.size())}, icfg);
  const Series unsorted_idx = pluss::make_series(unsorted, {static_cast<std::int64_t>(unsorted.size())}, icfg);
  const Series sorted_input = batch_first ? input.index_select(0, sorted_idx) : input.index_select(1, sorted_idx);
  const std::int64_t max_seq = batch_first ? input.sizes()[1] : input.sizes()[0];
  std::vector<std::int64_t> batch_sizes_vec;
  std::vector<Series> chunks;
  batch_sizes_vec.reserve(static_cast<std::size_t>(max_seq));
  for (std::int64_t t = 0; t < max_seq; ++t) {
    const std::int64_t current_batch = static_cast<std::int64_t>(
        std::count_if(lengths_vec.begin(), lengths_vec.end(), [=](std::int64_t v) { return v > t; }));
    if (current_batch == 0) break;
    batch_sizes_vec.push_back(current_batch);
    chunks.push_back(batch_first
                         ? ::pluss::detail::nn::narrow_dim_(::pluss::detail::nn::select_dim_(sorted_input, 1, t), 0, 0, current_batch)
                         : ::pluss::detail::nn::narrow_dim_(::pluss::detail::nn::select_dim_(sorted_input, 0, t), 0, 0, current_batch));
  }
  const Series batch_sizes =
      pluss::make_series(batch_sizes_vec, {static_cast<std::int64_t>(batch_sizes_vec.size())}, icfg);
  return PackedSequence{
      chunks.empty() ? Series() : pluss::unite(chunks, 0),
      batch_sizes,
      sorted_idx,
      unsorted_idx};
}

inline std::tuple<Series, Series> pad_packed_sequence(const PackedSequence& packed, bool batch_first = false) {
  ::pluss::detail::nn::require_rank_at_least_(packed.data, 1, "nn::pad_packed_sequence");
  const auto batch_sizes_vec = packed.batch_sizes.to(pluss::pCPU, ScalarType::Int64).to_vec<std::int64_t>();
  const std::int64_t max_seq = static_cast<std::int64_t>(batch_sizes_vec.size());
  const std::int64_t batch = packed.unsorted_indices.numel();
  std::vector<std::int64_t> tail_sizes(packed.data.sizes().begin() + 1, packed.data.sizes().end());
  const Config cfg = Config{}.with_dtype(packed.data.scalar_type()).with_platform(packed.data.platform());
  std::vector<std::int64_t> lengths(batch, 0);
  std::vector<Series> steps;
  steps.reserve(static_cast<std::size_t>(max_seq));
  std::int64_t offset = 0;
  for (std::int64_t t = 0; t < max_seq; ++t) {
    const std::int64_t current_batch = batch_sizes_vec[static_cast<std::size_t>(t)];
    Series step = ::pluss::detail::nn::narrow_dim_(packed.data, 0, offset, current_batch);
    if (current_batch < batch) {
      std::vector<std::int64_t> pad_sizes{batch - current_batch};
      pad_sizes.insert(pad_sizes.end(), tail_sizes.begin(), tail_sizes.end());
      step = pluss::unite(pluss::StackArray<Series>{step, pluss::zeros(pad_sizes, cfg)}, 0);
    }
    steps.push_back(step);
    for (std::int64_t i = 0; i < current_batch; ++i) lengths[static_cast<std::size_t>(i)] = t + 1;
    offset += current_batch;
  }
  const Config icfg = Config{}.with_dtype(ScalarType::Int64).with_platform(packed.data.platform());
  const Series lengths_s = pluss::make_series(lengths, {static_cast<std::int64_t>(lengths.size())}, icfg);
  Series padded = steps.empty() ? Series() : pluss::pile(steps, 0);
  if (batch_first) {
    padded = padded.transpose(0, 1);
  }
  const Series restored = batch_first ? padded.index_select(0, packed.unsorted_indices)
                                      : padded.index_select(1, packed.unsorted_indices);
  return {restored, lengths_s.index_select(0, packed.unsorted_indices)};
}

namespace detail {

inline pluss::ai::rnn::NetWeight make_weight_block_(std::int64_t gate_size,
                                                    std::int64_t layer_input_size,
                                                    std::int64_t hidden_recurrent_size,
                                                    bool with_bias,
                                                    bool split_bias,
                                                    std::int64_t proj_size,
                                                    Config cfg,
                                                    std::uint64_t seed) {
  const Config fcfg = ::pluss::detail::nn::floating_cfg_(cfg);
  const double bound = 1.0 / std::sqrt(static_cast<double>(hidden_recurrent_size > 0 ? hidden_recurrent_size : 1));
  pluss::ai::rnn::NetWeight out;
  out.weight_ih = ::pluss::detail::nn::uniform_({gate_size, layer_input_size}, -bound, bound, fcfg, seed);
  out.weight_hh = ::pluss::detail::nn::uniform_({gate_size, hidden_recurrent_size}, -bound, bound, fcfg, seed + 1ULL);
  if (with_bias) {
    out.bias_ih = ::pluss::detail::nn::uniform_({gate_size}, -bound, bound, fcfg, seed + 2ULL);
    if (split_bias) {
      out.bias_hh = ::pluss::detail::nn::uniform_({gate_size}, -bound, bound, fcfg, seed + 3ULL);
    }
  }
  if (proj_size > 0) {
    out.weight_hr = ::pluss::detail::nn::uniform_({proj_size, hidden_recurrent_size}, -bound, bound, fcfg, seed + 4ULL);
  }
  return out;
}

template <class Options>
class RNNBase : public Module {
public:
  using options_type = Options;

  explicit RNNBase(Options options) : Module(options.training), options_(std::move(options)) {}

  std::vector<pluss::ai::rnn::NetWeight>& params() noexcept { return params_; }
  const std::vector<pluss::ai::rnn::NetWeight>& params() const noexcept { return params_; }

  Options& options() noexcept { return options_; }
  const Options& options() const noexcept { return options_; }

protected:
  void on_training_changed_(bool enabled) noexcept override { options_.training = enabled; }

  void on_platform_changed_(Platform platform) override { options_.cfg = options_.cfg.with_platform(platform); }
  void on_dtype_changed_(ScalarType dtype) override { options_.cfg = options_.cfg.with_dtype(dtype); }

  void sync_parameter_records_() {
    this->clear_parameter_records_();
    for (std::size_t i = 0; i < params_.size(); ++i) {
      auto& block = params_[i];
      const auto prefix = "layer" + std::to_string(i) + ".";
      this->parameter_record(prefix + "weight_ih", block.weight_ih);
      this->parameter_record(prefix + "weight_hh", block.weight_hh);
      if (block.bias_ih.defined()) this->parameter_record(prefix + "bias_ih", block.bias_ih);
      if (block.bias_hh.defined()) this->parameter_record(prefix + "bias_hh", block.bias_hh);
      if (block.weight_hr.defined()) this->parameter_record(prefix + "weight_hr", block.weight_hr);
    }
  }

  template <class BuildBlock>
  void reset_parameter_blocks_(BuildBlock&& build_block,
                               std::int64_t num_layers,
                               bool bidirectional,
                               std::int64_t first_input_size,
                               std::int64_t recurrent_output_size) {
    params_.clear();
    const std::int64_t dirs = bidirectional ? 2 : 1;
    for (std::int64_t layer = 0; layer < num_layers; ++layer) {
      const std::int64_t layer_input = (layer == 0) ? first_input_size : recurrent_output_size * dirs;
      for (std::int64_t dir = 0; dir < dirs; ++dir) {
        params_.push_back(build_block(layer, dir, layer_input, dirs));
      }
    }
    sync_parameter_records_();
  }

  template <class State, class ForwardFn>
  auto forward_packed_(const PackedSequence& input, const State& state, bool batch_first, ForwardFn&& forward_fn) const {
    auto [padded, lengths] = pad_packed_sequence(input, batch_first);
    const auto [output, hidden] = forward_fn(padded, state);
    return std::make_tuple(pack_padded_sequence(output, lengths, batch_first), hidden);
  }

  mutable Options options_;
  std::vector<pluss::ai::rnn::NetWeight> params_;
};

} // namespace detail

} // namespace pluss::ai::nn
