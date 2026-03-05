#pragma once

#include "../fusion/Fusion.h"
#include "utils_config.h"

#include <string_view>

namespace pluss::ai::rnn {

inline Series rnn_impl(const Series& input,
                       const Series& hidden,
                       const NetWeight& params,
                       bool split_bias = false,
                       std::string_view nonlinearity = "tanh") {
  detail::require_rank_at_least_(input, 2, "rnn_impl");
  detail::require_rank_at_least_(hidden, 2, "rnn_impl");
  const Series gi = split_bias
                        ? pluss::ai::ff(input, params.weight_ih, params.bias_ih)
                        : pluss::ai::ff(input, params.weight_ih, unified_bias(params, input));
  const Series gh = split_bias ? pluss::ai::ff(hidden, params.weight_hh, params.bias_hh)
                               : pluss::ai::ff(hidden, params.weight_hh);
  return detail::apply_nonlinearity_(gi.add(gh), nonlinearity);
}

} // namespace pluss::ai::rnn
