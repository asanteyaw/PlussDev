#pragma once
#include "CodeM/core/ScalarType.h"
#include <cstdint>

namespace pluss::codem {

struct Config {
  ScalarType dtype{ScalarType::Float32};
  bool pinned{false};

  constexpr Config() = default;
  constexpr explicit Config(ScalarType dt) : dtype(dt) {}

  constexpr Config with_dtype(ScalarType dt) const { Config c=*this; c.dtype=dt; return c; }
  constexpr Config with_pinned(bool p) const { Config c=*this; c.pinned=p; return c; }
};

} // namespace pluss::codem
