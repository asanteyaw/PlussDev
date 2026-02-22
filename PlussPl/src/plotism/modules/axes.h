#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace pluss::plotism {

enum class AxisScale : std::uint8_t {
  Linear = 0,
  Log = 1,
};

struct Axis {
  std::string label{};
  std::optional<double> min{std::nullopt};
  std::optional<double> max{std::nullopt};
  AxisScale scale{AxisScale::Linear};
  bool inverted{false};
};

} // namespace pluss::plotism
