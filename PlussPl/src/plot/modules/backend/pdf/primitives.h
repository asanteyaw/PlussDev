#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "plot/modules/backend.h"
#include "plot/modules/core.h"

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
      const double r = static_cast<double>(r1 * 16 + r2) / 255.0;
      const double g = static_cast<double>(g1 * 16 + g2) / 255.0;
      const double b = static_cast<double>(b1 * 16 + b2) / 255.0;
      return PdfRgb{r, g, b};
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

inline std::pair<double, double> pdf_ensure_limits(double lo, double hi) {
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

inline std::pair<double, double> pdf_map_legend_anchor(LegendLocation location,
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
    stream_ << f(line_width) << " w\n";
    const double py = y_to_pdf(y) - h;
    stream_ << f(x) << " " << f(py) << " " << f(w) << " " << f(h) << " re S\n";
  }

  void stroke_line(double x1, double y1, double x2, double y2, const PdfRgb& color, double line_width = 1.0) {
    set_stroke_color(color);
    stream_ << f(line_width) << " w\n";
    stream_ << f(x1) << " " << f(y_to_pdf(y1)) << " m " << f(x2) << " " << f(y_to_pdf(y2)) << " l S\n";
  }

  void stroke_polyline(const std::vector<std::pair<double, double>>& points,
                       const PdfRgb& color,
                       double line_width = 1.0) {
    if (points.size() < 2) {
      return;
    }
    set_stroke_color(color);
    stream_ << f(line_width) << " w\n";
    stream_ << f(points[0].first) << " " << f(y_to_pdf(points[0].second)) << " m ";
    for (std::size_t i = 1; i < points.size(); ++i) {
      stream_ << f(points[i].first) << " " << f(y_to_pdf(points[i].second)) << " l ";
    }
    stream_ << "S\n";
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
    const double w = text_width_estimate(size, text);
    text_left(x - 0.5 * w, y, size, text, color);
  }

  void text_vertical_ccw(double x, double y, double size, std::string_view text, const PdfRgb& color) {
    if (text.empty()) {
      return;
    }
    set_fill_color(color);
    stream_ << "BT /F1 " << f(size) << " Tf 0 1 -1 0 " << f(x) << " " << f(y_to_pdf(y))
            << " Tm (" << pdf_escape_text(text) << ") Tj ET\n";
  }

  std::string finish() const { return stream_.str(); }

private:
  static double text_width_estimate(double size, std::string_view text) {
    return size * 0.55 * static_cast<double>(text.size());
  }

  static std::string f(double v) { return detail::format_double(v, 6); }

  double y_to_pdf(double y) const {
    return static_cast<double>(height_) - y;
  }

  void set_fill_color(const PdfRgb& color) {
    stream_ << f(color.r) << " " << f(color.g) << " " << f(color.b) << " rg\n";
  }

  void set_stroke_color(const PdfRgb& color) {
    stream_ << f(color.r) << " " << f(color.g) << " " << f(color.b) << " RG\n";
  }

  std::int32_t width_{0};
  std::int32_t height_{0};
  std::ostringstream stream_{};
};

inline std::string build_pdf_document(std::int32_t width, std::int32_t height, const std::string& content_stream) {
  std::ostringstream out;
  out << "%PDF-1.4\n";

  std::array<long long, 6> offsets{};
  auto write_object = [&](int id, const std::string& body) {
    offsets[static_cast<std::size_t>(id)] = static_cast<long long>(out.tellp());
    out << id << " 0 obj\n" << body << "\nendobj\n";
  };

  write_object(1, "<< /Type /Catalog /Pages 2 0 R >>");
  write_object(2, "<< /Type /Pages /Count 1 /Kids [3 0 R] >>");

  std::ostringstream page;
  page << "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 " << width << " " << height << "] "
       << "/Resources << /Font << /F1 5 0 R >> >> /Contents 4 0 R >>";
  write_object(3, page.str());

  std::ostringstream content;
  content << "<< /Length " << content_stream.size() << " >>\nstream\n" << content_stream << "endstream";
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
