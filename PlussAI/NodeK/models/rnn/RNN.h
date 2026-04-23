#pragma once

#include "gru_impl.h"
#include "lstm_impl.h"
#include "rnn_impl.h"
#include <NodeK/kernels/cudnn_rnn.h>

#include <tuple>
#include <utility>
#include <vector>

namespace pluss::ai::rnn::detail {

inline bool weight_tracks_slope_(const NetWeight& w) {
  return (w.weight_ih.defined() && w.weight_ih.tracks_slope()) ||
      (w.weight_hh.defined() && w.weight_hh.tracks_slope()) ||
      (w.bias_ih.defined() && w.bias_ih.tracks_slope()) ||
      (w.bias_hh.defined() && w.bias_hh.tracks_slope()) ||
      (w.weight_hr.defined() && w.weight_hr.tracks_slope());
}

inline bool any_weight_tracks_slope_(pluss::msharp::RangeView::Of<NetWeight> params) {
  for (const auto& p : params) {
    if (weight_tracks_slope_(p)) return true;
  }
  return false;
}

inline Series stack_dim0_(pluss::msharp::RangeView::Series xs) {
  PLUSS_CHECK(!xs.empty(), "pluss::rnn: stack_dim0 requires non-empty inputs");
  Series head = xs.front().unsqueeze(0);
  if (xs.size() == 1) return head;
  std::vector<Series> tail{};
  tail.reserve(xs.size() - 1);
  for (std::size_t i = 1; i < xs.size(); ++i) {
    tail.push_back(xs[i].unsqueeze(0));
  }
  return head.unite(tail, 0);
}

inline std::pair<Series, Series> rnn_layer(const Series& input,
                                           const Series& initial_hidden,
                                           const NetWeight& params,
                                           const NetConfig& cfg) {
  const std::int64_t seq_len = input.sizes()[0];
  std::vector<Series> outputs;
  outputs.reserve(static_cast<std::size_t>(seq_len));
  Series hidden = initial_hidden;
  Series precomputed;
  if (cfg.pre_compute_input) {
    precomputed = cfg.split_bias ? pluss::ai::ff(input, params.weight_ih, params.bias_ih)
                                 : pluss::ai::ff(input, params.weight_ih, unified_bias(params, input));
  }
  for (std::int64_t t = 0; t < seq_len; ++t) {
    const Series step = cfg.pre_compute_input ? select_dim_(precomputed, 0, t) : select_dim_(input, 0, t);
    hidden = cfg.pre_compute_input ? apply_nonlinearity_(
                                         step.add(cfg.split_bias ? pluss::ai::ff(hidden, params.weight_hh, params.bias_hh)
                                                                 : pluss::ai::ff(hidden, params.weight_hh)),
                                         cfg.nonlinearity)
                                   : rnn_impl(step, hidden, params, cfg.split_bias, cfg.nonlinearity);
    outputs.push_back(hidden);
  }
  return {stack_dim0_(outputs), hidden};
}

inline std::pair<Series, Series> gru_layer(const Series& input,
                                           const Series& initial_hidden,
                                           const NetWeight& params,
                                           const NetConfig& cfg) {
  const std::int64_t seq_len = input.sizes()[0];
  std::vector<Series> outputs;
  outputs.reserve(static_cast<std::size_t>(seq_len));
  Series hidden = initial_hidden;
  Series precomputed;
  if (cfg.pre_compute_input) {
    precomputed = cfg.split_bias ? pluss::ai::ff(input, params.weight_ih, params.bias_ih)
                                 : pluss::ai::ff(input, params.weight_ih, unified_bias(params, input));
  }
  for (std::int64_t t = 0; t < seq_len; ++t) {
    const Series step = cfg.pre_compute_input ? select_dim_(precomputed, 0, t) : select_dim_(input, 0, t);
    if (cfg.pre_compute_input) {
      const Series gh = cfg.split_bias ? pluss::ai::ff(hidden, params.weight_hh, params.bias_hh)
                                       : pluss::ai::ff(hidden, params.weight_hh);
      const Series r = gate_chunk(step, 0, 3).add(gate_chunk(gh, 0, 3)).sigmoid();
      const Series z = gate_chunk(step, 1, 3).add(gate_chunk(gh, 1, 3)).sigmoid();
      const Series n = gate_chunk(step, 2, 3).add(r.mul(gate_chunk(gh, 2, 3))).tanh();
      hidden = z.mul(hidden).add((1.0 - z).mul(n));
    } else {
      hidden = gru_impl(step, hidden, params, cfg.split_bias);
    }
    outputs.push_back(hidden);
  }
  return {stack_dim0_(outputs), hidden};
}

inline std::tuple<Series, Series, Series> lstm_layer(const Series& input,
                                                     const Series& initial_hidden,
                                                     const Series& initial_cell,
                                                     const NetWeight& params,
                                                     const NetConfig& cfg) {
  const std::int64_t seq_len = input.sizes()[0];
  std::vector<Series> outputs;
  outputs.reserve(static_cast<std::size_t>(seq_len));
  Series hidden = initial_hidden;
  Series cell = initial_cell;
  Series precomputed;
  if (cfg.pre_compute_input) {
    precomputed = cfg.split_bias ? pluss::ai::ff(input, params.weight_ih, params.bias_ih)
                                 : pluss::ai::ff(input, params.weight_ih, unified_bias(params, input));
  }
  for (std::int64_t t = 0; t < seq_len; ++t) {
    const Series step = cfg.pre_compute_input ? select_dim_(precomputed, 0, t) : select_dim_(input, 0, t);
    if (cfg.pre_compute_input) {
      const Series gh = cfg.split_bias ? pluss::ai::ff(hidden, params.weight_hh, params.bias_hh)
                                       : pluss::ai::ff(hidden, params.weight_hh);
      const Series gates = step.add(gh);
      const Series i = gate_chunk(gates, 0, 4).sigmoid();
      const Series f = gate_chunk(gates, 1, 4).sigmoid();
      const Series g = gate_chunk(gates, 2, 4).tanh();
      const Series o = gate_chunk(gates, 3, 4).sigmoid();
      cell = f.mul(cell).add(i.mul(g));
      hidden = o.mul(cell.tanh());
      if (params.weight_hr.defined()) {
        hidden = pluss::ai::ff(hidden, params.weight_hr);
      }
    } else {
      std::tie(hidden, cell) = lstm_impl(step, hidden, cell, params, cfg.split_bias);
    }
    outputs.push_back(hidden);
  }
  return {stack_dim0_(outputs), hidden, cell};
}

} // namespace pluss::ai::rnn::detail

namespace pluss::ai::rnn {

inline std::tuple<Series, Series> rnn(const Series& input,
                                      const Series& hx,
                                      pluss::msharp::RangeView::Of<NetWeight> params,
                                      NetConfig cfg = {}) {
  validate_config(cfg);
  detail::validate_param_count(params, cfg, "rnn");
  detail::validate_bias_layout(params, cfg, "rnn");
  const Series x0 = detail::maybe_batch_first_to_time_major(input, cfg);
  detail::require_rank_at_least_(x0, 3, "rnn");
  const std::int64_t batch = x0.sizes()[1];
  const std::int64_t dirs = cfg.bidirectional ? 2 : 1;
  const std::int64_t hidden_size = params.front().weight_hh.sizes()[0];
  Series state = hx;
  if (!state.defined()) {
    state = detail::zeros_state_({cfg.num_layers * dirs, batch, hidden_size}, x0);
  }

  const bool require_backward =
      cfg.training && (x0.tracks_slope() || state.tracks_slope() || detail::any_weight_tracks_slope_(params));
  if (x0.platform() == pluss::pCUDA) {
    if (const auto routed = ::pluss::ai::kernels::cudnn::try_rnn(x0, state, params, cfg, require_backward); routed.has_value()) {
      return {detail::maybe_time_major_to_batch_first(std::get<0>(*routed), cfg), std::get<1>(*routed)};
    }
  }

  std::vector<Series> final_hidden;
  final_hidden.reserve(static_cast<std::size_t>(cfg.num_layers * dirs));
  Series layer_input = x0;
  for (std::int64_t layer = 0; layer < cfg.num_layers; ++layer) {
    const std::int64_t base = layer * dirs;
    if (cfg.bidirectional) {
      auto [forward_out, forward_h] = detail::rnn_layer(layer_input, detail::select_dim_(state, 0, base), params[base], cfg);
      auto [backward_rev, backward_h] =
          detail::rnn_layer(detail::reverse_time_(layer_input), detail::select_dim_(state, 0, base + 1), params[base + 1], cfg);
      final_hidden.push_back(forward_h);
      final_hidden.push_back(backward_h);
      layer_input = pluss::unite(pluss::StackArray<Series>{forward_out, detail::reverse_time_(backward_rev)}, 2);
    } else {
      auto [layer_out, layer_h] = detail::rnn_layer(layer_input, detail::select_dim_(state, 0, base), params[base], cfg);
      final_hidden.push_back(layer_h);
      layer_input = layer_out;
    }
    if (layer + 1 < cfg.num_layers && cfg.dropout > 0.0) {
      layer_input = pluss::ai::dropout(layer_input, cfg.dropout, cfg.training, cfg.seed + static_cast<std::uint64_t>(layer));
    }
  }
  return {detail::maybe_time_major_to_batch_first(layer_input, cfg), detail::stack_dim0_(final_hidden)};
}

inline std::tuple<Series, Series> rnn(const Series& input,
                                      pluss::msharp::RangeView::Of<NetWeight> params,
                                      NetConfig cfg = {}) {
  return rnn(input, Series(), params, std::move(cfg));
}

inline std::tuple<Series, Series> rnn(const Series& input,
                                      const Series& hx,
                                      const NetWeight& params,
                                      NetConfig cfg = {}) {
  return rnn(input, hx, pluss::msharp::RangeView::Of<NetWeight>{&params, 1}, std::move(cfg));
}

inline std::tuple<Series, Series> rnn(const Series& input,
                                      const NetWeight& params,
                                      NetConfig cfg = {}) {
  return rnn(input, Series(), pluss::msharp::RangeView::Of<NetWeight>{&params, 1}, std::move(cfg));
}

inline std::tuple<Series, Series> gru(const Series& input,
                                      const Series& hx,
                                      pluss::msharp::RangeView::Of<NetWeight> params,
                                      NetConfig cfg = {}) {
  validate_config(cfg);
  detail::validate_param_count(params, cfg, "gru");
  detail::validate_bias_layout(params, cfg, "gru");
  const Series x0 = detail::maybe_batch_first_to_time_major(input, cfg);
  detail::require_rank_at_least_(x0, 3, "gru");
  const std::int64_t batch = x0.sizes()[1];
  const std::int64_t dirs = cfg.bidirectional ? 2 : 1;
  const std::int64_t hidden_size = params.front().weight_hh.sizes()[1];
  Series state = hx;
  if (!state.defined()) {
    state = detail::zeros_state_({cfg.num_layers * dirs, batch, hidden_size}, x0);
  }

  const bool require_backward =
      cfg.training && (x0.tracks_slope() || state.tracks_slope() || detail::any_weight_tracks_slope_(params));
  if (x0.platform() == pluss::pCUDA) {
    if (const auto routed = ::pluss::ai::kernels::cudnn::try_gru(x0, state, params, cfg, require_backward); routed.has_value()) {
      return {detail::maybe_time_major_to_batch_first(std::get<0>(*routed), cfg), std::get<1>(*routed)};
    }
  }

  std::vector<Series> final_hidden;
  final_hidden.reserve(static_cast<std::size_t>(cfg.num_layers * dirs));
  Series layer_input = x0;
  for (std::int64_t layer = 0; layer < cfg.num_layers; ++layer) {
    const std::int64_t base = layer * dirs;
    if (cfg.bidirectional) {
      auto [forward_out, forward_h] = detail::gru_layer(layer_input, detail::select_dim_(state, 0, base), params[base], cfg);
      auto [backward_rev, backward_h] =
          detail::gru_layer(detail::reverse_time_(layer_input), detail::select_dim_(state, 0, base + 1), params[base + 1], cfg);
      final_hidden.push_back(forward_h);
      final_hidden.push_back(backward_h);
      layer_input = pluss::unite(pluss::StackArray<Series>{forward_out, detail::reverse_time_(backward_rev)}, 2);
    } else {
      auto [layer_out, layer_h] = detail::gru_layer(layer_input, detail::select_dim_(state, 0, base), params[base], cfg);
      final_hidden.push_back(layer_h);
      layer_input = layer_out;
    }
    if (layer + 1 < cfg.num_layers && cfg.dropout > 0.0) {
      layer_input = pluss::ai::dropout(layer_input, cfg.dropout, cfg.training, cfg.seed + 101ULL + static_cast<std::uint64_t>(layer));
    }
  }
  return {detail::maybe_time_major_to_batch_first(layer_input, cfg), detail::stack_dim0_(final_hidden)};
}

inline std::tuple<Series, Series> gru(const Series& input,
                                      pluss::msharp::RangeView::Of<NetWeight> params,
                                      NetConfig cfg = {}) {
  return gru(input, Series(), params, std::move(cfg));
}

inline std::tuple<Series, Series> gru(const Series& input,
                                      const Series& hx,
                                      const NetWeight& params,
                                      NetConfig cfg = {}) {
  return gru(input, hx, pluss::msharp::RangeView::Of<NetWeight>{&params, 1}, std::move(cfg));
}

inline std::tuple<Series, Series> gru(const Series& input,
                                      const NetWeight& params,
                                      NetConfig cfg = {}) {
  return gru(input, Series(), pluss::msharp::RangeView::Of<NetWeight>{&params, 1}, std::move(cfg));
}

using LSTMState = std::tuple<Series, Series>;

inline std::tuple<Series, LSTMState> lstm(const Series& input,
                                          const LSTMState& state,
                                          pluss::msharp::RangeView::Of<NetWeight> params,
                                          NetConfig cfg = {}) {
  validate_config(cfg);
  detail::validate_param_count(params, cfg, "lstm");
  detail::validate_bias_layout(params, cfg, "lstm");
  const Series x0 = detail::maybe_batch_first_to_time_major(input, cfg);
  detail::require_rank_at_least_(x0, 3, "lstm");
  const std::int64_t batch = x0.sizes()[1];
  const std::int64_t dirs = cfg.bidirectional ? 2 : 1;
  const std::int64_t cell_hidden = params.front().weight_ih.sizes()[0] / 4;
  const std::int64_t hidden_out = params.front().weight_hr.defined() ? params.front().weight_hr.sizes()[0] : cell_hidden;
  Series hidden = std::get<0>(state);
  Series cell = std::get<1>(state);
  if (!hidden.defined()) {
    hidden = detail::zeros_state_({cfg.num_layers * dirs, batch, hidden_out}, x0);
  }
  if (!cell.defined()) {
    cell = detail::zeros_state_({cfg.num_layers * dirs, batch, cell_hidden}, x0);
  }

  const bool require_backward =
      cfg.training &&
      (x0.tracks_slope() || hidden.tracks_slope() || cell.tracks_slope() || detail::any_weight_tracks_slope_(params));
  if (x0.platform() == pluss::pCUDA) {
    if (const auto routed = ::pluss::ai::kernels::cudnn::try_lstm(x0, hidden, cell, params, cfg, require_backward);
        routed.has_value()) {
      return {detail::maybe_time_major_to_batch_first(std::get<0>(*routed), cfg), std::get<1>(*routed)};
    }
  }

  std::vector<Series> final_hidden;
  std::vector<Series> final_cell;
  final_hidden.reserve(static_cast<std::size_t>(cfg.num_layers * dirs));
  final_cell.reserve(static_cast<std::size_t>(cfg.num_layers * dirs));
  Series layer_input = x0;
  for (std::int64_t layer = 0; layer < cfg.num_layers; ++layer) {
    const std::int64_t base = layer * dirs;
    if (cfg.bidirectional) {
      auto [forward_out, forward_h, forward_c] = detail::lstm_layer(
          layer_input, detail::select_dim_(hidden, 0, base), detail::select_dim_(cell, 0, base), params[base], cfg);
      auto [backward_rev, backward_h, backward_c] = detail::lstm_layer(
          detail::reverse_time_(layer_input),
          detail::select_dim_(hidden, 0, base + 1),
          detail::select_dim_(cell, 0, base + 1),
          params[base + 1],
          cfg);
      final_hidden.push_back(forward_h);
      final_hidden.push_back(backward_h);
      final_cell.push_back(forward_c);
      final_cell.push_back(backward_c);
      layer_input = pluss::unite(pluss::StackArray<Series>{forward_out, detail::reverse_time_(backward_rev)}, 2);
    } else {
      auto [layer_out, layer_h, layer_c] = detail::lstm_layer(
          layer_input, detail::select_dim_(hidden, 0, base), detail::select_dim_(cell, 0, base), params[base], cfg);
      final_hidden.push_back(layer_h);
      final_cell.push_back(layer_c);
      layer_input = layer_out;
    }
    if (layer + 1 < cfg.num_layers && cfg.dropout > 0.0) {
      layer_input = pluss::ai::dropout(layer_input, cfg.dropout, cfg.training, cfg.seed + 211ULL + static_cast<std::uint64_t>(layer));
    }
  }
  return {
      detail::maybe_time_major_to_batch_first(layer_input, cfg),
      std::make_tuple(detail::stack_dim0_(final_hidden), detail::stack_dim0_(final_cell))};
}

inline std::tuple<Series, LSTMState> lstm(const Series& input,
                                          const Series& hx,
                                          const Series& cx,
                                          pluss::msharp::RangeView::Of<NetWeight> params,
                                          NetConfig cfg = {}) {
  return lstm(input, LSTMState{hx, cx}, params, std::move(cfg));
}

inline std::tuple<Series, LSTMState> lstm(const Series& input,
                                          pluss::msharp::RangeView::Of<NetWeight> params,
                                          NetConfig cfg = {}) {
  return lstm(input, LSTMState{}, params, std::move(cfg));
}

inline std::tuple<Series, LSTMState> lstm(const Series& input,
                                          const LSTMState& state,
                                          const NetWeight& params,
                                          NetConfig cfg = {}) {
  return lstm(input, state, pluss::msharp::RangeView::Of<NetWeight>{&params, 1}, std::move(cfg));
}

inline std::tuple<Series, LSTMState> lstm(const Series& input,
                                          const Series& hx,
                                          const Series& cx,
                                          const NetWeight& params,
                                          NetConfig cfg = {}) {
  return lstm(input, LSTMState{hx, cx}, pluss::msharp::RangeView::Of<NetWeight>{&params, 1}, std::move(cfg));
}

inline std::tuple<Series, LSTMState> lstm(const Series& input,
                                          const NetWeight& params,
                                          NetConfig cfg = {}) {
  return lstm(input, LSTMState{}, pluss::msharp::RangeView::Of<NetWeight>{&params, 1}, std::move(cfg));
}

} // namespace pluss::ai::rnn
