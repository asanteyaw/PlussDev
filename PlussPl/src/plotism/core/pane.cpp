#include "pluss/pl/plusspl.h"

#include <cmath>

namespace pluss::pl {
namespace {

[[nodiscard]] bool series_is_vector(const Series& series) noexcept {
  return series.dim() == 1 || (series.dim() == 2 &&
                               (series.sizes()[0] == 1 || series.sizes()[1] == 1));
}

[[nodiscard]] std::int64_t series_vector_extent(const Series& series) noexcept {
  return series.numel();
}

void require_pair_compatible(const Series& first, const Series& second, std::string_view where) {
  if (first.numel() == 0 || second.numel() == 0) {
    throw std::invalid_argument(std::string(where) + ": empty series inputs are not supported");
  }
  if (first.numel() == second.numel() || first.numel() == 1 || second.numel() == 1) {
    return;
  }
  throw std::invalid_argument(std::string(where) + ": series lengths must match or broadcast from scalar");
}

void require_optional_series_compatible(
    const std::optional<Series>& extra,
    std::int64_t target,
    std::string_view where,
    std::string_view what) {
  if (!extra.has_value()) {
    return;
  }
  if (extra->numel() == 0) {
    throw std::invalid_argument(std::string(where) + ": " + std::string(what) + " cannot be empty");
  }
  if (!(extra->numel() == target || extra->numel() == 1 || target == 1)) {
    throw std::invalid_argument(
        std::string(where) + ": " + std::string(what) + " length must match the plotted point count or be scalar");
  }
}

void require_triplet_compatible(const Series& x, const Series& y, const Series& z, std::string_view where) {
  require_pair_compatible(x, y, where);
  const std::int64_t target = std::max(x.numel(), y.numel());
  if (!(z.numel() == target || z.numel() == 1 || (target == 1 && z.numel() > 0))) {
    throw std::invalid_argument(std::string(where) + ": z length must match broadcasted x/y length");
  }
}

void require_cloud_compatible(const CloudData& data, std::string_view where) {
  require_pair_compatible(data.x, data.y, where);
  const std::int64_t target = std::max(data.x.numel(), data.y.numel());
  require_optional_series_compatible(data.size, target, where, "size");
  require_optional_series_compatible(data.color, target, where, "color");
}

void require_cloud3_compatible(const Cloud3Data& data, std::string_view where) {
  require_triplet_compatible(data.x, data.y, data.z, where);
  const std::int64_t target = std::max({data.x.numel(), data.y.numel(), data.z.numel()});
  require_optional_series_compatible(data.size, target, where, "size");
  require_optional_series_compatible(data.color, target, where, "color");
}

void require_fieldmap_compatible(const FieldmapData& data, std::string_view where) {
  if (data.z.dim() != 2) {
    throw std::invalid_argument(std::string(where) + ": z must be a rank-2 matrix");
  }
  const auto rows = data.z.sizes()[0];
  const auto cols = data.z.sizes()[1];
  if (rows <= 0 || cols <= 0) {
    throw std::invalid_argument(std::string(where) + ": z must be non-empty");
  }

  const bool x_vector_ok = series_is_vector(data.x) && series_vector_extent(data.x) == cols;
  const bool y_vector_ok = series_is_vector(data.y) && series_vector_extent(data.y) == rows;
  const bool x_matrix_ok = data.x.dim() == 2 && data.x.sizes()[0] == rows && data.x.sizes()[1] == cols;
  const bool y_matrix_ok = data.y.dim() == 2 && data.y.sizes()[0] == rows && data.y.sizes()[1] == cols;
  if (!((x_vector_ok && y_vector_ok) || (x_matrix_ok && y_matrix_ok))) {
    throw std::invalid_argument(
        std::string(where) +
        ": x/y must both be vectors with extents matching z columns/rows or full matrices matching z");
  }
  if (data.color.has_value() && data.color->numel() != data.z.numel()) {
    throw std::invalid_argument(std::string(where) + ": color data must match z element count");
  }
}

void require_ribbon_compatible(const RibbonData& data, std::string_view where) {
  if (!std::isfinite(data.base)) {
    throw std::invalid_argument(std::string(where) + ": base must be finite");
  }
  if (!(data.y.dim() == 1 || data.y.dim() == 2)) {
    throw std::invalid_argument(std::string(where) + ": y must be a vector or rank-2 matrix");
  }
  if (data.y.numel() == 0) {
    throw std::invalid_argument(std::string(where) + ": y cannot be empty");
  }
  const std::int64_t rows = data.y.dim() == 2 ? data.y.sizes()[0] : data.y.numel();
  const bool x_vector_ok = series_is_vector(data.x) && series_vector_extent(data.x) == rows;
  const bool x_matrix_ok = data.y.dim() == 2 && data.x.dim() == 2 &&
                           data.x.sizes()[0] == data.y.sizes()[0] &&
                           data.x.sizes()[1] == data.y.sizes()[1];
  if (!(x_vector_ok || x_matrix_ok)) {
    throw std::invalid_argument(
        std::string(where) + ": x must match the ribbon rows or the full y matrix shape");
  }
}

void require_spike_compatible(const SpikeData& data, std::string_view where) {
  require_pair_compatible(data.x, data.y, where);
  if (!std::isfinite(data.base)) {
    throw std::invalid_argument(std::string(where) + ": base must be finite");
  }
}

void require_wedge_compatible(const WedgeData& data, std::string_view where) {
  if (!series_is_vector(data.values) || data.values.numel() == 0) {
    throw std::invalid_argument(std::string(where) + ": values must be a non-empty vector");
  }
  const Series values = data.values.to(ScalarType::Float64).reshape({data.values.numel()});
  const auto host = values.to_vec<double>();
  bool any_positive = false;
  for (double value : host) {
    if (!std::isfinite(value)) {
      throw std::invalid_argument(std::string(where) + ": values must be finite");
    }
    if (value < 0.0) {
      throw std::invalid_argument(std::string(where) + ": values must be non-negative");
    }
    any_positive = any_positive || value > 0.0;
  }
  if (!any_positive) {
    throw std::invalid_argument(std::string(where) + ": at least one value must be positive");
  }
  if (data.explode.has_value()) {
    if (!series_is_vector(*data.explode)) {
      throw std::invalid_argument(std::string(where) + ": explode must be a vector");
    }
    require_pair_compatible(*data.explode, data.values, where);
  }
  if (!data.labels.empty() && data.labels.size() != static_cast<std::size_t>(data.values.numel())) {
    throw std::invalid_argument(std::string(where) + ": labels must match the wedge count");
  }
}

void require_spire3_compatible(const Spire3Data& data, std::string_view where) {
  if (data.z.dim() != 2) {
    throw std::invalid_argument(std::string(where) + ": z must be a rank-2 matrix");
  }
  const auto rows = data.z.sizes()[0];
  const auto cols = data.z.sizes()[1];
  if (rows <= 0 || cols <= 0) {
    throw std::invalid_argument(std::string(where) + ": z must be non-empty");
  }
  const bool x_vector_ok = series_is_vector(data.x) && series_vector_extent(data.x) == cols;
  const bool y_vector_ok = series_is_vector(data.y) && series_vector_extent(data.y) == rows;
  const bool x_matrix_ok = data.x.dim() == 2 && data.x.sizes()[0] == rows && data.x.sizes()[1] == cols;
  const bool y_matrix_ok = data.y.dim() == 2 && data.y.sizes()[0] == rows && data.y.sizes()[1] == cols;
  if (!((x_vector_ok && y_vector_ok) || (x_matrix_ok && y_matrix_ok))) {
    throw std::invalid_argument(
        std::string(where) +
        ": x/y must both be vectors with extents matching z columns/rows or full matrices matching z");
  }
  if (!(data.width > 0.0 && std::isfinite(data.width) && data.depth > 0.0 && std::isfinite(data.depth))) {
    throw std::invalid_argument(std::string(where) + ": width and depth must be positive and finite");
  }
  if (!std::isfinite(data.base)) {
    throw std::invalid_argument(std::string(where) + ": base must be finite");
  }
  if (data.color.has_value() && data.color->numel() != data.z.numel()) {
    throw std::invalid_argument(std::string(where) + ": color data must match z element count");
  }
}

void require_whisk_component(
    const std::optional<Series>& delta,
    const Series& y,
    std::string_view where,
    std::string_view what) {
  if (!delta.has_value()) {
    return;
  }
  const Series& value = *delta;
  if (value.numel() == 0) {
    throw std::invalid_argument(std::string(where) + ": " + std::string(what) + " cannot be empty");
  }
  if (value.numel() == 1) {
    return;
  }
  if (y.dim() == 2 && value.dim() == 2 &&
      value.sizes()[0] == y.sizes()[0] && value.sizes()[1] == y.sizes()[1]) {
    return;
  }
  if (series_is_vector(value) &&
      series_vector_extent(value) == (y.dim() == 2 ? y.sizes()[0] : y.numel())) {
    return;
  }
  if (value.numel() == y.numel()) {
    return;
  }
  throw std::invalid_argument(
      std::string(where) + ": " + std::string(what) + " must be scalar, match y rows, or match y element count");
}

void require_whisk_compatible(const WhiskData& data, std::string_view where) {
  if (!(data.y.dim() == 1 || data.y.dim() == 2)) {
    throw std::invalid_argument(std::string(where) + ": y must be a vector or rank-2 matrix");
  }
  if (data.y.numel() == 0) {
    throw std::invalid_argument(std::string(where) + ": y cannot be empty");
  }
  const std::int64_t rows = data.y.dim() == 2 ? data.y.sizes()[0] : data.y.numel();
  const bool x_vector_ok = series_is_vector(data.x) && series_vector_extent(data.x) == rows;
  const bool x_matrix_ok = data.y.dim() == 2 && data.x.dim() == 2 &&
                           data.x.sizes()[0] == data.y.sizes()[0] &&
                           data.x.sizes()[1] == data.y.sizes()[1];
  if (!(x_vector_ok || x_matrix_ok)) {
    throw std::invalid_argument(
        std::string(where) + ": x must match the whisk rows or the full y matrix shape");
  }
  require_whisk_component(data.xneg, data.y, where, "xneg");
  require_whisk_component(data.xpos, data.y, where, "xpos");
  require_whisk_component(data.yneg, data.y, where, "yneg");
  require_whisk_component(data.ypos, data.y, where, "ypos");
  if (!data.xneg.has_value() && !data.xpos.has_value() &&
      !data.yneg.has_value() && !data.ypos.has_value()) {
    throw std::invalid_argument(std::string(where) + ": at least one whisk delta must be present");
  }
}

void require_columns_compatible(const ColumnsData& data, std::string_view where) {
  require_pair_compatible(data.x, data.y, where);
  if (!std::isfinite(data.base)) {
    throw std::invalid_argument(std::string(where) + ": base must be finite");
  }
  if (!(data.width > 0.0 && std::isfinite(data.width))) {
    throw std::invalid_argument(std::string(where) + ": width must be positive and finite");
  }
}

void require_bucket_compatible(const BucketData& data, std::string_view where) {
  if (!series_is_vector(data.values) || data.values.numel() == 0) {
    throw std::invalid_argument(std::string(where) + ": values must be a non-empty vector");
  }
  if (data.bins <= 0) {
    throw std::invalid_argument(std::string(where) + ": bins must be positive");
  }
  if (data.range.has_value() && !(std::isfinite(data.range->low) && std::isfinite(data.range->high) &&
                                  data.range->high > data.range->low)) {
    throw std::invalid_argument(std::string(where) + ": range must be finite and increasing");
  }
}

void require_quartet_compatible(const QuartetData& data, std::string_view where) {
  if (!series_is_vector(data.values) || data.values.numel() == 0) {
    throw std::invalid_argument(std::string(where) + ": values must be a non-empty vector");
  }
  if (data.group.has_value()) {
    if (!series_is_vector(*data.group)) {
      throw std::invalid_argument(std::string(where) + ": group positions must be a vector");
    }
    require_pair_compatible(*data.group, data.values, where);
  }
  if (!(data.width > 0.0 && std::isfinite(data.width))) {
    throw std::invalid_argument(std::string(where) + ": width must be positive and finite");
  }
}

void require_violin_compatible(const ViolinData& data, std::string_view where) {
  if (!series_is_vector(data.values) || data.values.numel() == 0) {
    throw std::invalid_argument(std::string(where) + ": values must be a non-empty vector");
  }
  if (data.group.has_value()) {
    if (!series_is_vector(*data.group)) {
      throw std::invalid_argument(std::string(where) + ": group positions must be a vector");
    }
    require_pair_compatible(*data.group, data.values, where);
  }
  if (!(data.options.width > 0.0 && std::isfinite(data.options.width))) {
    throw std::invalid_argument(std::string(where) + ": width must be positive and finite");
  }
  if (!(data.options.bw_adjust > 0.0 && std::isfinite(data.options.bw_adjust))) {
    throw std::invalid_argument(std::string(where) + ": bw_adjust must be positive and finite");
  }
  if (!(data.options.cut >= 0.0 && std::isfinite(data.options.cut))) {
    throw std::invalid_argument(std::string(where) + ": cut must be non-negative and finite");
  }
}

void require_surface_compatible(const SurfaceData& data, std::string_view where) {
  if (data.z.dim() != 2) {
    throw std::invalid_argument(std::string(where) + ": z must be a rank-2 matrix");
  }
  const auto rows = data.z.sizes()[0];
  const auto cols = data.z.sizes()[1];
  if (rows <= 1 || cols <= 1) {
    throw std::invalid_argument(std::string(where) + ": z must be at least 2x2");
  }

  const bool x_vector_ok = series_is_vector(data.x) && series_vector_extent(data.x) == cols;
  const bool y_vector_ok = series_is_vector(data.y) && series_vector_extent(data.y) == rows;
  const bool x_matrix_ok = data.x.dim() == 2 && data.x.sizes()[0] == rows && data.x.sizes()[1] == cols;
  const bool y_matrix_ok = data.y.dim() == 2 && data.y.sizes()[0] == rows && data.y.sizes()[1] == cols;

  if (!((x_vector_ok && y_vector_ok) || (x_matrix_ok && y_matrix_ok))) {
    throw std::invalid_argument(
        std::string(where) +
        ": x/y must both be vectors with extents matching z columns/rows or full matrices matching z");
  }

  if (data.color.has_value()) {
    const Series& color = *data.color;
    if (color.numel() != data.z.numel()) {
      throw std::invalid_argument(std::string(where) + ": color data must match z element count");
    }
  }
}

void require_contour_compatible(const ContourData& data, std::string_view where) {
  if (data.z.dim() != 2) {
    throw std::invalid_argument(std::string(where) + ": z must be a rank-2 matrix");
  }
  const auto rows = data.z.sizes()[0];
  const auto cols = data.z.sizes()[1];
  if (rows <= 1 || cols <= 1) {
    throw std::invalid_argument(std::string(where) + ": z must be at least 2x2");
  }

  const bool x_vector_ok = series_is_vector(data.x) && series_vector_extent(data.x) == cols;
  const bool y_vector_ok = series_is_vector(data.y) && series_vector_extent(data.y) == rows;
  const bool x_matrix_ok = data.x.dim() == 2 && data.x.sizes()[0] == rows && data.x.sizes()[1] == cols;
  const bool y_matrix_ok = data.y.dim() == 2 && data.y.sizes()[0] == rows && data.y.sizes()[1] == cols;

  if (!((x_vector_ok && y_vector_ok) || (x_matrix_ok && y_matrix_ok))) {
    throw std::invalid_argument(
        std::string(where) +
        ": x/y must both be vectors with extents matching z columns/rows or full matrices matching z");
  }

  if (data.levels.empty() && data.auto_levels <= 0) {
    throw std::invalid_argument(std::string(where) + ": automatic contour levels must be positive");
  }

  for (const double level : data.levels) {
    if (!std::isfinite(level)) {
      throw std::invalid_argument(std::string(where) + ": contour levels must be finite");
    }
  }
}

void require_isoshell_compatible(const IsoShellData& data, std::string_view where) {
  if (data.values.dim() != 3) {
    throw std::invalid_argument(std::string(where) + ": values must be a rank-3 field");
  }
  const auto nx = data.values.sizes()[0];
  const auto ny = data.values.sizes()[1];
  const auto nz = data.values.sizes()[2];
  if (nx <= 1 || ny <= 1 || nz <= 1) {
    throw std::invalid_argument(std::string(where) + ": values must have at least 2 samples per axis");
  }
  if (!series_is_vector(data.x) || series_vector_extent(data.x) != nx) {
    throw std::invalid_argument(std::string(where) + ": x must be a vector matching values axis-0 extent");
  }
  if (!series_is_vector(data.y) || series_vector_extent(data.y) != ny) {
    throw std::invalid_argument(std::string(where) + ": y must be a vector matching values axis-1 extent");
  }
  if (!series_is_vector(data.z) || series_vector_extent(data.z) != nz) {
    throw std::invalid_argument(std::string(where) + ": z must be a vector matching values axis-2 extent");
  }
  if (data.levels.empty() && data.auto_levels <= 0) {
    throw std::invalid_argument(std::string(where) + ": automatic levels must be positive");
  }
  for (const double level : data.levels) {
    if (!std::isfinite(level)) {
      throw std::invalid_argument(std::string(where) + ": levels must be finite");
    }
  }
}

void validate_trace_payload(TraceKind kind, const TracePayload& payload) {
  std::visit(
      [&](const auto& data) {
        using Payload = std::decay_t<decltype(data)>;
        if constexpr (std::is_same_v<Payload, TrendData>) {
          require_pair_compatible(data.x, data.y, "trend");
        } else if constexpr (std::is_same_v<Payload, CloudData>) {
          require_cloud_compatible(data, "cloud");
        } else if constexpr (std::is_same_v<Payload, Cloud3Data>) {
          require_cloud3_compatible(data, "cloud3");
        } else if constexpr (std::is_same_v<Payload, FieldmapData>) {
          require_fieldmap_compatible(data, "fieldmap");
        } else if constexpr (std::is_same_v<Payload, RibbonData>) {
          require_ribbon_compatible(data, "ribbon");
        } else if constexpr (std::is_same_v<Payload, SpikeData>) {
          require_spike_compatible(data, "spike");
        } else if constexpr (std::is_same_v<Payload, WedgeData>) {
          require_wedge_compatible(data, "wedge");
        } else if constexpr (std::is_same_v<Payload, WhiskData>) {
          require_whisk_compatible(data, "whisk");
        } else if constexpr (std::is_same_v<Payload, ColumnsData>) {
          require_columns_compatible(data, "columns");
        } else if constexpr (std::is_same_v<Payload, BucketData>) {
          require_bucket_compatible(data, "bucket");
        } else if constexpr (std::is_same_v<Payload, QuartetData>) {
          require_quartet_compatible(data, "quartet");
        } else if constexpr (std::is_same_v<Payload, ViolinData>) {
          require_violin_compatible(data, "violin");
        } else if constexpr (std::is_same_v<Payload, RadialData>) {
          require_pair_compatible(data.theta, data.radius, "radial");
        } else if constexpr (std::is_same_v<Payload, ContourData>) {
          require_contour_compatible(data, "contour");
        } else if constexpr (std::is_same_v<Payload, IsoShellData>) {
          require_isoshell_compatible(data, "isoshell");
        } else if constexpr (std::is_same_v<Payload, Path3Data>) {
          require_triplet_compatible(data.x, data.y, data.z, "path3");
        } else if constexpr (std::is_same_v<Payload, Spire3Data>) {
          require_spire3_compatible(data, "spire3");
        } else if constexpr (std::is_same_v<Payload, SurfaceData>) {
          require_surface_compatible(data, kind == TraceKind::Sheet ? "sheet" : "lattice");
        }
      },
      payload);
}

[[nodiscard]] std::size_t pane_overlay_index(const Pane& pane) noexcept {
  return pane.trace_count() + pane.rules().size() + pane.bands().size() + pane.pins().size();
}

[[nodiscard]] RuleCue make_rule(const Pane& pane, CueAxis axis, double value, std::string label) {
  RuleCue cue;
  cue.axis = axis;
  cue.value = value;
  cue.label = std::move(label);
  cue.color = pane.theme_tokens().accent(pane_overlay_index(pane));
  cue.width = axis == CueAxis::Z ? 1.0 : 1.15;
  return cue;
}

[[nodiscard]] BandCue make_band(const Pane& pane, CueAxis axis, Range1d span, std::string label) {
  BandCue cue;
  cue.axis = axis;
  cue.span = span;
  cue.label = std::move(label);
  cue.stroke = pane.theme_tokens().accent(pane_overlay_index(pane));
  cue.fill = cue.stroke;
  cue.fill.alpha = 0.18;
  return cue;
}

[[nodiscard]] PinCue make_pin(
    const Pane& pane,
    std::string text,
    Vec3d anchor,
    double offset_x,
    double offset_y) {
  PinCue cue;
  cue.anchor = anchor;
  cue.text = std::move(text);
  cue.offset_x = offset_x;
  cue.offset_y = offset_y;
  cue.color = pane.theme_tokens().accent(pane_overlay_index(pane));
  return cue;
}

void require_tick_spec(const Series& positions,
                       pluss::msharp::RangeView::Of<std::string_view> labels,
                       std::string_view axis_name) {
  if (!series_is_vector(positions) || positions.numel() == 0) {
    throw std::invalid_argument(std::string(axis_name) + ": tick positions must be a non-empty vector");
  }
  if (labels.size() != static_cast<std::size_t>(positions.numel())) {
    throw std::invalid_argument(std::string(axis_name) + ": tick labels must match the tick positions");
  }
}

[[nodiscard]] std::vector<std::string> own_tick_labels(pluss::msharp::RangeView::Of<std::string_view> labels) {
  std::vector<std::string> out;
  out.reserve(labels.size());
  for (const auto label : labels) {
    out.emplace_back(label);
  }
  return out;
}

void require_category_spec(
    pluss::msharp::RangeView::Of<std::string_view> labels,
    double first,
    double step,
    std::string_view where) {
  if (labels.empty()) {
    throw std::invalid_argument(std::string(where) + ": labels cannot be empty");
  }
  if (!std::isfinite(first) || !std::isfinite(step) || step == 0.0) {
    throw std::invalid_argument(std::string(where) + ": first/step must be finite and step cannot be zero");
  }
}

[[nodiscard]] Series category_positions(double first, double step, std::size_t count) {
  if (count == 0) {
    return pluss::series::empty({0}, ScalarType::Float64);
  }
  const auto idx = pluss::series::arange(0, static_cast<std::int64_t>(count), 1, ScalarType::Float64);
  return idx.mul(step).add(first);
}

[[nodiscard]] bool finite_and_increasing(Range1d range) noexcept {
  return std::isfinite(range.low) && std::isfinite(range.high) && range.high > range.low;
}

void require_range(Range1d range, std::string_view where) {
  if (!finite_and_increasing(range)) {
    throw std::invalid_argument(std::string(where) + ": range must be finite and strictly increasing");
  }
}

void require_scale_policy(ScalePolicy scale, std::string_view where) {
  if (scale.kind != ScaleKind::SymLog) {
    return;
  }
  if (!std::isfinite(scale.base) || scale.base <= 1.0) {
    throw std::invalid_argument(std::string(where) + ": symlog base must be finite and greater than 1");
  }
  if (!std::isfinite(scale.linear_threshold) || scale.linear_threshold <= 0.0) {
    throw std::invalid_argument(std::string(where) + ": symlog linear threshold must be finite and positive");
  }
}

void require_scale_compatible_range(Range1d range, ScalePolicy scale, std::string_view where) {
  if (scale.kind == ScaleKind::Log10 && (range.low <= 0.0 || range.high <= 0.0)) {
    throw std::invalid_argument(std::string(where) + ": log10 limits must be strictly positive");
  }
}

void require_font_scale(double scale, std::string_view where) {
  if (!std::isfinite(scale) || scale <= 0.0) {
    throw std::invalid_argument(std::string(where) + ": scale must be finite and positive");
  }
}

void require_rotation_angle(double degrees, std::string_view where) {
  if (!std::isfinite(degrees)) {
    throw std::invalid_argument(std::string(where) + ": angle must be finite");
  }
}

void require_auto_tick_decimals(int decimals, std::string_view where) {
  if (decimals < 0 || decimals > 8) {
    throw std::invalid_argument(std::string(where) + ": decimals must be in [0, 8]");
  }
}

void require_axis_gap(double points, std::string_view where) {
  if (!std::isfinite(points) || points < 0.0) {
    throw std::invalid_argument(std::string(where) + ": spacing must be finite and non-negative");
  }
}

} // namespace

Pane::Pane(PaneKind kind, Theme theme, std::string title)
    : kind_(kind),
      theme_(std::move(theme)),
      title_(std::move(title)),
      lens_(kind == PaneKind::Planar ? Lens::planar() : Lens::spatial()) {}

Pane& Pane::title(std::string title) {
  title_ = std::move(title);
  return *this;
}

Pane& Pane::scale_x(ScalePolicy scale) {
  require_scale_policy(scale, "scale_x");
  if (x_axis_.limits.has_value()) {
    require_scale_compatible_range(*x_axis_.limits, scale, "scale_x");
  }
  x_axis_.scale = scale;
  return *this;
}

Pane& Pane::scale_y(ScalePolicy scale) {
  require_scale_policy(scale, "scale_y");
  if (y_axis_.limits.has_value()) {
    require_scale_compatible_range(*y_axis_.limits, scale, "scale_y");
  }
  y_axis_.scale = scale;
  return *this;
}

Pane& Pane::scale_z(ScalePolicy scale) {
  require_scale_policy(scale, "scale_z");
  if (z_axis_.limits.has_value()) {
    require_scale_compatible_range(*z_axis_.limits, scale, "scale_z");
  }
  z_axis_.scale = scale;
  return *this;
}

Pane& Pane::x_limits(Range1d limits) {
  require_range(limits, "x_limits");
  require_scale_compatible_range(limits, x_axis_.scale, "x_limits");
  x_axis_.limits = limits;
  x_axis_.limit_mode = LimitMode::Manual;
  return *this;
}

Pane& Pane::y_limits(Range1d limits) {
  require_range(limits, "y_limits");
  require_scale_compatible_range(limits, y_axis_.scale, "y_limits");
  y_axis_.limits = limits;
  y_axis_.limit_mode = LimitMode::Manual;
  return *this;
}

Pane& Pane::z_limits(Range1d limits) {
  require_range(limits, "z_limits");
  require_scale_compatible_range(limits, z_axis_.scale, "z_limits");
  z_axis_.limits = limits;
  z_axis_.limit_mode = LimitMode::Manual;
  return *this;
}

Pane& Pane::color_limits(Range1d limits) {
  require_range(limits, "color_limits");
  color_axis_.limits = limits;
  color_axis_.limit_mode = LimitMode::Manual;
  return *this;
}

Pane& Pane::x_limit_method(LimitMethod method) noexcept {
  x_axis_.limit_method = method;
  return *this;
}

Pane& Pane::y_limit_method(LimitMethod method) noexcept {
  y_axis_.limit_method = method;
  return *this;
}

Pane& Pane::z_limit_method(LimitMethod method) noexcept {
  z_axis_.limit_method = method;
  return *this;
}

Pane& Pane::color_limit_method(LimitMethod method) noexcept {
  color_axis_.limit_method = method;
  return *this;
}

Pane& Pane::x_tick_format(TickFormat format) noexcept {
  x_axis_.tick_format = format;
  return *this;
}

Pane& Pane::y_tick_format(TickFormat format) noexcept {
  y_axis_.tick_format = format;
  return *this;
}

Pane& Pane::z_tick_format(TickFormat format) noexcept {
  z_axis_.tick_format = format;
  return *this;
}

Pane& Pane::x_auto_tick_decimals(int decimals) {
  require_auto_tick_decimals(decimals, "x_auto_tick_decimals");
  x_axis_.auto_tick_decimals = decimals;
  return *this;
}

Pane& Pane::y_auto_tick_decimals(int decimals) {
  require_auto_tick_decimals(decimals, "y_auto_tick_decimals");
  y_axis_.auto_tick_decimals = decimals;
  return *this;
}

Pane& Pane::z_auto_tick_decimals(int decimals) {
  require_auto_tick_decimals(decimals, "z_auto_tick_decimals");
  z_axis_.auto_tick_decimals = decimals;
  return *this;
}

Pane& Pane::clear_x_auto_tick_decimals() noexcept {
  x_axis_.auto_tick_decimals.reset();
  return *this;
}

Pane& Pane::clear_y_auto_tick_decimals() noexcept {
  y_axis_.auto_tick_decimals.reset();
  return *this;
}

Pane& Pane::clear_z_auto_tick_decimals() noexcept {
  z_axis_.auto_tick_decimals.reset();
  return *this;
}

Pane& Pane::tick_font_scale(double scale) {
  require_font_scale(scale, "tick_font_scale");
  tick_font_scale_ = scale;
  return *this;
}

Pane& Pane::axis_label_font_scale(double scale) {
  require_font_scale(scale, "axis_label_font_scale");
  axis_label_font_scale_ = scale;
  return *this;
}

Pane& Pane::x_tick_rotation(double degrees) {
  require_rotation_angle(degrees, "x_tick_rotation");
  x_axis_.tick_rotation_deg = degrees;
  return *this;
}

Pane& Pane::y_tick_rotation(double degrees) {
  require_rotation_angle(degrees, "y_tick_rotation");
  y_axis_.tick_rotation_deg = degrees;
  return *this;
}

Pane& Pane::z_tick_rotation(double degrees) {
  require_rotation_angle(degrees, "z_tick_rotation");
  z_axis_.tick_rotation_deg = degrees;
  return *this;
}

Pane& Pane::x_tick_label_gap(double points) {
  require_axis_gap(points, "x_tick_label_gap");
  x_axis_.tick_label_gap = points;
  return *this;
}

Pane& Pane::y_tick_label_gap(double points) {
  require_axis_gap(points, "y_tick_label_gap");
  y_axis_.tick_label_gap = points;
  return *this;
}

Pane& Pane::z_tick_label_gap(double points) {
  require_axis_gap(points, "z_tick_label_gap");
  z_axis_.tick_label_gap = points;
  return *this;
}

Pane& Pane::x_label_gap(double points) {
  require_axis_gap(points, "x_label_gap");
  x_axis_.label_gap = points;
  return *this;
}

Pane& Pane::y_label_gap(double points) {
  require_axis_gap(points, "y_label_gap");
  y_axis_.label_gap = points;
  return *this;
}

Pane& Pane::z_label_gap(double points) {
  require_axis_gap(points, "z_label_gap");
  z_axis_.label_gap = points;
  return *this;
}

Pane& Pane::x_label_rotation(double degrees) {
  require_rotation_angle(degrees, "x_label_rotation");
  x_axis_.label_rotation_deg = degrees;
  return *this;
}

Pane& Pane::y_label_rotation(double degrees) {
  require_rotation_angle(degrees, "y_label_rotation");
  y_axis_.label_rotation_deg = degrees;
  return *this;
}

Pane& Pane::y_label_centered(bool enable) noexcept {
  y_axis_.label_centered_on_axis = enable;
  y_axis_.label_centered_explicit = true;
  return *this;
}

Pane& Pane::z_label_rotation(double degrees) {
  require_rotation_angle(degrees, "z_label_rotation");
  z_axis_.label_rotation_deg = degrees;
  return *this;
}

Pane& Pane::clear_x_tick_rotation() noexcept {
  x_axis_.tick_rotation_deg.reset();
  return *this;
}

Pane& Pane::clear_y_tick_rotation() noexcept {
  y_axis_.tick_rotation_deg.reset();
  return *this;
}

Pane& Pane::clear_z_tick_rotation() noexcept {
  z_axis_.tick_rotation_deg.reset();
  return *this;
}

Pane& Pane::clear_x_tick_label_gap() noexcept {
  x_axis_.tick_label_gap.reset();
  return *this;
}

Pane& Pane::clear_y_tick_label_gap() noexcept {
  y_axis_.tick_label_gap.reset();
  return *this;
}

Pane& Pane::clear_z_tick_label_gap() noexcept {
  z_axis_.tick_label_gap.reset();
  return *this;
}

Pane& Pane::clear_x_label_gap() noexcept {
  x_axis_.label_gap.reset();
  return *this;
}

Pane& Pane::clear_y_label_gap() noexcept {
  y_axis_.label_gap.reset();
  return *this;
}

Pane& Pane::clear_z_label_gap() noexcept {
  z_axis_.label_gap.reset();
  return *this;
}

Pane& Pane::x_tick_labels(bool enable) noexcept {
  x_axis_.tick_labels_visible = enable;
  return *this;
}

Pane& Pane::y_tick_labels(bool enable) noexcept {
  y_axis_.tick_labels_visible = enable;
  return *this;
}

Pane& Pane::z_tick_labels(bool enable) noexcept {
  z_axis_.tick_labels_visible = enable;
  return *this;
}

Pane& Pane::clear_x_label_rotation() noexcept {
  x_axis_.label_rotation_deg.reset();
  return *this;
}

Pane& Pane::clear_y_label_rotation() noexcept {
  y_axis_.label_rotation_deg.reset();
  return *this;
}

Pane& Pane::clear_z_label_rotation() noexcept {
  z_axis_.label_rotation_deg.reset();
  return *this;
}

Pane& Pane::auto_x() noexcept {
  x_axis_.limits.reset();
  x_axis_.limit_mode = LimitMode::AutoFit;
  return *this;
}

Pane& Pane::auto_y() noexcept {
  y_axis_.limits.reset();
  y_axis_.limit_mode = LimitMode::AutoFit;
  return *this;
}

Pane& Pane::auto_z() noexcept {
  z_axis_.limits.reset();
  z_axis_.limit_mode = LimitMode::AutoFit;
  return *this;
}

Pane& Pane::auto_color() noexcept {
  color_axis_.limits.reset();
  color_axis_.limit_mode = LimitMode::AutoFit;
  return *this;
}

Pane& Pane::x_label(std::string label) {
  x_axis_.label = std::move(label);
  return *this;
}

Pane& Pane::y_label(std::string label) {
  y_axis_.label = std::move(label);
  return *this;
}

Pane& Pane::z_label(std::string label) {
  z_axis_.label = std::move(label);
  return *this;
}

Pane& Pane::x_ticks(Series positions, pluss::msharp::RangeView::Of<std::string_view> labels) {
  require_tick_spec(positions, labels, "x_ticks");
  x_axis_.ticks = AxisTicks{std::move(positions), own_tick_labels(labels)};
  return *this;
}

Pane& Pane::y_ticks(Series positions, pluss::msharp::RangeView::Of<std::string_view> labels) {
  require_tick_spec(positions, labels, "y_ticks");
  y_axis_.ticks = AxisTicks{std::move(positions), own_tick_labels(labels)};
  return *this;
}

Pane& Pane::z_ticks(Series positions, pluss::msharp::RangeView::Of<std::string_view> labels) {
  require_tick_spec(positions, labels, "z_ticks");
  z_axis_.ticks = AxisTicks{std::move(positions), own_tick_labels(labels)};
  return *this;
}

Pane& Pane::clear_x_ticks() noexcept {
  x_axis_.ticks.reset();
  return *this;
}

Pane& Pane::clear_y_ticks() noexcept {
  y_axis_.ticks.reset();
  return *this;
}

Pane& Pane::clear_z_ticks() noexcept {
  z_axis_.ticks.reset();
  return *this;
}

Pane& Pane::x_categories(pluss::msharp::RangeView::Of<std::string_view> labels, double first, double step) {
  require_category_spec(labels, first, step, "x_categories");
  return x_ticks(category_positions(first, step, labels.size()), labels);
}

Pane& Pane::y_categories(pluss::msharp::RangeView::Of<std::string_view> labels, double first, double step) {
  require_category_spec(labels, first, step, "y_categories");
  return y_ticks(category_positions(first, step, labels.size()), labels);
}

Pane& Pane::z_categories(pluss::msharp::RangeView::Of<std::string_view> labels, double first, double step) {
  require_category_spec(labels, first, step, "z_categories");
  return z_ticks(category_positions(first, step, labels.size()), labels);
}

Pane& Pane::legend(KeyPlace place) noexcept {
  legend_place_ = place;
  return *this;
}

Pane& Pane::legend_frame(KeyFrame frame) noexcept {
  legend_frame_ = frame;
  return *this;
}

Pane& Pane::legend_shape(KeyShape shape) noexcept {
  legend_shape_ = shape;
  return *this;
}

Pane& Pane::grid(GridStyle style) noexcept {
  grid_style_ = style;
  sync_grid_flags_();
  return *this;
}

Pane& Pane::x_major_grid(bool enable) noexcept {
  x_axis_.major_grid = enable;
  sync_grid_style_();
  return *this;
}

Pane& Pane::y_major_grid(bool enable) noexcept {
  y_axis_.major_grid = enable;
  sync_grid_style_();
  return *this;
}

Pane& Pane::z_major_grid(bool enable) noexcept {
  z_axis_.major_grid = enable;
  sync_grid_style_();
  return *this;
}

Pane& Pane::x_minor_grid(bool enable) noexcept {
  x_axis_.minor_grid = enable;
  sync_grid_style_();
  return *this;
}

Pane& Pane::y_minor_grid(bool enable) noexcept {
  y_axis_.minor_grid = enable;
  sync_grid_style_();
  return *this;
}

Pane& Pane::z_minor_grid(bool enable) noexcept {
  z_axis_.minor_grid = enable;
  sync_grid_style_();
  return *this;
}

Pane& Pane::scale_guide(bool enable) noexcept {
  scale_guide_ = enable;
  return *this;
}

Pane& Pane::caption(std::string text) {
  caption_ = std::move(text);
  return *this;
}

Pane& Pane::rule_x(double value, std::string label) {
  if (!std::isfinite(value)) {
    throw std::invalid_argument("rule_x: value must be finite");
  }
  rules_.push_back(make_rule(*this, CueAxis::X, value, std::move(label)));
  return *this;
}

Pane& Pane::rule_y(double value, std::string label) {
  if (!std::isfinite(value)) {
    throw std::invalid_argument("rule_y: value must be finite");
  }
  rules_.push_back(make_rule(*this, CueAxis::Y, value, std::move(label)));
  return *this;
}

Pane& Pane::rule_z(double value, std::string label) {
  if (!std::isfinite(value)) {
    throw std::invalid_argument("rule_z: value must be finite");
  }
  rules_.push_back(make_rule(*this, CueAxis::Z, value, std::move(label)));
  return *this;
}

Pane& Pane::band_x(Range1d span, std::string label) {
  require_range(span, "band_x");
  bands_.push_back(make_band(*this, CueAxis::X, span, std::move(label)));
  return *this;
}

Pane& Pane::band_y(Range1d span, std::string label) {
  require_range(span, "band_y");
  bands_.push_back(make_band(*this, CueAxis::Y, span, std::move(label)));
  return *this;
}

Pane& Pane::band_z(Range1d span, std::string label) {
  require_range(span, "band_z");
  bands_.push_back(make_band(*this, CueAxis::Z, span, std::move(label)));
  return *this;
}

Pane& Pane::pin(std::string text, double x, double y, double offset_x, double offset_y) {
  if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(offset_x) || !std::isfinite(offset_y)) {
    throw std::invalid_argument("pin: anchor and offsets must be finite");
  }
  pins_.push_back(make_pin(*this, std::move(text), Vec3d{x, y, 0.0}, offset_x, offset_y));
  return *this;
}

Pane& Pane::pin(std::string text, Vec3d anchor, double offset_x, double offset_y) {
  if (!std::isfinite(anchor.x) || !std::isfinite(anchor.y) || !std::isfinite(anchor.z) ||
      !std::isfinite(offset_x) || !std::isfinite(offset_y)) {
    throw std::invalid_argument("pin: anchor and offsets must be finite");
  }
  pins_.push_back(make_pin(*this, std::move(text), anchor, offset_x, offset_y));
  return *this;
}

Pane& Pane::align_x(std::string group) {
  x_align_group_ = std::move(group);
  return *this;
}

Pane& Pane::align_y(std::string group) {
  y_align_group_ = std::move(group);
  return *this;
}

Pane& Pane::align_z(std::string group) {
  z_align_group_ = std::move(group);
  return *this;
}

Pane& Pane::align_xy(std::string group) {
  x_align_group_ = group;
  y_align_group_ = std::move(group);
  return *this;
}

Pane& Pane::align_xyz(std::string group) {
  x_align_group_ = group;
  y_align_group_ = group;
  z_align_group_ = std::move(group);
  return *this;
}

Pane& Pane::parallel_xy_labels(bool enable) noexcept {
  parallel_xy_labels_ = enable;
  return *this;
}

Pane& Pane::visible(bool enable) noexcept {
  visible_ = enable;
  return *this;
}

Pane& Pane::axes_visible(bool enable) noexcept {
  axes_visible_ = enable;
  return *this;
}

Pane& Pane::hide() noexcept {
  visible_ = false;
  return *this;
}

Pane& Pane::show() noexcept {
  visible_ = true;
  return *this;
}

Pane& Pane::clear_x_align() noexcept {
  x_align_group_.clear();
  return *this;
}

Pane& Pane::clear_y_align() noexcept {
  y_align_group_.clear();
  return *this;
}

Pane& Pane::clear_z_align() noexcept {
  z_align_group_.clear();
  return *this;
}

Pane& Pane::clear_align() noexcept {
  x_align_group_.clear();
  y_align_group_.clear();
  z_align_group_.clear();
  return *this;
}

Pane& Pane::place(std::int64_t row, std::int64_t col) {
  return place(row, col, 1, 1);
}

Pane& Pane::place(std::int64_t row, std::int64_t col, std::int64_t row_span, std::int64_t col_span) {
  if (row < 0 || col < 0 || row_span <= 0 || col_span <= 0) {
    throw std::invalid_argument("place: row/col must be non-negative and spans must be positive");
  }
  placement_ = PanePlacement{row, col, row_span, col_span, true};
  return *this;
}

void Pane::ensure_planar_(std::string_view op) const {
  if (kind_ != PaneKind::Planar) {
    throw std::logic_error(std::string(op) + " requires a planar pane");
  }
}

void Pane::ensure_volumetric_(std::string_view op) const {
  if (kind_ != PaneKind::Volumetric) {
    throw std::logic_error(std::string(op) + " requires a volumetric pane");
  }
}

Trace& Pane::add_trace_(TraceKind kind, TracePayload payload) {
  validate_trace_payload(kind, payload);
  traces_.emplace_back(kind, std::move(payload));
  Trace& trace = traces_.back();
  trace.stroke(theme_.accent(traces_.size() - 1));
  trace.palette(theme_.surface_palette());
  return trace;
}

void Pane::sync_grid_flags_() noexcept {
  const bool major = grid_style_ == GridStyle::Major || grid_style_ == GridStyle::MajorMinor;
  const bool minor = grid_style_ == GridStyle::MajorMinor;
  x_axis_.major_grid = major;
  y_axis_.major_grid = major;
  z_axis_.major_grid = major;
  x_axis_.minor_grid = minor;
  y_axis_.minor_grid = minor;
  z_axis_.minor_grid = minor;
}

void Pane::sync_grid_style_() noexcept {
  const bool any_minor = x_axis_.minor_grid || y_axis_.minor_grid || z_axis_.minor_grid;
  const bool any_major = x_axis_.major_grid || y_axis_.major_grid || z_axis_.major_grid;
  if (any_minor) {
    grid_style_ = GridStyle::MajorMinor;
  } else if (any_major) {
    grid_style_ = GridStyle::Major;
  } else {
    grid_style_ = GridStyle::None;
  }
}

} // namespace pluss::pl
