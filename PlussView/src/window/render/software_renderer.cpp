#include "pluss/view/window/renderer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

#include "text_raster.h"

namespace pluss::view::window {

namespace {

[[nodiscard]] constexpr int clamp_thickness(const int thickness) noexcept {
  return std::max(1, thickness);
}

[[nodiscard]] constexpr bool contains(const RectI rect, const PointI p) noexcept {
  return !rect.empty() && p.x >= rect.x && p.y >= rect.y && p.x < (rect.x + rect.width) &&
         p.y < (rect.y + rect.height);
}

[[nodiscard]] RectI canvas_bounds(const PixelCanvas& canvas) {
  const auto size = canvas.size();
  return RectI{0, 0, size.width, size.height};
}

[[nodiscard]] RectI intersect_rect(const RectI lhs, const RectI rhs) {
  const int x0 = std::max(lhs.x, rhs.x);
  const int y0 = std::max(lhs.y, rhs.y);
  const int x1 = std::min(lhs.x + lhs.width, rhs.x + rhs.width);
  const int y1 = std::min(lhs.y + lhs.height, rhs.y + rhs.height);
  return RectI{x0, y0, std::max(0, x1 - x0), std::max(0, y1 - y0)};
}

[[nodiscard]] std::optional<RectI> normalize_clip(const PixelCanvas& canvas,
                                                  const std::optional<RectI>& clip) {
  if (!clip.has_value()) {
    return std::nullopt;
  }
  const RectI clipped = intersect_rect(*clip, canvas_bounds(canvas));
  if (clipped.empty()) {
    return std::nullopt;
  }
  return clipped;
}

inline void set_pixel(PixelCanvas& canvas,
                      const PointI point,
                      const Color color,
                      const std::optional<RectI>& clip) {
  if (clip.has_value() && !contains(*clip, point)) {
    return;
  }
  canvas.set_pixel(point, color);
}

inline void blend_pixel(PixelCanvas& canvas,
                        const PointI point,
                        const Color color,
                        const std::uint8_t alpha,
                        const std::optional<RectI>& clip) {
  if (clip.has_value() && !contains(*clip, point)) {
    return;
  }
  canvas.blend_pixel(point, color, alpha);
}

[[nodiscard]] bool clip_line_segment(PointI& p0, PointI& p1, const RectI rect) {
  if (rect.empty()) {
    return false;
  }

  const double x_min = static_cast<double>(rect.x);
  const double y_min = static_cast<double>(rect.y);
  const double x_max = static_cast<double>(rect.x + rect.width - 1);
  const double y_max = static_cast<double>(rect.y + rect.height - 1);

  const double dx = static_cast<double>(p1.x - p0.x);
  const double dy = static_cast<double>(p1.y - p0.y);

  double t0 = 0.0;
  double t1 = 1.0;

  const auto test = [&](const double p, const double q) -> bool {
    if (std::abs(p) < 1e-12) {
      return q >= 0.0;
    }
    const double r = q / p;
    if (p < 0.0) {
      if (r > t1) {
        return false;
      }
      t0 = std::max(t0, r);
      return true;
    }
    if (r < t0) {
      return false;
    }
    t1 = std::min(t1, r);
    return true;
  };

  if (!test(-dx, static_cast<double>(p0.x) - x_min)) {
    return false;
  }
  if (!test(dx, x_max - static_cast<double>(p0.x))) {
    return false;
  }
  if (!test(-dy, static_cast<double>(p0.y) - y_min)) {
    return false;
  }
  if (!test(dy, y_max - static_cast<double>(p0.y))) {
    return false;
  }
  if (t1 < t0) {
    return false;
  }

  const auto map = [&](const double t) -> PointI {
    return PointI{
        static_cast<int>(std::lround(static_cast<double>(p0.x) + t * dx)),
        static_cast<int>(std::lround(static_cast<double>(p0.y) + t * dy)),
    };
  };

  const PointI c0 = map(t0);
  const PointI c1 = map(t1);
  p0 = c0;
  p1 = c1;
  return true;
}

void draw_disc(PixelCanvas& canvas,
               const PointI center,
               const int radius,
               const Color color,
               const std::optional<RectI>& clip) {
  const int r = std::max(1, radius);
  const int r2 = r * r;
  for (int dy = -r; dy <= r; ++dy) {
    for (int dx = -r; dx <= r; ++dx) {
      const int dist2 = dx * dx + dy * dy;
      if (dist2 <= r2) {
        set_pixel(canvas, PointI{center.x + dx, center.y + dy}, color, clip);
      }
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
  if (clip.has_value() && !clip_line_segment(p0, p1, *clip)) {
    return;
  }
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
  const double gradient = (dx == 0.0) ? 1.0 : dy / dx;

  const double xend1 = std::round(static_cast<double>(p0.x));
  const double yend1 = static_cast<double>(p0.y) + gradient * (xend1 - static_cast<double>(p0.x));
  const double xgap1 = rfpart(static_cast<double>(p0.x) + 0.5);
  const int xpxl1 = static_cast<int>(xend1);
  const int ypxl1 = static_cast<int>(std::floor(yend1));

  const auto plot = [&](const int x, const int y, const double intensity) {
    const auto alpha = static_cast<std::uint8_t>(std::clamp(intensity, 0.0, 1.0) * 255.0);
    if (steep) {
      blend_pixel(canvas, PointI{y, x}, color, alpha, clip);
    } else {
      blend_pixel(canvas, PointI{x, y}, color, alpha, clip);
    }
  };

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

void draw_thick_line(PixelCanvas& canvas,
                     PointI p0,
                     PointI p1,
                     const int thickness,
                     const Color color,
                     const std::optional<RectI>& clip) {
  if (clip.has_value() && !clip_line_segment(p0, p1, *clip)) {
    return;
  }

  const int dx = std::abs(p1.x - p0.x);
  const int dy = std::abs(p1.y - p0.y);
  const int sx = (p0.x < p1.x) ? 1 : -1;
  const int sy = (p0.y < p1.y) ? 1 : -1;

  int err = dx - dy;
  int x = p0.x;
  int y = p0.y;
  const int radius = std::max(1, thickness / 2);

  while (true) {
    draw_disc(canvas, PointI{x, y}, radius, color, clip);
    if (x == p1.x && y == p1.y) {
      break;
    }
    const int err2 = err * 2;
    if (err2 > -dy) {
      err -= dy;
      x += sx;
    }
    if (err2 < dx) {
      err += dx;
      y += sy;
    }
  }
}

void draw_line(PixelCanvas& canvas,
               const PointI p0,
               const PointI p1,
               const int thickness,
               const Color color,
               const std::optional<RectI>& clip) {
  if (clamp_thickness(thickness) == 1) {
    draw_wu_line(canvas, p0, p1, color, clip);
    return;
  }
  draw_thick_line(canvas, p0, p1, thickness, color, clip);
}

void fill_rect(PixelCanvas& canvas, const RectI rect, const Color color, const std::optional<RectI>& clip) {
  if (rect.empty()) {
    return;
  }

  const RectI bounded =
      clip.has_value() ? intersect_rect(intersect_rect(rect, *clip), canvas_bounds(canvas))
                       : intersect_rect(rect, canvas_bounds(canvas));
  if (bounded.empty()) {
    return;
  }

  const bool opaque = color.a == 255;
  for (int y = bounded.y; y < (bounded.y + bounded.height); ++y) {
    for (int x = bounded.x; x < (bounded.x + bounded.width); ++x) {
      if (opaque) {
        canvas.set_pixel(PointI{x, y}, color);
      } else {
        canvas.blend_pixel(PointI{x, y}, color, color.a);
      }
    }
  }
}

[[nodiscard]] constexpr int clamp_corner_radius(const RectI rect, const int radius) {
  if (rect.empty() || radius <= 0) {
    return 0;
  }
  const int max_radius = std::max(0, std::min(rect.width, rect.height) / 2);
  return std::clamp(radius, 0, max_radius);
}

[[nodiscard]] bool rounded_rect_contains(const RectI rect, const int radius, const double x, const double y) {
  if (rect.empty()) {
    return false;
  }
  const int r = clamp_corner_radius(rect, radius);
  if (r <= 0) {
    return x >= static_cast<double>(rect.x) && y >= static_cast<double>(rect.y) &&
           x < static_cast<double>(rect.x + rect.width) && y < static_cast<double>(rect.y + rect.height);
  }

  const double left = static_cast<double>(rect.x);
  const double top = static_cast<double>(rect.y);
  const double right = static_cast<double>(rect.x + rect.width);
  const double bottom = static_cast<double>(rect.y + rect.height);
  if (x < left || y < top || x >= right || y >= bottom) {
    return false;
  }

  const double r_f = static_cast<double>(r);
  if (x >= left + r_f && x < right - r_f) {
    return true;
  }
  if (y >= top + r_f && y < bottom - r_f) {
    return true;
  }

  const double cx = (x < left + r_f) ? (left + r_f) : (right - r_f - 1.0);
  const double cy = (y < top + r_f) ? (top + r_f) : (bottom - r_f - 1.0);
  const double dx = x - cx;
  const double dy = y - cy;
  const double dist2 = dx * dx + dy * dy;
  return dist2 <= (r_f * r_f);
}

[[nodiscard]] std::uint8_t rounded_rect_coverage(const RectI rect, const int radius, const int x, const int y) {
  static constexpr std::array<std::pair<double, double>, 4> kSamples{{
      {0.25, 0.25},
      {0.75, 0.25},
      {0.25, 0.75},
      {0.75, 0.75},
  }};
  int inside = 0;
  for (const auto [sx, sy] : kSamples) {
    if (rounded_rect_contains(rect, radius, static_cast<double>(x) + sx, static_cast<double>(y) + sy)) {
      ++inside;
    }
  }
  return static_cast<std::uint8_t>((inside * 255) / static_cast<int>(kSamples.size()));
}

void fill_rounded_rect(PixelCanvas& canvas,
                       const RectI rect,
                       const int radius,
                       const Color color,
                       const std::optional<RectI>& clip) {
  if (rect.empty()) {
    return;
  }

  const RectI bounded =
      clip.has_value() ? intersect_rect(intersect_rect(rect, *clip), canvas_bounds(canvas))
                       : intersect_rect(rect, canvas_bounds(canvas));
  if (bounded.empty()) {
    return;
  }

  const int r = clamp_corner_radius(rect, radius);
  for (int y = bounded.y; y < (bounded.y + bounded.height); ++y) {
    for (int x = bounded.x; x < (bounded.x + bounded.width); ++x) {
      const std::uint8_t coverage = rounded_rect_coverage(rect, r, x, y);
      if (coverage == 0) {
        continue;
      }
      const std::uint8_t alpha = static_cast<std::uint8_t>(
          (static_cast<std::uint16_t>(coverage) * static_cast<std::uint16_t>(color.a)) / 255U);
      if (alpha == 0) {
        continue;
      }
      if (alpha == 255) {
        canvas.set_pixel(PointI{x, y}, color);
      } else {
        canvas.blend_pixel(PointI{x, y}, color, alpha);
      }
    }
  }
}

[[nodiscard]] Color lerp_color(const Color a, const Color b, const double t) {
  const double u = std::clamp(t, 0.0, 1.0);
  const auto blend = [u](const std::uint8_t x, const std::uint8_t y) -> std::uint8_t {
    const double value = static_cast<double>(x) + (static_cast<double>(y) - static_cast<double>(x)) * u;
    return static_cast<std::uint8_t>(std::clamp(static_cast<int>(std::lround(value)), 0, 255));
  };
  return Color::rgba(blend(a.r, b.r), blend(a.g, b.g), blend(a.b, b.b), blend(a.a, b.a));
}

void fill_linear_gradient(PixelCanvas& canvas,
                          const FillLinearGradientCommand& command,
                          const std::optional<RectI>& clip) {
  if (command.rect.empty()) {
    return;
  }
  const RectI bounded =
      clip.has_value() ? intersect_rect(intersect_rect(command.rect, *clip), canvas_bounds(canvas))
                       : intersect_rect(command.rect, canvas_bounds(canvas));
  if (bounded.empty()) {
    return;
  }

  const int radius = clamp_corner_radius(command.rect, command.radius);
  const int denom = std::max(1, command.horizontal ? (command.rect.width - 1) : (command.rect.height - 1));
  for (int y = bounded.y; y < bounded.y + bounded.height; ++y) {
    for (int x = bounded.x; x < bounded.x + bounded.width; ++x) {
      const std::uint8_t coverage = rounded_rect_coverage(command.rect, radius, x, y);
      if (coverage == 0) {
        continue;
      }
      const int axis = command.horizontal ? (x - command.rect.x) : (y - command.rect.y);
      const double t = static_cast<double>(std::clamp(axis, 0, denom)) / static_cast<double>(denom);
      const Color color = lerp_color(command.start_color, command.end_color, t);
      const std::uint8_t alpha =
          static_cast<std::uint8_t>((static_cast<std::uint16_t>(coverage) * static_cast<std::uint16_t>(color.a)) /
                                    255U);
      if (alpha == 0) {
        continue;
      }
      if (alpha == 255) {
        canvas.set_pixel(PointI{x, y}, color);
      } else {
        canvas.blend_pixel(PointI{x, y}, color, alpha);
      }
    }
  }
}

void stroke_rounded_rect(PixelCanvas& canvas,
                         const RectI rect,
                         const int radius,
                         const int thickness,
                         const Color color,
                         const std::optional<RectI>& clip) {
  if (rect.empty()) {
    return;
  }
  const int t = clamp_thickness(thickness);
  const RectI outer = rect;
  const int outer_radius = clamp_corner_radius(outer, radius);

  const RectI inner{
      outer.x + t,
      outer.y + t,
      std::max(0, outer.width - (t * 2)),
      std::max(0, outer.height - (t * 2)),
  };
  const int inner_radius = clamp_corner_radius(inner, std::max(0, outer_radius - t));

  const RectI bounded =
      clip.has_value() ? intersect_rect(intersect_rect(outer, *clip), canvas_bounds(canvas))
                       : intersect_rect(outer, canvas_bounds(canvas));
  if (bounded.empty()) {
    return;
  }

  for (int y = bounded.y; y < (bounded.y + bounded.height); ++y) {
    for (int x = bounded.x; x < (bounded.x + bounded.width); ++x) {
      const std::uint8_t cov_outer = rounded_rect_coverage(outer, outer_radius, x, y);
      if (cov_outer == 0) {
        continue;
      }
      std::uint8_t cov_inner = 0;
      if (!inner.empty()) {
        cov_inner = rounded_rect_coverage(inner, inner_radius, x, y);
      }
      if (cov_outer <= cov_inner) {
        continue;
      }
      const std::uint8_t ring_cov = static_cast<std::uint8_t>(cov_outer - cov_inner);
      const std::uint8_t alpha = static_cast<std::uint8_t>(
          (static_cast<std::uint16_t>(ring_cov) * static_cast<std::uint16_t>(color.a)) / 255U);
      if (alpha == 0) {
        continue;
      }
      if (alpha == 255) {
        canvas.set_pixel(PointI{x, y}, color);
      } else {
        canvas.blend_pixel(PointI{x, y}, color, alpha);
      }
    }
  }
}

void stroke_rect(PixelCanvas& canvas, const StrokeRectCommand& command, const std::optional<RectI>& clip) {
  if (command.rect.empty()) {
    return;
  }

  const int t = clamp_thickness(command.thickness);
  const RectI r = command.rect;
  draw_line(canvas, PointI{r.x, r.y}, PointI{r.x + r.width - 1, r.y}, t, command.color, clip);
  draw_line(canvas, PointI{r.x, r.y}, PointI{r.x, r.y + r.height - 1}, t, command.color, clip);
  draw_line(canvas,
            PointI{r.x + r.width - 1, r.y},
            PointI{r.x + r.width - 1, r.y + r.height - 1},
            t,
            command.color,
            clip);
  draw_line(canvas,
            PointI{r.x, r.y + r.height - 1},
            PointI{r.x + r.width - 1, r.y + r.height - 1},
            t,
            command.color,
            clip);
}

void draw_polyline(PixelCanvas& canvas, const PolylineCommand& command, const std::optional<RectI>& clip) {
  if (command.points.size() < 2) {
    return;
  }

  for (std::size_t index = 1; index < command.points.size(); ++index) {
    draw_line(canvas,
              command.points[index - 1],
              command.points[index],
              clamp_thickness(command.thickness),
              command.color,
              clip);
  }
}

void fill_polygon(PixelCanvas& canvas, const FillPolygonCommand& command, const std::optional<RectI>& clip) {
  if (command.points.size() < 3) {
    return;
  }

  const RectI bounds = canvas_bounds(canvas);
  const RectI clip_rect = clip.has_value() ? intersect_rect(bounds, *clip) : bounds;
  if (clip_rect.empty()) {
    return;
  }

  int min_y = command.points.front().y;
  int max_y = command.points.front().y;
  for (const auto& point : command.points) {
    min_y = std::min(min_y, point.y);
    max_y = std::max(max_y, point.y);
  }

  min_y = std::max(min_y, clip_rect.y);
  max_y = std::min(max_y, clip_rect.y + clip_rect.height - 1);
  if (min_y > max_y) {
    return;
  }

  const bool opaque = command.color.a == 255;
  std::vector<double> intersections;
  intersections.reserve(command.points.size());

  for (int y = min_y; y <= max_y; ++y) {
    intersections.clear();
    const double scan_y = static_cast<double>(y) + 0.5;

    for (std::size_t i = 0; i < command.points.size(); ++i) {
      const PointI p0 = command.points[i];
      const PointI p1 = command.points[(i + 1) % command.points.size()];
      if (p0.y == p1.y) {
        continue;
      }
      const int y_lo = std::min(p0.y, p1.y);
      const int y_hi = std::max(p0.y, p1.y);
      if (scan_y < static_cast<double>(y_lo) || scan_y >= static_cast<double>(y_hi)) {
        continue;
      }
      const double t = (scan_y - static_cast<double>(p0.y)) / static_cast<double>(p1.y - p0.y);
      intersections.push_back(static_cast<double>(p0.x) + t * static_cast<double>(p1.x - p0.x));
    }

    if (intersections.size() < 2) {
      continue;
    }
    std::sort(intersections.begin(), intersections.end());

    for (std::size_t i = 0; i + 1 < intersections.size(); i += 2) {
      int x0 = static_cast<int>(std::ceil(intersections[i]));
      int x1 = static_cast<int>(std::floor(intersections[i + 1]));
      x0 = std::max(x0, clip_rect.x);
      x1 = std::min(x1, clip_rect.x + clip_rect.width - 1);
      if (x0 > x1) {
        continue;
      }
      for (int x = x0; x <= x1; ++x) {
        if (opaque) {
          canvas.set_pixel(PointI{x, y}, command.color);
        } else {
          canvas.blend_pixel(PointI{x, y}, command.color, command.color.a);
        }
      }
    }
  }
}

void draw_markers(PixelCanvas& canvas,
                  const MarkerCloudCommand& command,
                  const std::optional<RectI>& clip) {
  const int radius = std::max(1, command.radius);
  for (const auto point : command.points) {
    draw_disc(canvas, point, radius, command.color, clip);
  }
}

} // namespace

bool SoftwareRenderer::render(const Scene& scene, PixelCanvas& canvas, std::vector<RenderEvent>& events) {
  if (canvas.empty()) {
    events.push_back(RenderEvent{RenderEventKind::Error, "canvas_not_initialized"});
    return false;
  }

  events.push_back(RenderEvent{RenderEventKind::FrameStarted, "software_render_start"});
  std::optional<RectI> clip{};
  std::vector<std::optional<RectI>> clip_stack;
  clip_stack.reserve(16);

  for (const auto& command : scene.commands()) {
    std::visit(
        [&canvas, &events, &clip, &clip_stack](const auto& cmd) {
          using CommandT = std::decay_t<decltype(cmd)>;
          if constexpr (std::is_same_v<CommandT, ClearCommand>) {
            canvas.clear(cmd.color);
          } else if constexpr (std::is_same_v<CommandT, FillRectCommand>) {
            fill_rect(canvas, cmd.rect, cmd.color, clip);
          } else if constexpr (std::is_same_v<CommandT, FillRoundedRectCommand>) {
            fill_rounded_rect(canvas, cmd.rect, cmd.radius, cmd.color, clip);
          } else if constexpr (std::is_same_v<CommandT, FillLinearGradientCommand>) {
            fill_linear_gradient(canvas, cmd, clip);
          } else if constexpr (std::is_same_v<CommandT, StrokeRectCommand>) {
            stroke_rect(canvas, cmd, clip);
          } else if constexpr (std::is_same_v<CommandT, StrokeRoundedRectCommand>) {
            stroke_rounded_rect(canvas, cmd.rect, cmd.radius, cmd.thickness, cmd.color, clip);
          } else if constexpr (std::is_same_v<CommandT, LineCommand>) {
            draw_line(canvas, cmd.start, cmd.end, clamp_thickness(cmd.thickness), cmd.color, clip);
          } else if constexpr (std::is_same_v<CommandT, PolylineCommand>) {
            draw_polyline(canvas, cmd, clip);
          } else if constexpr (std::is_same_v<CommandT, FillPolygonCommand>) {
            fill_polygon(canvas, cmd, clip);
          } else if constexpr (std::is_same_v<CommandT, MarkerCloudCommand>) {
            draw_markers(canvas, cmd, clip);
          } else if constexpr (std::is_same_v<CommandT, SetClipRectCommand>) {
            clip_stack.push_back(clip);
            std::optional<RectI> next = normalize_clip(canvas, std::optional<RectI>{cmd.rect});
            if (next.has_value() && clip.has_value()) {
              next = normalize_clip(canvas, std::optional<RectI>{intersect_rect(*next, *clip)});
            } else if (!next.has_value() && clip.has_value()) {
              next = std::nullopt;
            }
            clip = next;
          } else if constexpr (std::is_same_v<CommandT, ClearClipRectCommand>) {
            if (!clip_stack.empty()) {
              clip = clip_stack.back();
              clip_stack.pop_back();
            } else {
              clip.reset();
            }
          } else if constexpr (std::is_same_v<CommandT, TextCommand>) {
            detail::render_text_command(canvas, cmd, clip, events);
          }
        },
        command);
  }

  events.push_back(RenderEvent{RenderEventKind::FrameFinished, "software_render_end"});
  return true;
}

std::unique_ptr<IRenderer> make_software_renderer() {
  return std::make_unique<SoftwareRenderer>();
}

} // namespace pluss::view::window
