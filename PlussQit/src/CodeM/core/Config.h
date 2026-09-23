#pragma once
#include "CodeM/core/ScalarType.h"
#include "CodeM/core/Platform.h"
#include <cstdint>

namespace pluss::codem {

struct Config {
  ScalarType dtype{ScalarType::Float32};
  bool pinned{false};
  Platform platform{Platform::CPU};

  constexpr Config() = default;
  constexpr explicit Config(ScalarType dt) : dtype(dt), specified_(DTypeSpecified) {}

  constexpr Config with_dtype(ScalarType dt) const {
    Config c = *this;
    c.dtype = dt;
    c.specified_ |= DTypeSpecified;
    return c;
  }
  constexpr Config with_pinned(bool p) const {
    Config c = *this;
    c.pinned = p;
    c.specified_ |= PinnedSpecified;
    return c;
  }
  constexpr Config with_platform(Platform p) const {
    Config c = *this;
    c.platform = p;
    c.specified_ |= PlatformSpecified;
    return c;
  }

  constexpr bool has_dtype() const noexcept { return (specified_ & DTypeSpecified) != 0; }
  constexpr bool has_pinned() const noexcept { return (specified_ & PinnedSpecified) != 0; }
  constexpr bool has_platform() const noexcept { return (specified_ & PlatformSpecified) != 0; }

private:
  enum : std::uint8_t {
    DTypeSpecified = 1u << 0,
    PinnedSpecified = 1u << 1,
    PlatformSpecified = 1u << 2,
  };
  std::uint8_t specified_{0};
};

} // namespace pluss::codem
