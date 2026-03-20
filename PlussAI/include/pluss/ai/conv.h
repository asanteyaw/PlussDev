#pragma once

#include "NodeK/models/cnn/convNd.h"
#include "NodeK/models/cnn/conv1d.h"

namespace pluss::ai {

inline bool cudnn_conv_enabled() noexcept {
  return ::pluss::ai::kernels::cudnn::route_enabled();
}

inline bool cudnn_conv_strict() noexcept {
  return ::pluss::ai::kernels::cudnn::route_strict();
}

inline bool cudnn_conv_ready() noexcept {
  return ::pluss::ai::kernels::cudnn::route_ready();
}

} // namespace pluss::ai
