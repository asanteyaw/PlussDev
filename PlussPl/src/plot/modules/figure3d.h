#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <fmt/format.h>

#include "plot/modules/backend/default_backend.h"
#include "plot/modules/camera_3d.h"
#include "plot/modules/core.h"
#include "plot/modules/detail/series_helpers.h"
#include "plot/modules/detail/show_helpers.h"
#include "plot/modules/projector_3d.h"
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

  Figure3D& data_aspect(std::array<double, 3> ratio) {
    data_aspect_ratio_ = sanitize_aspect_ratio_(ratio, "Figure3D::data_aspect");
    return *this;
  }

  Figure3D& data_aspect(double x, double y, double z) {
    return data_aspect({x, y, z});
  }

  Figure3D& plot_box_aspect(std::array<double, 3> ratio) {
    plot_box_aspect_ratio_ = sanitize_aspect_ratio_(ratio, "Figure3D::plot_box_aspect");
    return *this;
  }

  Figure3D& plot_box_aspect(double x, double y, double z) {
    return plot_box_aspect({x, y, z});
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

  [[deprecated("Use Figure3D::add_line3 with pluss::Series inputs.")]]
  Figure3D& add_line3(const std::vector<double>& x,
                      const std::vector<double>& y,
                      const std::vector<double>& z,
                      std::string name = "",
                      std::optional<Style> style = std::nullopt) {
    return add_line3_values_(x, y, z, std::move(name), style);
  }

  Figure3D& add_line3(const pluss::Series& x,
                      const pluss::Series& y,
                      const pluss::Series& z,
                      std::string name = "",
                      std::optional<Style> style = std::nullopt) {
    return add_line3_values_(detail::to_plot_vector(x, "Figure3D::add_line3"),
                             detail::to_plot_vector(y, "Figure3D::add_line3"),
                             detail::to_plot_vector(z, "Figure3D::add_line3"),
                             std::move(name),
                             style);
  }

  [[deprecated("Use Figure3D::add_scatter3 with pluss::Series inputs.")]]
  Figure3D& add_scatter3(const std::vector<double>& x,
                         const std::vector<double>& y,
                         const std::vector<double>& z,
                         std::string name = "",
                         std::optional<Style> style = std::nullopt) {
    return add_scatter3_values_(x, y, z, std::move(name), style);
  }

  Figure3D& add_scatter3(const pluss::Series& x,
                         const pluss::Series& y,
                         const pluss::Series& z,
                         std::string name = "",
                         std::optional<Style> style = std::nullopt) {
    return add_scatter3_values_(detail::to_plot_vector(x, "Figure3D::add_scatter3"),
                                detail::to_plot_vector(y, "Figure3D::add_scatter3"),
                                detail::to_plot_vector(z, "Figure3D::add_scatter3"),
                                std::move(name),
                                style);
  }

  [[deprecated("Use Figure3D::add_surface with pluss::Series inputs.")]]
  Figure3D& add_surface(const std::vector<double>& x,
                        const std::vector<double>& y,
                        const std::vector<double>& z,
                        std::string name = "",
                        std::optional<Style> style = std::nullopt) {
    return add_surface_values_(x, y, z, std::move(name), style);
  }

  Figure3D& add_surface(const pluss::Series& x,
                        const pluss::Series& y,
                        const pluss::Series& z,
                        std::string name = "",
                        std::optional<Style> style = std::nullopt) {
    const auto z_shape = z.sizes();
    if (z_shape.size() == 2 && x.dim() == 1 && y.dim() == 1) {
      return add_surface_grid_(detail::to_plot_vector(x, "Figure3D::add_surface"),
                               detail::to_plot_vector(y, "Figure3D::add_surface"),
                               detail::to_plot_vector(z, "Figure3D::add_surface"),
                               std::span<const std::int64_t>(z_shape.data(), z_shape.size()),
                               std::move(name),
                               style);
    }
    return add_surface_values_(detail::to_plot_vector(x, "Figure3D::add_surface"),
                               detail::to_plot_vector(y, "Figure3D::add_surface"),
                               detail::to_plot_vector(z, "Figure3D::add_surface"),
                               std::move(name),
                               style);
  }

  Figure3D& surface_filled(bool enabled = true) {
    current_surface_("Figure3D::surface_filled").filled = enabled;
    return *this;
  }

  Figure3D& surface_base_contours(bool enabled = true) {
    current_surface_("Figure3D::surface_base_contours").base_contours = enabled;
    return *this;
  }

  Figure3D& surface_lighting(bool enabled = true, double light_azimuth_deg = 45.0, double light_elevation_deg = 30.0) {
    auto& surface = current_surface_("Figure3D::surface_lighting");
    surface.lighted = enabled;
    surface.light_azimuth_deg = light_azimuth_deg;
    surface.light_elevation_deg = light_elevation_deg;
    return *this;
  }

  Figure3D& mesh_mode() {
    return surface_filled(false).surface_wireframe(true).surface_contours(false).surface_base_contours(false);
  }

  Figure3D& surfc_mode() {
    return surface_filled(true).surface_wireframe(false).surface_contours(false).surface_base_contours(true);
  }

  Figure3D& meshc_mode() {
    return surface_filled(false).surface_wireframe(true).surface_contours(false).surface_base_contours(true);
  }

  Figure3D& surfl_mode(double light_azimuth_deg = 45.0, double light_elevation_deg = 30.0) {
    return surface_filled(true).surface_wireframe(false).surface_base_contours(false).surface_lighting(true,
                                                                                                          light_azimuth_deg,
                                                                                                          light_elevation_deg);
  }

  Figure3D& waterfall_mode() {
    auto& surface = current_surface_("Figure3D::waterfall_mode");
    surface.filled = false;
    surface.wireframe = true;
    surface.wireframe_rows = true;
    surface.wireframe_cols = false;
    surface.curtain_mode = SurfaceCurtainMode::Rows;
    surface.contours = false;
    surface.base_contours = false;
    surface.lighted = false;
    return *this;
  }

  Figure3D& meshz_mode() {
    auto& surface = current_surface_("Figure3D::meshz_mode");
    surface.filled = false;
    surface.wireframe = true;
    surface.wireframe_rows = true;
    surface.wireframe_cols = true;
    surface.curtain_mode = SurfaceCurtainMode::Boundary;
    surface.contours = false;
    surface.base_contours = false;
    surface.lighted = false;
    return *this;
  }

  [[deprecated("Use Figure3D::add_surface with pluss::Series inputs.")]]
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
    std::vector<double> z_flat;
    z_flat.reserve(y_axis.size() * cols);
    for (const auto& row : z_rows) {
      if (row.size() != cols) {
        throw std::invalid_argument("plotism::Figure3D::add_surface: all z rows must match x-axis size");
      }
      z_flat.insert(z_flat.end(), row.begin(), row.end());
    }
    const std::array<std::int64_t, 2> shape{
        static_cast<std::int64_t>(y_axis.size()),
        static_cast<std::int64_t>(cols),
    };
    return add_surface_grid_(x_axis,
                             y_axis,
                             std::move(z_flat),
                             std::span<const std::int64_t>(shape.data(), shape.size()),
                             std::move(name),
                             style);
  }

  [[deprecated("Use Figure3D::add_fill3 with pluss::Series inputs.")]]
  Figure3D& add_fill3(const std::vector<double>& x,
                      const std::vector<double>& y,
                      const std::vector<double>& z,
                      std::string name = "",
                      std::optional<Style> style = std::nullopt) {
    return add_patch3_values_(x, y, z, std::move(name), style);
  }

  Figure3D& add_fill3(const pluss::Series& x,
                      const pluss::Series& y,
                      const pluss::Series& z,
                      std::string name = "",
                      std::optional<Style> style = std::nullopt) {
    return add_patch3_values_(detail::to_plot_vector(x, "Figure3D::add_fill3"),
                              detail::to_plot_vector(y, "Figure3D::add_fill3"),
                              detail::to_plot_vector(z, "Figure3D::add_fill3"),
                              std::move(name),
                              style);
  }

  Figure3D& add_contour3(const pluss::Series& x_axis,
                         const pluss::Series& y_axis,
                         const pluss::Series& z_rows,
                         std::string name = "",
                         std::int32_t levels = 8,
                         std::optional<Style> style = std::nullopt) {
    const auto z_shape = z_rows.sizes();
    if (z_shape.size() != 2) {
      throw std::invalid_argument("plotism::Figure3D::add_contour3: z_rows Series must be 2D");
    }
    return add_contour3_grid_(detail::to_plot_vector(x_axis, "Figure3D::add_contour3"),
                              detail::to_plot_vector(y_axis, "Figure3D::add_contour3"),
                              detail::to_plot_vector(z_rows, "Figure3D::add_contour3"),
                              std::span<const std::int64_t>(z_shape.data(), z_shape.size()),
                              std::move(name),
                              levels,
                              style);
  }

  Figure3D& surface_wireframe(bool enabled = true) {
    auto& surface = current_surface_("Figure3D::surface_wireframe");
    surface.wireframe = enabled;
    surface.wireframe_rows = enabled;
    surface.wireframe_cols = enabled;
    return *this;
  }

  Figure3D& surface_contours(bool enabled = true, std::int32_t stride = 3) {
    auto& surface = current_surface_("Figure3D::surface_contours");
    surface.contours = enabled;
    surface.contour_stride = std::max<std::int32_t>(1, stride);
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
    return detail::open_plot_viewer(title_, path, block, width, height);
  }

  bool show_popup(OutputFormat format = OutputFormat::Svg,
                  std::int32_t width = 1200,
                  std::int32_t height = 900) const {
    return show_window(true, format, width, height);
  }

  void show() const {
#if PLUSSPL_ENABLE_INTERACTIVE_SHOW
    try {
      if (show_window(detail::popup_plot_viewer_available(), OutputFormat::Svg)) {
        return;
      }
    } catch (...) {
      // Fallback to textual preview.
    }
#endif
    show(std::cout);
  }

  void save_svg(std::string_view filepath,
                std::int32_t width = 1200,
                std::int32_t height = 900,
                double dpi = 144.0) const {
    ensure_backend_().save(make_view_(),
                           RenderTarget{.path = filepath, .width = width, .height = height, .dpi = dpi, .format = OutputFormat::Svg});
  }

  void save_png(std::string_view filepath,
                std::int32_t width = 1200,
                std::int32_t height = 900,
                double dpi = 144.0) const {
    ensure_backend_().save(make_view_(),
                           RenderTarget{.path = filepath, .width = width, .height = height, .dpi = dpi, .format = OutputFormat::Png});
  }

  void save_pdf(std::string_view filepath,
                std::int32_t width = 1200,
                std::int32_t height = 900,
                double dpi = 144.0) const {
    ensure_backend_().save(make_view_(),
                           RenderTarget{.path = filepath, .width = width, .height = height, .dpi = dpi, .format = OutputFormat::Pdf});
  }

  Figure3DView view_data() const { return make_view_(); }

private:
  Figure3D& add_line3_values_(const std::vector<double>& x,
                              const std::vector<double>& y,
                              const std::vector<double>& z,
                              std::string name,
                              std::optional<Style> style) {
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

  Figure3D& add_scatter3_values_(const std::vector<double>& x,
                                 const std::vector<double>& y,
                                 const std::vector<double>& z,
                                 std::string name,
                                 std::optional<Style> style) {
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

  Figure3D& add_surface_values_(const std::vector<double>& x,
                                const std::vector<double>& y,
                                const std::vector<double>& z,
                                std::string name,
                                std::optional<Style> style) {
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

  Figure3D& add_patch3_values_(const std::vector<double>& x,
                               const std::vector<double>& y,
                               const std::vector<double>& z,
                               std::string name,
                               std::optional<Style> style) {
    detail::ensure_same_size(x, y, "Figure3D::add_fill3");
    detail::ensure_same_size(x, z, "Figure3D::add_fill3");
    traces_.push_back(Trace3D{
        .kind = Trace3DKind::Patch3,
        .name = std::move(name),
        .x = x,
        .y = y,
        .z = z,
        .filled = true,
        .wireframe = true,
        .wireframe_rows = false,
        .wireframe_cols = false,
        .style = style.value_or(style_),
    });
    return *this;
  }

  Figure3D& add_surface_grid_(const std::vector<double>& x_axis,
                              const std::vector<double>& y_axis,
                              std::vector<double> z_flat,
                              std::span<const std::int64_t> z_shape,
                              std::string name,
                              std::optional<Style> style) {
    if (x_axis.empty() || y_axis.empty() || z_flat.empty() || z_shape.size() != 2) {
      throw std::invalid_argument("plotism::Figure3D::add_surface: grid axes and z rows must be non-empty");
    }
    if (z_shape[0] != static_cast<std::int64_t>(y_axis.size())) {
      throw std::invalid_argument("plotism::Figure3D::add_surface: y-axis size must match z row count");
    }
    if (z_shape[1] != static_cast<std::int64_t>(x_axis.size())) {
      throw std::invalid_argument("plotism::Figure3D::add_surface: x-axis size must match z column count");
    }
    std::vector<double> x;
    std::vector<double> y;
    x.reserve(y_axis.size() * x_axis.size());
    y.reserve(y_axis.size() * x_axis.size());
    for (std::size_t row = 0; row < y_axis.size(); ++row) {
      for (std::size_t col = 0; col < x_axis.size(); ++col) {
        x.push_back(x_axis[col]);
        y.push_back(y_axis[row]);
      }
    }
    traces_.push_back(Trace3D{
        .kind = Trace3DKind::Surface,
        .name = std::move(name),
        .x = std::move(x),
        .y = std::move(y),
        .z = std::move(z_flat),
        .rows = static_cast<std::int64_t>(y_axis.size()),
        .cols = static_cast<std::int64_t>(x_axis.size()),
        .style = style.value_or(style_),
    });
    return *this;
  }

  Figure3D& add_contour3_grid_(const std::vector<double>& x_axis,
                               const std::vector<double>& y_axis,
                               const std::vector<double>& z_flat,
                               std::span<const std::int64_t> z_shape,
                               std::string name,
                               std::int32_t levels,
                               std::optional<Style> style) {
    if (x_axis.empty() || y_axis.empty() || z_flat.empty() || z_shape.size() != 2) {
      throw std::invalid_argument("plotism::Figure3D::add_contour3: grid axes and z rows must be non-empty");
    }
    if (z_shape[0] != static_cast<std::int64_t>(y_axis.size()) || z_shape[1] != static_cast<std::int64_t>(x_axis.size())) {
      throw std::invalid_argument("plotism::Figure3D::add_contour3: x/y sizes must match z grid");
    }
    const std::int64_t rows = z_shape[0];
    const std::int64_t cols = z_shape[1];
    if (rows < 2 || cols < 2) {
      throw std::invalid_argument("plotism::Figure3D::add_contour3: z grid must be at least 2x2");
    }

    const auto [zmin, zmax] = finite_minmax_3d_trace(z_flat);
    const int level_count = std::clamp(static_cast<int>(levels), 2, 24);
    for (int level_index = 0; level_index < level_count; ++level_index) {
      const double level_t = static_cast<double>(level_index + 1) / static_cast<double>(level_count + 1);
      const double level = zmin + (zmax - zmin) * level_t;

      for (std::int64_t row = 0; row + 1 < rows; ++row) {
        for (std::int64_t col = 0; col + 1 < cols; ++col) {
          const std::size_t i00 = static_cast<std::size_t>(row * cols + col);
          const std::size_t i01 = static_cast<std::size_t>(row * cols + (col + 1));
          const std::size_t i10 = static_cast<std::size_t>((row + 1) * cols + col);
          const std::size_t i11 = static_cast<std::size_t>((row + 1) * cols + (col + 1));
          const std::array<std::pair<std::size_t, std::size_t>, 4> edges{{
              {i00, i01},
              {i01, i11},
              {i11, i10},
              {i10, i00},
          }};

          std::vector<std::array<double, 3>> points;
          points.reserve(4);
          for (const auto& [ia, ib] : edges) {
            const double za = z_flat[ia];
            const double zb = z_flat[ib];
            if (!std::isfinite(za) || !std::isfinite(zb)) {
              continue;
            }
            const double lo = std::min(za, zb);
            const double hi = std::max(za, zb);
            if (!(level >= lo && level <= hi) || std::abs(zb - za) <= 1e-12) {
              continue;
            }
            const double t = std::clamp((level - za) / (zb - za), 0.0, 1.0);
            points.push_back({
                x_axis[ia % static_cast<std::size_t>(cols)] + (x_axis[ib % static_cast<std::size_t>(cols)] - x_axis[ia % static_cast<std::size_t>(cols)]) * t,
                y_axis[ia / static_cast<std::size_t>(cols)] + (y_axis[ib / static_cast<std::size_t>(cols)] - y_axis[ia / static_cast<std::size_t>(cols)]) * t,
                level,
            });
          }

          auto contour_style = style.value_or(style_);
          contour_style.marker = "none";
          contour_style.line_width = contour_style.line_width > 0.0 ? contour_style.line_width : 1.2;
          contour_style.color = detail::surface_color_3d(level, zmin, zmax);

          if (points.size() == 2) {
            add_line3_values_({points[0][0], points[1][0]},
                              {points[0][1], points[1][1]},
                              {points[0][2], points[1][2]},
                              name.empty() ? fmt::format("contour {:.4g}", level) : fmt::format("{} {:.4g}", name, level),
                              contour_style);
          } else if (points.size() == 4) {
            add_line3_values_({points[0][0], points[1][0]},
                              {points[0][1], points[1][1]},
                              {points[0][2], points[1][2]},
                              name.empty() ? fmt::format("contour {:.4g}", level) : fmt::format("{} {:.4g}", name, level),
                              contour_style);
            add_line3_values_({points[2][0], points[3][0]},
                              {points[2][1], points[3][1]},
                              {points[2][2], points[3][2]},
                              name.empty() ? fmt::format("contour {:.4g}", level) : fmt::format("{} {:.4g}", name, level),
                              contour_style);
          }
        }
      }
    }
    return *this;
  }

  static const char* trace_kind_name_(Trace3DKind k) noexcept {
    switch (k) {
      case Trace3DKind::Line3: return "line3";
      case Trace3DKind::Scatter3: return "scatter3";
      case Trace3DKind::Surface: return "surface";
      case Trace3DKind::Patch3: return "patch3";
      default: return "trace3d";
    }
  }

  Figure3DView make_view_() const {
    box_aspect_cache_ = compute_box_aspect_();
    return Figure3DView{
        .title = title_,
        .x_axis = x_axis_,
        .y_axis = y_axis_,
        .z_axis = z_axis_,
        .box_aspect = box_aspect_cache_,
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

  static std::array<double, 3> sanitize_aspect_ratio_(std::array<double, 3> ratio, std::string_view where) {
    for (double& value : ratio) {
      if (!(value > 0.0) || !std::isfinite(value)) {
        throw std::invalid_argument(fmt::format("{}: aspect ratio entries must be finite and > 0", where));
      }
    }
    return ratio;
  }

  std::array<double, 3> compute_axis_spans_() const {
    double xmin = std::numeric_limits<double>::infinity();
    double xmax = -std::numeric_limits<double>::infinity();
    double ymin = std::numeric_limits<double>::infinity();
    double ymax = -std::numeric_limits<double>::infinity();
    double zmin = std::numeric_limits<double>::infinity();
    double zmax = -std::numeric_limits<double>::infinity();

    for (const auto& trace : traces_) {
      const std::size_t n = std::min(trace.x.size(), std::min(trace.y.size(), trace.z.size()));
      for (std::size_t i = 0; i < n; ++i) {
        const double tx = x_axis_.transform_value(trace.x[i]);
        const double ty = y_axis_.transform_value(trace.y[i]);
        const double tz = z_axis_.transform_value(trace.z[i]);
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

    if (x_axis_.min.has_value()) xmin = x_axis_.transform_value(*x_axis_.min);
    if (x_axis_.max.has_value()) xmax = x_axis_.transform_value(*x_axis_.max);
    if (y_axis_.min.has_value()) ymin = y_axis_.transform_value(*y_axis_.min);
    if (y_axis_.max.has_value()) ymax = y_axis_.transform_value(*y_axis_.max);
    if (z_axis_.min.has_value()) zmin = z_axis_.transform_value(*z_axis_.min);
    if (z_axis_.max.has_value()) zmax = z_axis_.transform_value(*z_axis_.max);

    const auto ensure_limits = [](double lo, double hi) {
      if (!std::isfinite(lo) || !std::isfinite(hi)) {
        return std::pair<double, double>{0.0, 1.0};
      }
      if (lo == hi) {
        return std::pair<double, double>{lo - 0.5, hi + 0.5};
      }
      if (lo > hi) {
        std::swap(lo, hi);
      }
      return std::pair<double, double>{lo, hi};
    };
    std::tie(xmin, xmax) = ensure_limits(xmin, xmax);
    std::tie(ymin, ymax) = ensure_limits(ymin, ymax);
    std::tie(zmin, zmax) = ensure_limits(zmin, zmax);
    return {
        std::max(1e-9, std::abs(xmax - xmin)),
        std::max(1e-9, std::abs(ymax - ymin)),
        std::max(1e-9, std::abs(zmax - zmin)),
    };
  }

  std::array<double, 3> compute_box_aspect_() const {
    const auto spans = compute_axis_spans_();
    std::array<double, 3> aspect = spans;
    if (data_aspect_ratio_.has_value()) {
      for (std::size_t i = 0; i < 3; ++i) {
        aspect[i] = spans[i] / std::max(1e-9, (*data_aspect_ratio_)[i]);
      }
    }
    if (plot_box_aspect_ratio_.has_value()) {
      for (std::size_t i = 0; i < 3; ++i) {
        aspect[i] *= (*plot_box_aspect_ratio_)[i];
      }
    }
    const double max_component = std::max({aspect[0], aspect[1], aspect[2], 1e-9});
    for (double& value : aspect) {
      value = std::clamp(value / max_component, 0.15, 1.0);
    }
    return aspect;
  }

  std::string title_{};
  XAxis x_axis_{};
  YAxis y_axis_{};
  ZAxis z_axis_{};
  Camera3D camera_{};
  std::optional<std::array<double, 3>> data_aspect_ratio_{};
  std::optional<std::array<double, 3>> plot_box_aspect_ratio_{};
  Legend legend_{};
  std::optional<Colorbar> colorbar_{};
  Style style_{};
  std::vector<Trace3D> traces_{};
  mutable std::array<double, 3> box_aspect_cache_{1.0, 1.0, 1.0};
  mutable std::shared_ptr<const IBackend> backend_{};
};

} // namespace pluss::plotism
