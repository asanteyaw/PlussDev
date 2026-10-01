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
  Scatter = 5,
  Heatmap = 6,
  Corr = 7,
};

using FilterScalar = std::variant<
    double,
    std::string,
    pluss::Timeline,
    pluss::Duration>;
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

  [[nodiscard]] static Filter eq(std::string_view feature, const pluss::Duration& value) {
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

  [[nodiscard]] static Filter ne(std::string_view feature, const pluss::Duration& value) {
    auto out = eq(feature, value);
    out.op = FilterOp::Ne;
    return out;
  }

  [[nodiscard]] static Filter gt(std::string_view feature, double value) {
    auto out = eq(feature, value);
    out.op = FilterOp::Gt;
    return out;
  }

  [[nodiscard]] static Filter gt(std::string_view feature, const pluss::Duration& value) {
    auto out = eq(feature, value);
    out.op = FilterOp::Gt;
    return out;
  }

  [[nodiscard]] static Filter ge(std::string_view feature, double value) {
    auto out = eq(feature, value);
    out.op = FilterOp::Ge;
    return out;
  }

  [[nodiscard]] static Filter ge(std::string_view feature, const pluss::Duration& value) {
    auto out = eq(feature, value);
    out.op = FilterOp::Ge;
    return out;
  }

  [[nodiscard]] static Filter lt(std::string_view feature, double value) {
    auto out = eq(feature, value);
    out.op = FilterOp::Lt;
    return out;
  }

  [[nodiscard]] static Filter lt(std::string_view feature, const pluss::Duration& value) {
    auto out = eq(feature, value);
    out.op = FilterOp::Lt;
    return out;
  }

  [[nodiscard]] static Filter le(std::string_view feature, double value) {
    auto out = eq(feature, value);
    out.op = FilterOp::Le;
    return out;
  }

  [[nodiscard]] static Filter le(std::string_view feature, const pluss::Duration& value) {
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

  [[nodiscard]] static Filter between_duration(
      std::string_view feature,
      const pluss::Duration& low,
      const pluss::Duration& high) {
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

struct QueryGroupPlan {
  std::vector<std::string> keys{};
  std::vector<pluss::table::DataFrame::GroupSpec> specs{};
  bool dropnull{true};
  bool sort{false};
};

struct QueryPivotPlan {
  std::vector<std::string> index_features{};
  std::vector<std::string> column_features{};
  std::vector<std::string> value_features{};
  pluss::table::GroupOp op{pluss::table::GroupOp::Mean};
  bool dropna{true};
  bool sort{true};
};

struct QueryResamplePlan {
  std::string time_feature{};
  std::vector<std::string> value_features{};
  std::string freq{};
  std::int64_t step{1};
  pluss::subsp::stats::ts::ResampleOp op{
      pluss::subsp::stats::ts::ResampleOp::Mean};
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
  std::optional<QueryGroupPlan> group_plan{};
  std::optional<QueryPivotPlan> pivot_plan{};
  std::optional<QueryResamplePlan> resample_plan{};
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

  [[nodiscard]] QuerySpec group(
      std::initializer_list<std::string_view> keys,
      bool dropnull = true,
      bool sort = false) const {
    QuerySpec out = *this;
    out.pivot_plan.reset();
    out.resample_plan.reset();
    QueryGroupPlan plan;
    plan.dropnull = dropnull;
    plan.sort = sort;
    plan.keys.reserve(keys.size());
    for (const auto key : keys) {
      PLUSS_CHECK(!key.empty(), "pluss::bi::QuerySpec::group: key names cannot be empty");
      plan.keys.emplace_back(key);
    }
    out.group_plan = std::move(plan);
    return out;
  }

  [[nodiscard]] QuerySpec agg(
      std::initializer_list<pluss::table::DataFrame::GroupSpec> specs) const {
    PLUSS_CHECK(group_plan.has_value(),
                "pluss::bi::QuerySpec::agg: call group(...) before agg(...)");
    QuerySpec out = *this;
    out.group_plan->specs.assign(specs.begin(), specs.end());
    PLUSS_CHECK(!out.group_plan->specs.empty(),
                "pluss::bi::QuerySpec::agg: expected at least one GroupSpec");
    return out;
  }

  [[nodiscard]] QuerySpec pivot(
      std::initializer_list<std::string_view> index_features,
      std::initializer_list<std::string_view> column_features,
      std::initializer_list<std::string_view> value_features,
      pluss::table::GroupOp op = pluss::table::GroupOp::Mean,
      bool dropna = true,
      bool sort = true) const {
    QuerySpec out = *this;
    out.group_plan.reset();
    out.resample_plan.reset();
    QueryPivotPlan plan;
    plan.op = op;
    plan.dropna = dropna;
    plan.sort = sort;
    plan.index_features.reserve(index_features.size());
    plan.column_features.reserve(column_features.size());
    plan.value_features.reserve(value_features.size());
    for (const auto value : index_features) {
      PLUSS_CHECK(!value.empty(), "pluss::bi::QuerySpec::pivot: index feature names cannot be empty");
      plan.index_features.emplace_back(value);
    }
    for (const auto value : column_features) {
      PLUSS_CHECK(!value.empty(), "pluss::bi::QuerySpec::pivot: column feature names cannot be empty");
      plan.column_features.emplace_back(value);
    }
    for (const auto value : value_features) {
      PLUSS_CHECK(!value.empty(), "pluss::bi::QuerySpec::pivot: value feature names cannot be empty");
      plan.value_features.emplace_back(value);
    }
    out.pivot_plan = std::move(plan);
    return out;
  }

  [[nodiscard]] QuerySpec resample(
      std::string_view time_feature,
      std::string_view freq,
      pluss::subsp::stats::ts::ResampleOp op =
          pluss::subsp::stats::ts::ResampleOp::Mean,
      std::int64_t step = 1) const {
    QuerySpec out = *this;
    out.group_plan.reset();
    out.pivot_plan.reset();
    QueryResamplePlan plan;
    PLUSS_CHECK(!time_feature.empty(),
                "pluss::bi::QuerySpec::resample: time feature cannot be empty");
    PLUSS_CHECK(!freq.empty(),
                "pluss::bi::QuerySpec::resample: frequency cannot be empty");
    PLUSS_CHECK(step > 0,
                "pluss::bi::QuerySpec::resample: step must be > 0");
    plan.time_feature = std::string(time_feature);
    plan.freq = std::string(freq);
    plan.step = step;
    plan.op = op;
    out.resample_plan = std::move(plan);
    return out;
  }

  [[nodiscard]] QuerySpec resample(
      std::string_view time_feature,
      std::initializer_list<std::string_view> value_features,
      std::string_view freq,
      pluss::subsp::stats::ts::ResampleOp op =
          pluss::subsp::stats::ts::ResampleOp::Mean,
      std::int64_t step = 1) const {
    auto out = resample(time_feature, freq, op, step);
    out.resample_plan->value_features.reserve(value_features.size());
    for (const auto feature : value_features) {
      PLUSS_CHECK(!feature.empty(),
                  "pluss::bi::QuerySpec::resample: value feature names cannot be empty");
      out.resample_plan->value_features.emplace_back(feature);
    }
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
  std::vector<std::string> analysis_features{};
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

  [[nodiscard]] static BindSpec scatter(
      const pluss::table::FlashTableColumns& columns = {},
      std::string_view title = {}) {
    return BindSpec{
        .kind = ViewKind::Scatter,
        .title = std::string(title),
        .columns = columns,
    };
  }

  [[nodiscard]] static BindSpec heatmap(
      const pluss::table::FlashTableColumns& columns = {},
      std::string_view title = {}) {
    return BindSpec{
        .kind = ViewKind::Heatmap,
        .title = std::string(title),
        .columns = columns,
    };
  }

  [[nodiscard]] static BindSpec corr(
      std::initializer_list<std::string_view> features = {},
      std::string_view title = {}) {
    BindSpec out;
    out.kind = ViewKind::Corr;
    out.title = std::string(title);
    out.analysis_features.reserve(features.size());
    for (const auto feature : features) {
      out.analysis_features.emplace_back(feature);
    }
    return out;
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

class View {
public:
  View() = default;
  explicit View(std::string_view key)
      : key_(key) {}

  [[nodiscard]] const std::string& key() const noexcept {
    return key_;
  }

  View& key(std::string_view value) {
    key_ = std::string(value);
    return *this;
  }

  [[nodiscard]] const QuerySpec& query() const noexcept {
    return query_;
  }

  View& query(QuerySpec value) {
    query_ = std::move(value);
    return *this;
  }

  [[nodiscard]] const BindSpec& bind() const noexcept {
    return bind_;
  }

  [[nodiscard]] const LayoutSpec& layout() const noexcept {
    return layout_;
  }

  View& place(
      std::int64_t row,
      std::int64_t col,
      std::int64_t row_span = 1,
      std::int64_t col_span = 1) {
    layout_ = LayoutSpec::at(row, col, row_span, col_span);
    return *this;
  }

  View& title(std::string_view value) {
    bind_.title = std::string(value);
    return *this;
  }

  View& feature(std::string_view value) {
    if (bind_.kind == ViewKind::Kpi) {
      bind_.feature = std::string(value);
    } else {
      bind_.columns.feature = std::string(value);
    }
    return *this;
  }

  View& x(std::string_view value) {
    bind_.columns.x_feature = std::string(value);
    return *this;
  }

  View& y(std::string_view value) {
    bind_.columns.y_feature = std::string(value);
    return *this;
  }

  View& z(std::string_view value) {
    bind_.columns.z_feature = std::string(value);
    return *this;
  }

  View& hue(std::string_view value) {
    bind_.columns.hue_feature = std::string(value);
    return *this;
  }

  View& features(std::initializer_list<std::string_view> features) {
    bind_.analysis_features.clear();
    bind_.analysis_features.reserve(features.size());
    for (const auto feature : features) {
      bind_.analysis_features.emplace_back(feature);
    }
    return *this;
  }

  View& horizontal(bool enable = true) {
    bind_.horizontal = enable;
    return *this;
  }

  View& op(pluss::table::GroupOp value) {
    bind_.op = value;
    return *this;
  }

  View& pivot_index(std::initializer_list<std::string_view> features) {
    bind_.index_features.clear();
    bind_.index_features.reserve(features.size());
    for (const auto feature : features) {
      bind_.index_features.emplace_back(feature);
    }
    return *this;
  }

  View& pivot_columns(std::initializer_list<std::string_view> features) {
    bind_.column_features.clear();
    bind_.column_features.reserve(features.size());
    for (const auto feature : features) {
      bind_.column_features.emplace_back(feature);
    }
    return *this;
  }

  View& pivot_values(std::initializer_list<std::string_view> features) {
    bind_.value_features.clear();
    bind_.value_features.reserve(features.size());
    for (const auto feature : features) {
      bind_.value_features.emplace_back(feature);
    }
    return *this;
  }

  View& dropna(bool enable = true) {
    bind_.dropna = enable;
    return *this;
  }

  View& sort(bool enable = true) {
    bind_.sort = enable;
    return *this;
  }

  [[nodiscard]] static View make(
      std::string_view key,
      QuerySpec query,
      BindSpec bind) {
    View out(key);
    out.query_ = std::move(query);
    out.bind_ = std::move(bind);
    return out;
  }

  [[nodiscard]] static View kpi(std::string_view key) {
    View out(key);
    out.bind_.kind = ViewKind::Kpi;
    return out;
  }

  [[nodiscard]] static View line(std::string_view key) {
    View out(key);
    out.bind_ = BindSpec::line();
    return out;
  }

  [[nodiscard]] static View bar(std::string_view key) {
    View out(key);
    out.bind_ = BindSpec::bar();
    return out;
  }

  [[nodiscard]] static View scatter(std::string_view key) {
    View out(key);
    out.bind_ = BindSpec::scatter();
    return out;
  }

  [[nodiscard]] static View heatmap(std::string_view key) {
    View out(key);
    out.bind_ = BindSpec::heatmap();
    return out;
  }

  [[nodiscard]] static View corr(std::string_view key) {
    View out(key);
    out.bind_ = BindSpec::corr();
    return out;
  }

  [[nodiscard]] static View table(std::string_view key) {
    View out(key);
    out.bind_ = BindSpec::detail_table();
    return out;
  }

  [[nodiscard]] static View pivot(std::string_view key) {
    View out(key);
    out.bind_.kind = ViewKind::PivotTable;
    out.bind_.op = pluss::table::GroupOp::Mean;
    out.bind_.dropna = true;
    out.bind_.sort = true;
    return out;
  }

private:
  std::string key_{};
  QuerySpec query_{};
  BindSpec bind_{};
  LayoutSpec layout_{};
};

class Control {
public:
  Control() = default;
  explicit Control(std::string_view key)
      : key_(key) {}

  [[nodiscard]] const std::string& key() const noexcept {
    return key_;
  }

  Control& key(std::string_view value) {
    key_ = std::string(value);
    return *this;
  }

  [[nodiscard]] const std::string& label() const noexcept {
    return label_;
  }

  Control& label(std::string_view value) {
    label_ = std::string(value);
    return *this;
  }

  [[nodiscard]] const std::string& feature() const noexcept {
    return feature_;
  }

  Control& feature(std::string_view value) {
    feature_ = std::string(value);
    return *this;
  }

  [[nodiscard]] ControlKind kind() const noexcept {
    return kind_;
  }

  Control& kind(ControlKind value) {
    kind_ = value;
    return *this;
  }

  [[nodiscard]] const std::optional<std::string>& scope() const noexcept {
    return scope_;
  }

  Control& scope(std::string_view value) {
    scope_ = std::string(value);
    return *this;
  }

  [[nodiscard]] const LayoutSpec& layout() const noexcept {
    return layout_;
  }

  Control& place(
      std::int64_t row,
      std::int64_t col,
      std::int64_t row_span = 1,
      std::int64_t col_span = 1) {
    layout_ = LayoutSpec::at(row, col, row_span, col_span);
    return *this;
  }

  [[nodiscard]] static Control category(
      std::string_view key,
      std::string_view feature,
      std::string_view label = {}) {
    Control out(key);
    out.kind_ = ControlKind::Category;
    out.feature_ = std::string(feature);
    out.label_ = std::string(label);
    return out;
  }

  [[nodiscard]] static Control numeric_range(
      std::string_view key,
      std::string_view feature,
      std::string_view label = {}) {
    Control out(key);
    out.kind_ = ControlKind::NumericRange;
    out.feature_ = std::string(feature);
    out.label_ = std::string(label);
    return out;
  }

  [[nodiscard]] static Control temporal_range(
      std::string_view key,
      std::string_view feature,
      std::string_view label = {}) {
    Control out(key);
    out.kind_ = ControlKind::TemporalRange;
    out.feature_ = std::string(feature);
    out.label_ = std::string(label);
    return out;
  }

  [[nodiscard]] Control scoped(std::string_view value) const {
    Control out = *this;
    out.scope_ = std::string(value);
    return out;
  }

private:
  std::string key_{};
  std::string label_{};
  std::string feature_{};
  ControlKind kind_{ControlKind::Category};
  std::optional<std::string> scope_{};
  LayoutSpec layout_{};
};

class Tile {
public:
  Tile() = default;

  [[nodiscard]] const std::string& key() const noexcept {
    return key_;
  }

  [[nodiscard]] const LayoutSpec& layout() const noexcept {
    return layout_;
  }

  [[nodiscard]] const std::optional<View>& view() const noexcept {
    return view_;
  }

  [[nodiscard]] const std::optional<Control>& control() const noexcept {
    return control_;
  }

  [[nodiscard]] static Tile from_view(
      std::string_view key,
      LayoutSpec layout,
      View view) {
    Tile out;
    out.key_ = std::string(key);
    out.layout_ = layout;
    out.view_ = std::move(view);
    return out;
  }

  [[nodiscard]] static Tile from_control(
      std::string_view key,
      LayoutSpec layout,
      Control control) {
    Tile out;
    out.key_ = std::string(key);
    out.layout_ = layout;
    out.control_ = std::move(control);
    return out;
  }

private:
  std::string key_{};
  LayoutSpec layout_{};
  std::optional<View> view_{};
  std::optional<Control> control_{};
};

class Board {
public:
  Board() = default;
  explicit Board(std::string_view key_value)
      : key_(key_value) {}

  [[nodiscard]] const std::string& key() const noexcept {
    return key_;
  }

  [[nodiscard]] const std::string& title() const noexcept {
    return title_;
  }

  Board& title(std::string_view value) {
    title_ = std::string(value);
    ++version_;
    return *this;
  }

  [[nodiscard]] std::int64_t nrows() const noexcept {
    return rows_;
  }

  [[nodiscard]] std::int64_t ncols() const noexcept {
    return cols_;
  }

  [[nodiscard]] std::uint64_t version() const noexcept {
    return version_;
  }

  [[nodiscard]] const std::vector<Tile>& tiles() const noexcept {
    return tiles_;
  }

  Board& grid(std::int64_t row_count, std::int64_t col_count) {
    rows_ = row_count;
    cols_ = col_count;
    ++version_;
    return *this;
  }

  Board& control(const Control& value) {
    tiles_.push_back(Tile::from_control(value.key(), value.layout(), value));
    ++version_;
    return *this;
  }

  Board& view(const View& value) {
    tiles_.push_back(Tile::from_view(value.key(), value.layout(), value));
    ++version_;
    return *this;
  }

private:
  std::string key_{};
  std::string title_{};
  std::int64_t rows_{1};
  std::int64_t cols_{1};
  std::uint64_t version_{0};
  std::vector<Tile> tiles_{};
};

[[nodiscard]] inline QuerySpec from(std::string_view source) {
  return QuerySpec::from(source);
}

[[nodiscard]] inline QuerySpec stored(std::string_view name) {
  return QuerySpec::stored(name);
}

[[nodiscard]] inline QuerySpec sql(std::string_view statement) {
  return QuerySpec::sql(statement);
}

[[nodiscard]] inline QuerySpec traverse_from(std::string_view source) {
  return QuerySpec::traverse_from(source);
}

[[nodiscard]] inline Control category(std::string_view key) {
  auto out = Control(key);
  out.kind(ControlKind::Category);
  return out;
}

[[nodiscard]] inline Control range(std::string_view key) {
  auto out = Control(key);
  out.kind(ControlKind::NumericRange);
  return out;
}

[[nodiscard]] inline Control time_range(std::string_view key) {
  auto out = Control(key);
  out.kind(ControlKind::TemporalRange);
  return out;
}

[[nodiscard]] inline View kpi(std::string_view key) {
  return View::kpi(key);
}

[[nodiscard]] inline View line(std::string_view key) {
  return View::line(key);
}

[[nodiscard]] inline View bar(std::string_view key) {
  return View::bar(key);
}

[[nodiscard]] inline View scatter(std::string_view key) {
  return View::scatter(key);
}

[[nodiscard]] inline View heatmap(std::string_view key) {
  return View::heatmap(key);
}

[[nodiscard]] inline View corr(std::string_view key) {
  return View::corr(key);
}

[[nodiscard]] inline View table(std::string_view key) {
  return View::table(key);
}

[[nodiscard]] inline View pivot(std::string_view key) {
  return View::pivot(key);
}

} // namespace pluss::bi
