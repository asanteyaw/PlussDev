#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "plot/modules/figure.h"

namespace pluss::plotism {
namespace detail {

inline pluss::Series make_series_1d_plot2d(const std::vector<double>& values) {
  return pluss::make_series(values, {static_cast<std::int64_t>(values.size())});
}

inline std::vector<std::vector<double>> series_groups_plot2d(const pluss::Series& values, std::string_view where) {
  if (!values.defined()) {
    throw std::invalid_argument(std::string("plotism::") + std::string(where) + ": Series is undefined");
  }
  if (values.dim() == 1) {
    return {values.to_vec<double>()};
  }
  if (values.dim() != 2) {
    throw std::invalid_argument(std::string("plotism::") + std::string(where) + ": Series must be 1D or 2D");
  }

  const auto shape = values.sizes();
  if (shape.size() != 2 || shape[0] <= 0 || shape[1] <= 0) {
    throw std::invalid_argument(std::string("plotism::") + std::string(where) + ": Series has invalid shape");
  }
  const auto flat = values.to_vec<double>();
  const std::size_t rows = static_cast<std::size_t>(shape[0]);
  const std::size_t cols = static_cast<std::size_t>(shape[1]);
  std::vector<std::vector<double>> groups(cols, std::vector<double>(rows, 0.0));
  for (std::size_t row = 0; row < rows; ++row) {
    for (std::size_t col = 0; col < cols; ++col) {
      groups[col][row] = flat[row * cols + col];
    }
  }
  return groups;
}

inline std::vector<double> finite_values_plot2d(std::vector<double> values) {
  values.erase(std::remove_if(values.begin(), values.end(), [](double value) { return !std::isfinite(value); }), values.end());
  return values;
}

inline double quantile_plot2d(std::vector<double> values, double q) {
  values = finite_values_plot2d(std::move(values));
  if (values.empty()) {
    return 0.0;
  }
  std::sort(values.begin(), values.end());
  if (values.size() == 1) {
    return values.front();
  }
  const double clamped = std::clamp(q, 0.0, 1.0);
  const double position = clamped * static_cast<double>(values.size() - 1);
  const auto lower = static_cast<std::size_t>(std::floor(position));
  const auto upper = static_cast<std::size_t>(std::ceil(position));
  if (lower == upper) {
    return values[lower];
  }
  const double weight = position - static_cast<double>(lower);
  return values[lower] * (1.0 - weight) + values[upper] * weight;
}

inline double sample_stddev_plot2d(const std::vector<double>& values, double mean) {
  if (values.size() < 2) {
    return 0.0;
  }
  double sum = 0.0;
  for (double value : values) {
    const double delta = value - mean;
    sum += delta * delta;
  }
  return std::sqrt(sum / static_cast<double>(values.size() - 1));
}

inline void add_hidden_categorical_trace_plot2d(Figure2D& fig,
                                                const std::vector<double>& positions,
                                                const std::vector<std::string>& labels,
                                                double y_anchor) {
  if (positions.empty() || labels.empty() || positions.size() != labels.size()) {
    return;
  }
  Style hidden{};
  hidden.color = "#000000";
  hidden.alpha = 0.0;
  hidden.line_width = 0.0;
  hidden.marker = "none";
  fig.add_line(positions, std::vector<double>(positions.size(), y_anchor), "", labels, hidden);
}

inline Figure2D make_log_plot2d(const pluss::Series& x,
                                const pluss::Series& y,
                                std::string name,
                                bool log_x,
                                bool log_y) {
  Figure2D fig;
  fig.add_line(x, y, std::move(name));
  if (log_x) {
    fig.set_xscale("log");
  }
  if (log_y) {
    fig.set_yscale("log");
  }
  return fig;
}

inline Figure2D make_stem_plot2d(const pluss::Series& x,
                                 const pluss::Series& y,
                                 std::string name,
                                 std::optional<Style> style = std::nullopt) {
  const auto xv = detail::to_plot_vector(x, "plotism::stem");
  const auto yv = detail::to_plot_vector(y, "plotism::stem");
  detail::ensure_same_size(xv, yv, "plotism::stem");

  Figure2D fig;
  Style stem_style = style.value_or(Style{});
  if (stem_style.color.empty()) {
    stem_style.color = "#2563eb";
  }
  if (stem_style.line_width <= 0.0) {
    stem_style.line_width = 1.2;
  }
  stem_style.marker = "none";

  bool named = false;
  for (std::size_t i = 0; i < xv.size(); ++i) {
    fig.add_line(std::vector<double>{xv[i], xv[i]},
                 std::vector<double>{0.0, yv[i]},
                 named ? "" : name,
                 {},
                 stem_style);
    named = true;
  }

  Style marker_style = stem_style;
  marker_style.marker = "circle";
  marker_style.marker_size = std::max(3.2, marker_style.marker_size);
  fig.add_scatter(x, y, "", {}, marker_style);
  return fig;
}

inline Figure2D make_boxplot2d(const pluss::Series& values,
                               std::string name,
                               std::vector<std::string> labels,
                               std::optional<Style> style = std::nullopt) {
  auto groups = series_groups_plot2d(values, "plotism::boxplot");
  if (groups.empty()) {
    throw std::invalid_argument("plotism::boxplot: values must contain at least one group");
  }

  Figure2D fig;
  Style box_style = style.value_or(Style{});
  if (box_style.color.empty()) {
    box_style.color = "#60a5fa";
  }
  if (box_style.alpha >= 0.999) {
    box_style.alpha = 0.32;
  }
  if (box_style.line_width <= 0.0) {
    box_style.line_width = 1.2;
  }

  std::vector<double> positions;
  positions.reserve(groups.size());
  double global_lo = std::numeric_limits<double>::infinity();
  for (std::size_t group_index = 0; group_index < groups.size(); ++group_index) {
    auto group = finite_values_plot2d(std::move(groups[group_index]));
    if (group.empty()) {
      continue;
    }
    std::sort(group.begin(), group.end());
    const double q1 = quantile_plot2d(group, 0.25);
    const double q2 = quantile_plot2d(group, 0.50);
    const double q3 = quantile_plot2d(group, 0.75);
    const double iqr = std::max(1e-12, q3 - q1);
    const double lower_fence = q1 - 1.5 * iqr;
    const double upper_fence = q3 + 1.5 * iqr;

    double whisker_lo = q1;
    double whisker_hi = q3;
    std::vector<double> outliers;
    for (double value : group) {
      if (value < lower_fence || value > upper_fence) {
        outliers.push_back(value);
        continue;
      }
      whisker_lo = std::min(whisker_lo, value);
      whisker_hi = std::max(whisker_hi, value);
    }

    const double x = static_cast<double>(group_index + 1);
    positions.push_back(x);
    global_lo = std::min(global_lo, whisker_lo);
    fig.add_bar(std::vector<double>{x},
                std::vector<double>{q3},
                group_index == 0 ? name : "",
                {},
                std::vector<double>{0.56},
                std::vector<double>{q1},
                box_style);

    Style line_style = box_style;
    line_style.alpha = 1.0;
    line_style.line_width = std::max(1.2, box_style.line_width);
    line_style.marker = "none";
    fig.add_line(std::vector<double>{x - 0.28, x + 0.28}, std::vector<double>{q2, q2}, "", {}, line_style);
    fig.add_line(std::vector<double>{x, x}, std::vector<double>{whisker_lo, q1}, "", {}, line_style);
    fig.add_line(std::vector<double>{x, x}, std::vector<double>{q3, whisker_hi}, "", {}, line_style);
    fig.add_line(std::vector<double>{x - 0.18, x + 0.18}, std::vector<double>{whisker_lo, whisker_lo}, "", {}, line_style);
    fig.add_line(std::vector<double>{x - 0.18, x + 0.18}, std::vector<double>{whisker_hi, whisker_hi}, "", {}, line_style);

    if (!outliers.empty()) {
      Style outlier_style = line_style;
      outlier_style.marker = "circle";
      outlier_style.marker_size = 2.8;
      fig.add_scatter(std::vector<double>(outliers.size(), x), outliers, "", {}, outlier_style);
    }
  }

  if (!labels.empty()) {
    if (labels.size() != positions.size()) {
      throw std::invalid_argument("plotism::boxplot: labels size must match group count");
    }
    add_hidden_categorical_trace_plot2d(fig, positions, labels, std::isfinite(global_lo) ? global_lo : 0.0);
  }
  return fig;
}

inline Figure2D make_violin_plot2d(const pluss::Series& values,
                                   std::string name,
                                   std::vector<std::string> labels,
                                   std::optional<Style> style = std::nullopt) {
  auto groups = series_groups_plot2d(values, "plotism::violin");
  if (groups.empty()) {
    throw std::invalid_argument("plotism::violin: values must contain at least one group");
  }

  constexpr double pi = 3.14159265358979323846;
  constexpr std::size_t sample_count = 72;
  constexpr double max_half_width = 0.36;

  Figure2D fig;
  Style violin_style = style.value_or(Style{});
  if (violin_style.color.empty()) {
    violin_style.color = "#2a9d8f";
  }
  if (violin_style.alpha >= 0.999) {
    violin_style.alpha = 0.22;
  }
  if (violin_style.line_width <= 0.0) {
    violin_style.line_width = 1.1;
  }

  std::vector<double> positions;
  positions.reserve(groups.size());
  double global_lo = std::numeric_limits<double>::infinity();
  for (std::size_t group_index = 0; group_index < groups.size(); ++group_index) {
    auto group = finite_values_plot2d(std::move(groups[group_index]));
    if (group.empty()) {
      continue;
    }
    std::sort(group.begin(), group.end());
    const double lo = group.front();
    const double hi = group.back();
    const double mean = std::accumulate(group.begin(), group.end(), 0.0) / static_cast<double>(group.size());
    const double sigma = sample_stddev_plot2d(group, mean);
    const double bandwidth =
        std::max((hi - lo) / 24.0, (sigma > 0.0 ? 1.06 * sigma * std::pow(static_cast<double>(group.size()), -0.2) : 0.0));

    std::vector<double> y_grid(sample_count, lo);
    std::vector<double> width(sample_count, 0.0);
    double max_density = 0.0;
    for (std::size_t i = 0; i < sample_count; ++i) {
      const double t = static_cast<double>(i) / static_cast<double>(sample_count - 1);
      const double y = lo + (hi - lo) * t;
      y_grid[i] = y;
      if (bandwidth <= 1e-12) {
        width[i] = max_half_width;
        continue;
      }
      double density = 0.0;
      for (double value : group) {
        const double z = (y - value) / bandwidth;
        density += std::exp(-0.5 * z * z);
      }
      density /= static_cast<double>(group.size()) * bandwidth * std::sqrt(2.0 * pi);
      width[i] = density;
      max_density = std::max(max_density, density);
    }
    if (max_density <= 1e-12) {
      std::fill(width.begin(), width.end(), max_half_width * 0.4);
    } else {
      for (double& value : width) {
        value = max_half_width * value / max_density;
      }
    }

    const double center = static_cast<double>(group_index + 1);
    positions.push_back(center);
    global_lo = std::min(global_lo, lo);
    std::vector<double> poly_x;
    std::vector<double> poly_y;
    poly_x.reserve(sample_count * 2);
    poly_y.reserve(sample_count * 2);
    for (std::size_t i = 0; i < sample_count; ++i) {
      poly_x.push_back(center + width[i]);
      poly_y.push_back(y_grid[i]);
    }
    for (std::size_t i = sample_count; i > 0; --i) {
      const std::size_t idx = i - 1;
      poly_x.push_back(center - width[idx]);
      poly_y.push_back(y_grid[idx]);
    }
    fig.add_polygon(make_series_1d_plot2d(poly_x), make_series_1d_plot2d(poly_y), group_index == 0 ? name : "", violin_style);

    const double median = quantile_plot2d(group, 0.50);
    Style median_style = violin_style;
    median_style.alpha = 1.0;
    median_style.line_width = std::max(1.2, violin_style.line_width);
    fig.add_line(std::vector<double>{center - max_half_width * 0.65, center + max_half_width * 0.65},
                 std::vector<double>{median, median},
                 "",
                 {},
                 median_style);
  }

  if (!labels.empty()) {
    if (labels.size() != positions.size()) {
      throw std::invalid_argument("plotism::violin: labels size must match group count");
    }
    add_hidden_categorical_trace_plot2d(fig, positions, labels, std::isfinite(global_lo) ? global_lo : 0.0);
  }
  return fig;
}

inline Figure2D make_polar_plot2d(const pluss::Series& theta, const pluss::Series& rho, std::string name) {
  Figure2D fig;
  fig.theme("paper");
  Style trace_style{};
  trace_style.color = "#2563eb";
  trace_style.line_width = 2.0;
  trace_style.marker = "circle";
  trace_style.marker_size = 2.6;
  fig.polar_axes(true).add_line(theta, rho, std::move(name), {}, trace_style);
  return fig;
}

} // namespace detail

inline Figure2D line(const pluss::Series& y, std::string name = "") {
  Figure2D fig;
  fig.add_line(y, std::move(name));
  return fig;
}

inline Figure2D line(const pluss::Series& x, const pluss::Series& y, std::string name = "") {
  Figure2D fig;
  fig.add_line(x, y, std::move(name));
  return fig;
}

inline Figure2D scatter(const pluss::Series& y, std::string name = "") {
  Figure2D fig;
  fig.add_scatter(y, std::move(name));
  return fig;
}

inline Figure2D scatter(const pluss::Series& x, const pluss::Series& y, std::string name = "") {
  Figure2D fig;
  fig.add_scatter(x, y, std::move(name));
  return fig;
}

inline Figure2D bar(const pluss::Series& y, std::string name = "") {
  Figure2D fig;
  fig.add_bar(y, std::move(name));
  return fig;
}

inline Figure2D bar(const pluss::Series& x, const pluss::Series& y, std::string name = "") {
  Figure2D fig;
  fig.add_bar(x, y, std::move(name));
  return fig;
}

inline Figure2D hist(const pluss::Series& values, std::int64_t bins = 20, std::string name = "") {
  Figure2D fig;
  fig.add_hist(values, bins, std::move(name));
  return fig;
}

inline Figure2D step(const pluss::Series& y, std::string name = "") {
  Figure2D fig;
  fig.add_step(y, std::move(name));
  return fig;
}

inline Figure2D step(const pluss::Series& x, const pluss::Series& y, std::string name = "") {
  Figure2D fig;
  fig.add_step(x, y, std::move(name));
  return fig;
}

inline Figure2D area(const pluss::Series& y, std::string name = "") {
  Figure2D fig;
  fig.add_area(y, std::move(name));
  return fig;
}

inline Figure2D area(const pluss::Series& x, const pluss::Series& y, std::string name = "") {
  Figure2D fig;
  fig.add_area(x, y, std::move(name));
  return fig;
}

inline Figure2D band(const pluss::Series& x,
                     const pluss::Series& lower,
                     const pluss::Series& upper,
                     std::string name = "") {
  Figure2D fig;
  fig.add_band(x, lower, upper, std::move(name));
  return fig;
}

inline Figure2D errorbar(const pluss::Series& x,
                         const pluss::Series& y,
                         const pluss::Series& yerr,
                         std::string name = "") {
  Figure2D fig;
  fig.add_errorbar(x, y, yerr, std::move(name));
  return fig;
}

inline Figure2D heatmap(const pluss::Series& values, std::string name = "") {
  Figure2D fig;
  fig.add_heatmap(values, std::move(name));
  return fig;
}

inline Figure2D semilogx(const pluss::Series& x, const pluss::Series& y, std::string name = "") {
  return detail::make_log_plot2d(x, y, std::move(name), true, false);
}

inline Figure2D semilogy(const pluss::Series& x, const pluss::Series& y, std::string name = "") {
  return detail::make_log_plot2d(x, y, std::move(name), false, true);
}

inline Figure2D loglog(const pluss::Series& x, const pluss::Series& y, std::string name = "") {
  return detail::make_log_plot2d(x, y, std::move(name), true, true);
}

inline Figure2D stem(const pluss::Series& y, std::string name = "") {
  const auto values = detail::to_plot_vector(y, "plotism::stem");
  return detail::make_stem_plot2d(detail::make_series_1d_plot2d(detail::make_index_axis(values.size())), y, std::move(name));
}

inline Figure2D stem(const pluss::Series& x, const pluss::Series& y, std::string name = "") {
  return detail::make_stem_plot2d(x, y, std::move(name));
}

inline Figure2D boxplot(const pluss::Series& values,
                        std::string name = "",
                        std::vector<std::string> labels = {}) {
  return detail::make_boxplot2d(values, std::move(name), std::move(labels));
}

inline Figure2D violin(const pluss::Series& values,
                       std::string name = "",
                       std::vector<std::string> labels = {}) {
  return detail::make_violin_plot2d(values, std::move(name), std::move(labels));
}

inline Figure2D polarplot(const pluss::Series& theta, const pluss::Series& rho, std::string name = "") {
  return detail::make_polar_plot2d(theta, rho, std::move(name));
}

} // namespace pluss::plotism
