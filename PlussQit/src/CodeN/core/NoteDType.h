#pragma once

#include <cstdint>
#include <string_view>

namespace pluss::coden {

enum class NoteDType : ::std::uint8_t {
  Text = 0,
  Bytes = 1,
  Bool = 2,
  Int64 = 3,
  Float64 = 4,
};

constexpr ::std::string_view dtype_name(NoteDType dtype) noexcept {
  switch (dtype) {
  case NoteDType::Text:
    return "Text";
  case NoteDType::Bytes:
    return "Bytes";
  case NoteDType::Bool:
    return "Bool";
  case NoteDType::Int64:
    return "Int64";
  case NoteDType::Float64:
    return "Float64";
  }
  return "Unknown";
}

} // namespace pluss::coden
