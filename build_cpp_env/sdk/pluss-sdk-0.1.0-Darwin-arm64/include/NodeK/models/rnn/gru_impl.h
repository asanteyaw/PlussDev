#pragma once

#include "../fusion/Fusion.h"
#include "utils_config.h"

namespace pluss::ai::rnn {

inline Series gru_impl(const Series& input,
                       const Series& hidden,
                       const NetWeight& params,
                       bool split_bias = false) {
  detail::require_rank_at_least_(input, 2, "gru_impl");
  detail::require_rank_at_least_(hidden, 2, "gru_impl");
  detail::validate_impl_bias_layout(params, split_bias, "gru_impl");
  const Series gi = split_bias
                        ? pluss::ai::ff(input, params.weight_ih, params.bias_ih)
                        : pluss::ai::ff(input, params.weight_ih, unified_bias(params, input));
  const Series gh = split_bias ? pluss::ai::ff(hidden, params.weight_hh, params.bias_hh)
                               : pluss::ai::ff(hidden, params.weight_hh);
  const Series r = gate_chunk(gi, 0, 3).add(gate_chunk(gh, 0, 3)).sigmoid();
  const Series z = gate_chunk(gi, 1, 3).add(gate_chunk(gh, 1, 3)).sigmoid();
  const Series n = gate_chunk(gi, 2, 3).add(r.mul(gate_chunk(gh, 2, 3))).tanh();
  // Equivalent to z*hidden + (1-z)*n, but avoids scalar-left subtraction.
  return n.add(z.mul(hidden.sub(n)));
}

} // namespace pluss::ai::rnn
