#pragma once

#include "pluss/plotism/modules/backend/pdf/primitives.h"

namespace pluss::plotism {
namespace detail {
inline std::string render_pdf_2d(const Figure2DView& fig, const RenderTarget& target) {
  if (target.width <= 100 || target.height <= 100) {
    throw std::invalid_argument("plotism::Figure2D::save_pdf: width/height too small");
  }

  PdfPainter painter(target.width, target.height);
  const PdfRgb black = pdf_parse_color("black");
  const PdfRgb white = pdf_parse_color("white");
  const PdfRgb border = pdf_parse_color("#d0d0d0");

  painter.fill_rect(0.0, 0.0, static_cast<double>(target.width), static_cast<double>(target.height), white);
  if (!fig.title.empty()) {
    painter.text_center(static_cast<double>(target.width) * 0.5, 24.0, 20.0, fig.title, black);
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

      painter.stroke_rect(static_cast<double>(x0), static_cast<double>(y0), static_cast<double>(cell_w),
                          static_cast<double>(cell_h), border, 1.0);

      const int left = x0 + 50;
      const int right = x0 + cell_w - 16;
      const int top = y0 + 26;
      const int bottom = y0 + cell_h - 42;
      const int plot_w = std::max(1, right - left);
      const int plot_h = std::max(1, bottom - top);

      painter.stroke_line(static_cast<double>(left), static_cast<double>(bottom), static_cast<double>(right),
                          static_cast<double>(bottom), black, 1.0);
      painter.stroke_line(static_cast<double>(left), static_cast<double>(top), static_cast<double>(left),
                          static_cast<double>(bottom), black, 1.0);

      if (!sub.title.empty()) {
        painter.text_center(static_cast<double>(x0 + cell_w / 2), static_cast<double>(y0 + 18), 13.0, sub.title,
                            black);
      }
      if (!sub.x_axis.label.empty()) {
        painter.text_center(static_cast<double>(x0 + cell_w / 2), static_cast<double>(y0 + cell_h - 10), 12.0,
                            sub.x_axis.label, black);
      }
      if (!sub.y_axis.label.empty()) {
        painter.text_vertical_ccw(static_cast<double>(x0 + 14), static_cast<double>(y0 + cell_h / 2), 12.0,
                                  sub.y_axis.label, black);
      }

      double xmin = std::numeric_limits<double>::infinity();
      double xmax = -std::numeric_limits<double>::infinity();
      double ymin = std::numeric_limits<double>::infinity();
      double ymax = -std::numeric_limits<double>::infinity();
      for (const auto& t : sub.traces) {
        const std::size_t n = std::min(t.x.size(), t.y.size());
        for (std::size_t i = 0; i < n; ++i) {
          const double tx = pdf_axis_transform(t.x[i], sub.x_axis);
          const double ty = pdf_axis_transform(t.y[i], sub.y_axis);
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

      if (sub.x_axis.min.has_value()) xmin = pdf_axis_transform(*sub.x_axis.min, sub.x_axis);
      if (sub.x_axis.max.has_value()) xmax = pdf_axis_transform(*sub.x_axis.max, sub.x_axis);
      if (sub.y_axis.min.has_value()) ymin = pdf_axis_transform(*sub.y_axis.min, sub.y_axis);
      if (sub.y_axis.max.has_value()) ymax = pdf_axis_transform(*sub.y_axis.max, sub.y_axis);

      std::tie(xmin, xmax) = pdf_ensure_limits(xmin, xmax);
      std::tie(ymin, ymax) = pdf_ensure_limits(ymin, ymax);

      const auto sx = [&](double x) {
        const double tx = pdf_axis_transform(x, sub.x_axis);
        if (!std::isfinite(tx)) {
          return std::numeric_limits<double>::quiet_NaN();
        }
        double xn = (tx - xmin) / (xmax - xmin);
        if (sub.x_axis.inverted) {
          xn = 1.0 - xn;
        }
        return static_cast<double>(left) + xn * static_cast<double>(plot_w);
      };
      const auto sy = [&](double y) {
        const double ty = pdf_axis_transform(y, sub.y_axis);
        if (!std::isfinite(ty)) {
          return std::numeric_limits<double>::quiet_NaN();
        }
        double yn = (ty - ymin) / (ymax - ymin);
        if (sub.y_axis.inverted) {
          yn = 1.0 - yn;
        }
        return static_cast<double>(bottom) - yn * static_cast<double>(plot_h);
      };

      for (std::size_t ti = 0; ti < sub.traces.size(); ++ti) {
        const auto& t = sub.traces[ti];
        const std::string color_hex = t.style.color.empty() ? palette[ti % palette.size()] : t.style.color;
        const PdfRgb color = pdf_parse_color(color_hex);

        if (t.kind == Trace2DKind::Line) {
          std::vector<std::pair<double, double>> points;
          points.reserve(std::min(t.x.size(), t.y.size()));
          const std::size_t n = std::min(t.x.size(), t.y.size());
          for (std::size_t i = 0; i < n; ++i) {
            const double px = sx(t.x[i]);
            const double py = sy(t.y[i]);
            if (!std::isfinite(px) || !std::isfinite(py)) {
              continue;
            }
            points.push_back({px, py});
          }
          painter.stroke_polyline(points, color, t.style.line_width);
        } else if (t.kind == Trace2DKind::Scatter) {
          const std::size_t n = std::min(t.x.size(), t.y.size());
          for (std::size_t i = 0; i < n; ++i) {
            const double px = sx(t.x[i]);
            const double py = sy(t.y[i]);
            if (!std::isfinite(px) || !std::isfinite(py)) {
              continue;
            }
            painter.fill_circle(px, py, 2.5, color);
          }
        } else {
          const double barw =
              std::max(2.0, static_cast<double>(plot_w) / std::max<std::size_t>(1, t.x.size()) * 0.7);
          const std::size_t n = std::min(t.x.size(), t.y.size());
          for (std::size_t i = 0; i < n; ++i) {
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
            const double rect_h = std::abs(p0 - py);
            const double rect_y = std::min(py, p0);
            painter.fill_rect(rect_x, rect_y, barw, rect_h, color);
          }
        }
      }

      if (sub.legend.visible && !sub.traces.empty()) {
        const double legend_width = 114.0;
        const double legend_height = static_cast<double>(sub.traces.size() * 15 + 8);
        const auto [lx, ly] = pdf_map_legend_anchor(
            sub.legend.location,
            static_cast<double>(left),
            static_cast<double>(right),
            static_cast<double>(top),
            static_cast<double>(bottom),
            legend_width,
            legend_height);

        painter.fill_rect(lx, ly, legend_width, legend_height, white);
        painter.stroke_rect(lx, ly, legend_width, legend_height, pdf_parse_color("#aaaaaa"), 1.0);
        for (std::size_t ti = 0; ti < sub.traces.size(); ++ti) {
          const auto& t = sub.traces[ti];
          const std::string color_hex = t.style.color.empty() ? palette[ti % palette.size()] : t.style.color;
          const PdfRgb color = pdf_parse_color(color_hex);
          const double y = ly + 14 + static_cast<double>(ti) * 15.0;
          painter.stroke_line(lx + 6.0, y, lx + 20.0, y, color, 2.0);
          painter.text_left(lx + 24.0, y + 4.0, 11.0, t.name.empty() ? std::string("trace") : t.name, black);
        }
      }
    }
  }

  return build_pdf_document(target.width, target.height, painter.finish());
}

} // namespace detail
} // namespace pluss::plotism
