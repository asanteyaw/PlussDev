#include "pluss/pl/plusspl.h"
#include "plotism/render/export_bridge.h"
#include "plotism/spatex/spatex.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <limits>
#include <map>
#include <numeric>
#include <optional>
#include <sstream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace pluss::pl {
namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr char kMathTextMarker = '\x1F';

struct Point2d {
  double x{0.0};
  double y{0.0};
};

struct Point3d {
  double x{0.0};
  double y{0.0};
  double z{0.0};
};

struct MaterialCoefficients {
  double ambient{0.3};
  double diffuse{0.6};
  double specular{0.9};
  double exponent{20.0};
  double specular_color{1.0};
};

struct Rect {
  double x{0.0};
  double y{0.0};
  double width{0.0};
  double height{0.0};
};

struct Bounds2d {
  double min_x{std::numeric_limits<double>::infinity()};
  double max_x{-std::numeric_limits<double>::infinity()};
  double min_y{std::numeric_limits<double>::infinity()};
  double max_y{-std::numeric_limits<double>::infinity()};

  [[nodiscard]] bool valid() const noexcept {
    return std::isfinite(min_x) && std::isfinite(max_x) && std::isfinite(min_y) && std::isfinite(max_y);
  }

  void include(double x, double y) noexcept {
    if (!std::isfinite(x) || !std::isfinite(y)) {
      return;
    }
    min_x = std::min(min_x, x);
    max_x = std::max(max_x, x);
    min_y = std::min(min_y, y);
    max_y = std::max(max_y, y);
  }

  void pad_if_flat() noexcept {
    if (!valid()) {
      min_x = 0.0;
      max_x = 1.0;
      min_y = 0.0;
      max_y = 1.0;
      return;
    }
    if (min_x == max_x) {
      min_x -= 0.5;
      max_x += 0.5;
    }
    if (min_y == max_y) {
      min_y -= 0.5;
      max_y += 0.5;
    }
  }
};

struct Bounds3d {
  double min_x{std::numeric_limits<double>::infinity()};
  double max_x{-std::numeric_limits<double>::infinity()};
  double min_y{std::numeric_limits<double>::infinity()};
  double max_y{-std::numeric_limits<double>::infinity()};
  double min_z{std::numeric_limits<double>::infinity()};
  double max_z{-std::numeric_limits<double>::infinity()};

  [[nodiscard]] bool valid() const noexcept {
    return std::isfinite(min_x) && std::isfinite(max_x) &&
           std::isfinite(min_y) && std::isfinite(max_y) &&
           std::isfinite(min_z) && std::isfinite(max_z);
  }

  void include(double x, double y, double z) noexcept {
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) {
      return;
    }
    min_x = std::min(min_x, x);
    max_x = std::max(max_x, x);
    min_y = std::min(min_y, y);
    max_y = std::max(max_y, y);
    min_z = std::min(min_z, z);
    max_z = std::max(max_z, z);
  }

  void pad_if_flat() noexcept {
    if (!valid()) {
      min_x = min_y = min_z = -0.5;
      max_x = max_y = max_z = 0.5;
      return;
    }
    if (min_x == max_x) {
      min_x -= 0.5;
      max_x += 0.5;
    }
    if (min_y == max_y) {
      min_y -= 0.5;
      max_y += 0.5;
    }
    if (min_z == max_z) {
      min_z -= 0.5;
      max_z += 0.5;
    }
  }
};

struct FillStroke {
  Rgba stroke{};
  Rgba fill{};
  bool has_stroke{true};
  bool has_fill{false};
  double stroke_width{1.0};
};

struct PathCommand {
  std::vector<Point2d> points;
  FillStroke style{};
  bool closed{false};
};

struct TextCommand {
  Point2d anchor{};
  std::string text{};
  double size{12.0};
  Rgba color{};
  double rotation_deg{0.0};
  enum class Align : std::uint8_t { Start, Middle, End } align{Align::Start};
};

using DrawCommand = std::variant<PathCommand, TextCommand>;

struct DrawList {
  Extent2d extent{};
  Rgba background{};
  std::vector<DrawCommand> commands{};
};

struct PaneLayout {
  Rect outer{};
  Rect plot{};
};

struct KeyEntry {
  std::string label{};
  Rgba stroke{};
  Rgba fill{};
  bool filled{false};
  double stroke_width{1.0};
};

struct WrappedKeyEntry {
  std::size_t entry_index{0};
  std::vector<std::string> lines{};
  double block_height{0.0};
};

struct KeyBoxMetrics {
  std::vector<WrappedKeyEntry> wrapped_entries{};
  double box_height{0.0};
};

struct GroupRect {
  Rect rect{};
  bool valid{false};

  void include(Rect other) noexcept {
    if (!valid) {
      rect = other;
      valid = true;
      return;
    }
    const double x0 = std::min(rect.x, other.x);
    const double y0 = std::min(rect.y, other.y);
    const double x1 = std::max(rect.x + rect.width, other.x + other.width);
    const double y1 = std::max(rect.y + rect.height, other.y + other.height);
    rect = Rect{x0, y0, x1 - x0, y1 - y0};
  }
};

struct SeriesPairData {
  std::vector<double> first;
  std::vector<double> second;
};

struct SurfaceGrid {
  std::int64_t rows{0};
  std::int64_t cols{0};
  std::vector<double> x;
  std::vector<double> y;
  std::vector<double> z;
  std::optional<std::vector<double>> color;
};

struct VolumeField {
  std::int64_t nx{0};
  std::int64_t ny{0};
  std::int64_t nz{0};
  std::vector<double> x;
  std::vector<double> y;
  std::vector<double> z;
  std::vector<double> values;
};

struct CloudPoints {
  std::vector<double> x;
  std::vector<double> y;
  std::vector<double> size;
  std::optional<std::vector<double>> color;
};

struct Cloud3Points {
  std::vector<double> x;
  std::vector<double> y;
  std::vector<double> z;
  std::vector<double> size;
  std::optional<std::vector<double>> color;
};

struct RibbonGeometry {
  std::int64_t rows{0};
  std::int64_t cols{0};
  std::vector<double> x;
  std::vector<double> top;
  std::vector<double> bottom;
};

struct WhiskGeometry {
  std::int64_t rows{0};
  std::int64_t cols{0};
  std::vector<double> x;
  std::vector<double> y;
  std::vector<double> xlow;
  std::vector<double> xhigh;
  std::vector<double> ylow;
  std::vector<double> yhigh;
  bool has_x{false};
  bool has_y{false};
};

struct BucketGeometry {
  double low{0.0};
  double high{1.0};
  std::vector<double> counts;
};

struct QuartetSummary {
  double group{1.0};
  double q1{0.0};
  double median{0.0};
  double q3{0.0};
  double low_whisker{0.0};
  double high_whisker{0.0};
  std::vector<double> outliers;
};

struct ViolinShape {
  double group{1.0};
  double q1{0.0};
  double median{0.0};
  double q3{0.0};
  double low_whisker{0.0};
  double high_whisker{0.0};
  ViolinInner inner{ViolinInner::Box};
  std::vector<double> sample;
  std::vector<double> y;
  std::vector<double> left_half_width;
  std::vector<double> right_half_width;
};

struct IsoTriangle {
  std::array<Point3d, 3> vertices{};
  double level{0.0};
};

struct WedgeSlice {
  double start_angle{0.0};
  double end_angle{0.0};
  double explode{0.0};
  std::string label{};
  double fraction{0.0};
};

[[nodiscard]] std::string format_number(double value) {
  std::ostringstream out;
  out << std::fixed << std::setprecision(4) << value;
  return out.str();
}

[[nodiscard]] std::string svg_rgb(Rgba color) {
  auto byte_of = [](double channel) -> int {
    return static_cast<int>(std::round(std::clamp(channel, 0.0, 1.0) * 255.0));
  };
  std::ostringstream out;
  out << "rgb(" << byte_of(color.red) << "," << byte_of(color.green) << "," << byte_of(color.blue) << ")";
  return out.str();
}

[[nodiscard]] std::string escape_xml(std::string_view text) {
  std::string out;
  out.reserve(text.size());
  for (char c : text) {
    switch (c) {
      case '&': out += "&amp;"; break;
      case '<': out += "&lt;"; break;
      case '>': out += "&gt;"; break;
      case '"': out += "&quot;"; break;
      default: out.push_back(c); break;
    }
  }
  return out;
}

[[nodiscard]] std::string normalize_display_text(std::string text) {
  return plotism::spatex::normalize_display_text(text);
}

[[nodiscard]] bool looks_like_math_markup(std::string_view text) noexcept {
  return text.find('\\') != std::string_view::npos || text.find('$') != std::string_view::npos;
}

[[nodiscard]] std::vector<double> to_f64(const Series& series) {
  return series.to(ScalarType::Float64).to_vec<double>();
}

[[nodiscard]] Series flatten_f64(const Series& series) {
  return series.to(ScalarType::Float64).reshape({series.numel()});
}

[[nodiscard]] double scalar_f64(const Series& series) {
  return series.to(ScalarType::Float64).item<double>();
}

[[nodiscard]] bool series_is_vector(const Series& series) noexcept {
  return series.dim() == 1 || (series.dim() == 2 &&
                               (series.sizes()[0] == 1 || series.sizes()[1] == 1));
}

[[nodiscard]] Series as_column_matrix(const Series& series) {
  const Series values = series.to(ScalarType::Float64);
  if (series_is_vector(values)) {
    return values.reshape({values.numel(), 1});
  }
  return values;
}

[[nodiscard]] Series broadcast_plot_matrix(const Series& series, std::int64_t rows, std::int64_t cols) {
  Series values = series.to(ScalarType::Float64);
  if (values.numel() == 1) {
    return pluss::full({rows, cols}, scalar_f64(values), ScalarType::Float64);
  }
  if (series_is_vector(values) && values.numel() == rows) {
    const auto host = to_f64(values.reshape({rows}));
    std::vector<double> tiled;
    tiled.reserve(static_cast<std::size_t>(rows * cols));
    for (std::int64_t row = 0; row < rows; ++row) {
      for (std::int64_t col = 0; col < cols; ++col) {
        tiled.push_back(host[static_cast<std::size_t>(row)]);
      }
    }
    return pluss::make_series(tiled, {rows, cols});
  }
  if (values.dim() == 2 && values.sizes()[0] == rows && values.sizes()[1] == cols) {
    return values;
  }
  if (values.numel() == rows * cols) {
    return values.reshape({rows, cols});
  }
  throw std::invalid_argument("plot matrix broadcast failed");
}

[[nodiscard]] Series broadcast_delta_matrix(
    const std::optional<Series>& series,
    std::int64_t rows,
    std::int64_t cols) {
  if (!series.has_value()) {
    return pluss::zeros({rows, cols}, ScalarType::Float64);
  }
  return broadcast_plot_matrix(*series, rows, cols);
}

[[nodiscard]] Point3d make_point3d(double x, double y, double z) noexcept {
  return Point3d{x, y, z};
}

[[nodiscard]] Point3d add(Point3d lhs, Point3d rhs) noexcept {
  return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

[[nodiscard]] Point3d subtract(Point3d lhs, Point3d rhs) noexcept {
  return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

[[nodiscard]] Point3d scale(Point3d value, double factor) noexcept {
  return {value.x * factor, value.y * factor, value.z * factor};
}

[[nodiscard]] double dot(Point3d lhs, Point3d rhs) noexcept {
  return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

[[nodiscard]] Point3d cross(Point3d lhs, Point3d rhs) noexcept {
  return {
      lhs.y * rhs.z - lhs.z * rhs.y,
      lhs.z * rhs.x - lhs.x * rhs.z,
      lhs.x * rhs.y - lhs.y * rhs.x,
  };
}

[[nodiscard]] double norm(Point3d value) noexcept {
  return std::sqrt(dot(value, value));
}

[[nodiscard]] Point3d normalize(Point3d value) noexcept {
  const double length = norm(value);
  if (!(length > 0.0) || !std::isfinite(length)) {
    return {0.0, 0.0, 1.0};
  }
  return scale(value, 1.0 / length);
}

[[nodiscard]] Point3d reflect(Point3d incident, Point3d normal) noexcept {
  return subtract(scale(normal, 2.0 * dot(normal, incident)), incident);
}

[[nodiscard]] Point3d direction_from_angles(double azimuth_deg, double elevation_deg) noexcept {
  const double az = azimuth_deg * kPi / 180.0;
  const double el = elevation_deg * kPi / 180.0;
  const double c = std::cos(el);
  return normalize({c * std::cos(az), c * std::sin(az), std::sin(el)});
}

[[nodiscard]] Rgba mix(Rgba lhs, Rgba rhs, double t) noexcept {
  const double u = std::clamp(t, 0.0, 1.0);
  return {
      lhs.red + (rhs.red - lhs.red) * u,
      lhs.green + (rhs.green - lhs.green) * u,
      lhs.blue + (rhs.blue - lhs.blue) * u,
      lhs.alpha + (rhs.alpha - lhs.alpha) * u,
  };
}

[[nodiscard]] Rgba multiply_rgb(Rgba color, double factor) noexcept {
  return {
      std::clamp(color.red * factor, 0.0, 1.0),
      std::clamp(color.green * factor, 0.0, 1.0),
      std::clamp(color.blue * factor, 0.0, 1.0),
      color.alpha,
  };
}

[[nodiscard]] Rgba add_rgb(Rgba base, Rgba extra) noexcept {
  return {
      std::clamp(base.red + extra.red, 0.0, 1.0),
      std::clamp(base.green + extra.green, 0.0, 1.0),
      std::clamp(base.blue + extra.blue, 0.0, 1.0),
      std::max(base.alpha, extra.alpha),
  };
}

[[nodiscard]] std::optional<double> scale_value(double value, const ScalePolicy& policy) {
  switch (policy.kind) {
    case ScaleKind::Linear:
      return std::isfinite(value) ? std::optional<double>(value) : std::nullopt;
    case ScaleKind::Log10:
      if (!std::isfinite(value) || value <= 0.0) {
        return std::nullopt;
      }
      return std::log10(value);
    case ScaleKind::SymLog:
      if (!std::isfinite(value) || !std::isfinite(policy.base) || policy.base <= 1.0 ||
          !std::isfinite(policy.linear_threshold) || policy.linear_threshold <= 0.0) {
        return std::nullopt;
      }
      if (std::abs(value) <= policy.linear_threshold) {
        return value / policy.linear_threshold;
      }
      return (value < 0.0 ? -1.0 : 1.0) *
             (1.0 + std::log(std::abs(value) / policy.linear_threshold) / std::log(policy.base));
    default:
      return std::nullopt;
  }
}

[[nodiscard]] std::optional<double> unscale_value(double value, const ScalePolicy& policy) {
  switch (policy.kind) {
    case ScaleKind::Linear:
      return std::isfinite(value) ? std::optional<double>(value) : std::nullopt;
    case ScaleKind::Log10:
      if (!std::isfinite(value)) {
        return std::nullopt;
      }
      return std::pow(10.0, value);
    case ScaleKind::SymLog:
      if (!std::isfinite(value) || !std::isfinite(policy.base) || policy.base <= 1.0 ||
          !std::isfinite(policy.linear_threshold) || policy.linear_threshold <= 0.0) {
        return std::nullopt;
      }
      if (std::abs(value) <= 1.0) {
        return value * policy.linear_threshold;
      }
      return (value < 0.0 ? -1.0 : 1.0) *
             policy.linear_threshold *
             std::pow(policy.base, std::abs(value) - 1.0);
    default:
      return std::nullopt;
  }
}

[[nodiscard]] std::string format_tick_value(double value) {
  const double snapped = std::abs(value) <= 1.0e-12 ? 0.0 : value;
  std::ostringstream label;
  const double magnitude = std::abs(snapped);
  if (magnitude >= 1.0e4 || (magnitude > 0.0 && magnitude < 1.0e-3)) {
    label << std::scientific << std::setprecision(2) << snapped;
  } else {
    label << std::setprecision(4) << snapped;
  }
  return label.str();
}

[[nodiscard]] std::string format_tick_value(double value, TickFormat format) {
  if (!std::isfinite(value)) {
    return {};
  }
  const double snapped = std::abs(value) <= 1.0e-12 ? 0.0 : value;
  if (format == TickFormat::Auto) {
    return format_tick_value(snapped);
  }

  std::ostringstream out;
  switch (format) {
    case TickFormat::Fixed3:
      out << std::fixed << std::setprecision(3) << snapped;
      return out.str();
    case TickFormat::Fixed6:
      out << std::fixed << std::setprecision(6) << snapped;
      return out.str();
    case TickFormat::Scientific:
      out << std::scientific << std::setprecision(2) << snapped;
      return out.str();
    case TickFormat::Engineering: {
      if (snapped == 0.0) {
        return "0";
      }
      const double abs_value = std::abs(snapped);
      const int exponent3 = static_cast<int>(std::floor(std::log10(abs_value) / 3.0)) * 3;
      const double mantissa = snapped / std::pow(10.0, static_cast<double>(exponent3));
      out << std::fixed << std::setprecision(3) << mantissa << "e" << exponent3;
      return out.str();
    }
    case TickFormat::Integer:
      out << static_cast<long long>(std::llround(snapped));
      return out.str();
    case TickFormat::Percent:
      out << std::fixed << std::setprecision(1) << (snapped * 100.0) << "%";
      return out.str();
    case TickFormat::Auto:
    default:
      return format_tick_value(value);
  }
}

[[nodiscard]] double nice_step(double span, int target_ticks = 5);

[[nodiscard]] Range1d effective_range(const AxisState& axis, double derived_low, double derived_high) {
  if (axis.limit_mode == LimitMode::Manual && axis.limits.has_value()) {
    return *axis.limits;
  }
  if (!std::isfinite(derived_low) || !std::isfinite(derived_high)) {
    return axis.scale.kind == ScaleKind::Log10 ? Range1d{0.1, 10.0} : Range1d{-1.0, 1.0};
  }
  if (axis.scale.kind == ScaleKind::Log10) {
    if (derived_high <= 0.0) {
      return {0.1, 10.0};
    }
    if (derived_low <= 0.0) {
      derived_low = std::max(derived_high * 1.0e-3, 1.0e-6);
    }
  }

  if (derived_low == derived_high) {
    if (axis.scale.kind == ScaleKind::Log10) {
      return {derived_low / std::sqrt(10.0), derived_high * std::sqrt(10.0)};
    }
    return {derived_low - 0.5, derived_high + 0.5};
  }
  if (axis.limit_method == LimitMethod::Tight) {
    return {derived_low, derived_high};
  }

  const auto low_t = scale_value(derived_low, axis.scale);
  const auto high_t = scale_value(derived_high, axis.scale);
  if (!low_t.has_value() || !high_t.has_value()) {
    return {derived_low, derived_high};
  }
  double aligned_low_t = *low_t;
  double aligned_high_t = *high_t;
  if (!(aligned_high_t > aligned_low_t)) {
    return {derived_low, derived_high};
  }

  const double span_t = aligned_high_t - aligned_low_t;
  if (axis.limit_method == LimitMethod::Padded) {
    const double pad_t = std::max(span_t * 0.06, 1.0e-9);
    aligned_low_t -= pad_t;
    aligned_high_t += pad_t;
  } else if (axis.limit_method == LimitMethod::TickAligned) {
    const double step_t = nice_step(span_t, 5);
    aligned_low_t = std::floor(aligned_low_t / step_t) * step_t;
    aligned_high_t = std::ceil(aligned_high_t / step_t) * step_t;
    if (!(aligned_high_t > aligned_low_t)) {
      aligned_high_t = aligned_low_t + step_t;
    }
  }

  const auto aligned_low = unscale_value(aligned_low_t, axis.scale);
  const auto aligned_high = unscale_value(aligned_high_t, axis.scale);
  if (!aligned_low.has_value() || !aligned_high.has_value()) {
    return {derived_low, derived_high};
  }
  if (*aligned_high <= *aligned_low || !std::isfinite(*aligned_low) || !std::isfinite(*aligned_high)) {
    return {derived_low, derived_high};
  }
  return {*aligned_low, *aligned_high};
}

[[nodiscard]] double nice_step(double span, int target_ticks) {
  if (!(span > 0.0) || !std::isfinite(span)) {
    return 1.0;
  }
  const double raw = span / static_cast<double>(std::max(target_ticks, 1));
  const double exponent = std::floor(std::log10(raw));
  const double scaled = raw / std::pow(10.0, exponent);
  double nice = 1.0;
  if (scaled < 1.5) {
    nice = 1.0;
  } else if (scaled < 3.5) {
    nice = 2.0;
  } else if (scaled < 7.5) {
    nice = 5.0;
  } else {
    nice = 10.0;
  }
  return nice * std::pow(10.0, exponent);
}

[[nodiscard]] std::vector<std::pair<double, std::string>> linear_ticks(Range1d range, int target = 5) {
  const double step = nice_step(range.high - range.low, target);
  const double start = std::ceil(range.low / step) * step;
  std::vector<std::pair<double, std::string>> ticks;
  for (double value = start; value <= range.high + step * 0.5; value += step) {
    if (value >= range.low - step * 0.5 && value <= range.high + step * 0.5) {
      ticks.emplace_back(value, format_tick_value(value));
    }
    if (ticks.size() > 12) {
      break;
    }
  }
  return ticks;
}

[[nodiscard]] std::vector<std::pair<double, std::string>> log_ticks(Range1d range) {
  std::vector<std::pair<double, std::string>> ticks;
  if (range.low <= 0.0 || range.high <= 0.0) {
    return ticks;
  }
  const int low = static_cast<int>(std::ceil(std::log10(range.low)));
  const int high = static_cast<int>(std::floor(std::log10(range.high)));
  for (int power = low; power <= high && ticks.size() < 12; ++power) {
    const double value = std::pow(10.0, static_cast<double>(power));
    std::ostringstream label;
    label << "1e" << power;
    ticks.emplace_back(value, label.str());
  }
  return ticks;
}

[[nodiscard]] std::vector<std::pair<double, std::string>> symlog_ticks(Range1d range, const ScalePolicy& policy) {
  std::vector<std::pair<double, std::string>> ticks;
  const auto low_t = scale_value(range.low, policy);
  const auto high_t = scale_value(range.high, policy);
  if (!low_t.has_value() || !high_t.has_value() || *high_t <= *low_t) {
    return ticks;
  }
  const auto transformed_ticks = linear_ticks({*low_t, *high_t}, 7);
  ticks.reserve(transformed_ticks.size());
  const double eps = std::max(1.0e-9, 1.0e-7 * std::max(std::abs(range.low), std::abs(range.high)));
  for (const auto& [transformed, _] : transformed_ticks) {
    const auto value = unscale_value(transformed, policy);
    if (!value.has_value() || !std::isfinite(*value)) {
      continue;
    }
    if (!ticks.empty() && std::abs(ticks.back().first - *value) <= eps) {
      continue;
    }
    ticks.emplace_back(*value, format_tick_value(*value));
  }
  return ticks;
}

[[nodiscard]] std::vector<std::pair<double, std::string>> harmonize_auto_tick_labels(
    std::vector<std::pair<double, std::string>> ticks,
    std::optional<int> forced_decimals = std::nullopt) {
  if (ticks.empty()) {
    return ticks;
  }

  std::vector<double> values;
  values.reserve(ticks.size());
  double max_abs = 0.0;
  double min_nonzero_abs = std::numeric_limits<double>::infinity();
  for (const auto& [value, _] : ticks) {
    if (!std::isfinite(value)) {
      continue;
    }
    const double abs_value = std::abs(value);
    max_abs = std::max(max_abs, abs_value);
    if (abs_value > 0.0) {
      min_nonzero_abs = std::min(min_nonzero_abs, abs_value);
    }
    values.push_back(value);
  }
  if (values.empty()) {
    return ticks;
  }

  const auto [min_it, max_it] = std::minmax_element(values.begin(), values.end());
  const double span = std::max(*max_it - *min_it, 1.0e-12);
  const double eps = std::max(1.0e-9, span * 1.0e-8);

  if (forced_decimals.has_value()) {
    const int decimals = std::clamp(*forced_decimals, 0, 8);
    for (auto& [value, label] : ticks) {
      const double snapped = std::abs(value) <= eps ? 0.0 : value;
      std::ostringstream out;
      out << std::fixed << std::setprecision(decimals) << snapped;
      label = out.str();
    }
    return ticks;
  }

  if (max_abs >= 1.0e4 || (std::isfinite(min_nonzero_abs) && min_nonzero_abs < 1.0e-3)) {
    return ticks;
  }

  const auto is_effective_integer = [eps](double value) {
    return std::abs(value - std::round(value)) <= eps;
  };
  const bool has_fractional_ticks =
      std::any_of(values.begin(), values.end(), [&](double value) { return !is_effective_integer(value); });
  if (!has_fractional_ticks) {
    return ticks;
  }

  std::sort(values.begin(), values.end());
  values.erase(std::unique(values.begin(), values.end(), [eps](double lhs, double rhs) {
                 return std::abs(lhs - rhs) <= eps;
               }),
               values.end());

  double min_step = std::numeric_limits<double>::infinity();
  for (std::size_t i = 1; i < values.size(); ++i) {
    const double delta = values[i] - values[i - 1];
    if (delta > eps) {
      min_step = std::min(min_step, delta);
    }
  }

  int decimals = 1;
  if (std::isfinite(min_step)) {
    int detected = 6;
    for (int d = 0; d <= 6; ++d) {
      const double scaled = min_step * std::pow(10.0, static_cast<double>(d));
      const double rounded = std::round(scaled);
      const double tolerance = 1.0e-6 * std::max(1.0, std::abs(scaled));
      if (std::abs(scaled - rounded) <= tolerance) {
        detected = d;
        break;
      }
    }
    decimals = std::clamp(detected, 1, 3);
  }

  for (auto& [value, label] : ticks) {
    const double snapped = std::abs(value) <= eps ? 0.0 : value;
    std::ostringstream out;
    out << std::fixed << std::setprecision(decimals) << snapped;
    label = out.str();
  }
  return ticks;
}

[[nodiscard]] std::vector<std::pair<double, std::string>> ticks_for(const AxisState& axis, Range1d range) {
  if (axis.ticks.has_value()) {
    const auto positions = to_f64(flatten_f64(axis.ticks->positions));
    std::vector<std::pair<double, std::string>> ticks;
    const auto count = std::min(positions.size(), axis.ticks->labels.size());
    ticks.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
      ticks.emplace_back(positions[i], axis.ticks->labels[i]);
    }
    return ticks;
  }
  std::vector<std::pair<double, std::string>> ticks;
  if (axis.scale.kind == ScaleKind::Log10) {
    ticks = log_ticks(range);
  } else if (axis.scale.kind == ScaleKind::SymLog) {
    ticks = symlog_ticks(range, axis.scale);
  } else {
    ticks = linear_ticks(range);
  }

  if (axis.tick_format == TickFormat::Auto) {
    if (axis.scale.kind == ScaleKind::Linear || axis.scale.kind == ScaleKind::SymLog) {
      ticks = harmonize_auto_tick_labels(std::move(ticks), axis.auto_tick_decimals);
    }
  } else {
    for (auto& [value, label] : ticks) {
      label = format_tick_value(value, axis.tick_format);
    }
  }
  return ticks;
}

[[nodiscard]] std::vector<std::pair<double, std::string>> clamp_ticks(
    const std::vector<std::pair<double, std::string>>& ticks,
    Range1d range,
    const AxisState& axis) {
  std::vector<std::pair<double, std::string>> clipped;
  clipped.reserve(ticks.size() + 2);
  const double span = std::max(range.high - range.low, 1e-12);
  const double eps = std::max(1e-9 * span, 1e-9);
  const auto make_label = [&](double value) {
    return format_tick_value(value, axis.tick_format);
  };
  for (const auto& [value, label] : ticks) {
    if (!std::isfinite(value)) {
      continue;
    }
    if (value < range.low - eps || value > range.high + eps) {
      continue;
    }
    clipped.emplace_back(value, label);
  }

  std::stable_sort(clipped.begin(), clipped.end(), [](const auto& lhs, const auto& rhs) {
    return lhs.first < rhs.first;
  });

  std::vector<std::pair<double, std::string>> unique_ticks;
  unique_ticks.reserve(clipped.size());
  for (const auto& tick : clipped) {
    if (!unique_ticks.empty() && std::abs(unique_ticks.back().first - tick.first) <= eps) {
      continue;
    }
    unique_ticks.push_back(tick);
  }
  clipped = std::move(unique_ticks);

  if (clipped.empty()) {
    clipped.emplace_back(range.low, make_label(range.low));
    if (!axis.ticks.has_value() && range.high > range.low + eps) {
      clipped.emplace_back(range.high, make_label(range.high));
    }
    return clipped;
  }

  if (!axis.ticks.has_value() && axis.tick_format != TickFormat::Auto) {
    if (std::abs(clipped.front().first - range.low) > eps) {
      clipped.insert(clipped.begin(), {range.low, make_label(range.low)});
    } else {
      clipped.front().first = range.low;
      clipped.front().second = make_label(range.low);
    }
    if (range.high > range.low + eps) {
      if (std::abs(clipped.back().first - range.high) > eps) {
        clipped.emplace_back(range.high, make_label(range.high));
      } else {
        clipped.back().first = range.high;
        clipped.back().second = make_label(range.high);
      }
    }
  }
  if (axis.tick_format == TickFormat::Auto &&
      (axis.scale.kind == ScaleKind::Linear || axis.scale.kind == ScaleKind::SymLog)) {
    return harmonize_auto_tick_labels(std::move(clipped), axis.auto_tick_decimals);
  }
  return clipped;
}

[[nodiscard]] std::vector<double> uniform_grid_positions(Range1d range, int cells = 6) {
  const int clamped_cells = std::max(1, cells);
  const double span = std::max(range.high - range.low, 1e-12);
  std::vector<double> values;
  values.reserve(static_cast<std::size_t>(clamped_cells + 1));
  for (int i = 0; i <= clamped_cells; ++i) {
    const double t = static_cast<double>(i) / static_cast<double>(clamped_cells);
    values.push_back(range.low + span * t);
  }
  return values;
}

[[nodiscard]] std::vector<double> minor_tick_values(
    const AxisState& axis,
    Range1d range,
    const std::vector<std::pair<double, std::string>>& major_ticks) {
  std::vector<double> minor;
  if (!std::isfinite(range.low) || !std::isfinite(range.high) || !(range.high > range.low)) {
    return minor;
  }

  const double span = std::max(range.high - range.low, 1e-12);
  const double eps = std::max(1.0e-9, span * 1.0e-7);
  std::vector<double> major;
  major.reserve(major_ticks.size());
  for (const auto& [value, _] : major_ticks) {
    if (!std::isfinite(value)) {
      continue;
    }
    if (value < range.low - eps || value > range.high + eps) {
      continue;
    }
    major.push_back(value);
  }
  std::sort(major.begin(), major.end());
  major.erase(std::unique(major.begin(), major.end(), [eps](double lhs, double rhs) {
                return std::abs(lhs - rhs) <= eps;
              }),
              major.end());

  const auto is_major = [&](double value) {
    for (const double major_value : major) {
      if (std::abs(major_value - value) <= eps) {
        return true;
      }
    }
    return false;
  };
  const auto push_minor = [&](double value) {
    if (!std::isfinite(value)) {
      return;
    }
    if (value < range.low + eps || value > range.high - eps) {
      return;
    }
    if (is_major(value)) {
      return;
    }
    for (const double existing : minor) {
      if (std::abs(existing - value) <= eps) {
        return;
      }
    }
    minor.push_back(value);
  };

  if (axis.scale.kind == ScaleKind::Log10) {
    if (range.low <= 0.0 || range.high <= 0.0) {
      return minor;
    }
    const int low_pow = static_cast<int>(std::floor(std::log10(range.low)));
    const int high_pow = static_cast<int>(std::ceil(std::log10(range.high)));
    for (int power = low_pow; power < high_pow; ++power) {
      const double base = std::pow(10.0, static_cast<double>(power));
      for (int m = 2; m <= 9; ++m) {
        push_minor(base * static_cast<double>(m));
      }
      if (minor.size() >= 96) {
        break;
      }
    }
    std::sort(minor.begin(), minor.end());
    return minor;
  }

  if (major.size() < 2) {
    const auto fallback = linear_ticks(range, 9);
    for (const auto& [value, _] : fallback) {
      push_minor(value);
    }
    std::sort(minor.begin(), minor.end());
    return minor;
  }

  for (std::size_t i = 1; i < major.size(); ++i) {
    const double lo = major[i - 1];
    const double hi = major[i];
    if (!(hi > lo + eps)) {
      continue;
    }
    const auto lo_t = scale_value(lo, axis.scale);
    const auto hi_t = scale_value(hi, axis.scale);
    if (!lo_t.has_value() || !hi_t.has_value() || !(*hi_t > *lo_t)) {
      continue;
    }
    for (int div = 1; div < 5; ++div) {
      const double transformed = *lo_t + (*hi_t - *lo_t) * (static_cast<double>(div) / 5.0);
      const auto value = unscale_value(transformed, axis.scale);
      if (!value.has_value()) {
        continue;
      }
      push_minor(*value);
      if (minor.size() >= 128) {
        break;
      }
    }
    if (minor.size() >= 128) {
      break;
    }
  }

  std::sort(minor.begin(), minor.end());
  return minor;
}

[[nodiscard]] SeriesPairData normalize_pairs(const Series& first, const Series& second) {
  SeriesPairData out{to_f64(first), to_f64(second)};
  if (out.first.empty() || out.second.empty()) {
    return out;
  }
  if (out.first.size() == 1 && out.second.size() > 1) {
    out.first.assign(out.second.size(), out.first.front());
  } else if (out.second.size() == 1 && out.first.size() > 1) {
    out.second.assign(out.first.size(), out.second.front());
  }
  const auto count = std::min(out.first.size(), out.second.size());
  out.first.resize(count);
  out.second.resize(count);
  return out;
}

[[nodiscard]] std::vector<double> broadcast_optional_series(
    const std::optional<Series>& values,
    std::size_t count,
    double fallback) {
  std::vector<double> out(count, fallback);
  if (!values.has_value()) {
    return out;
  }
  const auto raw = to_f64(*values);
  if (raw.empty()) {
    return out;
  }
  if (raw.size() == 1 && count > 1) {
    out.assign(count, raw.front());
    return out;
  }
  const auto clipped = std::min(count, raw.size());
  std::copy(raw.begin(), raw.begin() + static_cast<std::ptrdiff_t>(clipped), out.begin());
  return out;
}

[[nodiscard]] CloudPoints normalize_cloud(const CloudData& data) {
  const auto xy = normalize_pairs(data.x, data.y);
  const auto count = std::min(xy.first.size(), xy.second.size());
  CloudPoints cloud;
  cloud.x = xy.first;
  cloud.y = xy.second;
  cloud.x.resize(count);
  cloud.y.resize(count);
  cloud.size = broadcast_optional_series(data.size, count, 18.0);
  if (data.color.has_value()) {
    cloud.color = broadcast_optional_series(data.color, count, 0.0);
  }
  return cloud;
}

[[nodiscard]] Cloud3Points normalize_cloud3(const Cloud3Data& data) {
  const auto xy = normalize_pairs(data.x, data.y);
  std::vector<double> z = to_f64(data.z);
  if (z.size() == 1 && xy.first.size() > 1) {
    z.assign(xy.first.size(), z.front());
  }
  const auto count = std::min({xy.first.size(), xy.second.size(), z.size()});
  Cloud3Points cloud;
  cloud.x = xy.first;
  cloud.y = xy.second;
  cloud.z = std::move(z);
  cloud.x.resize(count);
  cloud.y.resize(count);
  cloud.z.resize(count);
  cloud.size = broadcast_optional_series(data.size, count, 18.0);
  if (data.color.has_value()) {
    cloud.color = broadcast_optional_series(data.color, count, 0.0);
  }
  return cloud;
}

[[nodiscard]] std::optional<RibbonGeometry> normalize_ribbon(const RibbonData& data) {
  const Series y_matrix = as_column_matrix(data.y);
  if (y_matrix.dim() != 2 || y_matrix.sizes()[0] <= 0 || y_matrix.sizes()[1] <= 0) {
    return std::nullopt;
  }

  const std::int64_t rows = y_matrix.sizes()[0];
  const std::int64_t cols = y_matrix.sizes()[1];
  const Series x_matrix = broadcast_plot_matrix(data.x, rows, cols);
  const Series top = (cols > 1 ? y_matrix.cumsum(1) : y_matrix).add(data.base);
  const Series bottom = cols > 1
                            ? top.sub(y_matrix)
                            : pluss::full({rows, cols}, data.base, ScalarType::Float64);

  return RibbonGeometry{
      rows,
      cols,
      to_f64(x_matrix),
      to_f64(top),
      to_f64(bottom),
  };
}

[[nodiscard]] std::optional<WhiskGeometry> normalize_whisk(const WhiskData& data) {
  const Series y_matrix = as_column_matrix(data.y);
  if (y_matrix.dim() != 2 || y_matrix.sizes()[0] <= 0 || y_matrix.sizes()[1] <= 0) {
    return std::nullopt;
  }

  const std::int64_t rows = y_matrix.sizes()[0];
  const std::int64_t cols = y_matrix.sizes()[1];
  const Series x_matrix = broadcast_plot_matrix(data.x, rows, cols);
  const Series xneg = broadcast_delta_matrix(data.xneg, rows, cols);
  const Series xpos = broadcast_delta_matrix(data.xpos, rows, cols);
  const Series yneg = broadcast_delta_matrix(data.yneg, rows, cols);
  const Series ypos = broadcast_delta_matrix(data.ypos, rows, cols);

  return WhiskGeometry{
      rows,
      cols,
      to_f64(x_matrix),
      to_f64(y_matrix),
      to_f64(x_matrix.sub(xneg)),
      to_f64(x_matrix.add(xpos)),
      to_f64(y_matrix.sub(yneg)),
      to_f64(y_matrix.add(ypos)),
      data.xneg.has_value() || data.xpos.has_value(),
      data.yneg.has_value() || data.ypos.has_value(),
  };
}

[[nodiscard]] std::vector<WedgeSlice> wedge_slices(const WedgeData& data) {
  const auto values = to_f64(flatten_f64(data.values));
  std::vector<double> explode(values.size(), 0.0);
  if (data.explode.has_value()) {
    explode = to_f64(flatten_f64(*data.explode));
    if (explode.size() == 1 && values.size() > 1) {
      explode.assign(values.size(), explode.front());
    }
    explode.resize(values.size(), 0.0);
  }
  const double total = std::accumulate(values.begin(), values.end(), 0.0);
  std::vector<WedgeSlice> slices;
  slices.reserve(values.size());
  double angle = -0.5 * kPi;
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (!(values[i] > 0.0)) {
      continue;
    }
    const double fraction = values[i] / total;
    const double span = 2.0 * kPi * fraction;
    std::string label;
    if (i < data.labels.size()) {
      label = data.labels[i];
    } else {
      std::ostringstream out;
      out << std::fixed << std::setprecision(1) << fraction * 100.0 << "%";
      label = out.str();
    }
    slices.push_back(WedgeSlice{angle, angle + span, explode[i], std::move(label), fraction});
    angle += span;
  }
  return slices;
}

[[nodiscard]] BucketGeometry bucket_geometry(const BucketData& data) {
  BucketGeometry geometry;
  const Series values = flatten_f64(data.values);
  double low = data.range.has_value() ? data.range->low : scalar_f64(values.min());
  double high = data.range.has_value() ? data.range->high : scalar_f64(values.max());
  if (!(high > low)) {
    low -= 0.5;
    high += 0.5;
  }
  geometry.low = low;
  geometry.high = high;
  geometry.counts = to_f64(values.histc(data.bins, low, high));
  return geometry;
}

[[nodiscard]] std::vector<QuartetSummary> quartet_summaries(const QuartetData& data) {
  std::vector<QuartetSummary> summaries;
  const Series values = flatten_f64(data.values);

  auto append_summary = [&](double group, const Series& sample) {
    if (sample.numel() == 0) {
      return;
    }
    QuartetSummary summary;
    summary.group = group;
    summary.q1 = scalar_f64(sample.quantile(0.25));
    summary.median = scalar_f64(sample.median());
    summary.q3 = scalar_f64(sample.quantile(0.75));
    const double iqr = summary.q3 - summary.q1;
    const double lower_fence = summary.q1 - 1.5 * iqr;
    const double upper_fence = summary.q3 + 1.5 * iqr;
    const Series inlier_mask = sample.ge(lower_fence).logical_and(sample.le(upper_fence));
    const Series inliers = sample.masked_select(inlier_mask);
    summary.low_whisker = inliers.numel() > 0 ? scalar_f64(inliers.min()) : summary.q1;
    summary.high_whisker = inliers.numel() > 0 ? scalar_f64(inliers.max()) : summary.q3;
    const auto low_outliers = to_f64(sample.masked_select(sample.lt(lower_fence)));
    const auto high_outliers = to_f64(sample.masked_select(sample.gt(upper_fence)));
    summary.outliers.reserve(low_outliers.size() + high_outliers.size());
    summary.outliers.insert(summary.outliers.end(), low_outliers.begin(), low_outliers.end());
    summary.outliers.insert(summary.outliers.end(), high_outliers.begin(), high_outliers.end());
    summaries.push_back(std::move(summary));
  };

  if (!data.group.has_value()) {
    append_summary(1.0, values);
    return summaries;
  }

  const Series group = flatten_f64(*data.group);
  const auto unique_groups = to_f64(group.unique(true));
  summaries.reserve(unique_groups.size());
  for (const double group_value : unique_groups) {
    const Series mask = group.eq(group_value);
    append_summary(group_value, values.masked_select(mask));
  }
  return summaries;
}

[[nodiscard]] std::vector<ViolinShape> violin_shapes(const ViolinData& data) {
  constexpr std::size_t kResolution = 120;
  constexpr double kInvSqrt2Pi = 0.3989422804014327;
  struct ViolinKernel {
    std::vector<double> y{};
    std::vector<double> density{};
    std::vector<double> sample{};
    double group{1.0};
    double q1{0.0};
    double median{0.0};
    double q3{0.0};
    double low_whisker{0.0};
    double high_whisker{0.0};
    std::size_t count{0};
  };

  std::vector<ViolinKernel> kernels;
  const Series values = flatten_f64(data.values);

  auto append_kernel = [&](double group, const Series& sample) {
    const auto sample_values = to_f64(sample);
    if (sample_values.empty()) {
      return;
    }

    const std::size_t n = sample_values.size();
    const double q1 = scalar_f64(sample.quantile(0.25));
    const double median = scalar_f64(sample.median());
    const double q3 = scalar_f64(sample.quantile(0.75));
    const double min_value = scalar_f64(sample.min());
    const double max_value = scalar_f64(sample.max());
    const double span = std::max(max_value - min_value, 1e-9);

    double mean = 0.0;
    double m2 = 0.0;
    std::size_t count = 0;
    for (const double value : sample_values) {
      ++count;
      const double delta = value - mean;
      mean += delta / static_cast<double>(count);
      const double delta2 = value - mean;
      m2 += delta * delta2;
    }
    const double variance = count > 0 ? (m2 / static_cast<double>(count)) : 0.0;
    const double stddev = std::sqrt(std::max(variance, 0.0));
    const double iqr = std::max(q3 - q1, 0.0);
    const double robust_sigma = iqr > 0.0 ? iqr / 1.34 : stddev;
    const double sigma = std::max(robust_sigma, stddev);
    double bandwidth = 0.9 * sigma * std::pow(static_cast<double>(n), -0.2);
    if (!std::isfinite(bandwidth) || bandwidth <= 1e-6) {
      bandwidth = std::max(span / 22.0, 0.05);
    }
    bandwidth = std::max(bandwidth * std::max(1e-4, data.options.bw_adjust), span / 64.0);

    const double cut_extent = std::max(0.0, data.options.cut) * bandwidth;
    const double low = min_value - cut_extent;
    const double high = max_value + cut_extent;
    const double render_span = std::max(high - low, 1e-9);

    ViolinKernel kernel;
    kernel.group = group;
    kernel.q1 = q1;
    kernel.median = median;
    kernel.q3 = q3;
    kernel.low_whisker = min_value;
    kernel.high_whisker = max_value;
    kernel.sample = sample_values;
    kernel.count = n;
    kernel.y.resize(kResolution);
    kernel.density.assign(kResolution, 0.0);

    for (std::size_t i = 0; i < kResolution; ++i) {
      const double y = low + render_span * static_cast<double>(i) / static_cast<double>(kResolution - 1);
      kernel.y[i] = y;
      double sum = 0.0;
      for (const double value : sample_values) {
        const double u = (y - value) / bandwidth;
        sum += std::exp(-0.5 * u * u);
      }
      kernel.density[i] = sum / (static_cast<double>(n) * bandwidth) * kInvSqrt2Pi;
    }

    if (kernel.density.size() > 4) {
      std::vector<double> smoothed(kernel.density.size(), 0.0);
      const auto smooth_once = [&](std::vector<double>& src, std::vector<double>& dst) {
        std::fill(dst.begin(), dst.end(), 0.0);
        dst.front() = src.front();
        dst.back() = src.back();
        for (std::size_t i = 1; i + 1 < src.size(); ++i) {
          dst[i] = 0.25 * src[i - 1] + 0.5 * src[i] + 0.25 * src[i + 1];
        }
        src.swap(dst);
      };
      smooth_once(kernel.density, smoothed);
      smooth_once(kernel.density, smoothed);
    }
    kernels.push_back(std::move(kernel));
  };

  if (!data.group.has_value()) {
    append_kernel(1.0, values);
  } else {
    const Series group = flatten_f64(*data.group);
    const auto unique_groups = to_f64(group.unique(true));
    kernels.reserve(unique_groups.size());
    for (const double group_value : unique_groups) {
      append_kernel(group_value, values.masked_select(group.eq(group_value)));
    }
  }
  if (kernels.empty()) {
    return {};
  }

  const double max_count =
      static_cast<double>(std::max_element(
                              kernels.begin(),
                              kernels.end(),
                              [](const ViolinKernel& a, const ViolinKernel& b) { return a.count < b.count; })
                              ->count);

  std::vector<std::vector<double>> normalized(kernels.size());
  double global_peak = 0.0;
  for (std::size_t k = 0; k < kernels.size(); ++k) {
    const auto& density = kernels[k].density;
    auto& out = normalized[k];
    out.assign(density.size(), 0.0);
    if (density.empty()) {
      continue;
    }

    if (data.options.scale == ViolinScale::Width) {
      const double peak = *std::max_element(density.begin(), density.end());
      const double inv_peak = peak > 0.0 ? (1.0 / peak) : 0.0;
      std::transform(density.begin(), density.end(), out.begin(), [inv_peak](double value) { return value * inv_peak; });
    } else {
      double area = 0.0;
      for (std::size_t i = 1; i < density.size(); ++i) {
        const double dy = kernels[k].y[i] - kernels[k].y[i - 1];
        area += 0.5 * (density[i - 1] + density[i]) * std::max(1e-12, dy);
      }
      const double inv_area = area > 0.0 ? (1.0 / area) : 0.0;
      std::transform(density.begin(), density.end(), out.begin(), [inv_area](double value) { return value * inv_area; });
      if (data.options.scale == ViolinScale::Count && max_count > 0.0) {
        const double count_gain = static_cast<double>(kernels[k].count) / max_count;
        std::transform(out.begin(), out.end(), out.begin(), [count_gain](double value) { return value * count_gain; });
      }
    }

    const double local_peak = *std::max_element(out.begin(), out.end());
    global_peak = std::max(global_peak, local_peak);
  }

  const double half_width_target = std::clamp(0.5 * data.options.width, 0.02, 0.9);
  const double width_scale = global_peak > 0.0 ? (half_width_target / global_peak) : 0.0;

  std::vector<ViolinShape> shapes;
  shapes.reserve(kernels.size());
  for (std::size_t k = 0; k < kernels.size(); ++k) {
    ViolinShape shape;
    shape.group = kernels[k].group;
    shape.q1 = kernels[k].q1;
    shape.median = kernels[k].median;
    shape.q3 = kernels[k].q3;
    shape.low_whisker = kernels[k].low_whisker;
    shape.high_whisker = kernels[k].high_whisker;
    shape.inner = data.options.inner;
    shape.sample = std::move(kernels[k].sample);
    shape.y = std::move(kernels[k].y);
    shape.left_half_width.assign(normalized[k].size(), 0.0);
    shape.right_half_width.assign(normalized[k].size(), 0.0);

    for (std::size_t i = 0; i < normalized[k].size(); ++i) {
      const double width = std::max(0.0, normalized[k][i] * width_scale);
      if (data.options.side == ViolinSide::Left) {
        shape.left_half_width[i] = width;
      } else if (data.options.side == ViolinSide::Right) {
        shape.right_half_width[i] = width;
      } else {
        shape.left_half_width[i] = width;
        shape.right_half_width[i] = width;
      }
    }
    shapes.push_back(std::move(shape));
  }
  return shapes;
}

struct ViolinHalfWidths {
  double left{0.0};
  double right{0.0};
};

[[nodiscard]] ViolinHalfWidths widths_at_y(const ViolinShape& shape, double y) {
  if (shape.y.empty() || shape.left_half_width.empty() || shape.right_half_width.empty()) {
    return {};
  }
  if (y <= shape.y.front()) {
    return {shape.left_half_width.front(), shape.right_half_width.front()};
  }
  if (y >= shape.y.back()) {
    return {shape.left_half_width.back(), shape.right_half_width.back()};
  }
  const auto it = std::lower_bound(shape.y.begin(), shape.y.end(), y);
  if (it == shape.y.end()) {
    return {shape.left_half_width.back(), shape.right_half_width.back()};
  }
  const auto idx = static_cast<std::size_t>(std::distance(shape.y.begin(), it));
  if (idx == 0) {
    return {shape.left_half_width.front(), shape.right_half_width.front()};
  }
  const double y0 = shape.y[idx - 1];
  const double y1 = shape.y[idx];
  const double t = (y - y0) / std::max(y1 - y0, 1e-12);
  return {
      shape.left_half_width[idx - 1] + (shape.left_half_width[idx] - shape.left_half_width[idx - 1]) * t,
      shape.right_half_width[idx - 1] + (shape.right_half_width[idx] - shape.right_half_width[idx - 1]) * t,
  };
}

[[nodiscard]] std::optional<std::pair<Series, Series>> surface_coordinate_grids(
    const Series& x,
    const Series& y,
    std::int64_t rows,
    std::int64_t cols) {
  const Series x_values = x.to(ScalarType::Float64);
  const Series y_values = y.to(ScalarType::Float64);
  const std::int64_t grid_size = rows * cols;

  if (series_is_vector(x_values) && x_values.numel() == cols &&
      series_is_vector(y_values) && y_values.numel() == rows) {
    const Series x_row = x_values.reshape({1, cols});
    const Series y_col = y_values.reshape({rows, 1});
    const std::array<std::int64_t, 2> x_tile{rows, 1};
    const std::array<std::int64_t, 2> y_tile{1, cols};
    return std::pair<Series, Series>{
        x_row.tile(std::span<const std::int64_t>(x_tile.data(), x_tile.size())),
        y_col.tile(std::span<const std::int64_t>(y_tile.data(), y_tile.size())),
    };
  }

  if (x_values.numel() == grid_size && y_values.numel() == grid_size) {
    return std::pair<Series, Series>{
        x_values.reshape({rows, cols}),
        y_values.reshape({rows, cols}),
    };
  }

  return std::nullopt;
}

[[nodiscard]] std::optional<SurfaceGrid> normalize_surface_grid(const SurfaceData& data) {
  const Series z_values = data.z.to(ScalarType::Float64);
  if (z_values.dim() != 2 || z_values.sizes()[0] <= 0 || z_values.sizes()[1] <= 0) {
    return std::nullopt;
  }

  SurfaceGrid grid;
  grid.rows = z_values.sizes()[0];
  grid.cols = z_values.sizes()[1];
  grid.z = to_f64(z_values.reshape({grid.rows * grid.cols}));

  const auto coordinates = surface_coordinate_grids(data.x, data.y, grid.rows, grid.cols);
  if (!coordinates.has_value()) {
    return std::nullopt;
  }
  grid.x = to_f64(coordinates->first.reshape({grid.rows * grid.cols}));
  grid.y = to_f64(coordinates->second.reshape({grid.rows * grid.cols}));

  if (data.color.has_value()) {
    const Series color_values = data.color->to(ScalarType::Float64);
    if (color_values.numel() == grid.rows * grid.cols) {
      grid.color = to_f64(color_values.reshape({grid.rows * grid.cols}));
    }
  }

  return grid;
}

[[nodiscard]] std::optional<SurfaceGrid> normalize_surface_grid(const FieldmapData& data) {
  const Series z_values = data.z.to(ScalarType::Float64);
  if (z_values.dim() != 2 || z_values.sizes()[0] <= 0 || z_values.sizes()[1] <= 0) {
    return std::nullopt;
  }

  SurfaceGrid grid;
  grid.rows = z_values.sizes()[0];
  grid.cols = z_values.sizes()[1];
  grid.z = to_f64(z_values.reshape({grid.rows * grid.cols}));

  const auto coordinates = surface_coordinate_grids(data.x, data.y, grid.rows, grid.cols);
  if (!coordinates.has_value()) {
    return std::nullopt;
  }
  grid.x = to_f64(coordinates->first.reshape({grid.rows * grid.cols}));
  grid.y = to_f64(coordinates->second.reshape({grid.rows * grid.cols}));

  if (data.color.has_value()) {
    const Series color_values = data.color->to(ScalarType::Float64);
    if (color_values.numel() == grid.rows * grid.cols) {
      grid.color = to_f64(color_values.reshape({grid.rows * grid.cols}));
    }
  }

  return grid;
}

[[nodiscard]] std::optional<SurfaceGrid> normalize_surface_grid(const ContourData& data) {
  const Series z_values = data.z.to(ScalarType::Float64);
  if (z_values.dim() != 2 || z_values.sizes()[0] <= 0 || z_values.sizes()[1] <= 0) {
    return std::nullopt;
  }

  SurfaceGrid grid;
  grid.rows = z_values.sizes()[0];
  grid.cols = z_values.sizes()[1];
  grid.z = to_f64(z_values.reshape({grid.rows * grid.cols}));

  const auto coordinates = surface_coordinate_grids(data.x, data.y, grid.rows, grid.cols);
  if (!coordinates.has_value()) {
    return std::nullopt;
  }
  grid.x = to_f64(coordinates->first.reshape({grid.rows * grid.cols}));
  grid.y = to_f64(coordinates->second.reshape({grid.rows * grid.cols}));

  return grid;
}

[[nodiscard]] std::optional<SurfaceGrid> normalize_surface_grid(const Spire3Data& data) {
  const Series z_values = data.z.to(ScalarType::Float64);
  if (z_values.dim() != 2 || z_values.sizes()[0] <= 0 || z_values.sizes()[1] <= 0) {
    return std::nullopt;
  }

  SurfaceGrid grid;
  grid.rows = z_values.sizes()[0];
  grid.cols = z_values.sizes()[1];
  grid.z = to_f64(z_values.reshape({grid.rows * grid.cols}));

  const auto coordinates = surface_coordinate_grids(data.x, data.y, grid.rows, grid.cols);
  if (!coordinates.has_value()) {
    return std::nullopt;
  }
  grid.x = to_f64(coordinates->first.reshape({grid.rows * grid.cols}));
  grid.y = to_f64(coordinates->second.reshape({grid.rows * grid.cols}));

  if (data.color.has_value()) {
    const Series color_values = data.color->to(ScalarType::Float64);
    if (color_values.numel() == grid.rows * grid.cols) {
      grid.color = to_f64(color_values.reshape({grid.rows * grid.cols}));
    }
  }

  return grid;
}

[[nodiscard]] std::optional<VolumeField> normalize_volume_field(const IsoShellData& data) {
  const Series values = data.values.to(ScalarType::Float64);
  if (values.dim() != 3) {
    return std::nullopt;
  }
  VolumeField field;
  field.nx = values.sizes()[0];
  field.ny = values.sizes()[1];
  field.nz = values.sizes()[2];
  if (field.nx <= 1 || field.ny <= 1 || field.nz <= 1) {
    return std::nullopt;
  }

  field.x = to_f64(flatten_f64(data.x));
  field.y = to_f64(flatten_f64(data.y));
  field.z = to_f64(flatten_f64(data.z));
  if (field.x.size() != static_cast<std::size_t>(field.nx) ||
      field.y.size() != static_cast<std::size_t>(field.ny) ||
      field.z.size() != static_cast<std::size_t>(field.nz)) {
    return std::nullopt;
  }
  field.values = to_f64(values.reshape({field.nx * field.ny * field.nz}));
  if (field.values.size() != static_cast<std::size_t>(field.nx * field.ny * field.nz)) {
    return std::nullopt;
  }
  return field;
}

struct FiniteMinMax {
  bool has_value{false};
  double min{0.0};
  double max{0.0};
};

[[nodiscard]] FiniteMinMax finite_minmax(std::span<const double> values) {
  FiniteMinMax out;
  for (const double value : values) {
    if (!std::isfinite(value)) {
      continue;
    }
    if (!out.has_value) {
      out.has_value = true;
      out.min = value;
      out.max = value;
      continue;
    }
    out.min = std::min(out.min, value);
    out.max = std::max(out.max, value);
  }
  return out;
}

[[nodiscard]] std::vector<double> unique_sorted_levels(std::vector<double> levels) {
  std::sort(levels.begin(), levels.end());
  levels.erase(
      std::unique(
          levels.begin(),
          levels.end(),
          [](double lhs, double rhs) { return std::abs(lhs - rhs) <= 1e-12; }),
      levels.end());
  return levels;
}

[[nodiscard]] std::vector<double> auto_contour_levels(const SurfaceGrid& grid, int level_count) {
  const auto range = finite_minmax(grid.z);
  if (!range.has_value) {
    return {};
  }
  if (range.min == range.max) {
    return {range.min};
  }

  const int count = std::max(level_count, 1);
  std::vector<double> levels;
  levels.reserve(static_cast<std::size_t>(count));
  const double span = range.max - range.min;
  for (int i = 1; i <= count; ++i) {
    levels.push_back(range.min + span * static_cast<double>(i) / static_cast<double>(count + 1));
  }
  return levels;
}

[[nodiscard]] std::vector<double> auto_isoshell_levels(const VolumeField& field, int level_count) {
  const auto range = finite_minmax(field.values);
  if (!range.has_value) {
    return {};
  }
  if (range.min == range.max) {
    return {range.min};
  }
  const int count = std::max(level_count, 1);
  std::vector<double> levels;
  levels.reserve(static_cast<std::size_t>(count));
  const double span = range.max - range.min;
  for (int i = 1; i <= count; ++i) {
    levels.push_back(range.min + span * static_cast<double>(i) / static_cast<double>(count + 1));
  }
  return levels;
}

[[nodiscard]] std::vector<double> isoshell_levels(const IsoShellData& data, const VolumeField& field) {
  if (!data.levels.empty()) {
    return unique_sorted_levels(data.levels);
  }
  return auto_isoshell_levels(field, data.auto_levels);
}

[[nodiscard]] std::vector<double> contour_levels(const ContourData& data, const SurfaceGrid& grid) {
  if (!data.levels.empty()) {
    return unique_sorted_levels(data.levels);
  }
  return auto_contour_levels(grid, data.auto_levels);
}

[[nodiscard]] std::size_t volume_index(
    const VolumeField& field,
    std::int64_t ix,
    std::int64_t iy,
    std::int64_t iz) noexcept {
  return static_cast<std::size_t>((ix * field.ny + iy) * field.nz + iz);
}

[[nodiscard]] Point3d interpolate_iso_point(
    Point3d a,
    Point3d b,
    double va,
    double vb,
    double level) noexcept {
  const double denom = vb - va;
  if (std::abs(denom) <= 1e-12) {
    return make_point3d(0.5 * (a.x + b.x), 0.5 * (a.y + b.y), 0.5 * (a.z + b.z));
  }
  const double t = std::clamp((level - va) / denom, 0.0, 1.0);
  return make_point3d(
      a.x + (b.x - a.x) * t,
      a.y + (b.y - a.y) * t,
      a.z + (b.z - a.z) * t);
}

[[nodiscard]] std::vector<std::array<Point3d, 3>> iso_triangles_for_tetra(
    const std::array<Point3d, 4>& points,
    const std::array<double, 4>& values,
    double level) {
  const std::array<std::pair<int, int>, 6> edges{{
      {0, 1}, {0, 2}, {0, 3}, {1, 2}, {1, 3}, {2, 3},
  }};
  std::vector<Point3d> intersections;
  intersections.reserve(4);
  auto same_point = [](Point3d a, Point3d b) {
    const double dx = a.x - b.x;
    const double dy = a.y - b.y;
    const double dz = a.z - b.z;
    return dx * dx + dy * dy + dz * dz <= 1e-16;
  };

  for (const auto [ia, ib] : edges) {
    const double va = values[static_cast<std::size_t>(ia)];
    const double vb = values[static_cast<std::size_t>(ib)];
    if (!std::isfinite(va) || !std::isfinite(vb)) {
      continue;
    }
    const bool crosses = (va <= level && vb >= level) || (vb <= level && va >= level);
    if (!crosses) {
      continue;
    }
    const Point3d point = interpolate_iso_point(
        points[static_cast<std::size_t>(ia)],
        points[static_cast<std::size_t>(ib)],
        va,
        vb,
        level);
    bool duplicate = false;
    for (const Point3d existing : intersections) {
      if (same_point(existing, point)) {
        duplicate = true;
        break;
      }
    }
    if (!duplicate) {
      intersections.push_back(point);
    }
  }

  if (intersections.size() < 3 || intersections.size() > 4) {
    return {};
  }

  Point3d centroid{};
  for (const Point3d point : intersections) {
    centroid = add(centroid, point);
  }
  centroid = scale(centroid, 1.0 / static_cast<double>(intersections.size()));
  Point3d axis_u = subtract(intersections[0], centroid);
  if (norm(axis_u) <= 1e-12 && intersections.size() > 1) {
    axis_u = subtract(intersections[1], centroid);
  }
  axis_u = normalize(axis_u);
  Point3d normal = normalize(cross(subtract(intersections[1], intersections[0]), subtract(intersections[2], intersections[0])));
  if (norm(normal) <= 1e-12) {
    return {};
  }
  Point3d axis_v = normalize(cross(normal, axis_u));
  if (norm(axis_v) <= 1e-12) {
    return {};
  }

  std::sort(
      intersections.begin(),
      intersections.end(),
      [&](Point3d lhs, Point3d rhs) {
        const Point3d vl = subtract(lhs, centroid);
        const Point3d vr = subtract(rhs, centroid);
        const double al = std::atan2(dot(vl, axis_v), dot(vl, axis_u));
        const double ar = std::atan2(dot(vr, axis_v), dot(vr, axis_u));
        return al < ar;
      });

  if (intersections.size() == 3) {
    return {{{intersections[0], intersections[1], intersections[2]}}};
  }
  return {
      {intersections[0], intersections[1], intersections[2]},
      {intersections[0], intersections[2], intersections[3]},
  };
}

[[nodiscard]] std::vector<IsoTriangle> extract_isoshell_triangles(
    const VolumeField& field,
    const std::vector<double>& levels) {
  if (levels.empty()) {
    return {};
  }
  std::vector<IsoTriangle> triangles;
  const std::array<std::array<int, 4>, 6> tetrahedra{{
      {{0, 5, 1, 6}},
      {{0, 1, 2, 6}},
      {{0, 2, 3, 6}},
      {{0, 3, 7, 6}},
      {{0, 7, 4, 6}},
      {{0, 4, 5, 6}},
  }};

  for (std::int64_t ix = 0; ix + 1 < field.nx; ++ix) {
    for (std::int64_t iy = 0; iy + 1 < field.ny; ++iy) {
      for (std::int64_t iz = 0; iz + 1 < field.nz; ++iz) {
        const std::array<std::size_t, 8> corner_index{{
            volume_index(field, ix, iy, iz),
            volume_index(field, ix + 1, iy, iz),
            volume_index(field, ix + 1, iy + 1, iz),
            volume_index(field, ix, iy + 1, iz),
            volume_index(field, ix, iy, iz + 1),
            volume_index(field, ix + 1, iy, iz + 1),
            volume_index(field, ix + 1, iy + 1, iz + 1),
            volume_index(field, ix, iy + 1, iz + 1),
        }};
        const std::array<Point3d, 8> corners{{
            make_point3d(field.x[static_cast<std::size_t>(ix)], field.y[static_cast<std::size_t>(iy)], field.z[static_cast<std::size_t>(iz)]),
            make_point3d(field.x[static_cast<std::size_t>(ix + 1)], field.y[static_cast<std::size_t>(iy)], field.z[static_cast<std::size_t>(iz)]),
            make_point3d(field.x[static_cast<std::size_t>(ix + 1)], field.y[static_cast<std::size_t>(iy + 1)], field.z[static_cast<std::size_t>(iz)]),
            make_point3d(field.x[static_cast<std::size_t>(ix)], field.y[static_cast<std::size_t>(iy + 1)], field.z[static_cast<std::size_t>(iz)]),
            make_point3d(field.x[static_cast<std::size_t>(ix)], field.y[static_cast<std::size_t>(iy)], field.z[static_cast<std::size_t>(iz + 1)]),
            make_point3d(field.x[static_cast<std::size_t>(ix + 1)], field.y[static_cast<std::size_t>(iy)], field.z[static_cast<std::size_t>(iz + 1)]),
            make_point3d(field.x[static_cast<std::size_t>(ix + 1)], field.y[static_cast<std::size_t>(iy + 1)], field.z[static_cast<std::size_t>(iz + 1)]),
            make_point3d(field.x[static_cast<std::size_t>(ix)], field.y[static_cast<std::size_t>(iy + 1)], field.z[static_cast<std::size_t>(iz + 1)]),
        }};
        const std::array<double, 8> scalars{{
            field.values[corner_index[0]],
            field.values[corner_index[1]],
            field.values[corner_index[2]],
            field.values[corner_index[3]],
            field.values[corner_index[4]],
            field.values[corner_index[5]],
            field.values[corner_index[6]],
            field.values[corner_index[7]],
        }};
        const auto [min_it, max_it] = std::minmax_element(scalars.begin(), scalars.end());
        if (!std::isfinite(*min_it) || !std::isfinite(*max_it)) {
          continue;
        }
        for (const double level : levels) {
          if (level < *min_it || level > *max_it) {
            continue;
          }
          for (const auto& tetra : tetrahedra) {
            std::array<Point3d, 4> tetra_points{};
            std::array<double, 4> tetra_values{};
            for (std::size_t i = 0; i < tetra.size(); ++i) {
              const int index = tetra[i];
              tetra_points[i] = corners[static_cast<std::size_t>(index)];
              tetra_values[i] = scalars[static_cast<std::size_t>(index)];
            }
            for (const auto& tri : iso_triangles_for_tetra(tetra_points, tetra_values, level)) {
              triangles.push_back(IsoTriangle{tri, level});
            }
          }
        }
      }
    }
  }
  return triangles;
}

[[nodiscard]] std::array<Point2d, 4> center_cell_polygon(
    const SurfaceGrid& grid,
    std::int64_t row,
    std::int64_t col) {
  const auto idx = [&](std::int64_t r, std::int64_t c) -> std::size_t {
    return static_cast<std::size_t>(r * grid.cols + c);
  };
  const auto x_at = [&](std::int64_t r, std::int64_t c) -> double {
    return grid.x[idx(r, c)];
  };
  const auto y_at = [&](std::int64_t r, std::int64_t c) -> double {
    return grid.y[idx(r, c)];
  };

  const double cx = x_at(row, col);
  const double cy = y_at(row, col);
  const double x0 = col == 0 ? cx - 0.5 * (x_at(row, std::min<std::int64_t>(col + 1, grid.cols - 1)) - cx)
                             : 0.5 * (x_at(row, col - 1) + cx);
  const double x1 = col + 1 >= grid.cols ? cx + 0.5 * (cx - x_at(row, std::max<std::int64_t>(col - 1, 0)))
                                         : 0.5 * (cx + x_at(row, col + 1));
  const double y0 = row == 0 ? cy - 0.5 * (y_at(std::min<std::int64_t>(row + 1, grid.rows - 1), col) - cy)
                             : 0.5 * (y_at(row - 1, col) + cy);
  const double y1 = row + 1 >= grid.rows ? cy + 0.5 * (cy - y_at(std::max<std::int64_t>(row - 1, 0), col))
                                         : 0.5 * (cy + y_at(row + 1, col));
  return {{{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}}};
}

[[nodiscard]] Rect inset(Rect rect, double left, double top, double right, double bottom) {
  return Rect{
      rect.x + left,
      rect.y + top,
      std::max(0.0, rect.width - left - right),
      std::max(0.0, rect.height - top - bottom),
  };
}

[[nodiscard]] double default_planar_tick_label_gap(
    bool axes_visible,
    double tick_font,
    double rotation_deg,
    bool x_axis) {
  (void)axes_visible;
  (void)x_axis;
  const double abs_rotation = std::abs(rotation_deg);
  const double tick_len = std::clamp(tick_font * 0.45, 4.0, 9.0);
  // Use a virtual tick length even when axes are hidden so bordered and borderless spacing match.
  const double base_tick_len = tick_len;
  if (abs_rotation <= 1e-9) {
    return base_tick_len + std::clamp(tick_font * 0.74, 4.0, 13.0);
  }
  const double rotation_gain = std::clamp(abs_rotation / 90.0, 0.0, 1.0);
  return base_tick_len + std::clamp(tick_font * (0.28 + 0.16 * rotation_gain), 2.0, 9.0);
}

[[nodiscard]] double resolved_planar_tick_label_gap(
    const AxisState& axis,
    bool axes_visible,
    double tick_font,
    bool x_axis) {
  if (axis.tick_label_gap.has_value()) {
    return std::max(0.0, *axis.tick_label_gap);
  }
  return default_planar_tick_label_gap(axes_visible, tick_font, axis.tick_rotation_deg.value_or(0.0), x_axis);
}

[[nodiscard]] double resolved_axis_label_gap(
    const AxisState& axis,
    bool axes_visible,
    double axis_font,
    bool x_axis) {
  if (axis.label_gap.has_value()) {
    return std::max(0.0, *axis.label_gap);
  }
  if (!axes_visible) {
    return x_axis ? std::clamp(axis_font * 0.55, 4.0, 14.0) : std::clamp(axis_font * 0.45, 4.0, 12.0);
  }
  return x_axis ? std::clamp(axis_font * 0.85, 6.0, 20.0) : std::clamp(axis_font * 0.75, 6.0, 18.0);
}

[[nodiscard]] MosaicLayout resolved_mosaic_layout(const Studio& studio) {
  if (studio.mosaic_layout().active) {
    return studio.mosaic_layout();
  }

  MosaicLayout layout{};
  for (const Pane& pane : studio.panes()) {
    if (!pane.placement().explicit_slot) {
      continue;
    }
    layout.rows = std::max(layout.rows, pane.placement().row + pane.placement().row_span);
    layout.cols = std::max(layout.cols, pane.placement().col + pane.placement().col_span);
  }
  layout.active = layout.rows > 0 && layout.cols > 0;
  return layout;
}

[[nodiscard]] std::vector<PaneLayout> layout_panes(const Studio& studio) {
  std::vector<PaneLayout> layouts;
  if (studio.panes().empty()) {
    return layouts;
  }

  const bool tight = studio.tight_layout_enabled();
  const double pad = tight ? studio.tight_layout_padding() : 0.0;
  const auto compact_or_default = [tight, pad](double relaxed, double compact) {
    return (tight ? compact : relaxed) + pad;
  };
  const auto max_tick_label_chars = [](const AxisState& axis, std::size_t fallback_chars) {
    std::size_t chars = fallback_chars;
    if (axis.ticks.has_value()) {
      for (const auto& label : axis.ticks->labels) {
        chars = std::max(chars, label.size());
      }
    }
    return chars;
  };
  const auto pane_insets = [tight, &max_tick_label_chars](const Pane& pane) {
    const double top = tight ? 44.0 : 58.0;
    double left = tight ? 28.0 : 38.0;
    double right = pane.scale_guide_enabled() ? (tight ? 78.0 : 92.0) : (tight ? 20.0 : 28.0);
    if (pane.legend_place() == KeyPlace::OutsideEast) {
      right = std::max(right, tight ? 176.0 : 212.0);
    }
    double bottom = tight ? 42.0 : 56.0;

    if (pane.kind() == PaneKind::Planar) {
      const bool axes_visible = pane.axes_visible();
      if (!axes_visible) {
        left = tight ? 12.0 : 16.0;
        bottom = tight ? 18.0 : 22.0;
        if (!pane.scale_guide_enabled()) {
          right = tight ? 12.0 : 16.0;
        }
      }
      const double tick_font = std::clamp(10.0 * pane.tick_font_scale(), 7.0, 32.0);
      const double axis_font = std::clamp(11.0 * pane.axis_label_font_scale(), 8.0, 36.0);
      const auto axis_tick_depth = [&](const AxisState& axis, std::size_t fallback_chars, bool x_axis) {
        if (!axis.tick_labels_visible) {
          return axes_visible ? std::clamp(tick_font * 0.45, 4.0, 9.0) : 0.0;
        }
        const auto chars = max_tick_label_chars(axis, fallback_chars);
        const double extent = std::max(1.0, static_cast<double>(chars)) * tick_font * 0.56;
        const double radians = std::abs(axis.tick_rotation_deg.value_or(0.0)) * kPi / 180.0;
        return resolved_planar_tick_label_gap(axis, axes_visible, tick_font, x_axis)
               + tick_font * 0.35
               + 0.5 * std::sin(radians) * extent;
      };

      const double x_depth = axis_tick_depth(pane.x_axis(), 8, true);
      const double x_label_extent = pane.x_axis().label.empty() ? 0.0 : axis_font;
      const double x_label_pad =
          pane.x_axis().label.empty() ? 6.0 : resolved_axis_label_gap(pane.x_axis(), axes_visible, axis_font, true);
      bottom = std::max(bottom, x_depth + x_label_pad + x_label_extent);

      const double y_depth = axis_tick_depth(pane.y_axis(), 6, false);
      const double y_label_extent = pane.y_axis().label.empty() ? 0.0 : axis_font;
      const double y_label_pad =
          pane.y_axis().label.empty() ? 6.0 : resolved_axis_label_gap(pane.y_axis(), axes_visible, axis_font, false);
      left = std::max(left, y_depth + y_label_pad + y_label_extent);
    }

    return std::array<double, 4>{left, top, right, bottom};
  };

  const double left_margin = studio.y_captions().empty()
                                 ? compact_or_default(28.0, 18.0)
                                 : compact_or_default(54.0, 34.0);
  const double right_margin = studio.legend_dock() == KeyDock::East
                                  ? compact_or_default(276.0, 232.0)
                                  : compact_or_default(28.0, 18.0);
  const double top_margin = studio.headline_text().empty()
                                ? compact_or_default(28.0, 18.0)
                                : compact_or_default(58.0, 40.0);
  double bottom_margin = studio.x_captions().empty()
                             ? compact_or_default(28.0, 18.0)
                             : compact_or_default(56.0, 40.0);
  if (studio.legend_dock() == KeyDock::South) {
    bottom_margin += tight ? 78.0 : 104.0;
  }
  const double inner_width = std::max(0.0, studio.extent().width - left_margin - right_margin);
  const double inner_height = std::max(0.0, studio.extent().height - top_margin - bottom_margin);
  const MosaicLayout mosaic = resolved_mosaic_layout(studio);

  layouts.reserve(studio.pane_count());
  if (mosaic.active) {
    const double gap_x = tight ? std::max(6.0, mosaic.gutter_x * 0.62) : mosaic.gutter_x;
    const double gap_y = tight ? std::max(6.0, mosaic.gutter_y * 0.62) : mosaic.gutter_y;
    const double cell_width =
        std::max(0.0, inner_width - gap_x * static_cast<double>(std::max<std::int64_t>(mosaic.cols - 1, 0))) /
        static_cast<double>(mosaic.cols);
    const double cell_height =
        std::max(0.0, inner_height - gap_y * static_cast<double>(std::max<std::int64_t>(mosaic.rows - 1, 0))) /
        static_cast<double>(mosaic.rows);
    std::vector<bool> occupied(static_cast<std::size_t>(mosaic.rows * mosaic.cols), false);
    auto claim = [&](std::int64_t row, std::int64_t col, std::int64_t row_span, std::int64_t col_span) {
      if (row < 0 || col < 0 || row + row_span > mosaic.rows || col + col_span > mosaic.cols) {
        throw std::invalid_argument("pane placement exceeds mosaic bounds");
      }
      for (std::int64_t r = row; r < row + row_span; ++r) {
        for (std::int64_t c = col; c < col + col_span; ++c) {
          const auto index = static_cast<std::size_t>(r * mosaic.cols + c);
          if (occupied[index]) {
            throw std::invalid_argument("pane placement overlaps an occupied mosaic cell");
          }
          occupied[index] = true;
        }
      }
      return Rect{
          left_margin + static_cast<double>(col) * (cell_width + gap_x),
          top_margin + static_cast<double>(row) * (cell_height + gap_y),
          cell_width * static_cast<double>(col_span) + gap_x * static_cast<double>(col_span - 1),
          cell_height * static_cast<double>(row_span) + gap_y * static_cast<double>(row_span - 1),
      };
    };
    auto next_open = [&]() -> std::pair<std::int64_t, std::int64_t> {
      for (std::int64_t row = 0; row < mosaic.rows; ++row) {
        for (std::int64_t col = 0; col < mosaic.cols; ++col) {
          if (!occupied[static_cast<std::size_t>(row * mosaic.cols + col)]) {
            return {row, col};
          }
        }
      }
      throw std::invalid_argument("mosaic has no free cells left for pane placement");
    };

    for (const Pane& pane : studio.panes()) {
      PanePlacement placement = pane.placement();
      if (!placement.explicit_slot) {
        const auto [row, col] = next_open();
        placement = PanePlacement{row, col, 1, 1, true};
      }
      const Rect outer = claim(
          placement.row,
          placement.col,
          placement.row_span,
          placement.col_span);
      const auto insets = pane_insets(pane);
      layouts.push_back(PaneLayout{
          outer,
          inset(outer, insets[0], insets[1], insets[2], insets[3]),
      });
    }
    return layouts;
  }

  const double gap = tight ? 12.0 : 18.0;
  const double stacked_height =
      std::max(0.0,
               inner_height - gap * static_cast<double>(studio.pane_count() - 1));
  const double pane_height = stacked_height / static_cast<double>(studio.pane_count());
  for (std::size_t i = 0; i < studio.pane_count(); ++i) {
    const Pane& pane = studio.panes()[i];
    Rect outer{
        left_margin,
        top_margin + static_cast<double>(i) * (pane_height + gap),
        inner_width,
        pane_height,
    };
    const auto insets = pane_insets(pane);
    layouts.push_back(PaneLayout{
        outer,
        inset(outer, insets[0], insets[1], insets[2], insets[3]),
    });
  }
  return layouts;
}

[[nodiscard]] Bounds2d bounds_for_planar(const Pane& pane) {
  Bounds2d bounds;
  bool only_radial = true;
  bool saw_visible_trace = false;

  for (const Trace& trace : pane.traces()) {
    if (!trace.visible()) {
      continue;
    }
    saw_visible_trace = true;
    std::visit(
        [&](const auto& payload) {
          using Payload = std::decay_t<decltype(payload)>;
          if constexpr (std::is_same_v<Payload, TrendData>) {
            only_radial = false;
            const auto xy = normalize_pairs(payload.x, payload.y);
            for (std::size_t i = 0; i < xy.first.size(); ++i) {
              bounds.include(xy.first[i], xy.second[i]);
            }
          } else if constexpr (std::is_same_v<Payload, CloudData>) {
            only_radial = false;
            const auto cloud = normalize_cloud(payload);
            for (std::size_t i = 0; i < cloud.x.size(); ++i) {
              bounds.include(cloud.x[i], cloud.y[i]);
            }
          } else if constexpr (std::is_same_v<Payload, FieldmapData>) {
            only_radial = false;
            const auto grid = normalize_surface_grid(payload);
            if (!grid.has_value()) {
              return;
            }
            for (std::int64_t row = 0; row < grid->rows; ++row) {
              for (std::int64_t col = 0; col < grid->cols; ++col) {
                for (const Point2d& point : center_cell_polygon(*grid, row, col)) {
                  bounds.include(point.x, point.y);
                }
              }
            }
          } else if constexpr (std::is_same_v<Payload, SpikeData>) {
            only_radial = false;
            const auto xy = normalize_pairs(payload.x, payload.y);
            for (std::size_t i = 0; i < xy.first.size(); ++i) {
              bounds.include(xy.first[i], payload.base);
              bounds.include(xy.first[i], xy.second[i]);
            }
          } else if constexpr (std::is_same_v<Payload, WedgeData>) {
            only_radial = false;
            bounds.include(-1.0, -1.0);
            bounds.include(1.0, 1.0);
          } else if constexpr (std::is_same_v<Payload, RibbonData>) {
            only_radial = false;
            const auto geometry = normalize_ribbon(payload);
            if (!geometry.has_value()) {
              return;
            }
            for (std::size_t i = 0; i < geometry->x.size(); ++i) {
              bounds.include(geometry->x[i], geometry->top[i]);
              bounds.include(geometry->x[i], geometry->bottom[i]);
            }
          } else if constexpr (std::is_same_v<Payload, WhiskData>) {
            only_radial = false;
            const auto geometry = normalize_whisk(payload);
            if (!geometry.has_value()) {
              return;
            }
            for (std::size_t i = 0; i < geometry->x.size(); ++i) {
              bounds.include(geometry->xlow[i], geometry->ylow[i]);
              bounds.include(geometry->xhigh[i], geometry->yhigh[i]);
            }
          } else if constexpr (std::is_same_v<Payload, ColumnsData>) {
            only_radial = false;
            const auto xy = normalize_pairs(payload.x, payload.y);
            const double half_width = 0.5 * payload.width;
            for (std::size_t i = 0; i < xy.first.size(); ++i) {
              bounds.include(xy.first[i] - half_width, payload.base);
              bounds.include(xy.first[i] + half_width, xy.second[i]);
            }
          } else if constexpr (std::is_same_v<Payload, BucketData>) {
            only_radial = false;
            const auto geometry = bucket_geometry(payload);
            const double width =
                (geometry.high - geometry.low) / std::max<std::size_t>(geometry.counts.size(), 1U);
            for (std::size_t i = 0; i < geometry.counts.size(); ++i) {
              const double left = geometry.low + width * static_cast<double>(i);
              bounds.include(left, 0.0);
              bounds.include(left + width, geometry.counts[i]);
            }
          } else if constexpr (std::is_same_v<Payload, QuartetData>) {
            only_radial = false;
            const auto summaries = quartet_summaries(payload);
            for (const auto& summary : summaries) {
              bounds.include(summary.group - payload.width * 0.5, summary.low_whisker);
              bounds.include(summary.group + payload.width * 0.5, summary.high_whisker);
              for (const double outlier : summary.outliers) {
                bounds.include(summary.group, outlier);
              }
            }
          } else if constexpr (std::is_same_v<Payload, ViolinData>) {
            only_radial = false;
            const auto shapes = violin_shapes(payload);
            for (const auto& shape : shapes) {
              if (shape.y.empty() || shape.left_half_width.empty() || shape.right_half_width.empty()) {
                continue;
              }
              for (std::size_t i = 0; i < shape.y.size(); ++i) {
                bounds.include(shape.group - shape.left_half_width[i], shape.y[i]);
                bounds.include(shape.group + shape.right_half_width[i], shape.y[i]);
              }
            }
          } else if constexpr (std::is_same_v<Payload, ContourData>) {
            only_radial = false;
            const auto grid = normalize_surface_grid(payload);
            if (!grid.has_value()) {
              return;
            }
            for (std::size_t i = 0; i < grid->x.size(); ++i) {
              bounds.include(grid->x[i], grid->y[i]);
            }
          } else if constexpr (std::is_same_v<Payload, RadialData>) {
            const auto polar = normalize_pairs(payload.theta, payload.radius);
            for (std::size_t i = 0; i < polar.first.size(); ++i) {
              bounds.include(std::cos(polar.first[i]) * polar.second[i],
                             std::sin(polar.first[i]) * polar.second[i]);
            }
          } else {
            only_radial = false;
          }
        },
        trace.payload());
  }
  if (!saw_visible_trace) {
    only_radial = false;
  }

  const double ref_x =
      bounds.valid() ? 0.5 * (bounds.min_x + bounds.max_x) : (pane.x_axis().scale.kind == ScaleKind::Log10 ? 1.0 : 0.0);
  const double ref_y =
      bounds.valid() ? 0.5 * (bounds.min_y + bounds.max_y) : (pane.y_axis().scale.kind == ScaleKind::Log10 ? 1.0 : 0.0);

  for (const RuleCue& rule : pane.rules()) {
    if (rule.axis == CueAxis::X) {
      bounds.include(rule.value, ref_y);
    } else if (rule.axis == CueAxis::Y) {
      bounds.include(ref_x, rule.value);
    }
  }
  for (const BandCue& band : pane.bands()) {
    if (band.axis == CueAxis::X) {
      bounds.include(band.span.low, ref_y);
      bounds.include(band.span.high, ref_y);
    } else if (band.axis == CueAxis::Y) {
      bounds.include(ref_x, band.span.low);
      bounds.include(ref_x, band.span.high);
    }
  }
  for (const PinCue& pin : pane.pins()) {
    bounds.include(pin.anchor.x, pin.anchor.y);
  }

  bounds.pad_if_flat();
  if (only_radial && bounds.valid()) {
    const double radius = std::max(
        {std::abs(bounds.min_x), std::abs(bounds.max_x), std::abs(bounds.min_y), std::abs(bounds.max_y), 1.0});
    bounds.min_x = -radius;
    bounds.max_x = radius;
    bounds.min_y = -radius;
    bounds.max_y = radius;
  }

  const Range1d xr = effective_range(pane.x_axis(), bounds.min_x, bounds.max_x);
  const Range1d yr = effective_range(pane.y_axis(), bounds.min_y, bounds.max_y);
  return Bounds2d{xr.low, xr.high, yr.low, yr.high};
}

[[nodiscard]] Bounds3d bounds_for_volume(const Pane& pane) {
  Bounds3d bounds;
  for (const Trace& trace : pane.traces()) {
    if (!trace.visible()) {
      continue;
    }
    std::visit(
        [&](const auto& payload) {
          using Payload = std::decay_t<decltype(payload)>;
          if constexpr (std::is_same_v<Payload, Path3Data>) {
            const auto xy = normalize_pairs(payload.x, payload.y);
            const auto z = to_f64(payload.z);
            const auto count = std::min({xy.first.size(), xy.second.size(), z.size()});
            for (std::size_t i = 0; i < count; ++i) {
              bounds.include(xy.first[i], xy.second[i], z[i]);
            }
          } else if constexpr (std::is_same_v<Payload, Cloud3Data>) {
            const auto cloud = normalize_cloud3(payload);
            for (std::size_t i = 0; i < cloud.x.size(); ++i) {
              bounds.include(cloud.x[i], cloud.y[i], cloud.z[i]);
            }
          } else if constexpr (std::is_same_v<Payload, Spire3Data>) {
            const auto grid = normalize_surface_grid(payload);
            if (!grid.has_value()) {
              return;
            }
            const double hx = 0.5 * payload.width;
            const double hy = 0.5 * payload.depth;
            for (std::size_t i = 0; i < grid->z.size(); ++i) {
              bounds.include(grid->x[i] - hx, grid->y[i] - hy, payload.base);
              bounds.include(grid->x[i] + hx, grid->y[i] + hy, grid->z[i]);
            }
          } else if constexpr (std::is_same_v<Payload, IsoShellData>) {
            const auto field = normalize_volume_field(payload);
            if (!field.has_value()) {
              return;
            }
            const auto [x_min_it, x_max_it] = std::minmax_element(field->x.begin(), field->x.end());
            const auto [y_min_it, y_max_it] = std::minmax_element(field->y.begin(), field->y.end());
            const auto [z_min_it, z_max_it] = std::minmax_element(field->z.begin(), field->z.end());
            if (x_min_it == field->x.end() || y_min_it == field->y.end() || z_min_it == field->z.end()) {
              return;
            }
            bounds.include(*x_min_it, *y_min_it, *z_min_it);
            bounds.include(*x_max_it, *y_max_it, *z_max_it);
          } else if constexpr (std::is_same_v<Payload, ContourData>) {
            if (trace.kind() != TraceKind::Ridge3) {
              return;
            }
            const auto grid = normalize_surface_grid(payload);
            if (!grid.has_value()) {
              return;
            }
            const std::vector<double> levels = contour_levels(payload, *grid);
            if (levels.empty()) {
              return;
            }
            for (const double level : levels) {
              for (std::size_t i = 0; i < grid->x.size(); ++i) {
                bounds.include(grid->x[i], grid->y[i], level);
              }
            }
          } else if constexpr (std::is_same_v<Payload, SurfaceData>) {
            const auto grid = normalize_surface_grid(payload);
            if (!grid.has_value()) {
              return;
            }
            for (std::size_t i = 0; i < grid->z.size(); ++i) {
              bounds.include(grid->x[i], grid->y[i], grid->z[i]);
            }
          }
        },
        trace.payload());
  }

  const double ref_x = bounds.valid() ? 0.5 * (bounds.min_x + bounds.max_x) : 0.0;
  const double ref_y = bounds.valid() ? 0.5 * (bounds.min_y + bounds.max_y) : 0.0;
  const double ref_z = bounds.valid() ? 0.5 * (bounds.min_z + bounds.max_z) : 0.0;

  for (const RuleCue& rule : pane.rules()) {
    if (rule.axis == CueAxis::X) {
      bounds.include(rule.value, ref_y, ref_z);
    } else if (rule.axis == CueAxis::Y) {
      bounds.include(ref_x, rule.value, ref_z);
    } else {
      bounds.include(ref_x, ref_y, rule.value);
    }
  }
  for (const BandCue& band : pane.bands()) {
    if (band.axis == CueAxis::X) {
      bounds.include(band.span.low, ref_y, ref_z);
      bounds.include(band.span.high, ref_y, ref_z);
    } else if (band.axis == CueAxis::Y) {
      bounds.include(ref_x, band.span.low, ref_z);
      bounds.include(ref_x, band.span.high, ref_z);
    } else {
      bounds.include(ref_x, ref_y, band.span.low);
      bounds.include(ref_x, ref_y, band.span.high);
    }
  }
  for (const PinCue& pin : pane.pins()) {
    bounds.include(pin.anchor.x, pin.anchor.y, pin.anchor.z);
  }

  bounds.pad_if_flat();
  const Range1d xr = effective_range(pane.x_axis(), bounds.min_x, bounds.max_x);
  const Range1d yr = effective_range(pane.y_axis(), bounds.min_y, bounds.max_y);
  const Range1d zr = effective_range(pane.z_axis(), bounds.min_z, bounds.max_z);
  return Bounds3d{xr.low, xr.high, yr.low, yr.high, zr.low, zr.high};
}

void apply_aligned_limits(
    const Studio& studio,
    std::vector<std::optional<Bounds2d>>& planar_bounds,
    std::vector<std::optional<Bounds3d>>& volume_bounds) {
  struct SharedRange {
    double low{std::numeric_limits<double>::infinity()};
    double high{-std::numeric_limits<double>::infinity()};
    bool valid{false};

    void include(double lo, double hi) noexcept {
      if (!std::isfinite(lo) || !std::isfinite(hi)) {
        return;
      }
      low = std::min(low, lo);
      high = std::max(high, hi);
      valid = true;
    }
  };

  std::map<std::string, SharedRange> x_groups;
  std::map<std::string, SharedRange> y_groups;
  std::map<std::string, SharedRange> z_groups;

  for (std::size_t i = 0; i < studio.pane_count(); ++i) {
    const Pane& pane = studio.panes()[i];
    if (planar_bounds[i].has_value()) {
      const Bounds2d& bounds = *planar_bounds[i];
      if (!pane.x_align_group().empty()) {
        x_groups[pane.x_align_group()].include(bounds.min_x, bounds.max_x);
      }
      if (!pane.y_align_group().empty()) {
        y_groups[pane.y_align_group()].include(bounds.min_y, bounds.max_y);
      }
    }
    if (volume_bounds[i].has_value()) {
      const Bounds3d& bounds = *volume_bounds[i];
      if (!pane.x_align_group().empty()) {
        x_groups[pane.x_align_group()].include(bounds.min_x, bounds.max_x);
      }
      if (!pane.y_align_group().empty()) {
        y_groups[pane.y_align_group()].include(bounds.min_y, bounds.max_y);
      }
      if (!pane.z_align_group().empty()) {
        z_groups[pane.z_align_group()].include(bounds.min_z, bounds.max_z);
      }
    }
  }

  for (std::size_t i = 0; i < studio.pane_count(); ++i) {
    const Pane& pane = studio.panes()[i];
    if (planar_bounds[i].has_value()) {
      Bounds2d& bounds = *planar_bounds[i];
      if (!pane.x_align_group().empty()) {
        const auto it = x_groups.find(pane.x_align_group());
        if (it != x_groups.end() && it->second.valid) {
          bounds.min_x = it->second.low;
          bounds.max_x = it->second.high;
        }
      }
      if (!pane.y_align_group().empty()) {
        const auto it = y_groups.find(pane.y_align_group());
        if (it != y_groups.end() && it->second.valid) {
          bounds.min_y = it->second.low;
          bounds.max_y = it->second.high;
        }
      }
    }
    if (volume_bounds[i].has_value()) {
      Bounds3d& bounds = *volume_bounds[i];
      if (!pane.x_align_group().empty()) {
        const auto it = x_groups.find(pane.x_align_group());
        if (it != x_groups.end() && it->second.valid) {
          bounds.min_x = it->second.low;
          bounds.max_x = it->second.high;
        }
      }
      if (!pane.y_align_group().empty()) {
        const auto it = y_groups.find(pane.y_align_group());
        if (it != y_groups.end() && it->second.valid) {
          bounds.min_y = it->second.low;
          bounds.max_y = it->second.high;
        }
      }
      if (!pane.z_align_group().empty()) {
        const auto it = z_groups.find(pane.z_align_group());
        if (it != z_groups.end() && it->second.valid) {
          bounds.min_z = it->second.low;
          bounds.max_z = it->second.high;
        }
      }
    }
  }
}

[[nodiscard]] bool pane_is_radial_only(const Pane& pane) {
  if (pane.traces().empty()) {
    return false;
  }
  bool any_visible = false;
  for (const Trace& trace : pane.traces()) {
    if (!trace.visible()) {
      continue;
    }
    any_visible = true;
    if (trace.kind() != TraceKind::Radial) {
      return false;
    }
  }
  return any_visible;
}

[[nodiscard]] bool pane_is_wedge_only(const Pane& pane) {
  if (pane.traces().empty()) {
    return false;
  }
  bool any_visible = false;
  for (const Trace& trace : pane.traces()) {
    if (!trace.visible()) {
      continue;
    }
    any_visible = true;
    if (trace.kind() != TraceKind::Wedge) {
      return false;
    }
  }
  return any_visible;
}

[[nodiscard]] Point2d map_planar_point(
    double x,
    double y,
    const Pane& pane,
    const Bounds2d& bounds,
    const Rect& rect,
    bool equal_aspect = false) {
  const auto tx = scale_value(x, pane.x_axis().scale);
  const auto ty = scale_value(y, pane.y_axis().scale);
  const auto min_tx = scale_value(bounds.min_x, pane.x_axis().scale);
  const auto max_tx = scale_value(bounds.max_x, pane.x_axis().scale);
  const auto min_ty = scale_value(bounds.min_y, pane.y_axis().scale);
  const auto max_ty = scale_value(bounds.max_y, pane.y_axis().scale);
  if (!tx || !ty || !min_tx || !max_tx || !min_ty || !max_ty) {
    return {std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN()};
  }

  const double span_x = std::max(*max_tx - *min_tx, 1e-12);
  const double span_y = std::max(*max_ty - *min_ty, 1e-12);
  double sx = (*tx - *min_tx) / span_x;
  double sy = (*ty - *min_ty) / span_y;

  if (!equal_aspect) {
    return {rect.x + sx * rect.width, rect.y + (1.0 - sy) * rect.height};
  }

  const double size = std::min(rect.width, rect.height);
  const double x_offset = rect.x + (rect.width - size) * 0.5;
  const double y_offset = rect.y + (rect.height - size) * 0.5;
  return {x_offset + sx * size, y_offset + (1.0 - sy) * size};
}

struct CameraBasis {
  Point3d eye_direction{};
  Point3d forward{};
  Point3d right{};
  Point3d up{};
  double distance{3.2};
  double focal{1.0};
};

[[nodiscard]] Point3d bounds_center(const Bounds3d& bounds) noexcept {
  return {
      0.5 * (bounds.min_x + bounds.max_x),
      0.5 * (bounds.min_y + bounds.max_y),
      0.5 * (bounds.min_z + bounds.max_z),
  };
}

[[nodiscard]] Point3d normalize_box_point(Point3d point, const Bounds3d& bounds) noexcept {
  const Point3d center = bounds_center(bounds);
  const double span_x = std::max(bounds.max_x - bounds.min_x, 1e-9);
  const double span_y = std::max(bounds.max_y - bounds.min_y, 1e-9);
  const double span_z = std::max(bounds.max_z - bounds.min_z, 1e-9);
  return {
      (point.x - center.x) / span_x,
      (point.y - center.y) / span_y,
      (point.z - center.z) / span_z,
  };
}

[[nodiscard]] CameraBasis camera_basis(const Lens& lens) noexcept {
  const double az = lens.azimuth_deg() * kPi / 180.0;
  const double el = lens.elevation_deg() * kPi / 180.0;
  const double cos_el = std::cos(el);
  Point3d eye_direction = normalize({cos_el * std::cos(az), cos_el * std::sin(az), std::sin(el)});
  Point3d forward = scale(eye_direction, -1.0);
  Point3d up_reference = normalize({lens.up_vector().x, lens.up_vector().y, lens.up_vector().z});
  Point3d right = cross(forward, up_reference);
  if (norm(right) < 1e-8) {
    up_reference = std::abs(forward.z) < 0.95 ? Point3d{0.0, 0.0, 1.0} : Point3d{0.0, 1.0, 0.0};
    right = cross(forward, up_reference);
  }
  right = normalize(right);
  Point3d up = normalize(cross(right, forward));
  const double focal = 1.0 / std::tan(std::clamp(lens.view_angle_deg(), 4.0, 120.0) * kPi / 360.0);
  return CameraBasis{eye_direction, forward, right, up, 3.2, focal};
}

[[nodiscard]] Point3d rotate_world(Point3d point, const Lens& lens, Point3d /*center*/) {
  const CameraBasis basis = camera_basis(lens);
  return {
      dot(point, basis.right),
      dot(point, basis.up),
      dot(point, basis.forward),
  };
}

[[nodiscard]] Point3d camera_space_point(Point3d point, const Pane& pane, const Bounds3d& bounds) {
  const CameraBasis basis = camera_basis(pane.lens());
  const Point3d eye = scale(basis.eye_direction, basis.distance);
  const Point3d normalized = normalize_box_point(point, bounds);
  const Point3d relative = subtract(normalized, eye);
  return {
      dot(relative, basis.right),
      dot(relative, basis.up),
      dot(relative, basis.forward),
  };
}

[[nodiscard]] Point3d project_world(Point3d point, const Pane& pane, const Bounds3d& bounds) {
  const CameraBasis basis = camera_basis(pane.lens());
  Point3d rotated = camera_space_point(point, pane, bounds);
  if (pane.lens().projection() == ProjectionKind::Perspective) {
    const double depth = std::max(rotated.z, 0.25);
    const double scale = basis.focal / depth;
    rotated.x *= scale;
    rotated.y *= scale;
  }
  rotated.z = -rotated.z;
  return rotated;
}

[[nodiscard]] Bounds2d projected_bounds(const Pane& pane, const Bounds3d& bounds) {
  Bounds2d projected;
  const std::array<Point3d, 8> corners{{
      {bounds.min_x, bounds.min_y, bounds.min_z},
      {bounds.min_x, bounds.min_y, bounds.max_z},
      {bounds.min_x, bounds.max_y, bounds.min_z},
      {bounds.min_x, bounds.max_y, bounds.max_z},
      {bounds.max_x, bounds.min_y, bounds.min_z},
      {bounds.max_x, bounds.min_y, bounds.max_z},
      {bounds.max_x, bounds.max_y, bounds.min_z},
      {bounds.max_x, bounds.max_y, bounds.max_z},
  }};
  for (const Point3d& corner : corners) {
    const Point3d projected_corner = project_world(corner, pane, bounds);
    projected.include(projected_corner.x, projected_corner.y);
  }
  projected.pad_if_flat();
  return projected;
}

[[nodiscard]] Point2d map_projected_point(Point3d point, const Pane& pane, const Bounds3d& bounds, const Rect& rect) {
  const Point3d projected = project_world(point, pane, bounds);
  const Bounds2d projected_box = projected_bounds(pane, bounds);
  const double sx = (projected.x - projected_box.min_x) / std::max(projected_box.max_x - projected_box.min_x, 1e-12);
  const double sy = (projected.y - projected_box.min_y) / std::max(projected_box.max_y - projected_box.min_y, 1e-12);
  return {rect.x + sx * rect.width, rect.y + (1.0 - sy) * rect.height};
}

[[nodiscard]] Point3d bilinear_point3d(
    const std::array<Point3d, 4>& cell,
    double u,
    double v) noexcept {
  const Point3d top = add(scale(cell[0], 1.0 - u), scale(cell[1], u));
  const Point3d bottom = add(scale(cell[3], 1.0 - u), scale(cell[2], u));
  return add(scale(top, 1.0 - v), scale(bottom, v));
}

[[nodiscard]] double bilinear_scalar(
    double v00,
    double v01,
    double v11,
    double v10,
    double u,
    double v) noexcept {
  const double top = v00 + (v01 - v00) * u;
  const double bottom = v10 + (v11 - v10) * u;
  return top + (bottom - top) * v;
}

[[nodiscard]] MaterialCoefficients material_coefficients(MaterialProfile profile) noexcept {
  switch (profile) {
    case MaterialProfile::Dull:
      return {0.3, 0.8, 0.0, 10.0, 1.0};
    case MaterialProfile::Shiny:
      return {0.3, 0.6, 0.9, 20.0, 1.0};
    case MaterialProfile::Metal:
      return {0.3, 0.3, 1.0, 25.0, 0.5};
    case MaterialProfile::Default:
    default:
      return {0.3, 0.6, 0.9, 20.0, 1.0};
  }
}

[[nodiscard]] Rgba apply_surface_lighting(
    Rgba base,
    const Trace& trace,
    const Pane& pane,
    const std::array<Point3d, 4>& cell) noexcept {
  if (trace.lighting_mode() == LightingMode::None || pane.lights().empty()) {
    return base;
  }

  const Point3d edge_a = subtract(cell[1], cell[0]);
  const Point3d edge_b = subtract(cell[3], cell[0]);
  Point3d normal = normalize(cross(edge_a, edge_b));
  Point3d normal_cam = normalize(rotate_world(normal, pane.lens(), {0.0, 0.0, 0.0}));
  if (normal_cam.z < 0.0) {
    normal_cam = scale(normal_cam, -1.0);
  }

  const MaterialCoefficients material = material_coefficients(trace.material_profile());
  const Point3d view_dir{0.0, 0.0, 1.0};

  double diffuse_accum = 0.0;
  double specular_accum = 0.0;
  for (const LightSpec& light : pane.lights().lights()) {
    Point3d light_dir = direction_from_angles(light.azimuth_deg, light.elevation_deg);
    if (light.space == LightSpace::Data) {
      light_dir = normalize(rotate_world(light_dir, pane.lens(), {0.0, 0.0, 0.0}));
    }
    const double n_dot_l = std::max(0.0, dot(normal_cam, light_dir));
    diffuse_accum += n_dot_l;

    const Point3d reflected = normalize(reflect(light_dir, normal_cam));
    specular_accum += std::pow(std::max(0.0, dot(reflected, view_dir)), material.exponent);
  }

  const double light_scale = 1.0 / static_cast<double>(pane.lights().lights().size());
  const double diffuse = material.diffuse * diffuse_accum * light_scale;
  const double specular = material.specular * specular_accum * light_scale;
  const double intensity = std::clamp(material.ambient + diffuse, 0.15, 1.5);
  Rgba shaded = multiply_rgb(base, intensity);
  const Rgba highlight{
      specular * material.specular_color,
      specular * material.specular_color,
      specular * material.specular_color,
      base.alpha,
  };
  shaded = add_rgb(shaded, highlight);

  if (trace.shade_mode() == ShadeMode::Smooth) {
    shaded = mix(shaded, Rgba::from_u8(255, 255, 255, static_cast<unsigned>(std::round(base.alpha * 255.0))), 0.08);
  }

  return shaded;
}

[[nodiscard]] FillStroke surface_face_style(Rgba fill, const Trace& trace) noexcept {
  const double alpha = trace.transparent() ? 0.56 : 1.0;
  fill.alpha = alpha;
  switch (trace.shade_mode()) {
    case ShadeMode::Faceted:
      return FillStroke{multiply_rgb(fill, 0.45), fill, true, true, 0.45};
    case ShadeMode::Flat:
      return FillStroke{fill, fill, false, true, 0.0};
    case ShadeMode::Smooth:
      return FillStroke{fill, fill, false, true, 0.0};
    default:
      return FillStroke{fill, fill, false, true, 0.0};
  }
}

struct ContourSegment {
  Point2d start{};
  Point2d end{};
  double level{0.0};
};

struct ScalarVertex2d {
  Point2d point{};
  double value{0.0};
};

[[nodiscard]] bool edge_crosses(double a, double b, double level) noexcept {
  if (!std::isfinite(a) || !std::isfinite(b)) {
    return false;
  }
  if (std::abs(a - b) <= 1e-12) {
    return false;
  }
  return (level >= std::min(a, b) && level <= std::max(a, b));
}

[[nodiscard]] Point2d interpolate_level(Point2d a, Point2d b, double za, double zb, double level) noexcept {
  if (std::abs(zb - za) <= 1e-12) {
    return a;
  }
  const double t = std::clamp((level - za) / (zb - za), 0.0, 1.0);
  return {
      a.x + (b.x - a.x) * t,
      a.y + (b.y - a.y) * t,
  };
}

[[nodiscard]] std::vector<ContourSegment> contour_segments_for_level(
    const SurfaceGrid& grid,
    double level) {
  std::vector<ContourSegment> segments;
  for (std::int64_t row = 0; row + 1 < grid.rows; ++row) {
    for (std::int64_t col = 0; col + 1 < grid.cols; ++col) {
      const auto i00 = static_cast<std::size_t>(row * grid.cols + col);
      const auto i10 = static_cast<std::size_t>(row * grid.cols + (col + 1));
      const auto i01 = static_cast<std::size_t>((row + 1) * grid.cols + col);
      const auto i11 = static_cast<std::size_t>((row + 1) * grid.cols + (col + 1));

      const std::array<Point2d, 4> points{{
          {grid.x[i00], grid.y[i00]},
          {grid.x[i10], grid.y[i10]},
          {grid.x[i11], grid.y[i11]},
          {grid.x[i01], grid.y[i01]},
      }};
      const std::array<double, 4> values{{grid.z[i00], grid.z[i10], grid.z[i11], grid.z[i01]}};
      const std::array<std::pair<int, int>, 4> edges{{{0, 1}, {1, 2}, {2, 3}, {3, 0}}};

      std::array<std::pair<int, Point2d>, 4> intersections{};
      int count = 0;
      for (int edge = 0; edge < 4; ++edge) {
        const auto [a, b] = edges[static_cast<std::size_t>(edge)];
        if (!edge_crosses(values[static_cast<std::size_t>(a)], values[static_cast<std::size_t>(b)], level)) {
          continue;
        }
        intersections[static_cast<std::size_t>(count++)] = {
            edge,
            interpolate_level(points[static_cast<std::size_t>(a)],
                              points[static_cast<std::size_t>(b)],
                              values[static_cast<std::size_t>(a)],
                              values[static_cast<std::size_t>(b)],
                              level)};
      }

      if (count == 2) {
        segments.push_back({intersections[0].second, intersections[1].second, level});
      } else if (count == 4) {
        const double center = 0.25 * (values[0] + values[1] + values[2] + values[3]);
        const bool high_center = center >= level;
        if (high_center) {
          segments.push_back({intersections[0].second, intersections[1].second, level});
          segments.push_back({intersections[2].second, intersections[3].second, level});
        } else {
          segments.push_back({intersections[0].second, intersections[3].second, level});
          segments.push_back({intersections[1].second, intersections[2].second, level});
        }
      }
    }
  }
  return segments;
}

[[nodiscard]] std::vector<ScalarVertex2d> clip_polygon_above(
    const std::vector<ScalarVertex2d>& polygon,
    double threshold) {
  std::vector<ScalarVertex2d> out;
  if (polygon.empty()) {
    return out;
  }

  auto interpolate = [](const ScalarVertex2d& a, const ScalarVertex2d& b, double value) -> ScalarVertex2d {
    if (std::abs(b.value - a.value) <= 1e-12) {
      return a;
    }
    const double t = std::clamp((value - a.value) / (b.value - a.value), 0.0, 1.0);
    return {
        {
            a.point.x + (b.point.x - a.point.x) * t,
            a.point.y + (b.point.y - a.point.y) * t,
        },
        value,
    };
  };

  for (std::size_t i = 0; i < polygon.size(); ++i) {
    const ScalarVertex2d& current = polygon[i];
    const ScalarVertex2d& previous = polygon[(i + polygon.size() - 1) % polygon.size()];
    const bool current_inside = current.value >= threshold;
    const bool previous_inside = previous.value >= threshold;

    if (current_inside != previous_inside) {
      out.push_back(interpolate(previous, current, threshold));
    }
    if (current_inside) {
      out.push_back(current);
    }
  }
  return out;
}

[[nodiscard]] std::vector<ScalarVertex2d> clip_polygon_below(
    const std::vector<ScalarVertex2d>& polygon,
    double threshold) {
  std::vector<ScalarVertex2d> out;
  if (polygon.empty()) {
    return out;
  }

  auto interpolate = [](const ScalarVertex2d& a, const ScalarVertex2d& b, double value) -> ScalarVertex2d {
    if (std::abs(b.value - a.value) <= 1e-12) {
      return a;
    }
    const double t = std::clamp((value - a.value) / (b.value - a.value), 0.0, 1.0);
    return {
        {
            a.point.x + (b.point.x - a.point.x) * t,
            a.point.y + (b.point.y - a.point.y) * t,
        },
        value,
    };
  };

  for (std::size_t i = 0; i < polygon.size(); ++i) {
    const ScalarVertex2d& current = polygon[i];
    const ScalarVertex2d& previous = polygon[(i + polygon.size() - 1) % polygon.size()];
    const bool current_inside = current.value <= threshold;
    const bool previous_inside = previous.value <= threshold;

    if (current_inside != previous_inside) {
      out.push_back(interpolate(previous, current, threshold));
    }
    if (current_inside) {
      out.push_back(current);
    }
  }
  return out;
}

[[nodiscard]] std::vector<Point2d> contour_cell_band_polygon(
    const SurfaceGrid& grid,
    std::int64_t row,
    std::int64_t col,
    double low,
    double high) {
  const auto i00 = static_cast<std::size_t>(row * grid.cols + col);
  const auto i10 = static_cast<std::size_t>(row * grid.cols + (col + 1));
  const auto i11 = static_cast<std::size_t>((row + 1) * grid.cols + (col + 1));
  const auto i01 = static_cast<std::size_t>((row + 1) * grid.cols + col);

  std::vector<ScalarVertex2d> polygon{
      {{grid.x[i00], grid.y[i00]}, grid.z[i00]},
      {{grid.x[i10], grid.y[i10]}, grid.z[i10]},
      {{grid.x[i11], grid.y[i11]}, grid.z[i11]},
      {{grid.x[i01], grid.y[i01]}, grid.z[i01]},
  };

  polygon = clip_polygon_above(polygon, low);
  polygon = clip_polygon_below(polygon, high);

  std::vector<Point2d> points;
  points.reserve(polygon.size());
  for (const ScalarVertex2d& vertex : polygon) {
    points.push_back(vertex.point);
  }
  return points;
}

[[nodiscard]] std::vector<std::pair<double, double>> contour_intervals(
    const SurfaceGrid& grid,
    const std::vector<double>& levels) {
  std::vector<std::pair<double, double>> intervals;
  if (levels.size() < 2) {
    return intervals;
  }

  double min_value = std::numeric_limits<double>::infinity();
  double max_value = -std::numeric_limits<double>::infinity();
  for (const double value : grid.z) {
    if (!std::isfinite(value)) {
      continue;
    }
    min_value = std::min(min_value, value);
    max_value = std::max(max_value, value);
  }
  if (!std::isfinite(min_value) || !std::isfinite(max_value)) {
    return intervals;
  }

  if (levels.front() > min_value) {
    intervals.emplace_back(min_value, levels.front());
  }
  for (std::size_t i = 0; i + 1 < levels.size(); ++i) {
    intervals.emplace_back(levels[i], levels[i + 1]);
  }
  if (levels.back() < max_value) {
    intervals.emplace_back(levels.back(), max_value);
  }
  return intervals;
}

[[nodiscard]] Range1d surface_color_range(const Pane& pane, const Trace& trace, const SurfaceGrid& grid) {
  if (trace.color_limits().has_value()) {
    return *trace.color_limits();
  }
  if (pane.color_axis().limit_mode == LimitMode::Manual && pane.color_axis().limits.has_value()) {
    return *pane.color_axis().limits;
  }
  const auto& values = grid.color.has_value() ? *grid.color : grid.z;
  const auto [min_it, max_it] = std::minmax_element(values.begin(), values.end());
  if (min_it == values.end() || max_it == values.end()) {
    return {0.0, 1.0};
  }
  if (*min_it == *max_it) {
    return {*min_it - 0.5, *max_it + 0.5};
  }
  return {*min_it, *max_it};
}

[[nodiscard]] Range1d contour_color_range(const Pane& pane, const Trace& trace, const std::vector<double>& levels) {
  if (trace.color_limits().has_value()) {
    return *trace.color_limits();
  }
  if (pane.color_axis().limit_mode == LimitMode::Manual && pane.color_axis().limits.has_value()) {
    return *pane.color_axis().limits;
  }
  if (levels.empty()) {
    return {0.0, 1.0};
  }
  const auto [min_it, max_it] = std::minmax_element(levels.begin(), levels.end());
  if (*min_it == *max_it) {
    return {*min_it - 0.5, *max_it + 0.5};
  }
  return {*min_it, *max_it};
}

struct ScaleGuideSpec {
  PaletteRamp palette{};
  Range1d range{};
};

[[nodiscard]] std::optional<ScaleGuideSpec> scale_guide_spec(const Pane& pane) {
  if (!pane.scale_guide_enabled()) {
    return std::nullopt;
  }

  for (const Trace& trace : pane.traces()) {
    if (!trace.visible()) {
      continue;
    }

    std::optional<Range1d> range;
    std::visit(
        [&](const auto& payload) {
          using Payload = std::decay_t<decltype(payload)>;
          if constexpr (std::is_same_v<Payload, CloudData>) {
            if (!payload.color.has_value()) {
              return;
            }
            const Series color = flatten_f64(*payload.color);
            range = trace.color_limits().has_value()
                        ? trace.color_limits()
                        : std::optional<Range1d>{Range1d{scalar_f64(color.min()), scalar_f64(color.max())}};
          } else if constexpr (std::is_same_v<Payload, FieldmapData>) {
            const auto grid = normalize_surface_grid(payload);
            if (!grid.has_value()) {
              return;
            }
            range = surface_color_range(pane, trace, *grid);
          } else if constexpr (std::is_same_v<Payload, ContourData>) {
            const auto grid = normalize_surface_grid(payload);
            if (!grid.has_value()) {
              return;
            }
            const auto levels = contour_levels(payload, *grid);
            range = contour_color_range(pane, trace, levels);
          } else if constexpr (std::is_same_v<Payload, Spire3Data>) {
            const auto grid = normalize_surface_grid(payload);
            if (!grid.has_value()) {
              return;
            }
            range = surface_color_range(pane, trace, *grid);
          } else if constexpr (std::is_same_v<Payload, IsoShellData>) {
            const auto field = normalize_volume_field(payload);
            if (!field.has_value()) {
              return;
            }
            const auto levels = isoshell_levels(payload, *field);
            if (levels.empty()) {
              return;
            }
            range = Range1d{levels.front(), levels.back()};
          } else if constexpr (std::is_same_v<Payload, SurfaceData>) {
            const auto grid = normalize_surface_grid(payload);
            if (!grid.has_value()) {
              return;
            }
            range = surface_color_range(pane, trace, *grid);
          }
        },
        trace.payload());

    if (range.has_value()) {
      return ScaleGuideSpec{trace.palette_ramp(), *range};
    }
  }

  return std::nullopt;
}

[[nodiscard]] Rgba palette_lookup(const PaletteRamp& palette, Range1d range, double value) {
  const double span = std::max(range.high - range.low, 1e-12);
  return palette.sample((value - range.low) / span);
}

void add_path(
    DrawList& list,
    std::vector<Point2d> points,
    FillStroke style,
    bool closed = false) {
  if (points.size() < (closed ? 3U : 2U)) {
    return;
  }
  list.commands.emplace_back(PathCommand{std::move(points), style, closed});
}

void add_text(
    DrawList& list,
    Point2d anchor,
    std::string text,
    double size,
    Rgba color,
    TextCommand::Align align = TextCommand::Align::Start,
    double rotation_deg = 0.0) {
  const bool math_markup = looks_like_math_markup(text);
  text = normalize_display_text(std::move(text));
  if (math_markup) {
    text.insert(text.begin(), kMathTextMarker);
  }
  if (text.empty()) {
    return;
  }
  list.commands.emplace_back(TextCommand{anchor, std::move(text), size, color, rotation_deg, align});
}

void add_caption_block(DrawList& list, const Pane& pane, const PaneLayout& layout) {
  if (pane.caption_text().empty()) {
    return;
  }

  std::vector<std::string> lines;
  lines.reserve(32);
  std::string current;
  current.reserve(256);
  for (char ch : pane.caption_text()) {
    if (ch == '\r') {
      continue;
    }
    if (ch == '\n') {
      lines.push_back(std::move(current));
      current.clear();
      continue;
    }
    current.push_back(ch);
  }
  lines.push_back(std::move(current));

  const Rgba color = multiply_rgb(pane.theme_tokens().foreground(), 0.85);
  const double font_size = 10.0;

  if (lines.size() <= 1U) {
    add_text(
        list,
        {layout.outer.x + 10.0, layout.outer.y + layout.outer.height - 14.0},
        lines.empty() ? std::string{} : lines.front(),
        font_size,
        color);
    return;
  }

  const double line_gap = 2.0;
  const double line_step = font_size + line_gap;
  const double start_x = layout.plot.x + 8.0;
  const double start_y = layout.plot.y + 16.0;
  const double available_height = std::max(0.0, layout.plot.height - 8.0);
  const std::size_t max_lines = static_cast<std::size_t>(
      std::max(1.0, std::floor(available_height / std::max(1.0, line_step))));
  const std::size_t limit = std::min(lines.size(), max_lines);

  for (const auto i : std::views::iota(std::size_t{0}, limit)) {
    std::string line = lines[i];
    if (i + 1 == limit && lines.size() > limit && line.size() > 3U) {
      line.replace(line.size() - 3U, 3U, "...");
    }
    add_text(list, {start_x, start_y + static_cast<double>(i) * line_step}, std::move(line), font_size, color);
  }
}

[[nodiscard]] std::vector<KeyEntry> key_entries_for_pane(const Pane& pane, bool include_pane_title) {
  std::vector<KeyEntry> entries;
  for (const Trace& trace : pane.traces()) {
    if (!trace.visible() || trace.label_text().empty()) {
      continue;
    }
    KeyEntry entry;
    entry.label = include_pane_title && !pane.title_text().empty()
                      ? pane.title_text() + ": " + trace.label_text()
                      : trace.label_text();
    entry.stroke = trace.stroke_color();
    entry.stroke_width = trace.stroke_width();
    if (trace.kind() == TraceKind::Isoband || trace.kind() == TraceKind::Fieldmap ||
        trace.kind() == TraceKind::Sheet || trace.kind() == TraceKind::Spire3 || trace.kind() == TraceKind::IsoShell ||
        trace.kind() == TraceKind::Wedge ||
        trace.kind() == TraceKind::Ribbon || trace.kind() == TraceKind::Columns || trace.kind() == TraceKind::Bucket ||
        trace.kind() == TraceKind::Quartet || trace.kind() == TraceKind::Violin) {
      entry.filled = true;
      entry.fill = trace.palette_ramp().sample(0.65);
      entry.stroke = multiply_rgb(entry.fill, 0.55);
    }
    entries.push_back(std::move(entry));
  }
  return entries;
}

[[nodiscard]] double estimated_text_extent(std::string_view text, double font_size) {
  return std::max(1.0, static_cast<double>(text.size())) * font_size * 0.56;
}

[[nodiscard]] std::vector<std::string> wrap_key_label(
    const std::string& text,
    double max_text_width,
    double font_size) {
  if (text.empty()) {
    return {"-"};
  }
  const std::size_t max_chars =
      static_cast<std::size_t>(std::clamp(std::floor(max_text_width / (font_size * 0.56)), 4.0, 120.0));
  if (text.size() <= max_chars) {
    return {text};
  }

  std::vector<std::string> words;
  std::string token;
  for (char ch : text) {
    if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
      if (!token.empty()) {
        words.push_back(token);
        token.clear();
      }
      continue;
    }
    token.push_back(ch);
  }
  if (!token.empty()) {
    words.push_back(std::move(token));
  }
  if (words.empty()) {
    return {text};
  }

  std::vector<std::string> lines;
  std::string current;
  auto flush_current = [&]() {
    if (!current.empty()) {
      lines.push_back(current);
      current.clear();
    }
  };
  auto append_chunk = [&](std::string_view chunk) {
    if (current.empty()) {
      current.assign(chunk);
      return;
    }
    if (current.size() + 1 + chunk.size() <= max_chars) {
      current.push_back(' ');
      current.append(chunk);
      return;
    }
    flush_current();
    current.assign(chunk);
  };

  for (const std::string& word : words) {
    if (word.size() <= max_chars) {
      append_chunk(word);
      continue;
    }
    std::size_t start = 0;
    while (start < word.size()) {
      const std::size_t take = std::min(max_chars, word.size() - start);
      append_chunk(std::string_view(word).substr(start, take));
      flush_current();
      start += take;
    }
  }
  flush_current();

  constexpr std::size_t kMaxLines = 4;
  if (lines.size() > kMaxLines) {
    lines.resize(kMaxLines);
    std::string& tail = lines.back();
    if (tail.size() > 1) {
      tail = tail.substr(0, tail.size() - 1) + "...";
    }
  }
  return lines;
}

[[nodiscard]] KeyBoxMetrics compute_key_box_metrics(
    const std::vector<KeyEntry>& entries,
    std::string_view title,
    double box_width) {
  KeyBoxMetrics metrics;
  if (entries.empty()) {
    return metrics;
  }

  constexpr double kTopPad = 10.0;
  constexpr double kBottomPad = 10.0;
  constexpr double kLeftPad = 10.0;
  constexpr double kIconSpan = 24.0;
  constexpr double kLabelGap = 8.0;
  constexpr double kRightPad = 10.0;
  constexpr double kFont = 11.0;
  constexpr double kTitleHeight = 18.0;
  constexpr double kLineHeight = 12.0;
  constexpr double kRowPad = 4.0;
  constexpr double kRowMinHeight = 16.0;

  const double text_x = kLeftPad + kIconSpan + kLabelGap;
  const double label_width = std::max(24.0, box_width - text_x - kRightPad);

  metrics.wrapped_entries.reserve(entries.size());
  double total_height = kTopPad + (title.empty() ? 0.0 : kTitleHeight + 2.0);
  for (std::size_t i = 0; i < entries.size(); ++i) {
    auto lines = wrap_key_label(entries[i].label, label_width, kFont);
    const double text_block = static_cast<double>(std::max<std::size_t>(1, lines.size())) * kLineHeight;
    const double block_height = std::max(kRowMinHeight, text_block + kRowPad);
    total_height += block_height;
    metrics.wrapped_entries.push_back(WrappedKeyEntry{i, std::move(lines), block_height});
  }
  total_height += kBottomPad;
  metrics.box_height = total_height;
  return metrics;
}

[[nodiscard]] double estimate_key_width(
    const std::vector<KeyEntry>& entries,
    std::string_view title,
    double min_width,
    double max_width) {
  constexpr double kFont = 11.0;
  constexpr double kTextX = 42.0;
  constexpr double kRightPad = 10.0;
  double max_text_extent = estimated_text_extent(title, kFont);
  for (const auto& entry : entries) {
    max_text_extent = std::max(max_text_extent, estimated_text_extent(entry.label, kFont));
  }
  const double requested = kTextX + kRightPad + max_text_extent;
  return std::clamp(requested, min_width, max_width);
}

void render_key_box(
    DrawList& list,
    const Theme& theme,
    Rect box,
    std::string_view title,
    const std::vector<KeyEntry>& entries,
    KeyFrame frame) {
  if (entries.empty()) {
    return;
  }
  constexpr double kLeftPad = 10.0;
  constexpr double kIconSpan = 24.0;
  constexpr double kLabelGap = 8.0;
  constexpr double kTopPad = 10.0;
  constexpr double kTitleHeight = 18.0;
  constexpr double kLineHeight = 12.0;
  constexpr double kFont = 11.0;
  const KeyBoxMetrics metrics = compute_key_box_metrics(entries, title, box.width);
  if (metrics.wrapped_entries.empty()) {
    return;
  }

  bool draw_panel = true;
  Rgba panel_fill = theme.background();
  Rgba panel_stroke = theme.foreground();
  double panel_stroke_width = 0.7;
  switch (frame) {
    case KeyFrame::Framed:
      panel_fill = theme.background();
      panel_stroke = theme.foreground();
      panel_stroke.alpha = std::clamp(panel_stroke.alpha * 0.72, 0.0, 1.0);
      panel_stroke_width = 0.7;
      break;
    case KeyFrame::Soft:
      panel_fill = mix(theme.background(), Rgba::from_u8(255, 255, 255), 0.08);
      panel_fill.alpha = std::clamp(panel_fill.alpha * 0.94, 0.0, 1.0);
      panel_stroke = mix(theme.foreground(), theme.background(), 0.78);
      panel_stroke.alpha = std::clamp(panel_stroke.alpha * 0.28, 0.0, 1.0);
      panel_stroke_width = 0.6;
      break;
    case KeyFrame::None:
      draw_panel = false;
      break;
  }

  if (draw_panel) {
    add_path(
        list,
        {
            {box.x, box.y},
            {box.x + box.width, box.y},
            {box.x + box.width, box.y + metrics.box_height},
            {box.x, box.y + metrics.box_height},
        },
        FillStroke{panel_stroke, panel_fill, true, true, panel_stroke_width},
        true);
  }

  double cursor_y = box.y + kTopPad;
  if (!title.empty()) {
    add_text(list, {box.x + kLeftPad, cursor_y + 13.0}, std::string(title), kFont, theme.foreground());
    cursor_y += kTitleHeight + 2.0;
  }

  for (const WrappedKeyEntry& wrapped : metrics.wrapped_entries) {
    const KeyEntry& entry = entries[wrapped.entry_index];
    const double cy = cursor_y + wrapped.block_height * 0.5;
    if (entry.filled) {
      add_path(
          list,
          {
              {box.x + kLeftPad, cy - 5.0},
              {box.x + kLeftPad + kIconSpan, cy - 5.0},
              {box.x + kLeftPad + kIconSpan, cy + 5.0},
              {box.x + kLeftPad, cy + 5.0},
          },
          FillStroke{entry.stroke, entry.fill, true, true, 0.6},
          true);
    } else {
      add_path(
          list,
          {{box.x + kLeftPad, cy}, {box.x + kLeftPad + kIconSpan, cy}},
          FillStroke{entry.stroke, {}, true, false, entry.stroke_width},
          false);
    }

    double text_y = cursor_y + 10.0;
    for (const std::string& line : wrapped.lines) {
      add_text(
          list,
          {box.x + kLeftPad + kIconSpan + kLabelGap, text_y},
          line,
          kFont,
          theme.foreground());
      text_y += kLineHeight;
    }
    cursor_y += wrapped.block_height;
  }
}

[[nodiscard]] bool point_in_rect(const Rect& rect, Point2d point) noexcept {
  return point.x >= rect.x && point.x <= rect.x + rect.width && point.y >= rect.y && point.y <= rect.y + rect.height;
}

[[nodiscard]] double legend_overlap_score(
    const DrawList& list,
    const Rect& candidate,
    const Rect& pane_plot,
    std::size_t command_begin) {
  double score = 0.0;
  for (std::size_t i = command_begin; i < list.commands.size(); ++i) {
    std::visit(
        [&](const auto& command) {
          using Cmd = std::decay_t<decltype(command)>;
          if constexpr (std::is_same_v<Cmd, PathCommand>) {
            for (const auto& point : command.points) {
              if (point_in_rect(pane_plot, point) && point_in_rect(candidate, point)) {
                score += 1.0;
              }
            }
          } else if constexpr (std::is_same_v<Cmd, TextCommand>) {
            if (point_in_rect(pane_plot, command.anchor) && point_in_rect(candidate, command.anchor)) {
              score += 2.0;
            }
          }
        },
        list.commands[i]);
  }
  return score;
}

void render_legend(DrawList& list, const Pane& pane, const PaneLayout& layout, std::size_t command_begin) {
  if (pane.legend_place() == KeyPlace::None) {
    return;
  }

  const auto entries = key_entries_for_pane(pane, false);
  if (entries.empty()) {
    return;
  }

  const bool outside_east = pane.legend_place() == KeyPlace::OutsideEast;
  const double min_width = outside_east ? 168.0 : 126.0;
  const double max_width = outside_east ? 280.0 : std::max(160.0, layout.plot.width * 0.52);
  const double box_width = estimate_key_width(entries, {}, min_width, max_width);
  const double box_height = compute_key_box_metrics(entries, {}, box_width).box_height;

  const auto rect_for_place = [&](KeyPlace place) {
    double x = layout.plot.x + layout.plot.width - box_width - 8.0;
    double y = layout.plot.y + 8.0;
    if (place == KeyPlace::NorthWest) {
      x = layout.plot.x + 8.0;
    } else if (place == KeyPlace::SouthWest) {
      x = layout.plot.x + 8.0;
      y = layout.plot.y + layout.plot.height - box_height - 8.0;
    } else if (place == KeyPlace::SouthEast) {
      y = layout.plot.y + layout.plot.height - box_height - 8.0;
    } else if (place == KeyPlace::OutsideEast) {
      x = layout.plot.x + layout.plot.width + 22.0;
      y = layout.plot.y + 8.0;
    }
    return Rect{x, y, box_width, 0.0};
  };

  Rect legend_box{};
  if (pane.legend_place() == KeyPlace::Auto) {
    constexpr std::array<KeyPlace, 4> kCandidates{
        KeyPlace::NorthEast,
        KeyPlace::NorthWest,
        KeyPlace::SouthEast,
        KeyPlace::SouthWest,
    };
    double best_score = std::numeric_limits<double>::infinity();
    KeyPlace best_place = kCandidates.front();
    for (const KeyPlace place : kCandidates) {
      const Rect candidate = rect_for_place(place);
      const double score = legend_overlap_score(list, candidate, layout.plot, command_begin);
      if (score < best_score) {
        best_score = score;
        best_place = place;
      }
    }
    legend_box = rect_for_place(best_place);
  } else {
    legend_box = rect_for_place(pane.legend_place());
  }

  render_key_box(
      list,
      pane.theme_tokens(),
      legend_box,
      {},
      entries,
      pane.legend_frame());
}

void render_legend_dock(
    DrawList& list,
    const Studio& studio,
    const std::vector<PaneLayout>& layouts) {
  if (studio.legend_dock() == KeyDock::None || layouts.empty()) {
    return;
  }

  std::vector<KeyEntry> entries;
  const auto visible_panes = static_cast<std::size_t>(std::count_if(
      studio.panes().begin(),
      studio.panes().end(),
      [](const Pane& pane) { return pane.visible(); }));
  const bool include_titles = visible_panes > 1;
  for (const Pane& pane : studio.panes()) {
    if (!pane.visible()) {
      continue;
    }
    auto pane_entries = key_entries_for_pane(pane, include_titles);
    entries.insert(entries.end(),
                   std::make_move_iterator(pane_entries.begin()),
                   std::make_move_iterator(pane_entries.end()));
  }
  if (entries.empty()) {
    return;
  }

  Rect box{};
  if (studio.legend_dock() == KeyDock::East) {
    constexpr double kDockWidth = 220.0;
    box = Rect{
        studio.extent().width - (kDockWidth + 28.0),
        studio.headline_text().empty() ? 28.0 : 58.0,
        kDockWidth,
        0.0,
    };
  } else {
    box = Rect{
        54.0,
        studio.extent().height - 92.0,
        std::max(220.0, studio.extent().width - 108.0),
        0.0,
    };
  }

  render_key_box(list, studio.theme_tokens(), box, {}, entries, studio.legend_frame());
}

void render_shared_captions(
    DrawList& list,
    const Studio& studio,
    const std::vector<PaneLayout>& layouts) {
  std::map<std::string, GroupRect> x_groups;
  std::map<std::string, GroupRect> y_groups;
  std::map<std::string, GroupRect> z_groups;

  for (std::size_t i = 0; i < studio.pane_count(); ++i) {
    const Pane& pane = studio.panes()[i];
    if (!pane.visible()) {
      continue;
    }
    if (!pane.x_align_group().empty()) {
      x_groups[pane.x_align_group()].include(layouts[i].plot);
    }
    if (!pane.y_align_group().empty()) {
      y_groups[pane.y_align_group()].include(layouts[i].plot);
    }
    if (!pane.z_align_group().empty()) {
      z_groups[pane.z_align_group()].include(layouts[i].plot);
    }
  }

  for (const auto& [group, text] : studio.x_captions()) {
    const auto it = x_groups.find(group);
    if (it != x_groups.end() && it->second.valid && !text.empty()) {
      const Rect& rect = it->second.rect;
      add_text(
          list,
          {rect.x + rect.width * 0.5, rect.y + rect.height + 36.0},
          text,
          12.0,
          studio.theme_tokens().foreground(),
          TextCommand::Align::Middle);
    }
  }

  for (const auto& [group, text] : studio.y_captions()) {
    const auto it = y_groups.find(group);
    if (it != y_groups.end() && it->second.valid && !text.empty()) {
      const Rect& rect = it->second.rect;
      add_text(
          list,
          {rect.x - 18.0, rect.y + rect.height * 0.5},
          text,
          12.0,
          studio.theme_tokens().foreground(),
          TextCommand::Align::End);
    }
  }

  for (const auto& [group, text] : studio.z_captions()) {
    const auto it = z_groups.find(group);
    if (it != z_groups.end() && it->second.valid && !text.empty()) {
      const Rect& rect = it->second.rect;
      add_text(
          list,
          {rect.x + 8.0, rect.y - 10.0},
          text,
          12.0,
          studio.theme_tokens().foreground());
    }
  }
}

void render_scale_guide(DrawList& list, const Pane& pane, const PaneLayout& layout) {
  const auto guide = scale_guide_spec(pane);
  if (!guide.has_value()) {
    return;
  }

  const double bar_width = 14.0;
  const double bar_height = layout.plot.height;
  const double bar_x = layout.plot.x + layout.plot.width + 22.0;
  const double bar_y = layout.plot.y;
  constexpr int slices = 64;

  for (int i = 0; i < slices; ++i) {
    const double t0 = static_cast<double>(i) / static_cast<double>(slices);
    const double t1 = static_cast<double>(i + 1) / static_cast<double>(slices);
    const double y0 = bar_y + (1.0 - t1) * bar_height;
    const double y1 = bar_y + (1.0 - t0) * bar_height;
    const Rgba fill = guide->palette.sample(0.5 * (t0 + t1));
    add_path(
        list,
        {
            {bar_x, y0},
            {bar_x + bar_width, y0},
            {bar_x + bar_width, y1},
            {bar_x, y1},
        },
        FillStroke{fill, fill, false, true, 0.0},
        true);
  }

  add_path(
      list,
      {
          {bar_x, bar_y},
          {bar_x + bar_width, bar_y},
          {bar_x + bar_width, bar_y + bar_height},
          {bar_x, bar_y + bar_height},
      },
      FillStroke{pane.theme_tokens().foreground(), {}, true, false, 0.8},
      true);

  const auto ticks = linear_ticks(guide->range, 5);
  for (const auto& [value, label] : ticks) {
    const double t = (value - guide->range.low) / std::max(guide->range.high - guide->range.low, 1e-12);
    const double y = bar_y + (1.0 - std::clamp(t, 0.0, 1.0)) * bar_height;
    add_path(
        list,
        {{bar_x + bar_width, y}, {bar_x + bar_width + 5.0, y}},
        FillStroke{pane.theme_tokens().foreground(), {}, true, false, 0.8},
        false);
    add_text(
        list,
        {bar_x + bar_width + 8.0, y + 4.0},
        label,
        9.5,
        pane.theme_tokens().foreground());
  }
}

void render_planar_cues(
    DrawList& list,
    const Pane& pane,
    const PaneLayout& layout,
    const Bounds2d& bounds,
    bool radial_only) {
  for (const BandCue& band : pane.bands()) {
    if (band.axis == CueAxis::Z) {
      continue;
    }
    if (band.span.high < band.span.low) {
      continue;
    }
    if (band.axis == CueAxis::X) {
      const Point2d p0 = map_planar_point(band.span.low, bounds.min_y, pane, bounds, layout.plot, radial_only);
      const Point2d p1 = map_planar_point(band.span.high, bounds.max_y, pane, bounds, layout.plot, radial_only);
      add_path(
          list,
          {
              {p0.x, layout.plot.y},
              {p1.x, layout.plot.y},
              {p1.x, layout.plot.y + layout.plot.height},
              {p0.x, layout.plot.y + layout.plot.height},
          },
          FillStroke{band.stroke, band.fill, true, true, 0.7},
          true);
      if (!band.label.empty()) {
        add_text(list, {0.5 * (p0.x + p1.x), layout.plot.y + 14.0}, band.label, 10.0, band.stroke,
                 TextCommand::Align::Middle);
      }
    } else if (band.axis == CueAxis::Y) {
      const Point2d p0 = map_planar_point(bounds.min_x, band.span.low, pane, bounds, layout.plot, radial_only);
      const Point2d p1 = map_planar_point(bounds.max_x, band.span.high, pane, bounds, layout.plot, radial_only);
      add_path(
          list,
          {
              {layout.plot.x, p1.y},
              {layout.plot.x + layout.plot.width, p1.y},
              {layout.plot.x + layout.plot.width, p0.y},
              {layout.plot.x, p0.y},
          },
          FillStroke{band.stroke, band.fill, true, true, 0.7},
          true);
      if (!band.label.empty()) {
        add_text(list, {layout.plot.x + 10.0, 0.5 * (p0.y + p1.y)}, band.label, 10.0, band.stroke);
      }
    }
  }

  for (const RuleCue& rule : pane.rules()) {
    if (rule.axis == CueAxis::Z) {
      continue;
    }
    if (rule.axis == CueAxis::X) {
      const Point2d p = map_planar_point(rule.value, bounds.min_y, pane, bounds, layout.plot, radial_only);
      add_path(
          list,
          {{p.x, layout.plot.y}, {p.x, layout.plot.y + layout.plot.height}},
          FillStroke{rule.color, {}, true, false, rule.width},
          false);
      if (!rule.label.empty()) {
        add_text(list, {p.x + 4.0, layout.plot.y + 14.0}, rule.label, 10.0, rule.color);
      }
    } else if (rule.axis == CueAxis::Y) {
      const Point2d p = map_planar_point(bounds.min_x, rule.value, pane, bounds, layout.plot, radial_only);
      add_path(
          list,
          {{layout.plot.x, p.y}, {layout.plot.x + layout.plot.width, p.y}},
          FillStroke{rule.color, {}, true, false, rule.width},
          false);
      if (!rule.label.empty()) {
        add_text(list, {layout.plot.x + 10.0, p.y - 4.0}, rule.label, 10.0, rule.color);
      }
    }
  }

  for (const PinCue& pin : pane.pins()) {
    const Point2d anchor = map_planar_point(pin.anchor.x, pin.anchor.y, pane, bounds, layout.plot, radial_only);
    const Point2d label{anchor.x + pin.offset_x, anchor.y + pin.offset_y};
    add_path(list, {anchor, label}, FillStroke{pin.color, {}, true, false, 0.9}, false);
    add_path(
        list,
        {
            {anchor.x - 2.5, anchor.y - 2.5},
            {anchor.x + 2.5, anchor.y - 2.5},
            {anchor.x + 2.5, anchor.y + 2.5},
            {anchor.x - 2.5, anchor.y + 2.5},
        },
        FillStroke{pin.color, pin.color, true, true, 0.5},
        true);
    add_text(list, {label.x + 4.0, label.y - 2.0}, pin.text, 10.0, pin.color);
  }
}

void render_volume_cues(
    DrawList& list,
    const Pane& pane,
    const PaneLayout& layout,
    const Bounds3d& bounds) {
  auto draw_plane = [&](CueAxis axis, double value, const FillStroke& style, bool filled) {
    std::array<Point3d, 4> corners{};
    if (axis == CueAxis::X) {
      corners = {{{value, bounds.min_y, bounds.min_z},
                  {value, bounds.max_y, bounds.min_z},
                  {value, bounds.max_y, bounds.max_z},
                  {value, bounds.min_y, bounds.max_z}}};
    } else if (axis == CueAxis::Y) {
      corners = {{{bounds.min_x, value, bounds.min_z},
                  {bounds.max_x, value, bounds.min_z},
                  {bounds.max_x, value, bounds.max_z},
                  {bounds.min_x, value, bounds.max_z}}};
    } else {
      corners = {{{bounds.min_x, bounds.min_y, value},
                  {bounds.max_x, bounds.min_y, value},
                  {bounds.max_x, bounds.max_y, value},
                  {bounds.min_x, bounds.max_y, value}}};
    }
    std::vector<Point2d> polygon;
    polygon.reserve(corners.size());
    for (const Point3d& corner : corners) {
      polygon.push_back(map_projected_point(corner, pane, bounds, layout.plot));
    }
    add_path(list, std::move(polygon), style, filled);
  };

  for (const BandCue& band : pane.bands()) {
    if (band.span.high < band.span.low) {
      continue;
    }
    draw_plane(
        band.axis,
        band.span.low,
        FillStroke{band.stroke, band.fill, true, true, 0.6},
        true);
    draw_plane(
        band.axis,
        band.span.high,
        FillStroke{band.stroke, band.fill, true, true, 0.6},
        true);
    if (!band.label.empty()) {
      const Point3d anchor = band.axis == CueAxis::X
                                 ? Point3d{0.5 * (band.span.low + band.span.high), bounds.max_y, bounds.max_z}
                                 : (band.axis == CueAxis::Y ? Point3d{bounds.max_x, 0.5 * (band.span.low + band.span.high), bounds.max_z}
                                                            : Point3d{bounds.max_x, bounds.max_y, 0.5 * (band.span.low + band.span.high)});
      const Point2d label = map_projected_point(anchor, pane, bounds, layout.plot);
      add_text(list, {label.x + 6.0, label.y - 4.0}, band.label, 10.0, band.stroke);
    }
  }

  for (const RuleCue& rule : pane.rules()) {
    draw_plane(rule.axis, rule.value, FillStroke{rule.color, {}, true, false, rule.width}, false);
    if (!rule.label.empty()) {
      const Point3d anchor = rule.axis == CueAxis::X
                                 ? Point3d{rule.value, bounds.max_y, bounds.max_z}
                                 : (rule.axis == CueAxis::Y ? Point3d{bounds.max_x, rule.value, bounds.max_z}
                                                            : Point3d{bounds.max_x, bounds.max_y, rule.value});
      const Point2d label = map_projected_point(anchor, pane, bounds, layout.plot);
      add_text(list, {label.x + 6.0, label.y - 4.0}, rule.label, 10.0, rule.color);
    }
  }

  for (const PinCue& pin : pane.pins()) {
    const Point2d anchor = map_projected_point({pin.anchor.x, pin.anchor.y, pin.anchor.z}, pane, bounds, layout.plot);
    const Point2d label{anchor.x + pin.offset_x, anchor.y + pin.offset_y};
    add_path(list, {anchor, label}, FillStroke{pin.color, {}, true, false, 0.9}, false);
    add_path(
        list,
        {
            {anchor.x - 2.5, anchor.y - 2.5},
            {anchor.x + 2.5, anchor.y - 2.5},
            {anchor.x + 2.5, anchor.y + 2.5},
            {anchor.x - 2.5, anchor.y + 2.5},
        },
        FillStroke{pin.color, pin.color, true, true, 0.5},
        true);
    add_text(list, {label.x + 4.0, label.y - 2.0}, pin.text, 10.0, pin.color);
  }
}

void render_planar_axes(
    DrawList& list,
    const Pane& pane,
    const PaneLayout& layout,
    const Bounds2d& bounds,
    bool radial_only,
    bool wedge_only) {
  const auto caption_only_panel = [&]() {
    const bool no_traces =
        std::none_of(pane.traces().begin(), pane.traces().end(), [](const Trace& trace) { return trace.visible(); });
    return no_traces
           && pane.rules().empty()
           && pane.bands().empty()
           && pane.pins().empty()
           && pane.grid_style() == GridStyle::None
           && pane.x_axis().label.empty()
           && pane.y_axis().label.empty()
           && !pane.x_axis().ticks.has_value()
           && !pane.y_axis().ticks.has_value()
           && !pane.caption_text().empty();
  };

  add_path(
      list,
      {
          {layout.outer.x, layout.outer.y},
          {layout.outer.x + layout.outer.width, layout.outer.y},
          {layout.outer.x + layout.outer.width, layout.outer.y + layout.outer.height},
          {layout.outer.x, layout.outer.y + layout.outer.height},
      },
      FillStroke{
          pane.theme_tokens().foreground(),
          pane.theme_tokens().background(),
          false,
          true,
          0.0,
      },
      true);

  if (!pane.title_text().empty()) {
    add_text(
        list,
        {layout.outer.x + layout.outer.width * 0.5, layout.outer.y + 22.0},
        pane.title_text(),
        14.0,
        pane.theme_tokens().foreground(),
        TextCommand::Align::Middle);
  }

  const double tick_font =
      std::clamp(10.0 * pane.tick_font_scale(), 7.0, 32.0);
  const double axis_label_font =
      std::clamp(11.0 * pane.axis_label_font_scale(), 8.0, 36.0);
  const double x_tick_rotation = pane.x_axis().tick_rotation_deg.value_or(0.0);
  const double y_tick_rotation = pane.y_axis().tick_rotation_deg.value_or(0.0);
  const double x_label_rotation = pane.x_axis().label_rotation_deg.value_or(0.0);
  const double y_label_rotation = pane.y_axis().label_rotation_deg.value_or(0.0);
  const bool axes_visible = pane.axes_visible();

  if (wedge_only) {
    add_caption_block(list, pane, layout);
    return;
  }

  if (caption_only_panel()) {
    add_caption_block(list, pane, layout);
    return;
  }

  if (radial_only) {
    if (axes_visible) {
      const double radius = 0.5 * std::min(layout.plot.width, layout.plot.height);
      const Point2d center{layout.plot.x + layout.plot.width * 0.5, layout.plot.y + layout.plot.height * 0.5};
      for (int ring = 1; ring <= 4; ++ring) {
        std::vector<Point2d> circle;
        for (int i = 0; i <= 96; ++i) {
          const double angle = 2.0 * kPi * static_cast<double>(i) / 96.0;
          const double ring_radius = radius * static_cast<double>(ring) / 4.0;
          circle.push_back({center.x + std::cos(angle) * ring_radius, center.y + std::sin(angle) * ring_radius});
        }
        add_path(
            list,
            std::move(circle),
            FillStroke{Rgba::from_u8(195, 203, 214), {}, true, false, ring == 4 ? 1.0 : 0.6},
            false);
      }
      for (int spoke = 0; spoke < 12; ++spoke) {
        const double angle = 2.0 * kPi * static_cast<double>(spoke) / 12.0;
        add_path(
            list,
            {
                center,
                {center.x + std::cos(angle) * radius, center.y + std::sin(angle) * radius},
            },
            FillStroke{Rgba::from_u8(215, 221, 230), {}, true, false, 0.6},
            false);
      }
    }
  } else {
    const auto x_ticks = clamp_ticks(
        ticks_for(pane.x_axis(), {bounds.min_x, bounds.max_x}),
        {bounds.min_x, bounds.max_x},
        pane.x_axis());
    const auto y_ticks = clamp_ticks(
        ticks_for(pane.y_axis(), {bounds.min_y, bounds.max_y}),
        {bounds.min_y, bounds.max_y},
        pane.y_axis());
    const auto x_minor = pane.x_axis().minor_grid
                             ? minor_tick_values(pane.x_axis(), {bounds.min_x, bounds.max_x}, x_ticks)
                             : std::vector<double>{};
    const auto y_minor = pane.y_axis().minor_grid
                             ? minor_tick_values(pane.y_axis(), {bounds.min_y, bounds.max_y}, y_ticks)
                             : std::vector<double>{};
    const Rgba bg = pane.theme_tokens().background();
    const std::string_view theme_name = pane.theme_tokens().name();
    const bool darkgrid_style = theme_name.find("darkgrid") != std::string_view::npos;
    const bool dark_style = !darkgrid_style &&
                            (theme_name.find("flash_style_dark") != std::string_view::npos ||
                             theme_name.find("flash_dark") != std::string_view::npos);
    const double bg_luma = 0.2126 * bg.red + 0.7152 * bg.green + 0.0722 * bg.blue;
    const bool dark_background = bg_luma < 0.5;
    if (darkgrid_style || dark_style) {
      const Rgba plot_face = darkgrid_style ? Rgba::from_u8(239, 242, 246) : Rgba::from_u8(231, 236, 242);
      add_path(
          list,
          {
              {layout.plot.x, layout.plot.y},
              {layout.plot.x + layout.plot.width, layout.plot.y},
              {layout.plot.x + layout.plot.width, layout.plot.y + layout.plot.height},
              {layout.plot.x, layout.plot.y + layout.plot.height},
              {layout.plot.x, layout.plot.y},
          },
          FillStroke{plot_face, plot_face, false, true, 0.0},
          true);
    }
    const Rgba minor_grid_color = darkgrid_style
                                      ? Rgba::from_u8(184, 192, 204, 186)
                                      : (dark_background ? Rgba::from_u8(73, 86, 104, 170)
                                                         : Rgba::from_u8(234, 238, 244));
    const Rgba major_grid_color = darkgrid_style
                                      ? Rgba::from_u8(151, 163, 181, 210)
                                      : (dark_background ? Rgba::from_u8(96, 111, 131, 190)
                                                         : Rgba::from_u8(224, 229, 236));

    if (pane.x_axis().minor_grid) {
      for (const double value : x_minor) {
        const Point2d p = map_planar_point(value, bounds.min_y, pane, bounds, layout.plot, false);
        add_path(
            list,
            {{p.x, layout.plot.y}, {p.x, layout.plot.y + layout.plot.height}},
            FillStroke{minor_grid_color, {}, true, false, 0.5},
            false);
      }
    }
    if (pane.y_axis().minor_grid) {
      for (const double value : y_minor) {
        const Point2d p = map_planar_point(bounds.min_x, value, pane, bounds, layout.plot, false);
        add_path(
            list,
            {{layout.plot.x, p.y}, {layout.plot.x + layout.plot.width, p.y}},
            FillStroke{minor_grid_color, {}, true, false, 0.5},
            false);
      }
    }

    if (pane.x_axis().major_grid) {
      for (const auto& [value, _] : x_ticks) {
        const Point2d p = map_planar_point(value, bounds.min_y, pane, bounds, layout.plot, false);
        add_path(
            list,
            {{p.x, layout.plot.y}, {p.x, layout.plot.y + layout.plot.height}},
            FillStroke{major_grid_color, {}, true, false, 0.8},
            false);
      }
    }
    if (pane.y_axis().major_grid) {
      for (const auto& [value, _] : y_ticks) {
        const Point2d p = map_planar_point(bounds.min_x, value, pane, bounds, layout.plot, false);
        add_path(
            list,
            {{layout.plot.x, p.y}, {layout.plot.x + layout.plot.width, p.y}},
            FillStroke{major_grid_color, {}, true, false, 0.8},
            false);
      }
    }
    if (!axes_visible && (pane.x_axis().major_grid || pane.y_axis().major_grid)) {
      add_path(
          list,
          {
              {layout.plot.x, layout.plot.y},
              {layout.plot.x + layout.plot.width, layout.plot.y},
              {layout.plot.x + layout.plot.width, layout.plot.y + layout.plot.height},
              {layout.plot.x, layout.plot.y + layout.plot.height},
              {layout.plot.x, layout.plot.y},
          },
          FillStroke{major_grid_color, {}, true, false, 0.8},
          false);
    }
    const double tick_len = std::clamp(tick_font * 0.45, 4.0, 9.0);
    const double x_axis_y = layout.plot.y + layout.plot.height;
    const double y_axis_x = layout.plot.x;
    const Rgba tick_color = multiply_rgb(pane.theme_tokens().foreground(), 0.7);
    const Rgba axis_color = multiply_rgb(pane.theme_tokens().foreground(), 0.9);
    const double x_tick_gap = resolved_planar_tick_label_gap(pane.x_axis(), axes_visible, tick_font, true);
    const double y_tick_gap = resolved_planar_tick_label_gap(pane.y_axis(), axes_visible, tick_font, false);
    if (axes_visible) {
      add_path(
          list,
          {{layout.plot.x, x_axis_y}, {layout.plot.x + layout.plot.width, x_axis_y}},
          FillStroke{axis_color, {}, true, false, 1.0},
          false);
      add_path(
          list,
          {{y_axis_x, layout.plot.y}, {y_axis_x, layout.plot.y + layout.plot.height}},
          FillStroke{axis_color, {}, true, false, 1.0},
          false);
    }
    const auto x_tick_align = std::abs(x_tick_rotation) > 1e-9 ? TextCommand::Align::Start : TextCommand::Align::Middle;
    for (const auto& [value, label] : x_ticks) {
      const Point2d p = map_planar_point(value, bounds.min_y, pane, bounds, layout.plot, false);
      if (!std::isfinite(p.x) || !std::isfinite(p.y)) {
        continue;
      }
      if (axes_visible) {
        add_path(
            list,
            {{p.x, x_axis_y}, {p.x, x_axis_y + tick_len}},
            FillStroke{tick_color, {}, true, false, 0.9},
            false);
      }
      if (pane.x_axis().tick_labels_visible) {
        add_text(
            list,
            {p.x, x_axis_y + x_tick_gap + tick_font * 0.35},
            label,
            tick_font,
            pane.theme_tokens().foreground(),
            x_tick_align,
            x_tick_rotation);
      }
    }
    for (const auto& [value, label] : y_ticks) {
      const Point2d p = map_planar_point(bounds.min_x, value, pane, bounds, layout.plot, false);
      if (!std::isfinite(p.x) || !std::isfinite(p.y)) {
        continue;
      }
      if (axes_visible) {
        add_path(
            list,
            {{y_axis_x - tick_len, p.y}, {y_axis_x, p.y}},
            FillStroke{tick_color, {}, true, false, 0.9},
            false);
      }
      if (pane.y_axis().tick_labels_visible) {
        add_text(
            list,
            {y_axis_x - y_tick_gap, p.y + tick_font * 0.35},
            label,
            tick_font,
            pane.theme_tokens().foreground(),
            TextCommand::Align::End,
            y_tick_rotation);
      }
    }
  }

  if (!pane.x_axis().label.empty()) {
    const double x_axis_y = layout.plot.y + layout.plot.height;
    const double x_tick_len = axes_visible ? std::clamp(tick_font * 0.45, 4.0, 9.0) : 0.0;
    const double x_tick_gap = resolved_planar_tick_label_gap(pane.x_axis(), axes_visible, tick_font, true);
    const double x_label_gap = resolved_axis_label_gap(pane.x_axis(), axes_visible, axis_label_font, true);
    const double x_tick_extent_rot =
        pane.x_axis().tick_labels_visible
            ? tick_font * (std::abs(x_tick_rotation) <= 1e-9 ? 0.85 : 1.10)
            : 0.0;
    const double x_label_y = std::min(
        layout.outer.y + layout.outer.height - 8.0,
        x_axis_y + x_tick_len + x_tick_gap + x_tick_extent_rot + x_label_gap + axis_label_font * 0.85);
    add_text(
        list,
        {layout.plot.x + layout.plot.width * 0.5, x_label_y},
        pane.x_axis().label,
        axis_label_font,
        pane.theme_tokens().foreground(),
        TextCommand::Align::Middle,
        x_label_rotation);
  }
  if (!pane.y_axis().label.empty()) {
    const bool center_on_axis = pane.y_axis().label_centered_on_axis;
    const double y_label_gap = resolved_axis_label_gap(pane.y_axis(), axes_visible, axis_label_font, false);
    const double label_x = center_on_axis ? (layout.plot.x - (axis_label_font + y_label_gap)) : (layout.outer.x + 6.0);
    const double label_y = center_on_axis ? (layout.plot.y + layout.plot.height * 0.5) : (layout.plot.y - 8.0);
    const auto label_align = center_on_axis ? TextCommand::Align::Middle : TextCommand::Align::Start;
    const double label_rotation =
        pane.y_axis().label_rotation_deg.value_or(center_on_axis ? -90.0 : y_label_rotation);
    double label_font = axis_label_font;
    std::string label_text = pane.y_axis().label;
    if (center_on_axis) {
      const double rotation_mod = std::fmod(std::abs(label_rotation), 180.0);
      const bool near_vertical = std::abs(rotation_mod - 90.0) <= 20.0;
      if (near_vertical) {
        const double available = std::max(12.0, layout.plot.height - 10.0);
        const double needed = estimated_text_extent(label_text, label_font);
        if (needed > available) {
          label_font = std::max(7.0, label_font * (available / needed));
        }
        const double fitted_extent = estimated_text_extent(label_text, label_font);
        if (fitted_extent > available) {
          const std::size_t max_chars = static_cast<std::size_t>(
              std::clamp(std::floor(available / std::max(1.0, label_font * 0.56)), 4.0, 2048.0));
          if (label_text.size() > max_chars) {
            label_text = label_text.substr(0, max_chars > 3 ? max_chars - 3 : 1);
            label_text += "...";
          }
        }
      }
    }
    add_text(
        list,
        {label_x, label_y},
        label_text,
        label_font,
        pane.theme_tokens().foreground(),
        label_align,
        label_rotation);
  }
  add_caption_block(list, pane, layout);
}

[[nodiscard]] std::vector<Point2d> marker_circle(Point2d center, double radius, int steps = 14) {
  std::vector<Point2d> points;
  points.reserve(static_cast<std::size_t>(steps) + 1U);
  for (int i = 0; i < steps; ++i) {
    const double angle = 2.0 * kPi * static_cast<double>(i) / static_cast<double>(steps);
    points.push_back({center.x + radius * std::cos(angle), center.y + radius * std::sin(angle)});
  }
  return points;
}

void render_planar_traces(
    DrawList& list,
    const Pane& pane,
    const PaneLayout& layout,
    const Bounds2d& bounds,
    bool radial_only,
    bool wedge_only) {
  (void)wedge_only;
  for (const Trace& trace : pane.traces()) {
    if (!trace.visible()) {
      continue;
    }
    std::visit(
        [&](const auto& payload) {
          using Payload = std::decay_t<decltype(payload)>;
          if constexpr (std::is_same_v<Payload, TrendData>) {
            const auto xy = normalize_pairs(payload.x, payload.y);
            std::vector<Point2d> path;
            auto flush_segment = [&]() {
              if (path.size() >= 2) {
                add_path(
                    list,
                    path,
                    FillStroke{trace.stroke_color(), {}, true, false, trace.stroke_width()},
                    false);
              }
              path.clear();
            };
            for (std::size_t i = 0; i < xy.first.size(); ++i) {
              const Point2d mapped = map_planar_point(xy.first[i], xy.second[i], pane, bounds, layout.plot, false);
              if (std::isfinite(mapped.x) && std::isfinite(mapped.y)) {
                path.push_back(mapped);
              } else {
                flush_segment();
              }
            }
            flush_segment();
          } else if constexpr (std::is_same_v<Payload, CloudData>) {
            const auto cloud = normalize_cloud(payload);
            Range1d color_range{0.0, 1.0};
            if (payload.color.has_value()) {
              const Series color = flatten_f64(*payload.color);
              color_range = trace.color_limits().has_value()
                                ? *trace.color_limits()
                                : Range1d{scalar_f64(color.min()), scalar_f64(color.max())};
              if (!(color_range.high > color_range.low)) {
                color_range.low -= 0.5;
                color_range.high += 0.5;
              }
            }
            for (std::size_t i = 0; i < cloud.x.size(); ++i) {
              const Point2d center =
                  map_planar_point(cloud.x[i], cloud.y[i], pane, bounds, layout.plot, false);
              if (!std::isfinite(center.x) || !std::isfinite(center.y)) {
                continue;
              }
              const double radius = std::clamp(std::sqrt(std::max(cloud.size[i], 1.0)) * 0.31, 0.95, 9.0);
              Rgba fill = trace.stroke_color();
              if (cloud.color.has_value() && i < cloud.color->size()) {
                fill = palette_lookup(trace.palette_ramp(), color_range, (*cloud.color)[i]);
              }
              fill.alpha = std::clamp(fill.alpha * 0.82, 0.18, 1.0);
              add_path(
                  list,
                  marker_circle(center, radius),
                  FillStroke{{}, fill, false, true, 0.0},
                  true);
            }
          } else if constexpr (std::is_same_v<Payload, FieldmapData>) {
            const auto grid = normalize_surface_grid(payload);
            if (!grid.has_value()) {
              return;
            }
            const Range1d color_range = surface_color_range(pane, trace, *grid);
            for (std::int64_t row = 0; row < grid->rows; ++row) {
              for (std::int64_t col = 0; col < grid->cols; ++col) {
                const auto index = static_cast<std::size_t>(row * grid->cols + col);
                const auto cell = center_cell_polygon(*grid, row, col);
                std::vector<Point2d> polygon;
                polygon.reserve(cell.size());
                for (const Point2d& corner : cell) {
                  polygon.push_back(
                      map_planar_point(corner.x, corner.y, pane, bounds, layout.plot, false));
                }
                const double color_value =
                    grid->color.has_value() ? (*grid->color)[index] : grid->z[index];
                const Rgba fill = palette_lookup(trace.palette_ramp(), color_range, color_value);
                add_path(
                    list,
                    std::move(polygon),
                    FillStroke{multiply_rgb(fill, 0.55), fill, true, true, 0.4},
                    true);
              }
            }
          } else if constexpr (std::is_same_v<Payload, SpikeData>) {
            const auto xy = normalize_pairs(payload.x, payload.y);
            for (std::size_t i = 0; i < xy.first.size(); ++i) {
              const Point2d base =
                  map_planar_point(xy.first[i], payload.base, pane, bounds, layout.plot, false);
              const Point2d top =
                  map_planar_point(xy.first[i], xy.second[i], pane, bounds, layout.plot, false);
              add_path(list, {base, top}, FillStroke{trace.stroke_color(), {}, true, false, 1.0}, false);
              add_path(
                  list,
                  marker_circle(top, 3.0, 12),
                  FillStroke{trace.stroke_color(), pane.theme_tokens().background(), true, true, 0.9},
                  true);
            }
          } else if constexpr (std::is_same_v<Payload, WedgeData>) {
            const auto slices = wedge_slices(payload);
            const double radius = 0.45 * std::min(layout.plot.width, layout.plot.height);
            const Point2d center{layout.plot.x + 0.5 * layout.plot.width, layout.plot.y + 0.5 * layout.plot.height};
            for (std::size_t i = 0; i < slices.size(); ++i) {
              const auto& slice = slices[i];
              const double mid = 0.5 * (slice.start_angle + slice.end_angle);
              const double offset = slice.explode * radius * 0.18;
              const Point2d origin{center.x + std::cos(mid) * offset, center.y + std::sin(mid) * offset};
              std::vector<Point2d> polygon;
              polygon.push_back(origin);
              const int steps = std::max(8, static_cast<int>(48.0 * slice.fraction));
              for (int step = 0; step <= steps; ++step) {
                const double angle =
                    slice.start_angle + (slice.end_angle - slice.start_angle) * static_cast<double>(step) / static_cast<double>(steps);
                polygon.push_back({origin.x + std::cos(angle) * radius, origin.y + std::sin(angle) * radius});
              }
              const Rgba fill = trace.palette_ramp().sample(
                  static_cast<double>(i + 1) / static_cast<double>(slices.size() + 1));
              add_path(
                  list,
                  std::move(polygon),
                  FillStroke{multiply_rgb(fill, 0.55), fill, true, true, 0.9},
                  true);
              const Point2d label{
                  origin.x + std::cos(mid) * radius * 1.15,
                  origin.y + std::sin(mid) * radius * 1.15,
              };
              add_text(
                  list,
                  label,
                  slice.label,
                  10.0,
                  pane.theme_tokens().foreground(),
                  std::cos(mid) >= 0.0 ? TextCommand::Align::Start : TextCommand::Align::End);
            }
          } else if constexpr (std::is_same_v<Payload, RibbonData>) {
            const auto geometry = normalize_ribbon(payload);
            if (!geometry.has_value()) {
              return;
            }
            for (std::int64_t col = 0; col < geometry->cols; ++col) {
              std::vector<Point2d> polygon;
              polygon.reserve(static_cast<std::size_t>(geometry->rows) * 2U);
              const Rgba fill =
                  trace.palette_ramp().sample(geometry->cols == 1
                                                  ? 0.58
                                                  : static_cast<double>(col + 1) / static_cast<double>(geometry->cols + 1));
              for (std::int64_t row = 0; row < geometry->rows; ++row) {
                const auto index = static_cast<std::size_t>(row * geometry->cols + col);
                polygon.push_back(
                    map_planar_point(geometry->x[index], geometry->top[index], pane, bounds, layout.plot, false));
              }
              for (std::int64_t row = geometry->rows - 1; row >= 0; --row) {
                const auto index = static_cast<std::size_t>(row * geometry->cols + col);
                polygon.push_back(
                    map_planar_point(geometry->x[index], geometry->bottom[index], pane, bounds, layout.plot, false));
              }
              add_path(
                  list,
                  std::move(polygon),
                  FillStroke{multiply_rgb(fill, 0.55), fill, true, true, 0.9},
                  true);
            }
          } else if constexpr (std::is_same_v<Payload, WhiskData>) {
            const auto geometry = normalize_whisk(payload);
            if (!geometry.has_value()) {
              return;
            }
            for (std::int64_t col = 0; col < geometry->cols; ++col) {
              std::vector<Point2d> trace_path;
              trace_path.reserve(static_cast<std::size_t>(geometry->rows));
              for (std::int64_t row = 0; row < geometry->rows; ++row) {
                const auto index = static_cast<std::size_t>(row * geometry->cols + col);
                const Point2d center =
                    map_planar_point(geometry->x[index], geometry->y[index], pane, bounds, layout.plot, false);
                trace_path.push_back(center);

                if (geometry->has_y) {
                  const Point2d low =
                      map_planar_point(geometry->x[index], geometry->ylow[index], pane, bounds, layout.plot, false);
                  const Point2d high =
                      map_planar_point(geometry->x[index], geometry->yhigh[index], pane, bounds, layout.plot, false);
                  add_path(list, {low, high}, FillStroke{trace.stroke_color(), {}, true, false, 0.9}, false);
                  add_path(
                      list,
                      {{low.x - 4.0, low.y}, {low.x + 4.0, low.y}},
                      FillStroke{trace.stroke_color(), {}, true, false, 0.9},
                      false);
                  add_path(
                      list,
                      {{high.x - 4.0, high.y}, {high.x + 4.0, high.y}},
                      FillStroke{trace.stroke_color(), {}, true, false, 0.9},
                      false);
                }
                if (geometry->has_x) {
                  const Point2d left =
                      map_planar_point(geometry->xlow[index], geometry->y[index], pane, bounds, layout.plot, false);
                  const Point2d right =
                      map_planar_point(geometry->xhigh[index], geometry->y[index], pane, bounds, layout.plot, false);
                  add_path(list, {left, right}, FillStroke{trace.stroke_color(), {}, true, false, 0.9}, false);
                  add_path(
                      list,
                      {{left.x, left.y - 4.0}, {left.x, left.y + 4.0}},
                      FillStroke{trace.stroke_color(), {}, true, false, 0.9},
                      false);
                  add_path(
                      list,
                      {{right.x, right.y - 4.0}, {right.x, right.y + 4.0}},
                      FillStroke{trace.stroke_color(), {}, true, false, 0.9},
                      false);
                }
                add_path(
                    list,
                    marker_circle(center, 2.7, 12),
                    FillStroke{trace.stroke_color(), pane.theme_tokens().background(), true, true, 0.8},
                    true);
              }
              add_path(
                  list,
                  std::move(trace_path),
                  FillStroke{trace.stroke_color(), {}, true, false, trace.stroke_width()},
                  false);
            }
          } else if constexpr (std::is_same_v<Payload, ColumnsData>) {
            const auto xy = normalize_pairs(payload.x, payload.y);
            const double half_width = 0.5 * payload.width;
            const Rgba fill = trace.palette_ramp().sample(0.62);
            for (std::size_t i = 0; i < xy.first.size(); ++i) {
              Point2d base_left{};
              Point2d base_right{};
              Point2d top_left{};
              Point2d top_right{};
              if (payload.horizontal) {
                base_left = map_planar_point(payload.base, xy.second[i] - half_width, pane, bounds, layout.plot, false);
                base_right = map_planar_point(payload.base, xy.second[i] + half_width, pane, bounds, layout.plot, false);
                top_left = map_planar_point(xy.first[i], xy.second[i] - half_width, pane, bounds, layout.plot, false);
                top_right = map_planar_point(xy.first[i], xy.second[i] + half_width, pane, bounds, layout.plot, false);
              } else {
                base_left = map_planar_point(xy.first[i] - half_width, payload.base, pane, bounds, layout.plot, false);
                base_right = map_planar_point(xy.first[i] + half_width, payload.base, pane, bounds, layout.plot, false);
                top_left = map_planar_point(xy.first[i] - half_width, xy.second[i], pane, bounds, layout.plot, false);
                top_right = map_planar_point(xy.first[i] + half_width, xy.second[i], pane, bounds, layout.plot, false);
              }
              add_path(
                  list,
                  {
                      {base_left.x, base_left.y},
                      {base_right.x, base_right.y},
                      {top_right.x, top_right.y},
                      {top_left.x, top_left.y},
                  },
                  FillStroke{multiply_rgb(fill, 0.55), fill, true, true, 0.8},
                  true);
            }
          } else if constexpr (std::is_same_v<Payload, BucketData>) {
            const auto geometry = bucket_geometry(payload);
            const Rgba fill = trace.palette_ramp().sample(0.55);
            const double width =
                (geometry.high - geometry.low) / std::max<std::size_t>(geometry.counts.size(), 1U);
            for (std::size_t i = 0; i < geometry.counts.size(); ++i) {
              const double left = geometry.low + width * static_cast<double>(i);
              const double right = left + width;
              const Point2d base_left = map_planar_point(left, 0.0, pane, bounds, layout.plot, false);
              const Point2d base_right = map_planar_point(right, 0.0, pane, bounds, layout.plot, false);
              const Point2d top_left =
                  map_planar_point(left, geometry.counts[i], pane, bounds, layout.plot, false);
              const Point2d top_right =
                  map_planar_point(right, geometry.counts[i], pane, bounds, layout.plot, false);
              add_path(
                  list,
                  {
                      {base_left.x, base_left.y},
                      {base_right.x, base_right.y},
                      {top_right.x, top_right.y},
                      {top_left.x, top_left.y},
                  },
                  FillStroke{multiply_rgb(fill, 0.55), fill, true, true, 0.8},
                  true);
            }
          } else if constexpr (std::is_same_v<Payload, QuartetData>) {
            const auto summaries = quartet_summaries(payload);
            const Rgba fill = trace.palette_ramp().sample(0.42);
            const double half_width = 0.5 * payload.width;
            for (const auto& summary : summaries) {
              const Point2d q1_left =
                  map_planar_point(summary.group - half_width, summary.q1, pane, bounds, layout.plot, false);
              const Point2d q3_right =
                  map_planar_point(summary.group + half_width, summary.q3, pane, bounds, layout.plot, false);
              add_path(
                  list,
                  {
                      {q1_left.x, q1_left.y},
                      {q3_right.x, q1_left.y},
                      {q3_right.x, q3_right.y},
                      {q1_left.x, q3_right.y},
                  },
                  FillStroke{multiply_rgb(fill, 0.55), fill, true, true, 0.8},
                  true);

              const Point2d median_left =
                  map_planar_point(summary.group - half_width, summary.median, pane, bounds, layout.plot, false);
              const Point2d median_right =
                  map_planar_point(summary.group + half_width, summary.median, pane, bounds, layout.plot, false);
              add_path(list,
                       {median_left, median_right},
                       FillStroke{trace.stroke_color(), {}, true, false, std::max(1.2, static_cast<double>(trace.stroke_width()))},
                       false);

              const Point2d center_top =
                  map_planar_point(summary.group, summary.q3, pane, bounds, layout.plot, false);
              const Point2d whisker_top =
                  map_planar_point(summary.group, summary.high_whisker, pane, bounds, layout.plot, false);
              const Point2d center_bottom =
                  map_planar_point(summary.group, summary.q1, pane, bounds, layout.plot, false);
              const Point2d whisker_bottom =
                  map_planar_point(summary.group, summary.low_whisker, pane, bounds, layout.plot, false);
              add_path(list, {center_top, whisker_top}, FillStroke{trace.stroke_color(), {}, true, false, 0.9}, false);
              add_path(list, {center_bottom, whisker_bottom}, FillStroke{trace.stroke_color(), {}, true, false, 0.9}, false);

              const Point2d whisker_top_left =
                  map_planar_point(summary.group - half_width * 0.45, summary.high_whisker, pane, bounds, layout.plot, false);
              const Point2d whisker_top_right =
                  map_planar_point(summary.group + half_width * 0.45, summary.high_whisker, pane, bounds, layout.plot, false);
              const Point2d whisker_bottom_left =
                  map_planar_point(summary.group - half_width * 0.45, summary.low_whisker, pane, bounds, layout.plot, false);
              const Point2d whisker_bottom_right =
                  map_planar_point(summary.group + half_width * 0.45, summary.low_whisker, pane, bounds, layout.plot, false);
              add_path(list, {whisker_top_left, whisker_top_right}, FillStroke{trace.stroke_color(), {}, true, false, 0.9}, false);
              add_path(list,
                       {whisker_bottom_left, whisker_bottom_right},
                       FillStroke{trace.stroke_color(), {}, true, false, 0.9},
                       false);

              for (const double outlier : summary.outliers) {
                const Point2d center =
                    map_planar_point(summary.group, outlier, pane, bounds, layout.plot, false);
                add_path(
                    list,
                    marker_circle(center, 2.5, 10),
                    FillStroke{trace.stroke_color(), pane.theme_tokens().background(), true, true, 0.8},
                    true);
              }
            }
          } else if constexpr (std::is_same_v<Payload, ViolinData>) {
            const auto shapes = violin_shapes(payload);
            const Rgba fill = trace.palette_ramp().sample(0.42);
            for (const auto& shape : shapes) {
              if (shape.y.size() < 3 || shape.left_half_width.size() != shape.y.size()
                  || shape.right_half_width.size() != shape.y.size()) {
                continue;
              }

              std::vector<Point2d> polygon;
              polygon.reserve(shape.y.size() * 2);
              for (std::size_t i = 0; i < shape.y.size(); ++i) {
                const Point2d mapped =
                    map_planar_point(shape.group - shape.left_half_width[i], shape.y[i], pane, bounds, layout.plot, false);
                if (std::isfinite(mapped.x) && std::isfinite(mapped.y)) {
                  polygon.push_back(mapped);
                }
              }
              for (std::size_t i = shape.y.size(); i > 0; --i) {
                const std::size_t index = i - 1;
                const Point2d mapped =
                    map_planar_point(shape.group + shape.right_half_width[index], shape.y[index], pane, bounds, layout.plot, false);
                if (std::isfinite(mapped.x) && std::isfinite(mapped.y)) {
                  polygon.push_back(mapped);
                }
              }
              add_path(
                  list,
                  std::move(polygon),
                  FillStroke{multiply_rgb(fill, 0.58), fill, true, true, 0.9},
                  true);

              const auto median_width = widths_at_y(shape, shape.median);
              const auto q1_width = widths_at_y(shape, shape.q1);
              const auto q3_width = widths_at_y(shape, shape.q3);
              const auto whisk_low_width = widths_at_y(shape, shape.low_whisker);
              const auto whisk_high_width = widths_at_y(shape, shape.high_whisker);

              const auto add_inner_line = [&](double y,
                                              const ViolinHalfWidths& width,
                                              double stroke,
                                              bool emphasize = false) {
                const Point2d left = map_planar_point(shape.group - width.left, y, pane, bounds, layout.plot, false);
                const Point2d right = map_planar_point(shape.group + width.right, y, pane, bounds, layout.plot, false);
                add_path(
                    list,
                    {left, right},
                    FillStroke{
                        emphasize ? multiply_rgb(trace.stroke_color(), 0.82) : trace.stroke_color(),
                        {},
                        true,
                        false,
                        stroke},
                    false);
              };

              switch (shape.inner) {
                case ViolinInner::None:
                  break;
                case ViolinInner::Box: {
                  add_inner_line(shape.median, median_width, std::max(1.25, static_cast<double>(trace.stroke_width())), true);
                  const Point2d whisk_low =
                      map_planar_point(shape.group, shape.low_whisker, pane, bounds, layout.plot, false);
                  const Point2d whisk_high =
                      map_planar_point(shape.group, shape.high_whisker, pane, bounds, layout.plot, false);
                  add_path(list, {whisk_low, whisk_high}, FillStroke{trace.stroke_color(), {}, true, false, 0.9}, false);
                  add_inner_line(shape.q1, q1_width, 0.9);
                  add_inner_line(shape.q3, q3_width, 0.9);
                  add_inner_line(shape.low_whisker, whisk_low_width, 0.8);
                  add_inner_line(shape.high_whisker, whisk_high_width, 0.8);
                  break;
                }
                case ViolinInner::Quartile: {
                  add_inner_line(shape.q1, q1_width, 0.9);
                  add_inner_line(shape.median, median_width, std::max(1.2, static_cast<double>(trace.stroke_width())), true);
                  add_inner_line(shape.q3, q3_width, 0.9);
                  break;
                }
                case ViolinInner::Stick: {
                  const std::size_t n = shape.sample.size();
                  const std::size_t step = std::max<std::size_t>(1, n / 280);
                  for (std::size_t i = 0; i < n; i += step) {
                    const double value = shape.sample[i];
                    const auto width = widths_at_y(shape, value);
                    add_inner_line(value, {width.left * 0.92, width.right * 0.92}, 0.75);
                  }
                  break;
                }
                case ViolinInner::Point: {
                  const std::size_t n = shape.sample.size();
                  const std::size_t step = std::max<std::size_t>(1, n / 280);
                  for (std::size_t i = 0; i < n; i += step) {
                    const Point2d center = map_planar_point(shape.group, shape.sample[i], pane, bounds, layout.plot, false);
                    add_path(
                        list,
                        marker_circle(center, 1.9, 10),
                        FillStroke{trace.stroke_color(), pane.theme_tokens().background(), true, true, 0.65},
                        true);
                  }
                  break;
                }
              }
            }
          } else if constexpr (std::is_same_v<Payload, RadialData>) {
            const auto polar = normalize_pairs(payload.theta, payload.radius);
            std::vector<Point2d> path;
            for (std::size_t i = 0; i < polar.first.size(); ++i) {
              const double x = std::cos(polar.first[i]) * polar.second[i];
              const double y = std::sin(polar.first[i]) * polar.second[i];
              const Point2d mapped = map_planar_point(x, y, pane, bounds, layout.plot, radial_only);
              if (std::isfinite(mapped.x) && std::isfinite(mapped.y)) {
                path.push_back(mapped);
              }
            }
            add_path(list, std::move(path), FillStroke{trace.stroke_color(), {}, true, false, trace.stroke_width()}, false);
          } else if constexpr (std::is_same_v<Payload, ContourData>) {
            const auto grid = normalize_surface_grid(payload);
            if (!grid.has_value()) {
              return;
            }
            const std::vector<double> levels = contour_levels(payload, *grid);
            const Range1d color_range = contour_color_range(pane, trace, levels);

             if (trace.kind() == TraceKind::Isoband || payload.filled) {
              const auto intervals = contour_intervals(*grid, levels);
              for (const auto& [low, high] : intervals) {
                const double level_mid = 0.5 * (low + high);
                const Rgba fill = palette_lookup(trace.palette_ramp(), color_range, level_mid);
                for (std::int64_t row = 0; row + 1 < grid->rows; ++row) {
                  for (std::int64_t col = 0; col + 1 < grid->cols; ++col) {
                    const auto polygon_xy = contour_cell_band_polygon(*grid, row, col, low, high);
                    if (polygon_xy.size() < 3) {
                      continue;
                    }
                    std::vector<Point2d> polygon;
                    polygon.reserve(polygon_xy.size());
                    for (const Point2d& point : polygon_xy) {
                      const Point2d mapped =
                          map_planar_point(point.x, point.y, pane, bounds, layout.plot, false);
                      if (std::isfinite(mapped.x) && std::isfinite(mapped.y)) {
                        polygon.push_back(mapped);
                      }
                    }
                    if (polygon.size() >= 3) {
                      add_path(
                          list,
                          std::move(polygon),
                          FillStroke{multiply_rgb(fill, 0.65), fill, true, true, 0.18},
                          true);
                    }
                  }
                }
              }
            }

            for (const double level : levels) {
              const auto segments = contour_segments_for_level(*grid, level);
              Rgba stroke = trace.stroke_color();
              if (levels.size() > 1) {
                stroke = palette_lookup(trace.palette_ramp(), color_range, level);
              }
              double longest = -1.0;
              Point2d label_anchor{};
              for (const ContourSegment& segment : segments) {
                const Point2d start = map_planar_point(segment.start.x, segment.start.y, pane, bounds, layout.plot, false);
                const Point2d end = map_planar_point(segment.end.x, segment.end.y, pane, bounds, layout.plot, false);
                if (std::isfinite(start.x) && std::isfinite(start.y) &&
                    std::isfinite(end.x) && std::isfinite(end.y)) {
                  const double dx = end.x - start.x;
                  const double dy = end.y - start.y;
                  const double length = std::sqrt(dx * dx + dy * dy);
                  if (length > longest) {
                    longest = length;
                    label_anchor = {0.5 * (start.x + end.x), 0.5 * (start.y + end.y)};
                  }
                  add_path(list,
                           {start, end},
                           FillStroke{stroke, {}, true, false, trace.stroke_width()},
                           false);
                }
              }
              if (trace.annotate_levels() && longest > 24.0) {
                std::ostringstream label;
                label << std::setprecision(3) << level;
                add_text(
                    list,
                    label_anchor,
                    label.str(),
                    10.0,
                    pane.theme_tokens().foreground(),
                    TextCommand::Align::Middle);
              }
            }
          }
        },
        trace.payload());
  }
}

void render_volume_panel(DrawList& list, const Pane& pane, const PaneLayout& layout) {
  add_path(
      list,
      {
          {layout.outer.x, layout.outer.y},
          {layout.outer.x + layout.outer.width, layout.outer.y},
          {layout.outer.x + layout.outer.width, layout.outer.y + layout.outer.height},
          {layout.outer.x, layout.outer.y + layout.outer.height},
      },
      FillStroke{
          pane.theme_tokens().background(),
          pane.theme_tokens().background(),
          false,
          true,
          0.0,
      },
      true);
}

struct VolumeAxisFrame {
  double front_x{0.0};
  double rear_x{0.0};
  double front_y{0.0};
  double rear_y{0.0};
  double bottom_z{0.0};
  double top_z{0.0};
  Point3d x_start3{};
  Point3d x_end3{};
  Point3d y_start3{};
  Point3d y_end3{};
  Point3d z_start3{};
  Point3d z_end3{};
  Point2d x_start{};
  Point2d x_end{};
  Point2d y_start{};
  Point2d y_end{};
  Point2d z_start{};
  Point2d z_end{};
  Point2d pane_center{};
};

[[nodiscard]] VolumeAxisFrame resolve_volume_axis_frame(
    const Pane& pane,
    const Bounds3d& bounds,
    const Rect& plot_rect) {
  const CameraBasis basis = camera_basis(pane.lens());
  VolumeAxisFrame frame{};
  frame.front_x = basis.eye_direction.x >= 0.0 ? bounds.max_x : bounds.min_x;
  frame.rear_x = basis.eye_direction.x >= 0.0 ? bounds.min_x : bounds.max_x;
  frame.front_y = basis.eye_direction.y >= 0.0 ? bounds.max_y : bounds.min_y;
  frame.rear_y = basis.eye_direction.y >= 0.0 ? bounds.min_y : bounds.max_y;
  frame.bottom_z = bounds.min_z;
  frame.top_z = bounds.max_z;

  frame.x_start3 = {bounds.min_x, frame.front_y, frame.bottom_z};
  frame.x_end3 = {bounds.max_x, frame.front_y, frame.bottom_z};
  frame.y_start3 = {frame.front_x, bounds.min_y, frame.bottom_z};
  frame.y_end3 = {frame.front_x, bounds.max_y, frame.bottom_z};
  frame.z_start3 = frame.x_start3;
  frame.z_end3 = {frame.z_start3.x, frame.z_start3.y, frame.top_z};

  frame.x_start = map_projected_point(frame.x_start3, pane, bounds, plot_rect);
  frame.x_end = map_projected_point(frame.x_end3, pane, bounds, plot_rect);
  frame.y_start = map_projected_point(frame.y_start3, pane, bounds, plot_rect);
  frame.y_end = map_projected_point(frame.y_end3, pane, bounds, plot_rect);
  frame.z_start = map_projected_point(frame.z_start3, pane, bounds, plot_rect);
  frame.z_end = map_projected_point(frame.z_end3, pane, bounds, plot_rect);
  frame.pane_center = map_projected_point(bounds_center(bounds), pane, bounds, plot_rect);
  return frame;
}

void render_volume_grid(DrawList& list, const Pane& pane, const PaneLayout& layout, const Bounds3d& bounds) {
  if (pane.grid_style() == GridStyle::None) {
    return;
  }
  const auto x_ticks = clamp_ticks(
      ticks_for(pane.x_axis(), {bounds.min_x, bounds.max_x}),
      {bounds.min_x, bounds.max_x},
      pane.x_axis());
  const auto y_ticks = clamp_ticks(
      ticks_for(pane.y_axis(), {bounds.min_y, bounds.max_y}),
      {bounds.min_y, bounds.max_y},
      pane.y_axis());
  const auto z_ticks = clamp_ticks(
      ticks_for(pane.z_axis(), {bounds.min_z, bounds.max_z}),
      {bounds.min_z, bounds.max_z},
      pane.z_axis());
  std::vector<double> x_pos;
  std::vector<double> y_pos;
  std::vector<double> z_pos;
  x_pos.reserve(x_ticks.size());
  y_pos.reserve(y_ticks.size());
  z_pos.reserve(z_ticks.size());
  for (const auto& [value, _] : x_ticks) {
    x_pos.push_back(value);
  }
  for (const auto& [value, _] : y_ticks) {
    y_pos.push_back(value);
  }
  for (const auto& [value, _] : z_ticks) {
    z_pos.push_back(value);
  }
  if (x_pos.empty()) {
    x_pos = uniform_grid_positions({bounds.min_x, bounds.max_x}, 6);
  }
  if (y_pos.empty()) {
    y_pos = uniform_grid_positions({bounds.min_y, bounds.max_y}, 6);
  }
  if (z_pos.empty()) {
    z_pos = uniform_grid_positions({bounds.min_z, bounds.max_z}, 6);
  }
  const auto x_minor = pane.x_axis().minor_grid
                           ? minor_tick_values(pane.x_axis(), {bounds.min_x, bounds.max_x}, x_ticks)
                           : std::vector<double>{};
  const auto y_minor = pane.y_axis().minor_grid
                           ? minor_tick_values(pane.y_axis(), {bounds.min_y, bounds.max_y}, y_ticks)
                           : std::vector<double>{};
  const auto z_minor = pane.z_axis().minor_grid
                           ? minor_tick_values(pane.z_axis(), {bounds.min_z, bounds.max_z}, z_ticks)
                           : std::vector<double>{};

  const VolumeAxisFrame frame = resolve_volume_axis_frame(pane, bounds, layout.plot);
  const Rgba grid = Rgba::from_u8(190, 198, 208, 148);
  const Rgba minor_grid = Rgba::from_u8(205, 212, 221, 122);
  const double x_wall_y = frame.rear_y;
  const double y_wall_x = frame.rear_x;
  const auto draw_x_grid_lines = [&](double value, Rgba floor_color, double floor_width, Rgba wall_color, double wall_width) {
    add_path(
        list,
        {
            map_projected_point({value, bounds.min_y, frame.bottom_z}, pane, bounds, layout.plot),
            map_projected_point({value, bounds.max_y, frame.bottom_z}, pane, bounds, layout.plot),
        },
        FillStroke{floor_color, {}, true, false, floor_width},
        false);
    add_path(list,
             {
                 map_projected_point({value, x_wall_y, frame.bottom_z}, pane, bounds, layout.plot),
                 map_projected_point({value, x_wall_y, frame.top_z}, pane, bounds, layout.plot),
             },
             FillStroke{wall_color, {}, true, false, wall_width},
             false);
  };
  const auto draw_y_grid_lines = [&](double value, Rgba floor_color, double floor_width, Rgba wall_color, double wall_width) {
    add_path(
        list,
        {
            map_projected_point({bounds.min_x, value, frame.bottom_z}, pane, bounds, layout.plot),
            map_projected_point({bounds.max_x, value, frame.bottom_z}, pane, bounds, layout.plot),
        },
        FillStroke{floor_color, {}, true, false, floor_width},
        false);
    add_path(list,
             {
                 map_projected_point({y_wall_x, value, frame.bottom_z}, pane, bounds, layout.plot),
                 map_projected_point({y_wall_x, value, frame.top_z}, pane, bounds, layout.plot),
             },
             FillStroke{wall_color, {}, true, false, wall_width},
             false);
  };
  const auto draw_z_grid_lines = [&](double value, Rgba wall_color, double wall_width) {
    add_path(
        list,
        {
            map_projected_point({bounds.min_x, x_wall_y, value}, pane, bounds, layout.plot),
            map_projected_point({bounds.max_x, x_wall_y, value}, pane, bounds, layout.plot),
        },
        FillStroke{wall_color, {}, true, false, wall_width},
        false);
    add_path(
        list,
        {
            map_projected_point({y_wall_x, bounds.min_y, value}, pane, bounds, layout.plot),
            map_projected_point({y_wall_x, bounds.max_y, value}, pane, bounds, layout.plot),
        },
        FillStroke{wall_color, {}, true, false, wall_width},
        false);
  };

  for (const double value : x_minor) {
    draw_x_grid_lines(value, minor_grid, 0.38, minor_grid, 0.32);
  }
  for (const double value : y_minor) {
    draw_y_grid_lines(value, minor_grid, 0.38, minor_grid, 0.32);
  }
  for (const double value : z_minor) {
    draw_z_grid_lines(value, minor_grid, 0.32);
  }
  for (const double value : x_pos) {
    draw_x_grid_lines(value, grid, 0.65, grid, 0.55);
  }
  for (const double value : y_pos) {
    draw_y_grid_lines(value, grid, 0.65, grid, 0.55);
  }
  for (const double value : z_pos) {
    draw_z_grid_lines(value, grid, 0.55);
  }
}

void render_volume_axes(DrawList& list, const Pane& pane, const PaneLayout& layout, const Bounds3d& bounds) {
  if (!pane.title_text().empty()) {
    add_text(
        list,
        {layout.outer.x + layout.outer.width * 0.5, layout.outer.y + 22.0},
        pane.title_text(),
        14.0,
        pane.theme_tokens().foreground(),
        TextCommand::Align::Middle);
  }
  const bool axes_visible = pane.axes_visible();

  const auto x_ticks = clamp_ticks(
      ticks_for(pane.x_axis(), {bounds.min_x, bounds.max_x}),
      {bounds.min_x, bounds.max_x},
      pane.x_axis());
  const auto y_ticks = clamp_ticks(
      ticks_for(pane.y_axis(), {bounds.min_y, bounds.max_y}),
      {bounds.min_y, bounds.max_y},
      pane.y_axis());
  const auto z_ticks = clamp_ticks(
      ticks_for(pane.z_axis(), {bounds.min_z, bounds.max_z}),
      {bounds.min_z, bounds.max_z},
      pane.z_axis());
  const VolumeAxisFrame frame = resolve_volume_axis_frame(pane, bounds, layout.plot);
  const Rgba primary = multiply_rgb(pane.theme_tokens().foreground(), 0.58);
  const Rgba tick_stroke = multiply_rgb(pane.theme_tokens().foreground(), 0.72);

  const auto outward_normal = [&](Point2d a, Point2d b) {
    const double dx = b.x - a.x;
    const double dy = b.y - a.y;
    const double length = std::hypot(dx, dy);
    if (!(length > 1e-12)) {
      return Point2d{0.0, 1.0};
    }
    Point2d normal{-dy / length, dx / length};
    const Point2d midpoint{0.5 * (a.x + b.x), 0.5 * (a.y + b.y)};
    const Point2d to_center{frame.pane_center.x - midpoint.x, frame.pane_center.y - midpoint.y};
    if (normal.x * to_center.x + normal.y * to_center.y > 0.0) {
      normal.x = -normal.x;
      normal.y = -normal.y;
    }
    return normal;
  };
  const auto label_angle_deg = [](Point2d a, Point2d b) {
    double angle = std::atan2(b.y - a.y, b.x - a.x) * 180.0 / kPi;
    if (angle > 90.0) {
      angle -= 180.0;
    } else if (angle < -90.0) {
      angle += 180.0;
    }
    return angle;
  };
  const auto tick_text_align = [](Point2d outward) {
    if (std::abs(outward.x) < 0.35) {
      return TextCommand::Align::Middle;
    }
    return outward.x > 0.0 ? TextCommand::Align::Start : TextCommand::Align::End;
  };
  const auto unit_direction = [](Point2d from, Point2d to, Point2d fallback) {
    const double dx = to.x - from.x;
    const double dy = to.y - from.y;
    const double length = std::hypot(dx, dy);
    if (length > 1e-12) {
      return Point2d{dx / length, dy / length};
    }
    return fallback;
  };
  const auto max_tick_label_chars = [](const auto& ticks) {
    std::size_t max_chars = 1;
    for (const auto& entry : ticks) {
      max_chars = std::max(max_chars, entry.second.size());
    }
    return max_chars;
  };
  const auto estimated_text_extent = [](std::size_t chars, double font_size) {
    return static_cast<double>(chars) * font_size * 0.56;
  };
  const double plot_scale = std::max(1.0, std::min(layout.plot.width, layout.plot.height));
  const double tick_font_base = std::clamp(plot_scale * 0.016, 9.5, 11.8);
  const double axis_font_base = std::clamp(plot_scale * 0.019, 12.0, 14.6);
  const double tick_font_size = std::clamp(tick_font_base * pane.tick_font_scale(), 8.0, 24.0);
  const double axis_font_size = std::clamp(axis_font_base * pane.axis_label_font_scale(), 9.0, 32.0);
  const double tick_len_xy = std::clamp(plot_scale * 0.013, 5.8, 12.5);
  const double tick_len_z = std::clamp(plot_scale * 0.012, 5.2, 11.0);
  const double tick_to_text_gap = std::clamp(plot_scale * 0.020, 14.0, 30.0);
  const std::size_t x_tick_chars = max_tick_label_chars(x_ticks);
  const std::size_t y_tick_chars = max_tick_label_chars(y_ticks);
  const std::size_t z_tick_chars = max_tick_label_chars(z_ticks);
  const double x_tick_extent = estimated_text_extent(x_tick_chars, tick_font_size);
  const double y_tick_extent = estimated_text_extent(y_tick_chars, tick_font_size);
  const double z_tick_extent = estimated_text_extent(z_tick_chars, tick_font_size);
  const double axis_label_base_gap = std::clamp(plot_scale * 0.042, 26.0, 66.0);
  const double x_label_gap =
      axis_label_base_gap + tick_len_xy + tick_to_text_gap + std::clamp(x_tick_extent * 0.55, 12.0, 48.0);
  const double y_label_gap =
      axis_label_base_gap + tick_len_xy + tick_to_text_gap + std::clamp(y_tick_extent * 0.55, 12.0, 48.0);
  const double z_label_gap =
      axis_label_base_gap + tick_len_z + tick_to_text_gap + std::clamp(z_tick_extent * 0.52, 10.0, 42.0);

  const Point2d x_outward = outward_normal(frame.x_start, frame.x_end);
  const Point2d y_outward = outward_normal(frame.y_start, frame.y_end);
  const Point2d z_axis_start = frame.z_start;
  const Point2d z_axis_end = frame.z_end;
  const Point2d z_outward = outward_normal(z_axis_start, z_axis_end);
  const auto distance_sq = [](Point2d a, Point2d b) {
    const double dx = a.x - b.x;
    const double dy = a.y - b.y;
    return dx * dx + dy * dy;
  };
  struct EndpointPair {
    bool x_start{false};
    bool y_start{false};
    double dist_sq{0.0};
  };
  EndpointPair shared_pair{true, true, distance_sq(frame.x_start, frame.y_start)};
  for (const EndpointPair candidate : {
           EndpointPair{true, false, distance_sq(frame.x_start, frame.y_end)},
           EndpointPair{false, true, distance_sq(frame.x_end, frame.y_start)},
           EndpointPair{false, false, distance_sq(frame.x_end, frame.y_end)},
       }) {
    if (candidate.dist_sq < shared_pair.dist_sq) {
      shared_pair = candidate;
    }
  }
  const Point2d xy_corner = shared_pair.x_start ? frame.x_start : frame.x_end;
  const Point2d x_opposite_corner = shared_pair.x_start ? frame.x_end : frame.x_start;
  const Point2d y_opposite_corner = shared_pair.y_start ? frame.y_end : frame.y_start;
  const Point2d x_extension_at_xy = unit_direction(x_opposite_corner, xy_corner, x_outward);
  const Point2d y_extension_at_xy = unit_direction(y_opposite_corner, xy_corner, y_outward);
  const Point2d z_parallel_tick_dir = unit_direction(xy_corner, x_opposite_corner, x_extension_at_xy);
  const double x_tick_rotation = pane.x_axis().tick_rotation_deg.value_or(0.0);
  const double y_tick_rotation = pane.y_axis().tick_rotation_deg.value_or(0.0);
  const double z_tick_rotation = pane.z_axis().tick_rotation_deg.value_or(0.0);

  if (axes_visible) {
    add_path(list, {frame.x_start, frame.x_end}, FillStroke{primary, {}, true, false, 1.25}, false);
    add_path(list, {frame.y_start, frame.y_end}, FillStroke{primary, {}, true, false, 1.25}, false);
    add_path(list, {z_axis_start, z_axis_end}, FillStroke{primary, {}, true, false, 1.25}, false);
  }

  for (std::size_t i = 0; i < x_ticks.size(); ++i) {
    const auto& [value, label] = x_ticks[i];
    const Point2d p = map_projected_point({value, frame.front_y, frame.bottom_z}, pane, bounds, layout.plot);
    const Point2d rear = map_projected_point({value, frame.rear_y, frame.bottom_z}, pane, bounds, layout.plot);
    const Point2d toward_interior = unit_direction(p, rear, x_outward);
    Point2d tick_dir{-toward_interior.x, -toward_interior.y};
    if (!x_ticks.empty() && i + 1 == x_ticks.size()) {
      tick_dir = y_extension_at_xy;
    }
    const Point2d tick_end{p.x + tick_dir.x * tick_len_xy, p.y + tick_dir.y * tick_len_xy};
    const Point2d text_anchor{
        p.x + tick_dir.x * (tick_len_xy + tick_to_text_gap),
        p.y + tick_dir.y * (tick_len_xy + tick_to_text_gap),
    };
    if (axes_visible) {
      add_path(list, {p, tick_end}, FillStroke{tick_stroke, {}, true, false, 1.16}, false);
    }
    if (pane.x_axis().tick_labels_visible) {
      add_text(list,
               text_anchor,
               label,
               tick_font_size,
               pane.theme_tokens().foreground(),
               tick_text_align(tick_dir),
               x_tick_rotation);
    }
  }
  for (std::size_t i = 0; i < y_ticks.size(); ++i) {
    const auto& [value, label] = y_ticks[i];
    const Point2d p = map_projected_point({frame.front_x, value, frame.bottom_z}, pane, bounds, layout.plot);
    const Point2d rear = map_projected_point({frame.rear_x, value, frame.bottom_z}, pane, bounds, layout.plot);
    const Point2d toward_interior = unit_direction(p, rear, y_outward);
    Point2d tick_dir{-toward_interior.x, -toward_interior.y};
    if (!y_ticks.empty() && i == 0) {
      tick_dir = x_extension_at_xy;
    }
    const Point2d tick_end{p.x + tick_dir.x * tick_len_xy, p.y + tick_dir.y * tick_len_xy};
    const Point2d text_anchor{
        p.x + tick_dir.x * (tick_len_xy + tick_to_text_gap),
        p.y + tick_dir.y * (tick_len_xy + tick_to_text_gap),
    };
    if (axes_visible) {
      add_path(list, {p, tick_end}, FillStroke{tick_stroke, {}, true, false, 1.16}, false);
    }
    if (pane.y_axis().tick_labels_visible) {
      add_text(list,
               text_anchor,
               label,
               tick_font_size,
               pane.theme_tokens().foreground(),
               tick_text_align(tick_dir),
               y_tick_rotation);
    }
  }
  for (std::size_t i = 0; i < z_ticks.size(); ++i) {
    const auto& [value, label] = z_ticks[i];
    const Point2d p = map_projected_point({frame.z_start3.x, frame.z_start3.y, value}, pane, bounds, layout.plot);
    const Point2d tick_dir = z_parallel_tick_dir;
    const Point2d tick_end{p.x + tick_dir.x * tick_len_z, p.y + tick_dir.y * tick_len_z};
    const Point2d text_anchor{
        p.x + tick_dir.x * (tick_len_z + tick_to_text_gap),
        p.y + tick_dir.y * (tick_len_z + tick_to_text_gap),
    };
    if (axes_visible) {
      add_path(list, {p, tick_end}, FillStroke{tick_stroke, {}, true, false, 1.16}, false);
    }
    if (pane.z_axis().tick_labels_visible) {
      add_text(list,
               text_anchor,
               label,
               tick_font_size,
               pane.theme_tokens().foreground(),
               tick_text_align(tick_dir),
               z_tick_rotation);
    }
  }

  if (!pane.x_axis().label.empty()) {
    const Point2d mid{0.5 * (frame.x_start.x + frame.x_end.x), 0.5 * (frame.x_start.y + frame.x_end.y)};
    const double x_label_rotation = pane.x_axis().label_rotation_deg.value_or(
        pane.parallel_xy_labels() ? label_angle_deg(frame.x_start, frame.x_end) : 0.0);
    if (pane.parallel_xy_labels()) {
      add_text(list,
               {mid.x + x_outward.x * x_label_gap, mid.y + x_outward.y * x_label_gap},
               pane.x_axis().label,
               axis_font_size,
               pane.theme_tokens().foreground(),
               TextCommand::Align::Middle,
               x_label_rotation);
    } else {
      add_text(list,
               {mid.x, mid.y + 58.0},
               pane.x_axis().label,
               axis_font_size,
               pane.theme_tokens().foreground(),
               TextCommand::Align::Middle,
               x_label_rotation);
    }
  }
  if (!pane.y_axis().label.empty()) {
    const Point2d mid{0.5 * (frame.y_start.x + frame.y_end.x), 0.5 * (frame.y_start.y + frame.y_end.y)};
    const double y_label_rotation = pane.y_axis().label_rotation_deg.value_or(
        pane.parallel_xy_labels() ? label_angle_deg(frame.y_start, frame.y_end) : 0.0);
    if (pane.parallel_xy_labels()) {
      add_text(list,
               {mid.x + y_outward.x * y_label_gap, mid.y + y_outward.y * y_label_gap},
               pane.y_axis().label,
               axis_font_size,
               pane.theme_tokens().foreground(),
               TextCommand::Align::Middle,
               y_label_rotation);
    } else {
      add_text(list,
               {mid.x - 24.0, mid.y + 58.0},
               pane.y_axis().label,
               axis_font_size,
               pane.theme_tokens().foreground(),
               TextCommand::Align::End,
               y_label_rotation);
    }
  }
  if (!pane.z_axis().label.empty()) {
    const Point2d mid{0.5 * (z_axis_start.x + z_axis_end.x), 0.5 * (z_axis_start.y + z_axis_end.y)};
    const double z_label_rotation =
        pane.z_axis().label_rotation_deg.value_or(label_angle_deg(z_axis_start, z_axis_end));
    add_text(list,
             {mid.x + z_outward.x * z_label_gap, mid.y + z_outward.y * z_label_gap},
             pane.z_axis().label,
             axis_font_size,
             pane.theme_tokens().foreground(),
             TextCommand::Align::Middle,
             z_label_rotation);
  }
  add_caption_block(list, pane, layout);
}

void render_volume_traces(DrawList& list, const Pane& pane, const PaneLayout& layout, const Bounds3d& bounds) {
  struct SurfaceFace {
    std::vector<Point2d> polygon;
    double depth{0.0};
    FillStroke style{};
  };
  struct OverlayPath {
    std::vector<Point2d> points;
    double depth{0.0};
    FillStroke style{};
    bool closed{false};
  };
  struct OverlayLabel {
    Point2d anchor{};
    std::string text{};
  };

  std::vector<SurfaceFace> faces;
  std::vector<OverlayPath> overlays;
  std::vector<OverlayLabel> labels;

  for (const Trace& trace : pane.traces()) {
    if (!trace.visible()) {
      continue;
    }
    std::visit(
        [&](const auto& payload) {
          using Payload = std::decay_t<decltype(payload)>;
          if constexpr (std::is_same_v<Payload, Path3Data>) {
            const auto xy = normalize_pairs(payload.x, payload.y);
            const auto z = to_f64(payload.z);
            const auto count = std::min({xy.first.size(), xy.second.size(), z.size()});
            std::vector<Point2d> path;
            double depth = 0.0;
            for (std::size_t i = 0; i < count; ++i) {
              const Point3d world{xy.first[i], xy.second[i], z[i]};
              path.push_back(map_projected_point(world, pane, bounds, layout.plot));
              depth += project_world(world, pane, bounds).z;
            }
            overlays.push_back(OverlayPath{
                std::move(path),
                count > 0 ? depth / static_cast<double>(count) : 0.0,
                FillStroke{trace.stroke_color(), {}, true, false, trace.stroke_width()},
                false,
            });
          } else if constexpr (std::is_same_v<Payload, Cloud3Data>) {
            const auto cloud = normalize_cloud3(payload);
            Range1d color_range{0.0, 1.0};
            if (payload.color.has_value()) {
              const Series color = flatten_f64(*payload.color);
              color_range = trace.color_limits().has_value()
                                ? *trace.color_limits()
                                : Range1d{scalar_f64(color.min()), scalar_f64(color.max())};
              if (!(color_range.high > color_range.low)) {
                color_range.low -= 0.5;
                color_range.high += 0.5;
              }
            }
            for (std::size_t i = 0; i < cloud.x.size(); ++i) {
              const Point3d world{cloud.x[i], cloud.y[i], cloud.z[i]};
              const Point2d center =
                  map_projected_point(world, pane, bounds, layout.plot);
              Rgba fill = trace.stroke_color();
              if (cloud.color.has_value() && i < cloud.color->size()) {
                fill = palette_lookup(trace.palette_ramp(), color_range, (*cloud.color)[i]);
              }
              const double radius = std::clamp(std::sqrt(std::max(cloud.size[i], 1.0)) * 0.33, 2.0, 11.0);
              overlays.push_back(OverlayPath{
                  marker_circle(center, radius),
                  project_world(world, pane, bounds).z,
                  FillStroke{multiply_rgb(fill, 0.55), fill, true, true, 0.8},
                  true,
              });
            }
          } else if constexpr (std::is_same_v<Payload, Spire3Data>) {
            const auto grid = normalize_surface_grid(payload);
            if (!grid.has_value()) {
              return;
            }
            const Range1d color_range = surface_color_range(pane, trace, *grid);
            const double hx = 0.5 * payload.width;
            const double hy = 0.5 * payload.depth;
            for (std::size_t i = 0; i < grid->z.size(); ++i) {
              const double x0 = grid->x[i] - hx;
              const double x1 = grid->x[i] + hx;
              const double y0 = grid->y[i] - hy;
              const double y1 = grid->y[i] + hy;
              const double z0 = payload.base;
              const double z1 = grid->z[i];
              const std::array<Point3d, 8> corners{{
                  {x0, y0, z0}, {x1, y0, z0}, {x1, y1, z0}, {x0, y1, z0},
                  {x0, y0, z1}, {x1, y0, z1}, {x1, y1, z1}, {x0, y1, z1},
              }};
              const std::array<std::array<int, 4>, 6> quads{{
                  {{0, 1, 2, 3}},
                  {{4, 5, 6, 7}},
                  {{0, 1, 5, 4}},
                  {{1, 2, 6, 5}},
                  {{2, 3, 7, 6}},
                  {{3, 0, 4, 7}},
              }};
              const double color_value = grid->color.has_value() ? (*grid->color)[i] : grid->z[i];
              const Rgba fill = palette_lookup(trace.palette_ramp(), color_range, color_value);
              for (const auto& quad : quads) {
                std::vector<Point2d> polygon;
                polygon.reserve(4);
                double depth = 0.0;
                std::array<Point3d, 4> face3{};
                for (std::size_t q = 0; q < quad.size(); ++q) {
                  const Point3d vertex = corners[static_cast<std::size_t>(quad[q])];
                  face3[q] = vertex;
                  const Point3d projected = project_world(vertex, pane, bounds);
                  depth += projected.z;
                  polygon.push_back(map_projected_point(vertex, pane, bounds, layout.plot));
                }
                depth /= 4.0;
                const Rgba lit = apply_surface_lighting(fill, trace, pane, face3);
                faces.push_back(SurfaceFace{
                    std::move(polygon),
                    depth,
                    surface_face_style(lit, trace),
                });
              }
            }
          } else if constexpr (std::is_same_v<Payload, IsoShellData>) {
            const auto field = normalize_volume_field(payload);
            if (!field.has_value()) {
              return;
            }
            const std::vector<double> levels = isoshell_levels(payload, *field);
            if (levels.empty()) {
              return;
            }
            Range1d color_range{levels.front(), levels.back()};
            if (!(color_range.high > color_range.low)) {
              color_range.low -= 0.5;
              color_range.high += 0.5;
            }
            for (const IsoTriangle& triangle : extract_isoshell_triangles(*field, levels)) {
              std::vector<Point2d> polygon;
              polygon.reserve(3);
              double depth = 0.0;
              bool finite = true;
              for (const Point3d vertex : triangle.vertices) {
                const Point3d projected = project_world(vertex, pane, bounds);
                const Point2d mapped = map_projected_point(vertex, pane, bounds, layout.plot);
                finite = finite && std::isfinite(mapped.x) && std::isfinite(mapped.y) && std::isfinite(projected.z);
                depth += projected.z;
                polygon.push_back(mapped);
              }
              if (!finite) {
                continue;
              }
              depth /= 3.0;
              Rgba fill = palette_lookup(trace.palette_ramp(), color_range, triangle.level);
              const std::array<Point3d, 4> tri_cell{{
                  triangle.vertices[0],
                  triangle.vertices[1],
                  triangle.vertices[2],
                  triangle.vertices[2],
              }};
              fill = apply_surface_lighting(fill, trace, pane, tri_cell);
              faces.push_back(SurfaceFace{
                  std::move(polygon),
                  depth,
                  surface_face_style(fill, trace),
              });
            }
          } else if constexpr (std::is_same_v<Payload, ContourData>) {
            if (trace.kind() != TraceKind::Ridge3) {
              return;
            }
            const auto grid = normalize_surface_grid(payload);
            if (!grid.has_value()) {
              return;
            }
            const std::vector<double> levels = contour_levels(payload, *grid);
            const Range1d color_range = contour_color_range(pane, trace, levels);
            for (const double level : levels) {
              const auto segments = contour_segments_for_level(*grid, level);
              Rgba stroke = trace.stroke_color();
              if (levels.size() > 1) {
                stroke = palette_lookup(trace.palette_ramp(), color_range, level);
              }
              double longest = -1.0;
              Point2d label_anchor{};
              for (const ContourSegment& segment : segments) {
                const Point2d start =
                    map_projected_point({segment.start.x, segment.start.y, level}, pane, bounds, layout.plot);
                const Point2d end =
                    map_projected_point({segment.end.x, segment.end.y, level}, pane, bounds, layout.plot);
                if (std::isfinite(start.x) && std::isfinite(start.y) &&
                    std::isfinite(end.x) && std::isfinite(end.y)) {
                  const double dx = end.x - start.x;
                  const double dy = end.y - start.y;
                  const double length = std::sqrt(dx * dx + dy * dy);
                  if (length > longest) {
                    longest = length;
                    label_anchor = {0.5 * (start.x + end.x), 0.5 * (start.y + end.y)};
                  }
                  const Point3d mid{
                      0.5 * (segment.start.x + segment.end.x),
                      0.5 * (segment.start.y + segment.end.y),
                      level,
                  };
                  overlays.push_back(OverlayPath{
                      {start, end},
                      project_world(mid, pane, bounds).z,
                      FillStroke{stroke, {}, true, false, trace.stroke_width()},
                      false,
                  });
                }
              }
              if (trace.annotate_levels() && longest > 28.0) {
                std::ostringstream label;
                label << std::setprecision(3) << level;
                labels.push_back(OverlayLabel{label_anchor, label.str()});
              }
            }
          } else if constexpr (std::is_same_v<Payload, SurfaceData>) {
            const auto grid = normalize_surface_grid(payload);
            if (!grid.has_value()) {
              return;
            }
            const Range1d color_range = surface_color_range(pane, trace, *grid);

            if (trace.kind() == TraceKind::Lattice) {
              std::map<std::pair<std::size_t, std::size_t>, OverlayPath> visible_edges;
              auto append_edge = [&](std::size_t a, std::size_t b, double value_a, double value_b) {
                const auto key = std::minmax(a, b);
                const Point3d pa{grid->x[a], grid->y[a], grid->z[a]};
                const Point3d pb{grid->x[b], grid->y[b], grid->z[b]};
                const double depth = 0.5 * (project_world(pa, pane, bounds).z + project_world(pb, pane, bounds).z);
                const double color_value = 0.5 * (value_a + value_b);
                const Rgba stroke = grid->color.has_value()
                                        ? palette_lookup(trace.palette_ramp(), color_range, color_value)
                                        : palette_lookup(trace.palette_ramp(), color_range, color_value);
                visible_edges[key] = OverlayPath{
                    {
                        map_projected_point(pa, pane, bounds, layout.plot),
                        map_projected_point(pb, pane, bounds, layout.plot),
                    },
                    depth,
                    FillStroke{stroke, {}, true, false, trace.stroke_width()},
                    false,
                };
              };
              for (std::int64_t row = 0; row + 1 < grid->rows; ++row) {
                for (std::int64_t col = 0; col + 1 < grid->cols; ++col) {
                  const auto i00 = static_cast<std::size_t>(row * grid->cols + col);
                  const auto i01 = static_cast<std::size_t>(row * grid->cols + (col + 1));
                  const auto i10 = static_cast<std::size_t>((row + 1) * grid->cols + col);
                  const auto i11 = static_cast<std::size_t>((row + 1) * grid->cols + (col + 1));
                  const std::array<Point3d, 4> cell{{
                      {grid->x[i00], grid->y[i00], grid->z[i00]},
                      {grid->x[i01], grid->y[i01], grid->z[i01]},
                      {grid->x[i11], grid->y[i11], grid->z[i11]},
                      {grid->x[i10], grid->y[i10], grid->z[i10]},
                  }};
                  const Point3d normal_cam =
                      rotate_world(normalize(cross(subtract(cell[1], cell[0]), subtract(cell[3], cell[0]))),
                                   pane.lens(),
                                   {0.0, 0.0, 0.0});
                  if (normal_cam.z <= 0.0) {
                    continue;
                  }
                  const auto& scalar = grid->color.has_value() ? *grid->color : grid->z;
                  append_edge(i00, i01, scalar[i00], scalar[i01]);
                  append_edge(i01, i11, scalar[i01], scalar[i11]);
                  append_edge(i11, i10, scalar[i11], scalar[i10]);
                  append_edge(i10, i00, scalar[i10], scalar[i00]);
                }
              }
              for (auto& [_, edge] : visible_edges) {
                overlays.push_back(std::move(edge));
              }
            } else {
              for (std::int64_t row = 0; row + 1 < grid->rows; ++row) {
                for (std::int64_t col = 0; col + 1 < grid->cols; ++col) {
                  const auto i00 = static_cast<std::size_t>(row * grid->cols + col);
                  const auto i01 = static_cast<std::size_t>(row * grid->cols + (col + 1));
                  const auto i10 = static_cast<std::size_t>((row + 1) * grid->cols + col);
                  const auto i11 = static_cast<std::size_t>((row + 1) * grid->cols + (col + 1));
                  const std::array<Point3d, 4> cell{{
                      {grid->x[i00], grid->y[i00], grid->z[i00]},
                      {grid->x[i01], grid->y[i01], grid->z[i01]},
                      {grid->x[i11], grid->y[i11], grid->z[i11]},
                      {grid->x[i10], grid->y[i10], grid->z[i10]},
                  }};
                  const double c00 = grid->color.has_value() ? (*grid->color)[i00] : grid->z[i00];
                  const double c01 = grid->color.has_value() ? (*grid->color)[i01] : grid->z[i01];
                  const double c11 = grid->color.has_value() ? (*grid->color)[i11] : grid->z[i11];
                  const double c10 = grid->color.has_value() ? (*grid->color)[i10] : grid->z[i10];
                  const int subdivisions = trace.shade_mode() == ShadeMode::Smooth ? 3 : 1;
                  for (int v = 0; v < subdivisions; ++v) {
                    for (int u = 0; u < subdivisions; ++u) {
                      const double u0 = static_cast<double>(u) / static_cast<double>(subdivisions);
                      const double u1 = static_cast<double>(u + 1) / static_cast<double>(subdivisions);
                      const double v0 = static_cast<double>(v) / static_cast<double>(subdivisions);
                      const double v1 = static_cast<double>(v + 1) / static_cast<double>(subdivisions);
                      const std::array<Point3d, 4> subcell{{
                          bilinear_point3d(cell, u0, v0),
                          bilinear_point3d(cell, u1, v0),
                          bilinear_point3d(cell, u1, v1),
                          bilinear_point3d(cell, u0, v1),
                      }};
                      std::vector<Point2d> polygon;
                      polygon.reserve(4);
                      double depth = 0.0;
                      for (const Point3d& vertex : subcell) {
                        const Point3d projected = project_world(vertex, pane, bounds);
                        depth += projected.z;
                        polygon.push_back(map_projected_point(vertex, pane, bounds, layout.plot));
                      }
                      depth /= 4.0;
                      const double color_value = bilinear_scalar(
                          c00,
                          c01,
                          c11,
                          c10,
                          0.5 * (u0 + u1),
                          0.5 * (v0 + v1));
                      Rgba fill = palette_lookup(trace.palette_ramp(), color_range, color_value);
                      fill = apply_surface_lighting(fill, trace, pane, subcell);
                      faces.push_back(SurfaceFace{
                          std::move(polygon),
                          depth,
                          surface_face_style(fill, trace),
                      });
                    }
                  }
                }
              }
            }
          }
        },
        trace.payload());
  }

  std::sort(
      faces.begin(),
      faces.end(),
      [](const SurfaceFace& lhs, const SurfaceFace& rhs) { return lhs.depth < rhs.depth; });
  std::sort(
      overlays.begin(),
      overlays.end(),
      [](const OverlayPath& lhs, const OverlayPath& rhs) { return lhs.depth < rhs.depth; });

  for (const SurfaceFace& face : faces) {
    add_path(list, face.polygon, face.style, true);
  }
  for (const OverlayPath& overlay : overlays) {
    add_path(list, overlay.points, overlay.style, overlay.closed);
  }
  for (const OverlayLabel& label : labels) {
    add_text(
        list,
        label.anchor,
        label.text,
        10.0,
        pane.theme_tokens().foreground(),
        TextCommand::Align::Middle);
  }
}

DrawList build_draw_list(const Studio& studio) {
  DrawList list;
  list.extent = studio.extent();
  list.background = studio.theme_tokens().background();

  add_path(
      list,
      {
          {0.0, 0.0},
          {studio.extent().width, 0.0},
          {studio.extent().width, studio.extent().height},
          {0.0, studio.extent().height},
      },
      FillStroke{studio.theme_tokens().background(), studio.theme_tokens().background(), false, true, 0.0},
      true);

  if (!studio.headline_text().empty()) {
    add_text(
        list,
        {studio.extent().width * 0.5, 32.0},
        studio.headline_text(),
        18.0,
        studio.theme_tokens().foreground(),
        TextCommand::Align::Middle);
  }

  const auto layouts = layout_panes(studio);
  std::vector<std::optional<Bounds2d>> planar_bounds(studio.pane_count());
  std::vector<std::optional<Bounds3d>> volume_bounds(studio.pane_count());
  std::vector<bool> radial_only(studio.pane_count(), false);
  std::vector<bool> wedge_only(studio.pane_count(), false);

  for (std::size_t i = 0; i < studio.pane_count(); ++i) {
    const Pane& pane = studio.panes()[i];
    if (!pane.visible()) {
      continue;
    }
    if (pane.kind() == PaneKind::Planar) {
      radial_only[i] = pane_is_radial_only(pane);
      wedge_only[i] = pane_is_wedge_only(pane);
      planar_bounds[i] = bounds_for_planar(pane);
    } else {
      volume_bounds[i] = bounds_for_volume(pane);
    }
  }
  apply_aligned_limits(studio, planar_bounds, volume_bounds);

  for (std::size_t i = 0; i < studio.pane_count(); ++i) {
    const Pane& pane = studio.panes()[i];
    if (!pane.visible()) {
      continue;
    }
    const PaneLayout& layout = layouts[i];
    const std::size_t pane_command_begin = list.commands.size();
    if (pane.kind() == PaneKind::Planar) {
      const Bounds2d bounds = *planar_bounds[i];
      render_planar_axes(list, pane, layout, bounds, radial_only[i], wedge_only[i]);
      if (!wedge_only[i]) {
        render_planar_cues(list, pane, layout, bounds, radial_only[i]);
      }
      render_planar_traces(list, pane, layout, bounds, radial_only[i], wedge_only[i]);
      render_scale_guide(list, pane, layout);
      render_legend(list, pane, layout, pane_command_begin);
    } else {
      const Bounds3d bounds = *volume_bounds[i];
      render_volume_panel(list, pane, layout);
      render_volume_grid(list, pane, layout, bounds);
      render_volume_traces(list, pane, layout, bounds);
      render_volume_cues(list, pane, layout, bounds);
      render_volume_axes(list, pane, layout, bounds);
      render_scale_guide(list, pane, layout);
      render_legend(list, pane, layout, pane_command_begin);
    }
  }

  render_shared_captions(list, studio, layouts);
  render_legend_dock(list, studio, layouts);

  return list;
}

enum class MathRunKind {
  Normal,
  Subscript,
  Superscript,
  StretchDelimiter,
};

struct MathRun {
  MathRunKind kind{MathRunKind::Normal};
  std::string text{};
};

[[nodiscard]] std::size_t utf8_char_length(unsigned char lead) noexcept {
  if ((lead & 0x80U) == 0U) return 1;
  if ((lead & 0xE0U) == 0xC0U) return 2;
  if ((lead & 0xF0U) == 0xE0U) return 3;
  if ((lead & 0xF8U) == 0xF0U) return 4;
  return 1;
}

void append_math_run(std::vector<MathRun>& runs, MathRunKind kind, std::string chunk) {
  if (chunk.empty()) {
    return;
  }
  if (!runs.empty() && runs.back().kind == kind) {
    runs.back().text += chunk;
    return;
  }
  runs.push_back(MathRun{kind, std::move(chunk)});
}

[[nodiscard]] std::vector<MathRun> split_math_runs(std::string_view text) {
  constexpr char kStretchLeftMarker = '\x1D';
  constexpr char kStretchRightMarker = '\x1E';

  bool enable_math_scripts = false;
  if (!text.empty() && text.front() == kMathTextMarker) {
    enable_math_scripts = true;
    text.remove_prefix(1);
  }
  if (!enable_math_scripts &&
      (text.find(kStretchLeftMarker) != std::string_view::npos ||
       text.find(kStretchRightMarker) != std::string_view::npos)) {
    enable_math_scripts = true;
  }
  if (!enable_math_scripts) {
    return {MathRun{MathRunKind::Normal, std::string(text)}};
  }

  std::vector<MathRun> runs;
  runs.reserve(8);
  std::size_t i = 0;
  while (i < text.size()) {
    const char marker = text[i];
    if ((marker == kStretchLeftMarker || marker == kStretchRightMarker) && (i + 1) < text.size()) {
      const std::size_t j = i + 1;
      const auto len = std::min<std::size_t>(utf8_char_length(static_cast<unsigned char>(text[j])), text.size() - j);
      append_math_run(runs, MathRunKind::StretchDelimiter, std::string(text.substr(j, len)));
      i = j + len;
      continue;
    }
    if ((marker == '_' || marker == '^') && (i + 1) < text.size()) {
      const auto kind = (marker == '_') ? MathRunKind::Subscript : MathRunKind::Superscript;
      std::size_t j = i + 1;
      if (text[j] == '{') {
        std::size_t k = j + 1;
        while (k < text.size() && text[k] != '}') {
          ++k;
        }
        if (k < text.size() && k > j + 1) {
          append_math_run(runs, kind, std::string(text.substr(j + 1, k - (j + 1))));
          i = k + 1;
          continue;
        }
      } else {
        const auto len = std::min<std::size_t>(utf8_char_length(static_cast<unsigned char>(text[j])), text.size() - j);
        append_math_run(runs, kind, std::string(text.substr(j, len)));
        i = j + len;
        continue;
      }
    }

    const auto len = std::min<std::size_t>(utf8_char_length(static_cast<unsigned char>(text[i])), text.size() - i);
    append_math_run(runs, MathRunKind::Normal, std::string(text.substr(i, len)));
    i += len;
  }
  return runs;
}

[[nodiscard]] bool is_script_run(MathRunKind kind) noexcept {
  return kind == MathRunKind::Subscript || kind == MathRunKind::Superscript;
}

[[nodiscard]] bool is_stretch_delimiter_run(MathRunKind kind) noexcept {
  return kind == MathRunKind::StretchDelimiter;
}

[[nodiscard]] double stretch_delimiter_scale_for_context(std::span<const MathRun> runs, std::size_t idx) {
  if (idx >= runs.size()) {
    return 1.24;
  }
  const std::size_t start = idx > 4 ? idx - 4 : 0;
  const std::size_t end = std::min(runs.size(), idx + 5);
  bool has_script = false;
  bool has_fraction_hint = false;
  for (std::size_t i = start; i < end; ++i) {
    if (i == idx) {
      continue;
    }
    if (is_script_run(runs[i].kind)) {
      has_script = true;
    }
    if (runs[i].kind == MathRunKind::Normal) {
      const std::string_view txt = runs[i].text;
      const auto first = txt.find_first_not_of(" \t\r\n");
      if (first != std::string_view::npos) {
        const auto last = txt.find_last_not_of(" \t\r\n");
        if (first == last && txt[first] == '/') {
          has_fraction_hint = true;
        }
      }
    }
  }
  if (has_fraction_hint) {
    return 1.56;
  }
  if (has_script) {
    return 1.40;
  }
  return 1.24;
}

enum class PdfFontKind : std::uint8_t {
  Helvetica,
  Symbol,
};

struct PdfTextChunk {
  PdfFontKind font{PdfFontKind::Helvetica};
  std::string text{};
  double size{12.0};
  double rise{0.0};
};

void append_pdf_chunk(std::vector<PdfTextChunk>& chunks,
                      PdfFontKind font,
                      std::string_view text,
                      double size,
                      double rise) {
  if (text.empty()) {
    return;
  }
  if (!chunks.empty() && chunks.back().font == font && std::abs(chunks.back().size - size) < 1e-9 &&
      std::abs(chunks.back().rise - rise) < 1e-9) {
    chunks.back().text.append(text);
    return;
  }
  chunks.push_back(PdfTextChunk{font, std::string(text), size, rise});
}

[[nodiscard]] std::string escape_pdf_literal(std::string_view text) {
  std::string out;
  out.reserve(text.size() + 8);
  for (const char c : text) {
    if (c == '\\' || c == '(' || c == ')') {
      out.push_back('\\');
    }
    out.push_back(c);
  }
  return out;
}

void append_pdf_encoded_text(std::vector<PdfTextChunk>& chunks,
                             std::string_view text,
                             double size,
                             double rise) {
  constexpr auto kCombiningMarks = std::to_array<std::string_view>({
      "\xCC\x84", // COMBINING MACRON
      "\xCC\x82", // COMBINING CIRCUMFLEX ACCENT
      "\xCC\x83", // COMBINING TILDE
      "\xCC\x87", // COMBINING DOT ABOVE
      "\xCC\x88", // COMBINING DIAERESIS
      "\xE2\x83\x97", // COMBINING RIGHT ARROW ABOVE
  });
  constexpr auto kGreekToSymbol = std::to_array<std::pair<std::string_view, char>>({
      {"\xCE\xB1", 'a'}, // α
      {"\xCE\xB2", 'b'}, // β
      {"\xCE\xB3", 'g'}, // γ
      {"\xCE\xB4", 'd'}, // δ
      {"\xCE\xB5", 'e'}, // ε
      {"\xCE\xB6", 'z'}, // ζ
      {"\xCE\xB7", 'h'}, // η
      {"\xCE\xB8", 'q'}, // θ
      {"\xCE\xBA", 'k'}, // κ
      {"\xCE\xBB", 'l'}, // λ
      {"\xCE\xBC", 'm'}, // μ
      {"\xCE\xBD", 'n'}, // ν
      {"\xCE\xBE", 'x'}, // ξ
      {"\xCF\x80", 'p'}, // π
      {"\xCF\x81", 'r'}, // ρ
      {"\xCF\x82", 's'}, // ς
      {"\xCF\x83", 's'}, // σ
      {"\xCF\x84", 't'}, // τ
      {"\xCF\x86", 'f'}, // φ
      {"\xCF\x87", 'c'}, // χ
      {"\xCF\x88", 'y'}, // ψ
      {"\xCF\x89", 'w'}, // ω
      {"\xCE\x93", 'G'}, // Γ
      {"\xCE\x94", 'D'}, // Δ
      {"\xCE\x9B", 'L'}, // Λ
      {"\xCE\xA3", 'S'}, // Σ
      {"\xCE\xA0", 'P'}, // Π
      {"\xCE\xA6", 'F'}, // Φ
      {"\xCE\xA8", 'Y'}, // Ψ
      {"\xCE\xA9", 'W'}, // Ω
  });
  constexpr auto kMathAscii = std::to_array<std::pair<std::string_view, std::string_view>>({
      {"\xE2\x89\xA4", "<="},  // ≤
      {"\xE2\x89\xA5", ">="},  // ≥
      {"\xE2\x89\xA0", "!="},  // ≠
      {"\xE2\x89\x88", "~="},  // ≈
      {"\xE2\x88\x92", "-"},   // −
      {"\xE2\x88\x9A", "sqrt"},// √
      {"\xE2\x88\x9E", "inf"}, // ∞
      {"\xE2\x88\x82", "d"},   // ∂
      {"\xE2\x88\xAB", "int"}, // ∫
      {"\xE2\x88\x91", "sum"}, // ∑
      {"\xE2\x88\x8F", "prod"},// ∏
      {"\xE2\x88\x87", "nabla"}, // ∇
      {"\xE2\x86\x90", "<-"},  // ←
      {"\xE2\x86\x92", "->"},  // →
      {"\xE2\x86\x94", "<->"}, // ↔
      {"\xE2\x88\x88", "in"},  // ∈
      {"\xE2\x88\x89", "notin"}, // ∉
      {"\xE2\x8A\x82", "subset"}, // ⊂
      {"\xE2\x8A\x86", "subseteq"}, // ⊆
      {"\xE2\x8A\x83", "supset"}, // ⊃
      {"\xE2\x8A\x87", "supseteq"}, // ⊇
      {"\xE2\x88\xAA", "cup"}, // ∪
      {"\xE2\x88\xA9", "cap"}, // ∩
      {"\xE2\x88\x80", "forall"}, // ∀
      {"\xE2\x88\x83", "exists"}, // ∃
      {"\xE2\x88\x9D", "prop"}, // ∝
      {"\xE2\x8B\x85", "*"},   // ⋅
      {"\xE2\x8B\xAF", "..."}, // ⋯
      {"\xE2\x88\x99", "*"},   // ∙
      {"\xC3\x97", "x"},       // ×
      {"\xC2\xB1", "+/-"},     // ±
  });

  for (std::size_t i = 0; i < text.size();) {
    bool matched = false;
    for (const auto& mark : kCombiningMarks) {
      if (i + mark.size() <= text.size() && text.compare(i, mark.size(), mark) == 0) {
        i += mark.size();
        matched = true;
        break;
      }
    }
    if (matched) {
      continue;
    }
    for (const auto& [utf8, symbol] : kGreekToSymbol) {
      if (i + utf8.size() <= text.size() && text.compare(i, utf8.size(), utf8) == 0) {
        const std::string sym(1, symbol);
        append_pdf_chunk(chunks, PdfFontKind::Symbol, sym, size, rise);
        i += utf8.size();
        matched = true;
        break;
      }
    }
    if (matched) {
      continue;
    }
    for (const auto& [utf8, ascii] : kMathAscii) {
      if (i + utf8.size() <= text.size() && text.compare(i, utf8.size(), utf8) == 0) {
        append_pdf_chunk(chunks, PdfFontKind::Helvetica, ascii, size, rise);
        i += utf8.size();
        matched = true;
        break;
      }
    }
    if (matched) {
      continue;
    }

    const unsigned char lead = static_cast<unsigned char>(text[i]);
    const auto len = std::min<std::size_t>(utf8_char_length(lead), text.size() - i);
    if (lead < 0x80U) {
      append_pdf_chunk(chunks, PdfFontKind::Helvetica, text.substr(i, len), size, rise);
    } else {
      append_pdf_chunk(chunks, PdfFontKind::Helvetica, "?", size, rise);
    }
    i += len;
  }
}

[[nodiscard]] std::vector<PdfTextChunk> build_pdf_text_chunks(std::string_view text, double base_size) {
  std::vector<PdfTextChunk> chunks;
  const auto runs = split_math_runs(text);
  for (std::size_t idx = 0; idx < runs.size(); ++idx) {
    const auto& run = runs[idx];
    const bool scripted = is_script_run(run.kind);
    const bool stretch_delim = is_stretch_delimiter_run(run.kind);
    const double stretch_scale = stretch_delimiter_scale_for_context(runs, idx);
    const double run_size = scripted ? (base_size * 0.72) : (stretch_delim ? (base_size * stretch_scale) : base_size);
    double rise = 0.0;
    if (run.kind == MathRunKind::Superscript) {
      rise = base_size * 0.33;
    } else if (run.kind == MathRunKind::Subscript) {
      rise = -base_size * 0.20;
    } else if (stretch_delim) {
      rise = -(run_size - base_size) * 0.36;
    }
    append_pdf_encoded_text(chunks, run.text, run_size, rise);
  }
  return chunks;
}

[[nodiscard]] constexpr std::array<double, 128> make_helvetica_widths() {
  std::array<double, 128> widths{};
  widths.fill(556.0);
  widths[' '] = 278.0;
  widths['\t'] = 556.0;
  widths['0'] = 556.0;
  widths['1'] = 556.0;
  widths['2'] = 556.0;
  widths['3'] = 556.0;
  widths['4'] = 556.0;
  widths['5'] = 556.0;
  widths['6'] = 556.0;
  widths['7'] = 556.0;
  widths['8'] = 556.0;
  widths['9'] = 556.0;
  widths['!'] = 278.0;
  widths['"'] = 355.0;
  widths['#'] = 556.0;
  widths['$'] = 556.0;
  widths['%'] = 889.0;
  widths['&'] = 667.0;
  widths['\''] = 191.0;
  widths['('] = 333.0;
  widths[')'] = 333.0;
  widths['*'] = 389.0;
  widths['+'] = 584.0;
  widths[','] = 278.0;
  widths['-'] = 333.0;
  widths['.'] = 278.0;
  widths['/'] = 278.0;
  widths[':'] = 278.0;
  widths[';'] = 278.0;
  widths['<'] = 584.0;
  widths['='] = 584.0;
  widths['>'] = 584.0;
  widths['?'] = 556.0;
  widths['@'] = 1015.0;
  widths['A'] = 667.0;
  widths['B'] = 667.0;
  widths['C'] = 722.0;
  widths['D'] = 722.0;
  widths['E'] = 667.0;
  widths['F'] = 611.0;
  widths['G'] = 778.0;
  widths['H'] = 722.0;
  widths['I'] = 278.0;
  widths['J'] = 500.0;
  widths['K'] = 667.0;
  widths['L'] = 556.0;
  widths['M'] = 833.0;
  widths['N'] = 722.0;
  widths['O'] = 778.0;
  widths['P'] = 667.0;
  widths['Q'] = 778.0;
  widths['R'] = 722.0;
  widths['S'] = 667.0;
  widths['T'] = 611.0;
  widths['U'] = 722.0;
  widths['V'] = 667.0;
  widths['W'] = 944.0;
  widths['X'] = 667.0;
  widths['Y'] = 667.0;
  widths['Z'] = 611.0;
  widths['['] = 278.0;
  widths['\\'] = 278.0;
  widths[']'] = 278.0;
  widths['^'] = 469.0;
  widths['_'] = 556.0;
  widths['`'] = 333.0;
  widths['a'] = 556.0;
  widths['b'] = 556.0;
  widths['c'] = 500.0;
  widths['d'] = 556.0;
  widths['e'] = 556.0;
  widths['f'] = 278.0;
  widths['g'] = 556.0;
  widths['h'] = 556.0;
  widths['i'] = 222.0;
  widths['j'] = 222.0;
  widths['k'] = 500.0;
  widths['l'] = 222.0;
  widths['m'] = 833.0;
  widths['n'] = 556.0;
  widths['o'] = 556.0;
  widths['p'] = 556.0;
  widths['q'] = 556.0;
  widths['r'] = 333.0;
  widths['s'] = 500.0;
  widths['t'] = 278.0;
  widths['u'] = 556.0;
  widths['v'] = 500.0;
  widths['w'] = 722.0;
  widths['x'] = 500.0;
  widths['y'] = 500.0;
  widths['z'] = 500.0;
  widths['{'] = 334.0;
  widths['|'] = 260.0;
  widths['}'] = 334.0;
  widths['~'] = 584.0;
  return widths;
}

[[nodiscard]] constexpr std::array<double, 128> make_symbol_widths() {
  std::array<double, 128> widths{};
  widths.fill(600.0);
  widths[' '] = 250.0;
  widths['!'] = 333.0;
  widths['"'] = 713.0;
  widths['#'] = 500.0;
  widths['$'] = 549.0;
  widths['%'] = 833.0;
  widths['&'] = 778.0;
  widths['('] = 333.0;
  widths[')'] = 333.0;
  widths['+'] = 549.0;
  widths[','] = 250.0;
  widths['-'] = 549.0;
  widths['.'] = 250.0;
  widths['/'] = 278.0;
  widths['0'] = 500.0;
  widths['1'] = 500.0;
  widths['2'] = 500.0;
  widths['3'] = 500.0;
  widths['4'] = 500.0;
  widths['5'] = 500.0;
  widths['6'] = 500.0;
  widths['7'] = 500.0;
  widths['8'] = 500.0;
  widths['9'] = 500.0;
  widths[':'] = 278.0;
  widths[';'] = 278.0;
  widths['<'] = 549.0;
  widths['='] = 549.0;
  widths['>'] = 549.0;
  widths['?'] = 444.0;
  widths['@'] = 549.0;
  widths['['] = 333.0;
  widths['\\'] = 278.0;
  widths[']'] = 333.0;
  widths['^'] = 500.0;
  widths['_'] = 500.0;
  widths['`'] = 333.0;
  widths['{'] = 480.0;
  widths['|'] = 200.0;
  widths['}'] = 480.0;
  widths['~'] = 549.0;
  return widths;
}

[[nodiscard]] double pdf_glyph_width_units(PdfFontKind font, unsigned char c) {
  static constexpr auto kHelvetica = make_helvetica_widths();
  static constexpr auto kSymbol = make_symbol_widths();
  const auto& table = (font == PdfFontKind::Symbol) ? kSymbol : kHelvetica;
  if (c < table.size()) {
    return table[c];
  }
  return table['?'];
}

[[nodiscard]] double pdf_chunk_advance(const PdfTextChunk& chunk) {
  const double units = std::transform_reduce(
      chunk.text.begin(),
      chunk.text.end(),
      0.0,
      std::plus<>{},
      [&](char c) { return pdf_glyph_width_units(chunk.font, static_cast<unsigned char>(c)); });
  return chunk.size * units / 1000.0;
}

struct PdfPlacedChunk {
  PdfFontKind font{PdfFontKind::Helvetica};
  std::string text{};
  double size{12.0};
  double rise{0.0};
  double x_offset{0.0};
};

struct PdfFractionBar {
  double x0{0.0};
  double x1{0.0};
  double y_offset{0.0};
  double stroke_width{0.8};
};

struct PdfTextLayout {
  std::vector<PdfPlacedChunk> chunks{};
  std::vector<PdfFractionBar> bars{};
  double advance{0.0};
};

void append_placed_chunks(std::vector<PdfPlacedChunk>& placed,
                          const std::vector<PdfTextChunk>& chunks,
                          double x_start) {
  double cursor = x_start;
  for (const auto& chunk : chunks) {
    const double advance = pdf_chunk_advance(chunk);
    if (!chunk.text.empty()) {
      placed.push_back(PdfPlacedChunk{chunk.font, chunk.text, chunk.size, chunk.rise, cursor});
    }
    cursor += advance;
  }
}

[[nodiscard]] double chunks_advance(std::span<const PdfTextChunk> chunks) {
  double width = 0.0;
  for (const auto& chunk : chunks) {
    width += pdf_chunk_advance(chunk);
  }
  return width;
}

[[nodiscard]] bool is_fraction_separator_run(std::string_view text) {
  const auto first = text.find_first_not_of(" \t\r\n");
  if (first == std::string_view::npos) {
    return false;
  }
  const auto last = text.find_last_not_of(" \t\r\n");
  return first == last && text[first] == '/';
}

[[nodiscard]] PdfTextLayout layout_pdf_text(std::string_view text, double base_size, bool stack_fractions) {
  struct FractionTuning {
    double script_scale{0.72};
    double numerator_rise_em{0.60};
    double denominator_rise_em{-0.54};
    double inner_side_padding_em{0.08};
    double min_width_em{0.74};
    double bar_offset_em{0.03};
    double bar_width_em{0.055};
    double trailing_gap_em{0.10};
  };
  static constexpr FractionTuning kFractionTuning{};

  PdfTextLayout layout;
  layout.chunks.reserve(16);
  layout.bars.reserve(4);

  const auto runs = split_math_runs(text);
  std::size_t i = 0;
  double cursor = 0.0;
  while (i < runs.size()) {
    const bool has_fraction_triplet = stack_fractions && i + 2 < runs.size() &&
                                      runs[i].kind == MathRunKind::Superscript &&
                                      runs[i + 1].kind == MathRunKind::Normal &&
                                      is_fraction_separator_run(runs[i + 1].text) &&
                                      runs[i + 2].kind == MathRunKind::Subscript;
    if (has_fraction_triplet) {
      std::vector<PdfTextChunk> numer_chunks;
      std::vector<PdfTextChunk> denom_chunks;
      const double frac_size = base_size * kFractionTuning.script_scale;
      append_pdf_encoded_text(
          numer_chunks, runs[i].text, frac_size, base_size * kFractionTuning.numerator_rise_em);
      append_pdf_encoded_text(
          denom_chunks, runs[i + 2].text, frac_size, base_size * kFractionTuning.denominator_rise_em);
      const double numer_width = chunks_advance(numer_chunks);
      const double denom_width = chunks_advance(denom_chunks);
      const double inner_padding = base_size * kFractionTuning.inner_side_padding_em;
      const double frac_width =
          std::max({numer_width, denom_width, base_size * kFractionTuning.min_width_em}) + 2.0 * inner_padding;
      const double num_start = cursor + (frac_width - numer_width) * 0.5;
      const double den_start = cursor + (frac_width - denom_width) * 0.5;
      append_placed_chunks(layout.chunks, numer_chunks, num_start);
      append_placed_chunks(layout.chunks, denom_chunks, den_start);
      layout.bars.push_back(PdfFractionBar{
          cursor,
          cursor + frac_width,
          base_size * kFractionTuning.bar_offset_em,
          std::max(0.6, base_size * kFractionTuning.bar_width_em),
      });
      cursor += frac_width + base_size * kFractionTuning.trailing_gap_em;
      i += 3;
      continue;
    }

    const auto& run = runs[i];
    const bool scripted = is_script_run(run.kind);
    const bool stretch_delim = is_stretch_delimiter_run(run.kind);
    const double stretch_scale = stretch_delimiter_scale_for_context(runs, i);
    const double run_size = scripted ? (base_size * 0.72) : (stretch_delim ? (base_size * stretch_scale) : base_size);
    double rise = 0.0;
    if (run.kind == MathRunKind::Superscript) {
      rise = base_size * 0.33;
    } else if (run.kind == MathRunKind::Subscript) {
      rise = -base_size * 0.20;
    } else if (stretch_delim) {
      rise = -(run_size - base_size) * 0.36;
    }
    std::vector<PdfTextChunk> encoded;
    append_pdf_encoded_text(encoded, run.text, run_size, rise);
    append_placed_chunks(layout.chunks, encoded, cursor);
    cursor += chunks_advance(encoded);
    ++i;
  }

  layout.advance = cursor;
  return layout;
}

[[nodiscard]] std::string emit_svg(const DrawList& list) {
  auto svg_text_advance = [](std::string_view text, double size) -> double {
    double units = 0.0;
    for (std::size_t i = 0; i < text.size();) {
      const unsigned char lead = static_cast<unsigned char>(text[i]);
      const auto len = std::min<std::size_t>(utf8_char_length(lead), text.size() - i);
      if (lead < 0x80U) {
        units += pdf_glyph_width_units(PdfFontKind::Helvetica, lead);
      } else {
        units += 600.0;
      }
      i += len;
    }
    return size * units / 1000.0;
  };

  std::ostringstream out;
  out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  out << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << format_number(list.extent.width)
      << "\" height=\"" << format_number(list.extent.height) << "\" viewBox=\"0 0 "
      << format_number(list.extent.width) << " " << format_number(list.extent.height) << "\">\n";
  out << "<rect x=\"0\" y=\"0\" width=\"" << format_number(list.extent.width) << "\" height=\""
      << format_number(list.extent.height) << "\" fill=\"" << svg_rgb(list.background) << "\" fill-opacity=\""
      << format_number(list.background.alpha) << "\"/>\n";
  for (const DrawCommand& command : list.commands) {
    std::visit(
        [&](const auto& item) {
          using Item = std::decay_t<decltype(item)>;
          if constexpr (std::is_same_v<Item, PathCommand>) {
            out << "<path d=\"";
            if (!item.points.empty()) {
              out << "M " << format_number(item.points.front().x) << " " << format_number(item.points.front().y) << " ";
              for (std::size_t i = 1; i < item.points.size(); ++i) {
                out << "L " << format_number(item.points[i].x) << " " << format_number(item.points[i].y) << " ";
              }
              if (item.closed) {
                out << "Z";
              }
            }
            out << "\" ";
            out << "stroke=\""
                << (item.style.has_stroke ? svg_rgb(item.style.stroke) : std::string("none")) << "\" ";
            if (item.style.has_stroke) {
              out << "stroke-opacity=\"" << format_number(item.style.stroke.alpha) << "\" ";
              out << "stroke-width=\"" << format_number(item.style.stroke_width) << "\" ";
            }
            out << "fill=\""
                << (item.style.has_fill ? svg_rgb(item.style.fill) : std::string("none")) << "\" ";
            if (item.style.has_fill) {
              out << "fill-opacity=\"" << format_number(item.style.fill.alpha) << "\" ";
            }
            out << "stroke-linejoin=\"round\" stroke-linecap=\"round\"/>\n";
          } else if constexpr (std::is_same_v<Item, TextCommand>) {
            std::string anchor = "start";
            if (item.align == TextCommand::Align::Middle) {
              anchor = "middle";
            } else if (item.align == TextCommand::Align::End) {
              anchor = "end";
            }
            const auto runs = split_math_runs(item.text);
            const bool rich = std::any_of(
                runs.begin(), runs.end(), [](const MathRun& run) { return run.kind != MathRunKind::Normal; });

            constexpr bool kEnableStackedSvgFractions = false;
            if (!rich || std::abs(item.rotation_deg) > 1e-9 || !kEnableStackedSvgFractions) {
              out << "<text x=\"" << format_number(item.anchor.x) << "\" y=\"" << format_number(item.anchor.y)
                  << "\" font-family=\"Helvetica, Arial, sans-serif\" font-size=\"" << format_number(item.size)
                  << "\" fill=\"" << svg_rgb(item.color) << "\" fill-opacity=\"" << format_number(item.color.alpha)
                  << "\" text-anchor=\"" << anchor << "\"";
              if (std::abs(item.rotation_deg) > 1e-9) {
                out << " transform=\"rotate(" << format_number(item.rotation_deg) << " "
                    << format_number(item.anchor.x) << " " << format_number(item.anchor.y) << ")\"";
              }
              out << ">";
              if (!rich) {
                out << escape_xml(item.text);
              } else {
                for (std::size_t idx = 0; idx < runs.size(); ++idx) {
                  const auto& run = runs[idx];
                  if (run.kind == MathRunKind::Normal) {
                    out << escape_xml(run.text);
                  } else if (run.kind == MathRunKind::Subscript) {
                    out << "<tspan baseline-shift=\"sub\" font-size=\"72%\">" << escape_xml(run.text) << "</tspan>";
                  } else if (run.kind == MathRunKind::StretchDelimiter) {
                    const double scale = stretch_delimiter_scale_for_context(runs, idx);
                    const double dy_em = -(scale - 1.0) * 0.36;
                    out << "<tspan font-size=\"" << format_number(scale * 100.0) << "%\" dy=\""
                        << format_number(dy_em) << "em\">" << escape_xml(run.text) << "</tspan>";
                  } else {
                    out << "<tspan baseline-shift=\"super\" font-size=\"72%\">" << escape_xml(run.text) << "</tspan>";
                  }
                }
              }
              out << "</text>\n";
            } else {
              struct SvgPlacedText {
                std::string text{};
                double size{12.0};
                double rise{0.0};
                double x_offset{0.0};
              };
              struct SvgFractionBar {
                double x0{0.0};
                double x1{0.0};
                double y_offset{0.0};
                double stroke_width{0.8};
              };
              std::vector<SvgPlacedText> placed{};
              std::vector<SvgFractionBar> bars{};
              placed.reserve(24);
              bars.reserve(4);

              struct FractionTuning {
                double script_scale{0.72};
                double numerator_rise_em{0.60};
                double denominator_rise_em{-0.54};
                double inner_side_padding_em{0.08};
                double min_width_em{0.74};
                double bar_offset_em{0.03};
                double bar_width_em{0.055};
                double trailing_gap_em{0.10};
              };
              static constexpr FractionTuning kFractionTuning{};

              double cursor = 0.0;
              std::size_t i = 0;
              while (i < runs.size()) {
                const bool has_fraction_triplet = i + 2 < runs.size() &&
                                                  runs[i].kind == MathRunKind::Superscript &&
                                                  runs[i + 1].kind == MathRunKind::Normal &&
                                                  is_fraction_separator_run(runs[i + 1].text) &&
                                                  runs[i + 2].kind == MathRunKind::Subscript;
                if (has_fraction_triplet) {
                  const double frac_size = item.size * kFractionTuning.script_scale;
                  const double num_w = svg_text_advance(runs[i].text, frac_size);
                  const double den_w = svg_text_advance(runs[i + 2].text, frac_size);
                  const double inner_padding = item.size * kFractionTuning.inner_side_padding_em;
                  const double frac_w =
                      std::max({num_w, den_w, item.size * kFractionTuning.min_width_em}) + 2.0 * inner_padding;
                  const double num_start = cursor + (frac_w - num_w) * 0.5;
                  const double den_start = cursor + (frac_w - den_w) * 0.5;
                  if (!runs[i].text.empty()) {
                    placed.push_back(SvgPlacedText{
                        std::string(runs[i].text),
                        frac_size,
                        item.size * kFractionTuning.numerator_rise_em,
                        num_start,
                    });
                  }
                  if (!runs[i + 2].text.empty()) {
                    placed.push_back(SvgPlacedText{
                        std::string(runs[i + 2].text),
                        frac_size,
                        item.size * kFractionTuning.denominator_rise_em,
                        den_start,
                    });
                  }
                  bars.push_back(SvgFractionBar{
                      cursor,
                      cursor + frac_w,
                      item.size * kFractionTuning.bar_offset_em,
                      std::max(0.6, item.size * kFractionTuning.bar_width_em),
                  });
                  cursor += frac_w + item.size * kFractionTuning.trailing_gap_em;
                  i += 3;
                  continue;
                }

                const auto& run = runs[i];
                const bool scripted = is_script_run(run.kind);
                const bool stretch_delim = is_stretch_delimiter_run(run.kind);
                const double stretch_scale = stretch_delimiter_scale_for_context(runs, i);
                const double run_size =
                    scripted ? (item.size * 0.72) : (stretch_delim ? (item.size * stretch_scale) : item.size);
                double rise = 0.0;
                if (run.kind == MathRunKind::Superscript) {
                  rise = item.size * 0.33;
                } else if (run.kind == MathRunKind::Subscript) {
                  rise = -item.size * 0.20;
                } else if (stretch_delim) {
                  rise = -(run_size - item.size) * 0.36;
                }
                if (!run.text.empty()) {
                  placed.push_back(SvgPlacedText{std::string(run.text), run_size, rise, cursor});
                }
                cursor += svg_text_advance(run.text, run_size);
                ++i;
              }

              double dx = 0.0;
              if (item.align == TextCommand::Align::Middle) {
                dx = -cursor * 0.5;
              } else if (item.align == TextCommand::Align::End) {
                dx = -cursor;
              }

              for (const auto& chunk : placed) {
                out << "<text x=\"" << format_number(item.anchor.x + dx + chunk.x_offset) << "\" y=\""
                    << format_number(item.anchor.y + chunk.rise)
                    << "\" font-family=\"Helvetica, Arial, sans-serif\" font-size=\""
                    << format_number(chunk.size)
                    << "\" fill=\"" << svg_rgb(item.color) << "\" fill-opacity=\""
                    << format_number(item.color.alpha) << "\" text-anchor=\"start\">";
                out << escape_xml(chunk.text);
                out << "</text>\n";
              }
              for (const auto& bar : bars) {
                out << "<line x1=\"" << format_number(item.anchor.x + dx + bar.x0)
                    << "\" y1=\"" << format_number(item.anchor.y + bar.y_offset)
                    << "\" x2=\"" << format_number(item.anchor.x + dx + bar.x1)
                    << "\" y2=\"" << format_number(item.anchor.y + bar.y_offset)
                    << "\" stroke=\"" << svg_rgb(item.color)
                    << "\" stroke-opacity=\"" << format_number(item.color.alpha)
                    << "\" stroke-width=\"" << format_number(bar.stroke_width) << "\"/>\n";
              }
            }
          }
        },
        command);
  }
  out << "</svg>\n";
  return out.str();
}

[[nodiscard]] std::string emit_pdf(const DrawList& list) {
  struct AlphaKey {
    int stroke_milli{1000};
    int fill_milli{1000};
    auto operator<=>(const AlphaKey&) const = default;
  };
  auto quant_alpha = [](double alpha) -> int {
    return static_cast<int>(std::lround(std::clamp(alpha, 0.0, 1.0) * 1000.0));
  };
  auto dequant_alpha = [](int milli) -> double {
    return static_cast<double>(milli) / 1000.0;
  };
  auto make_alpha_key = [&](double stroke_alpha, double fill_alpha) -> AlphaKey {
    return AlphaKey{quant_alpha(stroke_alpha), quant_alpha(fill_alpha)};
  };

  std::map<AlphaKey, std::string> gs_names;
  auto register_alpha = [&](AlphaKey key) {
    if (gs_names.contains(key)) {
      return;
    }
    gs_names[key] = "GS" + std::to_string(gs_names.size() + 1);
  };
  register_alpha(make_alpha_key(1.0, 1.0));
  for (const DrawCommand& command : list.commands) {
    std::visit(
        [&](const auto& item) {
          using Item = std::decay_t<decltype(item)>;
          if constexpr (std::is_same_v<Item, PathCommand>) {
            const double stroke_alpha = item.style.has_stroke ? item.style.stroke.alpha : 1.0;
            const double fill_alpha = item.style.has_fill ? item.style.fill.alpha : 1.0;
            register_alpha(make_alpha_key(stroke_alpha, fill_alpha));
          } else if constexpr (std::is_same_v<Item, TextCommand>) {
            register_alpha(make_alpha_key(item.color.alpha, item.color.alpha));
          }
        },
        command);
  }

  std::ostringstream stream;
  stream << "1 J 1 j\n";
  std::optional<AlphaKey> active_alpha;
  auto apply_alpha = [&](AlphaKey key) {
    if (active_alpha.has_value() && *active_alpha == key) {
      return;
    }
    stream << "/" << gs_names[key] << " gs\n";
    active_alpha = key;
  };
  for (const DrawCommand& command : list.commands) {
    std::visit(
        [&](const auto& item) {
          using Item = std::decay_t<decltype(item)>;
          if constexpr (std::is_same_v<Item, PathCommand>) {
            if (item.points.empty()) {
              return;
            }
            const double stroke_alpha = item.style.has_stroke ? item.style.stroke.alpha : 1.0;
            const double fill_alpha = item.style.has_fill ? item.style.fill.alpha : 1.0;
            apply_alpha(make_alpha_key(stroke_alpha, fill_alpha));
            stream << format_number(item.style.stroke_width) << " w\n";
            if (item.style.has_stroke) {
              stream << format_number(item.style.stroke.red) << " "
                     << format_number(item.style.stroke.green) << " "
                     << format_number(item.style.stroke.blue) << " RG\n";
            }
            if (item.style.has_fill) {
              stream << format_number(item.style.fill.red) << " "
                     << format_number(item.style.fill.green) << " "
                     << format_number(item.style.fill.blue) << " rg\n";
            }
            const auto pdf_y = [&](double y) { return list.extent.height - y; };
            stream << format_number(item.points.front().x) << " " << format_number(pdf_y(item.points.front().y)) << " m\n";
            for (std::size_t i = 1; i < item.points.size(); ++i) {
              stream << format_number(item.points[i].x) << " " << format_number(pdf_y(item.points[i].y)) << " l\n";
            }
            if (item.closed) {
              stream << "h\n";
            }
            if (item.style.has_fill && item.style.has_stroke) {
              stream << "B\n";
            } else if (item.style.has_fill) {
              stream << "f\n";
            } else if (item.style.has_stroke) {
              stream << "S\n";
            }
          } else if constexpr (std::is_same_v<Item, TextCommand>) {
            apply_alpha(make_alpha_key(item.color.alpha, item.color.alpha));
            const bool rotated = std::abs(item.rotation_deg) > 1e-9;
            double x = item.anchor.x;
            constexpr bool kEnableStackedPdfFractions = false;
            const auto layout = layout_pdf_text(item.text, item.size, !rotated && kEnableStackedPdfFractions);
            const double rough_width = layout.advance;
            double dx = 0.0;
            if (item.align == TextCommand::Align::Middle) {
              dx = -rough_width * 0.5;
            } else if (item.align == TextCommand::Align::End) {
              dx = -rough_width;
            }
            const double y = list.extent.height - item.anchor.y;

            if (rotated) {
              const auto chunks = build_pdf_text_chunks(item.text, item.size);
              stream << "BT\n";
              stream << format_number(item.color.red) << " " << format_number(item.color.green) << " "
                     << format_number(item.color.blue) << " rg\n";
              const double theta = -item.rotation_deg * kPi / 180.0;
              const double c = std::cos(theta);
              const double s = std::sin(theta);
              stream << format_number(c) << " " << format_number(s) << " "
                     << format_number(-s) << " " << format_number(c) << " "
                     << format_number(x) << " " << format_number(y) << " Tm\n";
              stream << format_number(dx) << " 0 Td\n";
              PdfFontKind active_font = PdfFontKind::Helvetica;
              double active_size = -1.0;
              double active_rise = 1e9;
              for (const auto& chunk : chunks) {
                if (chunk.text.empty()) {
                  continue;
                }
                if (chunk.font != active_font || std::abs(chunk.size - active_size) > 1e-9) {
                  stream << "/" << (chunk.font == PdfFontKind::Symbol ? "F2 " : "F1 ")
                         << format_number(chunk.size) << " Tf\n";
                  active_font = chunk.font;
                  active_size = chunk.size;
                }
                if (std::abs(chunk.rise - active_rise) > 1e-9) {
                  stream << format_number(chunk.rise) << " Ts\n";
                  active_rise = chunk.rise;
                }
                stream << "(" << escape_pdf_literal(chunk.text) << ") Tj\n";
              }
              if (std::abs(active_rise) > 1e-9) {
                stream << "0 Ts\n";
              }
              stream << "ET\n";
            } else {
              if (layout.bars.empty()) {
                const auto chunks = build_pdf_text_chunks(item.text, item.size);
                stream << "BT\n";
                stream << format_number(item.color.red) << " " << format_number(item.color.green) << " "
                       << format_number(item.color.blue) << " rg\n";
                stream << "1 0 0 1 " << format_number(x + dx) << " " << format_number(y) << " Tm\n";
                PdfFontKind active_font = PdfFontKind::Helvetica;
                double active_size = -1.0;
                double active_rise = 1e9;
                for (const auto& chunk : chunks) {
                  if (chunk.text.empty()) {
                    continue;
                  }
                  if (chunk.font != active_font || std::abs(chunk.size - active_size) > 1e-9) {
                    stream << "/" << (chunk.font == PdfFontKind::Symbol ? "F2 " : "F1 ")
                           << format_number(chunk.size) << " Tf\n";
                    active_font = chunk.font;
                    active_size = chunk.size;
                  }
                  if (std::abs(chunk.rise - active_rise) > 1e-9) {
                    stream << format_number(chunk.rise) << " Ts\n";
                    active_rise = chunk.rise;
                  }
                  stream << "(" << escape_pdf_literal(chunk.text) << ") Tj\n";
                }
                if (std::abs(active_rise) > 1e-9) {
                  stream << "0 Ts\n";
                }
                stream << "ET\n";
              } else {
                for (const auto& chunk : layout.chunks) {
                  if (chunk.text.empty()) {
                    continue;
                  }
                  stream << "BT\n";
                  stream << "/" << (chunk.font == PdfFontKind::Symbol ? "F2 " : "F1 ")
                         << format_number(chunk.size) << " Tf\n";
                  stream << format_number(item.color.red) << " " << format_number(item.color.green) << " "
                         << format_number(item.color.blue) << " rg\n";
                  stream << format_number(x + dx + chunk.x_offset) << " "
                         << format_number(y + chunk.rise) << " Td\n";
                  stream << "(" << escape_pdf_literal(chunk.text) << ") Tj\n";
                  stream << "ET\n";
                }
                stream << format_number(item.color.red) << " " << format_number(item.color.green) << " "
                       << format_number(item.color.blue) << " RG\n";
                for (const auto& bar : layout.bars) {
                  stream << format_number(bar.stroke_width) << " w\n";
                  stream << format_number(x + dx + bar.x0) << " " << format_number(y + bar.y_offset) << " m\n";
                  stream << format_number(x + dx + bar.x1) << " " << format_number(y + bar.y_offset) << " l\n";
                  stream << "S\n";
                }
              }
            }
          }
        },
        command);
  }

  const std::string content = stream.str();
  std::vector<std::string> objects;
  objects.emplace_back("<< /Type /Catalog /Pages 2 0 R >>");
  objects.emplace_back("<< /Type /Pages /Kids [3 0 R] /Count 1 >>");
  objects.emplace_back("");
  {
    std::ostringstream contents;
    contents << "<< /Length " << content.size() << " >>\nstream\n" << content << "endstream";
    objects.push_back(contents.str());
  }
  objects.emplace_back("<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>");
  objects.emplace_back("<< /Type /Font /Subtype /Type1 /BaseFont /Symbol >>");

  std::vector<std::pair<AlphaKey, int>> gs_objects;
  int next_obj_id = 7;
  for (const auto& [key, _] : gs_names) {
    gs_objects.emplace_back(key, next_obj_id++);
    std::ostringstream gs;
    gs << "<< /Type /ExtGState /CA " << format_number(dequant_alpha(key.stroke_milli))
       << " /ca " << format_number(dequant_alpha(key.fill_milli)) << " >>";
    objects.push_back(gs.str());
  }

  std::ostringstream extgstate_dict;
  for (const auto& [key, obj_id] : gs_objects) {
    extgstate_dict << "/" << gs_names[key] << " " << obj_id << " 0 R ";
  }
  std::ostringstream page;
  page << "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 " << format_number(list.extent.width) << " "
       << format_number(list.extent.height)
       << "] /Resources << /Font << /F1 5 0 R /F2 6 0 R >> /ExtGState << " << extgstate_dict.str()
       << ">> >> /Contents 4 0 R >>";
  objects[2] = page.str();

  std::ostringstream pdf;
  pdf << "%PDF-1.4\n";
  pdf << "%\xC7\xEC\x8F\xA2\n";
  std::vector<long> offsets;
  offsets.push_back(0);
  for (std::size_t i = 0; i < objects.size(); ++i) {
    offsets.push_back(static_cast<long>(pdf.tellp()));
    pdf << (i + 1) << " 0 obj\n" << objects[i] << "\nendobj\n";
  }
  const long xref_offset = static_cast<long>(pdf.tellp());
  pdf << "xref\n0 " << (objects.size() + 1) << "\n";
  pdf << "0000000000 65535 f \n";
  for (std::size_t i = 1; i < offsets.size(); ++i) {
    pdf << std::setw(10) << std::setfill('0') << offsets[i] << " 00000 n \n";
  }
  pdf << "trailer\n<< /Size " << (objects.size() + 1) << " /Root 1 0 R >>\n";
  pdf << "startxref\n" << xref_offset << "\n%%EOF\n";
  return pdf.str();
}

void write_bytes(std::string_view path, const std::string& bytes) {
  const std::filesystem::path out_path(path);
  if (out_path.has_parent_path()) {
    std::filesystem::create_directories(out_path.parent_path());
  }
  std::ofstream out(out_path, std::ios::binary);
  if (!out) {
    throw std::runtime_error("PlussPl export failed: unable to open output file");
  }
  out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  if (!out) {
    throw std::runtime_error("PlussPl export failed: unable to write output file");
  }
}

} // namespace

namespace plotism::render {

std::string render_svg(const Studio& studio) {
  return emit_svg(build_draw_list(studio));
}

std::string render_pdf(const Studio& studio) {
  return emit_pdf(build_draw_list(studio));
}

void save_svg(const Studio& studio, std::string_view path) {
  write_bytes(path, render_svg(studio));
}

void save_pdf(const Studio& studio, std::string_view path) {
  write_bytes(path, render_pdf(studio));
}

} // namespace plotism::render

} // namespace pluss::pl
