#pragma once
#include <cstdint>
#include <string_view>

namespace pluss::codem {

enum class DeviceType : std::uint8_t { CPU };

struct Device final {
  DeviceType type{DeviceType::CPU};
  constexpr std::string_view name() const noexcept { return "cpu"; }
};

} // namespace pluss::codem
