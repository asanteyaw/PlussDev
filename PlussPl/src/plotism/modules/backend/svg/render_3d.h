#pragma once

#include "pluss/plotism/modules/backend/svg/primitives.h"

namespace pluss::plotism {
namespace detail {
inline std::string render_svg_3d(const Figure3DView& fig, const RenderTarget& target) {
  if (target.width <= 100 || target.height <= 100) {
    throw std::invalid_argument("plotism::Figure3D::save_svg: width/height too small");
  }

  std::ostringstream svg;
  svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << target.width << "\" height=\""
      << target.height << "\" viewBox=\"0 0 " << target.width << " " << target.height << "\">\n";
  svg << "<rect x=\"0\" y=\"0\" width=\"" << target.width << "\" height=\"" << target.height
      << "\" fill=\"white\"/>\n";
  if (!fig.title.empty()) {
    svg << "<text x=\"" << (target.width / 2)
        << "\" y=\"24\" text-anchor=\"middle\" font-size=\"20\" font-family=\"sans-serif\">"
        << detail::escape_xml(fig.title) << "</text>\n";
  }

  const int left = 90;
  const int right = target.width - 60;
  const int top = 70;
  const int bottom = target.height - 80;
  const int pw = std::max(1, right - left);
  const int ph = std::max(1, bottom - top);

  svg << "<rect x=\"" << left << "\" y=\"" << top << "\" width=\"" << pw << "\" height=\"" << ph
      << "\" fill=\"none\" stroke=\"#d0d0d0\"/>\n";

  double xmin = std::numeric_limits<double>::infinity();
  double xmax = -std::numeric_limits<double>::infinity();
  double ymin = std::numeric_limits<double>::infinity();
  double ymax = -std::numeric_limits<double>::infinity();
  double zmin = std::numeric_limits<double>::infinity();
  double zmax = -std::numeric_limits<double>::infinity();
  for (const auto& t : fig.traces) {
    for (std::size_t i = 0; i < t.x.size() && i < t.y.size() && i < t.z.size(); ++i) {
      const double tx = axis_transform(t.x[i], fig.x_axis);
      const double ty = axis_transform(t.y[i], fig.y_axis);
      const double tz = axis_transform(t.z[i], fig.z_axis);
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

  if (fig.x_axis.min.has_value()) xmin = axis_transform(*fig.x_axis.min, fig.x_axis);
  if (fig.x_axis.max.has_value()) xmax = axis_transform(*fig.x_axis.max, fig.x_axis);
  if (fig.y_axis.min.has_value()) ymin = axis_transform(*fig.y_axis.min, fig.y_axis);
  if (fig.y_axis.max.has_value()) ymax = axis_transform(*fig.y_axis.max, fig.y_axis);
  if (fig.z_axis.min.has_value()) zmin = axis_transform(*fig.z_axis.min, fig.z_axis);
  if (fig.z_axis.max.has_value()) zmax = axis_transform(*fig.z_axis.max, fig.z_axis);

  std::tie(xmin, xmax) = ensure_limits(xmin, xmax);
  std::tie(ymin, ymax) = ensure_limits(ymin, ymax);
  std::tie(zmin, zmax) = ensure_limits(zmin, zmax);

  const auto project = [&](double x, double y, double z) {
    const double tx = axis_transform(x, fig.x_axis);
    const double ty = axis_transform(y, fig.y_axis);
    const double tz = axis_transform(z, fig.z_axis);
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

    const double px = left + (xn * 0.78 + zn * 0.22) * static_cast<double>(pw);
    const double py = top + (1.0 - yn) * static_cast<double>(ph) - zn * 0.18 * static_cast<double>(ph);
    return std::pair<double, double>{px, py};
  };

  const auto& palette = detail::default_palette();
  for (std::size_t ti = 0; ti < fig.traces.size(); ++ti) {
    const auto& t = fig.traces[ti];
    const std::string color = t.style.color.empty() ? palette[ti % palette.size()] : t.style.color;
    if (t.kind == Trace3DKind::Line3) {
      svg << "<polyline fill=\"none\" stroke=\"" << color << "\" stroke-width=\"" << t.style.line_width
          << "\" points=\"";
      for (std::size_t i = 0; i < t.x.size() && i < t.y.size() && i < t.z.size(); ++i) {
        const auto [px, py] = project(t.x[i], t.y[i], t.z[i]);
        if (!std::isfinite(px) || !std::isfinite(py)) {
          continue;
        }
        svg << detail::format_double(px, 6) << "," << detail::format_double(py, 6) << " ";
      }
      svg << "\"/>\n";
    } else {
      const double r = (t.kind == Trace3DKind::Surface) ? 1.6 : 2.6;
      for (std::size_t i = 0; i < t.x.size() && i < t.y.size() && i < t.z.size(); ++i) {
        const auto [px, py] = project(t.x[i], t.y[i], t.z[i]);
        if (!std::isfinite(px) || !std::isfinite(py)) {
          continue;
        }
        svg << "<circle cx=\"" << detail::format_double(px, 6)
            << "\" cy=\"" << detail::format_double(py, 6)
            << "\" r=\"" << r << "\" fill=\"" << color << "\" fill-opacity=\"" << t.style.alpha
            << "\"/>\n";
      }
    }
  }

  if (fig.legend.visible && !fig.traces.empty()) {
    const double legend_width = 124.0;
    const double legend_height = static_cast<double>(fig.traces.size() * 15 + 8);
    const auto [lx, ly] = map_legend_anchor(
        fig.legend.location,
        static_cast<double>(left),
        static_cast<double>(right),
        static_cast<double>(top),
        static_cast<double>(bottom),
        legend_width,
        legend_height);
    svg << "<rect x=\"" << lx << "\" y=\"" << ly << "\" width=\"" << legend_width << "\" height=\""
        << legend_height << "\" fill=\"white\" fill-opacity=\"0.85\" stroke=\"#aaa\"/>\n";
    for (std::size_t ti = 0; ti < fig.traces.size(); ++ti) {
      const auto& t = fig.traces[ti];
      const std::string color = t.style.color.empty() ? palette[ti % palette.size()] : t.style.color;
      const double y = ly + 14 + static_cast<double>(ti) * 15.0;
      svg << "<line x1=\"" << (lx + 6) << "\" y1=\"" << y << "\" x2=\"" << (lx + 20)
          << "\" y2=\"" << y << "\" stroke=\"" << color << "\" stroke-width=\"2\"/>\n";
      svg << "<text x=\"" << (lx + 24)
          << "\" y=\"" << (y + 4)
          << "\" font-size=\"11\" font-family=\"sans-serif\">"
          << detail::escape_xml(t.name.empty() ? std::string("trace") : t.name) << "</text>\n";
    }
  }

  svg << "</svg>\n";
  return svg.str();
}
} // namespace detail
} // namespace pluss::plotism
