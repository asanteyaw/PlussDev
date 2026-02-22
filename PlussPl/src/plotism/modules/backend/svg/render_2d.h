#pragma once

#include "pluss/plotism/modules/backend/svg/primitives.h"

namespace pluss::plotism {
namespace detail {
inline std::string render_svg_2d(const Figure2DView& fig, const RenderTarget& target) {
  if (target.width <= 100 || target.height <= 100) {
    throw std::invalid_argument("plotism::Figure2D::save_svg: width/height too small");
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

  const int top_margin = 44;
  const int gap = 18;
  const int cell_w = (target.width - gap * (fig.cols + 1)) / fig.cols;
  const int cell_h = (target.height - top_margin - gap * (fig.rows + 1)) / fig.rows;

  const auto& palette = detail::default_palette();
  for (int r = 0; r < fig.rows; ++r) {
    for (int c = 0; c < fig.cols; ++c) {
      const int idx = r * fig.cols + c;
      const auto& sub = fig.subplots[static_cast<std::size_t>(idx)];
      const int x0 = gap + c * (cell_w + gap);
      const int y0 = top_margin + gap + r * (cell_h + gap);

      svg << "<rect x=\"" << x0 << "\" y=\"" << y0 << "\" width=\"" << cell_w << "\" height=\""
          << cell_h << "\" fill=\"none\" stroke=\"#d0d0d0\"/>\n";

      const int left = x0 + 50;
      const int right = x0 + cell_w - 16;
      const int top = y0 + 26;
      const int bottom = y0 + cell_h - 42;
      const int plot_w = std::max(1, right - left);
      const int plot_h = std::max(1, bottom - top);

      svg << "<line x1=\"" << left << "\" y1=\"" << bottom << "\" x2=\"" << right << "\" y2=\""
          << bottom << "\" stroke=\"#222\" stroke-width=\"1\"/>\n";
      svg << "<line x1=\"" << left << "\" y1=\"" << top << "\" x2=\"" << left << "\" y2=\"" << bottom
          << "\" stroke=\"#222\" stroke-width=\"1\"/>\n";

      if (!sub.title.empty()) {
        svg << "<text x=\"" << (x0 + cell_w / 2)
            << "\" y=\"" << (y0 + 18)
            << "\" text-anchor=\"middle\" font-size=\"13\" font-family=\"sans-serif\">"
            << detail::escape_xml(sub.title) << "</text>\n";
      }
      if (!sub.x_axis.label.empty()) {
        svg << "<text x=\"" << (x0 + cell_w / 2)
            << "\" y=\"" << (y0 + cell_h - 10)
            << "\" text-anchor=\"middle\" font-size=\"12\" font-family=\"sans-serif\">"
            << detail::escape_xml(sub.x_axis.label) << "</text>\n";
      }
      if (!sub.y_axis.label.empty()) {
        svg << "<text x=\"" << (x0 + 14)
            << "\" y=\"" << (y0 + cell_h / 2)
            << "\" text-anchor=\"middle\" font-size=\"12\" font-family=\"sans-serif\" transform=\"rotate(-90 "
            << (x0 + 14) << " " << (y0 + cell_h / 2) << ")\">" << detail::escape_xml(sub.y_axis.label)
            << "</text>\n";
      }

      double xmin = std::numeric_limits<double>::infinity();
      double xmax = -std::numeric_limits<double>::infinity();
      double ymin = std::numeric_limits<double>::infinity();
      double ymax = -std::numeric_limits<double>::infinity();
      for (const auto& t : sub.traces) {
        for (std::size_t i = 0; i < t.x.size() && i < t.y.size(); ++i) {
          const double tx = axis_transform(t.x[i], sub.x_axis);
          const double ty = axis_transform(t.y[i], sub.y_axis);
          if (std::isfinite(tx)) {
            xmin = std::min(xmin, tx);
            xmax = std::max(xmax, tx);
          }
          if (std::isfinite(ty)) {
            ymin = std::min(ymin, ty);
            ymax = std::max(ymax, ty);
          }
        }
        if ((t.kind == Trace2DKind::Bar || t.kind == Trace2DKind::Hist) && sub.y_axis.scale == AxisScale::Linear) {
          ymin = std::min(ymin, 0.0);
        }
      }

      if (sub.x_axis.min.has_value()) xmin = axis_transform(*sub.x_axis.min, sub.x_axis);
      if (sub.x_axis.max.has_value()) xmax = axis_transform(*sub.x_axis.max, sub.x_axis);
      if (sub.y_axis.min.has_value()) ymin = axis_transform(*sub.y_axis.min, sub.y_axis);
      if (sub.y_axis.max.has_value()) ymax = axis_transform(*sub.y_axis.max, sub.y_axis);

      std::tie(xmin, xmax) = ensure_limits(xmin, xmax);
      std::tie(ymin, ymax) = ensure_limits(ymin, ymax);

      const auto sx = [&](double x) {
        const double tx = axis_transform(x, sub.x_axis);
        if (!std::isfinite(tx)) {
          return std::numeric_limits<double>::quiet_NaN();
        }
        double xn = (tx - xmin) / (xmax - xmin);
        if (sub.x_axis.inverted) {
          xn = 1.0 - xn;
        }
        return left + xn * static_cast<double>(plot_w);
      };
      const auto sy = [&](double y) {
        const double ty = axis_transform(y, sub.y_axis);
        if (!std::isfinite(ty)) {
          return std::numeric_limits<double>::quiet_NaN();
        }
        double yn = (ty - ymin) / (ymax - ymin);
        if (sub.y_axis.inverted) {
          yn = 1.0 - yn;
        }
        return bottom - yn * static_cast<double>(plot_h);
      };

      for (std::size_t ti = 0; ti < sub.traces.size(); ++ti) {
        const auto& t = sub.traces[ti];
        const std::string color = t.style.color.empty() ? palette[ti % palette.size()] : t.style.color;
        if (t.kind == Trace2DKind::Line) {
          svg << "<polyline fill=\"none\" stroke=\"" << color << "\" stroke-width=\"" << t.style.line_width
              << "\" points=\"";
          for (std::size_t i = 0; i < t.x.size() && i < t.y.size(); ++i) {
            const double px = sx(t.x[i]);
            const double py = sy(t.y[i]);
            if (!std::isfinite(px) || !std::isfinite(py)) {
              continue;
            }
            svg << detail::format_double(px, 6) << "," << detail::format_double(py, 6) << " ";
          }
          svg << "\"/>\n";
        } else if (t.kind == Trace2DKind::Scatter) {
          for (std::size_t i = 0; i < t.x.size() && i < t.y.size(); ++i) {
            const double px = sx(t.x[i]);
            const double py = sy(t.y[i]);
            if (!std::isfinite(px) || !std::isfinite(py)) {
              continue;
            }
            svg << "<circle cx=\"" << detail::format_double(px, 6)
                << "\" cy=\"" << detail::format_double(py, 6)
                << "\" r=\"2.5\" fill=\"" << color << "\" fill-opacity=\"" << t.style.alpha
                << "\"/>\n";
          }
        } else {
          const double barw =
              std::max(2.0, static_cast<double>(plot_w) / std::max<std::size_t>(1, t.x.size()) * 0.7);
          for (std::size_t i = 0; i < t.x.size() && i < t.y.size(); ++i) {
            const double px = sx(t.x[i]);
            if (!std::isfinite(px)) {
              continue;
            }
            const double baseline_val = (sub.y_axis.scale == AxisScale::Linear) ? 0.0 : ymin;
            const double py = sy(std::max(baseline_val, t.y[i]));
            const double p0 = sy(baseline_val);
            if (!std::isfinite(py) || !std::isfinite(p0)) {
              continue;
            }
            const double rect_x = px - barw * 0.5;
            const double h = std::abs(p0 - py);
            const double ytop = std::min(py, p0);
            svg << "<rect x=\"" << detail::format_double(rect_x, 6)
                << "\" y=\"" << detail::format_double(ytop, 6)
                << "\" width=\"" << detail::format_double(barw, 6)
                << "\" height=\"" << detail::format_double(h, 6)
                << "\" fill=\"" << color << "\" fill-opacity=\"" << t.style.alpha << "\"/>\n";
          }
        }
      }

      if (sub.legend.visible && !sub.traces.empty()) {
        const double legend_width = 114.0;
        const double legend_height = static_cast<double>(sub.traces.size() * 15 + 8);
        const auto [lx, ly] = map_legend_anchor(
            sub.legend.location,
            static_cast<double>(left),
            static_cast<double>(right),
            static_cast<double>(top),
            static_cast<double>(bottom),
            legend_width,
            legend_height);

        svg << "<rect x=\"" << lx << "\" y=\"" << ly << "\" width=\"" << legend_width
            << "\" height=\"" << legend_height << "\" fill=\"white\" fill-opacity=\"0.85\" stroke=\"#aaa\"/>\n";
        for (std::size_t ti = 0; ti < sub.traces.size(); ++ti) {
          const auto& t = sub.traces[ti];
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
    }
  }

  svg << "</svg>\n";
  return svg.str();
}
} // namespace detail
} // namespace pluss::plotism
