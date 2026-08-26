#pragma once

#include "pluss/bi/session.h"

#include "pluss/codeh/codeh.h"
#include "pluss/pl.h"
#include "pluss/tils/aux/scalar_cast.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
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
      return column.btw(scalar_temporal_iso_(range.low), scalar_temporal_iso_(range.high), true);
    case pluss::table::ThreadKind::TimeIndex:
      return column.btw(scalar_temporal_iso_(range.low), scalar_temporal_iso_(range.high), true);
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
    return std::to_string(pluss::aux::as_f64(value.as_series()));
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
      (tile.kind == ViewKind::Line || tile.kind == ViewKind::Bar)) {
    const auto& bind = tile.view->bind();
    if (tile.kind == ViewKind::Line) {
      (void)pluss::pl::flash::lineplot(pane, *tile.table, bind.columns);
    } else {
      pluss::pl::flash::BarPlotOptions options;
      options.horizontal = bind.horizontal;
      (void)pluss::pl::flash::barplot(pane, *tile.table, bind.columns, options);
    }
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
    return rendered.native->show(
        title.empty() ? key_ : title,
        width,
        height,
        launch);
  }

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
    const auto cache_key = std::string(tile_id) + "\n" +
        detail::query_signature_(query) +
        current_filter_signature_for_tile_(tile_id);
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
      } else if (view.bind().kind == ViewKind::Line || view.bind().kind == ViewKind::Bar) {
        auto plot = std::make_shared<pluss::pl::Studio>(
            view.bind().title.empty() ? view.key() : view.bind().title);
        auto& pane = plot->pane2(view.bind().title.empty() ? view.key() : view.bind().title);
        if (view.bind().kind == ViewKind::Line) {
          (void)pluss::pl::flash::lineplot(pane, *out.table, view.bind().columns);
        } else {
          pluss::pl::flash::BarPlotOptions options;
          options.horizontal = view.bind().horizontal;
          (void)pluss::pl::flash::barplot(pane, *out.table, view.bind().columns, options);
        }
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
