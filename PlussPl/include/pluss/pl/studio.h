#pragma once

#include "pluss/codem/codem.h"
#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <concepts>
#include <initializer_list>
#include <map>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace pluss::pl {

struct Rgba {
  double red{0.0};
  double green{0.0};
  double blue{0.0};
  double alpha{1.0};

  [[nodiscard]] static constexpr Rgba from_u8(
      unsigned red_u8,
      unsigned green_u8,
      unsigned blue_u8,
      unsigned alpha_u8 = 255) noexcept {
    return Rgba{
        static_cast<double>(red_u8) / 255.0,
        static_cast<double>(green_u8) / 255.0,
        static_cast<double>(blue_u8) / 255.0,
        static_cast<double>(alpha_u8) / 255.0,
    };
  }
};

struct Vec3d {
  double x{0.0};
  double y{0.0};
  double z{0.0};
};

struct Range1d {
  double low{0.0};
  double high{1.0};
};

struct Extent2d {
  double width{1280.0};
  double height{820.0};
};

enum class PaneKind : std::uint8_t { Planar, Volumetric };
enum class GridStyle : std::uint8_t { None, Major, MajorMinor };
enum class KeyPlace : std::uint8_t { None, Auto, NorthEast, NorthWest, SouthEast, SouthWest, OutsideEast };
enum class KeyDock : std::uint8_t { None, East, South };
enum class KeyFrame : std::uint8_t { Framed, Soft, None };
enum class ScaleKind : std::uint8_t { Linear, Log10, SymLog };
enum class LimitMode : std::uint8_t { AutoFit, Manual };
enum class LimitMethod : std::uint8_t { TickAligned, Tight, Padded };
enum class TickFormat : std::uint8_t { Auto, Fixed3, Fixed6, Scientific, Engineering, Integer, Percent };
enum class ProjectionKind : std::uint8_t { Perspective, Orthographic };
enum class LightSpace : std::uint8_t { Camera, Data };
enum class LightStyle : std::uint8_t { Local, Infinite };
enum class ShadeMode : std::uint8_t { Faceted, Flat, Smooth };
enum class LightingMode : std::uint8_t { None, Flat, Gouraud };
enum class MaterialProfile : std::uint8_t { Default, Dull, Shiny, Metal };
enum class PdfEngine : std::uint8_t { Auto, Internal, SvgConvert };
enum class TraceKind : std::uint8_t {
  Trend,
  Cloud,
  Cloud3,
  Fieldmap,
  Ribbon,
  Spike,
  Wedge,
  Whisk,
  Band,
  Columns,
  Bucket,
  Quartet,
  Violin,
  Radial,
  Contour,
  Isoband,
  Ridge3,
  IsoShell,
  Path3,
  Spire3,
  Sheet,
  Lattice
};
enum class CueAxis : std::uint8_t { X, Y, Z };

struct RuleCue {
  CueAxis axis{CueAxis::X};
  double value{0.0};
  std::string label{};
  Rgba color{Rgba::from_u8(181, 80, 54, 220)};
  double width{1.1};
};

struct BandCue {
  CueAxis axis{CueAxis::Y};
  Range1d span{};
  std::string label{};
  Rgba stroke{Rgba::from_u8(181, 80, 54, 180)};
  Rgba fill{Rgba::from_u8(242, 178, 109, 72)};
};

struct PinCue {
  Vec3d anchor{};
  std::string text{};
  double offset_x{38.0};
  double offset_y{-18.0};
  Rgba color{Rgba::from_u8(42, 95, 153)};
};

class PaletteRamp {
 public:
  PaletteRamp() = default;
  PaletteRamp(std::string name, std::vector<Rgba> stops);

  [[nodiscard]] const std::string& name() const noexcept { return name_; }
  [[nodiscard]] const std::vector<Rgba>& stops() const noexcept { return stops_; }
  [[nodiscard]] bool empty() const noexcept { return stops_.empty(); }
  [[nodiscard]] Rgba sample(double position) const noexcept;

  [[nodiscard]] static PaletteRamp neutral();
  [[nodiscard]] static PaletteRamp thermal();
  [[nodiscard]] static PaletteRamp lagoon();
  [[nodiscard]] static PaletteRamp ocean();
  [[nodiscard]] static PaletteRamp meadow();

 private:
  std::string name_{};
  std::vector<Rgba> stops_{};
};

class Theme {
 public:
  Theme() = default;
  Theme(std::string name, std::vector<Rgba> accents, Rgba background, Rgba foreground, PaletteRamp surface);

  [[nodiscard]] const std::string& name() const noexcept { return name_; }
  [[nodiscard]] const std::vector<Rgba>& accents() const noexcept { return accents_; }
  [[nodiscard]] const Rgba& background() const noexcept { return background_; }
  [[nodiscard]] const Rgba& foreground() const noexcept { return foreground_; }
  [[nodiscard]] const PaletteRamp& surface_palette() const noexcept { return surface_palette_; }
  [[nodiscard]] Rgba accent(std::size_t index) const noexcept;

  [[nodiscard]] static Theme ocean();
  [[nodiscard]] static Theme paper();

 private:
  std::string name_{};
  std::vector<Rgba> accents_{};
  Rgba background_{Rgba::from_u8(255, 255, 255)};
  Rgba foreground_{Rgba::from_u8(20, 23, 28)};
  PaletteRamp surface_palette_{PaletteRamp::thermal()};
};

namespace themes {
[[nodiscard]] Theme ocean();
[[nodiscard]] Theme paper();
} // namespace themes

struct ScalePolicy {
  ScaleKind kind{ScaleKind::Linear};
  double base{10.0};
  double linear_threshold{1.0};

  [[nodiscard]] static constexpr ScalePolicy linear() noexcept { return {}; }
  [[nodiscard]] static constexpr ScalePolicy log10() noexcept {
    return ScalePolicy{ScaleKind::Log10, 10.0, 1.0};
  }
  [[nodiscard]] static constexpr ScalePolicy symlog(double base_value = 10.0,
                                                    double threshold = 1.0) noexcept {
    return ScalePolicy{ScaleKind::SymLog, base_value, threshold};
  }
};

struct AxisTicks {
  Series positions;
  std::vector<std::string> labels;
};

struct AxisState {
  ScalePolicy scale{};
  LimitMode limit_mode{LimitMode::AutoFit};
  LimitMethod limit_method{LimitMethod::TickAligned};
  TickFormat tick_format{TickFormat::Auto};
  std::optional<Range1d> limits{};
  std::string label{};
  std::optional<AxisTicks> ticks{};
  std::optional<double> tick_rotation_deg{};
  std::optional<double> label_rotation_deg{};
  bool major_grid{false};
  bool minor_grid{false};
};

class Lens {
 public:
  Lens() = default;

  [[nodiscard]] double azimuth_deg() const noexcept { return azimuth_deg_; }
  [[nodiscard]] double elevation_deg() const noexcept { return elevation_deg_; }
  [[nodiscard]] double view_angle_deg() const noexcept { return view_angle_deg_; }
  [[nodiscard]] ProjectionKind projection() const noexcept { return projection_; }
  [[nodiscard]] const Vec3d& target() const noexcept { return target_; }
  [[nodiscard]] const Vec3d& up_vector() const noexcept { return up_vector_; }

  Lens& look(double azimuth_deg, double elevation_deg) noexcept;
  Lens& spin(double delta_azimuth_deg, double delta_elevation_deg) noexcept;
  Lens& target_at(Vec3d target) noexcept;
  Lens& up(Vec3d up_vector) noexcept;
  Lens& project(ProjectionKind projection) noexcept;
  Lens& view_angle(double degrees) noexcept;

  [[nodiscard]] static Lens planar() noexcept;
  [[nodiscard]] static Lens spatial() noexcept;

 private:
  double azimuth_deg_{-37.5};
  double elevation_deg_{30.0};
  double view_angle_deg_{30.0};
  ProjectionKind projection_{ProjectionKind::Perspective};
  Vec3d target_{};
  Vec3d up_vector_{0.0, 0.0, 1.0};
};

struct LightSpec {
  double azimuth_deg{30.0};
  double elevation_deg{30.0};
  LightSpace space{LightSpace::Camera};
  LightStyle style{LightStyle::Local};
};

class LightRig {
 public:
  [[nodiscard]] const std::vector<LightSpec>& lights() const noexcept { return lights_; }
  [[nodiscard]] bool empty() const noexcept { return lights_.empty(); }

  LightRig& clear() noexcept;
  LightRig& add(LightSpec light);
  LightRig& headlight() noexcept;
  LightRig& lamp_right() noexcept;
  LightRig& lamp_left() noexcept;
  LightRig& sun_key() noexcept;

 private:
  std::vector<LightSpec> lights_{};
};

struct TrendData {
  Series x;
  Series y;
};

struct CloudData {
  Series x;
  Series y;
  std::optional<Series> size;
  std::optional<Series> color;
};

struct Cloud3Data {
  Series x;
  Series y;
  Series z;
  std::optional<Series> size;
  std::optional<Series> color;
};

struct FieldmapData {
  Series x;
  Series y;
  Series z;
  std::optional<Series> color;
};

struct ColumnsData {
  Series x;
  Series y;
  double base{0.0};
  double width{0.8};
};

struct RibbonData {
  Series x;
  Series y;
  double base{0.0};
};

struct SpikeData {
  Series x;
  Series y;
  double base{0.0};
};

struct WedgeData {
  Series values;
  std::optional<Series> explode;
  std::vector<std::string> labels;
};

struct WhiskData {
  Series x;
  Series y;
  std::optional<Series> xneg;
  std::optional<Series> xpos;
  std::optional<Series> yneg;
  std::optional<Series> ypos;
};

struct BucketData {
  Series values;
  int bins{12};
  std::optional<Range1d> range;
};

struct QuartetData {
  std::optional<Series> group;
  Series values;
  double width{0.7};
};

struct ViolinData {
  std::optional<Series> group;
  Series values;
  double width{0.8};
};

struct RadialData {
  Series theta;
  Series radius;
};

struct Path3Data {
  Series x;
  Series y;
  Series z;
};

struct Spire3Data {
  Series x;
  Series y;
  Series z;
  std::optional<Series> color;
  double width{0.8};
  double depth{0.8};
  double base{0.0};
};

struct ContourData {
  Series x;
  Series y;
  Series z;
  std::vector<double> levels;
  int auto_levels{8};
  bool filled{false};
  bool elevated{false};
};

struct IsoShellData {
  Series x;
  Series y;
  Series z;
  Series values;
  std::vector<double> levels;
  int auto_levels{4};
};

struct SurfaceData {
  Series x;
  Series y;
  Series z;
  std::optional<Series> color;
};

using TracePayload =
    std::variant<TrendData, CloudData, Cloud3Data, FieldmapData, RibbonData, SpikeData, WedgeData, WhiskData, ColumnsData, BucketData, QuartetData, ViolinData, RadialData, ContourData, IsoShellData, Path3Data, Spire3Data, SurfaceData>;

struct PanePlacement {
  std::int64_t row{0};
  std::int64_t col{0};
  std::int64_t row_span{1};
  std::int64_t col_span{1};
  bool explicit_slot{false};
};

struct MosaicLayout {
  std::int64_t rows{0};
  std::int64_t cols{0};
  double gutter_x{18.0};
  double gutter_y{18.0};
  bool active{false};
};

class Trace {
 public:
  Trace(TraceKind kind, TracePayload payload);

  [[nodiscard]] TraceKind kind() const noexcept { return kind_; }
  [[nodiscard]] const TracePayload& payload() const noexcept { return payload_; }
  [[nodiscard]] const std::string& label_text() const noexcept { return label_; }
  [[nodiscard]] float stroke_width() const noexcept { return stroke_width_; }
  [[nodiscard]] const Rgba& stroke_color() const noexcept { return stroke_color_; }
  [[nodiscard]] const PaletteRamp& palette_ramp() const noexcept { return palette_; }
  [[nodiscard]] const std::optional<Range1d>& color_limits() const noexcept { return color_limits_; }
  [[nodiscard]] ShadeMode shade_mode() const noexcept { return shade_mode_; }
  [[nodiscard]] LightingMode lighting_mode() const noexcept { return lighting_mode_; }
  [[nodiscard]] MaterialProfile material_profile() const noexcept { return material_profile_; }
  [[nodiscard]] bool transparent() const noexcept { return transparent_; }
  [[nodiscard]] bool annotate_levels() const noexcept { return annotate_levels_; }
  [[nodiscard]] bool visible() const noexcept { return visible_; }

  Trace& label(std::string label);
  Trace& width(float width) noexcept;
  Trace& stroke(Rgba color) noexcept;
  Trace& palette(PaletteRamp palette);
  Trace& color_limits(Range1d limits) noexcept;
  Trace& shade(ShadeMode mode) noexcept;
  Trace& lighting(LightingMode mode) noexcept;
  Trace& material(MaterialProfile profile) noexcept;
  Trace& transparent(bool enable = true) noexcept;
  Trace& annotate_levels(bool enable = true) noexcept;
  Trace& hide() noexcept;
  Trace& show() noexcept;

 private:
  TraceKind kind_{TraceKind::Trend};
  TracePayload payload_;
  std::string label_{};
  float stroke_width_{1.5F};
  Rgba stroke_color_{Rgba::from_u8(42, 95, 153)};
  PaletteRamp palette_{PaletteRamp::thermal()};
  std::optional<Range1d> color_limits_{};
  ShadeMode shade_mode_{ShadeMode::Faceted};
  LightingMode lighting_mode_{LightingMode::None};
  MaterialProfile material_profile_{MaterialProfile::Default};
  bool transparent_{false};
  bool annotate_levels_{false};
  bool visible_{true};
};

namespace detail {

template <class T>
concept ArithmeticValue = std::is_arithmetic_v<std::remove_cvref_t<T>>;

template <class Range>
concept ArithmeticRange =
    std::ranges::input_range<Range> && ArithmeticValue<std::ranges::range_value_t<Range>>;

inline Series to_series(const Series& values) {
  return values;
}

inline std::vector<double> to_f64_vector(const Series& values) {
  return values.to(ScalarType::Float64).to_vec<double>();
}

inline Series index_series(std::int64_t count, double first = 1.0) {
  if (count <= 0) {
    return pluss::make_series(std::vector<double>{}, {0});
  }
  if (count == 1) {
    return pluss::make_series(std::vector<double>{first}, {1});
  }
  return pluss::linspace(first, first + static_cast<double>(count - 1), count, ScalarType::Float64);
}

template <ArithmeticRange Range>
inline Series to_series(Range&& values) {
  using Value = std::remove_cvref_t<std::ranges::range_value_t<Range>>;
  std::vector<Value> buffer;
  if constexpr (std::ranges::sized_range<Range>) {
    buffer.reserve(static_cast<std::size_t>(std::ranges::size(values)));
  }
  for (auto&& value : values) {
    buffer.push_back(static_cast<Value>(value));
  }
  const auto extent = static_cast<std::int64_t>(buffer.size());
  return pluss::make_series(buffer, {extent});
}

template <ArithmeticRange Range>
inline std::vector<double> to_f64_vector(Range&& values) {
  std::vector<double> buffer;
  if constexpr (std::ranges::sized_range<Range>) {
    buffer.reserve(static_cast<std::size_t>(std::ranges::size(values)));
  }
  for (auto&& value : values) {
    buffer.push_back(static_cast<double>(value));
  }
  return buffer;
}

template <ArithmeticValue T>
inline Series to_series(std::initializer_list<T> values) {
  const std::vector<std::remove_cv_t<T>> buffer(values.begin(), values.end());
  const auto extent = static_cast<std::int64_t>(buffer.size());
  return pluss::make_series(buffer, {extent});
}

template <ArithmeticValue T>
inline std::vector<double> to_f64_vector(std::initializer_list<T> values) {
  std::vector<double> buffer;
  buffer.reserve(values.size());
  for (const auto value : values) {
    buffer.push_back(static_cast<double>(value));
  }
  return buffer;
}

} // namespace detail

class Pane {
 public:
  Pane(PaneKind kind, Theme theme, std::string title = {});

  [[nodiscard]] PaneKind kind() const noexcept { return kind_; }
  [[nodiscard]] const std::string& title_text() const noexcept { return title_; }
  [[nodiscard]] const Theme& theme_tokens() const noexcept { return theme_; }
  [[nodiscard]] Theme& theme_tokens() noexcept { return theme_; }
  [[nodiscard]] const AxisState& x_axis() const noexcept { return x_axis_; }
  [[nodiscard]] const AxisState& y_axis() const noexcept { return y_axis_; }
  [[nodiscard]] const AxisState& z_axis() const noexcept { return z_axis_; }
  [[nodiscard]] const AxisState& color_axis() const noexcept { return color_axis_; }
  [[nodiscard]] KeyPlace legend_place() const noexcept { return legend_place_; }
  [[nodiscard]] KeyFrame legend_frame() const noexcept { return legend_frame_; }
  [[nodiscard]] GridStyle grid_style() const noexcept { return grid_style_; }
  [[nodiscard]] const Lens& lens() const noexcept { return lens_; }
  [[nodiscard]] Lens& lens() noexcept { return lens_; }
  [[nodiscard]] const LightRig& lights() const noexcept { return lights_; }
  [[nodiscard]] LightRig& lights() noexcept { return lights_; }
  [[nodiscard]] const std::vector<Trace>& traces() const noexcept { return traces_; }
  [[nodiscard]] std::size_t trace_count() const noexcept { return traces_.size(); }
  [[nodiscard]] bool scale_guide_enabled() const noexcept { return scale_guide_; }
  [[nodiscard]] const std::string& caption_text() const noexcept { return caption_; }
  [[nodiscard]] const std::vector<RuleCue>& rules() const noexcept { return rules_; }
  [[nodiscard]] const std::vector<BandCue>& bands() const noexcept { return bands_; }
  [[nodiscard]] const std::vector<PinCue>& pins() const noexcept { return pins_; }
  [[nodiscard]] const std::string& x_align_group() const noexcept { return x_align_group_; }
  [[nodiscard]] const std::string& y_align_group() const noexcept { return y_align_group_; }
  [[nodiscard]] const std::string& z_align_group() const noexcept { return z_align_group_; }
  [[nodiscard]] bool parallel_xy_labels() const noexcept { return parallel_xy_labels_; }
  [[nodiscard]] const PanePlacement& placement() const noexcept { return placement_; }
  [[nodiscard]] double tick_font_scale() const noexcept { return tick_font_scale_; }
  [[nodiscard]] double axis_label_font_scale() const noexcept { return axis_label_font_scale_; }

  Pane& title(std::string title);
  Pane& scale_x(ScalePolicy scale);
  Pane& scale_y(ScalePolicy scale);
  Pane& scale_z(ScalePolicy scale);
  Pane& x_limits(Range1d limits);
  Pane& y_limits(Range1d limits);
  Pane& z_limits(Range1d limits);
  Pane& color_limits(Range1d limits);
  Pane& x_limit_method(LimitMethod method) noexcept;
  Pane& y_limit_method(LimitMethod method) noexcept;
  Pane& z_limit_method(LimitMethod method) noexcept;
  Pane& color_limit_method(LimitMethod method) noexcept;
  Pane& x_tick_format(TickFormat format) noexcept;
  Pane& y_tick_format(TickFormat format) noexcept;
  Pane& z_tick_format(TickFormat format) noexcept;
  Pane& tick_font_scale(double scale);
  Pane& axis_label_font_scale(double scale);
  Pane& x_tick_rotation(double degrees);
  Pane& y_tick_rotation(double degrees);
  Pane& z_tick_rotation(double degrees);
  Pane& x_label_rotation(double degrees);
  Pane& y_label_rotation(double degrees);
  Pane& z_label_rotation(double degrees);
  Pane& clear_x_tick_rotation() noexcept;
  Pane& clear_y_tick_rotation() noexcept;
  Pane& clear_z_tick_rotation() noexcept;
  Pane& clear_x_label_rotation() noexcept;
  Pane& clear_y_label_rotation() noexcept;
  Pane& clear_z_label_rotation() noexcept;
  Pane& auto_x() noexcept;
  Pane& auto_y() noexcept;
  Pane& auto_z() noexcept;
  Pane& auto_color() noexcept;
  Pane& x_label(std::string label);
  Pane& y_label(std::string label);
  Pane& z_label(std::string label);
  Pane& x_ticks(Series positions, std::vector<std::string> labels);
  Pane& y_ticks(Series positions, std::vector<std::string> labels);
  Pane& z_ticks(Series positions, std::vector<std::string> labels);
  Pane& clear_x_ticks() noexcept;
  Pane& clear_y_ticks() noexcept;
  Pane& clear_z_ticks() noexcept;
  Pane& x_categories(std::vector<std::string> labels, double first = 1.0, double step = 1.0);
  Pane& y_categories(std::vector<std::string> labels, double first = 1.0, double step = 1.0);
  Pane& z_categories(std::vector<std::string> labels, double first = 1.0, double step = 1.0);
  Pane& legend(KeyPlace place) noexcept;
  Pane& legend_frame(KeyFrame frame) noexcept;
  Pane& grid(GridStyle style) noexcept;
  Pane& scale_guide(bool enable = true) noexcept;
  Pane& caption(std::string text);
  Pane& rule_x(double value, std::string label = {});
  Pane& rule_y(double value, std::string label = {});
  Pane& rule_z(double value, std::string label = {});
  Pane& band_x(Range1d span, std::string label = {});
  Pane& band_y(Range1d span, std::string label = {});
  Pane& band_z(Range1d span, std::string label = {});
  Pane& pin(std::string text, double x, double y, double offset_x = 38.0, double offset_y = -18.0);
  Pane& pin(std::string text, Vec3d anchor, double offset_x = 38.0, double offset_y = -18.0);
  Pane& align_x(std::string group);
  Pane& align_y(std::string group);
  Pane& align_z(std::string group);
  Pane& align_xy(std::string group);
  Pane& align_xyz(std::string group);
  Pane& parallel_xy_labels(bool enable = true) noexcept;
  Pane& clear_x_align() noexcept;
  Pane& clear_y_align() noexcept;
  Pane& clear_z_align() noexcept;
  Pane& clear_align() noexcept;
  Pane& place(std::int64_t row, std::int64_t col);
  Pane& place(std::int64_t row, std::int64_t col, std::int64_t row_span, std::int64_t col_span);

  template <class Positions>
  Pane& x_ticks(Positions&& positions, std::vector<std::string> labels)
    requires(!std::same_as<std::remove_cvref_t<Positions>, Series>)
  {
    return x_ticks(detail::to_series(std::forward<Positions>(positions)), std::move(labels));
  }

  template <class Positions>
  Pane& y_ticks(Positions&& positions, std::vector<std::string> labels)
    requires(!std::same_as<std::remove_cvref_t<Positions>, Series>)
  {
    return y_ticks(detail::to_series(std::forward<Positions>(positions)), std::move(labels));
  }

  template <class Positions>
  Pane& z_ticks(Positions&& positions, std::vector<std::string> labels)
    requires(!std::same_as<std::remove_cvref_t<Positions>, Series>)
  {
    return z_ticks(detail::to_series(std::forward<Positions>(positions)), std::move(labels));
  }

  template <class X, class Y>
  Trace& trend(X&& x, Y&& y) {
    ensure_planar_("trend");
    return add_trace_(TraceKind::Trend,
                      TrendData{detail::to_series(std::forward<X>(x)),
                                detail::to_series(std::forward<Y>(y))});
  }

  template <class X, class Y>
  Trace& cloud(X&& x, Y&& y) {
    ensure_planar_("cloud");
    return add_trace_(TraceKind::Cloud,
                      CloudData{detail::to_series(std::forward<X>(x)),
                                detail::to_series(std::forward<Y>(y)),
                                std::nullopt,
                                std::nullopt});
  }

  template <class X, class Y, class Size>
  Trace& cloud(X&& x, Y&& y, Size&& size) {
    ensure_planar_("cloud");
    return add_trace_(TraceKind::Cloud,
                      CloudData{detail::to_series(std::forward<X>(x)),
                                detail::to_series(std::forward<Y>(y)),
                                detail::to_series(std::forward<Size>(size)),
                                std::nullopt});
  }

  template <class X, class Y, class Size, class Color>
  Trace& cloud(X&& x, Y&& y, Size&& size, Color&& color) {
    ensure_planar_("cloud");
    return add_trace_(TraceKind::Cloud,
                      CloudData{detail::to_series(std::forward<X>(x)),
                                detail::to_series(std::forward<Y>(y)),
                                detail::to_series(std::forward<Size>(size)),
                                detail::to_series(std::forward<Color>(color))});
  }

  template <class X, class Y, class Z>
  Trace& cloud3(X&& x, Y&& y, Z&& z) {
    ensure_volumetric_("cloud3");
    return add_trace_(TraceKind::Cloud3,
                      Cloud3Data{detail::to_series(std::forward<X>(x)),
                                 detail::to_series(std::forward<Y>(y)),
                                 detail::to_series(std::forward<Z>(z)),
                                 std::nullopt,
                                 std::nullopt});
  }

  template <class X, class Y, class Z, class Size>
  Trace& cloud3(X&& x, Y&& y, Z&& z, Size&& size) {
    ensure_volumetric_("cloud3");
    return add_trace_(TraceKind::Cloud3,
                      Cloud3Data{detail::to_series(std::forward<X>(x)),
                                 detail::to_series(std::forward<Y>(y)),
                                 detail::to_series(std::forward<Z>(z)),
                                 detail::to_series(std::forward<Size>(size)),
                                 std::nullopt});
  }

  template <class X, class Y, class Z, class Size, class Color>
  Trace& cloud3(X&& x, Y&& y, Z&& z, Size&& size, Color&& color) {
    ensure_volumetric_("cloud3");
    return add_trace_(TraceKind::Cloud3,
                      Cloud3Data{detail::to_series(std::forward<X>(x)),
                                 detail::to_series(std::forward<Y>(y)),
                                 detail::to_series(std::forward<Z>(z)),
                                 detail::to_series(std::forward<Size>(size)),
                                 detail::to_series(std::forward<Color>(color))});
  }

  template <class X, class Y, class Z>
  Trace& fieldmap(X&& x, Y&& y, Z&& z) {
    ensure_planar_("fieldmap");
    return add_trace_(TraceKind::Fieldmap,
                      FieldmapData{detail::to_series(std::forward<X>(x)),
                                   detail::to_series(std::forward<Y>(y)),
                                   detail::to_series(std::forward<Z>(z)),
                                   std::nullopt});
  }

  template <class X, class Y, class Z, class C>
  Trace& fieldmap(X&& x, Y&& y, Z&& z, C&& c) {
    ensure_planar_("fieldmap");
    return add_trace_(TraceKind::Fieldmap,
                      FieldmapData{detail::to_series(std::forward<X>(x)),
                                   detail::to_series(std::forward<Y>(y)),
                                   detail::to_series(std::forward<Z>(z)),
                                   detail::to_series(std::forward<C>(c))});
  }

  template <class Y>
  Trace& ribbon(Y&& y, double base = 0.0) {
    ensure_planar_("ribbon");
    const Series y_series = detail::to_series(std::forward<Y>(y));
    const std::int64_t extent = y_series.dim() == 2 ? y_series.sizes()[0] : y_series.numel();
    return add_trace_(TraceKind::Ribbon,
                      RibbonData{detail::index_series(extent),
                                 y_series,
                                 base});
  }

  template <class Y>
  Trace& spike(Y&& y, double base = 0.0) {
    ensure_planar_("spike");
    const Series y_series = detail::to_series(std::forward<Y>(y));
    return add_trace_(TraceKind::Spike,
                      SpikeData{detail::index_series(y_series.numel()),
                                y_series,
                                base});
  }

  template <class X, class Y>
  Trace& spike(X&& x, Y&& y, double base = 0.0) {
    ensure_planar_("spike");
    return add_trace_(TraceKind::Spike,
                      SpikeData{detail::to_series(std::forward<X>(x)),
                                detail::to_series(std::forward<Y>(y)),
                                base});
  }

  template <class Values>
  Trace& wedge(Values&& values) {
    ensure_planar_("wedge");
    return add_trace_(TraceKind::Wedge,
                      WedgeData{detail::to_series(std::forward<Values>(values)),
                                std::nullopt,
                                {}});
  }

  template <class Values>
  Trace& wedge(Values&& values, std::vector<std::string> labels) {
    ensure_planar_("wedge");
    return add_trace_(TraceKind::Wedge,
                      WedgeData{detail::to_series(std::forward<Values>(values)),
                                std::nullopt,
                                std::move(labels)});
  }

  template <class Values, class Explode>
  Trace& wedge(Values&& values, Explode&& explode, std::vector<std::string> labels) {
    ensure_planar_("wedge");
    return add_trace_(TraceKind::Wedge,
                      WedgeData{detail::to_series(std::forward<Values>(values)),
                                detail::to_series(std::forward<Explode>(explode)),
                                std::move(labels)});
  }

  template <class X, class Y>
  Trace& ribbon(X&& x, Y&& y, double base = 0.0) {
    ensure_planar_("ribbon");
    return add_trace_(TraceKind::Ribbon,
                      RibbonData{detail::to_series(std::forward<X>(x)),
                                 detail::to_series(std::forward<Y>(y)),
                                 base});
  }

  template <class Y, class E>
  Trace& whisk(Y&& y, E&& err) {
    ensure_planar_("whisk");
    const Series y_series = detail::to_series(std::forward<Y>(y));
    const std::int64_t extent = y_series.dim() == 2 ? y_series.sizes()[0] : y_series.numel();
    const Series delta = detail::to_series(std::forward<E>(err));
    return add_trace_(TraceKind::Whisk,
                      WhiskData{
                          detail::index_series(extent),
                          y_series,
                          std::nullopt,
                          std::nullopt,
                          delta,
                          delta,
                      });
  }

  template <class X, class Y, class E>
  Trace& whisk(X&& x, Y&& y, E&& err) {
    ensure_planar_("whisk");
    const Series delta = detail::to_series(std::forward<E>(err));
    return add_trace_(TraceKind::Whisk,
                      WhiskData{
                          detail::to_series(std::forward<X>(x)),
                          detail::to_series(std::forward<Y>(y)),
                          std::nullopt,
                          std::nullopt,
                          delta,
                          delta,
                      });
  }

  template <class X, class Y, class Neg, class Pos>
  Trace& whisk(X&& x, Y&& y, Neg&& neg, Pos&& pos) {
    ensure_planar_("whisk");
    return add_trace_(TraceKind::Whisk,
                      WhiskData{
                          detail::to_series(std::forward<X>(x)),
                          detail::to_series(std::forward<Y>(y)),
                          std::nullopt,
                          std::nullopt,
                          detail::to_series(std::forward<Neg>(neg)),
                          detail::to_series(std::forward<Pos>(pos)),
                      });
  }

  template <class X, class Y, class YNeg, class YPos, class XNeg, class XPos>
  Trace& whisk(X&& x, Y&& y, YNeg&& yneg, YPos&& ypos, XNeg&& xneg, XPos&& xpos) {
    ensure_planar_("whisk");
    return add_trace_(TraceKind::Whisk,
                      WhiskData{
                          detail::to_series(std::forward<X>(x)),
                          detail::to_series(std::forward<Y>(y)),
                          detail::to_series(std::forward<XNeg>(xneg)),
                          detail::to_series(std::forward<XPos>(xpos)),
                          detail::to_series(std::forward<YNeg>(yneg)),
                          detail::to_series(std::forward<YPos>(ypos)),
                      });
  }

  template <class Y>
  Trace& columns(Y&& y) {
    ensure_planar_("columns");
    const Series y_series = detail::to_series(std::forward<Y>(y));
    return add_trace_(TraceKind::Columns,
                      ColumnsData{detail::index_series(y_series.numel()),
                                  y_series,
                                  0.0,
                                  0.8});
  }

  template <class X, class Y>
  Trace& columns(X&& x, Y&& y) {
    ensure_planar_("columns");
    return add_trace_(TraceKind::Columns,
                      ColumnsData{detail::to_series(std::forward<X>(x)),
                                  detail::to_series(std::forward<Y>(y)),
                                  0.0,
                                  0.8});
  }

  template <class Values>
  Trace& bucket(Values&& values, int bins = 12) {
    ensure_planar_("bucket");
    return add_trace_(TraceKind::Bucket,
                      BucketData{detail::to_series(std::forward<Values>(values)), bins, std::nullopt});
  }

  template <class Values>
  Trace& bucket(Values&& values, int bins, Range1d range) {
    ensure_planar_("bucket");
    return add_trace_(TraceKind::Bucket,
                      BucketData{detail::to_series(std::forward<Values>(values)), bins, range});
  }

  template <class Values>
  Trace& quartet(Values&& values) {
    ensure_planar_("quartet");
    return add_trace_(TraceKind::Quartet,
                      QuartetData{std::nullopt, detail::to_series(std::forward<Values>(values)), 0.7});
  }

  template <class Group, class Values>
  Trace& quartet(Group&& group, Values&& values) {
    ensure_planar_("quartet");
    return add_trace_(TraceKind::Quartet,
                      QuartetData{detail::to_series(std::forward<Group>(group)),
                                  detail::to_series(std::forward<Values>(values)),
                                  0.7});
  }

  template <class Values>
  Trace& violin(Values&& values) {
    ensure_planar_("violin");
    return add_trace_(TraceKind::Violin,
                      ViolinData{std::nullopt, detail::to_series(std::forward<Values>(values)), 0.8});
  }

  template <class Group, class Values>
  Trace& violin(Group&& group, Values&& values) {
    ensure_planar_("violin");
    return add_trace_(TraceKind::Violin,
                      ViolinData{detail::to_series(std::forward<Group>(group)),
                                 detail::to_series(std::forward<Values>(values)),
                                 0.8});
  }

  template <class Theta, class Radius>
  Trace& radial(Theta&& theta, Radius&& radius) {
    ensure_planar_("radial");
    return add_trace_(TraceKind::Radial,
                      RadialData{detail::to_series(std::forward<Theta>(theta)),
                                 detail::to_series(std::forward<Radius>(radius))});
  }

  template <class X, class Y, class Z>
  Trace& path3(X&& x, Y&& y, Z&& z) {
    ensure_volumetric_("path3");
    return add_trace_(TraceKind::Path3,
                      Path3Data{detail::to_series(std::forward<X>(x)),
                                detail::to_series(std::forward<Y>(y)),
                                detail::to_series(std::forward<Z>(z))});
  }

  template <class Z>
  Trace& spire3(Z&& z) {
    ensure_volumetric_("spire3");
    const Series z_series = detail::to_series(std::forward<Z>(z));
    if (z_series.dim() != 2) {
      throw std::invalid_argument("spire3: z must be a rank-2 matrix");
    }
    const auto rows = z_series.sizes()[0];
    const auto cols = z_series.sizes()[1];
    return add_trace_(TraceKind::Spire3,
                      Spire3Data{detail::index_series(cols),
                                 detail::index_series(rows),
                                 z_series,
                                 std::nullopt,
                                 0.8,
                                 0.8,
                                 0.0});
  }

  template <class Y, class Z>
  Trace& spire3(Y&& y, Z&& z) {
    ensure_volumetric_("spire3");
    const Series z_series = detail::to_series(std::forward<Z>(z));
    if (z_series.dim() != 2) {
      throw std::invalid_argument("spire3: z must be a rank-2 matrix");
    }
    const auto cols = z_series.sizes()[1];
    return add_trace_(TraceKind::Spire3,
                      Spire3Data{detail::index_series(cols),
                                 detail::to_series(std::forward<Y>(y)),
                                 z_series,
                                 std::nullopt,
                                 0.8,
                                 0.8,
                                 0.0});
  }

  template <class X, class Y, class Z>
  Trace& spire3(X&& x, Y&& y, Z&& z) {
    ensure_volumetric_("spire3");
    return add_trace_(TraceKind::Spire3,
                      Spire3Data{detail::to_series(std::forward<X>(x)),
                                 detail::to_series(std::forward<Y>(y)),
                                 detail::to_series(std::forward<Z>(z)),
                                 std::nullopt,
                                 0.8,
                                 0.8,
                                 0.0});
  }

  template <class X, class Y, class Z, class C>
  Trace& spire3(X&& x, Y&& y, Z&& z, C&& c) {
    ensure_volumetric_("spire3");
    return add_trace_(TraceKind::Spire3,
                      Spire3Data{detail::to_series(std::forward<X>(x)),
                                 detail::to_series(std::forward<Y>(y)),
                                 detail::to_series(std::forward<Z>(z)),
                                 detail::to_series(std::forward<C>(c)),
                                 0.8,
                                 0.8,
                                 0.0});
  }

  template <class X, class Y, class Z>
  Trace& contour(X&& x, Y&& y, Z&& z) {
    ensure_planar_("contour");
    return add_trace_(TraceKind::Contour,
                      ContourData{detail::to_series(std::forward<X>(x)),
                                  detail::to_series(std::forward<Y>(y)),
                                  detail::to_series(std::forward<Z>(z)),
                                  {},
                                  8,
                                  false,
                                  false});
  }

  template <class X, class Y, class Z, class Levels>
  Trace& contour(X&& x, Y&& y, Z&& z, Levels&& levels)
    requires(!std::integral<std::remove_cvref_t<Levels>>)
  {
    ensure_planar_("contour");
    return add_trace_(TraceKind::Contour,
                      ContourData{detail::to_series(std::forward<X>(x)),
                                  detail::to_series(std::forward<Y>(y)),
                                  detail::to_series(std::forward<Z>(z)),
                                  detail::to_f64_vector(std::forward<Levels>(levels)),
                                  0,
                                  false,
                                  false});
  }

  template <class X, class Y, class Z>
  Trace& contour(X&& x, Y&& y, Z&& z, int level_count) {
    ensure_planar_("contour");
    return add_trace_(TraceKind::Contour,
                      ContourData{detail::to_series(std::forward<X>(x)),
                                  detail::to_series(std::forward<Y>(y)),
                                  detail::to_series(std::forward<Z>(z)),
                                  {},
                                  level_count,
                                  false,
                                  false});
  }

  template <class X, class Y, class Z>
  Trace& isoband(X&& x, Y&& y, Z&& z) {
    ensure_planar_("isoband");
    return add_trace_(TraceKind::Isoband,
                      ContourData{detail::to_series(std::forward<X>(x)),
                                  detail::to_series(std::forward<Y>(y)),
                                  detail::to_series(std::forward<Z>(z)),
                                  {},
                                  8,
                                  true,
                                  false});
  }

  template <class X, class Y, class Z, class Levels>
  Trace& isoband(X&& x, Y&& y, Z&& z, Levels&& levels)
    requires(!std::integral<std::remove_cvref_t<Levels>>)
  {
    ensure_planar_("isoband");
    return add_trace_(TraceKind::Isoband,
                      ContourData{detail::to_series(std::forward<X>(x)),
                                  detail::to_series(std::forward<Y>(y)),
                                  detail::to_series(std::forward<Z>(z)),
                                  detail::to_f64_vector(std::forward<Levels>(levels)),
                                  0,
                                  true,
                                  false});
  }

  template <class X, class Y, class Z>
  Trace& isoband(X&& x, Y&& y, Z&& z, int level_count) {
    ensure_planar_("isoband");
    return add_trace_(TraceKind::Isoband,
                      ContourData{detail::to_series(std::forward<X>(x)),
                                  detail::to_series(std::forward<Y>(y)),
                                  detail::to_series(std::forward<Z>(z)),
                                  {},
                                  level_count,
                                  true,
                                  false});
  }

  template <class X, class Y, class Z>
  Trace& ridge3(X&& x, Y&& y, Z&& z) {
    ensure_volumetric_("ridge3");
    return add_trace_(TraceKind::Ridge3,
                      ContourData{detail::to_series(std::forward<X>(x)),
                                  detail::to_series(std::forward<Y>(y)),
                                  detail::to_series(std::forward<Z>(z)),
                                  {},
                                  8,
                                  false,
                                  true});
  }

  template <class X, class Y, class Z, class Levels>
  Trace& ridge3(X&& x, Y&& y, Z&& z, Levels&& levels)
    requires(!std::integral<std::remove_cvref_t<Levels>>)
  {
    ensure_volumetric_("ridge3");
    return add_trace_(TraceKind::Ridge3,
                      ContourData{detail::to_series(std::forward<X>(x)),
                                  detail::to_series(std::forward<Y>(y)),
                                  detail::to_series(std::forward<Z>(z)),
                                  detail::to_f64_vector(std::forward<Levels>(levels)),
                                  0,
                                  false,
                                  true});
  }

  template <class X, class Y, class Z>
  Trace& ridge3(X&& x, Y&& y, Z&& z, int level_count) {
    ensure_volumetric_("ridge3");
    return add_trace_(TraceKind::Ridge3,
                      ContourData{detail::to_series(std::forward<X>(x)),
                                  detail::to_series(std::forward<Y>(y)),
                                  detail::to_series(std::forward<Z>(z)),
                                  {},
                                  level_count,
                                  false,
                                  true});
  }

  template <class Values>
  Trace& isoshell(Values&& values, int level_count = 4) {
    ensure_volumetric_("isoshell");
    const Series field = detail::to_series(std::forward<Values>(values));
    if (field.dim() != 3) {
      throw std::invalid_argument("isoshell: values must be a rank-3 field");
    }
    const auto nx = field.sizes()[0];
    const auto ny = field.sizes()[1];
    const auto nz = field.sizes()[2];
    return add_trace_(TraceKind::IsoShell,
                      IsoShellData{
                          detail::index_series(nx),
                          detail::index_series(ny),
                          detail::index_series(nz),
                          field,
                          {},
                          level_count,
                      });
  }

  template <class X, class Y, class Z, class Values>
  Trace& isoshell(X&& x, Y&& y, Z&& z, Values&& values, int level_count = 4) {
    ensure_volumetric_("isoshell");
    return add_trace_(TraceKind::IsoShell,
                      IsoShellData{
                          detail::to_series(std::forward<X>(x)),
                          detail::to_series(std::forward<Y>(y)),
                          detail::to_series(std::forward<Z>(z)),
                          detail::to_series(std::forward<Values>(values)),
                          {},
                          level_count,
                      });
  }

  template <class X, class Y, class Z, class Values, class Levels>
  Trace& isoshell(X&& x, Y&& y, Z&& z, Values&& values, Levels&& levels)
    requires(!std::integral<std::remove_cvref_t<Levels>>)
  {
    ensure_volumetric_("isoshell");
    return add_trace_(TraceKind::IsoShell,
                      IsoShellData{
                          detail::to_series(std::forward<X>(x)),
                          detail::to_series(std::forward<Y>(y)),
                          detail::to_series(std::forward<Z>(z)),
                          detail::to_series(std::forward<Values>(values)),
                          detail::to_f64_vector(std::forward<Levels>(levels)),
                          0,
                      });
  }

  template <class X, class Y, class Z>
  Trace& sheet(X&& x, Y&& y, Z&& z) {
    ensure_volumetric_("sheet");
    return add_trace_(TraceKind::Sheet,
                      SurfaceData{detail::to_series(std::forward<X>(x)),
                                  detail::to_series(std::forward<Y>(y)),
                                  detail::to_series(std::forward<Z>(z)),
                                  std::nullopt});
  }

  template <class X, class Y, class Z, class C>
  Trace& sheet(X&& x, Y&& y, Z&& z, C&& c) {
    ensure_volumetric_("sheet");
    return add_trace_(TraceKind::Sheet,
                      SurfaceData{detail::to_series(std::forward<X>(x)),
                                  detail::to_series(std::forward<Y>(y)),
                                  detail::to_series(std::forward<Z>(z)),
                                  detail::to_series(std::forward<C>(c))});
  }

  template <class X, class Y, class Z>
  Trace& lattice(X&& x, Y&& y, Z&& z) {
    ensure_volumetric_("lattice");
    return add_trace_(TraceKind::Lattice,
                      SurfaceData{detail::to_series(std::forward<X>(x)),
                                  detail::to_series(std::forward<Y>(y)),
                                  detail::to_series(std::forward<Z>(z)),
                                  std::nullopt});
  }

  template <class X, class Y, class Z, class C>
  Trace& lattice(X&& x, Y&& y, Z&& z, C&& c) {
    ensure_volumetric_("lattice");
    return add_trace_(TraceKind::Lattice,
                      SurfaceData{detail::to_series(std::forward<X>(x)),
                                  detail::to_series(std::forward<Y>(y)),
                                  detail::to_series(std::forward<Z>(z)),
                                  detail::to_series(std::forward<C>(c))});
  }

 private:
  void ensure_planar_(std::string_view op) const;
  void ensure_volumetric_(std::string_view op) const;
  Trace& add_trace_(TraceKind kind, TracePayload payload);
  void sync_grid_flags_() noexcept;

  PaneKind kind_{PaneKind::Planar};
  Theme theme_{Theme::paper()};
  std::string title_{};
  AxisState x_axis_{};
  AxisState y_axis_{};
  AxisState z_axis_{};
  AxisState color_axis_{};
  KeyPlace legend_place_{KeyPlace::None};
  KeyFrame legend_frame_{KeyFrame::Soft};
  GridStyle grid_style_{GridStyle::None};
  Lens lens_{Lens::spatial()};
  LightRig lights_{};
  std::vector<Trace> traces_{};
  bool scale_guide_{false};
  std::string caption_{};
  std::vector<RuleCue> rules_{};
  std::vector<BandCue> bands_{};
  std::vector<PinCue> pins_{};
  std::string x_align_group_{};
  std::string y_align_group_{};
  std::string z_align_group_{};
  bool parallel_xy_labels_{true};
  double tick_font_scale_{1.0};
  double axis_label_font_scale_{1.0};
  PanePlacement placement_{};
};

class Studio {
 public:
  explicit Studio(std::string name = {}, Theme theme = Theme::paper());

  [[nodiscard]] const std::string& name() const noexcept { return name_; }
  [[nodiscard]] const std::string& headline_text() const noexcept { return headline_; }
  [[nodiscard]] const Theme& theme_tokens() const noexcept { return theme_; }
  [[nodiscard]] Theme& theme_tokens() noexcept { return theme_; }
  [[nodiscard]] const std::vector<Pane>& panes() const noexcept { return panes_; }
  [[nodiscard]] std::size_t pane_count() const noexcept { return panes_.size(); }
  [[nodiscard]] const Extent2d& extent() const noexcept { return extent_; }
  [[nodiscard]] const MosaicLayout& mosaic_layout() const noexcept { return mosaic_layout_; }
  [[nodiscard]] bool tight_layout_enabled() const noexcept { return tight_layout_enabled_; }
  [[nodiscard]] double tight_layout_padding() const noexcept { return tight_layout_padding_; }
  [[nodiscard]] KeyDock legend_dock() const noexcept { return legend_dock_; }
  [[nodiscard]] KeyFrame legend_frame() const noexcept { return legend_frame_; }
  [[nodiscard]] const std::map<std::string, std::string>& x_captions() const noexcept { return x_captions_; }
  [[nodiscard]] const std::map<std::string, std::string>& y_captions() const noexcept { return y_captions_; }
  [[nodiscard]] const std::map<std::string, std::string>& z_captions() const noexcept { return z_captions_; }

  Studio& rename(std::string name);
  Studio& headline(std::string text);
  Studio& theme(Theme theme);
  Studio& size(double width, double height) noexcept;
  Studio& mosaic(std::int64_t rows, std::int64_t cols, double gutter_x = 18.0, double gutter_y = 18.0);
  Studio& tight_layout(bool enable = true, double padding = 6.0);
  Studio& legend(KeyDock dock) noexcept;
  Studio& legend_frame(KeyFrame frame) noexcept;
  Studio& x_caption(std::string group, std::string text);
  Studio& y_caption(std::string group, std::string text);
  Studio& z_caption(std::string group, std::string text);
  Pane& pane2(std::string title = {});
  Pane& pane3(std::string title = {});
  [[nodiscard]] std::string to_svg() const;
  [[nodiscard]] std::string to_pdf() const;
  void save_svg(std::string_view path) const;
  void save_pdf(std::string_view path, PdfEngine engine = PdfEngine::Auto) const;
  void save_png(std::string_view path, std::int64_t dpi = 144) const;
  bool show(std::string_view title = {}, std::int64_t width = 1280, std::int64_t height = 820, bool launch = true) const;

 private:
  std::string name_{};
  std::string headline_{};
  Theme theme_{Theme::paper()};
  Extent2d extent_{};
  MosaicLayout mosaic_layout_{};
  bool tight_layout_enabled_{false};
  double tight_layout_padding_{6.0};
  KeyDock legend_dock_{KeyDock::None};
  KeyFrame legend_frame_{KeyFrame::Soft};
  std::map<std::string, std::string> x_captions_{};
  std::map<std::string, std::string> y_captions_{};
  std::map<std::string, std::string> z_captions_{};
  std::vector<Pane> panes_{};
};

[[nodiscard]] Studio studio(std::string name = {});

} // namespace pluss::pl
