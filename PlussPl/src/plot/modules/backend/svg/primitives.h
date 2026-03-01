#pragma once

#include <algorithm>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "plot/modules/backend/render_surface_2d.h"

namespace pluss::plotism {

namespace detail {

inline double axis_transform(double value, const Axis& axis) {
  return axis_transform_2d(value, axis);
}

inline std::pair<double, double> ensure_limits(double lo, double hi) {
  return ensure_limits_2d(lo, hi);
}

inline std::pair<double, double> map_legend_anchor(LegendLocation location,
                                                   double left,
                                                   double right,
                                                   double top,
                                                   double bottom,
                                                   double legend_width,
                                                   double legend_height) {
  return map_legend_anchor_2d(location, left, right, top, bottom, legend_width, legend_height);
}

inline double nice_tick_step(double lo, double hi, int approx_tick_count = 5) {
  return nice_tick_step_2d(lo, hi, approx_tick_count);
}

inline std::vector<double> make_ticks(double lo, double hi, int approx_tick_count = 5) {
  return make_ticks_2d(lo, hi, approx_tick_count);
}

inline std::string format_tick_label(double transformed_value, const Axis& axis) {
  return format_tick_label_2d(transformed_value, axis);
}

inline double legend_width_hint(std::size_t entry_count, std::size_t longest_label, bool has_title) {
  return legend_width_hint_2d(entry_count, longest_label, has_title);
}

inline double legend_height_hint(std::size_t entry_count, bool has_title) {
  return legend_height_hint_2d(entry_count, has_title);
}

inline std::string svg_dasharray(std::string_view line_style, double line_width) {
  const double base = std::max(1.0, line_width);
  if (line_style == "dashed" || line_style == "dash" || line_style == "--") {
    return " stroke-dasharray=\"" + format_double(base * 5.0, 4) + "," + format_double(base * 3.0, 4) + "\"";
  }
  if (line_style == "dotted" || line_style == "dot" || line_style == ":") {
    return " stroke-dasharray=\"" + format_double(base, 4) + "," + format_double(base * 2.2, 4) + "\"";
  }
  if (line_style == "dashdot" || line_style == "-.") {
    return " stroke-dasharray=\"" + format_double(base * 5.0, 4) + "," + format_double(base * 2.2, 4) + ","
           + format_double(base, 4) + "," + format_double(base * 2.2, 4) + "\"";
  }
  return {};
}

inline std::string normalized_marker(std::string_view marker, bool scatter_like) {
  return canonical_marker_name(marker, scatter_like);
}

class SvgRenderSurface2D final : public RenderSurface2D {
public:
  void fill_rect(double x, double y, double w, double h, const FillStyle2D& style, double corner_radius = 0.0) override {
    if (!(w > 0.0 && h > 0.0)) {
      return;
    }
    stream_ << "<rect x=\"" << format_double(x, 6)
            << "\" y=\"" << format_double(y, 6)
            << "\" width=\"" << format_double(w, 6)
            << "\" height=\"" << format_double(h, 6) << "\"";
    if (corner_radius > 0.0) {
      stream_ << " rx=\"" << format_double(corner_radius, 6) << "\"";
    }
    stream_ << " fill=\"" << style.color << "\" fill-opacity=\"" << style.alpha << "\"/>\n";
  }

  void stroke_rect(double x,
                   double y,
                   double w,
                   double h,
                   const StrokeStyle2D& style,
                   double corner_radius = 0.0) override {
    if (!(w > 0.0 && h > 0.0)) {
      return;
    }
    stream_ << "<rect x=\"" << format_double(x, 6)
            << "\" y=\"" << format_double(y, 6)
            << "\" width=\"" << format_double(w, 6)
            << "\" height=\"" << format_double(h, 6) << "\"";
    if (corner_radius > 0.0) {
      stream_ << " rx=\"" << format_double(corner_radius, 6) << "\"";
    }
    stream_ << " fill=\"none\" stroke=\"" << style.color
            << "\" stroke-opacity=\"" << style.alpha
            << "\" stroke-width=\"" << format_double(style.line_width, 6) << "\""
            << svg_dasharray(style.line_style, style.line_width) << "/>\n";
  }

  void stroke_line(double x1, double y1, double x2, double y2, const StrokeStyle2D& style) override {
    stream_ << "<line x1=\"" << format_double(x1, 6)
            << "\" y1=\"" << format_double(y1, 6)
            << "\" x2=\"" << format_double(x2, 6)
            << "\" y2=\"" << format_double(y2, 6)
            << "\" stroke=\"" << style.color
            << "\" stroke-opacity=\"" << style.alpha
            << "\" stroke-width=\"" << format_double(style.line_width, 6)
            << "\" stroke-linecap=\"round\"" << svg_dasharray(style.line_style, style.line_width) << "/>\n";
  }

  void stroke_polyline(const std::vector<Point2D>& points, const StrokeStyle2D& style) override {
    if (points.size() < 2) {
      return;
    }
    stream_ << "<polyline fill=\"none\" stroke=\"" << style.color
            << "\" stroke-opacity=\"" << style.alpha
            << "\" stroke-width=\"" << format_double(style.line_width, 6)
            << "\" stroke-linejoin=\"round\" stroke-linecap=\"round\""
            << svg_dasharray(style.line_style, style.line_width) << " points=\"";
    for (const auto& point : points) {
      stream_ << format_double(point.x, 6) << "," << format_double(point.y, 6) << " ";
    }
    stream_ << "\"/>\n";
  }

  void fill_circle(double cx, double cy, double r, const FillStyle2D& style) override {
    if (!(r > 0.0)) {
      return;
    }
    stream_ << "<circle cx=\"" << format_double(cx, 6)
            << "\" cy=\"" << format_double(cy, 6)
            << "\" r=\"" << format_double(r, 6)
            << "\" fill=\"" << style.color
            << "\" fill-opacity=\"" << style.alpha << "\"/>\n";
  }

  void fill_polygon(const std::vector<Point2D>& points, const FillStyle2D& style) override {
    if (points.size() < 3) {
      return;
    }
    stream_ << "<polygon points=\"";
    for (const auto& point : points) {
      stream_ << format_double(point.x, 6) << "," << format_double(point.y, 6) << " ";
    }
    stream_ << "\" fill=\"" << style.color
            << "\" fill-opacity=\"" << style.alpha << "\"/>\n";
  }

  void text(double x, double y, std::string_view text, const TextStyle2D& style) override {
    if (text.empty()) {
      return;
    }
    stream_ << "<text x=\"" << format_double(x, 6)
            << "\" y=\"" << format_double(y, 6)
            << "\" text-anchor=\"" << anchor_name_(style.anchor)
            << "\" font-size=\"" << format_double(style.size, 6)
            << "\" font-family=\"" << escape_xml(style.font_family.empty() ? std::string("sans-serif") : style.font_family)
            << "\" fill=\"" << style.color
            << "\" fill-opacity=\"" << style.alpha << "\"";
    if (style.bold) {
      stream_ << " font-weight=\"600\"";
    }
    if (style.orientation == TextOrientation2D::VerticalCcw) {
      stream_ << " transform=\"rotate(-90 " << format_double(x, 6) << " " << format_double(y, 6) << ")\"";
    }
    stream_ << ">" << escape_xml(text) << "</text>\n";
  }

  std::string finish() const override { return stream_.str(); }

private:
  static const char* anchor_name_(TextAnchor2D anchor) {
    switch (anchor) {
      case TextAnchor2D::Center:
        return "middle";
      case TextAnchor2D::Right:
        return "end";
      case TextAnchor2D::Left:
      default:
        return "start";
    }
  }

  std::ostringstream stream_{};
};

inline void append_svg_marker(std::ostringstream& svg,
                              double x,
                              double y,
                              std::string_view marker,
                              double size,
                              std::string_view color,
                              double alpha) {
  const std::string kind = normalized_marker(marker, true);
  if (kind == "none") {
    return;
  }
  const double half = std::max(2.0, size);

  if (kind == "circle") {
    svg << "<circle cx=\"" << format_double(x, 6)
        << "\" cy=\"" << format_double(y, 6)
        << "\" r=\"" << format_double(half, 6)
        << "\" fill=\"" << color << "\" fill-opacity=\"" << alpha << "\"/>\n";
    return;
  }
  if (kind == "square") {
    svg << "<rect x=\"" << format_double(x - half, 6)
        << "\" y=\"" << format_double(y - half, 6)
        << "\" width=\"" << format_double(half * 2.0, 6)
        << "\" height=\"" << format_double(half * 2.0, 6)
        << "\" fill=\"" << color << "\" fill-opacity=\"" << alpha << "\"/>\n";
    return;
  }
  if (kind == "triangle") {
    svg << "<polygon points=\""
        << format_double(x, 6) << "," << format_double(y - half, 6) << " "
        << format_double(x + half, 6) << "," << format_double(y + half, 6) << " "
        << format_double(x - half, 6) << "," << format_double(y + half, 6)
        << "\" fill=\"" << color << "\" fill-opacity=\"" << alpha << "\"/>\n";
    return;
  }
  if (kind == "diamond") {
    svg << "<polygon points=\""
        << format_double(x, 6) << "," << format_double(y - half, 6) << " "
        << format_double(x + half, 6) << "," << format_double(y, 6) << " "
        << format_double(x, 6) << "," << format_double(y + half, 6) << " "
        << format_double(x - half, 6) << "," << format_double(y, 6)
        << "\" fill=\"" << color << "\" fill-opacity=\"" << alpha << "\"/>\n";
    return;
  }
  if (kind == "x") {
    svg << "<path d=\"M " << format_double(x - half, 6) << " " << format_double(y - half, 6)
        << " L " << format_double(x + half, 6) << " " << format_double(y + half, 6)
        << " M " << format_double(x - half, 6) << " " << format_double(y + half, 6)
        << " L " << format_double(x + half, 6) << " " << format_double(y - half, 6)
        << "\" fill=\"none\" stroke=\"" << color << "\" stroke-opacity=\"" << alpha
        << "\" stroke-width=\"1.4\" stroke-linecap=\"round\"/>\n";
    return;
  }
  if (kind == "plus") {
    svg << "<path d=\"M " << format_double(x - half, 6) << " " << format_double(y, 6)
        << " L " << format_double(x + half, 6) << " " << format_double(y, 6)
        << " M " << format_double(x, 6) << " " << format_double(y - half, 6)
        << " L " << format_double(x, 6) << " " << format_double(y + half, 6)
        << "\" fill=\"none\" stroke=\"" << color << "\" stroke-opacity=\"" << alpha
        << "\" stroke-width=\"1.4\" stroke-linecap=\"round\"/>\n";
  }
}

} // namespace detail
} // namespace pluss::plotism
