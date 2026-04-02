#pragma once

#include "NodeK/kernels/cudnn_attention.h"
#include "NodeK/models/attention/attention.h"

namespace pluss::ai {

inline bool cudnn_attention_enabled() noexcept {
  return ::pluss::ai::kernels::cudnn::attention_route_enabled();
}

inline bool cudnn_attention_strict() noexcept {
  return ::pluss::ai::kernels::cudnn::attention_route_strict();
}

inline bool cudnn_attention_ready() noexcept {
  return ::pluss::ai::kernels::cudnn::attention_route_ready();
}

inline bool cudnn_transformer_ready() noexcept {
  return cudnn_attention_ready();
}

inline bool cudnn_transformer_enabled() noexcept {
  return cudnn_attention_enabled();
}

inline bool cudnn_transformer_strict() noexcept {
  return cudnn_attention_strict();
}

} // namespace pluss::ai
