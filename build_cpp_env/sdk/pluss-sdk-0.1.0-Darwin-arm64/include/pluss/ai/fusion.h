#pragma once

#include "NodeK/models/fusion/Fusion.h"

namespace pluss::ai {

inline bool cublas_fusion_ready() noexcept {
  return ::pluss::cuda::cublas_available();
}

inline const char* cublas_fusion_error() noexcept {
  return ::pluss::cuda::cublas_error();
}

inline bool mps_fusion_ready() noexcept {
  return ::pluss::mps::is_ready();
}

} // namespace pluss::ai
