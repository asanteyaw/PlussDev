namespace {

using Studio = pluss::pl::Studio;
using Pane = pluss::pl::Pane;
using Trace = pluss::pl::Trace;
using Rgba = pluss::pl::Rgba;
namespace flash = pluss::pl::flash;

constexpr std::string_view kDefaultCatalogPlotName = "catalog";

Studio make_catalog_studio() {
  auto fig = pluss::pl::studio(std::string(kDefaultCatalogPlotName));
  fig.theme(pluss::pl::themes::paper());
  return fig;
}

Rgba catalog_color_for_index(std::size_t index) {
  static const std::array<Rgba, 10> palette{{
      Rgba::from_u8(15, 118, 110),
      Rgba::from_u8(194, 65, 12),
      Rgba::from_u8(29, 78, 216),
      Rgba::from_u8(185, 28, 28),
      Rgba::from_u8(77, 124, 15),
      Rgba::from_u8(3, 105, 161),
      Rgba::from_u8(180, 83, 9),
      Rgba::from_u8(15, 23, 42),
      Rgba::from_u8(190, 18, 60),
      Rgba::from_u8(87, 83, 78),
  }};
  return palette[index % palette.size()];
}

void catalog_style_trace(Trace& trace,
                         std::size_t index,
                         double alpha = 1.0,
                         double line_width = 1.5) {
  auto color = catalog_color_for_index(index);
  color.alpha = std::clamp(alpha, 0.05, 1.0);
  trace.stroke(color);
  trace.width(static_cast<float>(line_width));
  if (color.alpha < 0.999) {
    trace.transparent(true);
  }
}

std::optional<double> catalog_resolved_x_tick_rotation(const pluss::table::DataFramePlotOptions& options) {
  if (options.x_tick_rotation_deg.has_value()) {
    return options.x_tick_rotation_deg;
  }
  if (options.labeled_tick_rotation_deg.has_value()) {
    return options.labeled_tick_rotation_deg;
  }
  return std::nullopt;
}

void catalog_setup_pane(Pane& pane,
                        bool show_legend,
                        const pluss::table::DataFramePlotOptions& options) {
  pane.grid(pluss::pl::GridStyle::Major);
  pane.legend(show_legend ? pluss::pl::KeyPlace::Auto : pluss::pl::KeyPlace::None);
  if (options.x_label_rotation_deg.has_value()) {
    pane.x_label_rotation(*options.x_label_rotation_deg);
  }
  if (options.y_label_rotation_deg.has_value()) {
    pane.y_label_rotation(*options.y_label_rotation_deg);
  }
  if (options.x_tick_format.has_value()) {
    pane.x_tick_format(*options.x_tick_format);
  }
  if (options.y_tick_format.has_value()) {
    pane.y_tick_format(*options.y_tick_format);
  }
  if (options.x_tick_label_gap_points.has_value()) {
    pane.x_tick_label_gap(*options.x_tick_label_gap_points);
  }
  if (options.y_tick_label_gap_points.has_value()) {
    pane.y_tick_label_gap(*options.y_tick_label_gap_points);
  }
  if (options.x_label_gap_points.has_value()) {
    pane.x_label_gap(*options.x_label_gap_points);
  }
  if (options.y_label_gap_points.has_value()) {
    pane.y_label_gap(*options.y_label_gap_points);
  }
}

void catalog_apply_figure_options(Studio& figure, const pluss::table::DataFramePlotOptions& options) {
  const double width = options.width.value_or(figure.extent().width);
  const double height = options.height.value_or(figure.extent().height);
  figure.size(width, height);
  if (options.use_tight_layout) {
    figure.tight_layout(true, options.tight_layout_padding);
  }
}

std::vector<double> catalog_positional_plot_x(std::int64_t nrows) {
  std::vector<double> out(static_cast<std::size_t>(nrows), 0.0);
  std::iota(out.begin(), out.end(), 0.0);
  return out;
}

pluss::Series catalog_slate_to_plot_numeric_series(const pluss::table::Slate& t) {
  pluss::table::Slate numeric_slate = t;
  switch (t.kind()) {
    case pluss::table::SlateKind::Series: {
      break;
    }
    case pluss::table::SlateKind::Timeline: {
      numeric_slate = pluss::table::Slate(t.as_timeline().epoch_nanos(), pluss::table::detail::validity_mask(t));
      break;
    }
    case pluss::table::SlateKind::Note: {
      numeric_slate = t.cast({.series_dtype = pluss::codem::ScalarType::Float64}).series();
      break;
    }
    default:
      PLUSS_FAIL("Catalog::plot: unsupported slate kind for numeric plotting");
  }

  auto values = numeric_slate.as_series().to(pluss::codem::ScalarType::Float64);
  if (pluss::table::detail::has_validity_mask(numeric_slate)) {
    const auto nan_fill = pluss::Series::full(
        {values.numel()},
        std::numeric_limits<double>::quiet_NaN(),
        pluss::codem::ScalarType::Float64);
    values = values.where(pluss::table::detail::validity_mask(numeric_slate)->logical_not(), nan_fill);
  }
  return values.reshape({values.numel()});
}

std::vector<double> catalog_stream_to_plot_numeric_values(const Stream& stream) {
  return catalog_slate_to_plot_numeric_series(stream.to_slate()).to_vec<double>();
}

bool catalog_series_all_zero_i64(const pluss::Series& s) {
  return !s.to(pluss::codem::ScalarType::Int64).ne(0).any().item<bool>();
}

bool catalog_series_all_equal_i64(const pluss::Series& s, std::int64_t target) {
  return !s.to(pluss::codem::ScalarType::Int64).ne(target).any().item<bool>();
}

enum class CatalogTemporalLabelKind : std::uint8_t {
  Date,
  Time,
  DateTime,
};

CatalogTemporalLabelKind catalog_infer_temporal_label_kind(const pluss::Timeline& timeline) {
  const auto has_subsecond = !catalog_series_all_zero_i64(timeline.nanosecond());
  const auto has_clock_time = has_subsecond || !catalog_series_all_zero_i64(timeline.second())
                              || !catalog_series_all_zero_i64(timeline.minute())
                              || !catalog_series_all_zero_i64(timeline.hour());
  if (!has_clock_time) {
    return CatalogTemporalLabelKind::Date;
  }

  const auto epoch_day_anchor = catalog_series_all_equal_i64(timeline.year(), 1970)
                                && catalog_series_all_equal_i64(timeline.month(), 1)
                                && catalog_series_all_equal_i64(timeline.day(), 1);
  if (epoch_day_anchor) {
    return CatalogTemporalLabelKind::Time;
  }
  return CatalogTemporalLabelKind::DateTime;
}

std::vector<std::string> catalog_temporal_slate_to_plot_labels(
    const pluss::table::Slate& t,
    pluss::table::DateTimeLabelView datetime_label_view) {
  const auto timeline = t.as_timeline();
  const auto has_subsecond = !catalog_series_all_zero_i64(timeline.nanosecond());
  const auto rendered = [&]() -> pluss::Note {
    switch (catalog_infer_temporal_label_kind(timeline)) {
      case CatalogTemporalLabelKind::Date:
        return timeline.strftime("%Y-%m-%d");
      case CatalogTemporalLabelKind::Time:
        return has_subsecond ? timeline.strftime("%H:%M:%S.%f") : timeline.strftime("%H:%M:%S");
      case CatalogTemporalLabelKind::DateTime:
        switch (datetime_label_view) {
          case pluss::table::DateTimeLabelView::Date:
            return timeline.strftime("%Y-%m-%d");
          case pluss::table::DateTimeLabelView::Time:
            return has_subsecond ? timeline.strftime("%H:%M:%S.%f") : timeline.strftime("%H:%M:%S");
          case pluss::table::DateTimeLabelView::Full:
          default:
            return has_subsecond ? timeline.strftime("%Y-%m-%d %H:%M:%S.%f") : timeline.strftime("%Y-%m-%d %H:%M:%S");
        }
    }
    return timeline.strftime("%Y-%m-%d %H:%M:%S");
  }();

  std::vector<std::string> out;
  out.reserve(static_cast<std::size_t>(rendered.numel()));
  rendered.for_each_flat([&](std::string_view s) { out.emplace_back(s); });
  return out;
}

std::vector<std::string> catalog_slate_to_plot_labels(const pluss::table::Slate& t,
                                                       const pluss::table::DataFramePlotOptions& options) {
  if (t.kind() == pluss::table::SlateKind::Timeline) {
    return catalog_temporal_slate_to_plot_labels(t, options.datetime_label_view);
  }
  const auto note_like = (t.kind() == pluss::table::SlateKind::Note) ? t : t.cast().note();
  std::vector<std::string> out;
  const auto& note = note_like.as_note();
  out.reserve(static_cast<std::size_t>(note.numel()));
  note.for_each_flat([&](std::string_view s) { out.emplace_back(s); });
  return out;
}

template <class Range>
std::vector<std::string_view> catalog_label_views(const Range& labels) {
  std::vector<std::string_view> out;
  if constexpr (requires { labels.size(); }) {
    out.reserve(static_cast<std::size_t>(labels.size()));
  }
  for (const auto& label : labels) {
    out.emplace_back(std::string_view(label));
  }
  return out;
}

struct CatalogResolvedAxis {
  std::vector<double> values{};
  std::vector<std::string> labels{};
  std::string label{"record"};
  bool uses_labels{false};
};

bool catalog_slate_prefers_labeled_axis(const pluss::table::Slate& t) {
  return t.kind() == pluss::table::SlateKind::Note
         || t.kind() == pluss::table::SlateKind::Timeline;
}

CatalogResolvedAxis catalog_resolve_axis_from_slate(const pluss::table::Slate& axis_slate,
                                                     std::string_view axis_name,
                                                     std::string_view where,
                                                     const pluss::table::DataFramePlotOptions& options) {
  CatalogResolvedAxis axis;
  axis.label = std::string(axis_name);
  if (catalog_slate_prefers_labeled_axis(axis_slate)) {
    axis.values = catalog_positional_plot_x(axis_slate.size());
    axis.labels = catalog_slate_to_plot_labels(axis_slate, options);
    PLUSS_CHECK(static_cast<std::int64_t>(axis.labels.size()) == axis_slate.size(),
                std::string(where) + ": axis '" + std::string(axis_name) + "' length mismatch");
    axis.uses_labels = true;
    return axis;
  }

  axis.values = catalog_stream_to_plot_numeric_values(Stream::from_slate(axis_slate));
  PLUSS_CHECK(static_cast<std::int64_t>(axis.values.size()) == axis_slate.size(),
              std::string(where) + ": axis '" + std::string(axis_name) + "' length mismatch");
  return axis;
}

CatalogResolvedAxis catalog_resolve_self_axis(const Stream& stream,
                                              const pluss::table::DataFramePlotOptions& options) {
  if (!stream.has_label()) {
    return CatalogResolvedAxis{
        .values = catalog_positional_plot_x(stream.size()),
        .labels = {},
        .label = "record",
        .uses_labels = false,
    };
  }
  return catalog_resolve_axis_from_slate(*stream.label(), "record", "Catalog::plot", options);
}

std::pair<std::vector<double>, std::vector<std::string>> catalog_sampled_labeled_ticks(
    const CatalogResolvedAxis& axis,
    std::int64_t max_ticks) {
  const auto total = std::min(axis.values.size(), axis.labels.size());
  if (total == 0) {
    return {};
  }
  const std::size_t capped = static_cast<std::size_t>(std::max<std::int64_t>(2, max_ticks));
  if (total <= capped) {
    return {axis.values, axis.labels};
  }

  std::vector<double> sampled_values;
  std::vector<std::string> sampled_labels;
  sampled_values.reserve(capped);
  sampled_labels.reserve(capped);

  std::size_t prev = std::numeric_limits<std::size_t>::max();
  for (std::size_t i = 0; i < capped; ++i) {
    const auto idx = static_cast<std::size_t>(
        std::llround(static_cast<double>(i) * static_cast<double>(total - 1) / static_cast<double>(capped - 1)));
    if (idx == prev) {
      continue;
    }
    sampled_values.push_back(axis.values[idx]);
    sampled_labels.push_back(axis.labels[idx]);
    prev = idx;
  }
  return {std::move(sampled_values), std::move(sampled_labels)};
}

void catalog_apply_axis(Pane& pane,
                        const CatalogResolvedAxis& axis,
                        const pluss::table::DataFramePlotOptions& options,
                        bool default_if_crowded = false) {
  pane.x_label(axis.label);
  if (!axis.uses_labels || axis.labels.empty()) {
    return;
  }
  const auto [tick_values, tick_labels] = catalog_sampled_labeled_ticks(axis, options.max_labeled_ticks);
  const auto tick_label_views = catalog_label_views(tick_labels);
  pane.x_ticks(tick_values, pluss::msharp::RangeView::Of<std::string_view>(tick_label_views));
  if (const auto rot = catalog_resolved_x_tick_rotation(options); rot.has_value()) {
    pane.x_tick_rotation(*rot);
  } else if (default_if_crowded || axis.labels.size() > tick_labels.size()) {
    pane.x_tick_rotation(45.0);
  }
}

std::optional<std::string_view> catalog_resolve_x_key(
    std::optional<std::string_view> x_key,
    const pluss::table::DataFramePlotOptions& options) {
  if (x_key.has_value()) {
    return x_key;
  }
  if (options.default_x_feature.has_value()) {
    return std::string_view(*options.default_x_feature);
  }
  return std::nullopt;
}

std::optional<std::string_view> catalog_resolve_hue_key(
    std::optional<std::string_view> hue_key,
    const pluss::table::DataFramePlotOptions& options) {
  if (hue_key.has_value()) {
    return hue_key;
  }
  if (options.default_hue_feature.has_value()) {
    return std::string_view(*options.default_hue_feature);
  }
  return std::nullopt;
}

std::vector<std::string> catalog_resolve_plot_keys(const Catalog& base,
                                                   pluss::msharp::RangeView::Of<std::string_view> keys,
                                                   std::string_view where) {
  std::vector<std::string> out;
  if (keys.size() == 0) {
    for (const auto& key : base.keys()) {
      const auto kind = base.keys(key).kind();
      if (kind == StreamKind::Series || kind == StreamKind::Timeline) {
        out.push_back(key);
      }
    }
    PLUSS_CHECK(!out.empty(), std::string(where) + ": no plottable streams found");
    return out;
  }

  out.reserve(keys.size());
  for (const auto key : keys) {
    PLUSS_CHECK(base.has(key), std::string(where) + ": unknown key '" + std::string(key) + "'");
    out.emplace_back(key);
  }
  return out;
}

bool catalog_slate_supports_hue(const pluss::table::Slate& slate) {
  return slate.kind() == pluss::table::SlateKind::Note
         || slate.kind() == pluss::table::SlateKind::Timeline;
}

enum class ResolvedPairAlignBy : std::uint8_t {
  Label = 0,
  Pos = 1,
};

std::vector<std::string> catalog_stream_axis_labels(const Stream& stream,
                                                    const pluss::table::DataFramePlotOptions& options,
                                                    std::string_view where);

std::unordered_map<std::string, std::int64_t> catalog_unique_label_index(const std::vector<std::string>& labels,
                                                                         std::string_view where,
                                                                         std::string_view key);

ResolvedPairAlignBy catalog_resolve_xy_align_by(const Stream& x_stream,
                                                const Stream& y_stream,
                                                pluss::table::XYAlignBy align_by) {
  switch (align_by) {
    case pluss::table::XYAlignBy::Label:
      PLUSS_CHECK(x_stream.has_label() && y_stream.has_label(),
                  "Catalog::plot x/y alignment: label alignment requires labels on both streams");
      return ResolvedPairAlignBy::Label;
    case pluss::table::XYAlignBy::Pos:
      return ResolvedPairAlignBy::Pos;
    case pluss::table::XYAlignBy::Auto:
      return (x_stream.has_label() && y_stream.has_label()) ? ResolvedPairAlignBy::Label : ResolvedPairAlignBy::Pos;
  }
  PLUSS_FAIL("Catalog::plot x/y alignment: unsupported align_by");
}

struct CatalogAlignedXYSeries {
  CatalogResolvedAxis axis{};
  std::vector<double> y{};
};

CatalogAlignedXYSeries catalog_align_xy_series(const Stream& x_stream,
                                               const Stream& y_stream,
                                               const pluss::table::XYAlignOptions& align,
                                               const pluss::table::DataFramePlotOptions& plot_options,
                                               std::string_view where,
                                               std::string_view x_key,
                                               std::string_view y_key) {
  const auto x_slate = x_stream.to_slate();
  const auto full_axis = catalog_resolve_axis_from_slate(x_slate, x_key, where, plot_options);
  const auto y_values = catalog_stream_to_plot_numeric_values(y_stream);

  auto trim_axis_to = [&](std::int64_t n) {
    CatalogResolvedAxis axis;
    axis.label = full_axis.label;
    axis.uses_labels = full_axis.uses_labels;
    axis.values.reserve(static_cast<std::size_t>(n));
    axis.labels.reserve(static_cast<std::size_t>(std::min<std::int64_t>(
        n, static_cast<std::int64_t>(full_axis.labels.size()))));
    for (std::int64_t i = 0; i < n; ++i) {
      axis.values.push_back(full_axis.values[static_cast<std::size_t>(i)]);
      if (i < static_cast<std::int64_t>(full_axis.labels.size())) {
        axis.labels.push_back(full_axis.labels[static_cast<std::size_t>(i)]);
      }
    }
    return axis;
  };

  CatalogAlignedXYSeries out;
  const auto resolved_align_by = catalog_resolve_xy_align_by(x_stream, y_stream, align.align_by);
  switch (resolved_align_by) {
    case ResolvedPairAlignBy::Pos: {
      switch (align.mode) {
        case pluss::table::XYAlignMode::Strict: {
          PLUSS_CHECK(static_cast<std::int64_t>(full_axis.values.size()) == y_stream.size(),
                      std::string(where) + ": x/y length mismatch between '" + std::string(x_key)
                          + "' and '" + std::string(y_key) + "'");
          out.axis = full_axis;
          out.y = y_values;
          return out;
        }
        case pluss::table::XYAlignMode::Inner: {
          const auto n = std::min<std::int64_t>(static_cast<std::int64_t>(full_axis.values.size()), y_stream.size());
          out.axis = trim_axis_to(n);
          out.y.assign(y_values.begin(), y_values.begin() + n);
          return out;
        }
        case pluss::table::XYAlignMode::PadScalar: {
          const auto n = static_cast<std::int64_t>(full_axis.values.size());
          out.axis = full_axis;
          out.y.resize(static_cast<std::size_t>(n), align.pad_scalar);
          const auto overlap = std::min<std::int64_t>(n, y_stream.size());
          std::copy_n(y_values.begin(), overlap, out.y.begin());
          return out;
        }
      }
      break;
    }
    case ResolvedPairAlignBy::Label: {
      const auto x_case_labels = catalog_stream_axis_labels(x_stream, plot_options, where);
      const auto y_case_labels = catalog_stream_axis_labels(y_stream, plot_options, where);
      const auto y_index = catalog_unique_label_index(y_case_labels, where, y_key);

      if (align.mode == pluss::table::XYAlignMode::Strict) {
        PLUSS_CHECK(x_case_labels.size() == y_case_labels.size(),
                    std::string(where) + ": label-aligned x/y length mismatch between '" + std::string(x_key)
                        + "' and '" + std::string(y_key) + "'");
        for (std::size_t i = 0; i < x_case_labels.size(); ++i) {
          PLUSS_CHECK(x_case_labels[i] == y_case_labels[i],
                      std::string(where) + ": label-aligned x/y labels differ between '" + std::string(x_key)
                          + "' and '" + std::string(y_key) + "'");
        }
      }

      out.axis.label = full_axis.label;
      out.axis.uses_labels = full_axis.uses_labels;
      out.axis.values.reserve(full_axis.values.size());
      out.axis.labels.reserve(full_axis.labels.size());
      out.y.reserve(full_axis.values.size());

      for (std::size_t i = 0; i < x_case_labels.size(); ++i) {
        const auto hit = y_index.find(x_case_labels[i]);
        if (hit == y_index.end()) {
          PLUSS_CHECK(align.mode != pluss::table::XYAlignMode::Strict,
                      std::string(where) + ": missing label '" + x_case_labels[i] + "' from y key '"
                          + std::string(y_key) + "'");
          if (align.mode == pluss::table::XYAlignMode::Inner) {
            continue;
          }
          out.axis.values.push_back(full_axis.values[i]);
          if (i < full_axis.labels.size()) {
            out.axis.labels.push_back(full_axis.labels[i]);
          }
          out.y.push_back(align.pad_scalar);
          continue;
        }

        out.axis.values.push_back(full_axis.values[i]);
        if (i < full_axis.labels.size()) {
          out.axis.labels.push_back(full_axis.labels[i]);
        }
        out.y.push_back(y_values[static_cast<std::size_t>(hit->second)]);
      }
      return out;
    }
  }
  PLUSS_FAIL(std::string(where) + ": unsupported x/y alignment mode");
}

template <class PlotOptions, class DrawFn>
Studio catalog_make_xy_figure_aligned(const Catalog& base,
                                      pluss::msharp::RangeView::Of<std::string_view> y_keys,
                                      std::optional<std::string_view> x_key,
                                      const pluss::table::DataFramePlotOptions& plot_options,
                                      const PlotOptions& style_options,
                                      std::string_view where,
                                      DrawFn&& draw) {
  const auto names = catalog_resolve_plot_keys(base, y_keys, where);
  const auto resolved_x = catalog_resolve_x_key(x_key, plot_options);
  if (resolved_x.has_value()) {
    PLUSS_CHECK(base.has(*resolved_x), std::string(where) + ": unknown x key '" + std::string(*resolved_x) + "'");
  }

  auto fig = make_catalog_studio();
  catalog_apply_figure_options(fig, plot_options);
  auto& pane = fig.pane2();
  catalog_setup_pane(pane, names.size() > 1, plot_options);

  std::vector<CatalogAlignedXYSeries> aligned;
  aligned.reserve(names.size());
  if (resolved_x.has_value()) {
    const auto x_stream = base.keys(*resolved_x);
    for (const auto& key : names) {
      aligned.push_back(catalog_align_xy_series(
          x_stream, base.keys(key), style_options.align, plot_options, where, *resolved_x, key));
    }
  }

  if (resolved_x.has_value()) {
    if (names.size() == 1 || style_options.align.mode != pluss::table::XYAlignMode::Inner) {
      catalog_apply_axis(pane, aligned.front().axis, plot_options);
    } else {
      pane.x_label(std::string(*resolved_x));
    }
  } else if (names.size() == 1) {
    catalog_apply_axis(pane, catalog_resolve_self_axis(base.keys(names.front()), plot_options), plot_options);
  } else {
    pane.x_label("record");
  }
  pane.y_label(names.size() == 1 ? names.front() : std::string("value"));

  for (std::size_t i = 0; i < names.size(); ++i) {
    const auto& key = names[i];
    if (resolved_x.has_value()) {
      draw(pane, aligned[i].axis.values, aligned[i].y, key, i, style_options);
      continue;
    }
    const auto stream = base.keys(key);
    const auto axis = catalog_resolve_self_axis(stream, plot_options);
    const auto y_values = catalog_stream_to_plot_numeric_values(stream);
    PLUSS_CHECK(axis.values.size() == y_values.size(),
                std::string(where) + ": x/y length mismatch for key '" + key + "'");
    draw(pane, axis.values, y_values, key, i, style_options);
  }

  return fig;
}

ResolvedPairAlignBy catalog_resolve_pair_align_by(const Stream& left,
                                                  const Stream& right,
                                                  PairAlignBy align_by) {
  switch (align_by) {
    case PairAlignBy::Label:
      PLUSS_CHECK(left.has_label() && right.has_label(),
                  "Catalog::plot pairwise alignment: label alignment requires labels on both streams");
      return ResolvedPairAlignBy::Label;
    case PairAlignBy::Pos:
      return ResolvedPairAlignBy::Pos;
    case PairAlignBy::Auto:
      return (left.has_label() && right.has_label()) ? ResolvedPairAlignBy::Label : ResolvedPairAlignBy::Pos;
  }
  PLUSS_FAIL("Catalog::plot pairwise alignment: unsupported align_by");
}

ResolvedPairAlignBy catalog_resolve_group_align_by(const Catalog& base,
                                                   const std::vector<std::string>& keys,
                                                   PairAlignBy align_by) {
  switch (align_by) {
    case PairAlignBy::Label: {
      for (const auto& key : keys) {
        PLUSS_CHECK(base.keys(key).has_label(),
                    "Catalog::plot matrix alignment: label alignment requires labels on every selected stream");
      }
      return ResolvedPairAlignBy::Label;
    }
    case PairAlignBy::Pos:
      return ResolvedPairAlignBy::Pos;
    case PairAlignBy::Auto: {
      const bool all_labeled =
          std::all_of(keys.begin(), keys.end(), [&](const std::string& key) { return base.keys(key).has_label(); });
      return all_labeled ? ResolvedPairAlignBy::Label : ResolvedPairAlignBy::Pos;
    }
  }
  PLUSS_FAIL("Catalog::plot matrix alignment: unsupported align_by");
}

ResolvedPairAlignBy catalog_resolve_group_align_by(const Catalog& base,
                                                   const std::vector<std::string>& keys,
                                                   pluss::table::XYAlignBy align_by) {
  switch (align_by) {
    case pluss::table::XYAlignBy::Label: {
      for (const auto& key : keys) {
        PLUSS_CHECK(base.keys(key).has_label(),
                    "Catalog::plot group alignment: label alignment requires labels on every selected stream");
      }
      return ResolvedPairAlignBy::Label;
    }
    case pluss::table::XYAlignBy::Pos:
      return ResolvedPairAlignBy::Pos;
    case pluss::table::XYAlignBy::Auto: {
      const bool all_labeled =
          std::all_of(keys.begin(), keys.end(), [&](const std::string& key) { return base.keys(key).has_label(); });
      return all_labeled ? ResolvedPairAlignBy::Label : ResolvedPairAlignBy::Pos;
    }
  }
  PLUSS_FAIL("Catalog::plot group alignment: unsupported align_by");
}

std::vector<std::string> catalog_stream_axis_labels(const Stream& stream,
                                                    const pluss::table::DataFramePlotOptions& options,
                                                    std::string_view where) {
  PLUSS_CHECK(stream.has_label(), std::string(where) + ": stream has no labels");
  const auto labels = catalog_slate_to_plot_labels(*stream.label(), options);
  PLUSS_CHECK(static_cast<std::int64_t>(labels.size()) == stream.size(),
              std::string(where) + ": label/value length mismatch");
  return labels;
}

std::unordered_map<std::string, std::int64_t> catalog_unique_label_index(const std::vector<std::string>& labels,
                                                                          std::string_view where,
                                                                          std::string_view key) {
  std::unordered_map<std::string, std::int64_t> out;
  out.reserve(labels.size() * 2);
  for (std::size_t i = 0; i < labels.size(); ++i) {
    const auto [it, inserted] = out.emplace(labels[i], static_cast<std::int64_t>(i));
    PLUSS_CHECK(inserted,
                std::string(where) + ": duplicate label '" + labels[i] + "' in stream '" + std::string(key) + "'");
    (void)it;
  }
  return out;
}

struct CatalogPairGlobalContext {
  std::optional<std::vector<std::string>> common_labels{};
  std::optional<std::int64_t> common_length{};
};

CatalogPairGlobalContext catalog_build_global_pair_context(const Catalog& base,
                                                           const std::vector<std::string>& keys,
                                                           const CatalogPairAlignOptions& options,
                                                           const pluss::table::DataFramePlotOptions& plot_options,
                                                           std::string_view where) {
  CatalogPairGlobalContext ctx;
  if (keys.empty()) {
    return ctx;
  }
  if (options.align != PairAlign::GlobalInner) {
    return ctx;
  }

  switch (catalog_resolve_group_align_by(base, keys, options.align_by)) {
    case ResolvedPairAlignBy::Pos: {
      std::int64_t common = std::numeric_limits<std::int64_t>::max();
      for (const auto& key : keys) {
        common = std::min(common, base.keys(key).size());
      }
      ctx.common_length = (common == std::numeric_limits<std::int64_t>::max()) ? 0 : common;
      return ctx;
    }
    case ResolvedPairAlignBy::Label: {
      const auto first_labels = catalog_stream_axis_labels(base.keys(keys.front()), plot_options, where);
      std::vector<std::unordered_map<std::string, std::int64_t>> label_maps;
      label_maps.reserve(keys.size());
      for (const auto& key : keys) {
        const auto labels = catalog_stream_axis_labels(base.keys(key), plot_options, where);
        label_maps.push_back(catalog_unique_label_index(labels, where, key));
      }

      std::vector<std::string> common_labels;
      common_labels.reserve(first_labels.size());
      for (const auto& label : first_labels) {
        const bool present_in_all =
            std::all_of(label_maps.begin() + 1, label_maps.end(), [&](const auto& map) { return map.contains(label); });
        if (present_in_all) {
          common_labels.push_back(label);
        }
      }
      ctx.common_labels = std::move(common_labels);
      return ctx;
    }
  }
  PLUSS_FAIL(std::string(where) + ": unsupported global alignment mode");
}

struct CatalogAlignedNumericPair {
  std::vector<double> x{};
  std::vector<double> y{};
  std::vector<std::int64_t> x_indices{};
  std::vector<std::int64_t> y_indices{};
  std::vector<std::string> labels{};
  ResolvedPairAlignBy align_by{ResolvedPairAlignBy::Pos};
};

CatalogAlignedNumericPair catalog_downsample_pair(CatalogAlignedNumericPair pair, std::int64_t max_points) {
  if (max_points <= 0 || static_cast<std::int64_t>(pair.x.size()) <= max_points) {
    return pair;
  }

  if (max_points == 1) {
    CatalogAlignedNumericPair out;
    out.align_by = pair.align_by;
    out.x.push_back(pair.x.front());
    out.y.push_back(pair.y.front());
    out.x_indices.push_back(pair.x_indices.front());
    out.y_indices.push_back(pair.y_indices.front());
    if (!pair.labels.empty()) {
      out.labels.push_back(pair.labels.front());
    }
    return out;
  }

  std::vector<double> xs;
  std::vector<double> ys;
  std::vector<std::int64_t> x_indices;
  std::vector<std::int64_t> y_indices;
  std::vector<std::string> labels;
  xs.reserve(static_cast<std::size_t>(max_points));
  ys.reserve(static_cast<std::size_t>(max_points));
  x_indices.reserve(static_cast<std::size_t>(max_points));
  y_indices.reserve(static_cast<std::size_t>(max_points));
  if (!pair.labels.empty()) {
    labels.reserve(static_cast<std::size_t>(max_points));
  }
  const auto total = pair.x.size();
  for (std::int64_t i = 0; i < max_points; ++i) {
    const auto idx = static_cast<std::size_t>(
        std::llround(static_cast<double>(i) * static_cast<double>(total - 1) / static_cast<double>(max_points - 1)));
    xs.push_back(pair.x[idx]);
    ys.push_back(pair.y[idx]);
    x_indices.push_back(pair.x_indices[idx]);
    y_indices.push_back(pair.y_indices[idx]);
    if (!pair.labels.empty()) {
      labels.push_back(pair.labels[idx]);
    }
  }
  return CatalogAlignedNumericPair{
      .x = std::move(xs),
      .y = std::move(ys),
      .x_indices = std::move(x_indices),
      .y_indices = std::move(y_indices),
      .labels = std::move(labels),
      .align_by = pair.align_by,
  };
}

CatalogAlignedNumericPair catalog_align_numeric_pair(const Stream& x_stream,
                                                     const Stream& y_stream,
                                                     const CatalogPairAlignOptions& options,
                                                     const CatalogPairGlobalContext* global,
                                                     const pluss::table::DataFramePlotOptions& plot_options,
                                                     std::string_view where,
                                                     std::string_view x_key,
                                                     std::string_view y_key) {
  const auto x_values = catalog_stream_to_plot_numeric_values(x_stream);
  const auto y_values = catalog_stream_to_plot_numeric_values(y_stream);
  CatalogAlignedNumericPair out;

  const auto push_if_valid = [&](double xv,
                                 double yv,
                                 std::int64_t xi,
                                 std::int64_t yi,
                                 std::optional<std::string_view> label = std::nullopt) {
    if (std::isfinite(xv) && std::isfinite(yv)) {
      out.x.push_back(xv);
      out.y.push_back(yv);
      out.x_indices.push_back(xi);
      out.y_indices.push_back(yi);
      if (label.has_value()) {
        out.labels.emplace_back(*label);
      }
    }
  };

  auto align_by = catalog_resolve_pair_align_by(x_stream, y_stream, options.align_by);
  out.align_by = align_by;
  if (options.align == PairAlign::GlobalInner && global != nullptr) {
    if (global->common_labels.has_value()) {
      align_by = ResolvedPairAlignBy::Label;
    } else if (global->common_length.has_value()) {
      align_by = ResolvedPairAlignBy::Pos;
    }
    out.align_by = align_by;
  }

  switch (align_by) {
    case ResolvedPairAlignBy::Pos: {
      const auto n = (options.align == PairAlign::GlobalInner && global != nullptr && global->common_length.has_value())
                         ? *global->common_length
                         : std::min(x_stream.size(), y_stream.size());
      out.x.reserve(static_cast<std::size_t>(n));
      out.y.reserve(static_cast<std::size_t>(n));
      out.x_indices.reserve(static_cast<std::size_t>(n));
      out.y_indices.reserve(static_cast<std::size_t>(n));
      for (std::int64_t i = 0; i < n; ++i) {
        push_if_valid(x_values[static_cast<std::size_t>(i)], y_values[static_cast<std::size_t>(i)], i, i);
      }
      return out;
    }
    case ResolvedPairAlignBy::Label: {
      const auto x_labels = catalog_stream_axis_labels(x_stream, plot_options, where);
      const auto y_labels = catalog_stream_axis_labels(y_stream, plot_options, where);
      const auto y_index = catalog_unique_label_index(y_labels, where, y_key);

      std::vector<std::string> labels;
      if (options.align == PairAlign::GlobalInner && global != nullptr && global->common_labels.has_value()) {
        labels = *global->common_labels;
      } else {
        labels.reserve(x_labels.size());
        for (const auto& label : x_labels) {
          if (y_index.contains(label)) {
            labels.push_back(label);
          }
        }
      }

      out.x.reserve(labels.size());
      out.y.reserve(labels.size());
      out.x_indices.reserve(labels.size());
      out.y_indices.reserve(labels.size());
      out.labels.reserve(labels.size());
      const auto x_index = catalog_unique_label_index(x_labels, where, x_key);
      for (const auto& label : labels) {
        const auto xi = static_cast<std::size_t>(x_index.at(label));
        const auto yi = static_cast<std::size_t>(y_index.at(label));
        push_if_valid(
            x_values[xi], y_values[yi], static_cast<std::int64_t>(xi), static_cast<std::int64_t>(yi), label);
      }
      return out;
    }
  }
  PLUSS_FAIL(std::string(where) + ": unsupported pair alignment mode");
}

// N-way generalization of catalog_align_numeric_pair for plots that need
// more than two streams reconciled together at once (3D scatter's x/y/z
// plus optional size/color). A Catalog only enforces per-key type
// homogeneity, not length homogeneity across keys, so this never assumes
// the requested streams share a row count -- it aligns them the same way
// the 2D pairwise/matrix paths do: positionally (truncated to the shortest
// stream) or by shared axis labels, dropping any row where a value is
// non-finite in *any* of the requested streams. Reuses
// catalog_resolve_group_align_by/catalog_build_global_pair_context so the
// resolution policy is identical to scatter_matrix()/corr_heatmap().
struct CatalogAlignedNumericStreams {
  std::vector<std::vector<double>> columns{};
  ResolvedPairAlignBy align_by{ResolvedPairAlignBy::Pos};
};

CatalogAlignedNumericStreams catalog_align_numeric_streams(const Catalog& base,
                                                           const std::vector<std::string>& keys,
                                                           const pluss::table::XYAlignOptions& align,
                                                           const pluss::table::DataFramePlotOptions& plot_options,
                                                           std::string_view where) {
  PLUSS_CHECK(!keys.empty(), std::string(where) + ": at least one key is required for alignment");

  std::vector<Stream> streams;
  streams.reserve(keys.size());
  for (const auto& key : keys) streams.push_back(base.keys(key));

  std::vector<std::vector<double>> raw(streams.size());
  for (std::size_t k = 0; k < streams.size(); ++k) {
    raw[k] = catalog_stream_to_plot_numeric_values(streams[k]);
  }

  CatalogAlignedNumericStreams out;
  out.align_by = catalog_resolve_group_align_by(base, keys, align.align_by);
  out.columns.resize(keys.size());

  const auto row_is_finite = [&](const auto& value_at) {
    for (std::size_t k = 0; k < raw.size(); ++k) {
      if (!std::isfinite(value_at(k))) return false;
    }
    return true;
  };

  if (out.align_by == ResolvedPairAlignBy::Pos) {
    const auto anchor = streams.front().size();
    std::int64_t n = anchor;
    switch (align.mode) {
      case pluss::table::XYAlignMode::Strict:
        for (const auto& s : streams) {
          PLUSS_CHECK(s.size() == anchor,
                      std::string(where) + ": strict positional alignment requires equal lengths");
        }
        break;
      case pluss::table::XYAlignMode::Inner:
        n = std::numeric_limits<std::int64_t>::max();
        for (const auto& s : streams) n = std::min(n, s.size());
        if (n == std::numeric_limits<std::int64_t>::max()) n = 0;
        break;
      case pluss::table::XYAlignMode::PadScalar:
        n = anchor;
        break;
    }
    for (auto& column : out.columns) column.reserve(static_cast<std::size_t>(n));
    for (std::int64_t i = 0; i < n; ++i) {
      const auto value_at = [&](std::size_t k) {
        const auto idx = static_cast<std::size_t>(i);
        if (align.mode == pluss::table::XYAlignMode::PadScalar && idx >= raw[k].size()) {
          return align.pad_scalar;
        }
        return raw[k][idx];
      };
      if (!row_is_finite(value_at)) continue;
      for (std::size_t k = 0; k < raw.size(); ++k) out.columns[k].push_back(value_at(k));
    }
    return out;
  }

  std::vector<std::unordered_map<std::string, std::int64_t>> index_per_stream;
  index_per_stream.reserve(streams.size());
  for (std::size_t k = 0; k < streams.size(); ++k) {
    const auto labels = catalog_stream_axis_labels(streams[k], plot_options, where);
    index_per_stream.push_back(catalog_unique_label_index(labels, where, keys[k]));
  }

  const auto anchor_labels = catalog_stream_axis_labels(streams.front(), plot_options, where);
  std::vector<std::string> selected_labels;
  selected_labels.reserve(anchor_labels.size());
  switch (align.mode) {
    case pluss::table::XYAlignMode::Strict:
      for (std::size_t k = 1; k < streams.size(); ++k) {
        const auto labels = catalog_stream_axis_labels(streams[k], plot_options, where);
        PLUSS_CHECK(labels.size() == anchor_labels.size(),
                    std::string(where) + ": strict label alignment requires equal label counts");
        for (std::size_t i = 0; i < anchor_labels.size(); ++i) {
          PLUSS_CHECK(labels[i] == anchor_labels[i],
                      std::string(where) + ": strict label alignment requires identical label order");
        }
      }
      selected_labels = anchor_labels;
      break;
    case pluss::table::XYAlignMode::Inner:
      for (const auto& label : anchor_labels) {
        const bool present_everywhere = std::all_of(index_per_stream.begin() + 1,
                                                    index_per_stream.end(),
                                                    [&](const auto& idx) { return idx.contains(label); });
        if (present_everywhere) selected_labels.push_back(label);
      }
      break;
    case pluss::table::XYAlignMode::PadScalar:
      selected_labels = anchor_labels;
      break;
  }

  for (auto& column : out.columns) column.reserve(selected_labels.size());
  for (const auto& label : selected_labels) {
    const auto value_at = [&](std::size_t k) {
      const auto hit = index_per_stream[k].find(label);
      if (align.mode == pluss::table::XYAlignMode::PadScalar && hit == index_per_stream[k].end()) {
        return align.pad_scalar;
      }
      return raw[k][static_cast<std::size_t>(index_per_stream[k].at(label))];
    };
    if (!row_is_finite(value_at)) continue;
    for (std::size_t k = 0; k < raw.size(); ++k) out.columns[k].push_back(value_at(k));
  }
  return out;
}

pluss::table::SlateCorrMethod catalog_map_corr_method(pluss::table::DataFrame::CorrMethod method) {
  switch (method) {
    case pluss::table::DataFrame::CorrMethod::Pearson:
      return pluss::table::SlateCorrMethod::Pearson;
    case pluss::table::DataFrame::CorrMethod::Spearman:
      return pluss::table::SlateCorrMethod::Spearman;
    case pluss::table::DataFrame::CorrMethod::KendallTau:
      return pluss::table::SlateCorrMethod::KendallTau;
  }
  PLUSS_FAIL("Catalog::plot().corr_heatmap: unsupported correlation method");
}

pluss::Series catalog_matrix_series_from_rows(const std::vector<std::vector<double>>& rows) {
  const auto nrows = static_cast<std::int64_t>(rows.size());
  const auto ncols = nrows == 0 ? 0 : static_cast<std::int64_t>(rows.front().size());
  std::vector<double> flat;
  flat.reserve(static_cast<std::size_t>(nrows * ncols));
  for (const auto& row : rows) {
    PLUSS_CHECK(static_cast<std::int64_t>(row.size()) == ncols,
                "Catalog::plot matrix: inconsistent row width");
    flat.insert(flat.end(), row.begin(), row.end());
  }
  return pluss::make_series(std::move(flat), {nrows, ncols});
}

} // namespace

Stream Catalog::keys(std::string_view key) const {
  return at(key);
}

Catalog Catalog::keys(std::span<const std::string_view> field_names) const {
  return project_catalog(*this, normalize_names(field_names), "Catalog::keys");
}

Catalog Catalog::keys(std::initializer_list<std::string_view> field_names) const {
  return keys(std::span<const std::string_view>(field_names.begin(), field_names.size()));
}

CatalogKeyAxisAccessor Catalog::keyaxis() const {
  return CatalogKeyAxisAccessor(*this);
}

CatalogGroupBuilder Catalog::group() const {
  return CatalogGroupBuilder(*this);
}

CatalogCastAccessor Catalog::cast() const {
  return CatalogCastAccessor(*this, keys(), {});
}

CatalogCastAccessor Catalog::cast(const pluss::table::SlateCastOptions& options) const {
  return CatalogCastAccessor(*this, keys(), options);
}

CatalogNoteAccessor Catalog::nt() const {
  return CatalogNoteAccessor(*this, keys());
}

CatalogTimelineAccessor Catalog::tl() const {
  return CatalogTimelineAccessor(*this, keys());
}

CatalogTimeSeriesAccessor Catalog::ts() const {
  return CatalogTimeSeriesAccessor(*this, keys());
}

CatalogPlotAccessor Catalog::plot() const {
  return CatalogPlotAccessor(*this);
}

CatalogPlotAccessor Catalog::plot(const pluss::table::DataFramePlotOptions& options) const {
  return CatalogPlotAccessor(*this, options);
}

CatalogPlot3DAccessor Catalog::plot3d() const {
  return CatalogPlot3DAccessor(*this);
}

CatalogPlot3DAccessor Catalog::plot3d(const pluss::table::DataFramePlotOptions& options) const {
  return CatalogPlot3DAccessor(*this, options);
}

CatalogPlotAccessor::CatalogPlotAccessor(Catalog base, pluss::table::DataFramePlotOptions options)
    : base_(std::move(base)), options_(std::move(options)) {}

pluss::pl::Studio CatalogPlotAccessor::line(std::string_view y_key,
                                            std::optional<std::string_view> x_key) const {
  return line({y_key}, x_key, pluss::table::LinePlotOptions{});
}

pluss::pl::Studio CatalogPlotAccessor::line(pluss::msharp::RangeView::Of<std::string_view> y_keys,
                                            std::optional<std::string_view> x_key) const {
  return line(y_keys, x_key, pluss::table::LinePlotOptions{});
}

pluss::pl::Studio CatalogPlotAccessor::line(std::string_view y_key,
                                            std::optional<std::string_view> x_key,
                                            const pluss::table::LinePlotOptions& options) const {
  return line(pluss::msharp::RangeView::Of<std::string_view>({y_key}), x_key, options);
}

pluss::pl::Studio CatalogPlotAccessor::line(pluss::msharp::RangeView::Of<std::string_view> y_keys,
                                            std::optional<std::string_view> x_key,
                                            const pluss::table::LinePlotOptions& options) const {
  return catalog_make_xy_figure_aligned(
      base_, y_keys, x_key, options_, options, "Catalog::plot().line",
      [](Pane& pane,
         const std::vector<double>& x_values,
         const std::vector<double>& y_values,
         const std::string& name,
         std::size_t index,
         const pluss::table::LinePlotOptions& line_options) {
        auto& trace = flash::lineplot(pane, x_values, y_values);
        trace.label(name);
        catalog_style_trace(trace, index, line_options.alpha, line_options.line_width);
      });
}

pluss::pl::Studio CatalogPlotAccessor::step(std::string_view y_key,
                                            std::optional<std::string_view> x_key) const {
  return step({y_key}, x_key, pluss::table::StepPlotOptions{});
}

pluss::pl::Studio CatalogPlotAccessor::step(pluss::msharp::RangeView::Of<std::string_view> y_keys,
                                            std::optional<std::string_view> x_key) const {
  return step(y_keys, x_key, pluss::table::StepPlotOptions{});
}

pluss::pl::Studio CatalogPlotAccessor::step(std::string_view y_key,
                                            std::optional<std::string_view> x_key,
                                            const pluss::table::StepPlotOptions& options) const {
  return step(pluss::msharp::RangeView::Of<std::string_view>({y_key}), x_key, options);
}

pluss::pl::Studio CatalogPlotAccessor::step(pluss::msharp::RangeView::Of<std::string_view> y_keys,
                                            std::optional<std::string_view> x_key,
                                            const pluss::table::StepPlotOptions& options) const {
  return catalog_make_xy_figure_aligned(
      base_, y_keys, x_key, options_, options, "Catalog::plot().step",
      [](Pane& pane,
         const std::vector<double>& x_values,
         const std::vector<double>& y_values,
         const std::string& name,
         std::size_t index,
         const pluss::table::StepPlotOptions& step_options) {
        auto& trace = flash::stepplot(pane, x_values, y_values);
        trace.label(name);
        catalog_style_trace(trace, index, step_options.alpha, step_options.line_width);
      });
}

pluss::pl::Studio CatalogPlotAccessor::area(std::string_view y_key,
                                            std::optional<std::string_view> x_key) const {
  return area({y_key}, x_key, pluss::table::AreaPlotOptions{});
}

pluss::pl::Studio CatalogPlotAccessor::area(pluss::msharp::RangeView::Of<std::string_view> y_keys,
                                            std::optional<std::string_view> x_key) const {
  return area(y_keys, x_key, pluss::table::AreaPlotOptions{});
}

pluss::pl::Studio CatalogPlotAccessor::area(std::string_view y_key,
                                            std::optional<std::string_view> x_key,
                                            const pluss::table::AreaPlotOptions& options) const {
  return area(pluss::msharp::RangeView::Of<std::string_view>({y_key}), x_key, options);
}

pluss::pl::Studio CatalogPlotAccessor::area(pluss::msharp::RangeView::Of<std::string_view> y_keys,
                                            std::optional<std::string_view> x_key,
                                            const pluss::table::AreaPlotOptions& options) const {
  return catalog_make_xy_figure_aligned(
      base_, y_keys, x_key, options_, options, "Catalog::plot().area",
      [](Pane& pane,
         const std::vector<double>& x_values,
         const std::vector<double>& y_values,
         const std::string& name,
         std::size_t index,
         const pluss::table::AreaPlotOptions& area_options) {
        auto& trace = flash::areaplot(pane, x_values, y_values, 0.0);
        trace.label(name);
        catalog_style_trace(trace, index, area_options.alpha, area_options.line_width);
      });
}

pluss::pl::Studio CatalogPlotAccessor::scatter(std::string_view x_key,
                                               std::string_view y_key,
                                               std::optional<std::string_view> hue_key) const {
  return scatter(x_key, y_key, hue_key, CatalogScatterPlotOptions{});
}

pluss::pl::Studio CatalogPlotAccessor::scatter(std::string_view x_key,
                                               std::string_view y_key,
                                               std::optional<std::string_view> hue_key,
                                               const pluss::table::ScatterPlotOptions& options) const {
  CatalogScatterPlotOptions catalog_options;
  catalog_options.scatter = options;
  return scatter(x_key, y_key, hue_key, catalog_options);
}

pluss::pl::Studio CatalogPlotAccessor::scatter(std::string_view x_key,
                                               std::string_view y_key,
                                               std::optional<std::string_view> hue_key,
                                               const CatalogScatterPlotOptions& options) const {
  PLUSS_CHECK(base_.has(x_key), "Catalog::plot().scatter: unknown x key '" + std::string(x_key) + "'");
  PLUSS_CHECK(base_.has(y_key), "Catalog::plot().scatter: unknown y key '" + std::string(y_key) + "'");
  const auto& x_stream = base_.keys(x_key);
  const auto& y_stream = base_.keys(y_key);
  const auto x_slate = x_stream.to_slate();

  if (catalog_slate_prefers_labeled_axis(x_slate)) {
    const auto axis = catalog_resolve_axis_from_slate(x_slate, x_key, "Catalog::plot().scatter", options_);
    const auto y_values = catalog_stream_to_plot_numeric_values(y_stream);
    PLUSS_CHECK(axis.values.size() == y_values.size(),
                "Catalog::plot().scatter: x/y length mismatch between '" + std::string(x_key)
                    + "' and '" + std::string(y_key) + "'");

    auto fig = make_catalog_studio();
    catalog_apply_figure_options(fig, options_);
    auto& pane = fig.pane2();
    const auto resolved_hue = catalog_resolve_hue_key(hue_key, options_);
    catalog_setup_pane(pane, resolved_hue.has_value(), options_);
    catalog_apply_axis(pane, axis, options_);
    pane.y_label(std::string(y_key));

    if (!resolved_hue.has_value()) {
      auto& trace = flash::scatterplot(pane, axis.values, y_values);
      trace.label(std::string(y_key));
      catalog_style_trace(trace, 0, options.scatter.alpha, options.scatter.line_width);
      return fig;
    }

    PLUSS_CHECK(base_.has(*resolved_hue),
                "Catalog::plot().scatter: unknown hue key '" + std::string(*resolved_hue) + "'");
    const auto hue_slate = base_.keys(*resolved_hue).to_slate();
    PLUSS_CHECK(catalog_slate_supports_hue(hue_slate),
                "Catalog::plot().scatter: hue key '" + std::string(*resolved_hue)
                    + "' must be note-like or time-like");
    const auto hue_labels = catalog_slate_to_plot_labels(hue_slate, options_);
    PLUSS_CHECK(hue_labels.size() == axis.values.size(),
                "Catalog::plot().scatter: hue length mismatch for key '" + std::string(*resolved_hue) + "'");

    std::vector<std::string> groups;
    std::unordered_map<std::string, std::size_t> group_index;
    std::vector<std::vector<double>> grouped_x;
    std::vector<std::vector<double>> grouped_y;
    for (std::size_t i = 0; i < hue_labels.size(); ++i) {
      const auto [it, inserted] = group_index.emplace(hue_labels[i], groups.size());
      if (inserted) {
        groups.push_back(hue_labels[i]);
        grouped_x.emplace_back();
        grouped_y.emplace_back();
      }
      const auto idx = it->second;
      grouped_x[idx].push_back(axis.values[i]);
      grouped_y[idx].push_back(y_values[i]);
    }

    for (std::size_t i = 0; i < groups.size(); ++i) {
      auto& trace = flash::scatterplot(pane, grouped_x[i], grouped_y[i]);
      trace.label(groups[i]);
      catalog_style_trace(trace, i, options.scatter.alpha, options.scatter.line_width);
    }
    return fig;
  }

  const auto pair = catalog_align_numeric_pair(
      x_stream, y_stream, options.align, nullptr, options_, "Catalog::plot().scatter", x_key, y_key);

  auto fig = make_catalog_studio();
  catalog_apply_figure_options(fig, options_);
  auto& pane = fig.pane2();
  const auto resolved_hue = catalog_resolve_hue_key(hue_key, options_);
  catalog_setup_pane(pane, resolved_hue.has_value(), options_);
  pane.x_label(std::string(x_key));
  pane.y_label(std::string(y_key));

  if (!resolved_hue.has_value()) {
    auto& trace = flash::scatterplot(pane, pair.x, pair.y);
    trace.label(std::string(y_key));
    catalog_style_trace(trace, 0, options.scatter.alpha, options.scatter.line_width);
    return fig;
  }

  PLUSS_CHECK(base_.has(*resolved_hue),
              "Catalog::plot().scatter: unknown hue key '" + std::string(*resolved_hue) + "'");
  const auto& hue_stream = base_.keys(*resolved_hue);
  const auto hue_slate = hue_stream.to_slate();
  PLUSS_CHECK(catalog_slate_supports_hue(hue_slate),
              "Catalog::plot().scatter: hue key '" + std::string(*resolved_hue) + "' must be note-like or time-like");
  const auto hue_values = catalog_slate_to_plot_labels(hue_slate, options_);

  std::vector<std::string> groups;
  std::unordered_map<std::string, std::size_t> group_index;
  std::vector<std::vector<double>> grouped_x;
  std::vector<std::vector<double>> grouped_y;
  std::optional<std::unordered_map<std::string, std::int64_t>> hue_index;
  if (pair.align_by == ResolvedPairAlignBy::Label) {
    PLUSS_CHECK(hue_stream.has_label(),
                "Catalog::plot().scatter: label-aligned hue requires labels on hue stream '" + std::string(*resolved_hue)
                    + "'");
    hue_index = catalog_unique_label_index(
        catalog_stream_axis_labels(hue_stream, options_, "Catalog::plot().scatter"),
        "Catalog::plot().scatter",
        *resolved_hue);
  }

  for (std::size_t i = 0; i < pair.x.size(); ++i) {
    std::optional<std::string_view> group;
    if (pair.align_by == ResolvedPairAlignBy::Pos) {
      const auto idx = static_cast<std::size_t>(pair.x_indices[i]);
      if (idx < hue_values.size()) {
        group = hue_values[idx];
      }
    } else {
      PLUSS_CHECK(i < pair.labels.size(), "Catalog::plot().scatter: missing pair labels for hue alignment");
      const auto hit = hue_index->find(pair.labels[i]);
      if (hit != hue_index->end()) {
        const auto idx = static_cast<std::size_t>(hit->second);
        if (idx < hue_values.size()) {
          group = hue_values[idx];
        }
      }
    }
    if (!group.has_value()) {
      continue;
    }

    const auto [it, inserted] = group_index.emplace(std::string(*group), groups.size());
    if (inserted) {
      groups.push_back(std::string(*group));
      grouped_x.emplace_back();
      grouped_y.emplace_back();
    }
    const auto idx = it->second;
    grouped_x[idx].push_back(pair.x[i]);
    grouped_y[idx].push_back(pair.y[i]);
  }

  for (std::size_t i = 0; i < groups.size(); ++i) {
    auto& trace = flash::scatterplot(pane, grouped_x[i], grouped_y[i]);
    trace.label(groups[i]);
    catalog_style_trace(trace, i, options.scatter.alpha, options.scatter.line_width);
  }
  return fig;
}

pluss::pl::Studio CatalogPlotAccessor::bar(std::string_view y_key,
                                           std::optional<std::string_view> x_key) const {
  return bar({y_key}, x_key, pluss::table::BarPlotOptions{});
}

pluss::pl::Studio CatalogPlotAccessor::bar(pluss::msharp::RangeView::Of<std::string_view> y_keys,
                                           std::optional<std::string_view> x_key) const {
  return bar(y_keys, x_key, pluss::table::BarPlotOptions{});
}

pluss::pl::Studio CatalogPlotAccessor::bar(std::string_view y_key,
                                           std::optional<std::string_view> x_key,
                                           const pluss::table::BarPlotOptions& options) const {
  return bar(pluss::msharp::RangeView::Of<std::string_view>({y_key}), x_key, options);
}

pluss::pl::Studio CatalogPlotAccessor::bar(pluss::msharp::RangeView::Of<std::string_view> y_keys,
                                           std::optional<std::string_view> x_key,
                                           const pluss::table::BarPlotOptions& options) const {
  return catalog_make_xy_figure_aligned(
      base_, y_keys, x_key, options_, options, "Catalog::plot().bar",
      [](Pane& pane,
         const std::vector<double>& x_values,
         const std::vector<double>& y_values,
         const std::string& name,
         std::size_t index,
         const pluss::table::BarPlotOptions& bar_options) {
        flash::BarPlotOptions core_options;
        core_options.horizontal = bar_options.horizontal;
        auto& trace = bar_options.horizontal ? flash::barplot(pane, y_values, x_values, core_options)
                                             : flash::barplot(pane, x_values, y_values, core_options);
        trace.label(name);
        catalog_style_trace(trace, index, 0.88, 1.0);
      });
}

pluss::pl::Studio CatalogPlotAccessor::hist(std::string_view key, std::int64_t bins) const {
  pluss::table::HistPlotOptions options;
  options.bins = bins;
  return hist(key, options);
}

pluss::pl::Studio CatalogPlotAccessor::hist(pluss::msharp::RangeView::Of<std::string_view> keys,
                                            std::int64_t bins) const {
  pluss::table::HistPlotOptions options;
  options.bins = bins;
  return hist(keys, options);
}

pluss::pl::Studio CatalogPlotAccessor::hist(std::string_view key,
                                            const pluss::table::HistPlotOptions& options) const {
  return hist(pluss::msharp::RangeView::Of<std::string_view>({key}), options);
}

pluss::pl::Studio CatalogPlotAccessor::hist(pluss::msharp::RangeView::Of<std::string_view> keys,
                                            const pluss::table::HistPlotOptions& options) const {
  const auto names = catalog_resolve_plot_keys(base_, keys, "Catalog::plot().hist");
  auto fig = make_catalog_studio();
  catalog_apply_figure_options(fig, options_);
  auto& pane = fig.pane2();
  catalog_setup_pane(pane, names.size() > 1, options_);
  pane.x_label("value").y_label("count");

  const auto bins = static_cast<int>(std::max<std::int64_t>(1, options.bins > 0 ? options.bins : options_.default_bins));
  for (std::size_t i = 0; i < names.size(); ++i) {
    auto values = catalog_slate_to_plot_numeric_series(base_.keys(names[i]).to_slate());
    auto& trace = flash::histplot(pane, values, bins);
    trace.label(names[i]);
    catalog_style_trace(trace, i, options.alpha, options.line_width);
  }
  return fig;
}

pluss::pl::Studio CatalogPlotAccessor::density(std::string_view key, std::int64_t points) const {
  pluss::table::DensityPlotOptions options;
  options.points = points;
  return density(key, options);
}

pluss::pl::Studio CatalogPlotAccessor::density(pluss::msharp::RangeView::Of<std::string_view> keys,
                                               std::int64_t points) const {
  pluss::table::DensityPlotOptions options;
  options.points = points;
  return density(keys, options);
}

pluss::pl::Studio CatalogPlotAccessor::density(std::string_view key,
                                               const pluss::table::DensityPlotOptions& options) const {
  return density(pluss::msharp::RangeView::Of<std::string_view>({key}), options);
}

pluss::pl::Studio CatalogPlotAccessor::density(pluss::msharp::RangeView::Of<std::string_view> keys,
                                               const pluss::table::DensityPlotOptions& options) const {
  const auto names = catalog_resolve_plot_keys(base_, keys, "Catalog::plot().density");
  auto fig = make_catalog_studio();
  catalog_apply_figure_options(fig, options_);
  auto& pane = fig.pane2();
  catalog_setup_pane(pane, names.size() > 1, options_);
  pane.x_label("value").y_label("density");

  const auto points = std::max<std::int64_t>(16, options.points);
  for (std::size_t i = 0; i < names.size(); ++i) {
    auto values = catalog_slate_to_plot_numeric_series(base_.keys(names[i]).to_slate());
    auto& trace = flash::kdeplot(pane, values, points);
    trace.label(names[i]);
    catalog_style_trace(trace, i, options.alpha, options.line_width);
  }
  return fig;
}

pluss::pl::Studio CatalogPlotAccessor::corr_heatmap(pluss::msharp::RangeView::Of<std::string_view> keys) const {
  return corr_heatmap(keys, CatalogCorrHeatmapPlotOptions{});
}

pluss::pl::Studio CatalogPlotAccessor::corr_heatmap(pluss::msharp::RangeView::Of<std::string_view> keys,
                                                    const CatalogCorrHeatmapPlotOptions& options) const {
  const auto names = catalog_resolve_plot_keys(base_, keys, "Catalog::plot().corr_heatmap");
  PLUSS_CHECK(names.size() >= 2, "Catalog::plot().corr_heatmap: need at least two plottable streams");

  const auto global = catalog_build_global_pair_context(
      base_, names, options.align, options_, "Catalog::plot().corr_heatmap");
  const auto corr_method = catalog_map_corr_method(options.method);

  std::vector<std::vector<double>> rows;
  rows.reserve(names.size());
  for (std::size_t row = 0; row < names.size(); ++row) {
    std::vector<double> corr_row;
    corr_row.reserve(names.size());
    for (std::size_t col = 0; col < names.size(); ++col) {
      if (row == col) {
        corr_row.push_back(1.0);
        continue;
      }
      auto pair = catalog_align_numeric_pair(
          base_.keys(names[col]),
          base_.keys(names[row]),
          options.align,
          &global,
          options_,
          "Catalog::plot().corr_heatmap",
          names[col],
          names[row]);
      if (pair.x.size() < 2) {
        corr_row.push_back(std::numeric_limits<double>::quiet_NaN());
        continue;
      }
      const auto x = pluss::table::Slate(pluss::make_series(pair.x, {static_cast<std::int64_t>(pair.x.size())}));
      const auto y = pluss::table::Slate(pluss::make_series(pair.y, {static_cast<std::int64_t>(pair.y.size())}));
      corr_row.push_back(x.corr(y, corr_method));
    }
    rows.push_back(std::move(corr_row));
  }

  auto fig = make_catalog_studio();
  catalog_apply_figure_options(fig, options_);
  auto& pane = fig.pane2();
  catalog_setup_pane(pane, false, options_);

  const auto axis_positions = catalog_positional_plot_x(static_cast<std::int64_t>(names.size()));
  auto z = catalog_matrix_series_from_rows(rows);
  pluss::pl::flash::HeatmapOptions heatmap_options;
  heatmap_options.annotate_levels = options.heatmap.annotate_levels;
  heatmap_options.scale_guide = options.heatmap.scale_guide;
  heatmap_options.borderless = options.heatmap.borderless;
  auto& trace = flash::heatmap(pane, axis_positions, axis_positions, z, heatmap_options);
  trace.label("corr").shade(pluss::pl::ShadeMode::Smooth);

  const auto name_views = catalog_label_views(names);
  const auto name_range = pluss::msharp::RangeView::Of<std::string_view>(name_views);
  pane.x_ticks(axis_positions, name_range).y_ticks(axis_positions, name_range);
  pane.x_label("feature").y_label("feature");
  return fig;
}

pluss::pl::Studio CatalogPlotAccessor::scatter_matrix(pluss::msharp::RangeView::Of<std::string_view> keys) const {
  return scatter_matrix(keys, CatalogScatterMatrixPlotOptions{});
}

pluss::pl::Studio CatalogPlotAccessor::scatter_matrix(pluss::msharp::RangeView::Of<std::string_view> keys,
                                                      pluss::table::ScatterMatrixTriangle triangle) const {
  CatalogScatterMatrixPlotOptions options;
  options.matrix.pair.triangle = triangle;
  return scatter_matrix(keys, options);
}

pluss::pl::Studio CatalogPlotAccessor::scatter_matrix(pluss::msharp::RangeView::Of<std::string_view> keys,
                                                      const CatalogScatterMatrixPlotOptions& options) const {
  const auto names = catalog_resolve_plot_keys(base_, keys, "Catalog::plot().scatter_matrix");
  PLUSS_CHECK(names.size() >= 2, "Catalog::plot().scatter_matrix: need at least two plottable streams");

  const auto global = catalog_build_global_pair_context(
      base_, names, options.align, options_, "Catalog::plot().scatter_matrix");
  auto pair_options = options.matrix.pair;
  if (options.matrix.corner && pair_options.triangle == pluss::table::ScatterMatrixTriangle::Full) {
    pair_options.triangle = pluss::table::ScatterMatrixTriangle::Lower;
  }
  if (pair_options.labels.empty()) {
    pair_options.labels = names;
  }

  auto fig = make_catalog_studio();
  fig.mosaic(static_cast<std::int64_t>(names.size()),
             static_cast<std::int64_t>(names.size()),
             pair_options.grid_gap_x,
             pair_options.grid_gap_y);

  if (!options_.width.has_value() && !options_.height.has_value() && pair_options.auto_size) {
    const double width = std::max(pair_options.min_width,
                                  pair_options.outer_pad_x
                                      + static_cast<double>(names.size()) * pair_options.cell_width);
    const double height = std::max(pair_options.min_height,
                                   pair_options.outer_pad_y
                                       + static_cast<double>(names.size()) * pair_options.cell_height);
    fig.size(width, height);
  }
  catalog_apply_figure_options(fig, options_);

  for (std::size_t row = 0; row < names.size(); ++row) {
    for (std::size_t col = 0; col < names.size(); ++col) {
      const bool visible =
          pair_options.triangle == pluss::table::ScatterMatrixTriangle::Full
          || (pair_options.triangle == pluss::table::ScatterMatrixTriangle::Lower && row >= col)
          || (pair_options.triangle == pluss::table::ScatterMatrixTriangle::Upper && row <= col);
      if (!visible) {
        continue;
      }

      auto& pane = fig.pane2();
      pane.place(static_cast<std::int64_t>(row), static_cast<std::int64_t>(col));
      pane.grid(pair_options.show_grid ? pluss::pl::GridStyle::Major : pluss::pl::GridStyle::None);
      pane.legend(pluss::pl::KeyPlace::None);

      if (row == col) {
        const auto values = catalog_slate_to_plot_numeric_series(base_.keys(names[row]).to_slate());
        if (values.numel() > 0) {
          if (pair_options.diag_kind == pluss::pl::flash::PairPlotDiagKind::Density) {
            auto& trace = flash::kdeplot(pane, values, pair_options.diag_density_points);
            trace.label(names[row]);
            catalog_style_trace(trace, row, 1.0, pair_options.diag_trace_width);
            pane.y_label("density");
          } else {
            auto& trace = flash::histplot(pane, values, static_cast<int>(std::max<std::int64_t>(1, pair_options.diag_bins)));
            trace.label(names[row]);
            catalog_style_trace(trace, row, 0.95, pair_options.diag_trace_width);
            pane.y_label("count");
          }
        }
        pane.x_label(names[col]);
        continue;
      }

      auto pair = catalog_align_numeric_pair(
          base_.keys(names[col]),
          base_.keys(names[row]),
          options.align,
          &global,
          options_,
          "Catalog::plot().scatter_matrix",
          names[col],
          names[row]);
      pair = catalog_downsample_pair(std::move(pair), pair_options.max_points);
      if (!pair.x.empty()) {
        if (pair_options.kind == pluss::pl::flash::PairPlotKind::Reg) {
          pluss::pl::flash::RegPlotOptions reg_options;
          reg_options.show_scatter = true;
          reg_options.show_fit = true;
          reg_options.show_ci = false;
          auto& trace = flash::regplot(pane, pair.x, pair.y, reg_options);
          trace.label(names[row] + "_vs_" + names[col]);
          catalog_style_trace(trace, row + col, 0.92, pair_options.scatter_trace_width);
        } else {
          auto& trace = flash::scatterplot(pane, pair.x, pair.y);
          trace.label(names[row] + "_vs_" + names[col]);
          catalog_style_trace(trace, row + col, 0.92, pair_options.scatter_trace_width);
        }
      }

      pane.x_label(names[col]).y_label(names[row]);
    }
  }

  return fig;
}

pluss::pl::Studio CatalogPlotAccessor::auto_plot() const {
  return auto_plot(pluss::table::AutoPlotOptions{});
}

pluss::pl::Studio CatalogPlotAccessor::auto_plot(const pluss::table::AutoPlotOptions& options) const {
  (void)options;
  PLUSS_CHECK(base_.defined(), "Catalog::plot().auto_plot: undefined handle");
  PLUSS_CHECK(!base_.empty(), "Catalog::plot().auto_plot: empty catalog");
  const auto numeric_keys = catalog_resolve_plot_keys(base_, {}, "Catalog::plot().auto_plot");
  if (numeric_keys.size() == 1) {
    return base_.keys(numeric_keys.front()).plot(options_).auto_plot();
  }
  return line(pluss::msharp::RangeView::Of<std::string_view>{}, std::nullopt, pluss::table::LinePlotOptions{});
}

CatalogPlot3DAccessor::CatalogPlot3DAccessor(Catalog base, pluss::table::DataFramePlotOptions options)
    : base_(std::move(base)), options_(std::move(options)) {}

void CatalogPlot3DAccessor::apply_figure_options(pluss::pl::Studio& figure) const {
  const double width = options_.width.value_or(figure.extent().width);
  const double height = options_.height.value_or(figure.extent().height);
  figure.size(width, height);
  if (options_.use_tight_layout) {
    figure.tight_layout(true, options_.tight_layout_padding);
  }
}

pluss::pl::Studio CatalogPlot3DAccessor::scatter(std::string_view x_key,
                                                 std::string_view y_key,
                                                 std::string_view z_key,
                                                 std::optional<std::string_view> size_key,
                                                 std::optional<std::string_view> color_key,
                                                 const pluss::table::Scatter3DPlotOptions& options) const {
  CatalogScatter3DPlotOptions catalog_options;
  catalog_options.align = options.align.mode;
  catalog_options.align_by = options.align.align_by;
  catalog_options.pad_scalar = options.align.pad_scalar;
  catalog_options.scatter = options;
  return scatter(x_key, y_key, z_key, size_key, color_key, catalog_options);
}

pluss::pl::Studio CatalogPlot3DAccessor::scatter(std::string_view x_key,
                                                 std::string_view y_key,
                                                 std::string_view z_key,
                                                 std::optional<std::string_view> size_key,
                                                 std::optional<std::string_view> color_key,
                                                 const CatalogScatter3DPlotOptions& options) const {
  PLUSS_CHECK(base_.has(x_key), "Catalog::plot3d().scatter: unknown x key '" + std::string(x_key) + "'");
  PLUSS_CHECK(base_.has(y_key), "Catalog::plot3d().scatter: unknown y key '" + std::string(y_key) + "'");
  PLUSS_CHECK(base_.has(z_key), "Catalog::plot3d().scatter: unknown z key '" + std::string(z_key) + "'");
  if (size_key.has_value()) {
    PLUSS_CHECK(base_.has(*size_key), "Catalog::plot3d().scatter: unknown size key '" + std::string(*size_key) + "'");
  }
  if (color_key.has_value()) {
    PLUSS_CHECK(base_.has(*color_key), "Catalog::plot3d().scatter: unknown color key '" + std::string(*color_key) + "'");
  }

  // Keys may be ragged relative to each other (Catalog enforces per-key type
  // homogeneity, not length homogeneity across keys), so every requested
  // column is reconciled together -- by shared axis labels when all of them
  // carry labels, otherwise positionally. The first requested key anchors
  // the domain for strict/pad modes.
  std::vector<std::string> keys{std::string(x_key), std::string(y_key), std::string(z_key)};
  if (size_key.has_value()) keys.emplace_back(*size_key);
  if (color_key.has_value()) keys.emplace_back(*color_key);
  pluss::table::XYAlignOptions align_options{options.align, options.align_by, options.pad_scalar};
  const auto aligned = catalog_align_numeric_streams(
      base_, keys, align_options, options_, "Catalog::plot3d().scatter");
  const auto& x = aligned.columns[0];
  const auto& y = aligned.columns[1];
  const auto& z = aligned.columns[2];
  const auto n = static_cast<std::int64_t>(x.size());

  auto fig = make_catalog_studio();
  auto& pane = fig.pane3("scatter3d");
  pane.grid(pluss::pl::GridStyle::Major).legend(pluss::pl::KeyPlace::Auto);
  pane.x_label(std::string(x_key)).y_label(std::string(y_key)).z_label(std::string(z_key));
  pluss::table::plot::detail::apply_3d_scene_options(pane, options.scatter.scene);

  pluss::pl::Trace* trace = nullptr;
  if (size_key.has_value()) {
    const auto& s = aligned.columns[3];
    if (color_key.has_value()) {
      const auto& c = aligned.columns[4];
      trace = &flash::cloud3(pane, x, y, z, s, c).label("scatter3d");
    } else {
      trace = &flash::cloud3(pane, x, y, z, s).label("scatter3d");
    }
  } else if (color_key.has_value()) {
    const auto& c = aligned.columns[3];
    std::vector<double> default_size(static_cast<std::size_t>(n), options.scatter.default_marker_size);
    trace = &flash::cloud3(pane, x, y, z, default_size, c).label("scatter3d");
  } else {
    trace = &flash::cloud3(pane, x, y, z).label("scatter3d");
  }
  if (trace != nullptr) {
    pluss::table::plot::detail::apply_3d_trace_options(*trace, options.scatter.trace);
  }

  apply_figure_options(fig);
  return fig;
}

pluss::pl::Studio CatalogPlot3DAccessor::line(std::string_view x_key,
                                              std::string_view y_key,
                                              std::string_view z_key,
                                              const pluss::table::Line3DPlotOptions& options) const {
  CatalogLine3DPlotOptions catalog_options;
  catalog_options.align = options.align.mode;
  catalog_options.align_by = options.align.align_by;
  catalog_options.pad_scalar = options.align.pad_scalar;
  catalog_options.line = options;
  return line(x_key, y_key, z_key, catalog_options);
}

pluss::pl::Studio CatalogPlot3DAccessor::line(std::string_view x_key,
                                              std::string_view y_key,
                                              std::string_view z_key,
                                              const CatalogLine3DPlotOptions& options) const {
  PLUSS_CHECK(base_.has(x_key), "Catalog::plot3d().line: unknown x key '" + std::string(x_key) + "'");
  PLUSS_CHECK(base_.has(y_key), "Catalog::plot3d().line: unknown y key '" + std::string(y_key) + "'");
  PLUSS_CHECK(base_.has(z_key), "Catalog::plot3d().line: unknown z key '" + std::string(z_key) + "'");

  pluss::table::XYAlignOptions align_options{options.align, options.align_by, options.pad_scalar};
  const auto aligned = catalog_align_numeric_streams(
      base_, {std::string(x_key), std::string(y_key), std::string(z_key)}, align_options, options_,
      "Catalog::plot3d().line");
  const auto& x = aligned.columns[0];
  const auto& y = aligned.columns[1];
  const auto& z = aligned.columns[2];

  auto fig = make_catalog_studio();
  auto& pane = fig.pane3("line3d");
  pane.grid(pluss::pl::GridStyle::Major).legend(pluss::pl::KeyPlace::Auto);
  pane.x_label(std::string(x_key)).y_label(std::string(y_key)).z_label(std::string(z_key));
  pluss::table::plot::detail::apply_3d_scene_options(pane, options.line.scene);

  auto& trace = flash::path3(pane, x, y, z).label("line3d");
  pluss::table::plot::detail::apply_3d_trace_options(trace, options.line.trace);
  apply_figure_options(fig);
  return fig;
}

pluss::pl::Studio CatalogPlot3DAccessor::hist(std::string_view x_key,
                                              std::string_view y_key,
                                              const pluss::table::Hist3DPlotOptions& options) const {
  CatalogHist3DPlotOptions catalog_options;
  catalog_options.align = options.align.mode;
  catalog_options.align_by = options.align.align_by;
  catalog_options.pad_scalar = options.align.pad_scalar;
  catalog_options.hist = options;
  return hist(x_key, y_key, catalog_options);
}

pluss::pl::Studio CatalogPlot3DAccessor::hist(std::string_view x_key,
                                              std::string_view y_key,
                                              const CatalogHist3DPlotOptions& options) const {
  PLUSS_CHECK(base_.has(x_key), "Catalog::plot3d().hist: unknown x key '" + std::string(x_key) + "'");
  PLUSS_CHECK(base_.has(y_key), "Catalog::plot3d().hist: unknown y key '" + std::string(y_key) + "'");

  pluss::table::XYAlignOptions align_options{options.align, options.align_by, options.pad_scalar};
  const auto aligned = catalog_align_numeric_streams(
      base_, {std::string(x_key), std::string(y_key)}, align_options, options_, "Catalog::plot3d().hist");
  const auto& x = aligned.columns[0];
  const auto& y = aligned.columns[1];

  auto fig = make_catalog_studio();
  auto& pane = fig.pane3("hist3d");
  pane.grid(pluss::pl::GridStyle::Major).legend(pluss::pl::KeyPlace::Auto);
  pane.x_label(std::string(x_key)).y_label(std::string(y_key)).z_label(options.hist.normalize ? "density" : "count");
  pluss::table::plot::detail::apply_3d_scene_options(pane, options.hist.scene);

  pluss::pl::flash::Hist3DOptions hist_options;
  hist_options.bins_x = options.hist.bins_x;
  hist_options.bins_y = options.hist.bins_y;
  hist_options.normalize = options.hist.normalize;
  hist_options.smooth = options.hist.smooth;
  hist_options.smooth_passes = options.hist.smooth_passes;
  hist_options.as_surface = options.hist.as_surface;
  hist_options.as_wireframe = options.hist.as_wireframe;

  auto& trace = flash::hist3d(pane, x, y, hist_options).label(options.hist.normalize ? "density" : "count");
  pluss::table::plot::detail::apply_3d_trace_options(trace, options.hist.trace);
  apply_figure_options(fig);
  return fig;
}

namespace {
TabelizePolicy complete_triples() {
  TabelizePolicy policy;
  policy.align = Align::Inner;
  return policy;
}
}  // namespace

// surface()/wireframe() need the same case/x/y pivot-grid aggregation that
// DataFramePlot3DAccessor::surface/wireframe already implement (via
// DataFrame::pivot_table + build_pivot_grid_3d) -- delegating through
// tabelize() reuses that vectorized logic exactly rather than duplicating
// it at the kindle layer. Projecting down to just {x_key, y_key, z_key}
// before tabelizing matters: tabelizing the whole catalog would let an
// unrelated ragged key elsewhere change the alignment of the three keys
// actually being plotted. A surface needs complete (x, y, z) triples, so it
// asks for Align::Inner explicitly rather than relying on the default (which
// pads).
pluss::pl::Studio CatalogPlot3DAccessor::surface(std::string_view x_key,
                                                 std::string_view y_key,
                                                 std::string_view z_key,
                                                 pluss::table::DataFrame::GroupOp op,
                                                 const pluss::table::Surface3DPlotOptions& options) const {
  PLUSS_CHECK(base_.has(x_key), "Catalog::plot3d().surface: unknown x key '" + std::string(x_key) + "'");
  PLUSS_CHECK(base_.has(y_key), "Catalog::plot3d().surface: unknown y key '" + std::string(y_key) + "'");
  PLUSS_CHECK(base_.has(z_key), "Catalog::plot3d().surface: unknown z key '" + std::string(z_key) + "'");
  return base_.keys({x_key, y_key, z_key}).tabelize(complete_triples()).plot3d(options_).surface(x_key, y_key, z_key, op, options);
}

pluss::pl::Studio CatalogPlot3DAccessor::wireframe(std::string_view x_key,
                                                   std::string_view y_key,
                                                   std::string_view z_key,
                                                   pluss::table::DataFrame::GroupOp op,
                                                   const pluss::table::Wireframe3DPlotOptions& options) const {
  PLUSS_CHECK(base_.has(x_key), "Catalog::plot3d().wireframe: unknown x key '" + std::string(x_key) + "'");
  PLUSS_CHECK(base_.has(y_key), "Catalog::plot3d().wireframe: unknown y key '" + std::string(y_key) + "'");
  PLUSS_CHECK(base_.has(z_key), "Catalog::plot3d().wireframe: unknown z key '" + std::string(z_key) + "'");
  return base_.keys({x_key, y_key, z_key}).tabelize(complete_triples()).plot3d(options_).wireframe(x_key, y_key, z_key, op, options);
}

CatalogKeyAccessor::CatalogKeyAccessor(Catalog base, std::vector<std::string> keys)
    : base_(std::move(base)), keys_(std::move(keys)) {
  PLUSS_CHECK(base_.defined(), "Catalog::keys: undefined handle");
  if (base_.empty()) return;
  PLUSS_CHECK(!keys_.empty(), "Catalog::keys: at least one key is required");
  for (const auto& key : keys_) {
    PLUSS_CHECK(base_.has(key), "Catalog::keys: unknown key '" + key + "'");
  }
}

CatalogCastAccessor CatalogKeyAccessor::cast() const {
  return CatalogCastAccessor(base_, keys_, {});
}

CatalogCastAccessor CatalogKeyAccessor::cast(const pluss::table::SlateCastOptions& options) const {
  return CatalogCastAccessor(base_, keys_, options);
}

CatalogNoteAccessor CatalogKeyAccessor::nt() const {
  return CatalogNoteAccessor(base_, keys_);
}

CatalogTimelineAccessor CatalogKeyAccessor::tl() const {
  return CatalogTimelineAccessor(base_, keys_);
}

CatalogTimeSeriesAccessor CatalogKeyAccessor::ts() const {
  return CatalogTimeSeriesAccessor(base_, keys_);
}

Catalog CatalogKeyAccessor::fillnull(double value) const {
  return map_selected_([&](const Stream& stream) {
    return stream.kind() == StreamKind::Series ? stream.fillnull(value) : stream;
  });
}

Catalog CatalogKeyAccessor::fillnull(std::string_view value) const {
  return map_selected_([&](const Stream& stream) {
    return stream.kind() == StreamKind::Note ? stream.fillnull(value) : stream;
  });
}

Catalog CatalogKeyAccessor::fillnull(const pluss::Timeline& value) const {
  return map_selected_([&](const Stream& stream) {
    return stream.kind() == StreamKind::Timeline
               ? stream.fillnull(value)
               : stream;
  });
}

Catalog CatalogKeyAccessor::fillnull(const pluss::Duration& value) const {
  return map_selected_([&](const Stream& stream) {
    return stream.kind() == StreamKind::Duration ? stream.fillnull(value) : stream;
  });
}

Catalog CatalogKeyAccessor::dropnull() const {
  std::optional<pluss::table::Slate> keep;
  for (const auto& key : keys_) {
    const auto valid = base_.keys(key).notnull().to_slate();
    keep = keep.has_value() ? keep->land(valid) : valid;
  }
  if (!keep.has_value()) return base_;

  const auto mask = keep->as_series();
  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(base_.nkeys()));
  for (const auto& key : base_.keys()) {
    out.emplace_back(key, base_.keys(key).filter(mask));
  }
  return Catalog(std::move(out));
}

Catalog CatalogKeyAccessor::ffill() const {
  return map_selected_([](const Stream& stream) { return stream.ffill(); });
}

Catalog CatalogKeyAccessor::bfill() const {
  return map_selected_([](const Stream& stream) { return stream.bfill(); });
}

Catalog CatalogKeyAccessor::isnull() const {
  return map_selected_([](const Stream& stream) { return stream.isnull(); });
}

Catalog CatalogKeyAccessor::notnull() const {
  return map_selected_([](const Stream& stream) { return stream.notnull(); });
}

Catalog CatalogKeyAccessor::invalid() const {
  return map_selected_([](const Stream& stream) { return stream.invalid(); });
}

Catalog CatalogKeyAccessor::distinct(DistinctOptions options) const {
  return map_selected_([&](const Stream& stream) { return stream.distinct(options); });
}

Catalog CatalogKeyAccessor::freq(FreqOptions options) const {
  return map_selected_([&](const Stream& stream) { return stream.freq(options); });
}

Catalog CatalogKeyAccessor::ndistinct(DistinctOptions options) const {
  return map_selected_([&](const Stream& stream) { return stream.ndistinct(options); });
}

Catalog CatalogKeyAccessor::astype(pluss::codem::ScalarType dtype) const {
  return map_selected_([&](const Stream& stream) { return stream.astype(dtype); });
}

Catalog CatalogKeyAccessor::map(const std::function<Stream(const Stream&)>& fn) const {
  PLUSS_CHECK(static_cast<bool>(fn), "Catalog::keys().map: function is empty");
  return map_selected_([&](const Stream& stream) { return stream.map(fn); });
}

CatalogKeyAxisAccessor::CatalogKeyAxisAccessor(Catalog base) : base_(std::move(base)) {}

Catalog CatalogKeyAxisAccessor::like(std::string_view needle, bool case_sensitive) const {
  PLUSS_CHECK(base_.defined(), "Catalog::keyaxis: undefined handle");
  const auto token = case_sensitive ? std::string(needle) : lowercase_copy(needle);
  std::vector<std::string> selected;
  selected.reserve(static_cast<std::size_t>(base_.nkeys()));
  for (const auto& key : base_.keys()) {
    const auto hay = case_sensitive ? key : lowercase_copy(key);
    if (hay.find(token) != std::string::npos) selected.push_back(key);
  }
  return project_catalog(base_, selected, "Catalog::keyaxis().like");
}

Catalog CatalogKeyAxisAccessor::match(std::string_view pattern) const {
  PLUSS_CHECK(base_.defined(), "Catalog::keyaxis: undefined handle");
  try {
    const std::regex rx{std::string(pattern)};
    std::vector<std::string> selected;
    selected.reserve(static_cast<std::size_t>(base_.nkeys()));
    for (const auto& key : base_.keys()) {
      if (std::regex_search(key, rx)) selected.push_back(key);
    }
    return project_catalog(base_, selected, "Catalog::keyaxis().match");
  } catch (const std::regex_error& e) {
    PLUSS_FAIL("Catalog::keyaxis().match: invalid regex '" + std::string(pattern) + "': " + std::string(e.what()));
  }
}

Catalog CatalogKeyAxisAccessor::series() const {
  PLUSS_CHECK(base_.defined(), "Catalog::keyaxis: undefined handle");
  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(base_.nkeys()));
  for (const auto& key : base_.keys()) {
    const auto stream = base_.keys(key);
    if (stream.kind() == StreamKind::Series) out.emplace_back(key, std::move(stream));
  }
  return Catalog(std::move(out));
}

Catalog CatalogKeyAxisAccessor::note() const {
  PLUSS_CHECK(base_.defined(), "Catalog::keyaxis: undefined handle");
  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(base_.nkeys()));
  for (const auto& key : base_.keys()) {
    const auto stream = base_.keys(key);
    if (stream.kind() == StreamKind::Note) out.emplace_back(key, std::move(stream));
  }
  return Catalog(std::move(out));
}

Catalog CatalogKeyAxisAccessor::timeline() const {
  PLUSS_CHECK(base_.defined(), "Catalog::keyaxis: undefined handle");
  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(base_.nkeys()));
  for (const auto& key : base_.keys()) {
    const auto stream = base_.keys(key);
    if (stream.kind() == StreamKind::Timeline) {
      out.emplace_back(key, std::move(stream));
    }
  }
  return Catalog(std::move(out));
}

Catalog CatalogKeyAxisAccessor::duration() const {
  PLUSS_CHECK(base_.defined(), "Catalog::keyaxis: undefined handle");
  std::vector<std::pair<std::string, Stream>> out;
  out.reserve(static_cast<std::size_t>(base_.nkeys()));
  for (const auto& key : base_.keys()) {
    const auto stream = base_.keys(key);
    if (stream.kind() == StreamKind::Duration) out.emplace_back(key, std::move(stream));
  }
  return Catalog(std::move(out));
}

Catalog CatalogKeyAxisAccessor::prefix(std::string_view prefix) const {
  PLUSS_CHECK(base_.defined(), "Catalog::keyaxis: undefined handle");
  std::vector<std::string> selected;
  selected.reserve(static_cast<std::size_t>(base_.nkeys()));
  const auto token = std::string(prefix);
  for (const auto& key : base_.keys()) {
    if (key.rfind(token, 0) == 0) selected.push_back(key);
  }
  return project_catalog(base_, selected, "Catalog::keyaxis().prefix");
}

Catalog CatalogKeyAxisAccessor::suffix(std::string_view suffix) const {
  PLUSS_CHECK(base_.defined(), "Catalog::keyaxis: undefined handle");
  std::vector<std::string> selected;
  selected.reserve(static_cast<std::size_t>(base_.nkeys()));
  const auto token = std::string(suffix);
  for (const auto& key : base_.keys()) {
    if (key.size() >= token.size() &&
        key.compare(key.size() - token.size(), token.size(), token) == 0) {
      selected.push_back(key);
    }
  }
  return project_catalog(base_, selected, "Catalog::keyaxis().suffix");
}

CatalogCastAccessor::CatalogCastAccessor(Catalog base,
                                         std::vector<std::string> keys,
                                         pluss::table::SlateCastOptions options)
    : keysel_(std::move(base), std::move(keys)),
      options_(std::move(options)) {}

Catalog CatalogCastAccessor::series() const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.cast(options_).series(); });
}

Catalog CatalogCastAccessor::note() const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.cast(options_).note(); });
}

Catalog CatalogCastAccessor::timeline() const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.cast(options_).timeline(); });
}

Catalog CatalogCastAccessor::duration() const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.cast(options_).duration(); });
}

CatalogNoteAccessor::CatalogNoteAccessor(Catalog base, std::vector<std::string> keys)
    : keysel_(std::move(base), std::move(keys)) {}

#define PLUSS_CATALOG_NOTE_UNARY(method) \
  Catalog CatalogNoteAccessor::method() const { \
    return keysel_.map_selected_([](const Stream& stream) { return stream.nt().method(); }); \
  }

PLUSS_CATALOG_NOTE_UNARY(strip)
PLUSS_CATALOG_NOTE_UNARY(lstrip)
PLUSS_CATALOG_NOTE_UNARY(rstrip)
PLUSS_CATALOG_NOTE_UNARY(lower)
PLUSS_CATALOG_NOTE_UNARY(upper)
PLUSS_CATALOG_NOTE_UNARY(casefold)
PLUSS_CATALOG_NOTE_UNARY(titlecase)
PLUSS_CATALOG_NOTE_UNARY(length)

#undef PLUSS_CATALOG_NOTE_UNARY

Catalog CatalogNoteAccessor::strip(std::string_view chars) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.nt().strip(chars); });
}

Catalog CatalogNoteAccessor::lstrip(std::string_view chars) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.nt().lstrip(chars); });
}

Catalog CatalogNoteAccessor::rstrip(std::string_view chars) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.nt().rstrip(chars); });
}

Catalog CatalogNoteAccessor::starts(std::string_view prefix) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.nt().starts(prefix); });
}

Catalog CatalogNoteAccessor::ends(std::string_view suffix) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.nt().ends(suffix); });
}

Catalog CatalogNoteAccessor::has(std::string_view needle,
                                 const pluss::table::TextOptions& options) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.nt().has(needle, options); });
}

Catalog CatalogNoteAccessor::count(std::string_view pattern) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.nt().count(pattern); });
}

Catalog CatalogNoteAccessor::count(std::string_view pattern,
                                   const pluss::table::TextOptions& options) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.nt().count(pattern, options); });
}

Catalog CatalogNoteAccessor::matches(std::string_view pattern, bool full) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.nt().matches(pattern, full); });
}

Catalog CatalogNoteAccessor::fullmatch(std::string_view pattern) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.nt().fullmatch(pattern); });
}

std::unordered_map<std::string, pluss::table::DataFrame> CatalogNoteAccessor::findall(std::string_view pattern) const {
  std::unordered_map<std::string, pluss::table::DataFrame> out;
  out.reserve(keysel_.keys_.size());
  for (const auto& key : keysel_.keys_) out.emplace(key, keysel_.base_.keys(key).nt().findall(pattern));
  return out;
}

std::unordered_map<std::string, pluss::table::DataFrame> CatalogNoteAccessor::findall(
    std::string_view pattern,
    const pluss::table::TextOptions& options) const {
  std::unordered_map<std::string, pluss::table::DataFrame> out;
  out.reserve(keysel_.keys_.size());
  for (const auto& key : keysel_.keys_) out.emplace(key, keysel_.base_.keys(key).nt().findall(pattern, options));
  return out;
}

std::unordered_map<std::string, pluss::table::DataFrame> CatalogNoteAccessor::split(
    std::string_view delimiter,
    std::int64_t limit,
    std::string prefix) const {
  std::unordered_map<std::string, pluss::table::DataFrame> out;
  out.reserve(keysel_.keys_.size());
  for (const auto& key : keysel_.keys_) out.emplace(key, keysel_.base_.keys(key).nt().split(delimiter, limit, prefix));
  return out;
}

std::unordered_map<std::string, pluss::table::DataFrame> CatalogNoteAccessor::split(
    std::string_view pattern,
    const pluss::table::TextOptions& options) const {
  std::unordered_map<std::string, pluss::table::DataFrame> out;
  out.reserve(keysel_.keys_.size());
  for (const auto& key : keysel_.keys_) out.emplace(key, keysel_.base_.keys(key).nt().split(pattern, options));
  return out;
}

std::unordered_map<std::string, pluss::table::DataFrame> CatalogNoteAccessor::partition(
    std::string_view separator,
    std::string prefix) const {
  std::unordered_map<std::string, pluss::table::DataFrame> out;
  out.reserve(keysel_.keys_.size());
  for (const auto& key : keysel_.keys_) out.emplace(key, keysel_.base_.keys(key).nt().partition(separator, prefix));
  return out;
}

Catalog CatalogNoteAccessor::pad(std::int64_t width,
                                 pluss::table::TextAlign align,
                                 char fill) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.nt().pad(width, align, fill); });
}

Catalog CatalogNoteAccessor::chars(std::int64_t start,
                                   std::optional<std::int64_t> stop,
                                   std::int64_t step) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.nt().chars(start, stop, step); });
}

Catalog CatalogNoteAccessor::normalize(pluss::table::UnicodeForm form) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.nt().normalize(form); });
}

Catalog CatalogNoteAccessor::encode(std::string_view encoding) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.nt().encode(encoding); });
}

std::unordered_map<std::string, pluss::table::DataFrame> CatalogNoteAccessor::indicate(
    std::string_view delimiter,
    std::string prefix) const {
  std::unordered_map<std::string, pluss::table::DataFrame> out;
  out.reserve(keysel_.keys_.size());
  for (const auto& key : keysel_.keys_) out.emplace(key, keysel_.base_.keys(key).nt().indicate(delimiter, prefix));
  return out;
}

Catalog CatalogNoteAccessor::replace(std::string_view from, std::string_view to) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.nt().replace(from, to); });
}

Catalog CatalogNoteAccessor::replace(std::string_view from,
                                     std::string_view to,
                                     const pluss::table::TextOptions& options) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.nt().replace(from, to, options); });
}

CatalogTimelineAccessor::CatalogTimelineAccessor(Catalog base, std::vector<std::string> keys)
    : keysel_(std::move(base), std::move(keys)) {}

#define PLUSS_CATALOG_TL_UNARY(method) \
  Catalog CatalogTimelineAccessor::method() const { \
    return keysel_.map_selected_([](const Stream& stream) { return stream.tl().method(); }); \
  }

PLUSS_CATALOG_TL_UNARY(isnat)
PLUSS_CATALOG_TL_UNARY(valid)
PLUSS_CATALOG_TL_UNARY(year)
PLUSS_CATALOG_TL_UNARY(month)
PLUSS_CATALOG_TL_UNARY(day)
PLUSS_CATALOG_TL_UNARY(hour)
PLUSS_CATALOG_TL_UNARY(minute)
PLUSS_CATALOG_TL_UNARY(second)
PLUSS_CATALOG_TL_UNARY(weekday)
PLUSS_CATALOG_TL_UNARY(yearday)
PLUSS_CATALOG_TL_UNARY(quarter)
PLUSS_CATALOG_TL_UNARY(week)
PLUSS_CATALOG_TL_UNARY(weekend)
PLUSS_CATALOG_TL_UNARY(month_start)
PLUSS_CATALOG_TL_UNARY(month_end)
PLUSS_CATALOG_TL_UNARY(leap)
PLUSS_CATALOG_TL_UNARY(weekday_name)
PLUSS_CATALOG_TL_UNARY(month_name)
PLUSS_CATALOG_TL_UNARY(iso)
PLUSS_CATALOG_TL_UNARY(date)
PLUSS_CATALOG_TL_UNARY(time)
PLUSS_CATALOG_TL_UNARY(utc)

#undef PLUSS_CATALOG_TL_UNARY

Catalog CatalogTimelineAccessor::fmt(std::string_view pattern) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.tl().fmt(pattern); });
}

Catalog CatalogTimelineAccessor::epoch(pluss::table::TimeUnit unit) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.tl().epoch(unit); });
}

Catalog CatalogTimelineAccessor::floor(std::string_view unit) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.tl().floor(unit); });
}

Catalog CatalogTimelineAccessor::ceil(std::string_view unit) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.tl().ceil(unit); });
}

Catalog CatalogTimelineAccessor::round(std::string_view unit) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.tl().round(unit); });
}

Catalog CatalogTimelineAccessor::shift(std::int64_t periods, std::string_view freq) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.tl().shift(periods, freq); });
}

Catalog CatalogTimelineAccessor::period(std::string_view freq) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.tl().period(freq); });
}

Catalog CatalogTimelineAccessor::tz(std::string_view name) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.tl().tz(name); });
}

Catalog CatalogTimelineAccessor::localize(const pluss::table::TimelineLocalizeOptions& options) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.tl().localize(options); });
}

CatalogTimeSeriesAccessor::CatalogTimeSeriesAccessor(Catalog base, std::vector<std::string> keys)
    : keysel_(std::move(base), std::move(keys)) {}

#define PLUSS_CATALOG_TS_PERIODS(method) \
  Catalog CatalogTimeSeriesAccessor::method(std::int64_t periods) const { \
    return keysel_.map_selected_([&](const Stream& stream) { return stream.ts().method(periods); }); \
  }

PLUSS_CATALOG_TS_PERIODS(returns)
PLUSS_CATALOG_TS_PERIODS(log_returns)
PLUSS_CATALOG_TS_PERIODS(diff)
PLUSS_CATALOG_TS_PERIODS(lag)
PLUSS_CATALOG_TS_PERIODS(lead)

#undef PLUSS_CATALOG_TS_PERIODS

Catalog CatalogTimeSeriesAccessor::rolling(std::int64_t window,
                                           pluss::table::WindowStatistic statistic,
                                           const pluss::table::WindowOptions& options) const {
  return keysel_.map_selected_([&](const Stream& stream) { return stream.ts().rolling(window, statistic, options); });
}

CatalogGroupBuilder::CatalogGroupBuilder(Catalog base) : base_(std::move(base)) {}

std::unordered_map<std::string, Catalog> CatalogGroupBuilder::by(std::string_view key_field) const {
  PLUSS_CHECK(base_.defined(), "Catalog::group: undefined handle");
  const auto key = trim_copy(key_field);
  PLUSS_CHECK(!key.empty(), "Catalog::group().by: key field cannot be empty");
  PLUSS_CHECK(base_.has(key), "Catalog::group().by: unknown key '" + key + "'");

  const auto key_stream = base_.keys(key);
  const auto key_note = key_stream.to_slate().cast().note().as_note();
  const auto n = key_stream.size();

  std::unordered_map<std::string, std::vector<std::int64_t>> row_groups;
  row_groups.reserve(static_cast<std::size_t>(std::max<std::int64_t>(1, n / 8)));
  for (std::int64_t i = 0; i < n; ++i) {
    row_groups[std::string(key_note.at(i))].push_back(i);
  }

  std::unordered_map<std::string, Catalog> out;
  out.reserve(row_groups.size());
  for (const auto& [group_key, rows] : row_groups) {
    std::vector<std::pair<std::string, Stream>> grouped;
    grouped.reserve(static_cast<std::size_t>(base_.nkeys()));
    for (const auto& field : base_.keys()) {
      const auto stream = base_.keys(field);
      PLUSS_CHECK(stream.size() == n, "Catalog::group().by: key field requires aligned stream lengths");
      grouped.emplace_back(field, stream.take(rows));
    }
    out.emplace(group_key, Catalog(std::move(grouped)));
  }
  return out;
}
