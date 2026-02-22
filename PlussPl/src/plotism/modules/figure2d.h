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

#include "pluss/plotism/modules/backend/default_backend.h"
#include "pluss/plotism/modules/core.h"
#include "pluss/plotism/modules/detail/show_helpers.h"
#include "pluss/plotism/modules/trace.h"

namespace pluss::plotism {
class Figure2D {
public:
  Figure2D(std::int32_t rows = 1, std::int32_t cols = 1)
      : rows_(rows), cols_(cols), backend_(make_default_backend()) {
    if (rows_ <= 0 || cols_ <= 0) {
      throw std::invalid_argument("plotism::Figure2D: rows/cols must be > 0");
    }
    subplots_.resize(static_cast<std::size_t>(rows_ * cols_));
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
    }
    active_ = index;
    return *this;
  }

  Figure2D& title(std::string title) {
    current_().title = std::move(title);
    return *this;
  }

  Figure2D& x_label(std::string label) {
    current_().x_axis.label = std::move(label);
    return *this;
  }

  Figure2D& y_label(std::string label) {
    current_().y_axis.label = std::move(label);
    return *this;
  }

  Figure2D& x_limits(double min_v, double max_v) {
    current_().x_axis.min = min_v;
    current_().x_axis.max = max_v;
    return *this;
  }

  Figure2D& y_limits(double min_v, double max_v) {
    current_().y_axis.min = min_v;
    current_().y_axis.max = max_v;
    return *this;
  }

  Figure2D& set_xlim(double min_v, double max_v) {
    return x_limits(min_v, max_v);
  }

  Figure2D& set_ylim(double min_v, double max_v) {
    return y_limits(min_v, max_v);
  }

  Figure2D& set_xscale(std::string_view scale) {
    current_().x_axis.scale = detail::parse_axis_scale(scale, "Figure2D::set_xscale");
    return *this;
  }

  Figure2D& set_yscale(std::string_view scale) {
    current_().y_axis.scale = detail::parse_axis_scale(scale, "Figure2D::set_yscale");
    return *this;
  }

  Figure2D& invert_xaxis(bool enabled = true) {
    current_().x_axis.inverted = enabled;
    return *this;
  }

  Figure2D& invert_yaxis(bool enabled = true) {
    current_().y_axis.inverted = enabled;
    return *this;
  }

  Figure2D& legend(const Legend& l) {
    current_().legend = l;
    return *this;
  }

  Figure2D& style(const Style& s) {
    current_().style = s;
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
    std::vector<double> x(y.size(), 0.0);
    for (std::size_t i = 0; i < y.size(); ++i) {
      x[i] = static_cast<double>(i);
    }
    return add_line(x, y, std::move(name));
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

  Figure2D& add_bar(const std::vector<double>& y,
                    std::string name = "",
                    std::vector<std::string> x_labels = {},
                    std::optional<Style> style = std::nullopt) {
    std::vector<double> x(y.size(), 0.0);
    for (std::size_t i = 0; i < y.size(); ++i) {
      x[i] = static_cast<double>(i);
    }
    return add_bar(x, y, std::move(name), std::move(x_labels), style);
  }

  Figure2D& add_bar(const std::vector<double>& x,
                    const std::vector<double>& y,
                    std::string name = "",
                    std::vector<std::string> x_labels = {},
                    std::optional<Style> style = std::nullopt) {
    detail::ensure_same_size(x, y, "Figure2D::add_bar");
    if (!x_labels.empty() && x_labels.size() != x.size()) {
      throw std::invalid_argument("plotism::Figure2D::add_bar: x_labels size mismatch");
    }
    Trace2D t;
    t.kind = Trace2DKind::Bar;
    t.name = std::move(name);
    t.x = x;
    t.y = y;
    t.x_labels = std::move(x_labels);
    t.style = style.value_or(current_().style);
    current_().traces.push_back(std::move(t));
    return *this;
  }

  Figure2D& add_hist(const std::vector<double>& values,
                     std::int64_t bins = 20,
                     std::string name = "",
                     std::optional<Style> style = std::nullopt) {
    if (bins <= 0) {
      throw std::invalid_argument("plotism::Figure2D::add_hist: bins must be > 0");
    }
    std::vector<double> finite;
    finite.reserve(values.size());
    for (const auto v : values) {
      if (std::isfinite(v)) {
        finite.push_back(v);
      }
    }
    if (finite.empty()) {
      throw std::invalid_argument("plotism::Figure2D::add_hist: values contain no finite points");
    }
    const auto [min_it, max_it] = std::minmax_element(finite.begin(), finite.end());
    const double min_v = *min_it;
    const double max_v = *max_it;
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
    for (const auto v : finite) {
      std::int64_t idx = 0;
      if (span != 0.0) {
        idx = static_cast<std::int64_t>((v - min_v) / step);
        if (idx >= bins) {
          idx = bins - 1;
        }
        if (idx < 0) {
          idx = 0;
        }
      }
      y[static_cast<std::size_t>(idx)] += 1.0;
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

  std::int32_t rows_{1};
  std::int32_t cols_{1};
  std::int32_t active_{0};
  std::string figure_title_{};
  std::vector<Subplot2D> subplots_{};
  mutable std::shared_ptr<const IBackend> backend_{};
};

} // namespace pluss::plotism
