#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include "plot/modules/backend.h"
#include "plot/modules/core.h"

namespace pluss::plotism {

namespace detail {

inline double axis_transform(double value, const Axis& axis) {
  if (!std::isfinite(value)) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  if (axis.scale == AxisScale::Log) {
    if (value <= 0.0) {
      return std::numeric_limits<double>::quiet_NaN();
    }
    return std::log10(value);
  }
  return value;
}

inline std::pair<double, double> ensure_limits(double lo, double hi) {
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

inline std::pair<double, double> map_legend_anchor(LegendLocation location,
                                                    double left,
                                                    double right,
                                                    double top,
                                                    double bottom,
                                                    double legend_width,
                                                    double legend_height) {
  const double margin = 6.0;
  switch (location) {
    case LegendLocation::TopLeft:
      return {left + margin, top + margin};
    case LegendLocation::BottomLeft:
      return {left + margin, bottom - legend_height - margin};
    case LegendLocation::BottomRight:
      return {right - legend_width - margin, bottom - legend_height - margin};
    case LegendLocation::Best:
    case LegendLocation::TopRight:
    default:
      return {right - legend_width - margin, top + margin};
  }
}

} // namespace detail
} // namespace pluss::plotism
