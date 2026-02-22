#pragma once

#include <cmath>
#include <cstdint>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "pluss/plotism/common.h"

namespace pluss::plotism {

enum class Trace3DKind : std::uint8_t {
  Line3 = 0,
  Scatter3 = 1,
  Surface = 2,
};

struct Trace3D {
  Trace3DKind kind{Trace3DKind::Line3};
  std::string name{};
  std::vector<double> x{};
  std::vector<double> y{};
  std::vector<double> z{};
  Style style{};
};

class Figure3D {
public:
  Figure3D() = default;

  Figure3D& with_title(std::string title) {
    title_ = std::move(title);
    return *this;
  }
  Figure3D& x_label(std::string label) {
    x_axis_.label = std::move(label);
    return *this;
  }
  Figure3D& y_label(std::string label) {
    y_axis_.label = std::move(label);
    return *this;
  }
  Figure3D& z_label(std::string label) {
    z_axis_.label = std::move(label);
    return *this;
  }
  Figure3D& legend(const Legend& l) {
    legend_ = l;
    return *this;
  }
  Figure3D& style(const Style& s) {
    style_ = s;
    return *this;
  }

  Figure3D& add_line3(const std::vector<double>& x,
                      const std::vector<double>& y,
                      const std::vector<double>& z,
                      std::string name = "",
                      std::optional<Style> style = std::nullopt) {
    detail::ensure_same_size(x, y, "Figure3D::add_line3");
    detail::ensure_same_size(x, z, "Figure3D::add_line3");
    traces_.push_back(Trace3D{
        .kind = Trace3DKind::Line3,
        .name = std::move(name),
        .x = x,
        .y = y,
        .z = z,
        .style = style.value_or(style_),
    });
    return *this;
  }

  Figure3D& add_scatter3(const std::vector<double>& x,
                         const std::vector<double>& y,
                         const std::vector<double>& z,
                         std::string name = "",
                         std::optional<Style> style = std::nullopt) {
    detail::ensure_same_size(x, y, "Figure3D::add_scatter3");
    detail::ensure_same_size(x, z, "Figure3D::add_scatter3");
    traces_.push_back(Trace3D{
        .kind = Trace3DKind::Scatter3,
        .name = std::move(name),
        .x = x,
        .y = y,
        .z = z,
        .style = style.value_or(style_),
    });
    return *this;
  }

  Figure3D& add_surface(const std::vector<double>& x,
                        const std::vector<double>& y,
                        const std::vector<double>& z,
                        std::string name = "",
                        std::optional<Style> style = std::nullopt) {
    detail::ensure_same_size(x, y, "Figure3D::add_surface");
    detail::ensure_same_size(x, z, "Figure3D::add_surface");
    traces_.push_back(Trace3D{
        .kind = Trace3DKind::Surface,
        .name = std::move(name),
        .x = x,
        .y = y,
        .z = z,
        .style = style.value_or(style_),
    });
    return *this;
  }

  std::string to_string() const {
    std::ostringstream os;
    os << "Figure3D(traces=" << traces_.size();
    if (!title_.empty()) {
      os << ", title=\"" << title_ << "\"";
    }
    os << ")\n";
    for (const auto& t : traces_) {
      os << "  - " << trace_kind_name_(t.kind) << ": " << t.name << " [" << t.x.size() << " pts]\n";
    }
    return os.str();
  }

  std::string to_json() const {
    std::ostringstream os;
    os << "{";
    os << "\"type\":\"figure3d\"";
    os << ",\"title\":\"" << detail::escape_json(title_) << "\"";
    os << ",\"x_label\":\"" << detail::escape_json(x_axis_.label) << "\"";
    os << ",\"y_label\":\"" << detail::escape_json(y_axis_.label) << "\"";
    os << ",\"z_label\":\"" << detail::escape_json(z_axis_.label) << "\"";
    os << ",\"traces\":[";
    for (std::size_t i = 0; i < traces_.size(); ++i) {
      if (i > 0) {
        os << ",";
      }
      const auto& t = traces_[i];
      os << "{";
      os << "\"kind\":\"" << trace_kind_name_(t.kind) << "\"";
      os << ",\"name\":\"" << detail::escape_json(t.name) << "\"";
      os << ",\"x\":" << detail::vector_to_json(t.x);
      os << ",\"y\":" << detail::vector_to_json(t.y);
      os << ",\"z\":" << detail::vector_to_json(t.z);
      os << "}";
    }
    os << "]}";
    return os.str();
  }

  void show(std::ostream& os) const {
    os << to_string();
    if (traces_.empty()) {
      os << "\n";
      return;
    }
    const auto& t = traces_.front();
    const auto n = std::min<std::size_t>(10, std::min(t.x.size(), std::min(t.y.size(), t.z.size())));
    os << "  preview: ";
    for (std::size_t i = 0; i < n; ++i) {
      if (i > 0) {
        os << ", ";
      }
      os << "(" << detail::format_double(t.x[i]) << ", " << detail::format_double(t.y[i]) << ", "
         << detail::format_double(t.z[i]) << ")";
    }
    if (t.x.size() > n) {
      os << ", ...";
    }
    os << "\n\n";
  }

  void show() const { show(std::cout); }

  void save_svg(std::string_view filepath, std::int32_t width = 1200, std::int32_t height = 900) const {
    if (width <= 100 || height <= 100) {
      throw std::invalid_argument("plotism::Figure3D::save_svg: width/height too small");
    }

    std::ostringstream svg;
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << width << "\" height=\"" << height << "\" "
        << "viewBox=\"0 0 " << width << " " << height << "\">\n";
    svg << "<rect x=\"0\" y=\"0\" width=\"" << width << "\" height=\"" << height << "\" fill=\"white\"/>\n";
    if (!title_.empty()) {
      svg << "<text x=\"" << (width / 2) << "\" y=\"24\" text-anchor=\"middle\" font-size=\"20\" font-family=\"sans-serif\">"
          << detail::escape_xml(title_) << "</text>\n";
    }

    const int left = 90;
    const int right = width - 60;
    const int top = 70;
    const int bottom = height - 80;
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
    for (const auto& t : traces_) {
      const auto [xlo, xhi] = detail::finite_minmax(t.x);
      const auto [ylo, yhi] = detail::finite_minmax(t.y);
      const auto [zlo, zhi] = detail::finite_minmax(t.z);
      xmin = std::min(xmin, xlo);
      xmax = std::max(xmax, xhi);
      ymin = std::min(ymin, ylo);
      ymax = std::max(ymax, yhi);
      zmin = std::min(zmin, zlo);
      zmax = std::max(zmax, zhi);
    }
    if (!std::isfinite(xmin) || xmin == xmax) {
      xmin = 0.0;
      xmax = 1.0;
    }
    if (!std::isfinite(ymin) || ymin == ymax) {
      ymin = 0.0;
      ymax = 1.0;
    }
    if (!std::isfinite(zmin) || zmin == zmax) {
      zmin = 0.0;
      zmax = 1.0;
    }

    const auto project = [&](double x, double y, double z) {
      const double xn = (x - xmin) / (xmax - xmin);
      const double yn = (y - ymin) / (ymax - ymin);
      const double zn = (z - zmin) / (zmax - zmin);
      const double px = left + (xn * 0.78 + zn * 0.22) * static_cast<double>(pw);
      const double py = top + (1.0 - yn) * static_cast<double>(ph) - zn * 0.18 * static_cast<double>(ph);
      return std::pair<double, double>{px, py};
    };

    const auto& palette = detail::default_palette();
    for (std::size_t ti = 0; ti < traces_.size(); ++ti) {
      const auto& t = traces_[ti];
      const std::string color = t.style.color.empty() ? palette[ti % palette.size()] : t.style.color;
      if (t.kind == Trace3DKind::Line3) {
        svg << "<polyline fill=\"none\" stroke=\"" << color << "\" stroke-width=\"" << t.style.line_width << "\" points=\"";
        for (std::size_t i = 0; i < t.x.size() && i < t.y.size() && i < t.z.size(); ++i) {
          if (!std::isfinite(t.x[i]) || !std::isfinite(t.y[i]) || !std::isfinite(t.z[i])) {
            continue;
          }
          const auto [px, py] = project(t.x[i], t.y[i], t.z[i]);
          svg << detail::format_double(px, 6) << "," << detail::format_double(py, 6) << " ";
        }
        svg << "\"/>\n";
      } else {
        const double r = (t.kind == Trace3DKind::Surface) ? 1.6 : 2.6;
        for (std::size_t i = 0; i < t.x.size() && i < t.y.size() && i < t.z.size(); ++i) {
          if (!std::isfinite(t.x[i]) || !std::isfinite(t.y[i]) || !std::isfinite(t.z[i])) {
            continue;
          }
          const auto [px, py] = project(t.x[i], t.y[i], t.z[i]);
          svg << "<circle cx=\"" << detail::format_double(px, 6)
              << "\" cy=\"" << detail::format_double(py, 6)
              << "\" r=\"" << r << "\" fill=\"" << color
              << "\" fill-opacity=\"" << t.style.alpha << "\"/>\n";
        }
      }
    }

    if (legend_.visible && !traces_.empty()) {
      const double lx = right - 130.0;
      const double ly = top + 6.0;
      svg << "<rect x=\"" << lx << "\" y=\"" << ly << "\" width=\"124\" height=\"" << (traces_.size() * 15 + 8)
          << "\" fill=\"white\" fill-opacity=\"0.85\" stroke=\"#aaa\"/>\n";
      for (std::size_t ti = 0; ti < traces_.size(); ++ti) {
        const auto& t = traces_[ti];
        const std::string color = t.style.color.empty() ? palette[ti % palette.size()] : t.style.color;
        const double y = ly + 14 + static_cast<double>(ti) * 15.0;
        svg << "<line x1=\"" << (lx + 6) << "\" y1=\"" << y << "\" x2=\"" << (lx + 20) << "\" y2=\"" << y
            << "\" stroke=\"" << color << "\" stroke-width=\"2\"/>\n";
        svg << "<text x=\"" << (lx + 24) << "\" y=\"" << (y + 4)
            << "\" font-size=\"11\" font-family=\"sans-serif\">"
            << detail::escape_xml(t.name.empty() ? std::string("trace") : t.name) << "</text>\n";
      }
    }

    svg << "</svg>\n";
    detail::write_text_file(filepath, svg.str());
  }

  void save_png(std::string_view filepath, std::int32_t width = 1200, std::int32_t height = 900) const {
    (void)filepath;
    (void)width;
    (void)height;
    throw std::runtime_error(
        "plotism::Figure3D::save_png is not enabled in this build. Use save_svg() or wire a raster backend.");
  }

private:
  static const char* trace_kind_name_(Trace3DKind k) noexcept {
    switch (k) {
      case Trace3DKind::Line3: return "line3";
      case Trace3DKind::Scatter3: return "scatter3";
      case Trace3DKind::Surface: return "surface";
      default: return "trace3d";
    }
  }

  std::string title_{};
  Axis x_axis_{};
  Axis y_axis_{};
  Axis z_axis_{};
  Legend legend_{};
  Style style_{};
  std::vector<Trace3D> traces_{};
};

inline Figure3D line3(const std::vector<double>& x,
                      const std::vector<double>& y,
                      const std::vector<double>& z,
                      std::string name = "") {
  Figure3D fig;
  fig.add_line3(x, y, z, std::move(name));
  return fig;
}

inline Figure3D scatter3(const std::vector<double>& x,
                         const std::vector<double>& y,
                         const std::vector<double>& z,
                         std::string name = "") {
  Figure3D fig;
  fig.add_scatter3(x, y, z, std::move(name));
  return fig;
}

inline Figure3D surface(const std::vector<double>& x,
                        const std::vector<double>& y,
                        const std::vector<double>& z,
                        std::string name = "") {
  Figure3D fig;
  fig.add_surface(x, y, z, std::move(name));
  return fig;
}

} // namespace pluss::plotism

