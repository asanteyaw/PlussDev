#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iosfwd>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "MSharp/IntrusivePtr.h"
#include "loader/Config.h"
#include "loader/CsvLoader.h"
#include "loader/ExcelLoader.h"
#include "table/Thread.h"
#include "table/display/Display.h"
#include "pluss/codem/stack_array.h"
#include "pluss/codem/codem.h"

namespace pluss::loader {
class LoadResult;
}
namespace pluss::pl {
class Studio;
}
namespace pluss::kindle {
class Catalog;
struct KindelizePolicy;
struct TabelizePolicy;
}

namespace pluss::table {

namespace detail {
template <class T>
struct IsInitializerList : std::false_type {};
template <class T>
struct IsInitializerList<std::initializer_list<T>> : std::true_type {};
template <class T>
inline constexpr bool IsInitializerListV = IsInitializerList<std::remove_cvref_t<T>>::value;
}

namespace detail {
class DataFrameImpl;
}
class DataFrame;
class DataFrameView;
class DataFramePlotAccessor;
class DataFramePlot3DAccessor;
class AtlasPlotAccessor;
struct DataFramePlotOptions;
class DataFrameSqlAccessor;
class DataFrameSqlPrepared;
class DataFrameTsAccessor;
class DataFrameTsResampler;
class Atlas;
struct FrameMatch;

enum class FrameMatchRule : std::uint8_t {
  Unite = 0,
  Common = 1,
  KeepLeft = 2,
  KeepRight = 3,
};

enum class RepeatedCaseRule : std::uint8_t {
  PairInOrder = 0,
  Reject = 1,
};

struct FrameMatchOptions {
  FrameMatchRule cases{FrameMatchRule::Unite};
  FrameMatchRule features{FrameMatchRule::Unite};
  RepeatedCaseRule repeated_cases{RepeatedCaseRule::PairInOrder};
};

struct CategoryEncoding {
  Thread codes{};
  Thread levels{};
  bool ordered{false};

  Thread decode() const;
};

class ColumnRef {
public:
  ColumnRef(std::string name, Thread column);

  const std::string& name() const noexcept;
  const Thread& thread() const noexcept;

  pluss::Series mean() const;
  Thread head(std::int64_t n) const;
  ThreadValue value() const;
  Thread values() const;

private:
  std::string name_;
  Thread column_;
};

class DataFrame {
public:
  struct MatrixValues : std::tuple<
      std::vector<pluss::Series>,
      std::vector<pluss::Note>,
      std::vector<pluss::Timeline>,
      std::vector<pluss::TimeIndex>> {
    using Base = std::tuple<
        std::vector<pluss::Series>,
        std::vector<pluss::Note>,
        std::vector<pluss::Timeline>,
        std::vector<pluss::TimeIndex>>;
    using Base::Base;

    std::span<const pluss::Series> series_span() const noexcept { return std::get<0>(*this); }
    std::span<const pluss::Note> note_span() const noexcept { return std::get<1>(*this); }
    std::span<const pluss::Timeline> timeline_span() const noexcept { return std::get<2>(*this); }
    std::span<const pluss::TimeIndex> timeindex_span() const noexcept { return std::get<3>(*this); }

    std::size_t size() const noexcept {
      return std::get<0>(*this).size() + std::get<1>(*this).size()
             + std::get<2>(*this).size() + std::get<3>(*this).size();
    }

    pluss::Series as_series() const;
    pluss::Note as_note() const;
    pluss::Timeline as_timeline() const;
    pluss::TimeIndex as_timeindex() const;
  };

  class LetSlot {
  public:
    LetSlot(DataFrame& owner, std::string name);

    LetSlot& operator=(const Thread& value);
    LetSlot& operator=(const ThreadValue& value);
    LetSlot& operator=(const pluss::Series& value);
    LetSlot& operator=(const pluss::Note& value);
    LetSlot& operator=(const pluss::Timeline& value);
    LetSlot& operator=(const pluss::TimeIndex& value);
    LetSlot& operator=(const DataFrame& frame);

  private:
    DataFrame* owner_{nullptr};
    std::string name_{};
  };

  class LetSlots {
  public:
    LetSlots(DataFrame& owner, std::vector<std::string> names);

    LetSlots& operator=(const DataFrame& frame);
    LetSlots& operator=(std::initializer_list<Thread> values);
    LetSlots& operator=(const std::vector<Thread>& values);

  private:
    void assign_from_threads(std::span<const Thread> values);

    DataFrame* owner_{nullptr};
    std::vector<std::string> names_{};
  };

  enum class SummaryOp : std::uint8_t {
    Count = 0,
    Nunique = 1,
    Sum = 2,
    Mean = 3,
    Median = 4,
    Min = 5,
    Max = 6,
    Std = 7,
    Var = 8,
    Prod = 9,
  };

  enum class GroupOp : std::uint8_t {
    Count = 0,
    Nunique = 1,
    Sum = 2,
    Mean = 3,
    Min = 4,
    Max = 5,
    Std = 6,
    Var = 7,
  };

  enum class CorrMethod : std::uint8_t {
    Pearson = 0,
    Spearman = 1,
    KendallTau = 2,
  };

  enum class MissingOp : std::uint8_t {
    Fill = 0,
    ForwardFill = 1,
    BackwardFill = 2,
    DropAny = 3,
    DropAll = 4,
    IsNull = 5,
    NotNull = 6,
  };

  enum class CombineOp : std::uint8_t {
    Features = 0,
    Update = 1,
  };

  struct SummarySpec {
    std::string output;
    std::string feature;
    SummaryOp op{SummaryOp::Mean};
  };

  struct GroupSpec {
    std::string output;
    std::string feature;
    GroupOp op{GroupOp::Count};
  };

  struct DeriveSpec {
    std::string feature;
    std::function<Thread(const DataFrame&)> expr;
  };

  struct TransformSpec {
    std::string feature;
    std::function<Thread(const Thread&)> expr;
  };

  struct MissingSpec {
    MissingOp op{MissingOp::DropAny};
    std::vector<std::string> features{};
    std::optional<double> value{std::nullopt};
  };

  struct ArrangeSpec {
    std::string feature;
    bool ascending{true};
    bool nulls_last{true};
  };

  enum class ReorderPlacement : std::uint8_t {
    Front = 0,
    Back = 1,
    Before = 2,
    After = 3,
  };

  struct ReorderSpec {
    std::vector<std::string> features{};
    std::string pattern{};
    ReorderPlacement placement{ReorderPlacement::Front};
    std::string anchor{};

    static ReorderSpec move(std::string_view feature_name) {
      ReorderSpec spec;
      spec.features.emplace_back(feature_name);
      return spec;
    }

    static ReorderSpec move(std::initializer_list<std::string_view> feature_names) {
      ReorderSpec spec;
      spec.features.reserve(feature_names.size());
      for (const auto name : feature_names) {
        spec.features.emplace_back(name);
      }
      return spec;
    }

    static ReorderSpec move(pluss::RangeView<std::string_view> feature_names) {
      ReorderSpec spec;
      spec.features.reserve(feature_names.size());
      for (const auto name : feature_names) {
        spec.features.emplace_back(name);
      }
      return spec;
    }

    static ReorderSpec move_regex(std::string_view regex_pattern) {
      ReorderSpec spec;
      spec.pattern = std::string(regex_pattern);
      return spec;
    }

    [[nodiscard]] ReorderSpec first() const {
      auto out = *this;
      out.placement = ReorderPlacement::Front;
      out.anchor.clear();
      return out;
    }

    [[nodiscard]] ReorderSpec last() const {
      auto out = *this;
      out.placement = ReorderPlacement::Back;
      out.anchor.clear();
      return out;
    }

    [[nodiscard]] ReorderSpec before(std::string_view feature_name) const {
      auto out = *this;
      out.placement = ReorderPlacement::Before;
      out.anchor = std::string(feature_name);
      return out;
    }

    [[nodiscard]] ReorderSpec after(std::string_view feature_name) const {
      auto out = *this;
      out.placement = ReorderPlacement::After;
      out.anchor = std::string(feature_name);
      return out;
    }
  };

  struct CaseSelector {
    enum class Kind : std::uint8_t {
      PositionScalar = 0,
      PositionSlice = 1,
      PositionList = 2,
      LabelScalar = 3,
      LabelSlice = 4,
      LabelList = 5,
      Mask = 6,
    };

    Kind kind{Kind::PositionSlice};
    std::int64_t first{0};
    std::int64_t second{0};
    std::vector<std::int64_t> positions{};
    std::vector<std::string> labels{};
    Thread mask{};
  };

  struct FeatureSelector {
    enum class Kind : std::uint8_t {
      All = 0,
      PositionScalar = 1,
      PositionSlice = 2,
      PositionList = 3,
      NameScalar = 4,
      NameSlice = 5,
      NameList = 6,
    };

    Kind kind{Kind::All};
    std::int64_t first{0};
    std::int64_t second{0};
    std::vector<std::int64_t> positions{};
    std::vector<std::string> names{};
  };

  struct CastSpec {
    std::string feature;
    ThreadCastTarget target{ThreadCastTarget::Series};
    std::optional<pluss::codem::ScalarType> series_dtype{std::nullopt};
  };

  struct WindowSpec {
    std::string output;
    std::string feature;
    ThreadWindowOp op{ThreadWindowOp::RollingMean};
    std::int64_t window{3};
    std::int64_t min_periods{1};
  };

  struct CombineSpec {
    CombineOp op{CombineOp::Features};
    std::string collision_suffix{"_right"};
    bool overwrite{true};
    FrameMatchRule cases{FrameMatchRule::Unite};
  };

  struct SqlOptions {
    enum class MissingParamPolicy : std::uint8_t {
      Error = 0,
      Null = 1,
    };

    enum class CollisionPolicy : std::uint8_t {
      Error = 0,
      Suffix = 1,
      Overwrite = 2,
    };

    enum class ResultMode : std::uint8_t {
      Materialized = 0,
      Chunked = 1,
    };

    struct SourceBinding {
      std::string name{};
      const DataFrame* frame{nullptr};
    };

    using SqlParam = std::variant<std::nullptr_t, bool, std::int64_t, double, std::string>;

    // Additional named sources available to JOIN in SQL.
    std::vector<SourceBinding> sources{};
    // Template-style parameters used in SQL via `{param_name}` placeholders.
    std::unordered_map<std::string, SqlParam> params{};
    // Identifier placeholders via `{@ident_name}`.
    std::unordered_map<std::string, std::string> identifier_params{};
    MissingParamPolicy missing_param_policy{MissingParamPolicy::Error};
    bool reject_unused_params{false};
    CollisionPolicy collision_policy{CollisionPolicy::Error};
    std::optional<std::int64_t> max_intermediate_rows{std::nullopt};
    std::optional<std::int64_t> max_result_rows{std::nullopt};
    std::optional<std::int64_t> memory_budget_mb{std::nullopt};
    std::optional<std::int64_t> timeout_ms{std::nullopt};
    bool diagnostics_explain{false};
    bool diagnostics_profile{false};
    bool diagnostics_trace_tokens{false};
    bool diagnostics_trace_plan{false};
    bool enable_plan_cache{true};
    std::size_t plan_cache_capacity{256};
    ResultMode result_mode{ResultMode::Materialized};
    std::int64_t chunk_rows{1024};
    // Names that map to the case axis when projected in SELECT.
    // Example: case_axis_aliases={"case","idx"} enables `SELECT idx FROM df`.
    std::vector<std::string> case_axis_aliases{"case"};
  };

  using QueryFn = std::function<pluss::Series(const DataFrame&)>;

  DataFrame();
  explicit DataFrame(std::vector<std::pair<std::string, Thread>> columns,
                     std::optional<Thread> cases = std::nullopt);
  DataFrame(const DataFrame& other);
  DataFrame(DataFrame&& other) noexcept;
  DataFrame& operator=(const DataFrame& other);
  DataFrame& operator=(DataFrame&& other) noexcept;
  ~DataFrame();

  static DataFrame load(std::string_view source_type, std::string_view filepath);
  static DataFrame load(std::string_view source_type,
                        std::string_view filepath,
                        const pluss::loader::Config& config);
  static DataFrame load(std::string_view source_type,
                        std::string_view filepath,
                        std::initializer_list<pluss::loader::ConfigEntry> config_entries);
  static DataFrame load(std::string_view source_type,
                        std::string_view filepath,
                        const pluss::loader::ReadCsvOptions& options);
  static DataFrame load(std::string_view source_type,
                        std::string_view filepath,
                        char sep,
                        std::optional<std::size_t> ncases = std::nullopt,
                        std::vector<std::string> usefeats = {},
                        std::optional<std::string> case_col = std::nullopt,
                        pluss::loader::BadLinePolicy on_bad_lines = pluss::loader::BadLinePolicy::Error);
  static DataFrame load(std::string_view source_type,
                        std::string_view filepath,
                        char sep,
                        std::optional<std::size_t> ncases,
                        pluss::msharp::RangeView::Of<std::string_view> usefeats,
                        std::optional<std::string> case_col = std::nullopt,
                        pluss::loader::BadLinePolicy on_bad_lines = pluss::loader::BadLinePolicy::Error);
  static DataFrame load_csv(std::string_view filepath);
  static DataFrame load_csv(std::string_view filepath, const pluss::loader::ReadCsvOptions& options);
  static DataFrame load_csv(std::string_view filepath, const pluss::loader::Config& config);
  static DataFrame load_csv(std::string_view filepath,
                            std::initializer_list<pluss::loader::ConfigEntry> config_entries);
  static DataFrame load_csv(std::string_view filepath,
                            std::string_view header,
                            std::string_view case_feature = "none",
                            std::string_view engine = "auto",
                            std::optional<std::size_t> ncases = std::nullopt);
  static DataFrame load_csv(std::string_view filepath,
                            std::string_view header,
                            std::size_t ncases);
  static DataFrame load_csv(std::string_view filepath,
                            char sep,
                            std::optional<std::size_t> ncases = std::nullopt,
                            std::vector<std::string> usefeats = {},
                            std::optional<std::string> case_col = std::nullopt,
                            pluss::loader::BadLinePolicy on_bad_lines = pluss::loader::BadLinePolicy::Error);
  static DataFrame load_csv(std::string_view filepath,
                            char sep,
                            std::optional<std::size_t> ncases,
                            pluss::msharp::RangeView::Of<std::string_view> usefeats,
                            std::optional<std::string> case_col = std::nullopt,
                            pluss::loader::BadLinePolicy on_bad_lines = pluss::loader::BadLinePolicy::Error);
  static DataFrame load_txt(std::string_view filepath);
  static DataFrame load_txt(std::string_view filepath, const pluss::loader::Config& config);
  static DataFrame load_txt(std::string_view filepath,
                            std::initializer_list<pluss::loader::ConfigEntry> config_entries);
  static DataFrame load_excel(std::string_view filepath);
  static DataFrame load_excel(std::string_view filepath, const pluss::loader::ReadExcelOptions& options);
  static DataFrame load_excel(std::string_view filepath, const pluss::loader::Config& config);
  static DataFrame load_excel(std::string_view filepath,
                              std::initializer_list<pluss::loader::ConfigEntry> config_entries);
  static DataFrame load_excel(std::string_view filepath,
                              std::string_view sheet_name,
                              bool header = true);
  static DataFrame load_excel(std::string_view filepath,
                              std::int64_t sheet_index,
                              bool header = true);
  static pluss::loader::LoadResult load_excel(std::string_view filepath,
                                              std::nullptr_t,
                                              bool header = true);
  static pluss::loader::LoadResult load_excel(std::string_view filepath,
                                              const std::vector<std::string>& sheet_names,
                                              bool header = true);
  static pluss::loader::LoadResult load_excel(std::string_view filepath,
                                              std::initializer_list<std::string_view> sheet_names,
                                              bool header = true);
  static pluss::loader::LoadResult load_excel(std::string_view filepath,
                                              pluss::msharp::RangeView::Of<std::string_view> sheet_names,
                                              bool header = true);
  static pluss::loader::LoadResult load_excel(std::string_view filepath,
                                              const std::vector<std::int64_t>& sheet_indices,
                                              bool header = true);
  static pluss::loader::LoadResult load_excel(std::string_view filepath,
                                              std::initializer_list<std::int64_t> sheet_indices,
                                              bool header = true);
  static pluss::loader::LoadResult load_excel(std::string_view filepath,
                                              pluss::msharp::RangeView::Int sheet_indices,
                                              bool header = true);
  static DataFrame load_parquet(std::string_view filepath);
  static DataFrame load_parquet(std::string_view filepath, const pluss::loader::Config& config);
  static DataFrame load_parquet(std::string_view filepath,
                                std::initializer_list<pluss::loader::ConfigEntry> config_entries);
  static DataFrame load_json(std::string_view filepath);
  static DataFrame load_json(std::string_view filepath, const pluss::loader::Config& config);
  static DataFrame load_json(std::string_view filepath,
                             std::initializer_list<pluss::loader::ConfigEntry> config_entries);
  static DataFrame tabelize(const pluss::kindle::Catalog& catalog);
  static DataFrame tabelize(const pluss::kindle::Catalog& catalog,
                            const pluss::kindle::TabelizePolicy& policy);
  pluss::kindle::Catalog kindelize() const;
  pluss::kindle::Catalog kindelize(const pluss::kindle::KindelizePolicy& policy) const;

  bool defined() const noexcept;
  bool unique() const noexcept;
  std::int64_t height() const;
  std::int64_t ncases() const;
  std::int64_t nfeats() const;
  std::int64_t nfeatures() const;
  MatrixValues matrix() const;
  const std::vector<std::string>& feature_names() const;
  std::vector<Thread> features() const;
  const Thread& cases() const;
  pluss::Series cases_series() const;
  DataFrame case_study(const CaseSelector& rows) const;
  DataFrame case_study(const CaseSelector& rows, const FeatureSelector& features) const;
  std::optional<Thread> get(std::string_view feature_name) const;

  DataFrame with_cases(const Thread& cases) const;
  DataFrame& with_cases_(const Thread& cases);
  std::string title() const;
  DataFrame title(std::string title) const;
  DataFrame& title_(std::string title);

  bool has_feature(std::string_view name) const;
  Thread features(std::string_view name);
  Thread features(std::string_view name) const;
  DataFrame features(pluss::msharp::RangeView::Of<std::string_view> names);
  DataFrame features(pluss::msharp::RangeView::Of<std::string_view> names) const;
  DataFrame features(StackArray<std::string_view> names);
  DataFrame features(StackArray<std::string_view> names) const;
  template <class NameRange>
    requires(std::ranges::input_range<NameRange> &&
             !std::is_convertible_v<NameRange, std::string_view> &&
             !detail::IsInitializerListV<NameRange> &&
             !std::same_as<std::remove_cvref_t<NameRange>, StackArray<std::string_view>> &&
             std::convertible_to<std::remove_cvref_t<std::ranges::range_reference_t<NameRange>>, std::string_view>)
  DataFrame features(const NameRange& names) {
    return features(StackArray<std::string_view>(names));
  }
  template <class NameRange>
    requires(std::ranges::input_range<NameRange> &&
             !std::is_convertible_v<NameRange, std::string_view> &&
             !detail::IsInitializerListV<NameRange> &&
             !std::same_as<std::remove_cvref_t<NameRange>, StackArray<std::string_view>> &&
             std::convertible_to<std::remove_cvref_t<std::ranges::range_reference_t<NameRange>>, std::string_view>)
  DataFrame features(const NameRange& names) const {
    return features(StackArray<std::string_view>(names));
  }
  ColumnRef feature_ref(std::string_view name) const;
  DataFrame select(pluss::msharp::RangeView::Of<std::string_view> names) const;
  DataFrame select_like(std::string_view needle, bool case_sensitive = true) const;
  DataFrame select_regex(std::string_view pattern) const;
  DataFrame features(std::initializer_list<std::string_view> names);
  DataFrame features(std::initializer_list<std::string_view> names) const;
  DataFrame features(std::initializer_list<const char*> names);
  DataFrame features(std::initializer_list<const char*> names) const;
  DataFrame series_features() const;
  DataFrame note_features() const;
  DataFrame timeline_features() const;
  DataFrame cast_series(pluss::msharp::RangeView::Of<std::string_view> features,
                        std::optional<pluss::codem::ScalarType> dtype = std::nullopt) const;
  DataFrame cast_note(pluss::msharp::RangeView::Of<std::string_view> features) const;
  DataFrame cast_timeline(pluss::msharp::RangeView::Of<std::string_view> features) const;
  DataFrame cast_timeindex(pluss::msharp::RangeView::Of<std::string_view> features) const;
  DataFrame astype(pluss::msharp::RangeView::Of<std::string_view> features,
                   pluss::codem::ScalarType dtype) const;
  DataFrame insert(std::int64_t feature_index, std::string name, const Thread& feature) const;
  DataFrame place(std::string name,
                  const Thread& feature,
                  std::string_view anchor_feature,
                  bool after = false) const;
  DataFrame place(std::string_view name,
                  std::string_view anchor_feature,
                  bool after = false) const;
  DataFrame drop(pluss::msharp::RangeView::Of<std::string_view> names) const;
  DataFrame reorder(pluss::msharp::RangeView::Of<std::string_view> names) const;
  DataFrame reorder(const ReorderSpec& spec) const;
  DataFrame reorder(pluss::RangeView<ReorderSpec> specs) const;
  DataFrame reorder(std::initializer_list<ReorderSpec> specs) const {
    return reorder(pluss::RangeView<ReorderSpec>(specs));
  }
  DataFrame bucketize(pluss::msharp::RangeView::Double bins,
                      pluss::msharp::RangeView::Int labels = {},
                      bool right = true,
                      bool include_lowest = true) const;
  DataFrame bucketize(std::string_view feature_name,
                      pluss::msharp::RangeView::Double bins,
                      pluss::msharp::RangeView::Int labels = {},
                      bool right = true,
                      bool include_lowest = true) const;
  DataFrame bucketize(pluss::msharp::RangeView::Of<std::string_view> feature_names,
                      pluss::msharp::RangeView::Double bins,
                      pluss::msharp::RangeView::Int labels = {},
                      bool right = true,
                      bool include_lowest = true) const;
  DataFrame add_prefix(std::string_view prefix) const;
  DataFrame add_suffix(std::string_view suffix) const;
  DataFrame sort_values(pluss::msharp::RangeView::Of<std::string_view> features,
                        bool ascending = true,
                        bool nulls_last = true) const;
  DataFrame sort_values(pluss::msharp::RangeView::Of<ArrangeSpec> specs) const;
  DataFrame sort_cases(bool ascending = true, bool nulls_last = true) const;
  DataFrame top(std::string_view feature, std::int64_t n) const;
  DataFrame bottom(std::string_view feature, std::int64_t n) const;
  pluss::Series repeated_cases(pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame unique_cases(pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrameView observe(std::int64_t row, bool render = true) const;
  DataFrameView observe(std::int64_t start, std::int64_t stop, bool render = true) const;
  DataFrame set_cases(std::string_view feature_name, bool drop_feature = true) const;
  DataFrame reset_cases(std::string case_feature = "case") const;
  FrameMatch matched_with(const DataFrame& other,
                          const FrameMatchOptions& options = {}) const;
  DataFrame align_cases(const Thread& new_cases) const;
  DataFrame join(const DataFrame& other, std::string collision_suffix = "_right") const;
  DataFrame update(const DataFrame& other, bool overwrite = true) const;
  DataFrame unify(std::string_view left_on,
                  const DataFrame& right,
                  std::string_view right_on = "",
                  std::string_view how = "common",
                  std::string right_suffix = "_right",
                  std::string_view validate = "many_to_many") const;
  DataFrame unify(pluss::msharp::RangeView::Of<std::string_view> left_on,
                  const DataFrame& right,
                  pluss::msharp::RangeView::Of<std::string_view> right_on = {},
                  std::string_view how = "common",
                  std::string right_suffix = "_right",
                  std::string_view validate = "many_to_many") const;
  DataFrame unify(std::initializer_list<std::string_view> left_on,
                  const DataFrame& right,
                  std::initializer_list<std::string_view> right_on = {},
                  std::string_view how = "common",
                  std::string right_suffix = "_right",
                  std::string_view validate = "many_to_many") const;

  DataFramePlotAccessor plot() const;
  DataFramePlotAccessor plot(const DataFramePlotOptions& options) const;
  DataFramePlot3DAccessor plot3d() const;
  DataFramePlot3DAccessor plot3d(const DataFramePlotOptions& options) const;
  DataFrameSqlAccessor sql(SqlOptions options) const;
  DataFrameSqlAccessor sql() const;
  DataFrameSqlPrepared prepare_sql(std::string_view statement) const;
  DataFrameSqlPrepared prepare_sql(std::string_view statement, SqlOptions options) const;
  DataFrameTsAccessor ts(std::string_view feature) const;
  DataFrameTsAccessor ts(std::initializer_list<std::string_view> features) const;
  DataFrameTsAccessor ts(pluss::msharp::RangeView::Of<std::string_view> features) const;
  LetSlot set(std::string_view name);
  LetSlots set(pluss::msharp::RangeView::Of<std::string_view> names);
  LetSlots set(std::initializer_list<std::string_view> names) {
    return set(pluss::msharp::RangeView::Of<std::string_view>(names));
  }
  LetSlots set(std::initializer_list<const char*> names);
  DataFrame set_features(std::string_view name, const Thread& value) const;
  DataFrame& set_features_(std::string_view name, const Thread& value);
  DataFrame set_features(pluss::msharp::RangeView::Of<std::string_view> names,
                         pluss::msharp::RangeView::Of<Thread> values) const;
  DataFrame& set_features_(pluss::msharp::RangeView::Of<std::string_view> names,
                           pluss::msharp::RangeView::Of<Thread> values);

  DataFrame rename(std::string_view old_name, std::string new_name) const;
  DataFrame& rename_(std::string_view old_name, std::string new_name);
  DataFrame rename_features(
      pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> renames) const;
  DataFrame relabel_features(pluss::msharp::RangeView::Of<std::string_view> feature_names) const;
  DataFrame set_axis(pluss::msharp::RangeView::Of<std::string_view> feature_names) const;
  DataFrame set_feature_axis(pluss::msharp::RangeView::Of<std::string_view> feature_names) const;
  DataFrame conform_to(const DataFrame& other) const;
  DataFrame reindex_like(const DataFrame& other) const;
  DataFrame reindex_cases(const Thread& new_cases) const;
  DataFrame stitch_on(std::string_view left_feature,
                      const DataFrame& right,
                      std::string_view right_feature = "",
                      std::string right_suffix = "_right") const;
  DataFrame stitch_on(pluss::msharp::RangeView::Of<std::string_view> left_features,
                      const DataFrame& right,
                      pluss::msharp::RangeView::Of<std::string_view> right_features = {},
                      std::string right_suffix = "_right") const;
  DataFrame stitch_on(std::initializer_list<std::string_view> left_features,
                      const DataFrame& right,
                      std::initializer_list<std::string_view> right_features = {},
                      std::string right_suffix = "_right") const;

  DataFrameView head(std::int64_t n = 5, bool render = true) const;
  DataFrameView tail(std::int64_t n = 5, bool render = true) const;
  DataFrame sample(std::int64_t n) const;
  DataFrame sample(std::int64_t n, bool replace, int axis) const;
  DataFrame sample(std::int64_t n, std::uint64_t random_state, bool replace = false, int axis = 0) const;
  template <class Float>
    requires std::floating_point<Float>
  DataFrame sample(Float frac) const {
    return sample_fraction_(static_cast<double>(frac), std::nullopt, false, 0);
  }
  template <class Float>
    requires std::floating_point<Float>
  DataFrame sample(Float frac, bool replace, int axis) const {
    return sample_fraction_(static_cast<double>(frac), std::nullopt, replace, axis);
  }
  template <class Float>
    requires std::floating_point<Float>
  DataFrame sample(Float frac, std::uint64_t random_state, bool replace = false, int axis = 0) const {
    return sample_fraction_(static_cast<double>(frac), random_state, replace, axis);
  }
  DataFrame shuffle(std::optional<std::uint64_t> random_state = std::nullopt) const;
  DataFrame reshuffle(int axis = 0, std::optional<std::uint64_t> random_state = std::nullopt) const;
  DataFrame shift(std::int64_t periods = 1) const;
  DataFrame shift(pluss::msharp::RangeView::Of<std::string_view> features, std::int64_t periods = 1) const;
  DataFrame slice(std::int64_t start, std::int64_t stop) const;
  DataFrame take(pluss::msharp::RangeView::Int indices) const;
  DataFrame where(const pluss::Series& bool_mask) const;
  DataFrame where(const Thread& bool_mask) const;
  DataFrame filter(const pluss::Series& bool_mask) const;
  DataFrame filter(const Thread& bool_mask) const;
  DataFrame query(std::string_view expression) const;
  DataFrame assign(pluss::msharp::RangeView::Of<std::pair<std::string, Thread>> assignments) const;
  DataFrame assign(pluss::msharp::RangeView::Of<DeriveSpec> specs) const;
  Thread map_cases(
      const std::function<ThreadValue(const DataFrame&)>& fn) const;
  DataFrame apply_cases(
      const std::function<DataFrame(const DataFrame&)>& fn,
      ResultShape shape = ResultShape::Infer) const;
  DataFrame map_elements(
      const std::function<ThreadValue(const ThreadValue&)>& fn,
      pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  Thread aggregate_with(
      const std::function<Thread(const DataFrame&)>& fn,
      ResultShape shape = ResultShape::Reduce) const;
  DataFrame flow(
      const std::function<DataFrame(const DataFrame&)>& fn) const;
  DataFrame text_upper(
      pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame text_casefold(
      pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame text_title(
      pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame text_lengths(
      pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame text_starts_with(
      std::string_view prefix,
      pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame text_ends_with(
      std::string_view suffix,
      pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame text_matches(
      std::string_view pattern,
      bool full = false,
      pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame text_pad(
      std::int64_t width,
      TextAlign align = TextAlign::Left,
      char fill = ' ',
      pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame text_slice(
      std::int64_t start,
      std::optional<std::int64_t> stop = std::nullopt,
      std::int64_t step = 1,
      pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame text_normalize(
      UnicodeForm form = UnicodeForm::NFC,
      pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame text_encode(
      std::string_view encoding = "base64",
      pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame regex_groups(
      std::string_view feature,
      std::string_view pattern,
      pluss::msharp::RangeView::Of<std::string_view> names = {}) const;
  DataFrame split_feature(std::string_view feature,
                          std::string_view delimiter,
                          std::int64_t limit = -1,
                          std::string prefix = "part") const;
  DataFrame partition_feature(std::string_view feature,
                              std::string_view separator,
                              std::string prefix = "part") const;
  DataFrame text_indicators(std::string_view feature,
                            std::string_view delimiter = "|",
                            std::string prefix = "") const;
  DataFrame transform(const std::function<Thread(const Thread&)>& fn) const;
  DataFrame transform(std::string_view feature, const std::function<Thread(const DataFrame&)>& fn) const;
  DataFrame transform(std::string_view feature, const std::function<Thread(const Thread&)>& fn) const;
  DataFrame transform(pluss::msharp::RangeView::Of<TransformSpec> specs) const;
  DataFrame put(std::string_view feature, const Thread& bool_mask, const Thread& value) const;
  DataFrame put(std::string_view feature, const Thread& bool_mask, double value) const;
  DataFrame put(std::string_view feature, const Thread& bool_mask, std::string_view value) const;
  DataFrame put(std::string_view feature, const Thread& bool_mask, const char* value) const {
    return put(feature, bool_mask, std::string_view{value != nullptr ? value : ""});
  }
  DataFrame put(std::string_view feature, const Thread& bool_mask, pluss::null_t value) const;
  DataFrame put(std::string_view feature, const pluss::Series& bool_mask, const Thread& value) const {
    return put(feature, Thread(bool_mask), value);
  }
  DataFrame put(std::string_view feature, const pluss::Series& bool_mask, double value) const {
    return put(feature, Thread(bool_mask), value);
  }
  DataFrame put(std::string_view feature, const pluss::Series& bool_mask, std::string_view value) const {
    return put(feature, Thread(bool_mask), value);
  }
  DataFrame put(std::string_view feature, const pluss::Series& bool_mask, const char* value) const {
    return put(feature, Thread(bool_mask), value);
  }
  DataFrame put(std::string_view feature, const pluss::Series& bool_mask, pluss::null_t value) const {
    return put(feature, Thread(bool_mask), value);
  }
  DataFrame tz_convert(pluss::msharp::RangeView::Of<std::string_view> features, std::string_view tz_name) const;
  DataFrame tz_localize(pluss::msharp::RangeView::Of<std::string_view> features,
                        std::string_view tz_name,
                        int fold = 0,
                        std::string_view nonexistent = "raise") const;
  DataFrame to_utc(pluss::msharp::RangeView::Of<std::string_view> features) const;
  DataFrame shift_time(pluss::msharp::RangeView::Of<std::string_view> features,
                       int periods,
                       std::string_view freq) const;
  DataFrame floor_time(pluss::msharp::RangeView::Of<std::string_view> features,
                       std::string_view unit) const;
  DataFrame ceil_time(pluss::msharp::RangeView::Of<std::string_view> features,
                      std::string_view unit) const;
  DataFrame round_time(pluss::msharp::RangeView::Of<std::string_view> features,
                       std::string_view unit) const;
  DataFrameView resample(std::string_view time_feature,
                         pluss::msharp::RangeView::Of<std::string_view> value_features,
                         std::string_view freq,
                         std::string_view op = "mean",
                         std::int64_t step = 1,
                         bool render = true) const;
  DataFrameView aggregate(pluss::msharp::RangeView::Of<SummarySpec> specs, bool render = true) const;
  DataFrameView group_by(pluss::msharp::RangeView::Of<std::string_view> keys,
                         pluss::msharp::RangeView::Of<GroupSpec> specs,
                         bool dropna_keys = true,
                         bool render = true) const;
  DataFrameView pivot(std::string_view case_feature,
                      std::string_view feature_feature,
                      std::string_view value_feature,
                      bool dropna_keys = true,
                      bool render = true) const;
  DataFrameView pivot_table(std::string_view case_feature,
                            std::string_view feature_feature,
                            std::string_view value_feature,
                            GroupOp op = GroupOp::Mean,
                            bool dropna_keys = true,
                            bool render = true) const;
  DataFrame combine(const DataFrame& other) const;
  DataFrame combine(const DataFrame& other, const CombineSpec& spec) const;
  DataFrame extend_cases(const DataFrame& other,
                         FrameMatchRule features = FrameMatchRule::Unite,
                         bool fresh_cases = false,
                         bool reject_repeated_cases = false) const;
  Atlas unsqueeze() const;
  Atlas unsqueeze(const Thread& depth_labels) const;
  DataFrameView count_by(std::string_view feature,
                         std::int64_t top_n = 0,
                         bool dropna = true,
                         bool render = true) const;
  DataFrameView mean_by(std::string_view group_feature,
                        std::string_view value_feature,
                        std::int64_t top_n = 0,
                        std::int64_t min_count = 1,
                        bool dropna_group = true,
                        bool render = true) const;
  DataFrame add(double other) const;
  DataFrame add(const DataFrame& other) const;
  DataFrame sub(double other) const;
  DataFrame sub(const DataFrame& other) const;
  DataFrame subtract(double other) const;
  DataFrame subtract(const DataFrame& other) const;
  DataFrame mul(double other) const;
  DataFrame mul(const DataFrame& other) const;
  DataFrame multiply(double other) const;
  DataFrame multiply(const DataFrame& other) const;
  DataFrame div(double other) const;
  DataFrame div(const DataFrame& other) const;
  DataFrame divide(double other) const;
  DataFrame divide(const DataFrame& other) const;
  DataFrame truediv(double other) const;
  DataFrame truediv(const DataFrame& other) const;
  DataFrame floordiv(double other) const;
  DataFrame floordiv(const DataFrame& other) const;
  DataFrame mod(double other) const;
  DataFrame mod(const DataFrame& other) const;
  DataFrame pow(double other) const;
  DataFrame pow(const DataFrame& other) const;
  DataFrame radd(double other) const;
  DataFrame rsub(double other) const;
  DataFrame rmul(double other) const;
  DataFrame rdiv(double other) const;
  DataFrame rtruediv(double other) const;
  DataFrame rfloordiv(double other) const;
  DataFrame rmod(double other) const;
  DataFrame rpow(double other) const;
  DataFrame eq(double other) const;
  DataFrame eq(const DataFrame& other) const;
  DataFrame ne(double other) const;
  DataFrame ne(const DataFrame& other) const;
  DataFrame lt(double other) const;
  DataFrame lt(const DataFrame& other) const;
  DataFrame le(double other) const;
  DataFrame le(const DataFrame& other) const;
  DataFrame gt(double other) const;
  DataFrame gt(const DataFrame& other) const;
  DataFrame ge(double other) const;
  DataFrame ge(const DataFrame& other) const;
  DataFrame compare(double other) const;
  DataFrame compare(const DataFrame& other) const;
  DataFrameView sum(bool render = true) const;
  Thread sum(std::int64_t axis, bool skipna = true) const;
  Thread sum(int axis, bool skipna = true) const {
    return sum(static_cast<std::int64_t>(axis), skipna);
  }
  DataFrameView mean(bool render = true) const;
  Thread mean(std::int64_t axis, bool skipna = true) const;
  Thread mean(int axis, bool skipna = true) const {
    return mean(static_cast<std::int64_t>(axis), skipna);
  }
  DataFrameView median(bool render = true) const;
  Thread median(std::int64_t axis, bool skipna = true) const;
  Thread median(int axis, bool skipna = true) const {
    return median(static_cast<std::int64_t>(axis), skipna);
  }
  DataFrameView min(bool render = true) const;
  Thread min(std::int64_t axis, bool skipna = true) const;
  Thread min(int axis, bool skipna = true) const {
    return min(static_cast<std::int64_t>(axis), skipna);
  }
  DataFrameView max(bool render = true) const;
  Thread max(std::int64_t axis, bool skipna = true) const;
  Thread max(int axis, bool skipna = true) const {
    return max(static_cast<std::int64_t>(axis), skipna);
  }
  DataFrameView std(bool render = true) const;
  Thread std(std::int64_t axis, bool skipna = true) const;
  Thread std(int axis, bool skipna = true) const {
    return std(static_cast<std::int64_t>(axis), skipna);
  }
  DataFrameView var(bool render = true) const;
  Thread var(std::int64_t axis, bool skipna = true) const;
  Thread var(int axis, bool skipna = true) const {
    return var(static_cast<std::int64_t>(axis), skipna);
  }
  DataFrameView count(bool render = true) const;
  Thread count(std::int64_t axis) const;
  Thread count(int axis) const {
    return count(static_cast<std::int64_t>(axis));
  }
  DataFrameView prod(bool render = true) const;
  Thread prod(std::int64_t axis, bool skipna = true) const;
  Thread prod(int axis, bool skipna = true) const {
    return prod(static_cast<std::int64_t>(axis), skipna);
  }
  Thread any(pluss::msharp::RangeView::Of<std::string_view> features = {}, bool skipna = true) const;
  Thread any(std::int64_t axis,
             pluss::msharp::RangeView::Of<std::string_view> features = {},
             bool skipna = true) const;
  Thread all(pluss::msharp::RangeView::Of<std::string_view> features = {}, bool skipna = true) const;
  Thread all(std::int64_t axis,
             pluss::msharp::RangeView::Of<std::string_view> features = {},
             bool skipna = true) const;
  Thread idxmin(std::int64_t axis = 0, bool skipna = true) const;
  Thread idxmax(std::int64_t axis = 0, bool skipna = true) const;
  DataFrameView count_distinct(pluss::msharp::RangeView::Of<std::string_view> features = {},
                               bool dropna = true,
                               bool render = true) const;
  DataFrameView corr(CorrMethod method, bool render = false) const;
  DataFrameView corr(bool render = false) const;
  DataFrameView corr(pluss::msharp::RangeView::Of<std::string_view> features, CorrMethod method, bool render = false) const;
  DataFrameView corr(pluss::msharp::RangeView::Of<std::string_view> features, bool render = false) const;
  DataFrameView covariance(const StatisticOptions& options = {}, bool render = false) const;
  DataFrame percentiles(pluss::msharp::RangeView::Double points = {0.25, 0.5, 0.75},
                        std::int64_t min_count = 1) const;
  DataFrame most_frequent(bool drop_nulls = true) const;
  DataFrame ranks(RankTieRule ties = RankTieRule::Average,
                  RankNullRule nulls = RankNullRule::Keep,
                  bool ascending = true,
                  bool percentage = false) const;
  Thread asymmetry(std::int64_t axis = 0, std::int64_t min_count = 3) const;
  Thread tail_weight(std::int64_t axis = 0, std::int64_t min_count = 4) const;
  Thread variance(std::int64_t axis = 0,
                  const StatisticOptions& options = {}) const;
  Thread deviation(std::int64_t axis = 0,
                   const StatisticOptions& options = {}) const;
  Thread mean_error(std::int64_t axis = 0,
                    const StatisticOptions& options = {}) const;
  DataFrame running_sum(bool skip_nulls = true) const;
  DataFrame running_product(bool skip_nulls = true) const;
  DataFrame running_minimum(bool skip_nulls = true) const;
  DataFrame running_maximum(bool skip_nulls = true) const;
  Thread correlate_with(const DataFrame& other,
                        CorrMethod method = CorrMethod::Pearson,
                        std::int64_t min_count = 1) const;
  DataFrame matrix_product(const DataFrame& other) const;
  DataFrame moving_sum(std::int64_t window, const WindowOptions& options = {}) const;
  DataFrame moving_mean(std::int64_t window, const WindowOptions& options = {}) const;
  DataFrame moving_minimum(std::int64_t window, const WindowOptions& options = {}) const;
  DataFrame moving_maximum(std::int64_t window, const WindowOptions& options = {}) const;
  DataFrame moving_variance(std::int64_t window, const WindowOptions& options = {}) const;
  DataFrame moving_deviation(std::int64_t window, const WindowOptions& options = {}) const;
  DataFrame moving_mean_error(std::int64_t window, const WindowOptions& options = {}) const;
  DataFrame moving_asymmetry(std::int64_t window, const WindowOptions& options = {}) const;
  DataFrame moving_tail_weight(std::int64_t window, const WindowOptions& options = {}) const;
  DataFrame weighted_moving(
      pluss::msharp::RangeView::Double weights,
      WindowStatistic statistic = WindowStatistic::Mean,
      std::int64_t min_count = 1) const;
  DataFrame indexed_moving(
      const WindowBoundsFn& bounds,
      WindowStatistic statistic = WindowStatistic::Mean,
      const WindowOptions& options = {}) const;
  DataFrame growing_sum(std::int64_t min_count = 1) const;
  DataFrame growing_mean(std::int64_t min_count = 1) const;
  DataFrame growing_minimum(std::int64_t min_count = 1) const;
  DataFrame growing_maximum(std::int64_t min_count = 1) const;
  DataFrame growing_variance(const StatisticOptions& options = {}) const;
  DataFrame growing_deviation(const StatisticOptions& options = {}) const;
  DataFrame growing_mean_error(const StatisticOptions& options = {}) const;
  DataFrame growing_asymmetry(std::int64_t min_count = 3) const;
  DataFrame growing_tail_weight(std::int64_t min_count = 4) const;
  DataFrame exponential_mean(double alpha,
                             bool adjusted = true,
                             std::int64_t min_count = 1,
                             bool ignore_nulls = false) const;
  DataFrame exponential_variance(double alpha,
                                 bool adjusted = true,
                                 bool bias_corrected = true,
                                 std::int64_t min_count = 1,
                                 bool ignore_nulls = false) const;
  DataFrame exponential_deviation(double alpha,
                                  bool adjusted = true,
                                  bool bias_corrected = true,
                                  std::int64_t min_count = 1,
                                  bool ignore_nulls = false) const;
  DataFrame grouped_moving_mean(std::string_view group_feature,
                                std::int64_t window,
                                const WindowOptions& options = {}) const;
  DataFrame grouped_moving(std::string_view group_feature,
                           std::int64_t window,
                           WindowStatistic statistic = WindowStatistic::Mean,
                           const WindowOptions& options = {}) const;
  DataFrame time_moving_mean(std::string_view time_feature,
                             std::int64_t duration_nanos,
                             std::int64_t min_count = 1) const;
  DataFrame time_moving(std::string_view time_feature,
                        std::int64_t duration_nanos,
                        TimeWindowStatistic statistic = TimeWindowStatistic::Mean,
                        const StatisticOptions& options = {},
                        WindowClosure closure = WindowClosure::Right) const;
  DataFrame isna(pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame isnull(pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame notna(pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame notnull(pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame cast(pluss::msharp::RangeView::Of<CastSpec> specs) const;
  DataFrame cast(
      pluss::msharp::RangeView::Of<std::pair<std::string_view, pluss::codem::ScalarType>> feature_dtypes) const;
  DataFrame astype(
      pluss::msharp::RangeView::Of<std::pair<std::string_view, pluss::codem::ScalarType>> feature_dtypes) const;
  DataFrame window(pluss::msharp::RangeView::Of<WindowSpec> specs, bool keep_input = true) const;
  DataFrame rolling(pluss::msharp::RangeView::Of<WindowSpec> specs, bool keep_input = true) const;

  DataFrame fillna(double value) const;
  DataFrame fillna(double value, pluss::msharp::RangeView::Of<std::string_view> features) const;
  DataFrame fillna(std::string_view value) const;
  DataFrame fillna(std::string_view value, pluss::msharp::RangeView::Of<std::string_view> features) const;
  DataFrame fillna(const char* value) const {
    return fillna(std::string_view{value != nullptr ? value : ""});
  }
  DataFrame fillna(const char* value, pluss::msharp::RangeView::Of<std::string_view> features) const {
    return fillna(std::string_view{value != nullptr ? value : ""}, features);
  }
  DataFrame fillna(const pluss::Timeline& value) const;
  DataFrame fillna(const pluss::Timeline& value, pluss::msharp::RangeView::Of<std::string_view> features) const;
  DataFrame fillna(const pluss::TimeIndex& value) const;
  DataFrame fillna(const pluss::TimeIndex& value, pluss::msharp::RangeView::Of<std::string_view> features) const;
  DataFrame replace(double from, double to, pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame replace(double from, pluss::null_t to, pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame replace(pluss::null_t from, double to, pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame replace(std::string_view from,
                    std::string_view to,
                    pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame replace(const char* from,
                    const char* to,
                    pluss::msharp::RangeView::Of<std::string_view> features = {}) const {
    return replace(std::string_view{from != nullptr ? from : ""},
                   std::string_view{to != nullptr ? to : ""},
                   features);
  }
  DataFrame replace(std::string_view from,
                    pluss::null_t to,
                    pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame replace(pluss::null_t from,
                    std::string_view to,
                    pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame replace(const char* from,
                    pluss::null_t to,
                    pluss::msharp::RangeView::Of<std::string_view> features = {}) const {
    return replace(std::string_view{from != nullptr ? from : ""}, to, features);
  }
  DataFrame replace(pluss::null_t from,
                    const char* to,
                    pluss::msharp::RangeView::Of<std::string_view> features = {}) const {
    return replace(from, std::string_view{to != nullptr ? to : ""}, features);
  }
  DataFrame replace(const pluss::Series& from,
                    const pluss::Series& to,
                    pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame replace(const pluss::Series& from,
                    pluss::null_t to,
                    pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame replace(pluss::null_t from,
                    const pluss::Series& to,
                    pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame replace(const pluss::Note& from,
                    const pluss::Note& to,
                    pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame replace(const pluss::Note& from,
                    pluss::null_t to,
                    pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame replace(pluss::null_t from,
                    const pluss::Note& to,
                    pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame replace(const pluss::Timeline& from,
                    const pluss::Timeline& to,
                    pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame replace(const pluss::Timeline& from,
                    pluss::null_t to,
                    pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame replace(pluss::null_t from,
                    const pluss::Timeline& to,
                    pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame replace(const pluss::TimeIndex& from,
                    const pluss::TimeIndex& to,
                    pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame replace(const pluss::TimeIndex& from,
                    pluss::null_t to,
                    pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame replace(pluss::null_t from,
                    const pluss::TimeIndex& to,
                    pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame& fillna_(double value);
  DataFrame& fillna_(std::string_view value);
  DataFrame& fillna_(const char* value) {
    return fillna_(std::string_view{value != nullptr ? value : ""});
  }
  DataFrame& fillna_(const pluss::Timeline& value);
  DataFrame& fillna_(const pluss::TimeIndex& value);
  DataFrame& put_(std::string_view feature, const Thread& bool_mask, const Thread& value);
  DataFrame& put_(std::string_view feature, const Thread& bool_mask, double value);
  DataFrame& put_(std::string_view feature, const Thread& bool_mask, std::string_view value);
  DataFrame& put_(std::string_view feature, const Thread& bool_mask, const char* value) {
    return put_(feature, bool_mask, std::string_view{value != nullptr ? value : ""});
  }
  DataFrame& put_(std::string_view feature, const Thread& bool_mask, pluss::null_t value);
  DataFrame& put_(std::string_view feature, const pluss::Series& bool_mask, const Thread& value) {
    return put_(feature, Thread(bool_mask), value);
  }
  DataFrame& put_(std::string_view feature, const pluss::Series& bool_mask, double value) {
    return put_(feature, Thread(bool_mask), value);
  }
  DataFrame& put_(std::string_view feature, const pluss::Series& bool_mask, std::string_view value) {
    return put_(feature, Thread(bool_mask), value);
  }
  DataFrame& put_(std::string_view feature, const pluss::Series& bool_mask, const char* value) {
    return put_(feature, Thread(bool_mask), value);
  }
  DataFrame& put_(std::string_view feature, const pluss::Series& bool_mask, pluss::null_t value) {
    return put_(feature, Thread(bool_mask), value);
  }
  DataFrame dropna() const;
  DataFrame dropna_all(pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame ffill() const;
  DataFrame bfill() const;
  std::int64_t count_null_rows() const;
  DataFrameView describe(bool render = true) const;
  DataFrameView transpose(bool render = false) const;
  DataFrame lengthen(
      pluss::msharp::RangeView::Of<std::string_view> id_features = {},
      pluss::msharp::RangeView::Of<std::string_view> value_features = {},
      std::string feature_name = "feature",
      std::string value_name = "value",
      bool fresh_cases = true) const;
  CategoryEncoding catalogue(std::string_view feature,
                             bool ordered = false,
                             bool sort_levels = false) const;
  DataFrameView tally(std::int64_t top_n = 0, bool dropna = true, bool render = true) const;
  DataFrameView tally(pluss::msharp::RangeView::Of<std::string_view> features,
                      std::int64_t top_n = 0,
                      bool dropna = true,
                      bool render = true) const;
  DataFrameView tally(std::string_view feature,
                      std::int64_t top_n = 0,
                      bool dropna = true,
                      bool render = true) const;
  DataFrameView view(std::int64_t start, std::int64_t stop, bool render = true) const;
  DataFrame copy() const;
  bool equals(const DataFrame& other) const;

  std::string to_string(const display::DisplayOptions& options = {}) const;
  std::string info_string(const display::DisplayOptions& options = {}) const;
  void show(const display::DisplayOptions& options = {}) const;
  void show(std::ostream& os, const display::DisplayOptions& options = {}) const;
  void info(const display::DisplayOptions& options = {}) const;
  void info(std::ostream& os, const display::DisplayOptions& options = {}) const;
  void save_csv(std::string_view filepath) const;
  void save_csv(std::string_view filepath,
                bool add_cases,
                bool header = true,
                char sep = ',',
                std::string na_rep = "",
                std::optional<std::string> case_label = std::nullopt,
                std::optional<std::string> float_format = std::nullopt,
                char quotechar = '"',
                bool doublequote = true,
                std::string line_terminator = "\n",
                char decimal = '.',
                std::string encoding = "utf-8") const;
  void save_csv(std::string_view filepath, const pluss::loader::WriteCsvOptions& options) const;
  void save_csv(std::string_view filepath, const pluss::loader::Config& config) const;
  void save_csv(std::string_view filepath,
                std::initializer_list<pluss::loader::ConfigEntry> config_entries) const;
  void save_excel(std::string_view filepath) const;
  void save_excel(std::string_view filepath,
                  std::string sheet_name,
                  bool add_cases = false,
                  bool header = true,
                  std::string na_rep = "",
                  std::optional<std::string> case_label = std::nullopt) const;
  void save_excel(std::string_view filepath, const pluss::loader::WriteExcelOptions& options) const;
  void save_excel(std::string_view filepath, const pluss::loader::Config& config) const;
  void save_excel(std::string_view filepath,
                  std::initializer_list<pluss::loader::ConfigEntry> config_entries) const;
  void save_txt(std::string_view filepath) const;
  void save_txt(std::string_view filepath, const pluss::loader::Config& config) const;
  void save_txt(std::string_view filepath,
                std::initializer_list<pluss::loader::ConfigEntry> config_entries) const;
  void save_json(std::string_view filepath) const;
  void save_json(std::string_view filepath, const pluss::loader::Config& config) const;
  void save_json(std::string_view filepath,
                 std::initializer_list<pluss::loader::ConfigEntry> config_entries) const;
  void save_parquet(std::string_view filepath) const;
  void save_parquet(std::string_view filepath, const pluss::loader::Config& config) const;
  void save_parquet(std::string_view filepath,
                    std::initializer_list<pluss::loader::ConfigEntry> config_entries) const;
  void save_as(std::string_view target_type,
               std::string_view filepath,
               const pluss::loader::Config& config = {}) const;
  void save_as(std::string_view target_type,
               std::string_view filepath,
               std::initializer_list<pluss::loader::ConfigEntry> config_entries) const;

private:
  explicit DataFrame(pluss::msharp::intrusive_ptr<detail::DataFrameImpl> impl);
  detail::DataFrameImpl& mut_impl_();
  const detail::DataFrameImpl& impl_ref_() const;
  void suppress_next_stream_() const;
  bool consume_stream_suppression_() const;
  DataFrame arrange(pluss::msharp::RangeView::Of<std::string_view> features,
                    bool ascending = true,
                    bool nulls_last = true) const;
  DataFrame arrange(pluss::msharp::RangeView::Of<ArrangeSpec> specs) const;
  DataFrame query(const pluss::Series& bool_mask) const;
  DataFrame query(const QueryFn& predicate) const;
  DataFrame sample_impl_(std::int64_t n,
                         std::optional<std::uint64_t> random_state,
                         bool replace,
                         int axis) const;
  DataFrame sample_fraction_(double frac,
                             std::optional<std::uint64_t> random_state,
                             bool replace,
                             int axis) const;
  DataFrame summarize_specs(const std::vector<SummarySpec>& specs, bool render = true) const;
  DataFrameView tally_impl(const std::vector<std::string>& feature_names,
                           std::int64_t top_n,
                           bool dropna,
                           bool render) const;
  DataFrame summarize(pluss::msharp::RangeView::Of<SummarySpec> specs, bool render = true) const;
  DataFrame group(pluss::msharp::RangeView::Of<std::string_view> keys,
                  pluss::msharp::RangeView::Of<GroupSpec> specs,
                  bool dropna_keys = true,
                  bool render = true) const;
  DataFrame missing(const MissingSpec& spec) const;
  DataFrame missing(MissingOp op,
                    pluss::msharp::RangeView::Of<std::string_view> features = {},
                    std::optional<double> value = std::nullopt) const;

  pluss::msharp::intrusive_ptr<detail::DataFrameImpl> impl_{nullptr};
  friend void output_render(std::ostream& os, const DataFrame& frame);
  friend std::ostream& operator<<(std::ostream& os, const DataFrame& frame);
};

struct FrameMatch {
  DataFrame left{};
  DataFrame right{};
};

std::ostream& operator<<(std::ostream& os, const DataFrame& frame);
void output_render(std::ostream& os, const DataFrame& frame);

class DataFrameView final : public DataFrame {
public:
  DataFrameView();
  explicit DataFrameView(DataFrame frame,
                         bool auto_render_terminal = false,
                         std::string operation = {},
                         std::string source_title = {});
  DataFrameView(const DataFrameView& other);
  DataFrameView(DataFrameView&& other) noexcept;
  DataFrameView& operator=(const DataFrameView& other);
  DataFrameView& operator=(DataFrameView&& other) noexcept;
  ~DataFrameView();

  void show(const display::DisplayOptions& options = {}) const;
  void show(std::ostream& os, const display::DisplayOptions& options = {}) const;

private:
  struct State;
  std::shared_ptr<State> state_{};

  void consume_pending_display_() const;
  void maybe_autorender_() const;
  std::string resolved_window_title_() const;

  friend void output_render(std::ostream& os, const DataFrameView& frame);
  friend std::ostream& operator<<(std::ostream& os, const DataFrameView& frame);
};

std::ostream& operator<<(std::ostream& os, const DataFrameView& frame);
void output_render(std::ostream& os, const DataFrameView& frame);

enum class TsAlignMode : std::uint8_t {
  Exact = 0,
  Backward = 1,
  Forward = 2,
  Nearest = 3,
};

enum class TsReindexFill : std::uint8_t {
  None = 0,
  Forward = 1,
  Backward = 2,
  Zero = 3,
  Constant = 4,
};

struct TsRollingBlockOptions {
  bool include_mean{true};
  bool include_std{true};
  bool include_min{false};
  bool include_max{false};
  bool include_sum{false};
  bool include_var{false};
  std::int64_t min_periods{1};
};

class DataFrameTsAccessor {
 public:
  DataFrameTsAccessor(DataFrame frame, std::vector<std::string> features);

  DataFrame returns(std::int64_t periods = 1) const;
  DataFrame log_returns(std::int64_t periods = 1) const;
  DataFrame diff(std::int64_t periods = 1) const;
  DataFrame lag(std::int64_t periods = 1) const;
  DataFrame lead(std::int64_t periods = 1) const;
  DataFrame rolling_mean(std::int64_t window, std::int64_t min_periods = 1) const;
  DataFrame rolling_std(std::int64_t window, std::int64_t min_periods = 1) const;
  DataFrame rolling_sum(std::int64_t window, std::int64_t min_periods = 1) const;
  DataFrame rolling_var(std::int64_t window, std::int64_t min_periods = 1) const;
  DataFrame rolling_min(std::int64_t window, std::int64_t min_periods = 1) const;
  DataFrame rolling_max(std::int64_t window, std::int64_t min_periods = 1) const;
  DataFrame ema(double alpha) const;
  DataFrame ewm_mean(double alpha) const;
  DataFrame ewm_std(double alpha, double epsilon = 1e-12) const;
  DataFrame expanding_mean(std::int64_t min_periods = 1) const;
  DataFrame expanding_std(std::int64_t min_periods = 1, double epsilon = 1e-12) const;
  DataFrame zscore_rolling(std::int64_t window, std::int64_t min_periods = 1, double epsilon = 1e-12) const;
  DataFrame fill_gaps(pluss::subsp::stats::ts::GapFillMode mode = pluss::subsp::stats::ts::GapFillMode::Forward,
                      std::optional<double> constant = std::nullopt) const;

  DataFrameTsResampler resample(std::string_view time_feature,
                                std::string_view freq,
                                std::int64_t step = 1) const;
  DataFrame resample(std::string_view time_feature,
                     std::string_view freq,
                     pluss::subsp::stats::ts::ResampleOp op,
                     std::int64_t step = 1) const;
  DataFrame asfreq(std::string_view time_feature,
                   std::string_view freq,
                   std::int64_t step = 1,
                   pluss::subsp::stats::ts::GapFillMode fill_mode = pluss::subsp::stats::ts::GapFillMode::Forward,
                   std::optional<double> constant = std::nullopt,
                   pluss::subsp::stats::ts::ResampleOp op = pluss::subsp::stats::ts::ResampleOp::Last) const;
  DataFrame insert_gaps(std::string_view time_feature,
                        std::string_view freq,
                        std::int64_t step = 1) const;
  DataFrame align_to(std::string_view time_feature,
                     const Thread& target_axis,
                     TsAlignMode mode = TsAlignMode::Exact) const;
  DataFrame reindex_time(std::string_view time_feature,
                         const Thread& target_axis,
                         TsAlignMode mode = TsAlignMode::Exact,
                         TsReindexFill fill = TsReindexFill::None,
                         std::optional<double> constant = std::nullopt) const;

  DataFrame calendar(std::string_view time_feature) const;
  DataFrame lag_block(pluss::msharp::RangeView::Int lags) const;
  DataFrame rolling_block(pluss::msharp::RangeView::Int windows,
                          const TsRollingBlockOptions& options = {}) const;

 private:
  DataFrame frame_{};
  std::vector<std::string> features_{};
};

class DataFrameTsResampler {
 public:
  DataFrameTsResampler(DataFrame frame,
                       std::vector<std::string> features,
                       std::string time_feature,
                       std::string freq,
                       std::int64_t step);

  DataFrame mean() const;
  DataFrame sum() const;
  DataFrame min() const;
  DataFrame max() const;
  DataFrame first() const;
  DataFrame last() const;
  DataFrame count() const;

  DataFrame asfreq(std::optional<double> fill_value = std::nullopt) const;
  DataFrame ffill() const;
  DataFrame bfill() const;
  DataFrame nearest() const;

 private:
  DataFrame aggregate_(pluss::subsp::stats::ts::ResampleOp op) const;

  DataFrame frame_{};
  std::vector<std::string> features_{};
  std::string time_feature_{};
  std::string freq_{};
  std::int64_t step_{1};
};

DataFrame::CaseSelector pos(std::int64_t row);
DataFrame::CaseSelector pos(std::int64_t start, std::int64_t stop);
DataFrame::CaseSelector pos(std::initializer_list<std::int64_t> rows);
DataFrame::CaseSelector pos(pluss::RangeView<std::int64_t> rows);

DataFrame::CaseSelector label(std::string_view value);
DataFrame::CaseSelector label(const char* value);
DataFrame::CaseSelector label(std::int64_t value);
DataFrame::CaseSelector label(double value);
DataFrame::CaseSelector labels(std::initializer_list<std::string_view> values);
DataFrame::CaseSelector labels(pluss::RangeView<std::string_view> values);
DataFrame::CaseSelector labels(std::initializer_list<std::int64_t> values);
DataFrame::CaseSelector labels(pluss::RangeView<std::int64_t> values);
DataFrame::CaseSelector labels(const Thread& values);
DataFrame::CaseSelector label_range(std::string_view start, std::string_view stop);
DataFrame::CaseSelector label_range(std::int64_t start, std::int64_t stop);
DataFrame::CaseSelector label_range(double start, double stop);
DataFrame::CaseSelector label_range(const Thread& start, const Thread& stop);
DataFrame::CaseSelector mask(const Thread& values);
DataFrame::CaseSelector mask(const pluss::Series& values);

DataFrame::FeatureSelector all_features();
DataFrame::FeatureSelector col(std::string_view name);
DataFrame::FeatureSelector col(const char* name);
DataFrame::FeatureSelector col(std::int64_t position);
DataFrame::FeatureSelector cols(std::initializer_list<std::string_view> names);
DataFrame::FeatureSelector cols(pluss::RangeView<std::string_view> names);
DataFrame::FeatureSelector cols(std::initializer_list<std::int64_t> positions);
DataFrame::FeatureSelector cols(pluss::RangeView<std::int64_t> positions);
DataFrame::FeatureSelector cols(std::int64_t start, std::int64_t stop);
DataFrame::FeatureSelector col_range(std::string_view start, std::string_view stop);

} // namespace pluss::table

#include "table/plot/DataFramePlot.h"
#include "table/sql/DataFrameSql.h"
