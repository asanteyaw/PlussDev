#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <utility>

namespace pluss::plotism {

enum class AxisScale : std::uint8_t {
  Linear = 0,
  Log = 1,
};

enum class AxisKind : std::uint8_t {
  Generic = 0,
  X = 1,
  Y = 2,
  Z = 3,
};

struct Axis {
  explicit Axis(AxisKind kind = AxisKind::Generic) : kind(kind) {}

  AxisKind kind{AxisKind::Generic};
  std::string label{};
  std::optional<double> min{std::nullopt};
  std::optional<double> max{std::nullopt};
  AxisScale scale{AxisScale::Linear};
  bool inverted{false};

  Axis& set_label(std::string value) {
    label = std::move(value);
    return *this;
  }

  Axis& set_limits(double lo, double hi) {
    min = lo;
    max = hi;
    return *this;
  }

  Axis& clear_limits() {
    min.reset();
    max.reset();
    return *this;
  }

  Axis& set_scale(AxisScale value) {
    scale = value;
    return *this;
  }

  Axis& set_inverted(bool value = true) {
    inverted = value;
    return *this;
  }
};

struct XAxis : Axis {
  XAxis() : Axis(AxisKind::X) {}
};

struct YAxis : Axis {
  YAxis() : Axis(AxisKind::Y) {}
};

struct ZAxis : Axis {
  ZAxis() : Axis(AxisKind::Z) {}
};

} // namespace pluss::plotism
