#pragma once

#include <cmath>
#include <cstdint>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "pluss/plotism/common.h"

namespace pluss::plotism {

enum class Trace2DKind : std::uint8_t {
  Line = 0,
  Scatter = 1,
  Bar = 2,
  Hist = 3,
};

struct Trace2D {
  Trace2DKind kind{Trace2DKind::Line};
  std::string name{};
  std::vector<double> x{};
  std::vector<double> y{};
  std::vector<std::string> x_labels{};
  std::int64_t bins{0};
  Style style{};
};

struct Subplot2D {
  std::string title{};
  Axis x_axis{};
  Axis y_axis{};
  Legend legend{};
  Style style{};
  std::vector<Trace2D> traces{};
};

class Figure2D {
public:
  Figure2D(std::int32_t rows = 1, std::int32_t cols = 1) : rows_(rows), cols_(cols) {
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

  Figure2D& legend(const Legend& l) {
    current_().legend = l;
    return *this;
  }

  Figure2D& style(const Style& s) {
    current_().style = s;
    return *this;
  }

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
      std::ostringstream ls;
      ls << std::setprecision(6) << "[" << left << "," << right << ")";
      labels.push_back(ls.str());
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
    std::ostringstream os;
    os << "Figure2D(subplots=" << subplots_.size();
    if (!figure_title_.empty()) {
      os << ", title=\"" << figure_title_ << "\"";
    }
    os << ")\n";
    for (std::size_t i = 0; i < subplots_.size(); ++i) {
      os << "  subplot[" << i << "]: traces=" << subplots_[i].traces.size() << "\n";
    }
    return os.str();
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

  void show() const { show(std::cout); }

  void save_svg(std::string_view filepath, std::int32_t width = 1200, std::int32_t height = 800) const {
    if (width <= 100 || height <= 100) {
      throw std::invalid_argument("plotism::Figure2D::save_svg: width/height too small");
    }

    std::ostringstream svg;
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << width << "\" height=\"" << height << "\" ";
    svg << "viewBox=\"0 0 " << width << " " << height << "\">\n";
    svg << "<rect x=\"0\" y=\"0\" width=\"" << width << "\" height=\"" << height << "\" fill=\"white\"/>\n";
    if (!figure_title_.empty()) {
      svg << "<text x=\"" << (width / 2) << "\" y=\"24\" text-anchor=\"middle\" font-size=\"20\" font-family=\"sans-serif\">";
      svg << detail::escape_xml(figure_title_) << "</text>\n";
    }

    const int top_margin = 44;
    const int gap = 18;
    const int cell_w = (width - gap * (cols_ + 1)) / cols_;
    const int cell_h = (height - top_margin - gap * (rows_ + 1)) / rows_;

    const auto& palette = detail::default_palette();
    for (int r = 0; r < rows_; ++r) {
      for (int c = 0; c < cols_; ++c) {
        const int idx = r * cols_ + c;
        const auto& sub = subplots_[static_cast<std::size_t>(idx)];
        const int x0 = gap + c * (cell_w + gap);
        const int y0 = top_margin + gap + r * (cell_h + gap);

        svg << "<rect x=\"" << x0 << "\" y=\"" << y0 << "\" width=\"" << cell_w << "\" height=\"" << cell_h
            << "\" fill=\"none\" stroke=\"#d0d0d0\"/>\n";

        const int left = x0 + 50;
        const int right = x0 + cell_w - 16;
        const int top = y0 + 26;
        const int bottom = y0 + cell_h - 42;
        const int plot_w = std::max(1, right - left);
        const int plot_h = std::max(1, bottom - top);

        svg << "<line x1=\"" << left << "\" y1=\"" << bottom << "\" x2=\"" << right << "\" y2=\"" << bottom
            << "\" stroke=\"#222\" stroke-width=\"1\"/>\n";
        svg << "<line x1=\"" << left << "\" y1=\"" << top << "\" x2=\"" << left << "\" y2=\"" << bottom
            << "\" stroke=\"#222\" stroke-width=\"1\"/>\n";

        if (!sub.title.empty()) {
          svg << "<text x=\"" << (x0 + cell_w / 2) << "\" y=\"" << (y0 + 18)
              << "\" text-anchor=\"middle\" font-size=\"13\" font-family=\"sans-serif\">"
              << detail::escape_xml(sub.title) << "</text>\n";
        }
        if (!sub.x_axis.label.empty()) {
          svg << "<text x=\"" << (x0 + cell_w / 2) << "\" y=\"" << (y0 + cell_h - 10)
              << "\" text-anchor=\"middle\" font-size=\"12\" font-family=\"sans-serif\">"
              << detail::escape_xml(sub.x_axis.label) << "</text>\n";
        }
        if (!sub.y_axis.label.empty()) {
          svg << "<text x=\"" << (x0 + 14) << "\" y=\"" << (y0 + cell_h / 2)
              << "\" text-anchor=\"middle\" font-size=\"12\" font-family=\"sans-serif\" transform=\"rotate(-90 "
              << (x0 + 14) << " " << (y0 + cell_h / 2) << ")\">" << detail::escape_xml(sub.y_axis.label)
              << "</text>\n";
        }

        // Compute bounds.
        double xmin = std::numeric_limits<double>::infinity();
        double xmax = -std::numeric_limits<double>::infinity();
        double ymin = std::numeric_limits<double>::infinity();
        double ymax = -std::numeric_limits<double>::infinity();
        for (const auto& t : sub.traces) {
          const auto [xlo, xhi] = detail::finite_minmax(t.x);
          const auto [ylo, yhi] = detail::finite_minmax(t.y);
          xmin = std::min(xmin, xlo);
          xmax = std::max(xmax, xhi);
          ymin = std::min(ymin, ylo);
          ymax = std::max(ymax, yhi);
          if (t.kind == Trace2DKind::Bar || t.kind == Trace2DKind::Hist) {
            ymin = std::min(ymin, 0.0);
          }
        }
        if (!std::isfinite(xmin) || !std::isfinite(xmax)) {
          xmin = 0.0;
          xmax = 1.0;
        }
        if (!std::isfinite(ymin) || !std::isfinite(ymax)) {
          ymin = 0.0;
          ymax = 1.0;
        }
        if (sub.x_axis.min.has_value()) xmin = *sub.x_axis.min;
        if (sub.x_axis.max.has_value()) xmax = *sub.x_axis.max;
        if (sub.y_axis.min.has_value()) ymin = *sub.y_axis.min;
        if (sub.y_axis.max.has_value()) ymax = *sub.y_axis.max;
        if (xmin == xmax) xmax += 1.0;
        if (ymin == ymax) ymax += 1.0;

        const auto sx = [&](double x) {
          const double xn = (x - xmin) / (xmax - xmin);
          return left + xn * static_cast<double>(plot_w);
        };
        const auto sy = [&](double y) {
          const double yn = (y - ymin) / (ymax - ymin);
          return bottom - yn * static_cast<double>(plot_h);
        };

        for (std::size_t ti = 0; ti < sub.traces.size(); ++ti) {
          const auto& t = sub.traces[ti];
          std::string color = t.style.color.empty() ? palette[ti % palette.size()] : t.style.color;
          if (t.kind == Trace2DKind::Line) {
            svg << "<polyline fill=\"none\" stroke=\"" << color << "\" stroke-width=\"" << t.style.line_width
                << "\" points=\"";
            for (std::size_t i = 0; i < t.x.size() && i < t.y.size(); ++i) {
              if (!std::isfinite(t.x[i]) || !std::isfinite(t.y[i])) {
                continue;
              }
              svg << detail::format_double(sx(t.x[i]), 6) << "," << detail::format_double(sy(t.y[i]), 6) << " ";
            }
            svg << "\"/>\n";
          } else if (t.kind == Trace2DKind::Scatter) {
            for (std::size_t i = 0; i < t.x.size() && i < t.y.size(); ++i) {
              if (!std::isfinite(t.x[i]) || !std::isfinite(t.y[i])) {
                continue;
              }
              svg << "<circle cx=\"" << detail::format_double(sx(t.x[i]), 6)
                  << "\" cy=\"" << detail::format_double(sy(t.y[i]), 6)
                  << "\" r=\"2.5\" fill=\"" << color << "\" fill-opacity=\"" << t.style.alpha << "\"/>\n";
            }
          } else {
            const double barw = std::max(2.0, static_cast<double>(plot_w) / std::max<std::size_t>(1, t.x.size()) * 0.7);
            for (std::size_t i = 0; i < t.x.size() && i < t.y.size(); ++i) {
              if (!std::isfinite(t.x[i]) || !std::isfinite(t.y[i])) {
                continue;
              }
              const auto px = sx(t.x[i]) - barw * 0.5;
              const auto py = sy(std::max(0.0, t.y[i]));
              const auto p0 = sy(0.0);
              const auto h = std::abs(p0 - py);
              const auto ytop = std::min(py, p0);
              svg << "<rect x=\"" << detail::format_double(px, 6) << "\" y=\"" << detail::format_double(ytop, 6)
                  << "\" width=\"" << detail::format_double(barw, 6)
                  << "\" height=\"" << detail::format_double(h, 6) << "\" fill=\"" << color
                  << "\" fill-opacity=\"" << t.style.alpha << "\"/>\n";
            }
          }
        }

        // Simple legend in top-right.
        if (sub.legend.visible && !sub.traces.empty()) {
          double lx = right - 120;
          double ly = top + 6;
          svg << "<rect x=\"" << lx << "\" y=\"" << ly << "\" width=\"114\" height=\""
              << (sub.traces.size() * 15 + 8) << "\" fill=\"white\" fill-opacity=\"0.85\" stroke=\"#aaa\"/>\n";
          for (std::size_t ti = 0; ti < sub.traces.size(); ++ti) {
            const auto& t = sub.traces[ti];
            const std::string color = t.style.color.empty() ? palette[ti % palette.size()] : t.style.color;
            const double y = ly + 14 + static_cast<double>(ti) * 15.0;
            svg << "<line x1=\"" << (lx + 6) << "\" y1=\"" << y << "\" x2=\"" << (lx + 20) << "\" y2=\"" << y
                << "\" stroke=\"" << color << "\" stroke-width=\"2\"/>\n";
            svg << "<text x=\"" << (lx + 24) << "\" y=\"" << (y + 4)
                << "\" font-size=\"11\" font-family=\"sans-serif\">"
                << detail::escape_xml(t.name.empty() ? std::string("trace") : t.name) << "</text>\n";
          }
        }
      }
    }

    svg << "</svg>\n";
    detail::write_text_file(filepath, svg.str());
  }

  void save_png(std::string_view filepath, std::int32_t width = 1200, std::int32_t height = 800) const {
    (void)filepath;
    (void)width;
    (void)height;
    throw std::runtime_error(
        "plotism::Figure2D::save_png is not enabled in this build. Use save_svg() or wire a raster backend.");
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
};

inline Figure2D line(const std::vector<double>& y, std::string name = "") {
  Figure2D fig;
  fig.add_line(y, std::move(name));
  return fig;
}

inline Figure2D line(const std::vector<double>& x, const std::vector<double>& y, std::string name = "") {
  Figure2D fig;
  fig.add_line(x, y, std::move(name));
  return fig;
}

inline Figure2D scatter(const std::vector<double>& x, const std::vector<double>& y, std::string name = "") {
  Figure2D fig;
  fig.add_scatter(x, y, std::move(name));
  return fig;
}

inline Figure2D bar(const std::vector<double>& y, std::string name = "") {
  Figure2D fig;
  fig.add_bar(y, std::move(name));
  return fig;
}

inline Figure2D bar(const std::vector<double>& x, const std::vector<double>& y, std::string name = "") {
  Figure2D fig;
  fig.add_bar(x, y, std::move(name));
  return fig;
}

inline Figure2D hist(const std::vector<double>& values, std::int64_t bins = 20, std::string name = "") {
  Figure2D fig;
  fig.add_hist(values, bins, std::move(name));
  return fig;
}

} // namespace pluss::plotism

