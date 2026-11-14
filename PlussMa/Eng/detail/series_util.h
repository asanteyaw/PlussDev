#pragma once

// Small Series helpers shared by the engineering modules (internal: not part of the public API).

#include "pluss/codem/codem.h"

namespace pluss::subsp::eng::detail {

// Largest absolute value over every element, as a host double (zero for an empty array).
inline double max_abs(const Series &value) {
  if (value.numel() == 0) return 0.0;
  return value.abs().reshape(std::vector<std::int64_t>{value.numel()}).amax(-1, false).item<double>();
}

}  // namespace pluss::subsp::eng::detail
