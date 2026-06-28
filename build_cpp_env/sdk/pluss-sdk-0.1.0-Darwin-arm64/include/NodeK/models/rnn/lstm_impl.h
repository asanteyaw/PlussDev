#pragma once

#include "../fusion/Fusion.h"
#include "utils_config.h"

#include <tuple>

namespace pluss::ai::rnn {

inline std::tuple<Series, Series> lstm_impl(const Series& input,
                                            const Series& hidden,
                                            const Series& cell,
                                            const NetWeight& params,
                                            bool split_bias = false) {
  detail::require_rank_at_least_(input, 2, "lstm_impl");
  detail::require_rank_at_least_(hidden, 2, "lstm_impl");
  detail::require_rank_at_least_(cell, 2, "lstm_impl");
  detail::validate_impl_bias_layout(params, split_bias, "lstm_impl");
  const Series gi = split_bias
                        ? pluss::ai::ff(input, params.weight_ih, params.bias_ih)
                        : pluss::ai::ff(input, params.weight_ih, unified_bias(params, input));
  const Series gh = split_bias ? pluss::ai::ff(hidden, params.weight_hh, params.bias_hh)
                               : pluss::ai::ff(hidden, params.weight_hh);
  const Series gates = gi.add(gh);
  const Series i = gate_chunk(gates, 0, 4).sigmoid();
  const Series f = gate_chunk(gates, 1, 4).sigmoid();
  const Series g = gate_chunk(gates, 2, 4).tanh();
  const Series o = gate_chunk(gates, 3, 4).sigmoid();
  const Series next_cell = f.mul(cell).add(i.mul(g));
  Series next_hidden = o.mul(next_cell.tanh());
  if (params.weight_hr.defined()) {
    next_hidden = pluss::ai::ff(next_hidden, params.weight_hr);
  }
  return std::make_tuple(next_hidden, next_cell);
}

} // namespace pluss::ai::rnn
