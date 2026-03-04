#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "plot/modules/encoding_4d.h"
#include "plot/modules/detail/series_helpers.h"
#include "plot/modules/figure3d.h"
#include "plot/modules/view4d.h"

namespace pluss::plotism {

class Figure4D {
public:
  Figure4D& with_title(std::string title) {
    title_ = std::move(title);
    return *this;
  }

  Figure4D& x_label(std::string label) {
    x_label_ = std::move(label);
    return *this;
  }

  Figure4D& y_label(std::string label) {
    y_label_ = std::move(label);
    return *this;
  }

  Figure4D& z_label(std::string label) {
    z_label_ = std::move(label);
    return *this;
  }

  Figure4D& w_label(std::string label) {
    w_label_ = std::move(label);
    if (encoding_.title == "w") {
      encoding_.title = w_label_;
    }
    return *this;
  }

  Figure4D& view(double azimuth_deg, double elevation_deg) {
    camera_.azimuth_deg = azimuth_deg;
    camera_.elevation_deg = elevation_deg;
    return *this;
  }

  Figure4D& azimuth(double degrees) {
    camera_.azimuth_deg = degrees;
    return *this;
  }

  Figure4D& elevation(double degrees) {
    camera_.elevation_deg = degrees;
    return *this;
  }

  Figure4D& camera_distance(double distance) {
    camera_.distance = std::max(1.0, distance);
    return *this;
  }

  Figure4D& projection(ProjectionMode mode) {
    camera_.projection = mode;
    return *this;
  }

  Figure4D& theme(std::string name) {
    style_.theme = std::move(name);
    return *this;
  }

  Figure4D& font_family(std::string family) {
    style_.font_family = std::move(family);
    return *this;
  }

  Figure4D& encoding(const Encoding4D& encoding) {
    encoding_ = encoding;
    return *this;
  }

  Figure4D& encode_color(std::string title = "w", std::int32_t bins = 5) {
    encoding_ = Encoding4D{.strategy = Encoding4DStrategy::Color, .bins = bins, .title = std::move(title), .prefix = "color"};
    return *this;
  }

  Figure4D& encode_slice(std::string title = "w", std::int32_t bins = 4, std::string prefix = "slice") {
    encoding_ = Encoding4D{.strategy = Encoding4DStrategy::Slice, .bins = bins, .title = std::move(title), .prefix = std::move(prefix)};
    return *this;
  }

  Figure4D& encode_size(std::string title = "w", std::int32_t bins = 5) {
    encoding_ = Encoding4D{.strategy = Encoding4DStrategy::Size, .bins = bins, .title = std::move(title), .prefix = "size"};
    return *this;
  }

  Figure4D& encode_frame(std::string title = "w", std::int32_t bins = 4, std::string prefix = "frame") {
    encoding_ = Encoding4D{.strategy = Encoding4DStrategy::Frame, .bins = bins, .title = std::move(title), .prefix = std::move(prefix)};
    return *this;
  }

  Figure4D& encode_facet(std::string title = "w", std::int32_t bins = 4, std::string prefix = "facet") {
    encoding_ = Encoding4D{.strategy = Encoding4DStrategy::Facet, .bins = bins, .title = std::move(title), .prefix = std::move(prefix)};
    return *this;
  }

  [[deprecated("Use Figure4D::add_scatter4 with pluss::Series inputs.")]]
  Figure4D& add_scatter4(const std::vector<double>& x,
                         const std::vector<double>& y,
                         const std::vector<double>& z,
                         const std::vector<double>& w,
                         std::string name = "",
                         std::optional<Style> style = std::nullopt) {
    return add_scatter4_values_(x, y, z, w, std::move(name), style);
  }

  Figure4D& add_scatter4(const pluss::Series& x,
                         const pluss::Series& y,
                         const pluss::Series& z,
                         const pluss::Series& w,
                         std::string name = "",
                         std::optional<Style> style = std::nullopt) {
    return add_scatter4_values_(detail::to_plot_vector(x, "Figure4D::add_scatter4"),
                                detail::to_plot_vector(y, "Figure4D::add_scatter4"),
                                detail::to_plot_vector(z, "Figure4D::add_scatter4"),
                                detail::to_plot_vector(w, "Figure4D::add_scatter4"),
                                std::move(name),
                                style);
  }

  [[deprecated("Use Figure4D::add_line4 with pluss::Series inputs.")]]
  Figure4D& add_line4(const std::vector<double>& x,
                      const std::vector<double>& y,
                      const std::vector<double>& z,
                      const std::vector<double>& w,
                      std::string name = "",
                      std::optional<Style> style = std::nullopt) {
    return add_line4_values_(x, y, z, w, std::move(name), style);
  }

  Figure4D& add_line4(const pluss::Series& x,
                      const pluss::Series& y,
                      const pluss::Series& z,
                      const pluss::Series& w,
                      std::string name = "",
                      std::optional<Style> style = std::nullopt) {
    return add_line4_values_(detail::to_plot_vector(x, "Figure4D::add_line4"),
                             detail::to_plot_vector(y, "Figure4D::add_line4"),
                             detail::to_plot_vector(z, "Figure4D::add_line4"),
                             detail::to_plot_vector(w, "Figure4D::add_line4"),
                             std::move(name),
                             style);
  }

  Figure3D realize_color_figure() const {
    Figure3D fig;
    fig.with_title(title_);
    fig.x_label(x_label_).y_label(y_label_).z_label(z_label_);
    fig.theme(style_.theme).font_family(style_.font_family);
    fig.view(camera_.azimuth_deg, camera_.elevation_deg).camera_distance(camera_.distance).projection(camera_.projection);
    fig.legend(Legend{.visible = true, .location = LegendLocation::OutsideRight, .title = encoding_.title});
    fig.colorbar(Colorbar{.visible = true, .location = ColorbarLocation::Right, .title = encoding_.title});

    for (auto trace3 : detail::encode_color_4d(traces_, encoding_)) {
      if (trace3.kind == Trace3DKind::Line3) {
        fig.add_line3(series_from_values_(trace3.x), series_from_values_(trace3.y), series_from_values_(trace3.z), trace3.name, trace3.style);
      } else {
        fig.add_scatter3(series_from_values_(trace3.x), series_from_values_(trace3.y), series_from_values_(trace3.z), trace3.name, trace3.style);
      }
    }
    return fig;
  }

  Figure3D realize_size_figure() const {
    Figure3D fig;
    fig.with_title(title_);
    fig.x_label(x_label_).y_label(y_label_).z_label(z_label_);
    fig.theme(style_.theme).font_family(style_.font_family);
    fig.view(camera_.azimuth_deg, camera_.elevation_deg).camera_distance(camera_.distance).projection(camera_.projection);
    fig.legend(Legend{.visible = true, .location = LegendLocation::OutsideRight, .title = encoding_.title});

    for (auto trace3 : detail::encode_size_4d(traces_, encoding_)) {
      if (trace3.kind == Trace3DKind::Line3) {
        fig.add_line3(series_from_values_(trace3.x), series_from_values_(trace3.y), series_from_values_(trace3.z), trace3.name, trace3.style);
      } else {
        fig.add_scatter3(series_from_values_(trace3.x), series_from_values_(trace3.y), series_from_values_(trace3.z), trace3.name, trace3.style);
      }
    }
    return fig;
  }

  std::vector<Figure3D> realize_slice_figures() const {
    return realize_binned_figures_();
  }

  std::vector<Figure3D> realize_frame_figures() const {
    return realize_binned_figures_();
  }

  std::vector<Figure3D> realize_facet_figures() const {
    return realize_binned_figures_();
  }

  View4D realize_view() const {
    switch (encoding_.strategy) {
      case Encoding4DStrategy::Color:
        return View4D{title_, style_, View4DLayout::Single, {{.scene = realize_color_figure(), .title = title_}}};
      case Encoding4DStrategy::Size:
        return View4D{title_, style_, View4DLayout::Single, {{.scene = realize_size_figure(), .title = title_}}};
      case Encoding4DStrategy::Slice:
        return realize_binned_view_("slice");
      case Encoding4DStrategy::Frame:
        return realize_binned_view_("frame");
      case Encoding4DStrategy::Facet:
        return realize_binned_view_("facet");
      default:
        return View4D{};
    }
  }

  View4D realize_slice_view() const {
    return realize_binned_view_("slice");
  }

  View4D realize_frame_view() const {
    return realize_binned_view_("frame");
  }

  View4D realize_facet_view() const {
    return realize_binned_view_("facet");
  }

private:
  Figure4D& add_scatter4_values_(const std::vector<double>& x,
                                 const std::vector<double>& y,
                                 const std::vector<double>& z,
                                 const std::vector<double>& w,
                                 std::string name,
                                 std::optional<Style> style) {
    detail::ensure_same_size(x, y, "Figure4D::add_scatter4");
    detail::ensure_same_size(x, z, "Figure4D::add_scatter4");
    detail::ensure_same_size(x, w, "Figure4D::add_scatter4");
    Trace4D trace{
        .kind = Trace4DKind::Scatter4,
        .name = std::move(name),
        .x = x,
        .y = y,
        .z = z,
        .w = w,
        .style = style.value_or(style_),
    };
    if (trace.style.marker == "none") {
      trace.style.marker = "circle";
    }
    traces_.push_back(std::move(trace));
    return *this;
  }

  Figure4D& add_line4_values_(const std::vector<double>& x,
                              const std::vector<double>& y,
                              const std::vector<double>& z,
                              const std::vector<double>& w,
                              std::string name,
                              std::optional<Style> style) {
    detail::ensure_same_size(x, y, "Figure4D::add_line4");
    detail::ensure_same_size(x, z, "Figure4D::add_line4");
    detail::ensure_same_size(x, w, "Figure4D::add_line4");
    traces_.push_back(Trace4D{
        .kind = Trace4DKind::Line4,
        .name = std::move(name),
        .x = x,
        .y = y,
        .z = z,
        .w = w,
        .style = style.value_or(style_),
    });
    return *this;
  }

  static pluss::Series series_from_values_(const std::vector<double>& values) {
    return pluss::make_series(values,
                              {static_cast<std::int64_t>(values.size())},
                              pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));
  }

  std::vector<Figure3D> realize_binned_figures_() const {
    std::vector<Figure3D> figures;
    for (auto panel : make_binned_panels_()) {
      panel.scene.legend(Legend{.visible = true, .location = LegendLocation::OutsideRight, .title = encoding_.title});
      figures.push_back(std::move(panel.scene));
    }
    return figures;
  }

  View4D realize_binned_view_(std::string_view mode) const {
    auto panels = make_binned_panels_();
    if (panels.empty()) {
      return View4D{fmt::format("{} {}", title_, mode), style_, View4DLayout::Grid, {}};
    }
    std::optional<View4DSharedLegend> shared_legend;
    std::optional<View4DSharedColorbar> shared_colorbar;
    const auto first_view = panels.front().scene.view_data();
    if (first_view.legend.visible && !first_view.traces.empty()) {
      shared_legend = View4DSharedLegend{
          .legend = first_view.legend,
          .traces = std::vector<Trace3D>(first_view.traces.begin(), first_view.traces.end()),
      };
    }
    if (first_view.colorbar.has_value() && first_view.colorbar->visible) {
      auto it = std::find_if(first_view.traces.begin(), first_view.traces.end(), [](const Trace3D& trace) {
        return trace.kind == Trace3DKind::Surface && !trace.z.empty();
      });
      if (it != first_view.traces.end()) {
        Trace2D scalar_trace;
        scalar_trace.kind = Trace2DKind::Heatmap;
        scalar_trace.name = it->name;
        scalar_trace.values = it->z;
        scalar_trace.style = it->style;
        shared_colorbar = View4DSharedColorbar{
            .colorbar = *first_view.colorbar,
            .trace = std::move(scalar_trace),
        };
      }
    }
    for (auto& panel : panels) {
      panel.scene.legend(Legend{.visible = false});
      panel.scene.colorbar(Colorbar{.visible = false});
    }
    auto view = View4D{
        fmt::format("{} {} view", title_, mode),
        style_,
        panels.size() == 1 ? View4DLayout::Single : View4DLayout::Grid,
        std::move(panels),
    };
    if (shared_legend.has_value()) {
      view.shared_legend(std::move(*shared_legend));
    }
    if (shared_colorbar.has_value()) {
      view.shared_colorbar(std::move(*shared_colorbar));
    }
    return view;
  }

  std::vector<View4DPanel> make_binned_panels_() const {
    std::vector<View4DPanel> panels;
    const auto slices = detail::encode_slices_4d(traces_, encoding_);
    const auto w_values = detail::collect_w_values_4d(traces_);
    const auto [wmin, wmax] = detail::finite_minmax_4d(w_values);
    const auto bins = std::max<std::int32_t>(1, encoding_.bins);

    for (std::int32_t bin = 0; bin < bins; ++bin) {
      const auto& traces = slices[static_cast<std::size_t>(bin)];
      if (traces.empty()) {
        continue;
      }
      const double lo = wmin + (wmax - wmin) * static_cast<double>(bin) / static_cast<double>(bins);
      const double hi = wmin + (wmax - wmin) * static_cast<double>(bin + 1) / static_cast<double>(bins);

      Figure3D fig;
      fig.with_title(fmt::format("{} {} [{:.3g}, {:.3g}]", title_, encoding_.prefix, lo, hi));
      fig.x_label(x_label_).y_label(y_label_).z_label(z_label_);
      fig.theme(style_.theme).font_family(style_.font_family);
      fig.view(camera_.azimuth_deg, camera_.elevation_deg).camera_distance(camera_.distance).projection(camera_.projection);
      for (const auto& trace : traces) {
        if (trace.kind == Trace3DKind::Line3) {
          fig.add_line3(series_from_values_(trace.x), series_from_values_(trace.y), series_from_values_(trace.z), trace.name, trace.style);
        } else {
          fig.add_scatter3(series_from_values_(trace.x), series_from_values_(trace.y), series_from_values_(trace.z), trace.name, trace.style);
        }
      }
      panels.push_back(View4DPanel{
          .scene = std::move(fig),
          .title = fmt::format("{} [{:.3g}, {:.3g}]", encoding_.title, lo, hi),
      });
    }
    return panels;
  }
  std::string title_{};
  std::string x_label_{"x"};
  std::string y_label_{"y"};
  std::string z_label_{"z"};
  std::string w_label_{"w"};
  Style style_{};
  Encoding4D encoding_{};
  Camera3D camera_{};
  std::vector<Trace4D> traces_{};
};

} // namespace pluss::plotism
