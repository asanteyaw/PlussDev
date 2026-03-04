#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "plot/modules/backend/render_surface_2d.h"

namespace pluss::plotism {

namespace detail {

struct PdfRgb {
  double r{0.0};
  double g{0.0};
  double b{0.0};
};

inline int pdf_hex_digit(char c) {
  if (c >= '0' && c <= '9') {
    return static_cast<int>(c - '0');
  }
  if (c >= 'a' && c <= 'f') {
    return static_cast<int>(10 + c - 'a');
  }
  if (c >= 'A' && c <= 'F') {
    return static_cast<int>(10 + c - 'A');
  }
  return -1;
}

inline PdfRgb pdf_parse_color(std::string_view color) {
  if (color.size() == 7 && color[0] == '#') {
    const int r1 = pdf_hex_digit(color[1]);
    const int r2 = pdf_hex_digit(color[2]);
    const int g1 = pdf_hex_digit(color[3]);
    const int g2 = pdf_hex_digit(color[4]);
    const int b1 = pdf_hex_digit(color[5]);
    const int b2 = pdf_hex_digit(color[6]);
    if (r1 >= 0 && r2 >= 0 && g1 >= 0 && g2 >= 0 && b1 >= 0 && b2 >= 0) {
      return PdfRgb{
          .r = static_cast<double>(r1 * 16 + r2) / 255.0,
          .g = static_cast<double>(g1 * 16 + g2) / 255.0,
          .b = static_cast<double>(b1 * 16 + b2) / 255.0,
      };
    }
  }

  if (color == "white") return PdfRgb{1.0, 1.0, 1.0};
  if (color == "black") return PdfRgb{0.0, 0.0, 0.0};
  if (color == "red") return PdfRgb{1.0, 0.0, 0.0};
  if (color == "green") return PdfRgb{0.0, 0.6, 0.0};
  if (color == "blue") return PdfRgb{0.0, 0.0, 1.0};
  if (color == "gray" || color == "grey") return PdfRgb{0.5, 0.5, 0.5};
  if (color == "yellow") return PdfRgb{1.0, 1.0, 0.0};
  if (color == "orange") return PdfRgb{1.0, 0.55, 0.0};

  return PdfRgb{0.0, 0.0, 0.0};
}

inline std::string pdf_escape_text(std::string_view text) {
  std::string out;
  out.reserve(text.size() + 8);
  for (char c : text) {
    if (c == '\\' || c == '(' || c == ')') {
      out.push_back('\\');
      out.push_back(c);
      continue;
    }
    if (c == '\n' || c == '\r' || c == '\t') {
      out.push_back(' ');
      continue;
    }
    out.push_back(c);
  }
  return out;
}

inline double pdf_axis_transform(double value, const Axis& axis) {
  return axis_transform_2d(value, axis);
}

inline std::pair<double, double> pdf_ensure_limits(double lo, double hi) {
  return ensure_limits_2d(lo, hi);
}

inline std::pair<double, double> pdf_map_legend_anchor(LegendLocation location,
                                                       double left,
                                                       double right,
                                                       double top,
                                                       double bottom,
                                                       double legend_width,
                                                       double legend_height) {
  return map_legend_anchor_2d(location, left, right, top, bottom, legend_width, legend_height);
}

inline double pdf_nice_tick_step(double lo, double hi, int approx_tick_count = 5) {
  return nice_tick_step_2d(lo, hi, approx_tick_count);
}

inline std::vector<double> pdf_make_ticks(double lo, double hi, int approx_tick_count = 5) {
  return make_ticks_2d(lo, hi, approx_tick_count);
}

inline std::string pdf_format_tick_label(double transformed_value, const Axis& axis) {
  return format_tick_label_2d(transformed_value, axis);
}

inline std::string pdf_normalized_marker(std::string_view marker, bool scatter_like) {
  return canonical_marker_name(marker, scatter_like);
}

inline std::vector<double> pdf_dash_pattern(std::string_view line_style, double line_width) {
  const double base = std::max(1.0, line_width);
  if (line_style == "dashed" || line_style == "dash" || line_style == "--") {
    return {base * 5.0, base * 3.0};
  }
  if (line_style == "dotted" || line_style == "dot" || line_style == ":") {
    return {base, base * 2.2};
  }
  if (line_style == "dashdot" || line_style == "-.") {
    return {base * 5.0, base * 2.2, base, base * 2.2};
  }
  return {};
}

inline double pdf_legend_width_hint(std::size_t entry_count, std::size_t longest_label, bool has_title) {
  return legend_width_hint_2d(entry_count, longest_label, has_title);
}

inline double pdf_legend_height_hint(std::size_t entry_count, bool has_title) {
  return legend_height_hint_2d(entry_count, has_title);
}

class PdfPainter {
public:
  PdfPainter(std::int32_t width, std::int32_t height) : width_(width), height_(height) {
    if (width_ <= 100 || height_ <= 100) {
      throw std::invalid_argument("plotism::pdf backend: width/height too small");
    }
    stream_ << "1 J 1 j\n";
  }

  void fill_rect(double x, double y, double w, double h, const PdfRgb& color) {
    if (!(w > 0.0 && h > 0.0)) {
      return;
    }
    set_fill_color(color);
    const double py = y_to_pdf(y) - h;
    stream_ << f(x) << " " << f(py) << " " << f(w) << " " << f(h) << " re f\n";
  }

  void stroke_rect(double x, double y, double w, double h, const PdfRgb& color, double line_width = 1.0) {
    if (!(w > 0.0 && h > 0.0)) {
      return;
    }
    set_stroke_color(color);
    reset_dash_();
    stream_ << f(line_width) << " w\n";
    const double py = y_to_pdf(y) - h;
    stream_ << f(x) << " " << f(py) << " " << f(w) << " " << f(h) << " re S\n";
  }

  void stroke_line(double x1,
                   double y1,
                   double x2,
                   double y2,
                   const PdfRgb& color,
                   double line_width = 1.0,
                   const std::vector<double>& dash_pattern = {}) {
    set_stroke_color(color);
    set_dash_(dash_pattern);
    stream_ << f(line_width) << " w\n";
    stream_ << f(x1) << " " << f(y_to_pdf(y1)) << " m " << f(x2) << " " << f(y_to_pdf(y2)) << " l S\n";
  }

  void stroke_polyline(const std::vector<std::pair<double, double>>& points,
                       const PdfRgb& color,
                       double line_width = 1.0,
                       const std::vector<double>& dash_pattern = {}) {
    if (points.size() < 2) {
      return;
    }
    set_stroke_color(color);
    set_dash_(dash_pattern);
    stream_ << f(line_width) << " w\n";
    stream_ << f(points[0].first) << " " << f(y_to_pdf(points[0].second)) << " m ";
    for (std::size_t i = 1; i < points.size(); ++i) {
      stream_ << f(points[i].first) << " " << f(y_to_pdf(points[i].second)) << " l ";
    }
    stream_ << "S\n";
  }

  void fill_polygon(const std::vector<std::pair<double, double>>& points, const PdfRgb& color) {
    if (points.size() < 3) {
      return;
    }
    set_fill_color(color);
    stream_ << f(points[0].first) << " " << f(y_to_pdf(points[0].second)) << " m ";
    for (std::size_t i = 1; i < points.size(); ++i) {
      stream_ << f(points[i].first) << " " << f(y_to_pdf(points[i].second)) << " l ";
    }
    stream_ << "h f\n";
  }

  void fill_circle(double cx, double cy, double r, const PdfRgb& color) {
    if (!(r > 0.0) || !std::isfinite(cx) || !std::isfinite(cy)) {
      return;
    }
    set_fill_color(color);

    constexpr double k = 0.5522847498307936;
    const double cp = k * r;
    const double y = y_to_pdf(cy);

    stream_ << f(cx + r) << " " << f(y) << " m "
            << f(cx + r) << " " << f(y + cp) << " "
            << f(cx + cp) << " " << f(y + r) << " "
            << f(cx) << " " << f(y + r) << " c "
            << f(cx - cp) << " " << f(y + r) << " "
            << f(cx - r) << " " << f(y + cp) << " "
            << f(cx - r) << " " << f(y) << " c "
            << f(cx - r) << " " << f(y - cp) << " "
            << f(cx - cp) << " " << f(y - r) << " "
            << f(cx) << " " << f(y - r) << " c "
            << f(cx + cp) << " " << f(y - r) << " "
            << f(cx + r) << " " << f(y - cp) << " "
            << f(cx + r) << " " << f(y) << " c f\n";
  }

  void text_left(double x, double y, double size, std::string_view text, const PdfRgb& color) {
    if (text.empty()) {
      return;
    }
    set_fill_color(color);
    stream_ << "BT /F1 " << f(size) << " Tf 1 0 0 1 " << f(x) << " " << f(y_to_pdf(y))
            << " Tm (" << pdf_escape_text(text) << ") Tj ET\n";
  }

  void text_center(double x, double y, double size, std::string_view text, const PdfRgb& color) {
    text_left(x - 0.5 * text_width_estimate(size, text), y, size, text, color);
  }

  void text_right(double x, double y, double size, std::string_view text, const PdfRgb& color) {
    text_left(x - text_width_estimate(size, text), y, size, text, color);
  }

  void text_vertical_ccw(double x, double y, double size, std::string_view text, const PdfRgb& color) {
    if (text.empty()) {
      return;
    }
    set_fill_color(color);
    stream_ << "BT /F1 " << f(size) << " Tf 0 1 -1 0 " << f(x) << " " << f(y_to_pdf(y))
            << " Tm (" << pdf_escape_text(text) << ") Tj ET\n";
  }

  void text_rotated(double x,
                    double y,
                    double size,
                    std::string_view text,
                    const PdfRgb& color,
                    TextAnchor2D anchor,
                    double rotation_deg) {
    if (text.empty()) {
      return;
    }
    const double width = text_width_estimate(size, text);
    double offset = 0.0;
    if (anchor == TextAnchor2D::Center) {
      offset = -0.5 * width;
    } else if (anchor == TextAnchor2D::Right) {
      offset = -width;
    }
    const double radians = rotation_deg * 3.14159265358979323846 / 180.0;
    const double c = std::cos(radians);
    const double s = std::sin(radians);
    set_fill_color(color);
    stream_ << "BT /F1 " << f(size) << " Tf "
            << f(c) << " " << f(s) << " " << f(-s) << " " << f(c) << " "
            << f(x + offset * c) << " " << f(y_to_pdf(y) + offset * s)
            << " Tm (" << pdf_escape_text(text) << ") Tj ET\n";
  }

  void push_clip_rect(double x, double y, double w, double h) {
    if (!(w > 0.0 && h > 0.0)) {
      return;
    }
    const double py = y_to_pdf(y) - h;
    stream_ << "q\n" << f(x) << " " << f(py) << " " << f(w) << " " << f(h) << " re W n\n";
  }

  void pop_clip() {
    stream_ << "Q\n";
  }

  std::string finish() const { return stream_.str(); }

private:
  static double text_width_estimate(double size, std::string_view text) {
    return size * 0.55 * static_cast<double>(text.size());
  }

  static std::string f(double value) {
    return detail::format_double(value, 6);
  }

  double y_to_pdf(double y) const {
    return static_cast<double>(height_) - y;
  }

  void set_fill_color(const PdfRgb& color) {
    stream_ << f(color.r) << " " << f(color.g) << " " << f(color.b) << " rg\n";
  }

  void set_stroke_color(const PdfRgb& color) {
    stream_ << f(color.r) << " " << f(color.g) << " " << f(color.b) << " RG\n";
  }

  void reset_dash_() {
    stream_ << "[] 0 d\n";
  }

  void set_dash_(const std::vector<double>& pattern) {
    if (pattern.empty()) {
      reset_dash_();
      return;
    }
    stream_ << "[";
    for (std::size_t i = 0; i < pattern.size(); ++i) {
      if (i > 0) {
        stream_ << " ";
      }
      stream_ << f(pattern[i]);
    }
    stream_ << "] 0 d\n";
  }

  std::int32_t width_{0};
  std::int32_t height_{0};
  std::ostringstream stream_{};
};

class PdfRenderSurface2D final : public RenderSurface2D {
public:
  PdfRenderSurface2D(std::int32_t width, std::int32_t height) : painter_(width, height) {}

  void push_clip_rect(double x, double y, double w, double h) override {
    painter_.push_clip_rect(x, y, w, h);
  }

  void pop_clip() override {
    painter_.pop_clip();
  }

  void fill_rect(double x, double y, double w, double h, const FillStyle2D& style, double corner_radius = 0.0) override {
    (void)corner_radius;
    painter_.fill_rect(x, y, w, h, pdf_parse_color(style.color));
  }

  void stroke_rect(double x,
                   double y,
                   double w,
                   double h,
                   const StrokeStyle2D& style,
                   double corner_radius = 0.0) override {
    (void)corner_radius;
    painter_.stroke_rect(x, y, w, h, pdf_parse_color(style.color), style.line_width);
  }

  void stroke_line(double x1, double y1, double x2, double y2, const StrokeStyle2D& style) override {
    painter_.stroke_line(x1, y1, x2, y2, pdf_parse_color(style.color), style.line_width,
                         pdf_dash_pattern(style.line_style, style.line_width));
  }

  void stroke_polyline(const std::vector<Point2D>& points, const StrokeStyle2D& style) override {
    std::vector<std::pair<double, double>> pdf_points;
    pdf_points.reserve(points.size());
    for (const auto& point : points) {
      pdf_points.push_back({point.x, point.y});
    }
    painter_.stroke_polyline(pdf_points, pdf_parse_color(style.color), style.line_width,
                             pdf_dash_pattern(style.line_style, style.line_width));
  }

  void fill_circle(double cx, double cy, double r, const FillStyle2D& style) override {
    painter_.fill_circle(cx, cy, r, pdf_parse_color(style.color));
  }

  void fill_polygon(const std::vector<Point2D>& points, const FillStyle2D& style) override {
    std::vector<std::pair<double, double>> pdf_points;
    pdf_points.reserve(points.size());
    for (const auto& point : points) {
      pdf_points.push_back({point.x, point.y});
    }
    painter_.fill_polygon(pdf_points, pdf_parse_color(style.color));
  }

  void text(double x, double y, std::string_view text, const TextStyle2D& style) override {
    const PdfRgb color = pdf_parse_color(style.color);
    const double rotation = style.orientation == TextOrientation2D::VerticalCcw ? (style.rotation_deg - 90.0) : style.rotation_deg;
    if (std::abs(rotation) > 1e-9) {
      painter_.text_rotated(x, y, style.size, text, color, style.anchor, rotation);
      return;
    }
    switch (style.anchor) {
      case TextAnchor2D::Center:
        painter_.text_center(x, y, style.size, text, color);
        return;
      case TextAnchor2D::Right:
        painter_.text_right(x, y, style.size, text, color);
        return;
      case TextAnchor2D::Left:
      default:
        painter_.text_left(x, y, style.size, text, color);
        return;
    }
  }

  std::string finish() const override { return painter_.finish(); }

private:
  PdfPainter painter_;
};

inline void draw_pdf_marker(PdfPainter& painter,
                            double x,
                            double y,
                            std::string_view marker,
                            double size,
                            const PdfRgb& color) {
  const std::string kind = pdf_normalized_marker(marker, true);
  if (kind == "none") {
    return;
  }
  const double half = std::max(2.0, size);
  if (kind == "circle") {
    painter.fill_circle(x, y, half, color);
    return;
  }
  if (kind == "square") {
    painter.fill_rect(x - half, y - half, half * 2.0, half * 2.0, color);
    return;
  }
  if (kind == "triangle") {
    painter.fill_polygon({{x, y - half}, {x + half, y + half}, {x - half, y + half}}, color);
    return;
  }
  if (kind == "diamond") {
    painter.fill_polygon({{x, y - half}, {x + half, y}, {x, y + half}, {x - half, y}}, color);
    return;
  }
  if (kind == "x") {
    painter.stroke_line(x - half, y - half, x + half, y + half, color, 1.4);
    painter.stroke_line(x - half, y + half, x + half, y - half, color, 1.4);
    return;
  }
  if (kind == "plus") {
    painter.stroke_line(x - half, y, x + half, y, color, 1.4);
    painter.stroke_line(x, y - half, x, y + half, color, 1.4);
  }
}

inline std::string build_pdf_document(std::int32_t width,
                                      std::int32_t height,
                                      const std::string& content_stream,
                                      double dpi = 144.0) {
  std::ostringstream out;
  out << "%PDF-1.4\n";

  std::array<long long, 6> offsets{};
  auto write_object = [&](int id, const std::string& body) {
    offsets[static_cast<std::size_t>(id)] = static_cast<long long>(out.tellp());
    out << id << " 0 obj\n" << body << "\nendobj\n";
  };

  write_object(1, "<< /Type /Catalog /Pages 2 0 R >>");
  write_object(2, "<< /Type /Pages /Count 1 /Kids [3 0 R] >>");

  const double page_width_points = static_cast<double>(width) * 72.0 / std::max(1.0, dpi);
  const double page_height_points = static_cast<double>(height) * 72.0 / std::max(1.0, dpi);
  std::ostringstream page;
  page << "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 " << detail::format_double(page_width_points, 6)
       << " " << detail::format_double(page_height_points, 6) << "] "
       << "/Resources << /Font << /F1 5 0 R >> >> /Contents 4 0 R >>";
  write_object(3, page.str());

  const double pdf_scale = 72.0 / std::max(1.0, dpi);
  std::ostringstream scaled_stream;
  scaled_stream << "q " << detail::format_double(pdf_scale, 6) << " 0 0 " << detail::format_double(pdf_scale, 6)
                << " 0 0 cm\n"
                << content_stream
                << "Q\n";
  const std::string scaled_content = scaled_stream.str();

  std::ostringstream content;
  content << "<< /Length " << scaled_content.size() << " >>\nstream\n" << scaled_content << "endstream";
  write_object(4, content.str());

  write_object(5, "<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>");

  const long long xref_offset = static_cast<long long>(out.tellp());
  out << "xref\n0 6\n";
  out << "0000000000 65535 f \n";
  for (int i = 1; i <= 5; ++i) {
    out << std::setw(10) << std::setfill('0') << offsets[static_cast<std::size_t>(i)] << " 00000 n \n";
  }

  out << "trailer\n<< /Size 6 /Root 1 0 R >>\n";
  out << "startxref\n" << xref_offset << "\n%%EOF\n";
  return out.str();
}

} // namespace detail
} // namespace pluss::plotism
