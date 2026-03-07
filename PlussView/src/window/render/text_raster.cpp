#include "text_raster.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>
#include <optional>

#include "pluss/view/window/text_layout.h"

namespace pluss::view::window::detail {

namespace {

constexpr int kGlyphCols = 5;
constexpr int kGlyphRows = 7;

[[nodiscard]] bool contains(const RectI rect, const PointI p) {
  return !rect.empty() && p.x >= rect.x && p.y >= rect.y && p.x < (rect.x + rect.width) &&
         p.y < (rect.y + rect.height);
}

[[nodiscard]] std::array<std::uint8_t, 7> glyph_bitmap(char ch) {
  switch (ch) {
    case 'A': return {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
    case 'B': return {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E};
    case 'C': return {0x0F, 0x10, 0x10, 0x10, 0x10, 0x10, 0x0F};
    case 'D': return {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E};
    case 'E': return {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F};
    case 'F': return {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10};
    case 'G': return {0x0F, 0x10, 0x10, 0x17, 0x11, 0x11, 0x0F};
    case 'H': return {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
    case 'I': return {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E};
    case 'J': return {0x01, 0x01, 0x01, 0x01, 0x11, 0x11, 0x0E};
    case 'K': return {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11};
    case 'L': return {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F};
    case 'M': return {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11};
    case 'N': return {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11};
    case 'O': return {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
    case 'P': return {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10};
    case 'Q': return {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D};
    case 'R': return {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11};
    case 'S': return {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E};
    case 'T': return {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};
    case 'U': return {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
    case 'V': return {0x11, 0x11, 0x11, 0x11, 0x0A, 0x0A, 0x04};
    case 'W': return {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A};
    case 'X': return {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11};
    case 'Y': return {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04};
    case 'Z': return {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F};
    case 'a': return {0x00, 0x00, 0x0E, 0x01, 0x0F, 0x11, 0x0F};
    case 'b': return {0x10, 0x10, 0x16, 0x19, 0x11, 0x11, 0x1E};
    case 'c': return {0x00, 0x00, 0x0F, 0x10, 0x10, 0x10, 0x0F};
    case 'd': return {0x01, 0x01, 0x0D, 0x13, 0x11, 0x11, 0x0F};
    case 'e': return {0x00, 0x00, 0x0E, 0x11, 0x1F, 0x10, 0x0F};
    case 'f': return {0x03, 0x04, 0x0E, 0x04, 0x04, 0x04, 0x04};
    case 'g': return {0x00, 0x00, 0x0F, 0x11, 0x0F, 0x01, 0x0E};
    case 'h': return {0x10, 0x10, 0x16, 0x19, 0x11, 0x11, 0x11};
    case 'i': return {0x04, 0x00, 0x0C, 0x04, 0x04, 0x04, 0x0E};
    case 'j': return {0x02, 0x00, 0x06, 0x02, 0x02, 0x12, 0x0C};
    case 'k': return {0x10, 0x10, 0x12, 0x14, 0x18, 0x14, 0x12};
    case 'l': return {0x0C, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E};
    case 'm': return {0x00, 0x00, 0x1A, 0x15, 0x15, 0x15, 0x15};
    case 'n': return {0x00, 0x00, 0x16, 0x19, 0x11, 0x11, 0x11};
    case 'o': return {0x00, 0x00, 0x0E, 0x11, 0x11, 0x11, 0x0E};
    case 'p': return {0x00, 0x00, 0x1E, 0x11, 0x1E, 0x10, 0x10};
    case 'q': return {0x00, 0x00, 0x0D, 0x13, 0x0F, 0x01, 0x01};
    case 'r': return {0x00, 0x00, 0x16, 0x19, 0x10, 0x10, 0x10};
    case 's': return {0x00, 0x00, 0x0F, 0x10, 0x0E, 0x01, 0x1E};
    case 't': return {0x04, 0x04, 0x1E, 0x04, 0x04, 0x04, 0x03};
    case 'u': return {0x00, 0x00, 0x11, 0x11, 0x11, 0x13, 0x0D};
    case 'v': return {0x00, 0x00, 0x11, 0x11, 0x11, 0x0A, 0x04};
    case 'w': return {0x00, 0x00, 0x11, 0x11, 0x15, 0x15, 0x0A};
    case 'x': return {0x00, 0x00, 0x11, 0x0A, 0x04, 0x0A, 0x11};
    case 'y': return {0x00, 0x00, 0x11, 0x11, 0x0F, 0x01, 0x0E};
    case 'z': return {0x00, 0x00, 0x1F, 0x02, 0x04, 0x08, 0x1F};
    case '0': return {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E};
    case '1': return {0x04, 0x0C, 0x14, 0x04, 0x04, 0x04, 0x1F};
    case '2': return {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F};
    case '3': return {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E};
    case '4': return {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02};
    case '5': return {0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E};
    case '6': return {0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E};
    case '7': return {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08};
    case '8': return {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E};
    case '9': return {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E};
    case '.': return {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C};
    case ',': return {0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x08};
    case '!': return {0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04};
    case '?': return {0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04};
    case '\'': return {0x04, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00};
    case '"': return {0x0A, 0x0A, 0x0A, 0x00, 0x00, 0x00, 0x00};
    case ':': return {0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00};
    case ';': return {0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x08};
    case '-': return {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00};
    case '+': return {0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00};
    case '/': return {0x01, 0x02, 0x02, 0x04, 0x08, 0x08, 0x10};
    case '*': return {0x00, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x00};
    case '=': return {0x00, 0x1F, 0x00, 0x1F, 0x00, 0x00, 0x00};
    case '(': return {0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02};
    case ')': return {0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08};
    case '[': return {0x0E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0E};
    case ']': return {0x0E, 0x02, 0x02, 0x02, 0x02, 0x02, 0x0E};
    case '<': return {0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x02};
    case '>': return {0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08};
    case '_': return {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F};
    case '^': return {0x04, 0x0A, 0x11, 0x00, 0x00, 0x00, 0x00};
    case '|': return {0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};
    case '#': return {0x0A, 0x0A, 0x1F, 0x0A, 0x1F, 0x0A, 0x0A};
    case '&': return {0x0C, 0x12, 0x14, 0x08, 0x15, 0x12, 0x0D};
    case '%': return {0x18, 0x19, 0x02, 0x04, 0x08, 0x13, 0x03};
    case '@': return {0x0E, 0x11, 0x17, 0x15, 0x17, 0x10, 0x0E};
    case ' ': return {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    default:
      return {0x1F, 0x11, 0x01, 0x07, 0x04, 0x00, 0x04};
  }
}

void fill_block(PixelCanvas& canvas,
                const RectI rect,
                const Color color,
                const std::optional<RectI>& clip) {
  if (rect.empty()) {
    return;
  }
  const auto size = canvas.size();
  int x0 = std::max(0, rect.x);
  int y0 = std::max(0, rect.y);
  int x1 = std::min(size.width, rect.x + rect.width);
  int y1 = std::min(size.height, rect.y + rect.height);
  if (clip.has_value()) {
    x0 = std::max(x0, clip->x);
    y0 = std::max(y0, clip->y);
    x1 = std::min(x1, clip->x + clip->width);
    y1 = std::min(y1, clip->y + clip->height);
  }
  if (x0 >= x1 || y0 >= y1) {
    return;
  }

  for (int y = y0; y < y1; ++y) {
    for (int x = x0; x < x1; ++x) {
      canvas.set_pixel(PointI{x, y}, color);
    }
  }
}

void blend_block(PixelCanvas& canvas,
                 const RectI rect,
                 const Color color,
                 const std::uint8_t alpha,
                 const std::optional<RectI>& clip) {
  if (rect.empty() || alpha == 0) {
    return;
  }
  const auto size = canvas.size();
  int x0 = std::max(0, rect.x);
  int y0 = std::max(0, rect.y);
  int x1 = std::min(size.width, rect.x + rect.width);
  int y1 = std::min(size.height, rect.y + rect.height);
  if (clip.has_value()) {
    x0 = std::max(x0, clip->x);
    y0 = std::max(y0, clip->y);
    x1 = std::min(x1, clip->x + clip->width);
    y1 = std::min(y1, clip->y + clip->height);
  }
  if (x0 >= x1 || y0 >= y1) {
    return;
  }

  for (int y = y0; y < y1; ++y) {
    for (int x = x0; x < x1; ++x) {
      canvas.blend_pixel(PointI{x, y}, color, alpha);
    }
  }
}

[[nodiscard]] double fpart(const double x) {
  return x - std::floor(x);
}

[[nodiscard]] double rfpart(const double x) {
  return 1.0 - fpart(x);
}

void draw_wu_line(PixelCanvas& canvas,
                  PointI p0,
                  PointI p1,
                  const Color color,
                  const std::optional<RectI>& clip) {
  bool steep = std::abs(p1.y - p0.y) > std::abs(p1.x - p0.x);
  if (steep) {
    std::swap(p0.x, p0.y);
    std::swap(p1.x, p1.y);
  }
  if (p0.x > p1.x) {
    std::swap(p0, p1);
  }

  const double dx = static_cast<double>(p1.x - p0.x);
  const double dy = static_cast<double>(p1.y - p0.y);
  const double gradient = (dx == 0.0) ? 1.0 : (dy / dx);

  const auto plot = [&](const int x, const int y, const double intensity) {
    const auto alpha = static_cast<std::uint8_t>(std::clamp(intensity, 0.0, 1.0) * 255.0);
    if (alpha == 0) {
      return;
    }
    if (steep) {
      const PointI p{y, x};
      if (!clip.has_value() || contains(*clip, p)) {
        canvas.blend_pixel(p, color, alpha);
      }
    } else {
      const PointI p{x, y};
      if (!clip.has_value() || contains(*clip, p)) {
        canvas.blend_pixel(p, color, alpha);
      }
    }
  };

  const double xend1 = std::round(static_cast<double>(p0.x));
  const double yend1 = static_cast<double>(p0.y) + gradient * (xend1 - static_cast<double>(p0.x));
  const double xgap1 = rfpart(static_cast<double>(p0.x) + 0.5);
  const int xpxl1 = static_cast<int>(xend1);
  const int ypxl1 = static_cast<int>(std::floor(yend1));
  plot(xpxl1, ypxl1, rfpart(yend1) * xgap1);
  plot(xpxl1, ypxl1 + 1, fpart(yend1) * xgap1);

  double intery = yend1 + gradient;

  const double xend2 = std::round(static_cast<double>(p1.x));
  const double yend2 = static_cast<double>(p1.y) + gradient * (xend2 - static_cast<double>(p1.x));
  const double xgap2 = fpart(static_cast<double>(p1.x) + 0.5);
  const int xpxl2 = static_cast<int>(xend2);
  const int ypxl2 = static_cast<int>(std::floor(yend2));

  for (int x = xpxl1 + 1; x < xpxl2; ++x) {
    plot(x, static_cast<int>(std::floor(intery)), rfpart(intery));
    plot(x, static_cast<int>(std::floor(intery)) + 1, fpart(intery));
    intery += gradient;
  }

  plot(xpxl2, ypxl2, rfpart(yend2) * xgap2);
  plot(xpxl2, ypxl2 + 1, fpart(yend2) * xgap2);
}

void draw_line(PixelCanvas& canvas,
               PointI p0,
               PointI p1,
               const int thickness,
               const Color color,
               const std::optional<RectI>& clip) {
  const int t = std::max(1, thickness);
  if (t == 1) {
    draw_wu_line(canvas, p0, p1, color, clip);
    return;
  }

  const int dx = std::abs(p1.x - p0.x);
  const int dy = std::abs(p1.y - p0.y);
  const int sx = (p0.x < p1.x) ? 1 : -1;
  const int sy = (p0.y < p1.y) ? 1 : -1;
  int err = dx - dy;

  while (true) {
    fill_block(canvas, RectI{p0.x - t / 2, p0.y - t / 2, t, t}, color, clip);
    if (p0.x == p1.x && p0.y == p1.y) {
      break;
    }

    const int e2 = err * 2;
    if (e2 > -dy) {
      err -= dy;
      p0.x += sx;
    }
    if (e2 < dx) {
      err += dx;
      p0.y += sy;
    }
  }
}

void draw_glyph(PixelCanvas& canvas,
                const TextGlyph& glyph,
                const PointI anchor,
                const Color color,
                const std::optional<RectI>& clip) {
  if (glyph.ch == ' ') {
    return;
  }

  const auto bitmap = glyph_bitmap(glyph.ch);
  const int cell = std::max(1, (glyph.pixel_size + (kGlyphRows - 1)) / kGlyphRows);
  const int top = anchor.y + glyph.y - (kGlyphRows * cell);
  const auto edge_alpha = static_cast<std::uint8_t>(
      std::clamp(static_cast<int>(color.a * 3 / 4), 0, 255));
  const auto corner_alpha = static_cast<std::uint8_t>(
      std::clamp(static_cast<int>(color.a * 2 / 5), 0, 255));

  for (int row = 0; row < kGlyphRows; ++row) {
    const auto bits = bitmap[static_cast<std::size_t>(row)];
    for (int col = 0; col < kGlyphCols; ++col) {
      if (((bits >> (kGlyphCols - 1 - col)) & 0x01U) == 0U) {
        continue;
      }

      const int block_x = anchor.x + glyph.x + col * cell;
      const int block_y = top + row * cell;
      if (cell == 1) {
        fill_block(canvas, RectI{block_x, block_y, 1, 1}, color, clip);
        blend_block(canvas, RectI{block_x + 1, block_y, 1, 1}, color, edge_alpha, clip);
        blend_block(canvas, RectI{block_x, block_y + 1, 1, 1}, color, edge_alpha, clip);
        blend_block(canvas, RectI{block_x + 1, block_y + 1, 1, 1}, color, corner_alpha, clip);
        continue;
      }
      if (cell == 2) {
        fill_block(canvas, RectI{block_x, block_y, 2, 2}, color, clip);
        blend_block(canvas, RectI{block_x - 1, block_y, 1, 2}, color, corner_alpha, clip);
        blend_block(canvas, RectI{block_x + 2, block_y, 1, 2}, color, corner_alpha, clip);
        blend_block(canvas, RectI{block_x, block_y - 1, 2, 1}, color, corner_alpha, clip);
        blend_block(canvas, RectI{block_x, block_y + 2, 2, 1}, color, corner_alpha, clip);
        continue;
      }

      fill_block(canvas, RectI{block_x + 1, block_y + 1, cell - 2, cell - 2}, color, clip);

      for (int i = 1; i < (cell - 1); ++i) {
        const PointI top_p{block_x + i, block_y};
        const PointI bot_p{block_x + i, block_y + cell - 1};
        const PointI left_p{block_x, block_y + i};
        const PointI right_p{block_x + cell - 1, block_y + i};
        if (!clip.has_value() || contains(*clip, top_p)) canvas.blend_pixel(top_p, color, edge_alpha);
        if (!clip.has_value() || contains(*clip, bot_p)) canvas.blend_pixel(bot_p, color, edge_alpha);
        if (!clip.has_value() || contains(*clip, left_p)) canvas.blend_pixel(left_p, color, edge_alpha);
        if (!clip.has_value() || contains(*clip, right_p)) canvas.blend_pixel(right_p, color, edge_alpha);
      }

      const PointI p0{block_x, block_y};
      const PointI p1{block_x + cell - 1, block_y};
      const PointI p2{block_x, block_y + cell - 1};
      const PointI p3{block_x + cell - 1, block_y + cell - 1};
      if (!clip.has_value() || contains(*clip, p0)) canvas.blend_pixel(p0, color, corner_alpha);
      if (!clip.has_value() || contains(*clip, p1)) canvas.blend_pixel(p1, color, corner_alpha);
      if (!clip.has_value() || contains(*clip, p2)) canvas.blend_pixel(p2, color, corner_alpha);
      if (!clip.has_value() || contains(*clip, p3)) canvas.blend_pixel(p3, color, corner_alpha);
    }
  }
}

} // namespace

#if !defined(__APPLE__)
bool render_text_command_native(PixelCanvas& canvas,
                                const TextCommand& command,
                                const std::optional<RectI>& clip,
                                std::vector<RenderEvent>& events) {
  (void)canvas;
  (void)command;
  (void)clip;
  (void)events;
  return false;
}
#endif

void render_text_command(PixelCanvas& canvas,
                         const TextCommand& command,
                         const std::optional<RectI>& clip,
                         std::vector<RenderEvent>& events) {
  if (render_text_command_native(canvas, command, clip, events)) {
    return;
  }

  const TextLayout layout = layout_math_text(command.text, command.pixel_size);

  for (const auto& warning : layout.warnings) {
    events.push_back(RenderEvent{RenderEventKind::UnsupportedCommand, warning});
  }

  const auto draw_pass = [&](const int x_offset) {
    const PointI anchor{command.anchor.x + x_offset, command.anchor.y};
    for (const auto& line : layout.lines) {
      draw_line(canvas,
                PointI{anchor.x + line.x0, anchor.y + line.y0},
                PointI{anchor.x + line.x1, anchor.y + line.y1},
                line.thickness,
                command.color,
                clip);
    }
    for (const auto& glyph : layout.glyphs) {
      draw_glyph(canvas, glyph, anchor, command.color, clip);
    }
  };

  draw_pass(0);
  if (command.font_weight >= 600) {
    draw_pass(1);
  }
}

} // namespace pluss::view::window::detail
