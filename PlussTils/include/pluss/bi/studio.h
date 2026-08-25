#pragma once

#include "pluss/bi/session.h"

#include "pluss/codeh/codeh.h"
#include "pluss/pl.h"
#include "pluss/tils/aux/scalar_cast.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pluss::bi {

struct TileRender {
  std::string key{};
  LayoutSpec layout{};
  ViewKind kind{ViewKind::DetailTable};
  bool is_control{false};
  std::optional<Control> control{};
  std::optional<Filter> active_filter{};
  std::shared_ptr<pluss::pl::Studio> plot{};
  std::shared_ptr<pluss::table::DataFrame> table{};
  std::optional<pluss::table::ThreadValue> scalar{};
};

struct BoardRender {
  std::string key{};
  std::string title{};
  std::vector<TileRender> tiles{};
};

struct StudioRender {
  std::string key{};
  Session session{};
  std::vector<BoardRender> boards{};
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

[[nodiscard]] inline TileRender render_view_tile_(
    const Tile& tile,
    const pluss::bundle::Nexus& nexus,
    const Session& session) {
  PLUSS_CHECK(tile.view.has_value(), "pluss::bi: expected a view tile");
  const auto& view = *tile.view;
  const auto frame = resolve_query_(nexus, view.query, session);

  TileRender out;
  out.key = tile.key;
  out.layout = tile.layout;
  out.kind = view.bind.kind;

  if (view.bind.kind == ViewKind::Kpi) {
    out.scalar = reduce_kpi_(frame, view.bind);
    out.table = std::make_shared<pluss::table::DataFrame>(frame);
    return out;
  }

  if (view.bind.kind == ViewKind::DetailTable) {
    out.table = std::make_shared<pluss::table::DataFrame>(frame);
    return out;
  }

  if (view.bind.kind == ViewKind::PivotTable) {
    const auto index_views = string_views_(view.bind.index_features);
    const auto column_views = string_views_(view.bind.column_features);
    const auto value_views = string_views_(view.bind.value_features);
    pluss::table::DataFrame materialized = frame.pivot_table(
        pluss::msharp::RangeView::Of<std::string_view>(index_views),
        pluss::msharp::RangeView::Of<std::string_view>(column_views),
        pluss::msharp::RangeView::Of<std::string_view>(value_views),
        view.bind.op,
        view.bind.dropna,
        false,
        view.bind.sort);
    out.table = std::make_shared<pluss::table::DataFrame>(std::move(materialized));
    return out;
  }

  auto plot = std::make_shared<pluss::pl::Studio>(view.bind.title.empty() ? view.key : view.bind.title);
  auto& pane = plot->pane2(view.bind.title.empty() ? view.key : view.bind.title);
  if (view.bind.kind == ViewKind::Line) {
    (void)pluss::pl::flash::lineplot(pane, frame, view.bind.columns);
  } else if (view.bind.kind == ViewKind::Bar) {
    pluss::pl::flash::BarPlotOptions options;
    options.horizontal = view.bind.horizontal;
    (void)pluss::pl::flash::barplot(pane, frame, view.bind.columns, options);
  } else {
    PLUSS_FAIL("pluss::bi: unsupported view kind");
  }
  out.plot = std::move(plot);
  out.table = std::make_shared<pluss::table::DataFrame>(frame);
  return out;
}

[[nodiscard]] inline TileRender render_control_tile_(
    const Tile& tile,
    const Session& session) {
  PLUSS_CHECK(tile.control.has_value(), "pluss::bi: expected a control tile");

  TileRender out;
  out.key = tile.key;
  out.layout = tile.layout;
  out.is_control = true;
  out.control = tile.control;
  if (const auto active = session.filter(tile.control->key); active.has_value()) {
    out.active_filter = std::move(active);
    return out;
  }

  for (const auto& [_, value] : session.filters()) {
    if (value.feature != tile.control->feature) {
      continue;
    }
    if (tile.control->scope.has_value()) {
      if (!value.scope.has_value() || value.scope != tile.control->scope) {
        continue;
      }
    }
    out.active_filter = value;
    break;
  }
  return out;
}

inline void validate_tile_(const Tile& tile, std::int64_t rows, std::int64_t cols) {
  PLUSS_CHECK(!tile.key.empty(), "pluss::bi: tile key cannot be empty");
  PLUSS_CHECK(tile.layout.row >= 0 && tile.layout.col >= 0,
              "pluss::bi: tile coordinates cannot be negative");
  PLUSS_CHECK(tile.layout.row_span > 0 && tile.layout.col_span > 0,
              "pluss::bi: tile spans must be positive");
  PLUSS_CHECK(tile.layout.row + tile.layout.row_span <= rows,
              "pluss::bi: tile row span exceeds board shape");
  PLUSS_CHECK(tile.layout.col + tile.layout.col_span <= cols,
              "pluss::bi: tile column span exceeds board shape");
  const bool has_view = tile.view.has_value();
  const bool has_control = tile.control.has_value();
  PLUSS_CHECK(has_view != has_control,
              "pluss::bi: each tile must contain exactly one view or one control");
}

inline void validate_board_(const Board& board) {
  PLUSS_CHECK(!board.key.empty(), "pluss::bi: board key cannot be empty");
  PLUSS_CHECK(board.rows > 0 && board.cols > 0,
              "pluss::bi: board shape must be positive");
  std::set<std::string> seen;
  for (const auto& tile : board.tiles) {
    PLUSS_CHECK(seen.insert(tile.key).second,
                "pluss::bi: duplicate tile key '" + tile.key + "'");
    validate_tile_(tile, board.rows, board.cols);
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
    return *this;
  }

  Studio& source(const pluss::bundle::Nexus& nexus) {
    nexus_ = &nexus;
    return *this;
  }

  Studio& add(Board board) {
    boards_.push_back(std::move(board));
    return *this;
  }

  Studio& set_filter(Filter value) {
    session_.set_filter(std::move(value));
    return *this;
  }

  Studio& clear_filter(std::string_view key) {
    session_.clear_filter(key);
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
    out.boards.reserve(boards_.size());
    for (const auto& board : boards_) {
      detail::validate_board_(board);
      BoardRender board_out;
      board_out.key = board.key;
      board_out.title = board.title;
      board_out.tiles.reserve(board.tiles.size());
      for (const auto& tile : board.tiles) {
        if (tile.view.has_value()) {
          board_out.tiles.push_back(detail::render_view_tile_(tile, *nexus_, session_));
        } else if (tile.control.has_value()) {
          board_out.tiles.push_back(detail::render_control_tile_(tile, session_));
        }
      }
      out.boards.push_back(std::move(board_out));
    }
    return out;
  }

private:
  std::string key_{"studio"};
  const pluss::bundle::Nexus* nexus_{nullptr};
  Session session_{};
  std::vector<Board> boards_{};
};

inline Session& apply(
    Session& session,
    const Control& control,
    std::string_view value) {
  PLUSS_CHECK(control.kind == ControlKind::Category,
              "pluss::bi::apply(session, control, value): control is not categorical");
  auto filter = Filter::eq(control.feature, value);
  if (control.scope.has_value()) {
    filter = filter.scoped(*control.scope);
  }
  filter.key = control.key;
  session.set_filter(std::move(filter));
  return session;
}

inline Session& apply(
    Session& session,
    const Control& control,
    const std::vector<std::string>& values) {
  PLUSS_CHECK(control.kind == ControlKind::Category,
              "pluss::bi::apply(session, control, values): control is not categorical");
  auto filter = Filter::in(control.feature, values);
  if (control.scope.has_value()) {
    filter = filter.scoped(*control.scope);
  }
  filter.key = control.key;
  session.set_filter(std::move(filter));
  return session;
}

inline Session& apply(
    Session& session,
    const Control& control,
    double low,
    double high) {
  PLUSS_CHECK(control.kind == ControlKind::NumericRange,
              "pluss::bi::apply(session, control, low, high): control is not a numeric-range control");
  auto filter = Filter::between(control.feature, low, high);
  if (control.scope.has_value()) {
    filter = filter.scoped(*control.scope);
  }
  filter.key = control.key;
  session.set_filter(std::move(filter));
  return session;
}

inline Session& apply_time(
    Session& session,
    const Control& control,
    const pluss::Timeline& low,
    const pluss::Timeline& high) {
  PLUSS_CHECK(control.kind == ControlKind::TemporalRange,
              "pluss::bi::apply_time(session, control, low, high): control is not a temporal-range control");
  auto filter = Filter::between_time(control.feature, low, high);
  if (control.scope.has_value()) {
    filter = filter.scoped(*control.scope);
  }
  filter.key = control.key;
  session.set_filter(std::move(filter));
  return session;
}

inline Session& clear(Session& session, const Control& control) {
  session.clear_filter(control.key);
  return session;
}

} // namespace pluss::bi
