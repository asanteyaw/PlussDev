#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numeric>
#include <string>
#include <string_view>
#include <vector>

#include "plot/modules/backend/render_surface_2d.h"

namespace pluss::plotism {
namespace detail {

inline int recommended_png_supersample(std::int32_t width, std::int32_t height) {
  const std::int64_t pixels = static_cast<std::int64_t>(width) * static_cast<std::int64_t>(height);
  if (pixels <= 1'600'000) {
    return 4;
  }
  if (pixels <= 3'500'000) {
    return 3;
  }
  return 2;
}

struct RasterColor {
  std::uint8_t r{0};
  std::uint8_t g{0};
  std::uint8_t b{0};
  std::uint8_t a{255};
};

inline std::uint8_t clamp_u8(double value) {
  return static_cast<std::uint8_t>(std::clamp(std::lround(value), 0L, 255L));
}

inline RasterColor raster_parse_color(std::string_view color, double alpha = 1.0) {
  const auto hex_nibble = [](char c) -> int {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
  };

  if (color == "white") return RasterColor{255, 255, 255, clamp_u8(alpha * 255.0)};
  if (color == "black") return RasterColor{0, 0, 0, clamp_u8(alpha * 255.0)};
  if (color == "red") return RasterColor{220, 38, 38, clamp_u8(alpha * 255.0)};
  if (color == "green") return RasterColor{22, 163, 74, clamp_u8(alpha * 255.0)};
  if (color == "blue") return RasterColor{37, 99, 235, clamp_u8(alpha * 255.0)};
  if (color == "orange") return RasterColor{249, 115, 22, clamp_u8(alpha * 255.0)};
  if (color == "gray" || color == "grey") return RasterColor{107, 114, 128, clamp_u8(alpha * 255.0)};

  if (color.size() == 7 && color.front() == '#') {
    const int r1 = hex_nibble(color[1]);
    const int r2 = hex_nibble(color[2]);
    const int g1 = hex_nibble(color[3]);
    const int g2 = hex_nibble(color[4]);
    const int b1 = hex_nibble(color[5]);
    const int b2 = hex_nibble(color[6]);
    if (r1 >= 0 && r2 >= 0 && g1 >= 0 && g2 >= 0 && b1 >= 0 && b2 >= 0) {
      return RasterColor{
          static_cast<std::uint8_t>(r1 * 16 + r2),
          static_cast<std::uint8_t>(g1 * 16 + g2),
          static_cast<std::uint8_t>(b1 * 16 + b2),
          clamp_u8(alpha * 255.0),
      };
    }
  }
  if (color.size() == 4 && color.front() == '#') {
    const int r = hex_nibble(color[1]);
    const int g = hex_nibble(color[2]);
    const int b = hex_nibble(color[3]);
    if (r >= 0 && g >= 0 && b >= 0) {
      return RasterColor{
          static_cast<std::uint8_t>(r * 17),
          static_cast<std::uint8_t>(g * 17),
          static_cast<std::uint8_t>(b * 17),
          clamp_u8(alpha * 255.0),
      };
    }
  }
  return RasterColor{31, 41, 55, clamp_u8(alpha * 255.0)};
}

inline std::uint32_t png_crc32(const std::uint8_t* data, std::size_t size) {
  std::uint32_t crc = 0xffffffffu;
  for (std::size_t i = 0; i < size; ++i) {
    crc ^= static_cast<std::uint32_t>(data[i]);
    for (int bit = 0; bit < 8; ++bit) {
      const std::uint32_t mask = static_cast<std::uint32_t>(-(static_cast<std::int32_t>(crc & 1u)));
      crc = (crc >> 1u) ^ (0xedb88320u & mask);
    }
  }
  return ~crc;
}

inline std::uint32_t png_adler32(const std::vector<std::uint8_t>& data) {
  constexpr std::uint32_t mod = 65521u;
  std::uint32_t a = 1u;
  std::uint32_t b = 0u;
  for (std::uint8_t value : data) {
    a = (a + value) % mod;
    b = (b + a) % mod;
  }
  return (b << 16u) | a;
}

inline void append_png_u32(std::string& out, std::uint32_t value) {
  out.push_back(static_cast<char>((value >> 24u) & 0xffu));
  out.push_back(static_cast<char>((value >> 16u) & 0xffu));
  out.push_back(static_cast<char>((value >> 8u) & 0xffu));
  out.push_back(static_cast<char>(value & 0xffu));
}

inline void append_png_u32(std::vector<std::uint8_t>& out, std::uint32_t value) {
  out.push_back(static_cast<std::uint8_t>((value >> 24u) & 0xffu));
  out.push_back(static_cast<std::uint8_t>((value >> 16u) & 0xffu));
  out.push_back(static_cast<std::uint8_t>((value >> 8u) & 0xffu));
  out.push_back(static_cast<std::uint8_t>(value & 0xffu));
}

inline void append_png_chunk(std::string& out, const char type[4], const std::vector<std::uint8_t>& data) {
  append_png_u32(out, static_cast<std::uint32_t>(data.size()));
  const std::size_t chunk_start = out.size();
  out.append(type, 4);
  out.append(reinterpret_cast<const char*>(data.data()), static_cast<std::ptrdiff_t>(data.size()));
  const std::uint32_t crc =
      png_crc32(reinterpret_cast<const std::uint8_t*>(out.data() + static_cast<std::ptrdiff_t>(chunk_start)),
                4 + data.size());
  append_png_u32(out, crc);
}

inline std::string encode_png_rgba(std::int32_t width, std::int32_t height, const std::vector<std::uint8_t>& rgba) {
  if (width <= 0 || height <= 0) {
    throw std::invalid_argument("plotism::encode_png_rgba: width/height must be positive");
  }
  const std::size_t expected = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u;
  if (rgba.size() != expected) {
    throw std::invalid_argument("plotism::encode_png_rgba: rgba buffer size mismatch");
  }

  std::vector<std::uint8_t> scanlines;
  scanlines.reserve(static_cast<std::size_t>(height) * (static_cast<std::size_t>(width) * 4u + 1u));
  for (std::int32_t y = 0; y < height; ++y) {
    scanlines.push_back(0u);
    const std::size_t row = static_cast<std::size_t>(y) * static_cast<std::size_t>(width) * 4u;
    scanlines.insert(scanlines.end(), rgba.begin() + static_cast<std::ptrdiff_t>(row),
                     rgba.begin() + static_cast<std::ptrdiff_t>(row + static_cast<std::size_t>(width) * 4u));
  }

  std::vector<std::uint8_t> zlib;
  zlib.reserve(scanlines.size() + scanlines.size() / 65535u * 5u + 16u);
  zlib.push_back(0x78u);
  zlib.push_back(0x01u);

  std::size_t offset = 0;
  while (offset < scanlines.size()) {
    const std::size_t remaining = scanlines.size() - offset;
    const std::uint16_t block_len = static_cast<std::uint16_t>(std::min<std::size_t>(65535u, remaining));
    const bool final_block = (offset + block_len) == scanlines.size();
    zlib.push_back(final_block ? 0x01u : 0x00u);
    zlib.push_back(static_cast<std::uint8_t>(block_len & 0xffu));
    zlib.push_back(static_cast<std::uint8_t>((block_len >> 8u) & 0xffu));
    const std::uint16_t nlen = static_cast<std::uint16_t>(~block_len);
    zlib.push_back(static_cast<std::uint8_t>(nlen & 0xffu));
    zlib.push_back(static_cast<std::uint8_t>((nlen >> 8u) & 0xffu));
    zlib.insert(zlib.end(),
                scanlines.begin() + static_cast<std::ptrdiff_t>(offset),
                scanlines.begin() + static_cast<std::ptrdiff_t>(offset + block_len));
    offset += block_len;
  }

  append_png_u32(zlib, png_adler32(scanlines));

  std::string png;
  png.reserve(zlib.size() + scanlines.size() / 4u + 256u);
  png.append("\x89PNG\r\n\x1a\n", 8);

  std::vector<std::uint8_t> ihdr;
  ihdr.reserve(13u);
  ihdr.push_back(static_cast<std::uint8_t>((width >> 24) & 0xff));
  ihdr.push_back(static_cast<std::uint8_t>((width >> 16) & 0xff));
  ihdr.push_back(static_cast<std::uint8_t>((width >> 8) & 0xff));
  ihdr.push_back(static_cast<std::uint8_t>(width & 0xff));
  ihdr.push_back(static_cast<std::uint8_t>((height >> 24) & 0xff));
  ihdr.push_back(static_cast<std::uint8_t>((height >> 16) & 0xff));
  ihdr.push_back(static_cast<std::uint8_t>((height >> 8) & 0xff));
  ihdr.push_back(static_cast<std::uint8_t>(height & 0xff));
  ihdr.push_back(8u);
  ihdr.push_back(6u);
  ihdr.push_back(0u);
  ihdr.push_back(0u);
  ihdr.push_back(0u);
  append_png_chunk(png, "IHDR", ihdr);
  append_png_chunk(png, "IDAT", zlib);
  append_png_chunk(png, "IEND", {});
  return png;
}

inline double distance_to_segment(double px, double py, double x1, double y1, double x2, double y2) {
  const double dx = x2 - x1;
  const double dy = y2 - y1;
  const double denom = dx * dx + dy * dy;
  if (!(denom > 0.0)) {
    return std::hypot(px - x1, py - y1);
  }
  const double t = std::clamp(((px - x1) * dx + (py - y1) * dy) / denom, 0.0, 1.0);
  const double qx = x1 + t * dx;
  const double qy = y1 + t * dy;
  return std::hypot(px - qx, py - qy);
}

inline bool point_in_polygon(double px, double py, const std::vector<Point2D>& points) {
  bool inside = false;
  const std::size_t n = points.size();
  for (std::size_t i = 0, j = n - 1; i < n; j = i++) {
    const auto& a = points[i];
    const auto& b = points[j];
    const bool intersects = ((a.y > py) != (b.y > py)) &&
                            (px < (b.x - a.x) * (py - a.y) / ((b.y - a.y) + 1e-12) + a.x);
    if (intersects) {
      inside = !inside;
    }
  }
  return inside;
}

inline std::array<std::uint8_t, 7> raster_glyph_rows(char c) {
  switch (c) {
    case '0': return {0x0e, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0e};
    case '1': return {0x04, 0x0c, 0x04, 0x04, 0x04, 0x04, 0x0e};
    case '2': return {0x0e, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1f};
    case '3': return {0x1e, 0x01, 0x01, 0x0e, 0x01, 0x01, 0x1e};
    case '4': return {0x02, 0x06, 0x0a, 0x12, 0x1f, 0x02, 0x02};
    case '5': return {0x1f, 0x10, 0x10, 0x1e, 0x01, 0x01, 0x1e};
    case '6': return {0x06, 0x08, 0x10, 0x1e, 0x11, 0x11, 0x0e};
    case '7': return {0x1f, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08};
    case '8': return {0x0e, 0x11, 0x11, 0x0e, 0x11, 0x11, 0x0e};
    case '9': return {0x0e, 0x11, 0x11, 0x0f, 0x01, 0x02, 0x0c};
    case 'A': return {0x0e, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11};
    case 'B': return {0x1e, 0x11, 0x11, 0x1e, 0x11, 0x11, 0x1e};
    case 'C': return {0x0e, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0e};
    case 'D': return {0x1c, 0x12, 0x11, 0x11, 0x11, 0x12, 0x1c};
    case 'E': return {0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x1f};
    case 'F': return {0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x10};
    case 'G': return {0x0e, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0f};
    case 'H': return {0x11, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11};
    case 'I': return {0x0e, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0e};
    case 'J': return {0x01, 0x01, 0x01, 0x01, 0x11, 0x11, 0x0e};
    case 'K': return {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11};
    case 'L': return {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1f};
    case 'M': return {0x11, 0x1b, 0x15, 0x15, 0x11, 0x11, 0x11};
    case 'N': return {0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11};
    case 'O': return {0x0e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e};
    case 'P': return {0x1e, 0x11, 0x11, 0x1e, 0x10, 0x10, 0x10};
    case 'Q': return {0x0e, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0d};
    case 'R': return {0x1e, 0x11, 0x11, 0x1e, 0x14, 0x12, 0x11};
    case 'S': return {0x0f, 0x10, 0x10, 0x0e, 0x01, 0x01, 0x1e};
    case 'T': return {0x1f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};
    case 'U': return {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e};
    case 'V': return {0x11, 0x11, 0x11, 0x11, 0x11, 0x0a, 0x04};
    case 'W': return {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0a};
    case 'X': return {0x11, 0x11, 0x0a, 0x04, 0x0a, 0x11, 0x11};
    case 'Y': return {0x11, 0x11, 0x0a, 0x04, 0x04, 0x04, 0x04};
    case 'Z': return {0x1f, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1f};
    case '-': return {0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00};
    case '+': return {0x00, 0x04, 0x04, 0x1f, 0x04, 0x04, 0x00};
    case '.': return {0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x0c};
    case ',': return {0x00, 0x00, 0x00, 0x00, 0x0c, 0x0c, 0x08};
    case ':': return {0x00, 0x0c, 0x0c, 0x00, 0x0c, 0x0c, 0x00};
    case '/': return {0x01, 0x02, 0x02, 0x04, 0x08, 0x08, 0x10};
    case '(': return {0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02};
    case ')': return {0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08};
    case '[': return {0x0e, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0e};
    case ']': return {0x0e, 0x02, 0x02, 0x02, 0x02, 0x02, 0x0e};
    case '_': return {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f};
    case '=': return {0x00, 0x00, 0x1f, 0x00, 0x1f, 0x00, 0x00};
    case ' ': return {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    default: return {0x1f, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04};
  }
}

class RasterSurface2D final : public RenderSurface2D {
public:
  RasterSurface2D(std::int32_t width, std::int32_t height, int supersample = 2)
      : width_(width),
        height_(height),
        ss_(std::max(1, supersample)),
        raster_width_(width_ * ss_),
        raster_height_(height_ * ss_),
        rgba_(static_cast<std::size_t>(raster_width_) * static_cast<std::size_t>(raster_height_) * 4u, 255u) {}

  void push_clip_rect(double x, double y, double w, double h) override {
    if (!(w > 0.0 && h > 0.0)) {
      return;
    }
    const ClipRect clip{
        .x1 = x * static_cast<double>(ss_),
        .y1 = y * static_cast<double>(ss_),
        .x2 = (x + w) * static_cast<double>(ss_),
        .y2 = (y + h) * static_cast<double>(ss_),
    };
    if (clip_stack_.empty()) {
      clip_stack_.push_back(clip);
      return;
    }
    const auto& current = clip_stack_.back();
    clip_stack_.push_back(ClipRect{
        .x1 = std::max(current.x1, clip.x1),
        .y1 = std::max(current.y1, clip.y1),
        .x2 = std::min(current.x2, clip.x2),
        .y2 = std::min(current.y2, clip.y2),
    });
  }

  void pop_clip() override {
    if (!clip_stack_.empty()) {
      clip_stack_.pop_back();
    }
  }

  void fill_rect(double x, double y, double w, double h, const FillStyle2D& style, double corner_radius = 0.0) override {
    if (!(w > 0.0) || !(h > 0.0)) {
      return;
    }
    const RasterColor color = raster_parse_color(style.color, style.alpha);
    const double x1 = x;
    const double y1 = y;
    const double x2 = x + w;
    const double y2 = y + h;
    const double radius = std::max(0.0, std::min(corner_radius, 0.5 * std::min(w, h)));
    const int min_x = std::max(0, static_cast<int>(std::floor(x1 * ss_)) - 1);
    const int max_x = std::min(raster_width_ - 1, static_cast<int>(std::ceil(x2 * ss_)) + 1);
    const int min_y = std::max(0, static_cast<int>(std::floor(y1 * ss_)) - 1);
    const int max_y = std::min(raster_height_ - 1, static_cast<int>(std::ceil(y2 * ss_)) + 1);
    for (int py = min_y; py <= max_y; ++py) {
      const double yf = (static_cast<double>(py) + 0.5) / static_cast<double>(ss_);
      for (int px = min_x; px <= max_x; ++px) {
        const double xf = (static_cast<double>(px) + 0.5) / static_cast<double>(ss_);
        if (radius <= 0.0) {
          if (xf >= x1 && xf <= x2 && yf >= y1 && yf <= y2) {
            blend_pixel(px, py, color);
          }
          continue;
        }
        const double clamped_x = std::clamp(xf, x1 + radius, x2 - radius);
        const double clamped_y = std::clamp(yf, y1 + radius, y2 - radius);
        const double dx = xf - clamped_x;
        const double dy = yf - clamped_y;
        if (dx * dx + dy * dy <= radius * radius) {
          blend_pixel(px, py, color);
        }
      }
    }
  }

  void stroke_rect(double x,
                   double y,
                   double w,
                   double h,
                   const StrokeStyle2D& style,
                   double corner_radius = 0.0) override {
    if (!(w > 0.0) || !(h > 0.0)) {
      return;
    }
    const double line_width = std::max(1.0, style.line_width);
    if (corner_radius <= 0.0) {
      fill_rect(x, y, w, line_width, FillStyle2D{.color = style.color, .alpha = style.alpha});
      fill_rect(x, y + h - line_width, w, line_width, FillStyle2D{.color = style.color, .alpha = style.alpha});
      fill_rect(x, y, line_width, h, FillStyle2D{.color = style.color, .alpha = style.alpha});
      fill_rect(x + w - line_width, y, line_width, h, FillStyle2D{.color = style.color, .alpha = style.alpha});
      return;
    }
    stroke_line(x + corner_radius, y, x + w - corner_radius, y, style);
    stroke_line(x + corner_radius, y + h, x + w - corner_radius, y + h, style);
    stroke_line(x, y + corner_radius, x, y + h - corner_radius, style);
    stroke_line(x + w, y + corner_radius, x + w, y + h - corner_radius, style);
  }

  void stroke_line(double x1, double y1, double x2, double y2, const StrokeStyle2D& style) override {
    const RasterColor color = raster_parse_color(style.color, style.alpha);
    const double width = std::max(1.0, style.line_width);
    const double pad = width + 2.0;
    const int min_x = std::max(0, static_cast<int>(std::floor((std::min(x1, x2) - pad) * ss_)));
    const int max_x = std::min(raster_width_ - 1, static_cast<int>(std::ceil((std::max(x1, x2) + pad) * ss_)));
    const int min_y = std::max(0, static_cast<int>(std::floor((std::min(y1, y2) - pad) * ss_)));
    const int max_y = std::min(raster_height_ - 1, static_cast<int>(std::ceil((std::max(y1, y2) + pad) * ss_)));
    const auto dashed = dash_pattern(style.line_style, width);
    for (int py = min_y; py <= max_y; ++py) {
      const double yf = (static_cast<double>(py) + 0.5) / static_cast<double>(ss_);
      for (int px = min_x; px <= max_x; ++px) {
        const double xf = (static_cast<double>(px) + 0.5) / static_cast<double>(ss_);
        const double dist = distance_to_segment(xf, yf, x1, y1, x2, y2);
        if (dist > width * 0.5) {
          continue;
        }
        const double dx = x2 - x1;
        const double dy = y2 - y1;
        const double denom = dx * dx + dy * dy;
        double along = 0.0;
        if (denom > 0.0) {
          along = std::clamp(((xf - x1) * dx + (yf - y1) * dy) / std::sqrt(denom), 0.0, std::sqrt(denom));
        }
        if (!dashed.empty() && !dash_visible(along, dashed)) {
          continue;
        }
        blend_pixel(px, py, color);
      }
    }
  }

  void stroke_polyline(const std::vector<Point2D>& points, const StrokeStyle2D& style) override {
    if (points.size() < 2) {
      return;
    }
    for (std::size_t i = 1; i < points.size(); ++i) {
      stroke_line(points[i - 1].x, points[i - 1].y, points[i].x, points[i].y, style);
    }
  }

  void fill_circle(double cx, double cy, double r, const FillStyle2D& style) override {
    if (!(r > 0.0)) {
      return;
    }
    const RasterColor color = raster_parse_color(style.color, style.alpha);
    const int min_x = std::max(0, static_cast<int>(std::floor((cx - r) * ss_)) - 1);
    const int max_x = std::min(raster_width_ - 1, static_cast<int>(std::ceil((cx + r) * ss_)) + 1);
    const int min_y = std::max(0, static_cast<int>(std::floor((cy - r) * ss_)) - 1);
    const int max_y = std::min(raster_height_ - 1, static_cast<int>(std::ceil((cy + r) * ss_)) + 1);
    const double rr = r * r;
    for (int py = min_y; py <= max_y; ++py) {
      const double yf = (static_cast<double>(py) + 0.5) / static_cast<double>(ss_);
      for (int px = min_x; px <= max_x; ++px) {
        const double xf = (static_cast<double>(px) + 0.5) / static_cast<double>(ss_);
        const double dx = xf - cx;
        const double dy = yf - cy;
        if (dx * dx + dy * dy <= rr) {
          blend_pixel(px, py, color);
        }
      }
    }
  }

  void fill_polygon(const std::vector<Point2D>& points, const FillStyle2D& style) override {
    if (points.size() < 3) {
      return;
    }
    double min_xf = std::numeric_limits<double>::infinity();
    double max_xf = -std::numeric_limits<double>::infinity();
    double min_yf = std::numeric_limits<double>::infinity();
    double max_yf = -std::numeric_limits<double>::infinity();
    for (const auto& point : points) {
      min_xf = std::min(min_xf, point.x);
      max_xf = std::max(max_xf, point.x);
      min_yf = std::min(min_yf, point.y);
      max_yf = std::max(max_yf, point.y);
    }
    const RasterColor color = raster_parse_color(style.color, style.alpha);
    const int min_x = std::max(0, static_cast<int>(std::floor(min_xf * ss_)) - 1);
    const int max_x = std::min(raster_width_ - 1, static_cast<int>(std::ceil(max_xf * ss_)) + 1);
    const int min_y = std::max(0, static_cast<int>(std::floor(min_yf * ss_)) - 1);
    const int max_y = std::min(raster_height_ - 1, static_cast<int>(std::ceil(max_yf * ss_)) + 1);
    for (int py = min_y; py <= max_y; ++py) {
      const double yf = (static_cast<double>(py) + 0.5) / static_cast<double>(ss_);
      for (int px = min_x; px <= max_x; ++px) {
        const double xf = (static_cast<double>(px) + 0.5) / static_cast<double>(ss_);
        if (point_in_polygon(xf, yf, points)) {
          blend_pixel(px, py, color);
        }
      }
    }
  }

  void text(double x, double y, std::string_view text_value, const TextStyle2D& style) override {
    if (text_value.empty()) {
      return;
    }
    const RasterColor color = raster_parse_color(style.color, style.alpha);
    const int cell = std::max(1, static_cast<int>(std::round(style.size * static_cast<double>(ss_) / 8.0)));
    const int glyph_w = 5 * cell;
    const int glyph_h = 7 * cell;
    const int advance = 6 * cell;
    const int text_w = static_cast<int>(text_value.size()) * advance - cell;
    const int text_h = glyph_h;

    const double rotation = (style.orientation == TextOrientation2D::VerticalCcw ? -90.0 : 0.0) + style.rotation_deg;
    double anchor_offset_x = 0.0;
    if (style.anchor == TextAnchor2D::Center) {
      anchor_offset_x = -static_cast<double>(text_w) * 0.5;
    } else if (style.anchor == TextAnchor2D::Right) {
      anchor_offset_x = -static_cast<double>(text_w);
    }
    const double anchor_offset_y = -static_cast<double>(text_h) * 0.5;
    const double origin_x = x * static_cast<double>(ss_);
    const double origin_y = y * static_cast<double>(ss_);
    const double radians = rotation * 3.14159265358979323846 / 180.0;
    const double c = std::cos(radians);
    const double s = std::sin(radians);

    for (std::size_t i = 0; i < text_value.size(); ++i) {
      char c = text_value[i];
      if (c >= 'a' && c <= 'z') {
        c = static_cast<char>('A' + (c - 'a'));
      }
      const auto rows = raster_glyph_rows(c);
      for (int row = 0; row < 7; ++row) {
        for (int col = 0; col < 5; ++col) {
          if ((rows[static_cast<std::size_t>(row)] & (1u << (4 - col))) == 0u) {
            continue;
          }
          const double local_x = anchor_offset_x + static_cast<double>(static_cast<int>(i) * advance + col * cell);
          const double local_y = anchor_offset_y + static_cast<double>(row * cell);
          const double px = origin_x + local_x * c - local_y * s;
          const double py = origin_y + local_x * s + local_y * c;
          fill_text_block(px, py, cell, cell, color);
        }
      }
    }
  }

  std::string finish() const override {
    return encode_png_rgba(width_, height_, downsample_rgba());
  }

private:
  struct ClipRect {
    double x1{0.0};
    double y1{0.0};
    double x2{0.0};
    double y2{0.0};
  };

  bool clip_contains_pixel_(int x, int y) const {
    if (clip_stack_.empty()) {
      return true;
    }
    const auto& clip = clip_stack_.back();
    const double xf = static_cast<double>(x) + 0.5;
    const double yf = static_cast<double>(y) + 0.5;
    return xf >= clip.x1 && xf <= clip.x2 && yf >= clip.y1 && yf <= clip.y2;
  }

  void blend_pixel(int x, int y, const RasterColor& src) {
    if (x < 0 || x >= raster_width_ || y < 0 || y >= raster_height_) {
      return;
    }
    if (!clip_contains_pixel_(x, y)) {
      return;
    }
    const std::size_t idx = (static_cast<std::size_t>(y) * static_cast<std::size_t>(raster_width_) +
                             static_cast<std::size_t>(x)) * 4u;
    const double sa = static_cast<double>(src.a) / 255.0;
    const double da = static_cast<double>(rgba_[idx + 3]) / 255.0;
    const double out_a = sa + da * (1.0 - sa);
    if (!(out_a > 0.0)) {
      rgba_[idx + 0] = 0;
      rgba_[idx + 1] = 0;
      rgba_[idx + 2] = 0;
      rgba_[idx + 3] = 0;
      return;
    }
    const auto blend_channel = [&](std::uint8_t dst, std::uint8_t src_value) {
      const double dv = static_cast<double>(dst) / 255.0;
      const double sv = static_cast<double>(src_value) / 255.0;
      const double out = (sv * sa + dv * da * (1.0 - sa)) / out_a;
      return clamp_u8(out * 255.0);
    };
    rgba_[idx + 0] = blend_channel(rgba_[idx + 0], src.r);
    rgba_[idx + 1] = blend_channel(rgba_[idx + 1], src.g);
    rgba_[idx + 2] = blend_channel(rgba_[idx + 2], src.b);
    rgba_[idx + 3] = clamp_u8(out_a * 255.0);
  }

  void fill_text_block(double x, double y, int w, int h, const RasterColor& color) {
    const int min_x = std::max(0, static_cast<int>(std::floor(x)));
    const int max_x = std::min(raster_width_ - 1, static_cast<int>(std::ceil(x + static_cast<double>(w))) - 1);
    const int min_y = std::max(0, static_cast<int>(std::floor(y)));
    const int max_y = std::min(raster_height_ - 1, static_cast<int>(std::ceil(y + static_cast<double>(h))) - 1);
    for (int py = min_y; py <= max_y; ++py) {
      for (int px = min_x; px <= max_x; ++px) {
        blend_pixel(px, py, color);
      }
    }
  }

  std::vector<double> dash_pattern(std::string_view style, double width) const {
    if (style == "dashed" || style == "--") {
      return {4.0 * width, 2.5 * width};
    }
    if (style == "dotted" || style == ":" || style == ".") {
      return {width, 2.0 * width};
    }
    if (style == "dashdot" || style == "-.") {
      return {4.0 * width, 2.0 * width, width, 2.0 * width};
    }
    return {};
  }

  bool dash_visible(double along, const std::vector<double>& pattern) const {
    if (pattern.empty()) {
      return true;
    }
    const double cycle = std::accumulate(pattern.begin(), pattern.end(), 0.0);
    if (!(cycle > 0.0)) {
      return true;
    }
    double position = std::fmod(along, cycle);
    if (position < 0.0) {
      position += cycle;
    }
    double cursor = 0.0;
    for (std::size_t i = 0; i < pattern.size(); ++i) {
      const double next = cursor + pattern[i];
      if (position <= next) {
        return (i % 2u) == 0u;
      }
      cursor = next;
    }
    return true;
  }

  std::vector<std::uint8_t> downsample_rgba() const {
    std::vector<std::uint8_t> out(static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_) * 4u, 255u);
    const double denom = static_cast<double>(ss_ * ss_);
    for (int y = 0; y < height_; ++y) {
      for (int x = 0; x < width_; ++x) {
        double accum[4]{0.0, 0.0, 0.0, 0.0};
        for (int sy = 0; sy < ss_; ++sy) {
          for (int sx = 0; sx < ss_; ++sx) {
            const int rx = x * ss_ + sx;
            const int ry = y * ss_ + sy;
            const std::size_t src_idx = (static_cast<std::size_t>(ry) * static_cast<std::size_t>(raster_width_) +
                                         static_cast<std::size_t>(rx)) * 4u;
            accum[0] += rgba_[src_idx + 0];
            accum[1] += rgba_[src_idx + 1];
            accum[2] += rgba_[src_idx + 2];
            accum[3] += rgba_[src_idx + 3];
          }
        }
        const std::size_t dst_idx = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) +
                                     static_cast<std::size_t>(x)) * 4u;
        out[dst_idx + 0] = clamp_u8(accum[0] / denom);
        out[dst_idx + 1] = clamp_u8(accum[1] / denom);
        out[dst_idx + 2] = clamp_u8(accum[2] / denom);
        out[dst_idx + 3] = clamp_u8(accum[3] / denom);
      }
    }
    return out;
  }

  std::int32_t width_{0};
  std::int32_t height_{0};
  int ss_{2};
  int raster_width_{0};
  int raster_height_{0};
  std::vector<std::uint8_t> rgba_{};
  std::vector<ClipRect> clip_stack_{};
};

} // namespace detail
} // namespace pluss::plotism
