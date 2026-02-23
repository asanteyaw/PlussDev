#pragma once

#include "plot/modules/backend/pdf/primitives.h"

namespace pluss::plotism {
namespace detail {
inline std::string render_pdf_3d(const Figure3DView& fig, const RenderTarget& target) {
  if (target.width <= 100 || target.height <= 100) {
    throw std::invalid_argument("plotism::Figure3D::save_pdf: width/height too small");
  }

  PdfPainter painter(target.width, target.height);
  const PdfRgb black = pdf_parse_color("black");
  const PdfRgb white = pdf_parse_color("white");

  painter.fill_rect(0.0, 0.0, static_cast<double>(target.width), static_cast<double>(target.height), white);
  if (!fig.title.empty()) {
    painter.text_center(static_cast<double>(target.width) * 0.5, 24.0, 20.0, fig.title, black);
  }

  const int left = 90;
  const int right = target.width - 60;
  const int top = 70;
  const int bottom = target.height - 80;
  const int pw = std::max(1, right - left);
  const int ph = std::max(1, bottom - top);

  painter.stroke_rect(static_cast<double>(left), static_cast<double>(top), static_cast<double>(pw),
                      static_cast<double>(ph), pdf_parse_color("#d0d0d0"), 1.0);

  double xmin = std::numeric_limits<double>::infinity();
  double xmax = -std::numeric_limits<double>::infinity();
  double ymin = std::numeric_limits<double>::infinity();
  double ymax = -std::numeric_limits<double>::infinity();
  double zmin = std::numeric_limits<double>::infinity();
  double zmax = -std::numeric_limits<double>::infinity();
  for (const auto& t : fig.traces) {
    const std::size_t n = std::min(t.x.size(), std::min(t.y.size(), t.z.size()));
    for (std::size_t i = 0; i < n; ++i) {
      const double tx = pdf_axis_transform(t.x[i], fig.x_axis);
      const double ty = pdf_axis_transform(t.y[i], fig.y_axis);
      const double tz = pdf_axis_transform(t.z[i], fig.z_axis);
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

  if (fig.x_axis.min.has_value()) xmin = pdf_axis_transform(*fig.x_axis.min, fig.x_axis);
  if (fig.x_axis.max.has_value()) xmax = pdf_axis_transform(*fig.x_axis.max, fig.x_axis);
  if (fig.y_axis.min.has_value()) ymin = pdf_axis_transform(*fig.y_axis.min, fig.y_axis);
  if (fig.y_axis.max.has_value()) ymax = pdf_axis_transform(*fig.y_axis.max, fig.y_axis);
  if (fig.z_axis.min.has_value()) zmin = pdf_axis_transform(*fig.z_axis.min, fig.z_axis);
  if (fig.z_axis.max.has_value()) zmax = pdf_axis_transform(*fig.z_axis.max, fig.z_axis);

  std::tie(xmin, xmax) = pdf_ensure_limits(xmin, xmax);
  std::tie(ymin, ymax) = pdf_ensure_limits(ymin, ymax);
  std::tie(zmin, zmax) = pdf_ensure_limits(zmin, zmax);

  const auto project = [&](double x, double y, double z) {
    const double tx = pdf_axis_transform(x, fig.x_axis);
    const double ty = pdf_axis_transform(y, fig.y_axis);
    const double tz = pdf_axis_transform(z, fig.z_axis);
    if (!std::isfinite(tx) || !std::isfinite(ty) || !std::isfinite(tz)) {
      return std::pair<double, double>{std::numeric_limits<double>::quiet_NaN(),
                                       std::numeric_limits<double>::quiet_NaN()};
    }

    double xn = (tx - xmin) / (xmax - xmin);
    double yn = (ty - ymin) / (ymax - ymin);
    double zn = (tz - zmin) / (zmax - zmin);
    if (fig.x_axis.inverted) {
      xn = 1.0 - xn;
    }
    if (fig.y_axis.inverted) {
      yn = 1.0 - yn;
    }
    if (fig.z_axis.inverted) {
      zn = 1.0 - zn;
    }

    const double px = static_cast<double>(left) + (xn * 0.78 + zn * 0.22) * static_cast<double>(pw);
    const double py = static_cast<double>(top) + (1.0 - yn) * static_cast<double>(ph) - zn * 0.18 * static_cast<double>(ph);
    return std::pair<double, double>{px, py};
  };

  const auto& palette = detail::default_palette();
  for (std::size_t ti = 0; ti < fig.traces.size(); ++ti) {
    const auto& t = fig.traces[ti];
    const std::string color_hex = t.style.color.empty() ? palette[ti % palette.size()] : t.style.color;
    const PdfRgb color = pdf_parse_color(color_hex);

    if (t.kind == Trace3DKind::Line3) {
      std::vector<std::pair<double, double>> points;
      const std::size_t n = std::min(t.x.size(), std::min(t.y.size(), t.z.size()));
      points.reserve(n);
      for (std::size_t i = 0; i < n; ++i) {
        const auto [px, py] = project(t.x[i], t.y[i], t.z[i]);
        if (!std::isfinite(px) || !std::isfinite(py)) {
          continue;
        }
        points.push_back({px, py});
      }
      painter.stroke_polyline(points, color, t.style.line_width);
    } else {
      const double r = (t.kind == Trace3DKind::Surface) ? 1.6 : 2.6;
      const std::size_t n = std::min(t.x.size(), std::min(t.y.size(), t.z.size()));
      for (std::size_t i = 0; i < n; ++i) {
        const auto [px, py] = project(t.x[i], t.y[i], t.z[i]);
        if (!std::isfinite(px) || !std::isfinite(py)) {
          continue;
        }
        painter.fill_circle(px, py, r, color);
      }
    }
  }

  if (fig.legend.visible && !fig.traces.empty()) {
    const double legend_width = 124.0;
    const double legend_height = static_cast<double>(fig.traces.size() * 15 + 8);
    const auto [lx, ly] = pdf_map_legend_anchor(
        fig.legend.location,
        static_cast<double>(left),
        static_cast<double>(right),
        static_cast<double>(top),
        static_cast<double>(bottom),
        legend_width,
        legend_height);

    painter.fill_rect(lx, ly, legend_width, legend_height, white);
    painter.stroke_rect(lx, ly, legend_width, legend_height, pdf_parse_color("#aaaaaa"), 1.0);
    for (std::size_t ti = 0; ti < fig.traces.size(); ++ti) {
      const auto& t = fig.traces[ti];
      const std::string color_hex = t.style.color.empty() ? palette[ti % palette.size()] : t.style.color;
      const PdfRgb color = pdf_parse_color(color_hex);
      const double y = ly + 14 + static_cast<double>(ti) * 15.0;
      painter.stroke_line(lx + 6.0, y, lx + 20.0, y, color, 2.0);
      painter.text_left(lx + 24.0, y + 4.0, 11.0, t.name.empty() ? std::string("trace") : t.name, black);
    }
  }

  return build_pdf_document(target.width, target.height, painter.finish());
}

} // namespace detail
} // namespace pluss::plotism
