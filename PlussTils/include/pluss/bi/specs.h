#pragma once

#include "pluss/bundle/bundle.h"
#include "pluss/table/table.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace pluss::bi {

enum class FilterOp : std::uint8_t {
  Eq = 0,
  Ne = 1,
  Gt = 2,
  Ge = 3,
  Lt = 4,
  Le = 5,
  In = 6,
  Between = 7,
};

enum class QueryMode : std::uint8_t {
  Source = 0,
  StoredPlan = 1,
  Sql = 2,
  Traverse = 3,
};

enum class ControlKind : std::uint8_t {
  Category = 0,
  NumericRange = 1,
  TemporalRange = 2,
};

enum class ViewKind : std::uint8_t {
  Kpi = 0,
  Line = 1,
  Bar = 2,
  DetailTable = 3,
  PivotTable = 4,
};

using FilterScalar = std::variant<double, std::string, pluss::Timeline, pluss::TimeIndex>;
using FilterSet = std::variant<std::vector<double>, std::vector<std::string>>;

struct FilterRange {
  FilterScalar low{};
  FilterScalar high{};
};

struct Filter {
  std::string key{};
  std::string feature{};
  FilterOp op{FilterOp::Eq};
  std::variant<std::monostate, FilterScalar, FilterSet, FilterRange> payload{};
  std::optional<std::string> scope{};
  bool enabled{true};

  [[nodiscard]] Filter scoped(std::string_view value) const {
    Filter out = *this;
    out.scope = std::string(value);
    return out;
  }

  [[nodiscard]] static Filter eq(std::string_view feature, double value) {
    return Filter{
        .key = std::string(feature),
        .feature = std::string(feature),
        .op = FilterOp::Eq,
        .payload = FilterScalar(value),
    };
  }

  [[nodiscard]] static Filter eq(std::string_view feature, std::string_view value) {
    return Filter{
        .key = std::string(feature),
        .feature = std::string(feature),
        .op = FilterOp::Eq,
        .payload = FilterScalar(std::string(value)),
    };
  }

  [[nodiscard]] static Filter eq(std::string_view feature, const pluss::Timeline& value) {
    return Filter{
        .key = std::string(feature),
        .feature = std::string(feature),
        .op = FilterOp::Eq,
        .payload = FilterScalar(value),
    };
  }

  [[nodiscard]] static Filter eq(std::string_view feature, const pluss::TimeIndex& value) {
    return Filter{
        .key = std::string(feature),
        .feature = std::string(feature),
        .op = FilterOp::Eq,
        .payload = FilterScalar(value),
    };
  }

  [[nodiscard]] static Filter ne(std::string_view feature, double value) {
    auto out = eq(feature, value);
    out.op = FilterOp::Ne;
    return out;
  }

  [[nodiscard]] static Filter ne(std::string_view feature, std::string_view value) {
    auto out = eq(feature, value);
    out.op = FilterOp::Ne;
    return out;
  }

  [[nodiscard]] static Filter gt(std::string_view feature, double value) {
    auto out = eq(feature, value);
    out.op = FilterOp::Gt;
    return out;
  }

  [[nodiscard]] static Filter ge(std::string_view feature, double value) {
    auto out = eq(feature, value);
    out.op = FilterOp::Ge;
    return out;
  }

  [[nodiscard]] static Filter lt(std::string_view feature, double value) {
    auto out = eq(feature, value);
    out.op = FilterOp::Lt;
    return out;
  }

  [[nodiscard]] static Filter le(std::string_view feature, double value) {
    auto out = eq(feature, value);
    out.op = FilterOp::Le;
    return out;
  }

  [[nodiscard]] static Filter between(std::string_view feature, double low, double high) {
    return Filter{
        .key = std::string(feature),
        .feature = std::string(feature),
        .op = FilterOp::Between,
        .payload = FilterRange{
            .low = FilterScalar(low),
            .high = FilterScalar(high),
        },
    };
  }

  [[nodiscard]] static Filter between_time(
      std::string_view feature,
      const pluss::Timeline& low,
      const pluss::Timeline& high) {
    return Filter{
        .key = std::string(feature),
        .feature = std::string(feature),
        .op = FilterOp::Between,
        .payload = FilterRange{
            .low = FilterScalar(low),
            .high = FilterScalar(high),
        },
    };
  }

  [[nodiscard]] static Filter between_time(
      std::string_view feature,
      const pluss::TimeIndex& low,
      const pluss::TimeIndex& high) {
    return Filter{
        .key = std::string(feature),
        .feature = std::string(feature),
        .op = FilterOp::Between,
        .payload = FilterRange{
            .low = FilterScalar(low),
            .high = FilterScalar(high),
        },
    };
  }

  [[nodiscard]] static Filter in(std::string_view feature, std::vector<double> values) {
    return Filter{
        .key = std::string(feature),
        .feature = std::string(feature),
        .op = FilterOp::In,
        .payload = FilterSet(std::move(values)),
    };
  }

  [[nodiscard]] static Filter in(std::string_view feature, std::vector<std::string> values) {
    return Filter{
        .key = std::string(feature),
        .feature = std::string(feature),
        .op = FilterOp::In,
        .payload = FilterSet(std::move(values)),
    };
  }
};

struct LayoutSpec {
  std::int64_t row{0};
  std::int64_t col{0};
  std::int64_t row_span{1};
  std::int64_t col_span{1};
  std::optional<double> min_height{};

  [[nodiscard]] static LayoutSpec at(
      std::int64_t row,
      std::int64_t col,
      std::int64_t row_span = 1,
      std::int64_t col_span = 1) {
    return LayoutSpec{
        .row = row,
        .col = col,
        .row_span = row_span,
        .col_span = col_span,
    };
  }
};

struct ThemeSpec {
  std::string name{};
};

struct ExportSpec {
  std::optional<std::string> path{};
};

struct TraverseStep {
  enum class Kind : std::uint8_t {
    Join = 0,
    Via = 1,
  };

  Kind kind{Kind::Join};
  std::string target{};
  std::optional<pluss::bundle::nexus::JoinSpec> join_spec{};
  pluss::bundle::nexus::JoinHow how{pluss::bundle::nexus::JoinHow::Left};
};

struct SortSpec {
  std::string feature{};
  bool ascending{true};
  bool nulls_last{true};
};

struct QuerySpec {
  QueryMode mode{QueryMode::Source};
  std::string source{};
  std::string statement{};
  std::vector<TraverseStep> traverse{};
  std::vector<Filter> filters{};
  std::vector<SortSpec> sort_specs{};
  std::vector<std::string> select_features{};
  std::vector<std::string> drop_features{};
  std::optional<std::int64_t> head_n{};
  std::optional<std::int64_t> tail_n{};
  std::optional<std::string> scope{};
  std::optional<std::string> title{};

  [[nodiscard]] static QuerySpec from(std::string_view source) {
    return QuerySpec{
        .mode = QueryMode::Source,
        .source = std::string(source),
    };
  }

  [[nodiscard]] static QuerySpec stored(std::string_view name) {
    return QuerySpec{
        .mode = QueryMode::StoredPlan,
        .source = std::string(name),
    };
  }

  [[nodiscard]] static QuerySpec sql(std::string_view statement) {
    return QuerySpec{
        .mode = QueryMode::Sql,
        .statement = std::string(statement),
    };
  }

  [[nodiscard]] static QuerySpec traverse_from(std::string_view source) {
    return QuerySpec{
        .mode = QueryMode::Traverse,
        .source = std::string(source),
    };
  }

  [[nodiscard]] QuerySpec where(const Filter& value) const {
    QuerySpec out = *this;
    out.filters.push_back(value);
    return out;
  }

  [[nodiscard]] QuerySpec arrange(
      std::string_view feature,
      bool ascending = true,
      bool nulls_last = true) const {
    QuerySpec out = *this;
    out.sort_specs.push_back(SortSpec{
        .feature = std::string(feature),
        .ascending = ascending,
        .nulls_last = nulls_last,
    });
    return out;
  }

  [[nodiscard]] QuerySpec select(std::initializer_list<std::string_view> features) const {
    QuerySpec out = *this;
    out.select_features.clear();
    out.select_features.reserve(features.size());
    for (const auto feature : features) {
      out.select_features.emplace_back(feature);
    }
    return out;
  }

  [[nodiscard]] QuerySpec drop(std::initializer_list<std::string_view> features) const {
    QuerySpec out = *this;
    out.drop_features.reserve(out.drop_features.size() + features.size());
    for (const auto feature : features) {
      out.drop_features.emplace_back(feature);
    }
    return out;
  }

  [[nodiscard]] QuerySpec head(std::int64_t n) const {
    QuerySpec out = *this;
    out.head_n = n;
    return out;
  }

  [[nodiscard]] QuerySpec tail(std::int64_t n) const {
    QuerySpec out = *this;
    out.tail_n = n;
    return out;
  }

  [[nodiscard]] QuerySpec scope_as(std::string_view value) const {
    QuerySpec out = *this;
    out.scope = std::string(value);
    return out;
  }

  [[nodiscard]] QuerySpec named(std::string_view value) const {
    QuerySpec out = *this;
    out.title = std::string(value);
    return out;
  }

  [[nodiscard]] QuerySpec join(std::string_view target) const {
    QuerySpec out = *this;
    out.traverse.push_back(TraverseStep{
        .kind = TraverseStep::Kind::Join,
        .target = std::string(target),
    });
    return out;
  }

  [[nodiscard]] QuerySpec join(
      std::string_view target,
      const pluss::bundle::nexus::JoinSpec& spec) const {
    QuerySpec out = *this;
    out.traverse.push_back(TraverseStep{
        .kind = TraverseStep::Kind::Join,
        .target = std::string(target),
        .join_spec = spec,
    });
    return out;
  }

  [[nodiscard]] QuerySpec via(
      std::string_view target,
      pluss::bundle::nexus::JoinHow how = pluss::bundle::nexus::JoinHow::Left) const {
    QuerySpec out = *this;
    out.traverse.push_back(TraverseStep{
        .kind = TraverseStep::Kind::Via,
        .target = std::string(target),
        .how = how,
    });
    return out;
  }
};

struct BindSpec {
  ViewKind kind{ViewKind::DetailTable};
  std::string title{};
  std::optional<std::string> feature{};
  pluss::table::FlashTableColumns columns{};
  pluss::table::GroupOp op{pluss::table::GroupOp::Count};
  bool horizontal{false};
  std::vector<std::string> index_features{};
  std::vector<std::string> column_features{};
  std::vector<std::string> value_features{};
  bool dropna{true};
  bool sort{true};

  [[nodiscard]] static BindSpec kpi(
      std::string_view feature,
      pluss::table::GroupOp op = pluss::table::GroupOp::Count,
      std::string_view title = {}) {
    return BindSpec{
        .kind = ViewKind::Kpi,
        .title = std::string(title),
        .feature = std::string(feature),
        .op = op,
    };
  }

  [[nodiscard]] static BindSpec line(
      const pluss::table::FlashTableColumns& columns = {},
      std::string_view title = {}) {
    return BindSpec{
        .kind = ViewKind::Line,
        .title = std::string(title),
        .columns = columns,
    };
  }

  [[nodiscard]] static BindSpec bar(
      const pluss::table::FlashTableColumns& columns = {},
      bool horizontal = false,
      std::string_view title = {}) {
    return BindSpec{
        .kind = ViewKind::Bar,
        .title = std::string(title),
        .columns = columns,
        .horizontal = horizontal,
    };
  }

  [[nodiscard]] static BindSpec detail_table(std::string_view title = {}) {
    return BindSpec{
        .kind = ViewKind::DetailTable,
        .title = std::string(title),
    };
  }

  [[nodiscard]] static BindSpec pivot_table(
      std::initializer_list<std::string_view> index_features,
      std::initializer_list<std::string_view> column_features,
      std::initializer_list<std::string_view> value_features,
      pluss::table::GroupOp op = pluss::table::GroupOp::Mean,
      bool dropna = true,
      bool sort = true,
      std::string_view title = {}) {
    BindSpec out;
    out.kind = ViewKind::PivotTable;
    out.title = std::string(title);
    out.op = op;
    out.dropna = dropna;
    out.sort = sort;
    out.index_features.reserve(index_features.size());
    out.column_features.reserve(column_features.size());
    out.value_features.reserve(value_features.size());
    for (const auto value : index_features) {
      out.index_features.emplace_back(value);
    }
    for (const auto value : column_features) {
      out.column_features.emplace_back(value);
    }
    for (const auto value : value_features) {
      out.value_features.emplace_back(value);
    }
    return out;
  }
};

struct View {
  std::string key{};
  QuerySpec query{};
  BindSpec bind{};

  [[nodiscard]] static View make(
      std::string_view key,
      QuerySpec query,
      BindSpec bind) {
    return View{
        .key = std::string(key),
        .query = std::move(query),
        .bind = std::move(bind),
    };
  }
};

struct Control {
  std::string key{};
  std::string label{};
  std::string feature{};
  ControlKind kind{ControlKind::Category};
  std::optional<std::string> scope{};

  [[nodiscard]] static Control category(
      std::string_view key,
      std::string_view feature,
      std::string_view label = {}) {
    return Control{
        .key = std::string(key),
        .label = std::string(label),
        .feature = std::string(feature),
        .kind = ControlKind::Category,
    };
  }

  [[nodiscard]] static Control numeric_range(
      std::string_view key,
      std::string_view feature,
      std::string_view label = {}) {
    return Control{
        .key = std::string(key),
        .label = std::string(label),
        .feature = std::string(feature),
        .kind = ControlKind::NumericRange,
    };
  }

  [[nodiscard]] static Control temporal_range(
      std::string_view key,
      std::string_view feature,
      std::string_view label = {}) {
    return Control{
        .key = std::string(key),
        .label = std::string(label),
        .feature = std::string(feature),
        .kind = ControlKind::TemporalRange,
    };
  }

  [[nodiscard]] Control scoped(std::string_view value) const {
    Control out = *this;
    out.scope = std::string(value);
    return out;
  }
};

struct Tile {
  std::string key{};
  LayoutSpec layout{};
  std::optional<View> view{};
  std::optional<Control> control{};

  [[nodiscard]] static Tile from_view(
      std::string_view key,
      LayoutSpec layout,
      View view) {
    return Tile{
        .key = std::string(key),
        .layout = layout,
        .view = std::move(view),
    };
  }

  [[nodiscard]] static Tile from_control(
      std::string_view key,
      LayoutSpec layout,
      Control control) {
    return Tile{
        .key = std::string(key),
        .layout = layout,
        .control = std::move(control),
    };
  }
};

struct Board {
  std::string key{};
  std::string title{};
  std::int64_t rows{1};
  std::int64_t cols{1};
  std::vector<Tile> tiles{};

  Board() = default;
  explicit Board(std::string_view key_value)
      : key(key_value) {}

  [[nodiscard]] Board titled(std::string_view value) const {
    Board out = *this;
    out.title = std::string(value);
    return out;
  }

  [[nodiscard]] Board shape(std::int64_t row_count, std::int64_t col_count) const {
    Board out = *this;
    out.rows = row_count;
    out.cols = col_count;
    return out;
  }

  [[nodiscard]] Board add(Tile tile) const {
    Board out = *this;
    out.tiles.push_back(std::move(tile));
    return out;
  }
};

} // namespace pluss::bi
