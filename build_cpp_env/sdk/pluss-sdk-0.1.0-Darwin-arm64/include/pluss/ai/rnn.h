#pragma once

#include "NodeK/kernels/cudnn_rnn.h"
#include "NodeK/models/rnn/utils_config.h"
#include "NodeK/models/rnn/rnn_impl.h"
#include "NodeK/models/rnn/gru_impl.h"
#include "NodeK/models/rnn/lstm_impl.h"
#include "NodeK/models/rnn/RNN.h"

namespace pluss::ai {

inline bool cudnn_rnn_enabled() noexcept {
  return ::pluss::ai::kernels::cudnn::rnn_route_enabled();
}

inline bool cudnn_rnn_strict() noexcept {
  return ::pluss::ai::kernels::cudnn::rnn_route_strict();
}

inline bool cudnn_rnn_ready() noexcept {
  return ::pluss::ai::kernels::cudnn::rnn_route_ready();
}

} // namespace pluss::ai
