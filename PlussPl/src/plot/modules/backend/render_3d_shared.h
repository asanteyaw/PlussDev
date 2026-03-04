#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "plot/modules/artists/artist_2d.h"
#include "plot/modules/artists/artist_3d.h"
#include "plot/modules/backend/render_surface_2d.h"
#include "plot/modules/projector_3d.h"
#include "plot/modules/style_resolver.h"
#include "plot/modules/trace.h"

namespace pluss::plotism {
namespace detail {

struct SceneViewport3D {
  double left{0.0};
  double top{0.0};
  double width{0.0};
  double height{0.0};
};

struct SceneRender3DOptions {
  std::optional<SceneViewport3D> viewport{};
  bool clear_background{true};
  bool show_title{true};
  bool show_legend{true};
  bool show_colorbar{true};
  bool show_x_axis_label{true};
  bool show_y_axis_label{true};
  bool show_z_axis_label{true};
  int max_ticks_per_axis{0};
  double title_font_size{22.0};
  std::optional<std::string> title_override{};
};

struct Vector2D {
  double x{0.0};
  double y{0.0};
};

struct AxisEdgeSelection3D {
  AxisKind axis_kind{AxisKind::X};
  double fixed0{0.0};
  double fixed1{0.0};
  int corner0{0};
  int corner1{0};
};

inline Vector2D operator+(const Vector2D& lhs, const Vector2D& rhs) {
  return Vector2D{lhs.x + rhs.x, lhs.y + rhs.y};
}

inline Vector2D operator-(const Vector2D& lhs, const Vector2D& rhs) {
  return Vector2D{lhs.x - rhs.x, lhs.y - rhs.y};
}

inline Vector2D operator*(const Vector2D& lhs, double scale) {
  return Vector2D{lhs.x * scale, lhs.y * scale};
}

inline double length_2d(const Vector2D& v) {
  return std::sqrt(v.x * v.x + v.y * v.y);
}

inline Vector2D normalize_2d(const Vector2D& v) {
  const double len = length_2d(v);
  if (len <= 1e-9) {
    return Vector2D{0.0, -1.0};
  }
  return Vector2D{v.x / len, v.y / len};
}

inline Vector2D projected_delta_2d(const detail::ProjectedPoint3D& from, const detail::ProjectedPoint3D& to) {
  return Vector2D{to.x - from.x, to.y - from.y};
}

inline TextAnchor2D anchor_for_direction_3d(const Vector2D& direction) {
  if (direction.x >= 0.35) {
    return TextAnchor2D::Left;
  }
  if (direction.x <= -0.35) {
    return TextAnchor2D::Right;
  }
  return TextAnchor2D::Center;
}

inline double axis_display_coord_3d(double normalized, const Axis& axis) {
  return axis.inverted ? (1.0 - normalized) : normalized;
}

inline int corner_index_3d(double xb, double yb, double zb) {
  return (xb >= 0.5 ? 1 : 0) + (yb >= 0.5 ? 2 : 0) + (zb >= 0.5 ? 4 : 0);
}

inline std::array<double, 3> visible_face_bits_3d(const Camera3D& camera) {
  const auto direction = detail::camera_view_direction_3d(camera);
  return {
      direction[0] >= 0.0 ? 1.0 : 0.0,
      direction[1] >= 0.0 ? 1.0 : 0.0,
      direction[2] >= 0.0 ? 1.0 : 0.0,
  };
}

inline detail::ProjectedPoint3D axis_edge_point_3d(const PlotTransform3D& transform,
                                                   const AxisEdgeSelection3D& edge,
                                                   double normalized) {
  switch (edge.axis_kind) {
    case AxisKind::X:
      return transform.project_normalized(normalized, edge.fixed0, edge.fixed1);
    case AxisKind::Y:
      return transform.project_normalized(edge.fixed0, normalized, edge.fixed1);
    case AxisKind::Z:
      return transform.project_normalized(edge.fixed0, edge.fixed1, normalized);
    default:
      return transform.project_normalized(normalized, normalized, normalized);
  }
}

inline std::vector<AxisEdgeSelection3D> axis_edge_candidates_3d(AxisKind axis_kind) {
  std::vector<AxisEdgeSelection3D> candidates;
  candidates.reserve(4);
  for (int bit0 = 0; bit0 <= 1; ++bit0) {
    for (int bit1 = 0; bit1 <= 1; ++bit1) {
      const double fixed0 = static_cast<double>(bit0);
      const double fixed1 = static_cast<double>(bit1);
      switch (axis_kind) {
        case AxisKind::X:
          candidates.push_back(AxisEdgeSelection3D{
              .axis_kind = axis_kind,
              .fixed0 = fixed0,
              .fixed1 = fixed1,
              .corner0 = corner_index_3d(0.0, fixed0, fixed1),
              .corner1 = corner_index_3d(1.0, fixed0, fixed1),
          });
          break;
        case AxisKind::Y:
          candidates.push_back(AxisEdgeSelection3D{
              .axis_kind = axis_kind,
              .fixed0 = fixed0,
              .fixed1 = fixed1,
              .corner0 = corner_index_3d(fixed0, 0.0, fixed1),
              .corner1 = corner_index_3d(fixed0, 1.0, fixed1),
          });
          break;
        case AxisKind::Z:
          candidates.push_back(AxisEdgeSelection3D{
              .axis_kind = axis_kind,
              .fixed0 = fixed0,
              .fixed1 = fixed1,
              .corner0 = corner_index_3d(fixed0, fixed1, 0.0),
              .corner1 = corner_index_3d(fixed0, fixed1, 1.0),
          });
          break;
        default:
          break;
      }
    }
  }
  return candidates;
}

inline int edge_visible_face_count_3d(const AxisEdgeSelection3D& edge, const std::array<double, 3>& visible_faces) {
  switch (edge.axis_kind) {
    case AxisKind::X:
      return static_cast<int>(std::abs(edge.fixed0 - visible_faces[1]) < 1e-9) +
             static_cast<int>(std::abs(edge.fixed1 - visible_faces[2]) < 1e-9);
    case AxisKind::Y:
      return static_cast<int>(std::abs(edge.fixed0 - visible_faces[0]) < 1e-9) +
             static_cast<int>(std::abs(edge.fixed1 - visible_faces[2]) < 1e-9);
    case AxisKind::Z:
      return static_cast<int>(std::abs(edge.fixed0 - visible_faces[0]) < 1e-9) +
             static_cast<int>(std::abs(edge.fixed1 - visible_faces[1]) < 1e-9);
    default:
      return 0;
  }
}

inline AxisEdgeSelection3D choose_axis_edge_3d(const PlotTransform3D& transform,
                                               AxisKind axis_kind,
                                               const std::array<double, 3>& visible_faces,
                                               const detail::ProjectedPoint3D& plot_center) {
  AxisEdgeSelection3D best{};
  bool have_best = false;
  int best_face_hits = -1;
  double best_radial = -std::numeric_limits<double>::infinity();
  double best_depth = -std::numeric_limits<double>::infinity();
  for (const auto& candidate : axis_edge_candidates_3d(axis_kind)) {
    const auto midpoint = axis_edge_point_3d(transform, candidate, 0.5);
    const int face_hits = edge_visible_face_count_3d(candidate, visible_faces);
    const double radial = length_2d(projected_delta_2d(plot_center, midpoint));
    if (!have_best || face_hits > best_face_hits ||
        (face_hits == best_face_hits && radial > best_radial + 1e-9) ||
        (face_hits == best_face_hits && std::abs(radial - best_radial) <= 1e-9 && midpoint.depth > best_depth)) {
      best = candidate;
      best_face_hits = face_hits;
      best_radial = radial;
      best_depth = midpoint.depth;
      have_best = true;
    }
  }
  return best;
}

inline bool matches_axis_edge_3d(const std::pair<int, int>& edge, const AxisEdgeSelection3D& selected) {
  return (edge.first == selected.corner0 && edge.second == selected.corner1) ||
         (edge.first == selected.corner1 && edge.second == selected.corner0);
}

inline Vector2D axis_tick_direction_3d(const PlotTransform3D& transform,
                                       const AxisEdgeSelection3D& edge,
                                       double normalized,
                                       const detail::ProjectedPoint3D& plot_center) {
  const auto point = axis_edge_point_3d(transform, edge, normalized);
  Vector2D inward{};
  switch (edge.axis_kind) {
    case AxisKind::X: {
      const auto py = transform.project_normalized(normalized, 1.0 - edge.fixed0, edge.fixed1);
      const auto pz = transform.project_normalized(normalized, edge.fixed0, 1.0 - edge.fixed1);
      inward = projected_delta_2d(point, py) + projected_delta_2d(point, pz);
      break;
    }
    case AxisKind::Y: {
      const auto px = transform.project_normalized(1.0 - edge.fixed0, normalized, edge.fixed1);
      const auto pz = transform.project_normalized(edge.fixed0, normalized, 1.0 - edge.fixed1);
      inward = projected_delta_2d(point, px) + projected_delta_2d(point, pz);
      break;
    }
    case AxisKind::Z: {
      const auto px = transform.project_normalized(1.0 - edge.fixed0, edge.fixed1, normalized);
      const auto py = transform.project_normalized(edge.fixed0, 1.0 - edge.fixed1, normalized);
      inward = projected_delta_2d(point, px) + projected_delta_2d(point, py);
      break;
    }
    default:
      break;
  }
  Vector2D outward = normalize_2d(inward * -1.0);
  if (length_2d(outward) <= 1e-9) {
    outward = normalize_2d(projected_delta_2d(plot_center, point));
  }
  return outward;
}

inline std::vector<std::array<Point2D, 4>> visible_panes_3d(const PlotTransform3D& transform,
                                                            const std::array<double, 3>& visible_faces) {
  return {
      {{
          {transform.project_normalized(visible_faces[0], 0.0, 0.0).x, transform.project_normalized(visible_faces[0], 0.0, 0.0).y},
          {transform.project_normalized(visible_faces[0], 1.0, 0.0).x, transform.project_normalized(visible_faces[0], 1.0, 0.0).y},
          {transform.project_normalized(visible_faces[0], 1.0, 1.0).x, transform.project_normalized(visible_faces[0], 1.0, 1.0).y},
          {transform.project_normalized(visible_faces[0], 0.0, 1.0).x, transform.project_normalized(visible_faces[0], 0.0, 1.0).y},
      }},
      {{
          {transform.project_normalized(0.0, visible_faces[1], 0.0).x, transform.project_normalized(0.0, visible_faces[1], 0.0).y},
          {transform.project_normalized(1.0, visible_faces[1], 0.0).x, transform.project_normalized(1.0, visible_faces[1], 0.0).y},
          {transform.project_normalized(1.0, visible_faces[1], 1.0).x, transform.project_normalized(1.0, visible_faces[1], 1.0).y},
          {transform.project_normalized(0.0, visible_faces[1], 1.0).x, transform.project_normalized(0.0, visible_faces[1], 1.0).y},
      }},
      {{
          {transform.project_normalized(0.0, 0.0, visible_faces[2]).x, transform.project_normalized(0.0, 0.0, visible_faces[2]).y},
          {transform.project_normalized(1.0, 0.0, visible_faces[2]).x, transform.project_normalized(1.0, 0.0, visible_faces[2]).y},
          {transform.project_normalized(1.0, 1.0, visible_faces[2]).x, transform.project_normalized(1.0, 1.0, visible_faces[2]).y},
          {transform.project_normalized(0.0, 1.0, visible_faces[2]).x, transform.project_normalized(0.0, 1.0, visible_faces[2]).y},
      }},
  };
}

inline std::vector<ResolvedTick> limit_ticks_3d(std::vector<ResolvedTick> ticks, int max_ticks) {
  if (max_ticks <= 0 || ticks.size() <= static_cast<std::size_t>(max_ticks)) {
    return ticks;
  }
  if (max_ticks == 1) {
    return {ticks[ticks.size() / 2]};
  }

  std::vector<ResolvedTick> trimmed;
  trimmed.reserve(static_cast<std::size_t>(max_ticks));
  const double stride = static_cast<double>(ticks.size() - 1) / static_cast<double>(max_ticks - 1);
  std::size_t last_index = std::numeric_limits<std::size_t>::max();
  for (int i = 0; i < max_ticks; ++i) {
    std::size_t index = static_cast<std::size_t>(std::llround(static_cast<double>(i) * stride));
    index = std::min(index, ticks.size() - 1);
    if (index == last_index) {
      continue;
    }
    trimmed.push_back(ticks[index]);
    last_index = index;
  }
  if (trimmed.back().position != ticks.back().position || trimmed.back().label != ticks.back().label) {
    trimmed.back() = ticks.back();
  }
  return trimmed;
}

inline double estimate_text_width_3d(std::string_view text, double font_size) {
  double units = 0.0;
  for (const char c : text) {
    if (c >= '0' && c <= '9') {
      units += 0.62;
    } else if (c == '.' || c == ',' || c == ':' || c == ';') {
      units += 0.32;
    } else if (c == '-' || c == '+' || c == '/' || c == '\\') {
      units += 0.42;
    } else if (c == 'i' || c == 'l' || c == 'I' || c == '|' || c == ' ') {
      units += 0.36;
    } else if (c >= 'A' && c <= 'Z') {
      units += 0.74;
    } else {
      units += 0.58;
    }
  }
  return units * font_size;
}

inline double estimate_text_height_3d(double font_size) {
  return font_size * 1.2;
}

inline Vector2D axis_tangent_3d(const PlotTransform3D& transform, const AxisEdgeSelection3D& edge) {
  const auto start = axis_edge_point_3d(transform, edge, 0.0);
  const auto end = axis_edge_point_3d(transform, edge, 1.0);
  return normalize_2d(projected_delta_2d(start, end));
}

inline double axis_projected_length_3d(const PlotTransform3D& transform, const AxisEdgeSelection3D& edge) {
  const auto start = axis_edge_point_3d(transform, edge, 0.0);
  const auto end = axis_edge_point_3d(transform, edge, 1.0);
  return length_2d(projected_delta_2d(start, end));
}

inline double tick_label_rotation_3d(const Vector2D& tangent,
                                     double axis_length,
                                     const std::vector<ResolvedTick>& ticks,
                                     double font_size) {
  if (ticks.size() < 2) {
    return 0.0;
  }
  double widest = 0.0;
  for (const auto& tick : ticks) {
    widest = std::max(widest, estimate_text_width_3d(tick.label, font_size));
  }
  const double density = widest * static_cast<double>(ticks.size()) / std::max(1.0, axis_length);
  double angle = std::atan2(tangent.y, tangent.x) * 180.0 / 3.14159265358979323846;
  while (angle > 90.0) angle -= 180.0;
  while (angle < -90.0) angle += 180.0;
  if (density < 0.68 && std::abs(angle) < 24.0) {
    return 0.0;
  }
  if (std::abs(angle) > 82.0) {
    return angle > 0.0 ? 90.0 : -90.0;
  }
  if (std::abs(angle) < 12.0) {
    return 0.0;
  }
  return std::clamp(angle, -72.0, 72.0);
}

inline double tick_label_axis_extent_3d(std::string_view text,
                                        double font_size,
                                        double rotation_deg,
                                        const Vector2D& tangent) {
  const double width = estimate_text_width_3d(text, font_size);
  const double height = estimate_text_height_3d(font_size);
  const double label_angle = rotation_deg * 3.14159265358979323846 / 180.0;
  const double axis_angle = std::atan2(tangent.y, tangent.x);
  const double delta = label_angle - axis_angle;
  return 0.5 * (std::abs(width * std::cos(delta)) + std::abs(height * std::sin(delta)));
}

inline double adaptive_tick_font_size_3d(double axis_length, double panel_width, double panel_height, bool constrained_view) {
  const double panel_scale = std::sqrt(std::max(1.0, std::min(panel_width, panel_height)));
  const double axis_scale = std::sqrt(std::max(1.0, axis_length));
  const double base = 0.16 * panel_scale + 0.10 * axis_scale;
  const double floor = constrained_view ? 7.5 : 8.5;
  const double ceiling = constrained_view ? 10.5 : 11.5;
  return std::clamp(base, floor, ceiling);
}

inline int adaptive_tick_budget_3d(const std::vector<ResolvedTick>& ticks,
                                   double axis_length,
                                   const Vector2D& tangent,
                                   double font_size,
                                   int max_ticks_override) {
  if (ticks.size() <= 2) {
    return static_cast<int>(ticks.size());
  }

  double max_label_width = 0.0;
  for (const auto& tick : ticks) {
    max_label_width = std::max(max_label_width, estimate_text_width_3d(tick.label, font_size));
  }

  const double horizontal_weight = std::abs(tangent.x);
  const double vertical_weight = std::abs(tangent.y);
  const double label_footprint =
      max_label_width * horizontal_weight + font_size * (1.8 + 1.4 * vertical_weight) + font_size * 0.9;
  const int computed = std::max(2, static_cast<int>(std::floor(axis_length / std::max(font_size * 2.0, label_footprint))));
  const int available = static_cast<int>(ticks.size());
  const int capped = max_ticks_override > 0 ? std::min(computed, max_ticks_override) : computed;
  return std::clamp(capped, 2, available);
}

inline void render_3d_scene_with_options(RenderSurface2D& surface,
                                         const Figure3DView& fig,
                                         const RenderTarget& target,
                                         const SceneRender3DOptions& options) {
  if (target.width <= 100 || target.height <= 100) {
    throw std::invalid_argument("plotism::Figure3D::save: width/height too small");
  }

  const ResolvedTheme theme = detail::resolve_theme(fig.style);
  const SceneViewport3D viewport = options.viewport.value_or(SceneViewport3D{
      .left = 0.0,
      .top = 0.0,
      .width = static_cast<double>(target.width),
      .height = static_cast<double>(target.height),
  });
  const bool constrained_view = options.viewport.has_value();

  if (options.clear_background) {
    surface.fill_rect(0.0, 0.0, static_cast<double>(target.width), static_cast<double>(target.height),
                      FillStyle2D{.color = theme.page_background, .alpha = 1.0});
  }

  const std::string title = options.title_override.value_or(fig.title);
  if (options.show_title && !title.empty()) {
    surface.text(viewport.left + viewport.width * 0.5,
                 viewport.top + std::clamp(viewport.height * 0.065, 20.0, 30.0),
                 title,
                 TextStyle2D{
                     .color = theme.title_color,
                     .size = options.title_font_size,
                     .alpha = 1.0,
                     .bold = true,
                     .anchor = TextAnchor2D::Center,
                     .orientation = TextOrientation2D::Horizontal,
                     .font_family = theme.font_family,
                 });
  }

  double legend_width = 0.0;
  double legend_height = 0.0;
  if (options.show_legend && fig.legend.visible && !fig.traces.empty()) {
    std::size_t longest = fig.legend.title.size();
    for (const auto& trace : fig.traces) {
      longest = std::max(longest, trace.name.empty() ? std::size_t{5} : trace.name.size());
    }
    const bool has_title = !fig.legend.title.empty();
    legend_width = legend_width_hint_2d(fig.traces.size(), longest, has_title);
    legend_height = legend_height_hint_2d(fig.traces.size(), has_title);
  }
  const bool outside_legend = options.show_legend && fig.legend.location == LegendLocation::OutsideRight && legend_width > 0.0;

  const Trace3D* surface_for_colorbar = nullptr;
  if (options.show_colorbar && fig.colorbar.has_value() && fig.colorbar->visible) {
    auto it = std::find_if(fig.traces.begin(), fig.traces.end(), [](const Trace3D& trace) {
      return trace.kind == Trace3DKind::Surface && !trace.z.empty();
    });
    if (it != fig.traces.end()) {
      surface_for_colorbar = &(*it);
    }
  }
  const bool show_colorbar = surface_for_colorbar != nullptr;
  const double colorbar_width = show_colorbar ? std::clamp(viewport.width * 0.06, 44.0, 64.0) : 0.0;
  const double aux_gap = (show_colorbar && outside_legend) ? 12.0 : 0.0;

  const double left_margin = constrained_view ? std::clamp(viewport.width * 0.145, 64.0, 108.0)
                                              : std::clamp(viewport.width * 0.14, 56.0, 110.0);
  const double right_margin = constrained_view ? std::clamp(viewport.width * 0.085, 36.0, 72.0)
                                               : std::clamp(viewport.width * 0.08, 44.0, 90.0);
  const double top_margin = constrained_view ? std::clamp(viewport.height * (options.show_title ? 0.145 : 0.08), 34.0, 70.0)
                                             : std::clamp(viewport.height * (options.show_title ? 0.16 : 0.08), 40.0, 84.0);
  const double bottom_margin = constrained_view ? std::clamp(viewport.height * 0.095, 42.0, 78.0)
                                                : std::clamp(viewport.height * 0.10, 52.0, 96.0);

  const double left = viewport.left + left_margin;
  const double right = viewport.left + viewport.width - right_margin - (outside_legend ? (legend_width + 16.0) : 0.0) -
                       (show_colorbar ? (colorbar_width + 16.0 + aux_gap) : 0.0);
  const double top = viewport.top + top_margin;
  const double bottom = viewport.top + viewport.height - bottom_margin;
  const double pw = std::max(1.0, right - left);
  const double ph = std::max(1.0, bottom - top);

  const double panel_radius = constrained_view ? 10.0 : 14.0;
  surface.fill_rect(left, top, pw, ph, FillStyle2D{.color = theme.panel_background, .alpha = 1.0}, panel_radius);
  surface.stroke_rect(left, top, pw, ph,
                      StrokeStyle2D{
                          .color = theme.panel_border,
                          .line_width = 1.0,
                          .alpha = 1.0,
                          .line_style = "solid",
                      },
                      panel_radius);

  double xmin = std::numeric_limits<double>::infinity();
  double xmax = -std::numeric_limits<double>::infinity();
  double ymin = std::numeric_limits<double>::infinity();
  double ymax = -std::numeric_limits<double>::infinity();
  double zmin = std::numeric_limits<double>::infinity();
  double zmax = -std::numeric_limits<double>::infinity();
  for (const auto& t : fig.traces) {
    const std::size_t n = std::min(t.x.size(), std::min(t.y.size(), t.z.size()));
    for (std::size_t i = 0; i < n; ++i) {
      const double tx = axis_transform_2d(t.x[i], fig.x_axis);
      const double ty = axis_transform_2d(t.y[i], fig.y_axis);
      const double tz = axis_transform_2d(t.z[i], fig.z_axis);
      if (std::isfinite(tx)) {
        xmin = std::min(xmin, tx);
        xmax = std::max(xmax, tx);
      }
      if (std::isfinite(ty)) {
        ymin = std::min(ymin, ty);
        ymax = std::max(ymax, ty);
      }
      if (std::isfinite(tz)) {
        zmin = std::min(zmin, tz);
        zmax = std::max(zmax, tz);
      }
    }
  }

  if (fig.x_axis.min.has_value()) xmin = axis_transform_2d(*fig.x_axis.min, fig.x_axis);
  if (fig.x_axis.max.has_value()) xmax = axis_transform_2d(*fig.x_axis.max, fig.x_axis);
  if (fig.y_axis.min.has_value()) ymin = axis_transform_2d(*fig.y_axis.min, fig.y_axis);
  if (fig.y_axis.max.has_value()) ymax = axis_transform_2d(*fig.y_axis.max, fig.y_axis);
  if (fig.z_axis.min.has_value()) zmin = axis_transform_2d(*fig.z_axis.min, fig.z_axis);
  if (fig.z_axis.max.has_value()) zmax = axis_transform_2d(*fig.z_axis.max, fig.z_axis);

  std::tie(xmin, xmax) = ensure_limits_2d(xmin, xmax);
  std::tie(ymin, ymax) = ensure_limits_2d(ymin, ymax);
  std::tie(zmin, zmax) = ensure_limits_2d(zmin, zmax);

  const PlotTransform3D transform{
      .xmin = xmin,
      .xmax = xmax,
      .ymin = ymin,
      .ymax = ymax,
      .zmin = zmin,
      .zmax = zmax,
      .left = left,
      .top = top,
      .width = pw,
      .height = ph,
      .projection_extent = detail::fitted_projection_extent_3d(fig.camera, fig.box_aspect),
      .box_aspect = fig.box_aspect,
      .x_axis = &fig.x_axis,
      .y_axis = &fig.y_axis,
      .z_axis = &fig.z_axis,
      .camera = &fig.camera,
  };

  const std::array<std::array<double, 3>, 8> corners{{
      {0.0, 0.0, 0.0},
      {1.0, 0.0, 0.0},
      {1.0, 1.0, 0.0},
      {0.0, 1.0, 0.0},
      {0.0, 0.0, 1.0},
      {1.0, 0.0, 1.0},
      {1.0, 1.0, 1.0},
      {0.0, 1.0, 1.0},
  }};
  const std::array<std::pair<int, int>, 12> edges{{
      {0, 1}, {1, 2}, {2, 3}, {3, 0},
      {4, 5}, {5, 6}, {6, 7}, {7, 4},
      {0, 4}, {1, 5}, {2, 6}, {3, 7},
  }};

  std::array<detail::ProjectedPoint3D, 8> projected_corners{};
  for (std::size_t i = 0; i < corners.size(); ++i) {
    projected_corners[i] = transform.project_normalized(corners[i][0], corners[i][1], corners[i][2]);
  }
  const auto plot_center = transform.project_normalized(0.5, 0.5, 0.5);
  const auto visible_faces = visible_face_bits_3d(fig.camera);
  const AxisEdgeSelection3D x_axis_edge = choose_axis_edge_3d(transform, AxisKind::X, visible_faces, plot_center);
  const AxisEdgeSelection3D y_axis_edge = choose_axis_edge_3d(transform, AxisKind::Y, visible_faces, plot_center);
  const AxisEdgeSelection3D z_axis_edge = choose_axis_edge_3d(transform, AxisKind::Z, visible_faces, plot_center);

  struct Pane3D {
    double depth{0.0};
    std::array<Point2D, 4> polygon{};
  };
  std::vector<Pane3D> panes;
  for (const auto& pane : visible_panes_3d(transform, visible_faces)) {
    double depth = 0.0;
    for (const auto& point : pane) {
      depth += point.y;
    }
    panes.push_back(Pane3D{.depth = depth, .polygon = pane});
  }
  surface.push_clip_rect(left, top, pw, ph);
  std::sort(panes.begin(), panes.end(), [](const Pane3D& lhs, const Pane3D& rhs) { return lhs.depth < rhs.depth; });
  for (const auto& pane : panes) {
    surface.fill_polygon(std::vector<Point2D>(pane.polygon.begin(), pane.polygon.end()),
                         FillStyle2D{.color = theme.plot_background, .alpha = 0.72});
  }

  for (const auto& edge : edges) {
    const auto& p1 = projected_corners[static_cast<std::size_t>(edge.first)];
    const auto& p2 = projected_corners[static_cast<std::size_t>(edge.second)];
    const bool primary = matches_axis_edge_3d(edge, x_axis_edge) ||
                         matches_axis_edge_3d(edge, y_axis_edge) ||
                         matches_axis_edge_3d(edge, z_axis_edge);
    surface.stroke_line(p1.x, p1.y, p2.x, p2.y,
                        StrokeStyle2D{
                            .color = primary ? theme.axis_color : theme.grid_color,
                            .line_width = primary ? 1.8 : 1.0,
                            .alpha = primary ? 1.0 : 0.95,
                            .line_style = "solid",
                        });
  }

  const double x_axis_length = axis_projected_length_3d(transform, x_axis_edge);
  const double y_axis_length = axis_projected_length_3d(transform, y_axis_edge);
  const double z_axis_length = axis_projected_length_3d(transform, z_axis_edge);
  const Vector2D x_tangent = axis_tangent_3d(transform, x_axis_edge);
  const Vector2D y_tangent = axis_tangent_3d(transform, y_axis_edge);
  const Vector2D z_tangent = axis_tangent_3d(transform, z_axis_edge);
  const double x_tick_font = adaptive_tick_font_size_3d(x_axis_length, pw, ph, constrained_view);
  const double y_tick_font = adaptive_tick_font_size_3d(y_axis_length, pw, ph, constrained_view);
  const double z_tick_font = adaptive_tick_font_size_3d(z_axis_length, pw, ph, constrained_view);
  const auto x_ticks = limit_ticks_3d(resolve_ticks_2d(fig.x_axis, xmin, xmax),
                                      adaptive_tick_budget_3d(resolve_ticks_2d(fig.x_axis, xmin, xmax),
                                                              x_axis_length,
                                                              x_tangent,
                                                              x_tick_font,
                                                              options.max_ticks_per_axis));
  const auto y_ticks = limit_ticks_3d(resolve_ticks_2d(fig.y_axis, ymin, ymax),
                                      adaptive_tick_budget_3d(resolve_ticks_2d(fig.y_axis, ymin, ymax),
                                                              y_axis_length,
                                                              y_tangent,
                                                              y_tick_font,
                                                              options.max_ticks_per_axis));
  const auto z_ticks = limit_ticks_3d(resolve_ticks_2d(fig.z_axis, zmin, zmax),
                                      adaptive_tick_budget_3d(resolve_ticks_2d(fig.z_axis, zmin, zmax),
                                                              z_axis_length,
                                                              z_tangent,
                                                              z_tick_font,
                                                              options.max_ticks_per_axis));
  const auto clamp_t = [](double value) {
    if (!std::isfinite(value)) {
      return 0.5;
    }
    return std::clamp(value, 0.0, 1.0);
  };
  const auto should_draw_inner_grid = [](double t) {
    return t > 1e-6 && t < 1.0 - 1e-6;
  };
  const auto stroke_gridline = [&](const detail::ProjectedPoint3D& p1, const detail::ProjectedPoint3D& p2) {
    surface.stroke_line(p1.x, p1.y, p2.x, p2.y,
                        StrokeStyle2D{
                            .color = theme.grid_color,
                            .line_width = 0.9,
                            .alpha = 0.78,
                            .line_style = "solid",
                        });
  };

  for (const auto& tick : x_ticks) {
    const double t = axis_display_coord_3d(clamp_t((tick.position - xmin) / std::max(1e-12, xmax - xmin)), fig.x_axis);
    if (!should_draw_inner_grid(t)) {
      continue;
    }
    stroke_gridline(transform.project_normalized(t, 0.0, visible_faces[2]),
                    transform.project_normalized(t, 1.0, visible_faces[2]));
    stroke_gridline(transform.project_normalized(t, visible_faces[1], 0.0),
                    transform.project_normalized(t, visible_faces[1], 1.0));
  }
  for (const auto& tick : y_ticks) {
    const double t = axis_display_coord_3d(clamp_t((tick.position - ymin) / std::max(1e-12, ymax - ymin)), fig.y_axis);
    if (!should_draw_inner_grid(t)) {
      continue;
    }
    stroke_gridline(transform.project_normalized(0.0, t, visible_faces[2]),
                    transform.project_normalized(1.0, t, visible_faces[2]));
    stroke_gridline(transform.project_normalized(visible_faces[0], t, 0.0),
                    transform.project_normalized(visible_faces[0], t, 1.0));
  }
  for (const auto& tick : z_ticks) {
    const double t = axis_display_coord_3d(clamp_t((tick.position - zmin) / std::max(1e-12, zmax - zmin)), fig.z_axis);
    if (!should_draw_inner_grid(t)) {
      continue;
    }
    stroke_gridline(transform.project_normalized(visible_faces[0], 0.0, t),
                    transform.project_normalized(visible_faces[0], 1.0, t));
    stroke_gridline(transform.project_normalized(0.0, visible_faces[1], t),
                    transform.project_normalized(1.0, visible_faces[1], t));
  }

  DepthSorter3D depth_sorter{};
  for (std::size_t ti = 0; ti < fig.traces.size(); ++ti) {
    Trace3D styled_trace = fig.traces[ti];
    detail::apply_trace_style_defaults(styled_trace, theme, ti);
    artist_for_trace_3d(styled_trace.kind).emit(depth_sorter, styled_trace, transform, theme);
  }
  depth_sorter.paint(surface);
  surface.pop_clip();

  const auto draw_ticks = [&](const Axis& axis,
                              const std::vector<ResolvedTick>& ticks,
                              const AxisEdgeSelection3D& edge,
                              const Vector2D& tangent,
                              double axis_length,
                              double tick_font_size,
                              double lo,
                              double hi) {
    const double span = std::max(1e-12, hi - lo);
    const double tick_len = std::clamp(0.055 * tick_font_size + 0.012 * axis_length, 0.65 * tick_font_size, 1.15 * tick_font_size);
    const double label_len = tick_len + 0.95 * tick_font_size;
    const double rotation_deg = tick_label_rotation_3d(tangent, axis_length, ticks, tick_font_size);
    const double tangent_norm = std::max(1e-9, length_2d(tangent));
    double prev_projection = -std::numeric_limits<double>::infinity();
    double prev_half_extent = 0.0;
    for (const auto& tick : ticks) {
      const double base_t = clamp_t((tick.position - lo) / span);
      const double axis_t = axis_display_coord_3d(base_t, axis);
      const auto point = axis_edge_point_3d(transform, edge, axis_t);
      const auto direction = axis_tick_direction_3d(transform, edge, axis_t, plot_center);
      surface.stroke_line(point.x, point.y, point.x + direction.x * tick_len, point.y + direction.y * tick_len,
                          StrokeStyle2D{
                              .color = theme.tick_color,
                              .line_width = 1.0,
                              .alpha = 1.0,
                              .line_style = "solid",
                          });
      const double text_x = point.x + direction.x * label_len;
      const double text_y = point.y + direction.y * label_len + 3.0;
      const double axis_projection = (text_x * tangent.x + text_y * tangent.y) / tangent_norm;
      const double half_extent = tick_label_axis_extent_3d(tick.label, tick_font_size, rotation_deg, tangent) + 2.0;
      const bool collides = std::isfinite(prev_projection) && axis_projection - prev_projection < (prev_half_extent + half_extent);
      if (collides && base_t > 1e-6 && base_t < 1.0 - 1e-6) {
        continue;
      }
      prev_projection = axis_projection;
      prev_half_extent = half_extent;
      const auto text_anchor = std::abs(rotation_deg) > 1e-9 ? TextAnchor2D::Center : anchor_for_direction_3d(direction);
      surface.text(text_x,
                   text_y,
                   tick.label,
                   TextStyle2D{
                       .color = theme.tick_label_color,
                       .size = tick_font_size,
                       .alpha = 1.0,
                       .bold = false,
                       .anchor = text_anchor,
                       .orientation = TextOrientation2D::Horizontal,
                       .rotation_deg = rotation_deg,
                       .font_family = theme.font_family,
                   });
    }
  };

  draw_ticks(fig.x_axis, x_ticks, x_axis_edge, x_tangent, x_axis_length, x_tick_font, xmin, xmax);
  draw_ticks(fig.y_axis, y_ticks, y_axis_edge, y_tangent, y_axis_length, y_tick_font, ymin, ymax);
  draw_ticks(fig.z_axis, z_ticks, z_axis_edge, z_tangent, z_axis_length, z_tick_font, zmin, zmax);

  const auto draw_axis_label = [&](const Axis& axis,
                                   const AxisEdgeSelection3D& edge,
                                   bool visible,
                                   double axis_length,
                                   double tick_font_size) {
    if (!visible) {
      return;
    }
    if (axis.label.empty()) {
      return;
    }
    const double axis_label_size = std::clamp(tick_font_size + 1.2, tick_font_size + 0.6, tick_font_size + 2.0);
    const double axis_label_offset = 1.3 * tick_font_size + std::clamp(0.025 * axis_length, 4.0, 12.0);
    const double label_t = axis.inverted ? 0.0 : 1.0;
    const auto point = axis_edge_point_3d(transform, edge, label_t);
    const auto direction = axis_tick_direction_3d(transform, edge, label_t, plot_center);
    surface.text(point.x + direction.x * axis_label_offset,
                 point.y + direction.y * axis_label_offset,
                 axis.label,
                 TextStyle2D{
                     .color = theme.axis_label_color,
                     .size = axis_label_size,
                     .alpha = 1.0,
                     .bold = false,
                     .anchor = anchor_for_direction_3d(direction),
                     .orientation = TextOrientation2D::Horizontal,
                     .font_family = theme.font_family,
                 });
  };

  draw_axis_label(fig.x_axis, x_axis_edge, options.show_x_axis_label, x_axis_length, x_tick_font);
  draw_axis_label(fig.y_axis, y_axis_edge, options.show_y_axis_label, y_axis_length, y_tick_font);
  draw_axis_label(fig.z_axis, z_axis_edge, options.show_z_axis_label, z_axis_length, z_tick_font);

  if (show_colorbar) {
    Trace2D scalar_trace;
    scalar_trace.kind = Trace2DKind::Heatmap;
    scalar_trace.name = surface_for_colorbar->name;
    scalar_trace.values = surface_for_colorbar->z;
    scalar_trace.style = surface_for_colorbar->style;

    Colorbar colorbar = *fig.colorbar;
    if (colorbar.title.empty()) {
      colorbar.title = !surface_for_colorbar->name.empty() ? surface_for_colorbar->name : fig.z_axis.label;
    }

    const double colorbar_x = right + 14.0;
    const double colorbar_y = top + 6.0;
    const double colorbar_h = std::max(60.0, ph - 12.0);
    const ColorbarArtist2D colorbar_artist{};
    colorbar_artist.draw(surface, colorbar_x, colorbar_y, colorbar_width, colorbar_h, scalar_trace, colorbar, theme);
  }

  if (options.show_legend && fig.legend.visible && !fig.traces.empty()) {
    const bool has_title = !fig.legend.title.empty();
    const auto [lx, ly] = outside_legend
                              ? std::pair<double, double>{right + 14.0 + (show_colorbar ? (colorbar_width + 16.0 + aux_gap) : 0.0), top + 4.0}
                              : map_legend_anchor_2d(fig.legend.location, left, right, top, bottom, legend_width, legend_height);

    surface.fill_rect(lx, ly, legend_width, legend_height,
                      FillStyle2D{.color = theme.legend_background, .alpha = 0.92},
                      8.0);
    surface.stroke_rect(lx, ly, legend_width, legend_height,
                        StrokeStyle2D{
                            .color = theme.legend_border,
                            .line_width = 1.0,
                            .alpha = 1.0,
                            .line_style = "solid",
                        },
                        8.0);
    double y = ly + 14.0;
    if (has_title) {
      surface.text(lx + 10.0, y, fig.legend.title,
                   TextStyle2D{
                       .color = theme.legend_title_color,
                       .size = 11.0,
                       .alpha = 1.0,
                       .bold = true,
                       .anchor = TextAnchor2D::Left,
                       .orientation = TextOrientation2D::Horizontal,
                       .font_family = theme.font_family,
                   });
      y += 14.0;
    }
    const LegendArtist3D legend_artist{};
    for (std::size_t ti = 0; ti < fig.traces.size(); ++ti) {
      Trace3D styled_trace = fig.traces[ti];
      detail::apply_trace_style_defaults(styled_trace, theme, ti);
      const double sample_y = y + static_cast<double>(ti) * 16.0;
      legend_artist.draw_sample(surface, styled_trace, lx + 8.0, lx + 26.0, sample_y, theme);
      surface.text(lx + 32.0, sample_y + 4.0, styled_trace.name.empty() ? std::string("trace") : styled_trace.name,
                   TextStyle2D{
                       .color = theme.legend_text_color,
                       .size = 11.0,
                       .alpha = 1.0,
                       .bold = false,
                       .anchor = TextAnchor2D::Left,
                       .orientation = TextOrientation2D::Horizontal,
                       .font_family = theme.font_family,
                   });
    }
  }
}

inline void render_3d_scene(RenderSurface2D& surface, const Figure3DView& fig, const RenderTarget& target) {
  render_3d_scene_with_options(surface, fig, target, SceneRender3DOptions{});
}

} // namespace detail
} // namespace pluss::plotism
