#pragma once

#include "pluss/bi/session.h"

#include "BI_tools/browser_launch.h"
#include "pluss/codeh/codeh.h"
#include "pluss/pl.h"
#include "pluss/tils/aux/scalar_cast.h"
#include "pluss/tils/view/html_window.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace pluss::bi {

struct TileRender {
  std::string key{};
  LayoutSpec layout{};
  ViewKind kind{ViewKind::DetailTable};
  bool is_control{false};
  std::optional<View> view{};
  std::optional<Control> control{};
  std::optional<Filter> active_filter{};
  std::shared_ptr<pluss::pl::Studio> plot{};
  std::shared_ptr<pluss::table::DataFrame> table{};
  std::optional<pluss::table::ThreadValue> scalar{};
};

struct BoardRender {
  std::string key{};
  std::string title{};
  std::int64_t rows{1};
  std::int64_t cols{1};
  std::vector<TileRender> tiles{};
  std::shared_ptr<pluss::pl::Studio> native{};
};

struct CacheStats {
  std::size_t query_hits{0};
  std::size_t query_misses{0};
  std::size_t tile_hits{0};
  std::size_t tile_misses{0};
  std::size_t board_hits{0};
  std::size_t board_misses{0};
};

struct StudioRender {
  std::string key{};
  Session session{};
  std::vector<BoardRender> boards{};
  std::shared_ptr<pluss::pl::Studio> native{};
  CacheStats cache{};
  std::map<std::string, std::vector<std::string>> dependencies{};
};

namespace detail {

[[nodiscard]] inline std::string dashboard_html_(
    const StudioRender& rendered,
    const pluss::bundle::Nexus& nexus,
    std::string_view title,
    std::int64_t width,
    std::int64_t height,
    std::string_view svg_payload = {});

// Studio::show()'s runtime host selector. `BackendPreference::Auto` -- the
// default -- now means "open a real OS browser tab" rather than "pick a
// native WebKit/WebView2 backend automatically": browser hosting is the
// primary PlussBI runtime (see ARCHITECTURE.md). Explicitly requesting
// `NativeWebView`/`PlussWebView`/`PlussWebViewOnly` still routes through
// PlussTilsView's native windowing for callers that want it.
[[nodiscard]] inline bool launch_html_(
    const std::string& html, const pluss::view::HtmlWindowOptions& options) {
  if (options.backend_preference != pluss::view::BackendPreference::Auto) {
    return pluss::view::show_html_window(html, options);
  }
  const auto path = pluss::bi_tools::open_html_in_browser(html, options.title);
  return !path.empty();
}

[[nodiscard]] inline bool frame_has_feature_(
    const pluss::table::DataFrame& frame,
    std::string_view feature) {
  const auto& features = frame.features();
  return std::find(features.begin(), features.end(), std::string(feature)) != features.end();
}

[[nodiscard]] inline std::vector<std::string_view> string_views_(
    const std::vector<std::string>& values) {
  std::vector<std::string_view> out;
  out.reserve(values.size());
  for (const auto& value : values) {
    out.push_back(value);
  }
  return out;
}

[[nodiscard]] inline std::vector<std::string> numeric_feature_names_excluding_(
    const pluss::table::DataFrame& frame,
    std::string_view excluded) {
  const auto is_numeric_series = [](const pluss::Series& series) {
    const auto dtype = series.dtype();
    return dtype.scalar == pluss::codem::ScalarType::Bool ||
           dtype.is_integral() || dtype.is_floating();
  };
  std::vector<std::string> out;
  const auto names = frame.features();
  out.reserve(names.size());
  for (const auto& name : names) {
    if (name == excluded) {
      continue;
    }
    const auto column = frame.features(name);
    if (column.kind() == pluss::table::ThreadKind::Series &&
        is_numeric_series(column.as_series())) {
      out.push_back(name);
    }
  }
  return out;
}

[[nodiscard]] inline std::string join_strings_(
    const std::vector<std::string>& values,
    std::string_view sep = ",") {
  std::ostringstream out;
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i > 0) {
      out << sep;
    }
    out << values[i];
  }
  return out.str();
}

[[nodiscard]] inline pluss::Timeline scalar_timeline_(const FilterScalar& value) {
  if (std::holds_alternative<pluss::Timeline>(value)) {
    return std::get<pluss::Timeline>(value);
  }
  if (std::holds_alternative<pluss::TimeIndex>(value)) {
    return std::get<pluss::TimeIndex>(value).timeline();
  }
  if (std::holds_alternative<std::string>(value)) {
    return pluss::make_timeline(std::vector<std::string>{std::get<std::string>(value)});
  }
  PLUSS_FAIL("pluss::bi: temporal filter expected Timeline, TimeIndex, or ISO string");
}

[[nodiscard]] inline pluss::TimeIndex scalar_timeindex_(const FilterScalar& value) {
  if (std::holds_alternative<pluss::TimeIndex>(value)) {
    return std::get<pluss::TimeIndex>(value);
  }
  if (std::holds_alternative<pluss::Timeline>(value)) {
    return pluss::make_timeindex(std::get<pluss::Timeline>(value), false);
  }
  if (std::holds_alternative<std::string>(value)) {
    return pluss::make_timeindex(std::vector<std::string>{std::get<std::string>(value)}, false);
  }
  PLUSS_FAIL("pluss::bi: temporal filter expected TimeIndex, Timeline, or ISO string");
}

[[nodiscard]] inline std::string scalar_temporal_iso_(const FilterScalar& value) {
  if (std::holds_alternative<std::string>(value)) {
    return std::get<std::string>(value);
  }
  if (std::holds_alternative<pluss::Timeline>(value)) {
    const auto& timeline = std::get<pluss::Timeline>(value);
    PLUSS_CHECK(timeline.numel() == 1,
                "pluss::bi: temporal scalar filters require a size-1 Timeline bound");
    return std::string(timeline.to_note_iso().at(0));
  }
  if (std::holds_alternative<pluss::TimeIndex>(value)) {
    const auto& index = std::get<pluss::TimeIndex>(value);
    PLUSS_CHECK(index.size() == 1,
                "pluss::bi: temporal scalar filters require a size-1 TimeIndex bound");
    return std::string(index.to_note_iso().at(0));
  }
  PLUSS_FAIL("pluss::bi: temporal filter expected Timeline, TimeIndex, or ISO string");
}

[[nodiscard]] inline pluss::table::Thread apply_scalar_filter_(
    const pluss::table::Thread& column,
    FilterOp op,
    const FilterScalar& value) {
  switch (column.kind()) {
    case pluss::table::ThreadKind::Series: {
      PLUSS_CHECK(std::holds_alternative<double>(value),
                  "pluss::bi: numeric filter payload must be a double");
      const double scalar = std::get<double>(value);
      switch (op) {
        case FilterOp::Eq: return column == scalar;
        case FilterOp::Ne: return column != scalar;
        case FilterOp::Gt: return column > scalar;
        case FilterOp::Ge: return column >= scalar;
        case FilterOp::Lt: return column < scalar;
        case FilterOp::Le: return column <= scalar;
        default: break;
      }
      break;
    }
    case pluss::table::ThreadKind::Note: {
      PLUSS_CHECK(std::holds_alternative<std::string>(value),
                  "pluss::bi: text filter payload must be a string");
      const auto& scalar = std::get<std::string>(value);
      switch (op) {
        case FilterOp::Eq: return column == scalar;
        case FilterOp::Ne: return column != scalar;
        case FilterOp::Gt: return column > scalar;
        case FilterOp::Ge: return column >= scalar;
        case FilterOp::Lt: return column < scalar;
        case FilterOp::Le: return column <= scalar;
        default: break;
      }
      break;
    }
    case pluss::table::ThreadKind::Timeline: {
      const auto scalar = scalar_timeline_(value);
      switch (op) {
        case FilterOp::Eq: return column == scalar;
        case FilterOp::Ne: return column != scalar;
        case FilterOp::Gt: return column > scalar;
        case FilterOp::Ge: return column >= scalar;
        case FilterOp::Lt: return column < scalar;
        case FilterOp::Le: return column <= scalar;
        default: break;
      }
      break;
    }
    case pluss::table::ThreadKind::TimeIndex: {
      const auto scalar = scalar_timeindex_(value);
      switch (op) {
        case FilterOp::Eq: return column == scalar;
        case FilterOp::Ne: return column != scalar;
        case FilterOp::Gt: return column > scalar;
        case FilterOp::Ge: return column >= scalar;
        case FilterOp::Lt: return column < scalar;
        case FilterOp::Le: return column <= scalar;
        default: break;
      }
      break;
    }
  }
  PLUSS_FAIL("pluss::bi: unsupported scalar filter operator");
}

[[nodiscard]] inline pluss::table::Thread apply_between_filter_(
    const pluss::table::Thread& column,
    const FilterRange& range) {
  switch (column.kind()) {
    case pluss::table::ThreadKind::Series:
      PLUSS_CHECK(std::holds_alternative<double>(range.low) &&
                      std::holds_alternative<double>(range.high),
                  "pluss::bi: numeric between filter requires double bounds");
      return (column >= std::get<double>(range.low)) &
             (column <= std::get<double>(range.high));
    case pluss::table::ThreadKind::Timeline:
      return column.between(scalar_temporal_iso_(range.low), scalar_temporal_iso_(range.high), true);
    case pluss::table::ThreadKind::TimeIndex:
      return column.between(scalar_temporal_iso_(range.low), scalar_temporal_iso_(range.high), true);
    case pluss::table::ThreadKind::Note:
      PLUSS_FAIL("pluss::bi: between filter is not supported for text features");
  }
  PLUSS_FAIL("pluss::bi: unsupported between filter payload");
}

[[nodiscard]] inline pluss::table::Thread apply_set_filter_(
    const pluss::table::Thread& column,
    const FilterSet& set_value) {
  if (std::holds_alternative<std::vector<double>>(set_value)) {
    PLUSS_CHECK(column.kind() == pluss::table::ThreadKind::Series,
                "pluss::bi: numeric set filter requires a numeric feature");
    const auto& values = std::get<std::vector<double>>(set_value);
    PLUSS_CHECK(!values.empty(), "pluss::bi: set filter cannot be empty");
    auto mask = column == values.front();
    for (std::size_t i = 1; i < values.size(); ++i) {
      mask = mask | (column == values[i]);
    }
    return mask;
  }

  PLUSS_CHECK(column.kind() == pluss::table::ThreadKind::Note,
              "pluss::bi: text set filter requires a text feature");
  const auto& values = std::get<std::vector<std::string>>(set_value);
  PLUSS_CHECK(!values.empty(), "pluss::bi: set filter cannot be empty");
  auto mask = column == values.front();
  for (std::size_t i = 1; i < values.size(); ++i) {
    mask = mask | (column == values[i]);
  }
  return mask;
}

[[nodiscard]] inline pluss::table::Thread apply_filter_(
    const pluss::table::Thread& column,
    const Filter& filter) {
  if (std::holds_alternative<FilterScalar>(filter.payload)) {
    return apply_scalar_filter_(column, filter.op, std::get<FilterScalar>(filter.payload));
  }
  if (std::holds_alternative<FilterSet>(filter.payload)) {
    PLUSS_CHECK(filter.op == FilterOp::In,
                "pluss::bi: set payloads require FilterOp::In");
    return apply_set_filter_(column, std::get<FilterSet>(filter.payload));
  }
  if (std::holds_alternative<FilterRange>(filter.payload)) {
    PLUSS_CHECK(filter.op == FilterOp::Between,
                "pluss::bi: range payloads require FilterOp::Between");
    return apply_between_filter_(column, std::get<FilterRange>(filter.payload));
  }
  PLUSS_FAIL("pluss::bi: filter payload is missing");
}

[[nodiscard]] inline bool filter_scope_matches_(
    const Filter& filter,
    const std::optional<std::string>& query_scope) {
  if (!filter.scope.has_value()) {
    return true;
  }
  return query_scope.has_value() && filter.scope == query_scope;
}

[[nodiscard]] inline pluss::table::DataFrame apply_filter_list_(
    const pluss::table::DataFrame& frame,
    const std::vector<Filter>& filters,
    const std::optional<std::string>& query_scope,
    bool strict_missing,
    bool strict_scoped_missing) {
  auto out = frame;
  for (const auto& filter : filters) {
    if (!filter.enabled) {
      continue;
    }
    if (!filter_scope_matches_(filter, query_scope)) {
      continue;
    }
    const bool exists = frame_has_feature_(out, filter.feature);
    if (!exists) {
      if (strict_missing || (strict_scoped_missing && filter.scope.has_value())) {
        PLUSS_FAIL("pluss::bi: filter '" + filter.key +
                   "' targets missing feature '" + filter.feature + "'");
      }
      continue;
    }
    out = out.filter(apply_filter_(out.features(filter.feature), filter));
  }
  return out;
}

[[nodiscard]] inline pluss::table::DataFrame resolve_query_(
    const pluss::bundle::Nexus& nexus,
    const QuerySpec& query,
    const Session& session) {
  PLUSS_CHECK(
      query.mode == QueryMode::Sql ? !query.statement.empty() : !query.source.empty(),
      "pluss::bi: query source/statement cannot be empty");

  pluss::table::DataFrame out;
  switch (query.mode) {
    case QueryMode::Source:
      out = nexus.frame(query.source);
      break;
    case QueryMode::StoredPlan:
      out = nexus.run(query.source);
      break;
    case QueryMode::Sql:
      out = nexus.query().execute(query.statement);
      break;
    case QueryMode::Traverse: {
      auto traverse = nexus.from(query.source);
      for (const auto& step : query.traverse) {
        if (step.kind == TraverseStep::Kind::Join) {
          traverse = step.join_spec.has_value()
              ? traverse.join(step.target, *step.join_spec)
              : traverse.join(step.target);
        } else {
          traverse = traverse.via(step.target, step.how);
        }
      }
      out = traverse.collect();
      break;
    }
  }

  out = apply_filter_list_(out, query.filters, query.scope, true, true);

  std::vector<Filter> session_filters;
  session_filters.reserve(session.filters().size());
  for (const auto& [_, value] : session.filters()) {
    session_filters.push_back(value);
  }
  out = apply_filter_list_(out, session_filters, query.scope, false, true);

  if (query.group_plan.has_value()) {
    const auto& plan = *query.group_plan;
    PLUSS_CHECK(!plan.keys.empty(),
                "pluss::bi: grouped query requires at least one key");
    PLUSS_CHECK(!plan.specs.empty(),
                "pluss::bi: grouped query requires at least one GroupSpec");
    pluss::table::DataFrame::GroupOptions options;
    options.dropnull = plan.dropnull;
    options.sort = plan.sort;
    const auto key_views = string_views_(plan.keys);
    out = out.group(options)
              .by(pluss::msharp::RangeView::Of<std::string_view>(key_views))
              .agg(pluss::msharp::RangeView::Of<pluss::table::DataFrame::GroupSpec>(
                  std::span<const pluss::table::DataFrame::GroupSpec>(
                      plan.specs.data(),
                      plan.specs.size())));
  }

  if (query.pivot_plan.has_value()) {
    const auto& plan = *query.pivot_plan;
    PLUSS_CHECK(!plan.value_features.empty(),
                "pluss::bi: pivot query requires at least one value feature");
    const auto index_views = string_views_(plan.index_features);
    const auto column_views = string_views_(plan.column_features);
    const auto value_views = string_views_(plan.value_features);
    out = out.pivot_table(
        pluss::msharp::RangeView::Of<std::string_view>(index_views),
        pluss::msharp::RangeView::Of<std::string_view>(column_views),
        pluss::msharp::RangeView::Of<std::string_view>(value_views),
        plan.op,
        plan.dropna,
        false,
        plan.sort);
  }

  if (query.resample_plan.has_value()) {
    const auto& plan = *query.resample_plan;
    auto value_features = !plan.value_features.empty()
        ? plan.value_features
        : numeric_feature_names_excluding_(out, plan.time_feature);
    PLUSS_CHECK(!value_features.empty(),
                "pluss::bi: resample query requires at least one numeric value feature");

    std::vector<std::string> selected_features;
    selected_features.reserve(value_features.size() + 1);
    selected_features.push_back(plan.time_feature);
    selected_features.insert(selected_features.end(), value_features.begin(), value_features.end());
    const auto selected_views = string_views_(selected_features);
    out = out.features(pluss::msharp::RangeView::Of<std::string_view>(selected_views))
              .ts()
              .resample(plan.time_feature, plan.freq, plan.op, plan.step);
  }

  if (!query.sort_specs.empty()) {
    std::vector<pluss::table::DataFrame::ArrangeSpec> specs;
    specs.reserve(query.sort_specs.size());
    for (const auto& spec : query.sort_specs) {
      PLUSS_CHECK(!spec.feature.empty(),
                  "pluss::bi: sort feature name cannot be empty");
      specs.push_back(pluss::table::DataFrame::ArrangeSpec{
          .feature = spec.feature,
          .ascending = spec.ascending,
          .nulls_last = spec.nulls_last,
      });
    }
    out = out.arrange(pluss::msharp::RangeView::Of<pluss::table::DataFrame::ArrangeSpec>(specs));
  }

  if (!query.select_features.empty()) {
    const auto views = string_views_(query.select_features);
    out = out.features(pluss::msharp::RangeView::Of<std::string_view>(views));
  }
  if (!query.drop_features.empty()) {
    const auto views = string_views_(query.drop_features);
    out = out.drop(pluss::msharp::RangeView::Of<std::string_view>(views));
  }
  if (query.head_n.has_value()) {
    out = out.head(*query.head_n);
  }
  if (query.tail_n.has_value()) {
    out = out.tail(*query.tail_n);
  }
  if (query.title.has_value()) {
    out = out.title(*query.title);
  }
  return out;
}

[[nodiscard]] inline pluss::table::ThreadValue reduce_kpi_(
    const pluss::table::DataFrame& frame,
    const BindSpec& bind) {
  PLUSS_CHECK(bind.feature.has_value(),
              "pluss::bi: KPI bindings require a target feature");
  const auto column = frame.features(*bind.feature);
  switch (bind.op) {
    case pluss::table::GroupOp::Count: return column.count(true);
    case pluss::table::GroupOp::Ndistinct: return column.ndistinct(true);
    case pluss::table::GroupOp::Sum: return column.sum();
    case pluss::table::GroupOp::Mean: return column.mean();
    case pluss::table::GroupOp::Median: return column.median();
    case pluss::table::GroupOp::Min: return column.min();
    case pluss::table::GroupOp::Max: return column.max();
    case pluss::table::GroupOp::Std: return column.std(true);
    case pluss::table::GroupOp::Var: return column.var(true);
    case pluss::table::GroupOp::Mode: return pluss::table::ThreadValue(column.mode());
  }
  PLUSS_FAIL("pluss::bi: unsupported KPI reducer");
}

[[nodiscard]] inline std::string scalar_text_(const pluss::table::ThreadValue& value) {
  try {
    return std::to_string(pluss::tils::as_f64(value.as_series()));
  } catch (...) {
  }
  try {
    const auto note = value.as_note();
    if (note.numel() > 0) {
      return std::string(note.at(0));
    }
  } catch (...) {
  }
  try {
    const auto timeline = value.as_timeline();
    if (timeline.numel() > 0) {
      return std::string(timeline.to_note_iso().at(0));
    }
  } catch (...) {
  }
  try {
    const auto index = value.as_timeindex();
    if (index.size() > 0) {
      return std::string(index.to_note_iso().at(0));
    }
  } catch (...) {
  }
  return "<value>";
}

[[nodiscard]] inline std::string placeholder_caption_(const TileRender& tile) {
  std::ostringstream out;
  if (tile.is_control && tile.control.has_value()) {
    const auto label = tile.control->label().empty() ? tile.control->key() : tile.control->label();
    out << "Control: " << label << "\nfeature: " << tile.control->feature();
    if (tile.active_filter.has_value()) {
      out << "\nactive: " << tile.active_filter->key;
    } else {
      out << "\nactive: none";
    }
    return out.str();
  }

  if (tile.view.has_value()) {
    const auto& bind = tile.view->bind();
    const auto title = bind.title.empty() ? tile.view->key() : bind.title;
    out << title;
    if (tile.kind == ViewKind::Kpi && tile.scalar.has_value()) {
      out << "\n" << scalar_text_(*tile.scalar);
    } else if (tile.table != nullptr) {
      out << "\n" << tile.table->ncases() << " cases x "
          << tile.table->nfeatures() << " feat";
    }
    return out.str();
  }

  out << tile.key;
  return out.str();
}

[[nodiscard]] inline bool is_plot_view_kind_(ViewKind kind) {
  return kind == ViewKind::Line ||
         kind == ViewKind::Bar ||
         kind == ViewKind::Scatter ||
         kind == ViewKind::Heatmap ||
         kind == ViewKind::Corr;
}

[[nodiscard]] inline pluss::table::DataFrame select_analysis_features_(
    const pluss::table::DataFrame& frame,
    const std::vector<std::string>& features,
    std::string_view where) {
  if (features.empty()) {
    return frame;
  }
  const auto views = string_views_(features);
  return frame.features(pluss::msharp::RangeView::Of<std::string_view>(views));
}

[[nodiscard]] inline pluss::Series implicit_positions_series_(std::int64_t size) {
  std::vector<double> values(static_cast<std::size_t>(size));
  for (std::int64_t i = 0; i < size; ++i) {
    values[static_cast<std::size_t>(i)] = static_cast<double>(i);
  }
  return pluss::make_series(std::move(values), {size});
}

[[nodiscard]] inline pluss::Series pack_numeric_columns_(
    const std::vector<pluss::Series>& columns,
    std::string_view where) {
  PLUSS_CHECK(!columns.empty(), std::string(where) + ": no numeric columns to pack");
  const auto rows = columns.front().numel();
  auto head = columns.front().to(pluss::codem::ScalarType::Float64).reshape({rows, 1});
  if (columns.size() == 1) {
    return head;
  }
  std::vector<pluss::Series> tail;
  tail.reserve(columns.size() - 1);
  for (std::size_t i = 1; i < columns.size(); ++i) {
    PLUSS_CHECK(columns[i].numel() == rows,
                std::string(where) + ": column length mismatch while packing matrix");
    tail.push_back(columns[i].to(pluss::codem::ScalarType::Float64).reshape({rows, 1}));
  }
  return head.unite(tail, 1);
}

inline void render_plot_tile_into_pane_(
    pluss::pl::Pane& pane,
    const pluss::table::DataFrame& frame,
    const BindSpec& bind) {
  switch (bind.kind) {
    case ViewKind::Line:
      (void)pluss::pl::flash::lineplot(pane, frame, bind.columns);
      return;
    case ViewKind::Bar: {
      pluss::pl::flash::BarPlotOptions options;
      options.horizontal = bind.horizontal;
      (void)pluss::pl::flash::barplot(pane, frame, bind.columns, options);
      return;
    }
    case ViewKind::Scatter:
      PLUSS_CHECK(!bind.columns.hue_feature.has_value(),
                  "pluss::bi: scatter hue binding is not yet available on board views");
      (void)pluss::pl::flash::scatterplot(pane, frame, bind.columns);
      return;
    case ViewKind::Heatmap: {
      const auto heatmap_frame =
          select_analysis_features_(frame, bind.analysis_features, "pluss::bi: heatmap");
      (void)pluss::pl::flash::heatmap(pane, heatmap_frame, bind.columns);
      return;
    }
    case ViewKind::Corr: {
      const auto corr_source =
          select_analysis_features_(frame, bind.analysis_features, "pluss::bi: corr");
      const pluss::table::DataFrame corr_frame = corr_source.corr(false);
      const auto names = corr_frame.features();
      PLUSS_CHECK(!names.empty(),
                  "pluss::bi: corr view requires at least one numeric feature");
      std::vector<pluss::Series> columns;
      columns.reserve(names.size());
      for (const auto& name : names) {
        columns.push_back(
            corr_frame.features(name).as_series().to(pluss::codem::ScalarType::Float64).reshape({corr_frame.ncases()}));
      }
      const auto axis = implicit_positions_series_(static_cast<std::int64_t>(names.size()));
      const auto z = pack_numeric_columns_(columns, "pluss::bi: corr");
      const auto label_views = string_views_(names);
      (void)pluss::pl::flash::heatmap(
          pane,
          axis,
          axis,
          z,
          {});
      pane.x_ticks(axis.to_vec<double>(), pluss::msharp::RangeView::Of<std::string_view>(label_views))
          .y_ticks(axis.to_vec<double>(), pluss::msharp::RangeView::Of<std::string_view>(label_views));
      pane.x_label("feature").y_label("feature");
      return;
    }
    default:
      break;
  }
  PLUSS_FAIL("pluss::bi: unsupported plot binding");
}

inline void mount_tile_into_native_(
    pluss::pl::Studio& studio,
    const TileRender& tile,
    std::int64_t row_offset = 0) {
  const auto pane_title = [&]() -> std::string {
    if (tile.is_control && tile.control.has_value()) {
      return tile.control->label().empty() ? tile.control->key() : tile.control->label();
    }
    if (tile.view.has_value()) {
      const auto& bind = tile.view->bind();
      return bind.title.empty() ? tile.view->key() : bind.title;
    }
    return tile.key;
  }();

  auto& pane = studio.pane2(pane_title);
  pane.place(
      row_offset + tile.layout.row,
      tile.layout.col,
      tile.layout.row_span,
      tile.layout.col_span);

  if (tile.view.has_value() &&
      tile.table != nullptr &&
      is_plot_view_kind_(tile.kind)) {
    render_plot_tile_into_pane_(pane, *tile.table, tile.view->bind());
    return;
  }

  pane.grid(pluss::pl::GridStyle::None)
      .axes_visible(false)
      .caption(placeholder_caption_(tile));
}

[[nodiscard]] inline std::shared_ptr<pluss::pl::Studio> compose_board_native_(
    const BoardRender& board) {
  auto out = std::make_shared<pluss::pl::Studio>(
      board.title.empty() ? board.key : board.title);
  out->mosaic(board.rows, board.cols);
  out->tight_layout(true);
  for (const auto& tile : board.tiles) {
    mount_tile_into_native_(*out, tile);
  }
  return out;
}

[[nodiscard]] inline std::shared_ptr<pluss::pl::Studio> compose_studio_native_(
    std::string_view key,
    const std::vector<BoardRender>& boards) {
  if (boards.empty()) {
    return std::make_shared<pluss::pl::Studio>(std::string(key));
  }

  std::int64_t total_rows = 0;
  std::int64_t max_cols = 1;
  for (const auto& board : boards) {
    total_rows += std::max<std::int64_t>(1, board.rows);
    max_cols = std::max(max_cols, board.cols);
  }

  auto out = std::make_shared<pluss::pl::Studio>(std::string(key));
  out->mosaic(total_rows, max_cols);
  out->tight_layout(true);

  std::int64_t row_offset = 0;
  for (const auto& board : boards) {
    out->row_title(row_offset, board.title.empty() ? board.key : board.title);
    for (const auto& tile : board.tiles) {
      mount_tile_into_native_(*out, tile, row_offset);
    }
    row_offset += std::max<std::int64_t>(1, board.rows);
  }
  return out;
}

[[nodiscard]] inline std::string encode_filter_scalar_(const FilterScalar& value) {
  if (std::holds_alternative<double>(value)) {
    std::ostringstream out;
    out << std::get<double>(value);
    return out.str();
  }
  if (std::holds_alternative<std::string>(value)) {
    return std::get<std::string>(value);
  }
  return scalar_temporal_iso_(value);
}

[[nodiscard]] inline std::string encode_filter_(const Filter& filter) {
  std::ostringstream out;
  out << filter.key << '|' << filter.feature << '|' << static_cast<int>(filter.op)
      << '|' << filter.enabled << '|';
  if (filter.scope.has_value()) {
    out << *filter.scope;
  }
  out << '|';
  if (std::holds_alternative<FilterScalar>(filter.payload)) {
    out << encode_filter_scalar_(std::get<FilterScalar>(filter.payload));
  } else if (std::holds_alternative<FilterSet>(filter.payload)) {
    const auto& set_value = std::get<FilterSet>(filter.payload);
    if (std::holds_alternative<std::vector<double>>(set_value)) {
      const auto& values = std::get<std::vector<double>>(set_value);
      for (const auto value : values) {
        out << value << ',';
      }
    } else {
      const auto& values = std::get<std::vector<std::string>>(set_value);
      for (const auto& value : values) {
        out << value << ',';
      }
    }
  } else if (std::holds_alternative<FilterRange>(filter.payload)) {
    const auto& range = std::get<FilterRange>(filter.payload);
    out << encode_filter_scalar_(range.low) << ".." << encode_filter_scalar_(range.high);
  }
  return out.str();
}

[[nodiscard]] inline std::string query_signature_(const QuerySpec& query) {
  std::ostringstream out;
  out << static_cast<int>(query.mode) << '\n'
      << query.source << '\n'
      << query.statement << '\n';
  for (const auto& step : query.traverse) {
    out << static_cast<int>(step.kind) << '|'
        << step.target << '|'
        << static_cast<int>(step.how) << '|';
    if (step.join_spec.has_value()) {
      for (const auto& key : step.join_spec->keys) {
        out << key.first << '=' << key.second << ';';
      }
    }
    out << '\n';
  }
  for (const auto& filter : query.filters) {
    out << encode_filter_(filter) << '\n';
  }
  for (const auto& sort : query.sort_specs) {
    out << sort.feature << '|' << sort.ascending << '|' << sort.nulls_last << '\n';
  }
  out << join_strings_(query.select_features) << '\n'
      << join_strings_(query.drop_features) << '\n'
      << (query.group_plan.has_value() ? "group" : "") << '\n';
  if (query.group_plan.has_value()) {
    const auto& plan = *query.group_plan;
    out << join_strings_(plan.keys) << '\n'
        << plan.dropnull << '|' << plan.sort << '\n';
    for (const auto& spec : plan.specs) {
      out << spec.output << '|' << spec.feature << '|' << static_cast<int>(spec.op) << '\n';
    }
  }
  out << (query.pivot_plan.has_value() ? "pivot" : "") << '\n';
  if (query.pivot_plan.has_value()) {
    const auto& plan = *query.pivot_plan;
    out << join_strings_(plan.index_features) << '\n'
        << join_strings_(plan.column_features) << '\n'
        << join_strings_(plan.value_features) << '\n'
        << static_cast<int>(plan.op) << '|' << plan.dropna << '|' << plan.sort << '\n';
  }
  out << (query.resample_plan.has_value() ? "resample" : "") << '\n';
  if (query.resample_plan.has_value()) {
    const auto& plan = *query.resample_plan;
    out << plan.time_feature << '\n'
        << join_strings_(plan.value_features) << '\n'
        << plan.freq << '\n'
        << static_cast<int>(plan.op) << '|' << plan.step << '\n';
  }
  out
      << (query.head_n.has_value() ? std::to_string(*query.head_n) : std::string()) << '\n'
      << (query.tail_n.has_value() ? std::to_string(*query.tail_n) : std::string()) << '\n'
      << (query.scope.has_value() ? *query.scope : std::string()) << '\n'
      << (query.title.has_value() ? *query.title : std::string()) << '\n';
  return out.str();
}

[[nodiscard]] inline std::string bind_signature_(const BindSpec& bind) {
  std::ostringstream out;
  out << static_cast<int>(bind.kind) << '\n'
      << bind.title << '\n'
      << (bind.feature.has_value() ? *bind.feature : std::string()) << '\n'
      << (bind.columns.feature.has_value() ? *bind.columns.feature : std::string()) << '\n'
      << (bind.columns.x_feature.has_value() ? *bind.columns.x_feature : std::string()) << '\n'
      << (bind.columns.y_feature.has_value() ? *bind.columns.y_feature : std::string()) << '\n'
      << (bind.columns.z_feature.has_value() ? *bind.columns.z_feature : std::string()) << '\n'
      << bind.horizontal << '\n'
      << static_cast<int>(bind.op) << '\n'
      << join_strings_(bind.index_features) << '\n'
      << join_strings_(bind.column_features) << '\n'
      << join_strings_(bind.value_features) << '\n'
      << bind.dropna << '\n'
      << bind.sort << '\n';
  return out.str();
}

[[nodiscard]] inline std::string control_signature_(const Control& control) {
  std::ostringstream out;
  out << control.key() << '\n'
      << control.label() << '\n'
      << control.feature() << '\n'
      << static_cast<int>(control.kind()) << '\n'
      << (control.scope().has_value() ? *control.scope() : std::string()) << '\n'
      << control.layout().row << '|'
      << control.layout().col << '|'
      << control.layout().row_span << '|'
      << control.layout().col_span << '\n';
  return out.str();
}

[[nodiscard]] inline std::string view_signature_(const View& view) {
  std::ostringstream out;
  out << view.key() << '\n'
      << query_signature_(view.query()) << '\n'
      << bind_signature_(view.bind()) << '\n'
      << view.layout().row << '|'
      << view.layout().col << '|'
      << view.layout().row_span << '|'
      << view.layout().col_span << '\n';
  return out.str();
}

[[nodiscard]] inline std::string board_signature_(const Board& board, std::uint64_t session_version) {
  std::ostringstream out;
  out << board.key() << '\n'
      << board.title() << '\n'
      << board.nrows() << '|'
      << board.ncols() << '|'
      << board.version() << '|'
      << session_version << '\n';
  for (const auto& tile : board.tiles()) {
    out << tile.key() << '\n';
  }
  return out.str();
}

[[nodiscard]] inline std::string tile_id_(std::string_view board_key, std::string_view tile_key) {
  return std::string(board_key) + "/" + std::string(tile_key);
}

[[nodiscard]] inline std::string tile_cache_key_(
    std::string_view board_key,
    const Tile& tile,
    std::uint64_t board_version,
    std::uint64_t session_version) {
  std::ostringstream out;
  out << board_key << '\n'
      << board_version << '\n'
      << session_version << '\n'
      << tile.key() << '\n'
      << tile.layout().row << '|'
      << tile.layout().col << '|'
      << tile.layout().row_span << '|'
      << tile.layout().col_span << '\n';
  if (tile.view().has_value()) {
    out << view_signature_(*tile.view());
  } else if (tile.control().has_value()) {
    out << control_signature_(*tile.control());
  }
  return out.str();
}

[[nodiscard]] inline bool tile_matches_filter_(
    const TileRender& tile,
    const Filter& filter) {
  if (tile.is_control && tile.control.has_value()) {
    if (tile.control->key() == filter.key) {
      return true;
    }
    if (tile.control->feature() != filter.feature) {
      return false;
    }
    if (tile.control->scope().has_value()) {
      return filter.scope.has_value() && tile.control->scope() == filter.scope;
    }
    return true;
  }

  if (!tile.view.has_value() || tile.table == nullptr) {
    return false;
  }
  const auto& query_scope = tile.view->query().scope;
  if (filter.scope.has_value()) {
    if (!query_scope.has_value() || query_scope != filter.scope) {
      return false;
    }
  }
  return frame_has_feature_(*tile.table, filter.feature);
}

inline void validate_tile_(const Tile& tile, std::int64_t rows, std::int64_t cols) {
  PLUSS_CHECK(!tile.key().empty(), "pluss::bi: tile key cannot be empty");
  PLUSS_CHECK(tile.layout().row >= 0 && tile.layout().col >= 0,
              "pluss::bi: tile coordinates cannot be negative");
  PLUSS_CHECK(tile.layout().row_span > 0 && tile.layout().col_span > 0,
              "pluss::bi: tile spans must be positive");
  PLUSS_CHECK(tile.layout().row + tile.layout().row_span <= rows,
              "pluss::bi: tile row span exceeds board shape");
  PLUSS_CHECK(tile.layout().col + tile.layout().col_span <= cols,
              "pluss::bi: tile column span exceeds board shape");
  const bool has_view = tile.view().has_value();
  const bool has_control = tile.control().has_value();
  PLUSS_CHECK(has_view != has_control,
              "pluss::bi: each tile must contain exactly one view or one control");
}

inline void validate_board_(const Board& board) {
  PLUSS_CHECK(!board.key().empty(), "pluss::bi: board key cannot be empty");
  PLUSS_CHECK(board.nrows() > 0 && board.ncols() > 0,
              "pluss::bi: board shape must be positive");
  std::set<std::string> seen;
  for (const auto& tile : board.tiles()) {
    PLUSS_CHECK(seen.insert(tile.key()).second,
                "pluss::bi: duplicate tile key '" + tile.key() + "'");
    validate_tile_(tile, board.nrows(), board.ncols());
  }
}

} // namespace detail

class Studio {
public:
  Studio() = default;
  explicit Studio(std::string_view key)
      : key_(key) {}

  [[nodiscard]] const std::string& key() const noexcept {
    return key_;
  }

  [[nodiscard]] const Session& session() const noexcept {
    return session_;
  }

  Studio& session(const Session& value) {
    session_ = value;
    invalidate_all_caches_();
    return *this;
  }

  Studio& source(const pluss::bundle::Nexus& nexus) {
    nexus_ = &nexus;
    invalidate_all_caches_();
    return *this;
  }

  Board& board(std::string_view key) {
    const auto it = std::find_if(
        boards_.begin(),
        boards_.end(),
        [&](const Board& board) { return board.key() == key; });
    if (it != boards_.end()) {
      return *it;
    }
    boards_.emplace_back(key);
    invalidate_all_caches_();
    return boards_.back();
  }

  Studio& set_filter(Filter value) {
    const auto filter_key = value.key;
    session_.set_filter(std::move(value));
    invalidate_for_filter_key_(filter_key);
    return *this;
  }

  Studio& clear_filter(std::string_view key) {
    session_.clear_filter(key);
    invalidate_for_filter_key_(key);
    return *this;
  }

  Studio& refresh() {
    invalidate_all_caches_();
    return *this;
  }

  [[nodiscard]] const std::vector<Board>& boards() const noexcept {
    return boards_;
  }

  [[nodiscard]] StudioRender render() const {
    PLUSS_CHECK(nexus_ != nullptr, "pluss::bi::Studio::render: source Nexus has not been set");

    StudioRender out;
    out.key = key_;
    out.session = session_;

    for (const auto& board : boards_) {
      detail::validate_board_(board);

      BoardRender board_out;
      board_out.key = board.key();
      board_out.title = board.title();
      board_out.rows = board.nrows();
      board_out.cols = board.ncols();
      board_out.tiles.reserve(board.tiles().size());

      for (const auto& tile : board.tiles()) {
        board_out.tiles.push_back(render_tile_cached_(board, tile, out.cache));
      }

      const auto board_key = detail::board_signature_(board, 0) +
          current_filter_signature_for_board_(board.key());
      if (const auto it = board_native_cache_.find(board_key); it != board_native_cache_.end()) {
        board_out.native = it->second;
        ++out.cache.board_hits;
      } else {
        board_out.native = detail::compose_board_native_(board_out);
        board_native_cache_[board_key] = board_out.native;
        board_cache_owner_[board_key] = board.key();
        ++out.cache.board_misses;
      }

      out.boards.push_back(std::move(board_out));
    }

    out.native = detail::compose_studio_native_(key_, out.boards);
    rebuild_dependency_graph_(out);
    normalize_cache_keys_(out);
    return out;
  }

  bool show(
      std::string_view title = {},
      std::int64_t width = 1280,
      std::int64_t height = 820,
      bool launch = true) const {
    const auto rendered = render();
    PLUSS_CHECK(rendered.native != nullptr,
                "pluss::bi::Studio::show: native board studio was not created");
    if (!launch) {
      return true;
    }
    pluss::view::HtmlWindowOptions options{};
    options.title = std::string(title.empty() ? key_ : title);
    options.width = width;
    options.height = height;
    options.svg_payload = rendered.native->to_svg();
    options.backend_preference = pluss::view::BackendPreference::Auto;
    const auto html = detail::dashboard_html_(
        rendered,
        *nexus_,
        title.empty() ? key_ : title,
        width,
        height,
        options.svg_payload);
    return detail::launch_html_(html, options);
  }

  bool show(
      const pluss::view::HtmlWindowOptions& options,
      bool launch = true) const {
    const auto rendered = render();
    PLUSS_CHECK(rendered.native != nullptr,
                "pluss::bi::Studio::show: native board studio was not created");
    if (!launch) {
      return true;
    }
    auto effective = options;
    if (effective.title.empty()) {
      effective.title = key_;
    }
    if (effective.width <= 0) {
      effective.width = 1280;
    }
    if (effective.height <= 0) {
      effective.height = 820;
    }
    if (effective.svg_payload.empty()) {
      effective.svg_payload = rendered.native->to_svg();
    }
    const auto html = detail::dashboard_html_(
        rendered,
        *nexus_,
        effective.title,
        effective.width,
        effective.height,
        effective.svg_payload);
    return detail::launch_html_(html, effective);
  }

  void save(std::string_view path) const;
  static Studio load(std::string_view path);
  static Studio load(std::string_view path, const pluss::bundle::Nexus& nexus);
  Studio& restore_session(std::string_view path);

  void export_html(
      std::string_view path,
      std::string_view title = {},
      std::int64_t width = 1280,
      std::int64_t height = 820) const;
  void export_svg(std::string_view path) const;
  void export_pdf(
      std::string_view path,
      pluss::pl::PdfEngine engine = pluss::pl::PdfEngine::Auto) const;
  void export_png(std::string_view path, std::int64_t dpi = 144) const;

private:
  [[nodiscard]] std::string current_filter_signature_for_keys_(
      const std::vector<std::string>& keys) const {
    if (keys.empty()) {
      return {};
    }
    std::ostringstream out;
    for (const auto& key : keys) {
      if (const auto it = session_.filters().find(key); it != session_.filters().end()) {
        out << detail::encode_filter_(it->second) << '\n';
      }
    }
    return out.str();
  }

  [[nodiscard]] std::string current_filter_signature_for_tile_(
      std::string_view tile_id) const {
    if (const auto it = reverse_dependency_graph_.find(std::string(tile_id));
        it != reverse_dependency_graph_.end()) {
      return current_filter_signature_for_keys_(it->second);
    }
    return {};
  }

  [[nodiscard]] std::string current_filter_signature_for_board_(
      std::string_view board_key) const {
    if (const auto it = board_dependency_graph_.find(std::string(board_key));
        it != board_dependency_graph_.end()) {
      return current_filter_signature_for_keys_(it->second);
    }
    return {};
  }

  [[nodiscard]] std::shared_ptr<pluss::table::DataFrame> resolve_query_cached_(
      std::string_view tile_id,
      const QuerySpec& query,
      CacheStats& stats) const {
    (void)tile_id;
    const auto cache_key = detail::query_signature_(query) +
        "\n" +
        std::to_string(session_.version());
    if (const auto it = query_cache_.find(cache_key); it != query_cache_.end()) {
      ++stats.query_hits;
      return it->second;
    }

    auto frame = std::make_shared<pluss::table::DataFrame>(
        detail::resolve_query_(*nexus_, query, session_));
    query_cache_[cache_key] = frame;
    ++stats.query_misses;
    return frame;
  }

  [[nodiscard]] TileRender render_tile_cached_(
      const Board& board,
      const Tile& tile,
      CacheStats& stats) const {
    const auto tile_id = detail::tile_id_(board.key(), tile.key());
    const auto cache_key = detail::tile_cache_key_(
        board.key(),
        tile,
        board.version(),
        0) + current_filter_signature_for_tile_(tile_id);
    if (const auto it = tile_cache_.find(cache_key); it != tile_cache_.end()) {
      ++stats.tile_hits;
      return *it->second;
    }

    TileRender out;
    out.key = tile.key();
    out.layout = tile.layout();

    if (tile.view().has_value()) {
      const auto& view = *tile.view();
      const auto frame = resolve_query_cached_(tile_id, view.query(), stats);
      out.kind = view.bind().kind;
      out.view = view;
      out.table = frame;

      if (view.bind().kind == ViewKind::Kpi) {
        out.scalar = detail::reduce_kpi_(*frame, view.bind());
      } else if (view.bind().kind == ViewKind::PivotTable) {
        const auto index_views = detail::string_views_(view.bind().index_features);
        const auto column_views = detail::string_views_(view.bind().column_features);
        const auto value_views = detail::string_views_(view.bind().value_features);
        pluss::table::DataFrame materialized = frame->pivot_table(
            pluss::msharp::RangeView::Of<std::string_view>(index_views),
            pluss::msharp::RangeView::Of<std::string_view>(column_views),
            pluss::msharp::RangeView::Of<std::string_view>(value_views),
            view.bind().op,
            view.bind().dropna,
            false,
            view.bind().sort);
        out.table = std::make_shared<pluss::table::DataFrame>(std::move(materialized));
      } else if (detail::is_plot_view_kind_(view.bind().kind)) {
        auto plot = std::make_shared<pluss::pl::Studio>(
            view.bind().title.empty() ? view.key() : view.bind().title);
        auto& pane = plot->pane2(view.bind().title.empty() ? view.key() : view.bind().title);
        detail::render_plot_tile_into_pane_(pane, *out.table, view.bind());
        out.plot = std::move(plot);
      }
    } else if (tile.control().has_value()) {
      out.kind = ViewKind::DetailTable;
      out.is_control = true;
      out.control = *tile.control();
      if (const auto active = session_.filter(tile.control()->key()); active.has_value()) {
        out.active_filter = std::move(active);
      } else {
        for (const auto& [_, value] : session_.filters()) {
          if (value.feature != tile.control()->feature()) {
            continue;
          }
          if (tile.control()->scope().has_value()) {
            if (!value.scope.has_value() || value.scope != tile.control()->scope()) {
              continue;
            }
          }
          out.active_filter = value;
          break;
        }
      }
    } else {
      PLUSS_FAIL("pluss::bi: tile must contain a view or a control");
    }

    tile_cache_[cache_key] = std::make_shared<TileRender>(out);
    tile_cache_owner_[cache_key] = tile_id;
    ++stats.tile_misses;
    return out;
  }

  void rebuild_dependency_graph_(StudioRender& render) const {
    dependency_graph_.clear();
    reverse_dependency_graph_.clear();
    board_dependency_graph_.clear();
    render.dependencies.clear();

    for (const auto& [filter_key, filter] : render.session.filters()) {
      auto& entries = render.dependencies[filter_key];
      for (const auto& board : render.boards) {
        for (const auto& tile : board.tiles) {
          if (detail::tile_matches_filter_(tile, filter)) {
            entries.push_back(detail::tile_id_(board.key, tile.key));
          }
        }
      }
      std::sort(entries.begin(), entries.end());
      entries.erase(std::unique(entries.begin(), entries.end()), entries.end());
      dependency_graph_[filter_key] = entries;
      for (const auto& tile_id : entries) {
        reverse_dependency_graph_[tile_id].push_back(filter_key);
        const auto pos = tile_id.find('/');
        if (pos != std::string::npos) {
          board_dependency_graph_[tile_id.substr(0, pos)].push_back(filter_key);
        }
      }
    }

    for (auto& [tile_id, keys] : reverse_dependency_graph_) {
      std::sort(keys.begin(), keys.end());
      keys.erase(std::unique(keys.begin(), keys.end()), keys.end());
    }
    for (auto& [board_key, keys] : board_dependency_graph_) {
      std::sort(keys.begin(), keys.end());
      keys.erase(std::unique(keys.begin(), keys.end()), keys.end());
    }
  }

  void normalize_cache_keys_(const StudioRender& render) const {
    const auto board_count = std::min(render.boards.size(), boards_.size());
    for (std::size_t board_index = 0; board_index < board_count; ++board_index) {
      const auto& board = boards_[board_index];
      const auto& board_render = render.boards[board_index];
      const auto board_key = detail::board_signature_(board, 0) +
          current_filter_signature_for_board_(board.key());
      if (board_render.native != nullptr) {
        board_native_cache_[board_key] = board_render.native;
        board_cache_owner_[board_key] = board.key();
      }

      const auto tile_count = std::min(board.tiles().size(), board_render.tiles.size());
      for (std::size_t tile_index = 0; tile_index < tile_count; ++tile_index) {
        const auto& tile = board.tiles()[tile_index];
        const auto& tile_render = board_render.tiles[tile_index];
        const auto tile_id = detail::tile_id_(board.key(), tile.key());
        const auto tile_key = detail::tile_cache_key_(
            board.key(),
            tile,
            board.version(),
            0) + current_filter_signature_for_tile_(tile_id);
        tile_cache_[tile_key] = std::make_shared<TileRender>(tile_render);
        tile_cache_owner_[tile_key] = tile_id;
      }
    }
  }

  void invalidate_all_caches_() const {
    query_cache_.clear();
    tile_cache_.clear();
    board_native_cache_.clear();
    tile_cache_owner_.clear();
    board_cache_owner_.clear();
    dependency_graph_.clear();
    reverse_dependency_graph_.clear();
    board_dependency_graph_.clear();
  }

  void invalidate_for_filter_key_(std::string_view filter_key) const {
    query_cache_.clear();

    const auto it = dependency_graph_.find(std::string(filter_key));
    if (it == dependency_graph_.end()) {
      tile_cache_.clear();
      board_native_cache_.clear();
      tile_cache_owner_.clear();
      board_cache_owner_.clear();
      return;
    }

    const std::unordered_set<std::string> affected_tiles(it->second.begin(), it->second.end());
    std::unordered_set<std::string> affected_boards;
    for (const auto& tile_id : affected_tiles) {
      const auto pos = tile_id.find('/');
      if (pos != std::string::npos) {
        affected_boards.insert(tile_id.substr(0, pos));
      }
    }

    for (auto cache_it = tile_cache_.begin(); cache_it != tile_cache_.end();) {
      const auto owner_it = tile_cache_owner_.find(cache_it->first);
      if (owner_it != tile_cache_owner_.end() &&
          affected_tiles.find(owner_it->second) != affected_tiles.end()) {
        tile_cache_owner_.erase(cache_it->first);
        cache_it = tile_cache_.erase(cache_it);
      } else {
        ++cache_it;
      }
    }

    for (auto cache_it = board_native_cache_.begin(); cache_it != board_native_cache_.end();) {
      const auto owner_it = board_cache_owner_.find(cache_it->first);
      if (owner_it != board_cache_owner_.end() &&
          affected_boards.find(owner_it->second) != affected_boards.end()) {
        board_cache_owner_.erase(cache_it->first);
        cache_it = board_native_cache_.erase(cache_it);
      } else {
        ++cache_it;
      }
    }
  }

  std::string key_{"studio"};
  const pluss::bundle::Nexus* nexus_{nullptr};
  Session session_{};
  std::vector<Board> boards_{};

  mutable std::unordered_map<std::string, std::shared_ptr<pluss::table::DataFrame>> query_cache_{};
  mutable std::unordered_map<std::string, std::shared_ptr<TileRender>> tile_cache_{};
  mutable std::unordered_map<std::string, std::shared_ptr<pluss::pl::Studio>> board_native_cache_{};
  mutable std::unordered_map<std::string, std::string> tile_cache_owner_{};
  mutable std::unordered_map<std::string, std::string> board_cache_owner_{};
  mutable std::map<std::string, std::vector<std::string>> dependency_graph_{};
  mutable std::unordered_map<std::string, std::vector<std::string>> reverse_dependency_graph_{};
  mutable std::unordered_map<std::string, std::vector<std::string>> board_dependency_graph_{};
};

namespace detail {

struct PersistedTile {
  std::string key{};
  std::optional<Control> control{};
  std::optional<View> view{};
};

struct PersistedBoard {
  std::string key{};
  std::string title{};
  std::int64_t rows{1};
  std::int64_t cols{1};
  std::vector<PersistedTile> tiles{};
};

struct PersistedStudio {
  std::string key{"studio"};
  Session session{};
  std::vector<PersistedBoard> boards{};
};

[[nodiscard]] inline bool persist_safe_char_(unsigned char ch) {
  return std::isalnum(ch) != 0 || ch == '-' || ch == '_' || ch == '.' || ch == '~';
}

[[nodiscard]] inline std::string encode_atom_(std::string_view value) {
  std::ostringstream out;
  out << std::uppercase << std::hex;
  for (const unsigned char ch : value) {
    if (persist_safe_char_(ch)) {
      out << static_cast<char>(ch);
    } else {
      out << '%' << std::setw(2) << std::setfill('0')
          << static_cast<int>(ch);
    }
  }
  return out.str();
}

[[nodiscard]] inline std::string decode_atom_(std::string_view value) {
  auto hex_value = [](char ch) -> int {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'A' && ch <= 'F') return 10 + (ch - 'A');
    if (ch >= 'a' && ch <= 'f') return 10 + (ch - 'a');
    return -1;
  };

  std::string out;
  out.reserve(value.size());
  for (std::size_t i = 0; i < value.size(); ++i) {
    const char ch = value[i];
    if (ch != '%') {
      out.push_back(ch);
      continue;
    }
    PLUSS_CHECK(i + 2 < value.size(),
                "pluss::bi::Studio::load: malformed escaped field");
    const int hi = hex_value(value[i + 1]);
    const int lo = hex_value(value[i + 2]);
    PLUSS_CHECK(hi >= 0 && lo >= 0,
                "pluss::bi::Studio::load: malformed escaped field");
    out.push_back(static_cast<char>((hi << 4) | lo));
    i += 2;
  }
  return out;
}

[[nodiscard]] inline std::vector<std::string> split_preserve_empty_(
    std::string_view text,
    char sep) {
  std::vector<std::string> out;
  std::size_t start = 0;
  while (start <= text.size()) {
    const auto pos = text.find(sep, start);
    if (pos == std::string_view::npos) {
      out.emplace_back(text.substr(start));
      break;
    }
    out.emplace_back(text.substr(start, pos - start));
    start = pos + 1;
  }
  return out;
}

[[nodiscard]] inline std::string encode_list_(const std::vector<std::string>& values) {
  std::ostringstream out;
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i > 0) {
      out << '~';
    }
    out << encode_atom_(values[i]);
  }
  return out.str();
}

[[nodiscard]] inline std::vector<std::string> decode_list_(std::string_view encoded) {
  if (encoded.empty()) {
    return {};
  }
  const auto parts = split_preserve_empty_(encoded, '~');
  std::vector<std::string> out;
  out.reserve(parts.size());
  for (const auto& part : parts) {
    out.push_back(decode_atom_(part));
  }
  return out;
}

[[nodiscard]] inline std::string encode_rows_(const std::vector<std::vector<std::string>>& rows) {
  std::ostringstream out;
  for (std::size_t i = 0; i < rows.size(); ++i) {
    if (i > 0) {
      out << '^';
    }
    out << encode_list_(rows[i]);
  }
  return out.str();
}

[[nodiscard]] inline std::vector<std::vector<std::string>> decode_rows_(
    std::string_view encoded) {
  if (encoded.empty()) {
    return {};
  }
  const auto parts = split_preserve_empty_(encoded, '^');
  std::vector<std::vector<std::string>> out;
  out.reserve(parts.size());
  for (const auto& part : parts) {
    out.push_back(decode_list_(part));
  }
  return out;
}

inline void append_record_(
    std::vector<std::string>& lines,
    std::string_view tag,
    const std::vector<std::string>& encoded_fields) {
  std::ostringstream out;
  out << tag;
  for (const auto& field : encoded_fields) {
    out << '|' << field;
  }
  lines.push_back(out.str());
}

[[nodiscard]] inline std::string bool_token_(bool value) {
  return value ? "1" : "0";
}

[[nodiscard]] inline bool parse_bool_(std::string_view token, std::string_view where) {
  if (token == "1" || token == "true") {
    return true;
  }
  if (token == "0" || token == "false") {
    return false;
  }
  PLUSS_FAIL(std::string(where) + ": invalid bool token '" + std::string(token) + "'");
}

template <class Int>
[[nodiscard]] inline Int parse_int_(std::string_view token, std::string_view where) {
  Int out{};
  const auto* begin = token.data();
  const auto* end = token.data() + token.size();
  const auto result = std::from_chars(begin, end, out);
  PLUSS_CHECK(result.ec == std::errc{} && result.ptr == end,
              std::string(where) + ": invalid integer token '" + std::string(token) + "'");
  return out;
}

[[nodiscard]] inline double parse_double_(std::string_view token, std::string_view where) {
  std::size_t used = 0;
  const double value = std::stod(std::string(token), &used);
  PLUSS_CHECK(used == token.size(),
              std::string(where) + ": invalid double token '" + std::string(token) + "'");
  return value;
}

[[nodiscard]] inline std::optional<std::string> parse_optional_atom_(std::string_view token) {
  if (token.empty()) {
    return std::nullopt;
  }
  return decode_atom_(token);
}

[[nodiscard]] inline std::string encode_optional_atom_(const std::optional<std::string>& value) {
  return value.has_value() ? encode_atom_(*value) : std::string{};
}

[[nodiscard]] inline std::string encode_optional_int_(const std::optional<std::int64_t>& value) {
  return value.has_value() ? std::to_string(*value) : std::string{};
}

[[nodiscard]] inline std::optional<std::int64_t> parse_optional_int_(
    std::string_view token,
    std::string_view where) {
  if (token.empty()) {
    return std::nullopt;
  }
  return parse_int_<std::int64_t>(token, where);
}

[[nodiscard]] inline std::string format_double_(double value) {
  std::ostringstream out;
  out << std::setprecision(17) << value;
  return out.str();
}

[[nodiscard]] inline std::string encode_filter_op_(FilterOp op) {
  switch (op) {
    case FilterOp::Eq: return "eq";
    case FilterOp::Ne: return "ne";
    case FilterOp::Gt: return "gt";
    case FilterOp::Ge: return "ge";
    case FilterOp::Lt: return "lt";
    case FilterOp::Le: return "le";
    case FilterOp::In: return "in";
    case FilterOp::Between: return "between";
  }
  PLUSS_FAIL("pluss::bi: unsupported FilterOp");
}

[[nodiscard]] inline FilterOp decode_filter_op_(std::string_view token) {
  if (token == "eq") return FilterOp::Eq;
  if (token == "ne") return FilterOp::Ne;
  if (token == "gt") return FilterOp::Gt;
  if (token == "ge") return FilterOp::Ge;
  if (token == "lt") return FilterOp::Lt;
  if (token == "le") return FilterOp::Le;
  if (token == "in") return FilterOp::In;
  if (token == "between") return FilterOp::Between;
  PLUSS_FAIL("pluss::bi::Studio::load: unsupported FilterOp '" + std::string(token) + "'");
}

[[nodiscard]] inline std::string encode_query_mode_(QueryMode mode) {
  switch (mode) {
    case QueryMode::Source: return "source";
    case QueryMode::StoredPlan: return "stored";
    case QueryMode::Sql: return "sql";
    case QueryMode::Traverse: return "traverse";
  }
  PLUSS_FAIL("pluss::bi: unsupported QueryMode");
}

[[nodiscard]] inline QueryMode decode_query_mode_(std::string_view token) {
  if (token == "source") return QueryMode::Source;
  if (token == "stored") return QueryMode::StoredPlan;
  if (token == "sql") return QueryMode::Sql;
  if (token == "traverse") return QueryMode::Traverse;
  PLUSS_FAIL("pluss::bi::Studio::load: unsupported QueryMode '" + std::string(token) + "'");
}

[[nodiscard]] inline std::string encode_control_kind_(ControlKind kind) {
  switch (kind) {
    case ControlKind::Category: return "category";
    case ControlKind::NumericRange: return "numeric_range";
    case ControlKind::TemporalRange: return "temporal_range";
  }
  PLUSS_FAIL("pluss::bi: unsupported ControlKind");
}

[[nodiscard]] inline ControlKind decode_control_kind_(std::string_view token) {
  if (token == "category") return ControlKind::Category;
  if (token == "numeric_range") return ControlKind::NumericRange;
  if (token == "temporal_range") return ControlKind::TemporalRange;
  PLUSS_FAIL("pluss::bi::Studio::load: unsupported ControlKind '" + std::string(token) + "'");
}

[[nodiscard]] inline std::string encode_view_kind_(ViewKind kind) {
  switch (kind) {
    case ViewKind::Kpi: return "kpi";
    case ViewKind::Line: return "line";
    case ViewKind::Bar: return "bar";
    case ViewKind::DetailTable: return "detail_table";
    case ViewKind::PivotTable: return "pivot_table";
    case ViewKind::Scatter: return "scatter";
    case ViewKind::Heatmap: return "heatmap";
    case ViewKind::Corr: return "corr";
  }
  PLUSS_FAIL("pluss::bi: unsupported ViewKind");
}

[[nodiscard]] inline ViewKind decode_view_kind_(std::string_view token) {
  if (token == "kpi") return ViewKind::Kpi;
  if (token == "line") return ViewKind::Line;
  if (token == "bar") return ViewKind::Bar;
  if (token == "detail_table") return ViewKind::DetailTable;
  if (token == "pivot_table") return ViewKind::PivotTable;
  if (token == "scatter") return ViewKind::Scatter;
  if (token == "heatmap") return ViewKind::Heatmap;
  if (token == "corr") return ViewKind::Corr;
  PLUSS_FAIL("pluss::bi::Studio::load: unsupported ViewKind '" + std::string(token) + "'");
}

[[nodiscard]] inline std::string encode_group_op_(pluss::table::GroupOp op) {
  switch (op) {
    case pluss::table::GroupOp::Count: return "count";
    case pluss::table::GroupOp::Ndistinct: return "ndistinct";
    case pluss::table::GroupOp::Sum: return "sum";
    case pluss::table::GroupOp::Mean: return "mean";
    case pluss::table::GroupOp::Median: return "median";
    case pluss::table::GroupOp::Min: return "min";
    case pluss::table::GroupOp::Max: return "max";
    case pluss::table::GroupOp::Std: return "std";
    case pluss::table::GroupOp::Var: return "var";
    case pluss::table::GroupOp::Mode: return "mode";
  }
  PLUSS_FAIL("pluss::bi: unsupported GroupOp");
}

[[nodiscard]] inline pluss::table::GroupOp decode_group_op_(std::string_view token) {
  if (token == "count") return pluss::table::GroupOp::Count;
  if (token == "ndistinct") return pluss::table::GroupOp::Ndistinct;
  if (token == "sum") return pluss::table::GroupOp::Sum;
  if (token == "mean") return pluss::table::GroupOp::Mean;
  if (token == "median") return pluss::table::GroupOp::Median;
  if (token == "min") return pluss::table::GroupOp::Min;
  if (token == "max") return pluss::table::GroupOp::Max;
  if (token == "std") return pluss::table::GroupOp::Std;
  if (token == "var") return pluss::table::GroupOp::Var;
  if (token == "mode") return pluss::table::GroupOp::Mode;
  PLUSS_FAIL("pluss::bi::Studio::load: unsupported GroupOp '" + std::string(token) + "'");
}

[[nodiscard]] inline std::string encode_resample_op_(
    pluss::subsp::stats::ts::ResampleOp op) {
  switch (op) {
    case pluss::subsp::stats::ts::ResampleOp::Mean: return "mean";
    case pluss::subsp::stats::ts::ResampleOp::Sum: return "sum";
    case pluss::subsp::stats::ts::ResampleOp::Min: return "min";
    case pluss::subsp::stats::ts::ResampleOp::Max: return "max";
    case pluss::subsp::stats::ts::ResampleOp::First: return "first";
    case pluss::subsp::stats::ts::ResampleOp::Last: return "last";
    case pluss::subsp::stats::ts::ResampleOp::Count: return "count";
  }
  PLUSS_FAIL("pluss::bi: unsupported ResampleOp");
}

[[nodiscard]] inline pluss::subsp::stats::ts::ResampleOp decode_resample_op_(
    std::string_view token) {
  if (token == "mean") return pluss::subsp::stats::ts::ResampleOp::Mean;
  if (token == "sum") return pluss::subsp::stats::ts::ResampleOp::Sum;
  if (token == "min") return pluss::subsp::stats::ts::ResampleOp::Min;
  if (token == "max") return pluss::subsp::stats::ts::ResampleOp::Max;
  if (token == "first") return pluss::subsp::stats::ts::ResampleOp::First;
  if (token == "last") return pluss::subsp::stats::ts::ResampleOp::Last;
  if (token == "count") return pluss::subsp::stats::ts::ResampleOp::Count;
  PLUSS_FAIL("pluss::bi::Studio::load: unsupported ResampleOp '" + std::string(token) + "'");
}

[[nodiscard]] inline std::string encode_traverse_kind_(TraverseStep::Kind kind) {
  switch (kind) {
    case TraverseStep::Kind::Join: return "join";
    case TraverseStep::Kind::Via: return "via";
  }
  PLUSS_FAIL("pluss::bi: unsupported TraverseStep::Kind");
}

[[nodiscard]] inline TraverseStep::Kind decode_traverse_kind_(std::string_view token) {
  if (token == "join") return TraverseStep::Kind::Join;
  if (token == "via") return TraverseStep::Kind::Via;
  PLUSS_FAIL("pluss::bi::Studio::load: unsupported TraverseStep::Kind '" + std::string(token) + "'");
}

[[nodiscard]] inline std::string encode_join_how_(pluss::bundle::nexus::JoinHow how) {
  using JoinHow = pluss::bundle::nexus::JoinHow;
  switch (how) {
    case JoinHow::Inner: return "inner";
    case JoinHow::Left: return "left";
    case JoinHow::Right: return "right";
    case JoinHow::Outer: return "outer";
    case JoinHow::Cross: return "cross";
    case JoinHow::LeftOnly: return "left_only";
    case JoinHow::RightOnly: return "right_only";
  }
  PLUSS_FAIL("pluss::bi: unsupported JoinHow");
}

[[nodiscard]] inline pluss::bundle::nexus::JoinHow decode_join_how_(
    std::string_view token) {
  using JoinHow = pluss::bundle::nexus::JoinHow;
  if (token == "inner") return JoinHow::Inner;
  if (token == "left") return JoinHow::Left;
  if (token == "right") return JoinHow::Right;
  if (token == "outer") return JoinHow::Outer;
  if (token == "cross") return JoinHow::Cross;
  if (token == "left_only") return JoinHow::LeftOnly;
  if (token == "right_only") return JoinHow::RightOnly;
  PLUSS_FAIL("pluss::bi::Studio::load: unsupported JoinHow '" + std::string(token) + "'");
}

[[nodiscard]] inline std::string encode_cardinality_(
    pluss::bundle::nexus::RelationshipCardinality card) {
  using Card = pluss::bundle::nexus::RelationshipCardinality;
  switch (card) {
    case Card::OneToOne: return "one_to_one";
    case Card::OneToMany: return "one_to_many";
    case Card::ManyToOne: return "many_to_one";
    case Card::ManyToMany: return "many_to_many";
  }
  PLUSS_FAIL("pluss::bi: unsupported RelationshipCardinality");
}

[[nodiscard]] inline pluss::bundle::nexus::RelationshipCardinality decode_cardinality_(
    std::string_view token) {
  using Card = pluss::bundle::nexus::RelationshipCardinality;
  if (token == "one_to_one") return Card::OneToOne;
  if (token == "one_to_many") return Card::OneToMany;
  if (token == "many_to_one") return Card::ManyToOne;
  if (token == "many_to_many") return Card::ManyToMany;
  PLUSS_FAIL(
      "pluss::bi::Studio::load: unsupported RelationshipCardinality '" + std::string(token) +
      "'");
}

[[nodiscard]] inline std::string filter_scalar_type_token_(const FilterScalar& value) {
  if (std::holds_alternative<double>(value)) return "double";
  if (std::holds_alternative<std::string>(value)) return "text";
  if (std::holds_alternative<pluss::Timeline>(value)) return "timeline";
  if (std::holds_alternative<pluss::TimeIndex>(value)) return "timeindex";
  PLUSS_FAIL("pluss::bi: unsupported filter scalar payload");
}

[[nodiscard]] inline std::string filter_scalar_value_token_(const FilterScalar& value) {
  if (std::holds_alternative<double>(value)) return format_double_(std::get<double>(value));
  if (std::holds_alternative<std::string>(value)) return std::get<std::string>(value);
  if (std::holds_alternative<pluss::Timeline>(value)) return scalar_temporal_iso_(value);
  if (std::holds_alternative<pluss::TimeIndex>(value)) return scalar_temporal_iso_(value);
  PLUSS_FAIL("pluss::bi: unsupported filter scalar payload");
}

[[nodiscard]] inline FilterScalar decode_filter_scalar_(
    std::string_view type,
    std::string_view value,
    std::string_view where) {
  if (type == "double") {
    return FilterScalar(parse_double_(value, where));
  }
  if (type == "text") {
    return FilterScalar(std::string(value));
  }
  if (type == "timeline") {
    return FilterScalar(pluss::make_timeline(std::vector<std::string>{std::string(value)}));
  }
  if (type == "timeindex") {
    return FilterScalar(pluss::make_timeindex(std::vector<std::string>{std::string(value)}, false));
  }
  PLUSS_FAIL(std::string(where) + ": unsupported filter scalar type '" + std::string(type) + "'");
}

[[nodiscard]] inline std::vector<std::string> serialize_filter_fields_(const Filter& filter) {
  std::vector<std::string> out{
      filter.key,
      filter.feature,
      encode_filter_op_(filter.op),
      "none",
      "",
      "",
      "",
      "",
      filter.scope.value_or(""),
      bool_token_(filter.enabled),
  };

  if (std::holds_alternative<FilterScalar>(filter.payload)) {
    const auto& scalar = std::get<FilterScalar>(filter.payload);
    out[3] = "scalar";
    out[4] = filter_scalar_type_token_(scalar);
    out[5] = filter_scalar_value_token_(scalar);
  } else if (std::holds_alternative<FilterSet>(filter.payload)) {
    const auto& set_value = std::get<FilterSet>(filter.payload);
    out[3] = "set";
    if (std::holds_alternative<std::vector<double>>(set_value)) {
      out[4] = "double";
      std::vector<std::string> values;
      for (const auto value : std::get<std::vector<double>>(set_value)) {
        values.push_back(format_double_(value));
      }
      out[5] = encode_list_(values);
    } else {
      out[4] = "text";
      out[5] = encode_list_(std::get<std::vector<std::string>>(set_value));
    }
  } else if (std::holds_alternative<FilterRange>(filter.payload)) {
    const auto& range = std::get<FilterRange>(filter.payload);
    out[3] = "range";
    out[4] = filter_scalar_type_token_(range.low);
    out[5] = filter_scalar_value_token_(range.low);
    out[6] = filter_scalar_type_token_(range.high);
    out[7] = filter_scalar_value_token_(range.high);
  }

  return out;
}

[[nodiscard]] inline Filter deserialize_filter_fields_(
    const std::vector<std::string>& fields,
    std::string_view where) {
  PLUSS_CHECK(fields.size() == 10,
              std::string(where) + ": malformed filter payload");
  Filter out;
  out.key = fields[0];
  out.feature = fields[1];
  out.op = decode_filter_op_(fields[2]);
  const auto& payload_kind = fields[3];
  if (payload_kind == "none") {
    out.payload = std::monostate{};
  } else if (payload_kind == "scalar") {
    out.payload = decode_filter_scalar_(fields[4], fields[5], where);
  } else if (payload_kind == "set") {
    if (fields[4] == "double") {
      std::vector<double> values;
      for (const auto& token : decode_list_(fields[5])) {
        values.push_back(parse_double_(token, where));
      }
      out.payload = FilterSet(std::move(values));
    } else if (fields[4] == "text") {
      out.payload = FilterSet(decode_list_(fields[5]));
    } else {
      PLUSS_FAIL(std::string(where) + ": unsupported set filter type '" + fields[4] + "'");
    }
  } else if (payload_kind == "range") {
    out.payload = FilterRange{
        .low = decode_filter_scalar_(fields[4], fields[5], where),
        .high = decode_filter_scalar_(fields[6], fields[7], where),
    };
  } else {
    PLUSS_FAIL(std::string(where) + ": unsupported filter payload kind '" + payload_kind + "'");
  }
  if (!fields[8].empty()) {
    out.scope = fields[8];
  }
  out.enabled = parse_bool_(fields[9], where);
  return out;
}

[[nodiscard]] inline std::vector<std::vector<std::string>> serialize_sort_rows_(
    const std::vector<SortSpec>& specs) {
  std::vector<std::vector<std::string>> rows;
  rows.reserve(specs.size());
  for (const auto& spec : specs) {
    rows.push_back({spec.feature, bool_token_(spec.ascending), bool_token_(spec.nulls_last)});
  }
  return rows;
}

[[nodiscard]] inline std::vector<SortSpec> deserialize_sort_rows_(
    const std::vector<std::vector<std::string>>& rows,
    std::string_view where) {
  std::vector<SortSpec> out;
  out.reserve(rows.size());
  for (const auto& row : rows) {
    PLUSS_CHECK(row.size() == 3, std::string(where) + ": malformed sort row");
    out.push_back(SortSpec{
        .feature = row[0],
        .ascending = parse_bool_(row[1], where),
        .nulls_last = parse_bool_(row[2], where),
    });
  }
  return out;
}

[[nodiscard]] inline std::vector<std::vector<std::string>> serialize_group_spec_rows_(
    const std::vector<pluss::table::DataFrame::GroupSpec>& specs) {
  std::vector<std::vector<std::string>> rows;
  rows.reserve(specs.size());
  for (const auto& spec : specs) {
    rows.push_back({spec.output, spec.feature, encode_group_op_(spec.op)});
  }
  return rows;
}

[[nodiscard]] inline std::vector<pluss::table::DataFrame::GroupSpec> deserialize_group_spec_rows_(
    const std::vector<std::vector<std::string>>& rows,
    std::string_view where) {
  std::vector<pluss::table::DataFrame::GroupSpec> out;
  out.reserve(rows.size());
  for (const auto& row : rows) {
    PLUSS_CHECK(row.size() == 3, std::string(where) + ": malformed group row");
    out.push_back(pluss::table::DataFrame::GroupSpec{
        .output = row[0],
        .feature = row[1],
        .op = decode_group_op_(row[2]),
    });
  }
  return out;
}

[[nodiscard]] inline std::vector<std::vector<std::string>> serialize_traverse_rows_(
    const std::vector<TraverseStep>& steps) {
  std::vector<std::vector<std::string>> rows;
  rows.reserve(steps.size());
  for (const auto& step : steps) {
    std::vector<std::vector<std::string>> key_pairs;
    if (step.join_spec.has_value()) {
      for (const auto& [left, right] : step.join_spec->keys) {
        key_pairs.push_back({left, right});
      }
    }
    rows.push_back({
        encode_traverse_kind_(step.kind),
        step.target,
        encode_join_how_(step.how),
        step.join_spec.has_value() ? encode_join_how_(step.join_spec->how) : std::string{},
        step.join_spec.has_value() ? step.join_spec->right_suffix : std::string{},
        step.join_spec.has_value() && step.join_spec->validate.has_value()
            ? encode_cardinality_(*step.join_spec->validate)
            : std::string{},
        step.join_spec.has_value() && step.join_spec->relationship_name.has_value()
            ? *step.join_spec->relationship_name
            : std::string{},
        step.join_spec.has_value() && step.join_spec->result_name.has_value()
            ? *step.join_spec->result_name
            : std::string{},
        encode_rows_(key_pairs),
    });
  }
  return rows;
}

[[nodiscard]] inline std::vector<TraverseStep> deserialize_traverse_rows_(
    const std::vector<std::vector<std::string>>& rows,
    std::string_view where) {
  std::vector<TraverseStep> out;
  out.reserve(rows.size());
  for (const auto& row : rows) {
    PLUSS_CHECK(row.size() == 9, std::string(where) + ": malformed traverse row");
    TraverseStep step;
    step.kind = decode_traverse_kind_(row[0]);
    step.target = row[1];
    step.how = decode_join_how_(row[2]);

    if (!row[3].empty() || !row[4].empty() || !row[5].empty() || !row[6].empty() ||
        !row[7].empty() || !row[8].empty()) {
      pluss::bundle::nexus::JoinSpec spec;
      spec.how = row[3].empty()
          ? pluss::bundle::nexus::JoinHow::Inner
          : decode_join_how_(row[3]);
      spec.right_suffix = row[4];
      if (!row[5].empty()) {
        spec.validate = decode_cardinality_(row[5]);
      }
      if (!row[6].empty()) {
        spec.relationship_name = row[6];
      }
      if (!row[7].empty()) {
        spec.result_name = row[7];
      }
      for (const auto& pair_row : decode_rows_(row[8])) {
        PLUSS_CHECK(pair_row.size() == 2, std::string(where) + ": malformed join key pair");
        spec.keys.emplace_back(pair_row[0], pair_row[1]);
      }
      step.join_spec = std::move(spec);
    }

    out.push_back(std::move(step));
  }
  return out;
}

[[nodiscard]] inline std::filesystem::path normalize_bundle_path_(std::string_view path) {
  std::filesystem::path out(path);
  if (out.extension().empty()) {
    out += ".plussbi";
  }
  return out;
}

[[nodiscard]] inline std::filesystem::path normalize_html_path_(std::string_view path) {
  std::filesystem::path out(path);
  if (out.extension().empty()) {
    out += ".html";
  }
  return out;
}

inline void write_text_file_(const std::filesystem::path& path, std::string_view text) {
  std::error_code ec;
  if (path.has_parent_path()) {
    std::filesystem::create_directories(path.parent_path(), ec);
  }
  std::ofstream out(path, std::ios::out | std::ios::trunc | std::ios::binary);
  PLUSS_CHECK(out.is_open(),
              "pluss::bi::Studio::save: failed to open '" + path.string() + "'");
  out << text;
  out.close();
  PLUSS_CHECK(static_cast<bool>(out),
              "pluss::bi::Studio::save: failed while writing '" + path.string() + "'");
}

[[nodiscard]] inline std::string read_text_file_(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::in | std::ios::binary);
  PLUSS_CHECK(in.is_open(),
              "pluss::bi::Studio::load: failed to open '" + path.string() + "'");
  std::ostringstream buffer;
  buffer << in.rdbuf();
  PLUSS_CHECK(static_cast<bool>(in) || in.eof(),
              "pluss::bi::Studio::load: failed while reading '" + path.string() + "'");
  return buffer.str();
}

[[nodiscard]] inline std::string escape_html_(std::string_view text) {
  std::string out;
  out.reserve(text.size() + 32);
  for (const char ch : text) {
    switch (ch) {
      case '&': out += "&amp;"; break;
      case '<': out += "&lt;"; break;
      case '>': out += "&gt;"; break;
      case '"': out += "&quot;"; break;
      case '\'': out += "&#39;"; break;
      default: out.push_back(ch); break;
    }
  }
  return out;
}

[[nodiscard]] inline std::string studio_html_(
    const pluss::pl::Studio& native,
    std::string_view title,
    std::int64_t width,
    std::int64_t height) {
  const auto page_title = title.empty() ? std::string("PlussBI Studio") : std::string(title);
  const auto svg_payload = native.to_svg();
  return "<!doctype html>\n"
         "<html lang=\"en\">\n"
         "<head>\n"
         "  <meta charset=\"utf-8\" />\n"
         "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" />\n"
         "  <title>" + escape_html_(page_title) + "</title>\n"
         "  <style>\n"
         "    html, body { margin: 0; padding: 0; background: #eef4f8; min-height: 100%; }\n"
         "    body { font-family: Helvetica, Arial, sans-serif; color: #20242a; }\n"
         "    .shell { padding: 16px 18px 22px; }\n"
         "    .title { font-size: 18px; font-weight: 700; margin: 0 0 10px; }\n"
         "    .card { background: #ffffff; border: 1px solid #d7e2ea; border-radius: 12px; "
         "box-shadow: 0 14px 34px rgba(24, 38, 53, 0.08); overflow: auto; }\n"
         "    .svg-wrap { min-width: " + std::to_string(std::max<std::int64_t>(640, width)) +
         "px; min-height: " + std::to_string(std::max<std::int64_t>(420, height)) + "px; }\n"
         "    .svg-wrap svg { display: block; width: 100%; height: auto; }\n"
         "  </style>\n"
         "</head>\n"
         "<body>\n"
         "  <div class=\"shell\">\n"
         "    <div class=\"title\">" + escape_html_(page_title) + "</div>\n"
         "    <div class=\"card\"><div class=\"svg-wrap\">" + svg_payload + "</div></div>\n"
         "  </div>\n"
         "</body>\n"
         "</html>\n";
}

[[nodiscard]] inline std::string thread_scalar_text_at_(
    const pluss::table::Thread& thread,
    std::int64_t row) {
  if (!pluss::table::detail::valid_at(thread, row)) {
    return "NULL";
  }
  return scalar_text_(thread.slice(row, row + 1).min());
}

[[nodiscard]] inline std::optional<pluss::table::DataFrame> control_source_frame_(
    const pluss::bundle::Nexus& nexus,
    const Session& session,
    const Control& control) {
  const auto session_filters = session.filters();
  for (const auto& key : nexus.keys()) {
    auto frame = nexus.frame(key);
    if (!frame_has_feature_(frame, control.feature())) {
      continue;
    }
    std::vector<Filter> filters;
    filters.reserve(session_filters.size());
    for (const auto& [filter_key, filter] : session_filters) {
      if (filter_key == control.key()) {
        continue;
      }
      filters.push_back(filter);
    }
    return apply_filter_list_(frame, filters, control.scope(), false, false);
  }
  return std::nullopt;
}

[[nodiscard]] inline std::string filter_payload_summary_(
    const std::optional<Filter>& filter) {
  if (!filter.has_value()) {
    return "none";
  }
  const auto& value = *filter;
  if (std::holds_alternative<FilterScalar>(value.payload)) {
    return encode_filter_scalar_(std::get<FilterScalar>(value.payload));
  }
  if (std::holds_alternative<FilterRange>(value.payload)) {
    const auto& range = std::get<FilterRange>(value.payload);
    return encode_filter_scalar_(range.low) + " to " + encode_filter_scalar_(range.high);
  }
  if (std::holds_alternative<FilterSet>(value.payload)) {
    if (std::holds_alternative<std::vector<double>>(std::get<FilterSet>(value.payload))) {
      std::vector<std::string> tokens;
      for (const auto item : std::get<std::vector<double>>(std::get<FilterSet>(value.payload))) {
        tokens.push_back(format_double_(item));
      }
      return join_strings_(tokens, ", ");
    }
    return join_strings_(std::get<std::vector<std::string>>(std::get<FilterSet>(value.payload)), ", ");
  }
  return "none";
}

[[nodiscard]] inline std::string iframe_srcdoc_(std::string_view html) {
  return escape_html_(html);
}

[[nodiscard]] inline std::string tile_title_(const TileRender& tile) {
  if (tile.is_control && tile.control.has_value()) {
    return tile.control->label().empty() ? tile.control->key() : tile.control->label();
  }
  if (tile.view.has_value()) {
    const auto& bind = tile.view->bind();
    return bind.title.empty() ? tile.view->key() : bind.title;
  }
  return tile.key;
}

[[nodiscard]] inline std::int64_t tile_min_height_px_(const TileRender& tile) {
  if (tile.layout.min_height.has_value()) {
    return static_cast<std::int64_t>(std::llround(*tile.layout.min_height));
  }
  if (tile.kind == ViewKind::DetailTable || tile.kind == ViewKind::PivotTable) {
    return std::max<std::int64_t>(260, 220 * tile.layout.row_span);
  }
  if (tile.kind == ViewKind::Kpi || tile.is_control) {
    return std::max<std::int64_t>(140, 120 * tile.layout.row_span);
  }
  return std::max<std::int64_t>(220, 180 * tile.layout.row_span);
}

[[nodiscard]] inline std::string tile_frame_html_(
    const pluss::table::DataFrame& frame,
    std::string_view title) {
  pluss::table::display::DisplayOptions options{};
  options.window_title = std::string(title);
  options.viewer_width = 960;
  options.viewer_height = 560;
  options.html_sortable = true;
  options.html_search = true;
  options.html_copy_button = true;
  options.html_desktop_table_viewport = true;
  return pluss::table::display::render_html(frame, options);
}

[[nodiscard]] inline std::string render_table_tile_html_(
    const TileRender& tile) {
  PLUSS_CHECK(tile.table != nullptr,
              "pluss::bi: table tile expected materialized DataFrame");
  const auto title = tile_title_(tile);
  const auto html = tile_frame_html_(*tile.table, title);
  return "<div class=\"bi-table-shell\"><iframe class=\"bi-table-frame\" "
         "loading=\"lazy\" srcdoc=\"" + iframe_srcdoc_(html) + "\"></iframe></div>";
}

[[nodiscard]] inline std::string render_kpi_tile_html_(
    const TileRender& tile) {
  const auto title = escape_html_(tile_title_(tile));
  const auto value = tile.scalar.has_value() ? escape_html_(scalar_text_(*tile.scalar)) : std::string("NULL");
  std::ostringstream out;
  out << "<div class=\"bi-kpi\">"
      << "<div class=\"bi-kpi-label\">" << title << "</div>"
      << "<div class=\"bi-kpi-value\">" << value << "</div>";
  if (tile.table != nullptr) {
    out << "<div class=\"bi-kpi-meta\">" << tile.table->ncases() << " filtered cases</div>";
  }
  out << "</div>";
  return out.str();
}

[[nodiscard]] inline std::string render_plot_tile_html_(
    const TileRender& tile) {
  PLUSS_CHECK(tile.plot != nullptr,
              "pluss::bi: plot tile expected plot studio");
  return "<div class=\"bi-svg-wrap\">" + tile.plot->to_svg() + "</div>";
}

[[nodiscard]] inline std::string render_control_tile_html_(
    const pluss::bundle::Nexus& nexus,
    const Session& session,
    const TileRender& tile) {
  PLUSS_CHECK(tile.control.has_value(),
              "pluss::bi: control tile expected control spec");
  const auto& control = *tile.control;
  const auto source = control_source_frame_(nexus, session, control);
  const auto current = escape_html_(filter_payload_summary_(tile.active_filter));
  const auto title = escape_html_(tile_title_(tile));
  const auto feature = escape_html_(control.feature());
  const auto control_id = escape_html_(control.key());

  std::ostringstream out;
  out << "<div class=\"bi-control\" data-control-key=\"" << control_id << "\">"
      << "<div class=\"bi-control-head\">"
      << "<div><div class=\"bi-control-label\">" << title << "</div>"
      << "<div class=\"bi-control-feature\">" << feature << "</div></div>"
      << "<span class=\"bi-chip\" id=\"chip-" << control_id << "\">" << current << "</span>"
      << "</div>";

  if (!source.has_value()) {
    out << "<div class=\"bi-empty\">No source feature matched this control yet.</div></div>";
    return out.str();
  }

  const auto column = source->features(control.feature());
  switch (control.kind()) {
    case ControlKind::Category: {
      auto values = column.distinct(true);
      const auto option_count = std::min<std::int64_t>(values.size(), 200);
      out << "<label class=\"bi-field-label\" for=\"select-" << control_id << "\">Values</label>";
      out << "<select class=\"bi-select\" id=\"select-" << control_id << "\" "
          << "onchange=\"plussBiSyncSelect('" << control_id << "')\">";
      out << "<option value=\"\">All</option>";
      for (std::int64_t row = 0; row < option_count; ++row) {
        const auto value = thread_scalar_text_at_(values, row);
        const auto escaped = escape_html_(value);
        out << "<option value=\"" << escaped << "\">" << escaped << "</option>";
      }
      out << "</select>";
      if (values.size() > option_count) {
        out << "<div class=\"bi-help\">Showing the first " << option_count
            << " distinct values in the current filtered slice.</div>";
      } else {
        out << "<div class=\"bi-help\">" << values.size()
            << " distinct values in the current filtered slice.</div>";
      }
      break;
    }
    case ControlKind::NumericRange: {
      const auto low = scalar_text_(column.min());
      const auto high = scalar_text_(column.max());
      out << "<div class=\"bi-range-grid\">"
          << "<div><label class=\"bi-field-label\" for=\"low-" << control_id << "\">Min</label>"
          << "<input class=\"bi-input\" id=\"low-" << control_id
          << "\" type=\"number\" step=\"any\" value=\"" << escape_html_(low)
          << "\" oninput=\"plussBiSyncRange('" << control_id << "')\"></div>"
          << "<div><label class=\"bi-field-label\" for=\"high-" << control_id << "\">Max</label>"
          << "<input class=\"bi-input\" id=\"high-" << control_id
          << "\" type=\"number\" step=\"any\" value=\"" << escape_html_(high)
          << "\" oninput=\"plussBiSyncRange('" << control_id << "')\"></div>"
          << "</div>"
          << "<div class=\"bi-help\">Current data span: " << escape_html_(low)
          << " to " << escape_html_(high) << ".</div>";
      break;
    }
    case ControlKind::TemporalRange: {
      const auto low = scalar_text_(column.min());
      const auto high = scalar_text_(column.max());
      out << "<div class=\"bi-range-grid\">"
          << "<div><label class=\"bi-field-label\" for=\"low-" << control_id << "\">Start</label>"
          << "<input class=\"bi-input\" id=\"low-" << control_id
          << "\" type=\"text\" value=\"" << escape_html_(low)
          << "\" oninput=\"plussBiSyncRange('" << control_id << "')\"></div>"
          << "<div><label class=\"bi-field-label\" for=\"high-" << control_id << "\">End</label>"
          << "<input class=\"bi-input\" id=\"high-" << control_id
          << "\" type=\"text\" value=\"" << escape_html_(high)
          << "\" oninput=\"plussBiSyncRange('" << control_id << "')\"></div>"
          << "</div>"
          << "<div class=\"bi-help\">Use ISO-like temporal text bounds. Current span: "
          << escape_html_(low) << " to " << escape_html_(high) << ".</div>";
      break;
    }
  }

  out << "</div>";
  return out.str();
}

[[nodiscard]] inline std::string render_tile_html_(
    const pluss::bundle::Nexus& nexus,
    const Session& session,
    const TileRender& tile) {
  if (tile.is_control) {
    return render_control_tile_html_(nexus, session, tile);
  }
  if (tile.kind == ViewKind::Kpi) {
    return render_kpi_tile_html_(tile);
  }
  if (tile.kind == ViewKind::DetailTable || tile.kind == ViewKind::PivotTable) {
    return render_table_tile_html_(tile);
  }
  if (is_plot_view_kind_(tile.kind)) {
    return render_plot_tile_html_(tile);
  }
  return "<div class=\"bi-empty\">" + escape_html_(placeholder_caption_(tile)) + "</div>";
}

[[nodiscard]] inline std::string render_board_html_(
    const pluss::bundle::Nexus& nexus,
    const Session& session,
    const BoardRender& board) {
  std::ostringstream out;
  out << "<section class=\"bi-board\">"
      << "<div class=\"bi-board-head\">"
      << "<h2>" << escape_html_(board.title.empty() ? board.key : board.title) << "</h2>"
      << "<span class=\"bi-board-meta\">" << board.tiles.size() << " tiles</span>"
      << "</div>"
      << "<div class=\"bi-grid\" style=\"grid-template-columns:repeat("
      << std::max<std::int64_t>(1, board.cols) << ", minmax(0, 1fr));\">";
  for (const auto& tile : board.tiles) {
    out << "<article class=\"bi-tile\" style=\"grid-column:"
        << (tile.layout.col + 1) << " / span " << std::max<std::int64_t>(1, tile.layout.col_span)
        << ";grid-row:" << (tile.layout.row + 1) << " / span "
        << std::max<std::int64_t>(1, tile.layout.row_span)
        << ";min-height:" << tile_min_height_px_(tile) << "px;\">"
        << "<div class=\"bi-tile-title\">" << escape_html_(tile_title_(tile)) << "</div>"
        << render_tile_html_(nexus, session, tile)
        << "</article>";
  }
  out << "</div></section>";
  return out.str();
}

[[nodiscard]] inline std::string dashboard_html_(
    const StudioRender& rendered,
    const pluss::bundle::Nexus& nexus,
    std::string_view title,
    std::int64_t width,
    std::int64_t height,
    std::string_view svg_payload) {
  const auto page_title = title.empty() ? std::string("PlussBI Studio") : std::string(title);
  std::ostringstream body;
  body << "<div class=\"bi-shell\">"
       << "<header class=\"bi-topbar\">"
       << "<div><div class=\"bi-kicker\">PlussBI</div><h1>" << escape_html_(page_title) << "</h1></div>"
       << "<div class=\"bi-session\">";
  if (rendered.session.filters().empty()) {
    body << "<span class=\"bi-chip muted\">No active session filters</span>";
  } else {
    for (const auto& [key, filter] : rendered.session.filters()) {
      body << "<span class=\"bi-chip\">" << escape_html_(key) << ": "
           << escape_html_(filter_payload_summary_(filter)) << "</span>";
    }
  }
  body << "</div></header>";
  for (const auto& board : rendered.boards) {
    body << render_board_html_(nexus, rendered.session, board);
  }
  body << "</div>";

  std::ostringstream html;
  html << "<!doctype html>\n"
       << "<html lang=\"en\">\n"
       << "<head>\n"
       << "  <meta charset=\"utf-8\" />\n"
       << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" />\n"
       << "  <title>" << escape_html_(page_title) << "</title>\n"
       << "  <style>\n"
       << "    :root { color-scheme: light; --bg:#eef3f8; --card:#ffffff; --ink:#12212f; --muted:#627483; --line:#d8e2eb; --accent:#0b7e87; --chip:#e6f6f7; }\n"
       << "    * { box-sizing:border-box; }\n"
       << "    html, body { margin:0; padding:0; background:var(--bg); color:var(--ink); min-height:100%; }\n"
       << "    body { font-family: 'SF Pro Text', 'Segoe UI', Helvetica, Arial, sans-serif; }\n"
       << "    .bi-shell { width:min(100%, " << std::max<std::int64_t>(960, width) << "px); margin:0 auto; padding:20px; }\n"
       << "    .bi-topbar { display:flex; justify-content:space-between; gap:18px; align-items:flex-start; margin-bottom:18px; padding:18px 20px; border:1px solid var(--line); border-radius:18px; background:linear-gradient(135deg,#dff4f6 0%,#fffaf3 100%); box-shadow:0 18px 40px rgba(18,33,47,.08); }\n"
       << "    .bi-kicker { font-size:12px; letter-spacing:.12em; text-transform:uppercase; color:var(--accent); font-weight:800; margin-bottom:6px; }\n"
       << "    h1 { margin:0; font-size:30px; line-height:1.1; }\n"
       << "    .bi-session { display:flex; flex-wrap:wrap; gap:8px; justify-content:flex-end; }\n"
       << "    .bi-chip { display:inline-flex; align-items:center; gap:6px; padding:7px 10px; border-radius:999px; background:var(--chip); border:1px solid #bfe8ea; color:#0e5760; font-size:12px; font-weight:700; }\n"
       << "    .bi-chip.muted { background:#f4f7fa; border-color:var(--line); color:var(--muted); }\n"
       << "    .bi-board { margin-bottom:18px; padding:16px; border:1px solid var(--line); border-radius:18px; background:var(--card); box-shadow:0 14px 34px rgba(24,38,53,.08); }\n"
       << "    .bi-board-head { display:flex; justify-content:space-between; align-items:center; gap:12px; margin-bottom:14px; }\n"
       << "    .bi-board-head h2 { margin:0; font-size:22px; }\n"
       << "    .bi-board-meta { color:var(--muted); font-size:13px; }\n"
       << "    .bi-grid { display:grid; gap:14px; grid-auto-rows:minmax(140px, auto); }\n"
       << "    .bi-tile { display:flex; flex-direction:column; gap:12px; padding:14px; border:1px solid #e3ebf2; border-radius:16px; background:linear-gradient(180deg,#fff 0%,#fbfdff 100%); overflow:hidden; }\n"
       << "    .bi-tile-title { font-size:13px; font-weight:800; letter-spacing:.02em; color:#334757; }\n"
       << "    .bi-kpi { display:flex; flex:1; flex-direction:column; justify-content:center; gap:8px; min-height:100%; }\n"
       << "    .bi-kpi-label { color:var(--muted); font-size:13px; font-weight:700; }\n"
       << "    .bi-kpi-value { font-size:38px; line-height:1; font-weight:800; letter-spacing:-.03em; }\n"
       << "    .bi-kpi-meta { color:var(--muted); font-size:12px; }\n"
       << "    .bi-control { display:flex; flex:1; flex-direction:column; gap:10px; }\n"
       << "    .bi-control-head { display:flex; justify-content:space-between; gap:12px; align-items:flex-start; }\n"
       << "    .bi-control-label { font-weight:800; }\n"
       << "    .bi-control-feature { color:var(--muted); font-size:12px; margin-top:4px; }\n"
       << "    .bi-field-label { display:block; margin-bottom:6px; font-size:12px; color:var(--muted); font-weight:700; }\n"
       << "    .bi-select, .bi-input { width:100%; border:1px solid #c8d5df; border-radius:12px; background:#fff; padding:10px 12px; font:inherit; color:inherit; }\n"
       << "    .bi-range-grid { display:grid; gap:10px; grid-template-columns:repeat(2, minmax(0,1fr)); }\n"
       << "    .bi-help, .bi-empty { color:var(--muted); font-size:12px; line-height:1.45; }\n"
       << "    .bi-svg-wrap { flex:1; min-height:100%; overflow:auto; }\n"
       << "    .bi-svg-wrap svg { display:block; width:100%; height:auto; min-height:" << std::max<std::int64_t>(420, height / 2) << "px; }\n"
       << "    .bi-table-shell { flex:1; min-height:0; }\n"
       << "    .bi-table-frame { width:100%; min-height:100%; height:100%; border:0; border-radius:12px; background:#fff; }\n"
       << "    @media (max-width: 980px) { .bi-shell { padding:14px; } .bi-topbar { flex-direction:column; } .bi-range-grid { grid-template-columns:1fr; } .bi-grid { grid-template-columns:1fr !important; } .bi-tile { grid-column:auto !important; grid-row:auto !important; min-height:180px !important; } }\n"
       << "  </style>\n"
       << "  <script>\n"
       << "    function plussBiSyncSelect(key) { var select = document.getElementById('select-' + key); var chip = document.getElementById('chip-' + key); if (!select || !chip) return; chip.textContent = select.value || 'all'; }\n"
       << "    function plussBiSyncRange(key) { var low = document.getElementById('low-' + key); var high = document.getElementById('high-' + key); var chip = document.getElementById('chip-' + key); if (!low || !high || !chip) return; chip.textContent = (low.value || 'min') + ' to ' + (high.value || 'max'); }\n"
       << "  </script>\n"
       << "</head>\n"
       << "<body>\n"
       << body.str()
       << "</body>\n"
       << "</html>\n";
  return html.str();
}

[[nodiscard]] inline View make_view_kind_(std::string_view key, ViewKind kind) {
  switch (kind) {
    case ViewKind::Kpi: return kpi(key);
    case ViewKind::Line: return line(key);
    case ViewKind::Bar: return bar(key);
    case ViewKind::DetailTable: return table(key);
    case ViewKind::PivotTable: return pivot(key);
    case ViewKind::Scatter: return scatter(key);
    case ViewKind::Heatmap: return heatmap(key);
    case ViewKind::Corr: return corr(key);
  }
  PLUSS_FAIL("pluss::bi: unsupported ViewKind");
}

[[nodiscard]] inline PersistedStudio snapshot_studio_(const Studio& studio) {
  PersistedStudio out;
  out.key = studio.key();
  out.session = studio.session();
  out.boards.reserve(studio.boards().size());
  for (const auto& board : studio.boards()) {
    PersistedBoard board_out;
    board_out.key = board.key();
    board_out.title = board.title();
    board_out.rows = board.nrows();
    board_out.cols = board.ncols();
    board_out.tiles.reserve(board.tiles().size());
    for (const auto& tile : board.tiles()) {
      PersistedTile tile_out;
      tile_out.key = tile.key();
      tile_out.control = tile.control();
      tile_out.view = tile.view();
      board_out.tiles.push_back(std::move(tile_out));
    }
    out.boards.push_back(std::move(board_out));
  }
  return out;
}

[[nodiscard]] inline PersistedBoard& find_persisted_board_(
    PersistedStudio& studio,
    std::string_view key,
    std::string_view where) {
  const auto it = std::find_if(
      studio.boards.begin(),
      studio.boards.end(),
      [&](const PersistedBoard& board) { return board.key == key; });
  PLUSS_CHECK(it != studio.boards.end(),
              std::string(where) + ": unknown board '" + std::string(key) + "'");
  return *it;
}

[[nodiscard]] inline PersistedTile& find_persisted_tile_(
    PersistedBoard& board,
    std::string_view key,
    std::string_view where) {
  const auto it = std::find_if(
      board.tiles.begin(),
      board.tiles.end(),
      [&](const PersistedTile& tile) { return tile.key == key; });
  PLUSS_CHECK(it != board.tiles.end(),
              std::string(where) + ": unknown tile '" + std::string(key) + "'");
  return *it;
}

[[nodiscard]] inline const PersistedTile& find_persisted_tile_(
    const PersistedBoard& board,
    std::string_view key,
    std::string_view where) {
  const auto it = std::find_if(
      board.tiles.begin(),
      board.tiles.end(),
      [&](const PersistedTile& tile) { return tile.key == key; });
  PLUSS_CHECK(it != board.tiles.end(),
              std::string(where) + ": unknown tile '" + std::string(key) + "'");
  return *it;
}

inline void validate_query_contract_(const QuerySpec& query, std::string_view where) {
  switch (query.mode) {
    case QueryMode::Source:
    case QueryMode::StoredPlan:
    case QueryMode::Traverse:
      PLUSS_CHECK(!query.source.empty(),
                  std::string(where) + ": query source cannot be empty");
      break;
    case QueryMode::Sql:
      PLUSS_CHECK(!query.statement.empty(),
                  std::string(where) + ": SQL statement cannot be empty");
      break;
  }

  if (query.head_n.has_value()) {
    PLUSS_CHECK(*query.head_n > 0,
                std::string(where) + ": head() size must be positive");
  }
  if (query.tail_n.has_value()) {
    PLUSS_CHECK(*query.tail_n > 0,
                std::string(where) + ": tail() size must be positive");
  }

  const int plan_count = static_cast<int>(query.group_plan.has_value()) +
      static_cast<int>(query.pivot_plan.has_value()) +
      static_cast<int>(query.resample_plan.has_value());
  PLUSS_CHECK(plan_count <= 1,
              std::string(where) + ": group, pivot, and resample plans are mutually exclusive");

  if (query.group_plan.has_value()) {
    const auto& plan = *query.group_plan;
    PLUSS_CHECK(!plan.keys.empty(),
                std::string(where) + ": grouped query requires at least one key");
    PLUSS_CHECK(!plan.specs.empty(),
                std::string(where) + ": grouped query requires at least one GroupSpec");
  }
  if (query.pivot_plan.has_value()) {
    const auto& plan = *query.pivot_plan;
    PLUSS_CHECK(!plan.value_features.empty(),
                std::string(where) + ": pivot query requires at least one value feature");
  }
  if (query.resample_plan.has_value()) {
    const auto& plan = *query.resample_plan;
    PLUSS_CHECK(!plan.time_feature.empty(),
                std::string(where) + ": resample query requires a time feature");
    PLUSS_CHECK(!plan.freq.empty(),
                std::string(where) + ": resample query requires a frequency");
    PLUSS_CHECK(plan.step > 0,
                std::string(where) + ": resample query step must be positive");
  }
}

inline void validate_view_contract_(const View& view, std::string_view where) {
  PLUSS_CHECK(!view.key().empty(), std::string(where) + ": view key cannot be empty");
  validate_query_contract_(view.query(), where);
  const auto kind = view.bind().kind;
  if (kind == ViewKind::Kpi) {
    PLUSS_CHECK(view.bind().feature.has_value() && !view.bind().feature->empty(),
                std::string(where) + ": KPI view requires feature(...)");
  }
  if (kind == ViewKind::PivotTable) {
    PLUSS_CHECK(!view.bind().index_features.empty(),
                std::string(where) + ": pivot view requires pivot_index(...)");
    PLUSS_CHECK(!view.bind().column_features.empty(),
                std::string(where) + ": pivot view requires pivot_columns(...)");
    PLUSS_CHECK(!view.bind().value_features.empty(),
                std::string(where) + ": pivot view requires pivot_values(...)");
  }
}

inline void validate_control_contract_(const Control& control, std::string_view where) {
  PLUSS_CHECK(!control.key().empty(), std::string(where) + ": control key cannot be empty");
  PLUSS_CHECK(!control.feature().empty(), std::string(where) + ": control feature cannot be empty");
}

inline void validate_persisted_studio_(const PersistedStudio& studio, std::string_view where) {
  PLUSS_CHECK(!studio.key.empty(), std::string(where) + ": studio key cannot be empty");
  std::set<std::string> board_keys;
  for (const auto& board : studio.boards) {
    PLUSS_CHECK(board_keys.insert(board.key).second,
                std::string(where) + ": duplicate board key '" + board.key + "'");
    PLUSS_CHECK(!board.key.empty(), std::string(where) + ": board key cannot be empty");
    PLUSS_CHECK(board.rows > 0 && board.cols > 0,
                std::string(where) + ": board shape must be positive");
    std::set<std::string> tile_keys;
    for (const auto& tile : board.tiles) {
      PLUSS_CHECK(tile_keys.insert(tile.key).second,
                  std::string(where) + ": duplicate tile key '" + tile.key + "'");
      PLUSS_CHECK(tile.control.has_value() != tile.view.has_value(),
                  std::string(where) + ": each tile must contain exactly one control or one view");
      if (tile.control.has_value()) {
        validate_control_contract_(*tile.control, where);
      }
      if (tile.view.has_value()) {
        validate_view_contract_(*tile.view, where);
      }
      PLUSS_CHECK((tile.control.has_value() ? tile.control->layout() : tile.view->layout()).row >= 0,
                  std::string(where) + ": tile row cannot be negative");
      PLUSS_CHECK((tile.control.has_value() ? tile.control->layout() : tile.view->layout()).col >= 0,
                  std::string(where) + ": tile column cannot be negative");
    }
  }
}

inline void write_bundle_(const PersistedStudio& studio, std::string_view path) {
  validate_persisted_studio_(studio, "pluss::bi::Studio::save");

  std::vector<std::string> lines;
  lines.emplace_back("PLUSSBI|1");
  append_record_(lines, "studio", {encode_atom_(studio.key)});

  for (const auto& [key, filter] : studio.session.filters()) {
    (void)key;
    const auto raw_fields = serialize_filter_fields_(filter);
    std::vector<std::string> encoded_fields;
    encoded_fields.reserve(raw_fields.size());
    for (const auto& field : raw_fields) {
      encoded_fields.push_back(encode_atom_(field));
    }
    append_record_(lines, "session_filter", encoded_fields);
  }

  for (const auto& [key, value] : studio.session.params()) {
    std::vector<std::string> encoded_fields;
    encoded_fields.push_back(encode_atom_(key));
    if (std::holds_alternative<double>(value)) {
      encoded_fields.push_back(encode_atom_("double"));
      encoded_fields.push_back(encode_atom_(format_double_(std::get<double>(value))));
    } else {
      encoded_fields.push_back(encode_atom_("text"));
      encoded_fields.push_back(encode_atom_(std::get<std::string>(value)));
    }
    append_record_(lines, "session_param", encoded_fields);
  }

  for (const auto& board : studio.boards) {
    append_record_(lines, "board", {
        encode_atom_(board.key),
        encode_atom_(board.title),
        encode_atom_(std::to_string(board.rows)),
        encode_atom_(std::to_string(board.cols)),
    });

    for (const auto& tile : board.tiles) {
      if (tile.control.has_value()) {
        const auto& control = *tile.control;
        append_record_(lines, "control", {
            encode_atom_(board.key),
            encode_atom_(control.key()),
            encode_atom_(control.label()),
            encode_atom_(control.feature()),
            encode_atom_(encode_control_kind_(control.kind())),
            encode_optional_atom_(control.scope()),
            encode_atom_(std::to_string(control.layout().row)),
            encode_atom_(std::to_string(control.layout().col)),
            encode_atom_(std::to_string(control.layout().row_span)),
            encode_atom_(std::to_string(control.layout().col_span)),
        });
        continue;
      }

      const auto& view = *tile.view;
      append_record_(lines, "view", {
          encode_atom_(board.key),
          encode_atom_(view.key()),
          encode_atom_(std::to_string(view.layout().row)),
          encode_atom_(std::to_string(view.layout().col)),
          encode_atom_(std::to_string(view.layout().row_span)),
          encode_atom_(std::to_string(view.layout().col_span)),
          encode_atom_(encode_view_kind_(view.bind().kind)),
      });

      append_record_(lines, "view_query", {
          encode_atom_(board.key),
          encode_atom_(view.key()),
          encode_atom_(encode_query_mode_(view.query().mode)),
          encode_atom_(view.query().source),
          encode_atom_(view.query().statement),
          encode_optional_atom_(view.query().scope),
          encode_optional_atom_(view.query().title),
          encode_optional_int_(view.query().head_n),
          encode_optional_int_(view.query().tail_n),
          encode_atom_(encode_list_(view.query().select_features)),
          encode_atom_(encode_list_(view.query().drop_features)),
          encode_atom_(encode_rows_([&]() {
            std::vector<std::vector<std::string>> rows;
            rows.reserve(view.query().filters.size());
            for (const auto& filter : view.query().filters) {
              rows.push_back(serialize_filter_fields_(filter));
            }
            return rows;
          }())),
          encode_atom_(encode_rows_(serialize_sort_rows_(view.query().sort_specs))),
          encode_atom_(encode_rows_(serialize_traverse_rows_(view.query().traverse))),
      });

      if (view.query().group_plan.has_value()) {
        const auto& plan = *view.query().group_plan;
        append_record_(lines, "view_query_group", {
            encode_atom_(board.key),
            encode_atom_(view.key()),
            encode_atom_(encode_list_(plan.keys)),
            encode_atom_(bool_token_(plan.dropnull)),
            encode_atom_(bool_token_(plan.sort)),
            encode_atom_(encode_rows_(serialize_group_spec_rows_(plan.specs))),
        });
      }

      if (view.query().pivot_plan.has_value()) {
        const auto& plan = *view.query().pivot_plan;
        append_record_(lines, "view_query_pivot", {
            encode_atom_(board.key),
            encode_atom_(view.key()),
            encode_atom_(encode_list_(plan.index_features)),
            encode_atom_(encode_list_(plan.column_features)),
            encode_atom_(encode_list_(plan.value_features)),
            encode_atom_(encode_group_op_(plan.op)),
            encode_atom_(bool_token_(plan.dropna)),
            encode_atom_(bool_token_(plan.sort)),
        });
      }

      if (view.query().resample_plan.has_value()) {
        const auto& plan = *view.query().resample_plan;
        append_record_(lines, "view_query_resample", {
            encode_atom_(board.key),
            encode_atom_(view.key()),
            encode_atom_(plan.time_feature),
            encode_atom_(encode_list_(plan.value_features)),
            encode_atom_(plan.freq),
            encode_atom_(std::to_string(plan.step)),
            encode_atom_(encode_resample_op_(plan.op)),
        });
      }

      append_record_(lines, "view_bind", {
          encode_atom_(board.key),
          encode_atom_(view.key()),
          encode_atom_(view.bind().title),
          encode_optional_atom_(view.bind().feature),
          encode_optional_atom_(view.bind().columns.feature),
          encode_optional_atom_(view.bind().columns.x_feature),
          encode_optional_atom_(view.bind().columns.y_feature),
          encode_optional_atom_(view.bind().columns.z_feature),
          encode_optional_atom_(view.bind().columns.hue_feature),
          encode_atom_(encode_group_op_(view.bind().op)),
          encode_atom_(bool_token_(view.bind().horizontal)),
          encode_atom_(encode_list_(view.bind().analysis_features)),
          encode_atom_(encode_list_(view.bind().index_features)),
          encode_atom_(encode_list_(view.bind().column_features)),
          encode_atom_(encode_list_(view.bind().value_features)),
          encode_atom_(bool_token_(view.bind().dropna)),
          encode_atom_(bool_token_(view.bind().sort)),
      });
    }
  }

  std::ostringstream text;
  for (std::size_t i = 0; i < lines.size(); ++i) {
    text << lines[i];
    if (i + 1 < lines.size()) {
      text << '\n';
    }
  }
  write_text_file_(normalize_bundle_path_(path), text.str());
}

[[nodiscard]] inline PersistedStudio read_bundle_(std::string_view path) {
  const auto normalized = normalize_bundle_path_(path);
  const auto text = read_text_file_(normalized);
  std::istringstream stream(text);
  std::string line;
  std::int64_t line_no = 0;
  PersistedStudio studio;
  bool saw_magic = false;

  while (std::getline(stream, line)) {
    ++line_no;
    if (line.empty()) {
      continue;
    }
    const auto parts = split_preserve_empty_(line, '|');
    PLUSS_CHECK(!parts.empty(), "pluss::bi::Studio::load: malformed bundle record");
    if (!saw_magic) {
      PLUSS_CHECK(parts.size() == 2 && parts[0] == "PLUSSBI" && parts[1] == "1",
                  "pluss::bi::Studio::load: unsupported or malformed bundle header");
      saw_magic = true;
      continue;
    }

    auto decode_field = [&](std::size_t index, std::string_view where) -> std::string {
      PLUSS_CHECK(index < parts.size(),
                  std::string(where) + ": missing field at line " + std::to_string(line_no));
      return decode_atom_(parts[index]);
    };

    const auto& tag = parts[0];
    const auto where = std::string("pluss::bi::Studio::load(") +
        normalized.string() + ") line " + std::to_string(line_no);

    if (tag == "studio") {
      PLUSS_CHECK(parts.size() == 2, where + ": malformed studio record");
      studio.key = decode_field(1, where);
      continue;
    }

    if (tag == "session_filter") {
      PLUSS_CHECK(parts.size() == 11, where + ": malformed session_filter record");
      std::vector<std::string> fields;
      fields.reserve(10);
      for (std::size_t i = 1; i < parts.size(); ++i) {
        fields.push_back(decode_field(i, where));
      }
      studio.session.set_filter(deserialize_filter_fields_(fields, where));
      continue;
    }

    if (tag == "session_param") {
      PLUSS_CHECK(parts.size() == 4, where + ": malformed session_param record");
      const auto key = decode_field(1, where);
      const auto kind = decode_field(2, where);
      const auto value = decode_field(3, where);
      if (kind == "double") {
        studio.session.set_param(key, parse_double_(value, where));
      } else if (kind == "text") {
        studio.session.set_param(key, value);
      } else {
        PLUSS_FAIL(where + ": unsupported session param type '" + kind + "'");
      }
      continue;
    }

    if (tag == "board") {
      PLUSS_CHECK(parts.size() == 5, where + ": malformed board record");
      PersistedBoard board;
      board.key = decode_field(1, where);
      board.title = decode_field(2, where);
      board.rows = parse_int_<std::int64_t>(decode_field(3, where), where);
      board.cols = parse_int_<std::int64_t>(decode_field(4, where), where);
      PLUSS_CHECK(
          std::none_of(
              studio.boards.begin(),
              studio.boards.end(),
              [&](const PersistedBoard& existing) { return existing.key == board.key; }),
          where + ": duplicate board key '" + board.key + "'");
      studio.boards.push_back(std::move(board));
      continue;
    }

    if (tag == "control") {
      PLUSS_CHECK(parts.size() == 11, where + ": malformed control record");
      auto& board = find_persisted_board_(studio, decode_field(1, where), where);
      const auto key = decode_field(2, where);
      PLUSS_CHECK(
          std::none_of(
              board.tiles.begin(),
              board.tiles.end(),
              [&](const PersistedTile& tile) { return tile.key == key; }),
          where + ": duplicate tile key '" + key + "'");
      Control control(key);
      control.label(decode_field(3, where))
          .feature(decode_field(4, where))
          .kind(decode_control_kind_(decode_field(5, where)));
      if (const auto scope = parse_optional_atom_(parts[6]); scope.has_value()) {
        control.scope(*scope);
      }
      control.place(
          parse_int_<std::int64_t>(decode_field(7, where), where),
          parse_int_<std::int64_t>(decode_field(8, where), where),
          parse_int_<std::int64_t>(decode_field(9, where), where),
          parse_int_<std::int64_t>(decode_field(10, where), where));
      board.tiles.push_back(PersistedTile{
          .key = key,
          .control = control,
      });
      continue;
    }

    if (tag == "view") {
      PLUSS_CHECK(parts.size() == 8, where + ": malformed view record");
      auto& board = find_persisted_board_(studio, decode_field(1, where), where);
      const auto key = decode_field(2, where);
      PLUSS_CHECK(
          std::none_of(
              board.tiles.begin(),
              board.tiles.end(),
              [&](const PersistedTile& tile) { return tile.key == key; }),
          where + ": duplicate tile key '" + key + "'");
      auto view = make_view_kind_(key, decode_view_kind_(decode_field(7, where)));
      view.place(
          parse_int_<std::int64_t>(decode_field(3, where), where),
          parse_int_<std::int64_t>(decode_field(4, where), where),
          parse_int_<std::int64_t>(decode_field(5, where), where),
          parse_int_<std::int64_t>(decode_field(6, where), where));
      board.tiles.push_back(PersistedTile{
          .key = key,
          .view = view,
      });
      continue;
    }

    if (tag == "view_query") {
      PLUSS_CHECK(parts.size() == 15, where + ": malformed view_query record");
      auto& board = find_persisted_board_(studio, decode_field(1, where), where);
      auto& tile = find_persisted_tile_(board, decode_field(2, where), where);
      PLUSS_CHECK(tile.view.has_value(), where + ": view_query target is not a view");
      QuerySpec query;
      query.mode = decode_query_mode_(decode_field(3, where));
      query.source = decode_field(4, where);
      query.statement = decode_field(5, where);
      if (const auto scope = parse_optional_atom_(parts[6]); scope.has_value()) {
        query.scope = *scope;
      }
      if (const auto title = parse_optional_atom_(parts[7]); title.has_value()) {
        query.title = *title;
      }
      query.head_n = parse_optional_int_(decode_field(8, where), where);
      query.tail_n = parse_optional_int_(decode_field(9, where), where);
      query.select_features = decode_list_(decode_field(10, where));
      query.drop_features = decode_list_(decode_field(11, where));
      for (const auto& row : decode_rows_(decode_field(12, where))) {
        query.filters.push_back(deserialize_filter_fields_(row, where));
      }
      query.sort_specs = deserialize_sort_rows_(decode_rows_(decode_field(13, where)), where);
      query.traverse = deserialize_traverse_rows_(decode_rows_(decode_field(14, where)), where);
      tile.view->query(std::move(query));
      continue;
    }

    if (tag == "view_query_group") {
      PLUSS_CHECK(parts.size() == 7, where + ": malformed view_query_group record");
      auto& board = find_persisted_board_(studio, decode_field(1, where), where);
      auto& tile = find_persisted_tile_(board, decode_field(2, where), where);
      PLUSS_CHECK(tile.view.has_value(), where + ": view_query_group target is not a view");
      auto query = tile.view->query();
      PLUSS_CHECK(!query.pivot_plan.has_value() && !query.resample_plan.has_value(),
                  where + ": grouped query cannot also carry pivot/resample plan");
      QueryGroupPlan plan;
      plan.keys = decode_list_(decode_field(3, where));
      plan.dropnull = parse_bool_(decode_field(4, where), where);
      plan.sort = parse_bool_(decode_field(5, where), where);
      plan.specs = deserialize_group_spec_rows_(decode_rows_(decode_field(6, where)), where);
      query.group_plan = std::move(plan);
      tile.view->query(std::move(query));
      continue;
    }

    if (tag == "view_query_pivot") {
      PLUSS_CHECK(parts.size() == 9, where + ": malformed view_query_pivot record");
      auto& board = find_persisted_board_(studio, decode_field(1, where), where);
      auto& tile = find_persisted_tile_(board, decode_field(2, where), where);
      PLUSS_CHECK(tile.view.has_value(), where + ": view_query_pivot target is not a view");
      auto query = tile.view->query();
      PLUSS_CHECK(!query.group_plan.has_value() && !query.resample_plan.has_value(),
                  where + ": pivot query cannot also carry group/resample plan");
      QueryPivotPlan plan;
      plan.index_features = decode_list_(decode_field(3, where));
      plan.column_features = decode_list_(decode_field(4, where));
      plan.value_features = decode_list_(decode_field(5, where));
      plan.op = decode_group_op_(decode_field(6, where));
      plan.dropna = parse_bool_(decode_field(7, where), where);
      plan.sort = parse_bool_(decode_field(8, where), where);
      query.pivot_plan = std::move(plan);
      tile.view->query(std::move(query));
      continue;
    }

    if (tag == "view_query_resample") {
      PLUSS_CHECK(parts.size() == 8, where + ": malformed view_query_resample record");
      auto& board = find_persisted_board_(studio, decode_field(1, where), where);
      auto& tile = find_persisted_tile_(board, decode_field(2, where), where);
      PLUSS_CHECK(tile.view.has_value(), where + ": view_query_resample target is not a view");
      auto query = tile.view->query();
      PLUSS_CHECK(!query.group_plan.has_value() && !query.pivot_plan.has_value(),
                  where + ": resample query cannot also carry group/pivot plan");
      QueryResamplePlan plan;
      plan.time_feature = decode_field(3, where);
      plan.value_features = decode_list_(decode_field(4, where));
      plan.freq = decode_field(5, where);
      plan.step = parse_int_<std::int64_t>(decode_field(6, where), where);
      plan.op = decode_resample_op_(decode_field(7, where));
      query.resample_plan = std::move(plan);
      tile.view->query(std::move(query));
      continue;
    }

    if (tag == "view_bind") {
      PLUSS_CHECK(parts.size() == 18, where + ": malformed view_bind record");
      auto& board = find_persisted_board_(studio, decode_field(1, where), where);
      auto& tile = find_persisted_tile_(board, decode_field(2, where), where);
      PLUSS_CHECK(tile.view.has_value(), where + ": view_bind target is not a view");
      BindSpec bind;
      bind.kind = tile.view->bind().kind;
      bind.title = decode_field(3, where);
      if (const auto value = parse_optional_atom_(parts[4]); value.has_value()) {
        bind.feature = *value;
      }
      if (const auto value = parse_optional_atom_(parts[5]); value.has_value()) {
        bind.columns.feature = *value;
      }
      if (const auto value = parse_optional_atom_(parts[6]); value.has_value()) {
        bind.columns.x_feature = *value;
      }
      if (const auto value = parse_optional_atom_(parts[7]); value.has_value()) {
        bind.columns.y_feature = *value;
      }
      if (const auto value = parse_optional_atom_(parts[8]); value.has_value()) {
        bind.columns.z_feature = *value;
      }
      if (const auto value = parse_optional_atom_(parts[9]); value.has_value()) {
        bind.columns.hue_feature = *value;
      }
      bind.op = decode_group_op_(decode_field(10, where));
      bind.horizontal = parse_bool_(decode_field(11, where), where);
      bind.analysis_features = decode_list_(decode_field(12, where));
      bind.index_features = decode_list_(decode_field(13, where));
      bind.column_features = decode_list_(decode_field(14, where));
      bind.value_features = decode_list_(decode_field(15, where));
      bind.dropna = parse_bool_(decode_field(16, where), where);
      bind.sort = parse_bool_(decode_field(17, where), where);

      const auto layout = tile.view->layout();
      const auto key = tile.view->key();
      const auto query = tile.view->query();
      auto rebuilt = View::make(key, query, bind);
      rebuilt.place(layout.row, layout.col, layout.row_span, layout.col_span);
      tile.view = std::move(rebuilt);
      continue;
    }

    PLUSS_FAIL(where + ": unknown bundle record tag '" + tag + "'");
  }

  PLUSS_CHECK(saw_magic,
              "pluss::bi::Studio::load: unsupported or malformed bundle header");
  validate_persisted_studio_(studio, "pluss::bi::Studio::load");
  return studio;
}

[[nodiscard]] inline Studio materialize_studio_(
    const PersistedStudio& persisted,
    const pluss::bundle::Nexus* nexus) {
  validate_persisted_studio_(persisted, "pluss::bi::Studio::load");
  Studio out(persisted.key);
  if (nexus != nullptr) {
    out.source(*nexus);
  }
  out.session(persisted.session);
  for (const auto& board_state : persisted.boards) {
    auto& board = out.board(board_state.key);
    board.grid(board_state.rows, board_state.cols);
    if (!board_state.title.empty()) {
      board.title(board_state.title);
    }
    for (const auto& tile : board_state.tiles) {
      if (tile.control.has_value()) {
        board.control(*tile.control);
      } else if (tile.view.has_value()) {
        board.view(*tile.view);
      }
    }
  }
  return out;
}

} // namespace detail

inline void Studio::save(std::string_view path) const {
  detail::write_bundle_(detail::snapshot_studio_(*this), path);
}

inline Studio Studio::load(std::string_view path) {
  return detail::materialize_studio_(detail::read_bundle_(path), nullptr);
}

inline Studio Studio::load(
    std::string_view path,
    const pluss::bundle::Nexus& nexus) {
  return detail::materialize_studio_(detail::read_bundle_(path), &nexus);
}

inline Studio& Studio::restore_session(std::string_view path) {
  session(detail::read_bundle_(path).session);
  return *this;
}

inline void Studio::export_html(
    std::string_view path,
    std::string_view title,
    std::int64_t width,
    std::int64_t height) const {
  PLUSS_CHECK(width > 0 && height > 0,
              "pluss::bi::Studio::export_html: width and height must be positive");
  const auto rendered = render();
  PLUSS_CHECK(rendered.native != nullptr,
              "pluss::bi::Studio::export_html: native board studio was not created");
  const auto html = detail::dashboard_html_(
      rendered,
      *nexus_,
      title.empty() ? key_ : title,
      width,
      height,
      rendered.native->to_svg());
  detail::write_text_file_(detail::normalize_html_path_(path), html);
}

inline void Studio::export_svg(std::string_view path) const {
  const auto rendered = render();
  PLUSS_CHECK(rendered.native != nullptr,
              "pluss::bi::Studio::export_svg: native board studio was not created");
  rendered.native->save_svg(path);
}

inline void Studio::export_pdf(
    std::string_view path,
    pluss::pl::PdfEngine engine) const {
  const auto rendered = render();
  PLUSS_CHECK(rendered.native != nullptr,
              "pluss::bi::Studio::export_pdf: native board studio was not created");
  rendered.native->save_pdf(path, engine);
}

inline void Studio::export_png(std::string_view path, std::int64_t dpi) const {
  PLUSS_CHECK(dpi > 0,
              "pluss::bi::Studio::export_png: dpi must be positive");
  const auto rendered = render();
  PLUSS_CHECK(rendered.native != nullptr,
              "pluss::bi::Studio::export_png: native board studio was not created");
  rendered.native->save_png(path, dpi);
}

inline Session& apply(
    Session& session,
    const Control& control,
    std::string_view value) {
  PLUSS_CHECK(control.kind() == ControlKind::Category,
              "pluss::bi::apply(session, control, value): control is not categorical");
  auto filter = Filter::eq(control.feature(), value);
  if (control.scope().has_value()) {
    filter = filter.scoped(*control.scope());
  }
  filter.key = control.key();
  session.set_filter(std::move(filter));
  return session;
}

inline Session& apply(
    Session& session,
    const Control& control,
    const std::vector<std::string>& values) {
  PLUSS_CHECK(control.kind() == ControlKind::Category,
              "pluss::bi::apply(session, control, values): control is not categorical");
  auto filter = Filter::in(control.feature(), values);
  if (control.scope().has_value()) {
    filter = filter.scoped(*control.scope());
  }
  filter.key = control.key();
  session.set_filter(std::move(filter));
  return session;
}

inline Session& apply(
    Session& session,
    const Control& control,
    double low,
    double high) {
  PLUSS_CHECK(control.kind() == ControlKind::NumericRange,
              "pluss::bi::apply(session, control, low, high): control is not a numeric-range control");
  auto filter = Filter::between(control.feature(), low, high);
  if (control.scope().has_value()) {
    filter = filter.scoped(*control.scope());
  }
  filter.key = control.key();
  session.set_filter(std::move(filter));
  return session;
}

inline Session& apply_time(
    Session& session,
    const Control& control,
    const pluss::Timeline& low,
    const pluss::Timeline& high) {
  PLUSS_CHECK(control.kind() == ControlKind::TemporalRange,
              "pluss::bi::apply_time(session, control, low, high): control is not a temporal-range control");
  auto filter = Filter::between_time(control.feature(), low, high);
  if (control.scope().has_value()) {
    filter = filter.scoped(*control.scope());
  }
  filter.key = control.key();
  session.set_filter(std::move(filter));
  return session;
}

inline Session& clear(Session& session, const Control& control) {
  session.clear_filter(control.key());
  return session;
}

} // namespace pluss::bi
