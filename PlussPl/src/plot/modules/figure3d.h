#pragma once

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <fmt/format.h>

#include "plot/modules/backend/default_backend.h"
#include "plot/modules/camera_3d.h"
#include "plot/modules/core.h"
#include "plot/modules/detail/show_helpers.h"
#include "plot/modules/trace.h"

namespace pluss::plotism {
class Figure3D {
public:
  Figure3D() : backend_(make_default_backend()) {}

  Figure3D& with_title(std::string title) {
    title_ = std::move(title);
    return *this;
  }

  XAxis& xaxis() noexcept { return x_axis_; }
  YAxis& yaxis() noexcept { return y_axis_; }
  ZAxis& zaxis() noexcept { return z_axis_; }
  const XAxis& xaxis() const noexcept { return x_axis_; }
  const YAxis& yaxis() const noexcept { return y_axis_; }
  const ZAxis& zaxis() const noexcept { return z_axis_; }

  Figure3D& x_label(std::string label) {
    x_axis_.set_label(std::move(label));
    return *this;
  }

  Figure3D& y_label(std::string label) {
    y_axis_.set_label(std::move(label));
    return *this;
  }

  Figure3D& z_label(std::string label) {
    z_axis_.set_label(std::move(label));
    return *this;
  }

  Figure3D& view(double azimuth_deg, double elevation_deg) {
    camera_.azimuth_deg = azimuth_deg;
    camera_.elevation_deg = elevation_deg;
    return *this;
  }

  Figure3D& azimuth(double degrees) {
    camera_.azimuth_deg = degrees;
    return *this;
  }

  Figure3D& elevation(double degrees) {
    camera_.elevation_deg = degrees;
    return *this;
  }

  Figure3D& camera_distance(double distance) {
    camera_.distance = std::max(1.0, distance);
    return *this;
  }

  Figure3D& projection(ProjectionMode mode) {
    camera_.projection = mode;
    return *this;
  }

  Figure3D& x_limits(double min_v, double max_v) {
    x_axis_.set_limits(min_v, max_v);
    return *this;
  }

  Figure3D& y_limits(double min_v, double max_v) {
    y_axis_.set_limits(min_v, max_v);
    return *this;
  }

  Figure3D& z_limits(double min_v, double max_v) {
    z_axis_.set_limits(min_v, max_v);
    return *this;
  }

  Figure3D& set_xscale(std::string_view scale) {
    x_axis_.set_scale(detail::parse_axis_scale(scale, "Figure3D::set_xscale"));
    return *this;
  }

  Figure3D& set_yscale(std::string_view scale) {
    y_axis_.set_scale(detail::parse_axis_scale(scale, "Figure3D::set_yscale"));
    return *this;
  }

  Figure3D& set_zscale(std::string_view scale) {
    z_axis_.set_scale(detail::parse_axis_scale(scale, "Figure3D::set_zscale"));
    return *this;
  }

  Figure3D& invert_xaxis(bool enabled = true) {
    x_axis_.set_inverted(enabled);
    return *this;
  }

  Figure3D& invert_yaxis(bool enabled = true) {
    y_axis_.set_inverted(enabled);
    return *this;
  }

  Figure3D& invert_zaxis(bool enabled = true) {
    z_axis_.set_inverted(enabled);
    return *this;
  }

  Figure3D& legend(const Legend& l) {
    legend_ = l;
    return *this;
  }

  Figure3D& colorbar(const Colorbar& c) {
    colorbar_ = c;
    return *this;
  }

  Figure3D& theme(std::string name) {
    style_.theme = std::move(name);
    return *this;
  }

  Figure3D& font_family(std::string family) {
    style_.font_family = std::move(family);
    return *this;
  }

  Figure3D& style(const Style& s) {
    style_ = s;
    return *this;
  }

  Figure3D& set_backend(std::shared_ptr<const IBackend> backend) {
    if (!backend) {
      throw std::invalid_argument("plotism::Figure3D::set_backend: backend cannot be null");
    }
    backend_ = std::move(backend);
    return *this;
  }

  const std::shared_ptr<const IBackend>& backend() const noexcept { return backend_; }

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

  Figure3D& surface_wireframe(bool enabled = true) {
    current_surface_("Figure3D::surface_wireframe").wireframe = enabled;
    return *this;
  }

  Figure3D& surface_contours(bool enabled = true, std::int32_t stride = 3) {
    auto& surface = current_surface_("Figure3D::surface_contours");
    surface.contours = enabled;
    surface.contour_stride = std::max<std::int32_t>(1, stride);
    return *this;
  }

  Figure3D& add_surface(const std::vector<double>& x_axis,
                        const std::vector<double>& y_axis,
                        const std::vector<std::vector<double>>& z_rows,
                        std::string name = "",
                        std::optional<Style> style = std::nullopt) {
    if (x_axis.empty() || y_axis.empty() || z_rows.empty() || z_rows.front().empty()) {
      throw std::invalid_argument("plotism::Figure3D::add_surface: grid axes and z rows must be non-empty");
    }
    if (z_rows.size() != y_axis.size()) {
      throw std::invalid_argument("plotism::Figure3D::add_surface: y-axis size must match z row count");
    }
    const std::size_t cols = x_axis.size();
    std::vector<double> x;
    std::vector<double> y;
    std::vector<double> z;
    x.reserve(y_axis.size() * cols);
    y.reserve(y_axis.size() * cols);
    z.reserve(y_axis.size() * cols);
    for (std::size_t row = 0; row < z_rows.size(); ++row) {
      if (z_rows[row].size() != cols) {
        throw std::invalid_argument("plotism::Figure3D::add_surface: all z rows must match x-axis size");
      }
      for (std::size_t col = 0; col < cols; ++col) {
        x.push_back(x_axis[col]);
        y.push_back(y_axis[row]);
        z.push_back(z_rows[row][col]);
      }
    }
    traces_.push_back(Trace3D{
        .kind = Trace3DKind::Surface,
        .name = std::move(name),
        .x = std::move(x),
        .y = std::move(y),
        .z = std::move(z),
        .rows = static_cast<std::int64_t>(y_axis.size()),
        .cols = static_cast<std::int64_t>(cols),
        .style = style.value_or(style_),
    });
    return *this;
  }

  std::string to_string() const {
    std::string out = fmt::format("Figure3D(traces={}", traces_.size());
    if (!title_.empty()) {
      out += fmt::format(", title=\"{}\"", title_);
    }
    out += ")\n";
    for (const auto& t : traces_) {
      out += fmt::format("  - {}: {} [{} pts]\n", trace_kind_name_(t.kind), t.name, t.x.size());
    }
    return out;
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

  bool show_window(bool block = false,
                   OutputFormat format = OutputFormat::Svg,
                   std::int32_t width = 1200,
                   std::int32_t height = 900) const {
    const std::string path = detail::make_temp_plot_path("plusspl_3d", detail::output_extension(format));
    ensure_backend_().save(make_view_(),
                           RenderTarget{.path = path, .width = width, .height = height, .format = format});
    return detail::open_in_default_viewer(path, block);
  }

  void show() const {
#if PLUSSPL_ENABLE_INTERACTIVE_SHOW
    try {
      if (show_window(false, OutputFormat::Svg)) {
        return;
      }
    } catch (...) {
      // Fallback to textual preview.
    }
#endif
    show(std::cout);
  }

  void save_svg(std::string_view filepath, std::int32_t width = 1200, std::int32_t height = 900) const {
    ensure_backend_().save(make_view_(),
                           RenderTarget{.path = filepath, .width = width, .height = height, .format = OutputFormat::Svg});
  }

  void save_png(std::string_view filepath, std::int32_t width = 1200, std::int32_t height = 900) const {
    ensure_backend_().save(make_view_(),
                           RenderTarget{.path = filepath, .width = width, .height = height, .format = OutputFormat::Png});
  }

  void save_pdf(std::string_view filepath, std::int32_t width = 1200, std::int32_t height = 900) const {
    ensure_backend_().save(make_view_(),
                           RenderTarget{.path = filepath, .width = width, .height = height, .format = OutputFormat::Pdf});
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

  Figure3DView make_view_() const {
    return Figure3DView{
        .title = title_,
        .x_axis = x_axis_,
        .y_axis = y_axis_,
        .z_axis = z_axis_,
        .camera = camera_,
        .legend = legend_,
        .colorbar = colorbar_,
        .style = style_,
        .traces = traces_,
    };
  }

  Trace3D& current_surface_(std::string_view where) {
    auto it = std::find_if(traces_.rbegin(), traces_.rend(), [](const Trace3D& trace) {
      return trace.kind == Trace3DKind::Surface;
    });
    if (it == traces_.rend()) {
      throw std::invalid_argument(fmt::format("{}: no surface trace available", where));
    }
    return *it;
  }

  const IBackend& ensure_backend_() const {
    if (!backend_) {
      backend_ = make_default_backend();
    }
    return *backend_;
  }

  std::string title_{};
  XAxis x_axis_{};
  YAxis y_axis_{};
  ZAxis z_axis_{};
  Camera3D camera_{};
  Legend legend_{};
  std::optional<Colorbar> colorbar_{};
  Style style_{};
  std::vector<Trace3D> traces_{};
  mutable std::shared_ptr<const IBackend> backend_{};
};

} // namespace pluss::plotism
