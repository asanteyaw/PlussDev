#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <fmt/format.h>

#include "pluss/codem/series_factory.h"
#include "plot/modules/backend/default_backend.h"
#include "plot/modules/core.h"
#include "plot/modules/detail/series_helpers.h"
#include "plot/modules/detail/show_helpers.h"
#include "plot/modules/trace.h"

namespace pluss::plotism {
namespace detail {

struct HeatmapData2D {
  std::vector<double> values{};
  std::int64_t rows{0};
  std::int64_t cols{0};
};

inline HeatmapData2D flatten_heatmap_rows(const std::vector<std::vector<double>>& rows, std::string_view where) {
  if (rows.empty() || rows.front().empty()) {
    throw std::invalid_argument(fmt::format("plotism::{}: heatmap values must be non-empty", where));
  }
  const std::size_t cols = rows.front().size();
  HeatmapData2D out;
  out.rows = static_cast<std::int64_t>(rows.size());
  out.cols = static_cast<std::int64_t>(cols);
  out.values.reserve(rows.size() * cols);
  for (const auto& row : rows) {
    if (row.size() != cols) {
      throw std::invalid_argument(fmt::format("plotism::{}: heatmap rows must have equal length", where));
    }
    out.values.insert(out.values.end(), row.begin(), row.end());
  }
  return out;
}

inline HeatmapData2D flatten_heatmap_series(const pluss::Series& values, std::string_view where) {
  if (!values.defined()) {
    throw std::invalid_argument(fmt::format("plotism::{}: Series is undefined", where));
  }
  if (values.dim() != 2) {
    throw std::invalid_argument(fmt::format("plotism::{}: heatmap Series must be 2D", where));
  }
  const auto shape = values.sizes();
  if (shape.size() != 2 || shape[0] <= 0 || shape[1] <= 0) {
    throw std::invalid_argument(fmt::format("plotism::{}: heatmap Series has invalid shape", where));
  }
  return HeatmapData2D{
      .values = values.to_vec<double>(),
      .rows = shape[0],
      .cols = shape[1],
  };
}

inline std::vector<double> make_constant_axis(std::size_t size, double value) {
  return std::vector<double>(size, value);
}

} // namespace detail

class Figure2D {
public:
  Figure2D(std::int32_t rows = 1, std::int32_t cols = 1)
      : rows_(rows), cols_(cols), backend_(make_default_backend()) {
    if (rows_ <= 0 || cols_ <= 0) {
      throw std::invalid_argument("plotism::Figure2D: rows/cols must be > 0");
    }
    subplots_.resize(static_cast<std::size_t>(rows_ * cols_));
    for (auto& subplot : subplots_) {
      apply_figure_style_defaults_(subplot);
    }
  }

  std::int32_t rows() const noexcept { return rows_; }
  std::int32_t cols() const noexcept { return cols_; }
  std::int32_t active_subplot() const noexcept { return active_; }
  const std::vector<Subplot2D>& subplots() const noexcept { return subplots_; }

  Figure2D& with_title(std::string title) {
    figure_title_ = std::move(title);
    return *this;
  }

  const std::string& title() const noexcept { return figure_title_; }

  Figure2D& subplot(std::int32_t rows, std::int32_t cols, std::int32_t index) {
    if (rows <= 0 || cols <= 0) {
      throw std::invalid_argument("plotism::Figure2D::subplot: rows/cols must be > 0");
    }
    const auto total = rows * cols;
    if (index < 0 || index >= total) {
      throw std::invalid_argument("plotism::Figure2D::subplot: subplot index out of range");
    }
    if (rows_ != rows || cols_ != cols) {
      rows_ = rows;
      cols_ = cols;
      subplots_.clear();
      subplots_.resize(static_cast<std::size_t>(rows_ * cols_));
      for (auto& subplot : subplots_) {
        apply_figure_style_defaults_(subplot);
      }
    }
    active_ = index;
    return *this;
  }

  Figure2D& title(std::string title) {
    current_().title = std::move(title);
    return *this;
  }

  XAxis& xaxis() noexcept { return current_().x_axis; }
  YAxis& yaxis() noexcept { return current_().y_axis; }
  const XAxis& xaxis() const noexcept { return current_().x_axis; }
  const YAxis& yaxis() const noexcept { return current_().y_axis; }

  Figure2D& x_label(std::string label) {
    current_().x_axis.set_label(std::move(label));
    return *this;
  }

  Figure2D& y_label(std::string label) {
    current_().y_axis.set_label(std::move(label));
    return *this;
  }

  Figure2D& x_limits(double min_v, double max_v) {
    current_().x_axis.set_limits(min_v, max_v);
    return *this;
  }

  Figure2D& y_limits(double min_v, double max_v) {
    current_().y_axis.set_limits(min_v, max_v);
    return *this;
  }

  Figure2D& set_xlim(double min_v, double max_v) {
    return x_limits(min_v, max_v);
  }

  Figure2D& set_ylim(double min_v, double max_v) {
    return y_limits(min_v, max_v);
  }

  Figure2D& set_xscale(std::string_view scale) {
    current_().x_axis.set_scale(detail::parse_axis_scale(scale, "Figure2D::set_xscale"));
    return *this;
  }

  Figure2D& set_yscale(std::string_view scale) {
    current_().y_axis.set_scale(detail::parse_axis_scale(scale, "Figure2D::set_yscale"));
    return *this;
  }

  Figure2D& invert_xaxis(bool enabled = true) {
    current_().x_axis.set_inverted(enabled);
    return *this;
  }

  Figure2D& invert_yaxis(bool enabled = true) {
    current_().y_axis.set_inverted(enabled);
    return *this;
  }

  Figure2D& legend(const Legend& l) {
    current_().legend = l;
    return *this;
  }

  Figure2D& colorbar(const Colorbar& c) {
    current_().colorbar = c;
    return *this;
  }

  Figure2D& theme(std::string name) {
    figure_style_.theme = name;
    for (auto& subplot : subplots_) {
      subplot.style.theme = name;
    }
    return *this;
  }

  Figure2D& font_family(std::string family) {
    figure_style_.font_family = family;
    for (auto& subplot : subplots_) {
      subplot.style.font_family = family;
    }
    return *this;
  }

  Figure2D& style(const Style& s) {
    current_().style = s;
    if (current_().style.theme.empty()) {
      current_().style.theme = figure_style_.theme;
    }
    if (current_().style.font_family.empty()) {
      current_().style.font_family = figure_style_.font_family;
    }
    return *this;
  }

  Figure2D& set_backend(std::shared_ptr<const IBackend> backend) {
    if (!backend) {
      throw std::invalid_argument("plotism::Figure2D::set_backend: backend cannot be null");
    }
    backend_ = std::move(backend);
    return *this;
  }

  const std::shared_ptr<const IBackend>& backend() const noexcept { return backend_; }

  Figure2D& add_line(const std::vector<double>& y, std::string name = "") {
    std::vector<double> x = detail::make_index_axis(y.size());
    return add_line(x, y, std::move(name));
  }

  Figure2D& add_line(const pluss::Series& y, std::string name = "") {
    return add_line(detail::to_plot_vector(y, "Figure2D::add_line"), std::move(name));
  }

  template <typename YInput>
    requires detail::PlotInput<YInput>
  Figure2D& add_line(const YInput& y, std::string name = "") {
    return add_line(detail::to_plot_vector(y, "Figure2D::add_line"), std::move(name));
  }

  Figure2D& add_line(const std::vector<double>& x,
                     const std::vector<double>& y,
                     std::string name = "",
                     std::vector<std::string> x_labels = {},
                     std::optional<Style> style = std::nullopt) {
    detail::ensure_same_size(x, y, "Figure2D::add_line");
    if (!x_labels.empty() && x_labels.size() != x.size()) {
      throw std::invalid_argument("plotism::Figure2D::add_line: x_labels size mismatch");
    }
    Trace2D t;
    t.kind = Trace2DKind::Line;
    t.name = std::move(name);
    t.x = x;
    t.y = y;
    t.x_labels = std::move(x_labels);
    t.style = style.value_or(current_().style);
    current_().traces.push_back(std::move(t));
    return *this;
  }

  Figure2D& add_line(const pluss::Series& x,
                     const pluss::Series& y,
                     std::string name = "",
                     std::vector<std::string> x_labels = {},
                     std::optional<Style> style = std::nullopt) {
    return add_line(detail::to_plot_vector(x, "Figure2D::add_line"),
                    detail::to_plot_vector(y, "Figure2D::add_line"),
                    std::move(name),
                    std::move(x_labels),
                    style);
  }

  template <typename XInput, typename YInput>
    requires detail::PlotInput<XInput> && detail::PlotInput<YInput>
  Figure2D& add_line(const XInput& x,
                     const YInput& y,
                     std::string name = "",
                     std::vector<std::string> x_labels = {},
                     std::optional<Style> style = std::nullopt) {
    return add_line(detail::to_plot_vector(x, "Figure2D::add_line"),
                    detail::to_plot_vector(y, "Figure2D::add_line"),
                    std::move(name),
                    std::move(x_labels),
                    style);
  }

  Figure2D& add_scatter(const std::vector<double>& y, std::string name = "") {
    std::vector<double> x = detail::make_index_axis(y.size());
    return add_scatter(x, y, std::move(name));
  }

  Figure2D& add_scatter(const pluss::Series& y, std::string name = "") {
    return add_scatter(detail::to_plot_vector(y, "Figure2D::add_scatter"), std::move(name));
  }

  template <typename YInput>
    requires detail::PlotInput<YInput>
  Figure2D& add_scatter(const YInput& y, std::string name = "") {
    return add_scatter(detail::to_plot_vector(y, "Figure2D::add_scatter"), std::move(name));
  }

  Figure2D& add_scatter(const std::vector<double>& x,
                        const std::vector<double>& y,
                        std::string name = "",
                        std::vector<std::string> x_labels = {},
                        std::optional<Style> style = std::nullopt) {
    detail::ensure_same_size(x, y, "Figure2D::add_scatter");
    if (!x_labels.empty() && x_labels.size() != x.size()) {
      throw std::invalid_argument("plotism::Figure2D::add_scatter: x_labels size mismatch");
    }
    Trace2D t;
    t.kind = Trace2DKind::Scatter;
    t.name = std::move(name);
    t.x = x;
    t.y = y;
    t.x_labels = std::move(x_labels);
    t.style = style.value_or(current_().style);
    current_().traces.push_back(std::move(t));
    return *this;
  }

  Figure2D& add_scatter(const pluss::Series& x,
                        const pluss::Series& y,
                        std::string name = "",
                        std::vector<std::string> x_labels = {},
                        std::optional<Style> style = std::nullopt) {
    return add_scatter(detail::to_plot_vector(x, "Figure2D::add_scatter"),
                       detail::to_plot_vector(y, "Figure2D::add_scatter"),
                       std::move(name),
                       std::move(x_labels),
                       style);
  }

  template <typename XInput, typename YInput>
    requires detail::PlotInput<XInput> && detail::PlotInput<YInput>
  Figure2D& add_scatter(const XInput& x,
                        const YInput& y,
                        std::string name = "",
                        std::vector<std::string> x_labels = {},
                        std::optional<Style> style = std::nullopt) {
    return add_scatter(detail::to_plot_vector(x, "Figure2D::add_scatter"),
                       detail::to_plot_vector(y, "Figure2D::add_scatter"),
                       std::move(name),
                       std::move(x_labels),
                       style);
  }

  Figure2D& add_bar(const std::vector<double>& y,
                    std::string name = "",
                    std::vector<std::string> x_labels = {},
                    std::optional<Style> style = std::nullopt) {
    std::vector<double> x = detail::make_index_axis(y.size());
    return add_bar(x, y, std::move(name), std::move(x_labels), style);
  }

  Figure2D& add_bar(const pluss::Series& y,
                    std::string name = "",
                    std::vector<std::string> x_labels = {},
                    std::optional<Style> style = std::nullopt) {
    return add_bar(detail::to_plot_vector(y, "Figure2D::add_bar"), std::move(name), std::move(x_labels), style);
  }

  template <typename YInput>
    requires detail::PlotInput<YInput>
  Figure2D& add_bar(const YInput& y,
                    std::string name = "",
                    std::vector<std::string> x_labels = {},
                    std::optional<Style> style = std::nullopt) {
    return add_bar(detail::to_plot_vector(y, "Figure2D::add_bar"), std::move(name), std::move(x_labels), style);
  }

  Figure2D& add_bar(const std::vector<double>& x,
                    const std::vector<double>& y,
                    std::string name = "",
                    std::vector<std::string> x_labels = {},
                    std::optional<Style> style = std::nullopt) {
    return add_bar(x, y, std::move(name), std::move(x_labels), {}, {}, style);
  }

  Figure2D& add_bar(const std::vector<double>& x,
                    const std::vector<double>& y,
                    std::string name,
                    std::vector<std::string> x_labels,
                    std::vector<double> widths,
                    std::vector<double> baselines = {},
                    std::optional<Style> style = std::nullopt) {
    detail::ensure_same_size(x, y, "Figure2D::add_bar");
    if (!x_labels.empty() && x_labels.size() != x.size()) {
      throw std::invalid_argument("plotism::Figure2D::add_bar: x_labels size mismatch");
    }
    if (!widths.empty() && widths.size() != x.size()) {
      throw std::invalid_argument("plotism::Figure2D::add_bar: widths size mismatch");
    }
    if (!baselines.empty() && baselines.size() != x.size()) {
      throw std::invalid_argument("plotism::Figure2D::add_bar: baselines size mismatch");
    }
    Trace2D t;
    t.kind = Trace2DKind::Bar;
    t.name = std::move(name);
    t.x = x;
    t.x2 = std::move(widths);
    t.y = y;
    t.y2 = std::move(baselines);
    t.x_labels = std::move(x_labels);
    t.style = style.value_or(current_().style);
    current_().traces.push_back(std::move(t));
    return *this;
  }

  Figure2D& add_bar(const pluss::Series& x,
                    const pluss::Series& y,
                    std::string name = "",
                    std::vector<std::string> x_labels = {},
                    std::optional<Style> style = std::nullopt) {
    return add_bar(detail::to_plot_vector(x, "Figure2D::add_bar"),
                   detail::to_plot_vector(y, "Figure2D::add_bar"),
                   std::move(name),
                   std::move(x_labels),
                   style);
  }

  template <typename XInput, typename YInput>
    requires detail::PlotInput<XInput> && detail::PlotInput<YInput>
  Figure2D& add_bar(const XInput& x,
                    const YInput& y,
                    std::string name = "",
                    std::vector<std::string> x_labels = {},
                    std::optional<Style> style = std::nullopt) {
    return add_bar(detail::to_plot_vector(x, "Figure2D::add_bar"),
                   detail::to_plot_vector(y, "Figure2D::add_bar"),
                   std::move(name),
                   std::move(x_labels),
                   style);
  }

  Figure2D& add_hist(const std::vector<double>& values,
                     std::int64_t bins = 20,
                     std::string name = "",
                     std::optional<Style> style = std::nullopt) {
    if (bins <= 0) {
      throw std::invalid_argument("plotism::Figure2D::add_hist: bins must be > 0");
    }
    if (values.empty()) {
      throw std::invalid_argument("plotism::Figure2D::add_hist: values contain no finite points");
    }

    const auto value_series = pluss::make_series(values, {static_cast<std::int64_t>(values.size())});
    const auto finite_series = value_series.masked_select(value_series.isfinite());
    const auto finite = finite_series.to_vec<double>();
    if (finite.empty()) {
      throw std::invalid_argument("plotism::Figure2D::add_hist: values contain no finite points");
    }

    const double min_v = finite_series.amin().item<double>();
    const double max_v = finite_series.amax().item<double>();
    const double span = max_v - min_v;
    const double step = (span == 0.0) ? 1.0 : (span / static_cast<double>(bins));

    std::vector<double> x(static_cast<std::size_t>(bins), 0.0);
    std::vector<double> y(static_cast<std::size_t>(bins), 0.0);
    std::vector<std::string> labels;
    labels.reserve(static_cast<std::size_t>(bins));
    for (std::int64_t i = 0; i < bins; ++i) {
      const double left = min_v + static_cast<double>(i) * step;
      const double right = left + step;
      x[static_cast<std::size_t>(i)] = left + 0.5 * step;
      labels.push_back(fmt::format("[{:.6g},{:.6g})", left, right));
    }

    if (span == 0.0) {
      y[0] = static_cast<double>(finite.size());
    } else {
      const auto counts = finite_series.histc(bins, min_v, max_v).to_vec<double>();
      if (counts.size() != y.size()) {
        throw std::runtime_error("plotism::Figure2D::add_hist: unexpected histc result size");
      }
      y = counts;
    }

    Trace2D t;
    t.kind = Trace2DKind::Hist;
    t.name = std::move(name);
    t.x = std::move(x);
    t.y = std::move(y);
    t.x_labels = std::move(labels);
    t.bins = bins;
    t.style = style.value_or(current_().style);
    current_().traces.push_back(std::move(t));
    return *this;
  }

  Figure2D& add_hist(const pluss::Series& values,
                     std::int64_t bins = 20,
                     std::string name = "",
                     std::optional<Style> style = std::nullopt) {
    return add_hist(detail::to_plot_vector(values, "Figure2D::add_hist"),
                    bins,
                    std::move(name),
                    style);
  }

  template <typename ValuesInput>
    requires detail::PlotInput<ValuesInput>
  Figure2D& add_hist(const ValuesInput& values,
                     std::int64_t bins = 20,
                     std::string name = "",
                     std::optional<Style> style = std::nullopt) {
    return add_hist(detail::to_plot_vector(values, "Figure2D::add_hist"),
                    bins,
                    std::move(name),
                    style);
  }

  Figure2D& add_step(const std::vector<double>& y, std::string name = "") {
    std::vector<double> x = detail::make_index_axis(y.size());
    return add_step(x, y, std::move(name));
  }

  Figure2D& add_step(const pluss::Series& y, std::string name = "") {
    return add_step(detail::to_plot_vector(y, "Figure2D::add_step"), std::move(name));
  }

  template <typename YInput>
    requires detail::PlotInput<YInput>
  Figure2D& add_step(const YInput& y, std::string name = "") {
    return add_step(detail::to_plot_vector(y, "Figure2D::add_step"), std::move(name));
  }

  Figure2D& add_step(const std::vector<double>& x,
                     const std::vector<double>& y,
                     std::string name = "",
                     std::vector<std::string> x_labels = {},
                     std::optional<Style> style = std::nullopt) {
    detail::ensure_same_size(x, y, "Figure2D::add_step");
    if (!x_labels.empty() && x_labels.size() != x.size()) {
      throw std::invalid_argument("plotism::Figure2D::add_step: x_labels size mismatch");
    }
    Trace2D t;
    t.kind = Trace2DKind::Step;
    t.name = std::move(name);
    t.x = x;
    t.y = y;
    t.x_labels = std::move(x_labels);
    t.style = style.value_or(current_().style);
    current_().traces.push_back(std::move(t));
    return *this;
  }

  Figure2D& add_step(const pluss::Series& x,
                     const pluss::Series& y,
                     std::string name = "",
                     std::vector<std::string> x_labels = {},
                     std::optional<Style> style = std::nullopt) {
    return add_step(detail::to_plot_vector(x, "Figure2D::add_step"),
                    detail::to_plot_vector(y, "Figure2D::add_step"),
                    std::move(name),
                    std::move(x_labels),
                    style);
  }

  template <typename XInput, typename YInput>
    requires detail::PlotInput<XInput> && detail::PlotInput<YInput>
  Figure2D& add_step(const XInput& x,
                     const YInput& y,
                     std::string name = "",
                     std::vector<std::string> x_labels = {},
                     std::optional<Style> style = std::nullopt) {
    return add_step(detail::to_plot_vector(x, "Figure2D::add_step"),
                    detail::to_plot_vector(y, "Figure2D::add_step"),
                    std::move(name),
                    std::move(x_labels),
                    style);
  }

  Figure2D& add_area(const std::vector<double>& y, std::string name = "", std::optional<Style> style = std::nullopt) {
    std::vector<double> x = detail::make_index_axis(y.size());
    return add_area(x, y, std::move(name), style);
  }

  Figure2D& add_area(const pluss::Series& y, std::string name = "", std::optional<Style> style = std::nullopt) {
    return add_area(detail::to_plot_vector(y, "Figure2D::add_area"), std::move(name), style);
  }

  template <typename YInput>
    requires detail::PlotInput<YInput>
  Figure2D& add_area(const YInput& y, std::string name = "", std::optional<Style> style = std::nullopt) {
    return add_area(detail::to_plot_vector(y, "Figure2D::add_area"), std::move(name), style);
  }

  Figure2D& add_area(const std::vector<double>& x,
                     const std::vector<double>& y,
                     std::string name = "",
                     std::optional<Style> style = std::nullopt) {
    detail::ensure_same_size(x, y, "Figure2D::add_area");
    Trace2D t;
    t.kind = Trace2DKind::Area;
    t.name = std::move(name);
    t.x = x;
    t.y = y;
    t.y2 = detail::make_constant_axis(y.size(), 0.0);
    t.style = style.value_or(current_().style);
    current_().traces.push_back(std::move(t));
    return *this;
  }

  Figure2D& add_area(const pluss::Series& x,
                     const pluss::Series& y,
                     std::string name = "",
                     std::optional<Style> style = std::nullopt) {
    return add_area(detail::to_plot_vector(x, "Figure2D::add_area"),
                    detail::to_plot_vector(y, "Figure2D::add_area"),
                    std::move(name),
                    style);
  }

  template <typename XInput, typename YInput>
    requires detail::PlotInput<XInput> && detail::PlotInput<YInput>
  Figure2D& add_area(const XInput& x,
                     const YInput& y,
                     std::string name = "",
                     std::optional<Style> style = std::nullopt) {
    return add_area(detail::to_plot_vector(x, "Figure2D::add_area"),
                    detail::to_plot_vector(y, "Figure2D::add_area"),
                    std::move(name),
                    style);
  }

  Figure2D& add_band(const std::vector<double>& lower,
                     const std::vector<double>& upper,
                     std::string name = "",
                     std::optional<Style> style = std::nullopt) {
    std::vector<double> x = detail::make_index_axis(lower.size());
    return add_band(x, lower, upper, std::move(name), style);
  }

  Figure2D& add_band(const std::vector<double>& x,
                     const std::vector<double>& lower,
                     const std::vector<double>& upper,
                     std::string name = "",
                     std::optional<Style> style = std::nullopt) {
    detail::ensure_same_size(x, lower, "Figure2D::add_band");
    detail::ensure_same_size(x, upper, "Figure2D::add_band");
    Trace2D t;
    t.kind = Trace2DKind::Band;
    t.name = std::move(name);
    t.x = x;
    t.y = lower;
    t.y2 = upper;
    t.style = style.value_or(current_().style);
    current_().traces.push_back(std::move(t));
    return *this;
  }

  Figure2D& add_band(const pluss::Series& x,
                     const pluss::Series& lower,
                     const pluss::Series& upper,
                     std::string name = "",
                     std::optional<Style> style = std::nullopt) {
    return add_band(detail::to_plot_vector(x, "Figure2D::add_band"),
                    detail::to_plot_vector(lower, "Figure2D::add_band"),
                    detail::to_plot_vector(upper, "Figure2D::add_band"),
                    std::move(name),
                    style);
  }

  template <typename XInput, typename LowerInput, typename UpperInput>
    requires detail::PlotInput<XInput> && detail::PlotInput<LowerInput> && detail::PlotInput<UpperInput>
  Figure2D& add_band(const XInput& x,
                     const LowerInput& lower,
                     const UpperInput& upper,
                     std::string name = "",
                     std::optional<Style> style = std::nullopt) {
    return add_band(detail::to_plot_vector(x, "Figure2D::add_band"),
                    detail::to_plot_vector(lower, "Figure2D::add_band"),
                    detail::to_plot_vector(upper, "Figure2D::add_band"),
                    std::move(name),
                    style);
  }

  Figure2D& add_errorbar(const std::vector<double>& y,
                         const std::vector<double>& yerr,
                         std::string name = "",
                         std::optional<Style> style = std::nullopt) {
    std::vector<double> x = detail::make_index_axis(y.size());
    return add_errorbar(x, y, yerr, std::move(name), style);
  }

  Figure2D& add_errorbar(const std::vector<double>& x,
                         const std::vector<double>& y,
                         const std::vector<double>& yerr,
                         std::string name = "",
                         std::optional<Style> style = std::nullopt) {
    detail::ensure_same_size(x, y, "Figure2D::add_errorbar");
    detail::ensure_same_size(x, yerr, "Figure2D::add_errorbar");
    Trace2D t;
    t.kind = Trace2DKind::ErrorBar;
    t.name = std::move(name);
    t.x = x;
    t.y = y;
    t.y2 = yerr;
    t.style = style.value_or(current_().style);
    current_().traces.push_back(std::move(t));
    return *this;
  }

  Figure2D& add_errorbar(const pluss::Series& x,
                         const pluss::Series& y,
                         const pluss::Series& yerr,
                         std::string name = "",
                         std::optional<Style> style = std::nullopt) {
    return add_errorbar(detail::to_plot_vector(x, "Figure2D::add_errorbar"),
                        detail::to_plot_vector(y, "Figure2D::add_errorbar"),
                        detail::to_plot_vector(yerr, "Figure2D::add_errorbar"),
                        std::move(name),
                        style);
  }

  template <typename XInput, typename YInput, typename ErrInput>
    requires detail::PlotInput<XInput> && detail::PlotInput<YInput> && detail::PlotInput<ErrInput>
  Figure2D& add_errorbar(const XInput& x,
                         const YInput& y,
                         const ErrInput& yerr,
                         std::string name = "",
                         std::optional<Style> style = std::nullopt) {
    return add_errorbar(detail::to_plot_vector(x, "Figure2D::add_errorbar"),
                        detail::to_plot_vector(y, "Figure2D::add_errorbar"),
                        detail::to_plot_vector(yerr, "Figure2D::add_errorbar"),
                        std::move(name),
                        style);
  }

  Figure2D& fill_between(const std::vector<double>& x,
                         const std::vector<double>& lower,
                         const std::vector<double>& upper,
                         std::string name = "",
                         std::optional<Style> style = std::nullopt) {
    return add_band(x, lower, upper, std::move(name), style);
  }

  Figure2D& add_heatmap(const std::vector<std::vector<double>>& values,
                        std::string name = "",
                        std::vector<std::string> x_labels = {},
                        std::vector<std::string> y_labels = {},
                        std::optional<Style> style = std::nullopt) {
    const auto data = detail::flatten_heatmap_rows(values, "Figure2D::add_heatmap");
    return add_heatmap_(data, std::move(name), std::move(x_labels), std::move(y_labels), style);
  }

  Figure2D& add_heatmap(const pluss::Series& values,
                        std::string name = "",
                        std::vector<std::string> x_labels = {},
                        std::vector<std::string> y_labels = {},
                        std::optional<Style> style = std::nullopt) {
    const auto data = detail::flatten_heatmap_series(values, "Figure2D::add_heatmap");
    return add_heatmap_(data, std::move(name), std::move(x_labels), std::move(y_labels), style);
  }

  Figure2D& annotate(std::string text, double x, double y, std::optional<Style> style = std::nullopt) {
    Trace2D t;
    t.kind = Trace2DKind::Annotation;
    t.text = std::move(text);
    t.x = {x};
    t.y = {y};
    t.style = style.value_or(current_().style);
    current_().traces.push_back(std::move(t));
    return *this;
  }

  Figure2D& annotate(std::string text,
                     double x,
                     double y,
                     double x_text,
                     double y_text,
                     std::optional<Style> style = std::nullopt) {
    Trace2D t;
    t.kind = Trace2DKind::Annotation;
    t.text = std::move(text);
    t.x = {x};
    t.y = {y};
    t.x2 = {x_text};
    t.y2 = {y_text};
    t.style = style.value_or(current_().style);
    current_().traces.push_back(std::move(t));
    return *this;
  }

  Figure2D& label(std::string text, double x, double y, std::optional<Style> style = std::nullopt) {
    return annotate(std::move(text), x, y, style);
  }

  std::string to_string() const {
    std::string out = fmt::format("Figure2D(subplots={}", subplots_.size());
    if (!figure_title_.empty()) {
      out += fmt::format(", title=\"{}\"", figure_title_);
    }
    out += ")\n";
    for (std::size_t i = 0; i < subplots_.size(); ++i) {
      out += fmt::format("  subplot[{}]: traces={}\n", i, subplots_[i].traces.size());
    }
    return out;
  }

  std::string to_json() const {
    std::ostringstream os;
    os << "{";
    os << "\"type\":\"figure2d\"";
    os << ",\"title\":\"" << detail::escape_json(figure_title_) << "\"";
    os << ",\"rows\":" << rows_;
    os << ",\"cols\":" << cols_;
    os << ",\"subplots\":[";
    for (std::size_t i = 0; i < subplots_.size(); ++i) {
      if (i > 0) {
        os << ",";
      }
      const auto& s = subplots_[i];
      os << "{";
      os << "\"title\":\"" << detail::escape_json(s.title) << "\"";
      os << ",\"x_label\":\"" << detail::escape_json(s.x_axis.label) << "\"";
      os << ",\"y_label\":\"" << detail::escape_json(s.y_axis.label) << "\"";
      if (s.colorbar.has_value()) {
        os << ",\"colorbar\":{\"visible\":" << (s.colorbar->visible ? "true" : "false")
           << ",\"title\":\"" << detail::escape_json(s.colorbar->title) << "\"}";
      }
      os << ",\"traces\":[";
      for (std::size_t j = 0; j < s.traces.size(); ++j) {
        if (j > 0) {
          os << ",";
        }
        const auto& t = s.traces[j];
        os << "{";
        os << "\"kind\":\"" << trace_kind_name_(t.kind) << "\"";
        os << ",\"name\":\"" << detail::escape_json(t.name) << "\"";
        os << ",\"x\":" << detail::vector_to_json(t.x);
        os << ",\"y\":" << detail::vector_to_json(t.y);
        if (!t.x_labels.empty()) {
          os << ",\"x_labels\":[";
          for (std::size_t k = 0; k < t.x_labels.size(); ++k) {
            if (k > 0) {
              os << ",";
            }
            os << "\"" << detail::escape_json(t.x_labels[k]) << "\"";
          }
          os << "]";
        }
        os << "}";
      }
      os << "]}";
    }
    os << "]}";
    return os.str();
  }

  void show(std::ostream& os) const {
    os << to_string();
    if (subplots_.empty() || subplots_.front().traces.empty()) {
      os << "\n";
      return;
    }
    const auto& t = subplots_.front().traces.front();
    const auto n = std::min<std::size_t>(10, std::min(t.x.size(), t.y.size()));
    os << "  preview: ";
    for (std::size_t i = 0; i < n; ++i) {
      if (i > 0) {
        os << ", ";
      }
      os << "(" << detail::format_double(t.x[i]) << ", " << detail::format_double(t.y[i]) << ")";
    }
    if (t.x.size() > n) {
      os << ", ...";
    }
    os << "\n\n";
  }

  bool show_window(bool block = false,
                   OutputFormat format = OutputFormat::Svg,
                   std::int32_t width = 1200,
                   std::int32_t height = 800) const {
    const std::string path = detail::make_temp_plot_path("plusspl_2d", detail::output_extension(format));
    ensure_backend_().save(make_view_(),
                           RenderTarget{.path = path, .width = width, .height = height, .format = format});
    return detail::open_in_default_viewer(path, block);
  }

  void show() const {
#if PLUSSPL_ENABLE_INTERACTIVE_SHOW
    try {
      if (show_window(false, OutputFormat::Svg)) {
        return;
      }
    } catch (...) {
      // Fallback to textual preview.
    }
#endif
    show(std::cout);
  }

  void save_svg(std::string_view filepath, std::int32_t width = 1200, std::int32_t height = 800) const {
    ensure_backend_().save(make_view_(),
                           RenderTarget{.path = filepath, .width = width, .height = height, .format = OutputFormat::Svg});
  }

  void save_png(std::string_view filepath, std::int32_t width = 1200, std::int32_t height = 800) const {
    ensure_backend_().save(make_view_(),
                           RenderTarget{.path = filepath, .width = width, .height = height, .format = OutputFormat::Png});
  }

  void save_pdf(std::string_view filepath, std::int32_t width = 1200, std::int32_t height = 800) const {
    ensure_backend_().save(make_view_(),
                           RenderTarget{.path = filepath, .width = width, .height = height, .format = OutputFormat::Pdf});
  }

private:
  static const char* trace_kind_name_(Trace2DKind k) noexcept {
    switch (k) {
      case Trace2DKind::Line: return "line";
      case Trace2DKind::Scatter: return "scatter";
      case Trace2DKind::Bar: return "bar";
      case Trace2DKind::Hist: return "hist";
      case Trace2DKind::Step: return "step";
      case Trace2DKind::Area: return "area";
      case Trace2DKind::Band: return "band";
      case Trace2DKind::Heatmap: return "heatmap";
      case Trace2DKind::Annotation: return "annotation";
      case Trace2DKind::ErrorBar: return "errorbar";
      default: return "trace";
    }
  }

  Figure2DView make_view_() const {
    return Figure2DView{.title = figure_title_, .rows = rows_, .cols = cols_, .subplots = subplots_};
  }

  const IBackend& ensure_backend_() const {
    if (!backend_) {
      backend_ = make_default_backend();
    }
    return *backend_;
  }

  Subplot2D& current_() {
    return subplots_[static_cast<std::size_t>(active_)];
  }
  const Subplot2D& current_() const {
    return subplots_[static_cast<std::size_t>(active_)];
  }

  void apply_figure_style_defaults_(Subplot2D& subplot) const {
    if (subplot.style.theme.empty()) {
      subplot.style.theme = figure_style_.theme;
    }
    if (subplot.style.font_family.empty()) {
      subplot.style.font_family = figure_style_.font_family;
    }
  }

  Figure2D& add_heatmap_(const detail::HeatmapData2D& data,
                         std::string name,
                         std::vector<std::string> x_labels,
                         std::vector<std::string> y_labels,
                         std::optional<Style> style) {
    if (!x_labels.empty() && static_cast<std::int64_t>(x_labels.size()) != data.cols) {
      throw std::invalid_argument("plotism::Figure2D::add_heatmap: x_labels size mismatch");
    }
    if (!y_labels.empty() && static_cast<std::int64_t>(y_labels.size()) != data.rows) {
      throw std::invalid_argument("plotism::Figure2D::add_heatmap: y_labels size mismatch");
    }
    Trace2D t;
    t.kind = Trace2DKind::Heatmap;
    t.name = std::move(name);
    t.x = detail::make_index_axis(static_cast<std::size_t>(data.cols));
    t.y2 = detail::make_index_axis(static_cast<std::size_t>(data.rows));
    t.values = data.values;
    t.rows = data.rows;
    t.cols = data.cols;
    t.x_labels = std::move(x_labels);
    t.y_labels = std::move(y_labels);
    t.style = style.value_or(current_().style);
    if (!current_().colorbar.has_value()) {
      current_().colorbar = Colorbar{.visible = true, .location = ColorbarLocation::Right, .title = t.name};
    } else if (current_().colorbar->title.empty() && !t.name.empty()) {
      current_().colorbar->title = t.name;
    }
    current_().traces.push_back(std::move(t));
    return *this;
  }

  std::int32_t rows_{1};
  std::int32_t cols_{1};
  std::int32_t active_{0};
  std::string figure_title_{};
  Style figure_style_{};
  std::vector<Subplot2D> subplots_{};
  mutable std::shared_ptr<const IBackend> backend_{};
};

} // namespace pluss::plotism
