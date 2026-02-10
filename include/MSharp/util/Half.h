#pragma once

#include <cstdint>

namespace pluss {
struct Half {
  uint16_t value;
  Half() noexcept : value(0) {}
  explicit Half(uint16_t v) noexcept : value(v) {}
};
} // namespace pluss
