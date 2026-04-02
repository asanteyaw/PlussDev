#pragma once
#include "CodeM/core/Device.h"
#include "CodeM/core/ScalarType.h"

namespace pluss::codem {

struct SeriesOptions final {
  Device device{};
  ScalarType dtype{ScalarType::Float32};

  constexpr SeriesOptions& device_(Device d) noexcept { device = d; return *this; }
  constexpr SeriesOptions& dtype_(ScalarType s) noexcept { dtype = s; return *this; }
};

} // namespace pluss::codem
