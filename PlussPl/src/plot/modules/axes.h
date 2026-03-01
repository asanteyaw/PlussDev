#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "plot/modules/core.h"

namespace pluss::plotism {

enum class AxisScale : std::uint8_t {
  Linear = 0,
  Log = 1,
};

enum class TickLocatorKind : std::uint8_t {
  Auto = 0,
  Linear = 1,
  Log = 2,
  Categorical = 3,
};

enum class TickFormatterKind : std::uint8_t {
  Auto = 0,
  Numeric = 1,
  Scientific = 2,
  Log = 3,
  Categorical = 4,
};

enum class AxisKind : std::uint8_t {
  Generic = 0,
  X = 1,
  Y = 2,
  Z = 3,
};

struct ResolvedTick {
  double position{0.0};
  std::string label{};
};

struct TickLocator {
  TickLocatorKind kind{TickLocatorKind::Auto};
  int approx_count{5};

  std::vector<double> locate(double lo,
                             double hi,
                             AxisScale axis_scale,
                             const std::vector<double>* categorical_positions = nullptr) const {
    const TickLocatorKind effective = effective_kind(axis_scale, categorical_positions);
    if (effective == TickLocatorKind::Categorical && categorical_positions && !categorical_positions->empty()) {
      return *categorical_positions;
    }

    const auto [min_v, max_v] = ensure_limits(lo, hi);
    const double step = nice_step(min_v, max_v, approx_count);
    const double start = std::ceil(min_v / step) * step;
    std::vector<double> ticks;
    for (double value = start; value <= max_v + step * 0.5; value += step) {
      if (std::isfinite(value)) {
        ticks.push_back(std::abs(value) < step * 1e-9 ? 0.0 : value);
      }
    }
    if (ticks.empty()) {
      ticks.push_back(min_v);
      ticks.push_back(max_v);
    }
    return ticks;
  }

private:
  TickLocatorKind effective_kind(AxisScale axis_scale, const std::vector<double>* categorical_positions) const {
    if (kind == TickLocatorKind::Auto) {
      if (categorical_positions && !categorical_positions->empty()) {
        return TickLocatorKind::Categorical;
      }
      return axis_scale == AxisScale::Log ? TickLocatorKind::Log : TickLocatorKind::Linear;
    }
    return kind;
  }

  static std::pair<double, double> ensure_limits(double lo, double hi) {
    if (!std::isfinite(lo) || !std::isfinite(hi)) {
      return {0.0, 1.0};
    }
    if (lo == hi) {
      return {lo - 0.5, hi + 0.5};
    }
    if (lo > hi) {
      std::swap(lo, hi);
    }
    return {lo, hi};
  }

  static double nice_step(double lo, double hi, int approx_tick_count) {
    const double span = std::abs(hi - lo);
    if (!(span > 0.0) || !std::isfinite(span)) {
      return 1.0;
    }
    const double raw = span / static_cast<double>(std::max(1, approx_tick_count));
    const double magnitude = std::pow(10.0, std::floor(std::log10(raw)));
    const double residual = raw / magnitude;
    double nice = 1.0;
    if (residual >= 5.0) {
      nice = 5.0;
    } else if (residual >= 2.0) {
      nice = 2.0;
    }
    return nice * magnitude;
  }
};

struct TickFormatter {
  TickFormatterKind kind{TickFormatterKind::Auto};
  int precision{6};

  std::vector<std::string> format(const std::vector<double>& ticks,
                                  AxisScale axis_scale,
                                  const std::vector<std::string>* categorical_labels = nullptr) const {
    const TickFormatterKind effective = effective_kind(axis_scale, categorical_labels);
    if (effective == TickFormatterKind::Categorical && categorical_labels && categorical_labels->size() == ticks.size()) {
      return *categorical_labels;
    }

    std::vector<std::string> labels;
    labels.reserve(ticks.size());
    for (double tick : ticks) {
      labels.push_back(format_one(tick, axis_scale, effective));
    }
    return labels;
  }

private:
  TickFormatterKind effective_kind(AxisScale axis_scale, const std::vector<std::string>* categorical_labels) const {
    if (kind == TickFormatterKind::Auto) {
      if (categorical_labels && !categorical_labels->empty()) {
        return TickFormatterKind::Categorical;
      }
      return axis_scale == AxisScale::Log ? TickFormatterKind::Log : TickFormatterKind::Numeric;
    }
    return kind;
  }

  std::string format_one(double transformed_value, AxisScale axis_scale, TickFormatterKind effective) const {
    if (!std::isfinite(transformed_value)) {
      return "NaN";
    }
    if (effective == TickFormatterKind::Scientific) {
      std::ostringstream out;
      out.setf(std::ios::scientific, std::ios::floatfield);
      out.precision(std::max(1, precision));
      const double raw = axis_scale == AxisScale::Log ? std::pow(10.0, transformed_value) : transformed_value;
      out << raw;
      return out.str();
    }
    if (effective == TickFormatterKind::Log) {
      return detail::format_double(std::pow(10.0, transformed_value), precision);
    }
    return detail::format_double(transformed_value, precision);
  }
};

struct Axis {
  explicit Axis(AxisKind kind = AxisKind::Generic) : kind(kind) {}

  AxisKind kind{AxisKind::Generic};
  std::string label{};
  std::optional<double> min{std::nullopt};
  std::optional<double> max{std::nullopt};
  AxisScale scale{AxisScale::Linear};
  bool inverted{false};
  TickLocator tick_locator{};
  TickFormatter tick_formatter{};

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

  Axis& set_tick_locator(TickLocator locator) {
    tick_locator = std::move(locator);
    return *this;
  }

  Axis& set_tick_formatter(TickFormatter formatter) {
    tick_formatter = std::move(formatter);
    return *this;
  }

  double transform_value(double value) const {
    if (!std::isfinite(value)) {
      return std::numeric_limits<double>::quiet_NaN();
    }
    if (scale == AxisScale::Log) {
      if (value <= 0.0) {
        return std::numeric_limits<double>::quiet_NaN();
      }
      return std::log10(value);
    }
    return value;
  }

  std::vector<ResolvedTick> resolve_ticks(double lo,
                                          double hi,
                                          const std::vector<double>* categorical_positions = nullptr,
                                          const std::vector<std::string>* categorical_labels = nullptr) const {
    const auto positions = tick_locator.locate(lo, hi, scale, categorical_positions);
    const auto labels = tick_formatter.format(positions, scale, categorical_labels);
    std::vector<ResolvedTick> ticks;
    ticks.reserve(positions.size());
    for (std::size_t i = 0; i < positions.size(); ++i) {
      ticks.push_back(ResolvedTick{
          .position = positions[i],
          .label = i < labels.size() ? labels[i] : detail::format_double(positions[i], tick_formatter.precision),
      });
    }
    return ticks;
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

using AxisModel = Axis;

} // namespace pluss::plotism
