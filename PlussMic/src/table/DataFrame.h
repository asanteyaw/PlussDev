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
namespace pluss::scribble {
class Registry;
}

namespace pluss::table {

namespace detail {
template <class T>
struct IsInitializerList : std::false_type {};
template <class T>
struct IsInitializerList<std::initializer_list<T>> : std::true_type {};
template <class T>
inline constexpr bool IsInitializerListV = IsInitializerList<std::remove_cvref_t<T>>::value;

template <class NameRange>
concept StringViewNameRange =
    std::ranges::input_range<NameRange> &&
    !std::is_convertible_v<NameRange, std::string_view> &&
    !IsInitializerListV<NameRange> &&
    !std::same_as<std::remove_cvref_t<NameRange>, StackArray<std::string_view>> &&
    std::convertible_to<std::remove_cvref_t<std::ranges::range_reference_t<NameRange>>, std::string_view>;

inline auto stack_name_view(const StackArray<std::string_view>& names) {
  return pluss::msharp::RangeView::Of<std::string_view>(names.data().values);
}

template <StringViewNameRange NameRange, class Fn>
decltype(auto) with_stack_name_view(const NameRange& names, Fn&& fn) {
  auto owned = StackArray<std::string_view>(names);
  return std::forward<Fn>(fn)(stack_name_view(owned));
}

template <StringViewNameRange LeftRange, StringViewNameRange RightRange, class Fn>
decltype(auto) with_stack_name_views(const LeftRange& left, const RightRange& right, Fn&& fn) {
  auto owned_left = StackArray<std::string_view>(left);
  auto owned_right = StackArray<std::string_view>(right);
  return std::forward<Fn>(fn)(stack_name_view(owned_left), stack_name_view(owned_right));
}
}

namespace detail {
class DataFrameImpl;
struct GroupStateCache;
}
class DataFrame;
class DataFrameView;
class DataFramePlotAccessor;
class DataFramePlot3DAccessor;
class DataFrameNoteAccessor;
class DataFrameTimelineAccessor;
class DataFrameLabelAccessor;
class DataFrameFeatureAxisAccessor;
class DataFrameCastAccessor;
class DataFrameGroupBuilder;
class DataFrameGrouped;
class DataFrameSortBuilder;
class ContingencyTable;
class AtlasPlotAccessor;
struct DataFramePlotOptions;
class DataFrameSqlAccessor;
class DataFrameSqlPrepared;
class DataFrameTsAccessor;
class DataFrameTsWindowAccessor;
class DataFrameTsResampler;
class Atlas;
struct FrameMatch;

enum class TsReindexFill : std::uint8_t {
  None = 0,
  Forward = 1,
  Backward = 2,
  Zero = 3,
  Constant = 4,
};

struct TsGroup {
  std::vector<Thread> keys{};
  std::vector<std::string> names{};

  TsGroup() = default;
  TsGroup(Thread key);
  TsGroup(const DataFrame& frame);

  bool defined() const noexcept;
  std::int64_t size() const;
};

struct TsOptions {
  Thread axis{};
  TsGroup group{};
  Thread anchor{};
};

namespace detail {
enum class TsGroupedWindowAggOp : std::uint8_t {
  Sum = 0,
  Mean = 1,
  Min = 2,
  Max = 3,
  Count = 4,
};

DataFrame grouped_window_aggregate(const DataFrame& frame,
                                   const Thread& axis,
                                   const TsGroup& groups,
                                   const Thread* anchor,
                                   std::int64_t duration_nanos,
                                   WindowClosure closure,
                                   std::string_view value_feature,
                                   TsGroupedWindowAggOp op,
                                   std::string_view where);
}

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

enum class EncodeMode : std::uint8_t {
  OneHot = 0,
  Ordinal = 1,
};

struct EncodeOptions {
  std::string prefix{};
  bool drop_first{false};
  bool sort_levels{false};
  bool ordered{false};
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

  class DTypes {
  public:
    using Entry = std::pair<std::string, std::string>;
    using const_iterator = std::vector<Entry>::const_iterator;

    DTypes() = default;
    explicit DTypes(std::vector<Entry> entries);

    std::int64_t size() const noexcept;
    bool empty() const noexcept;
    bool contains(std::string_view feature) const noexcept;
    const std::string& at(std::string_view feature) const;
    const std::string& operator[](std::string_view feature) const;
    const std::vector<Entry>& entries() const noexcept;
    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;

  private:
    std::vector<Entry> entries_{};
  };

  class LetSlot {
  public:
    LetSlot(DataFrame& owner, std::string name, AlignOptions options = {});

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
    AlignOptions options_{};
  };

  class LetSlots {
  public:
    LetSlots(DataFrame& owner, std::vector<std::string> names, AlignOptions options = {});

    LetSlots& operator=(const DataFrame& frame);
    LetSlots& operator=(std::initializer_list<Thread> values);
    LetSlots& operator=(const std::vector<Thread>& values);

  private:
    void assign_from_threads(std::span<const Thread> values);

    DataFrame* owner_{nullptr};
    std::vector<std::string> names_{};
    AlignOptions options_{};
  };

  enum class SummaryOp : std::uint8_t {
    Count = 0,
    Ndistinct = 1,
    Sum = 2,
    Mean = 3,
    Median = 4,
    Min = 5,
    Max = 6,
    Std = 7,
    Var = 8,
    Prod = 9,
  };

  using GroupOp = ::pluss::table::GroupOp;

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
    Invalid = 7,
  };

  enum class CombineOp : std::uint8_t {
    Features = 0,
    Update = 1,
  };

  enum class FreqKind : std::uint8_t {
    Count = 0,
    Prop = 1,
  };

  struct SummarySpec {
    std::string output;
    std::string feature;
    SummaryOp op{SummaryOp::Mean};
  };

  struct FreqOptions {
    FreqKind kind{FreqKind::Count};
    bool dropnull{true};
    bool sort{true};
    bool ascending{false};
    std::int64_t top{0};
  };

  struct GroupSpec {
    std::string output;
    std::string feature;
    GroupOp op{GroupOp::Count};
  };

  enum class GroupLayout : std::uint8_t {
    Flat = 0,
  };

  struct GroupOptions {
    bool dropnull{true};
    bool sort{false};
    GroupLayout layout{GroupLayout::Flat};
  };

  struct SplitOptions {
    Axis axis{Axis::Case};
    bool shuffle{false};
    std::optional<std::uint64_t> random_state{std::nullopt};
  };

  struct PivotValueSpec {
    std::string output{};
    std::string feature{};
    GroupOp op{GroupOp::Mean};
  };

  struct PivotTableOptions {
    std::vector<std::string> index{};
    std::vector<std::string> columns{};
    std::vector<std::string> values{};
    std::vector<GroupOp> aggfuncs{GroupOp::Mean};
    std::vector<PivotValueSpec> specs{};
    std::optional<ThreadValue::HostAny> fill_value{std::nullopt};
    bool margins{false};
    std::string margins_name{"All"};
    bool dropna{true};
    bool sort{true};
    bool render{false};
    std::string index_separator{"\x1F"};
    std::string column_separator{" | "};
    std::string value_separator{" :: "};
  };

  struct ContingencyOptions {
    bool dropnull{true};
    bool sort{true};
    bool totals{false};
    std::string separator{" | "};
    std::string null_label{"NULL"};
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

  struct FillSpec {
    std::string feature{};
    Thread value{};
  };

  struct FillOptions {
    std::vector<std::string> features{};
    std::int64_t limit{-1};
  };

  struct DropOptions {
    std::vector<std::string> features{};
    Axis axis{Axis::Case};
    bool all{false};
    std::int64_t threshold{-1};
  };

  struct ArrangeSpec {
    std::string feature;
    bool ascending{true};
    bool nulls_last{true};
  };

  struct SetLabelOptions {
    std::vector<std::string> features{};
    bool drop{true};
    bool verify{false};
    bool sort{false};
    std::string separator{"\x1F"};
  };

  struct ResetLabelOptions {
    bool drop{false};
    std::vector<std::string> names{};
    std::string prefix{"case"};
  };

  struct LabelDropOptions {
    std::vector<std::string> names{};
    bool duplicated{false};
    bool repeated{false};
    bool keep_first{true};
    bool keep_last{false};
  };

  struct AlignSpec {
    std::optional<Thread> label{};
    std::vector<std::string> features{};
    std::optional<double> fill{std::nullopt};
  };

  struct CasePick {
    std::string label{};
    std::string value{};
    std::vector<std::string> names{};
    std::vector<std::string> values{};
    std::vector<std::string> key{};
    bool drop{false};
  };

  struct SortLabelOptions {
    std::vector<std::string> names{};
    bool ascending{true};
    bool nulls_last{true};
  };

  struct LabelRename {
    std::string from;
    std::string to;
  };

  struct LabelGroup {
    std::string label{};
    std::vector<std::string> names{};
  };

  struct UnstackOptions {
    std::string label{};
    bool drop{true};
    bool sort{true};
    std::string separator{"_"};
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

  struct Case {
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
    std::vector<std::string> label_values{};
    Thread mask{};

    static Case at(std::int64_t row);
    static Case slice(std::int64_t start, std::int64_t stop);
    static Case take(std::initializer_list<std::int64_t> rows);
    static Case take(pluss::RangeView<std::int64_t> rows);
    static Case label(std::string_view value);
    static Case label(const char* value);
    static Case label(std::int64_t value);
    static Case label(double value);
    static Case labels(std::initializer_list<std::string_view> values);
    static Case labels(pluss::RangeView<std::string_view> values);
    static Case labels(std::initializer_list<std::int64_t> values);
    static Case labels(pluss::RangeView<std::int64_t> values);
    static Case labels(const Thread& values);
    static Case span(std::string_view start, std::string_view stop);
    static Case span(std::int64_t start, std::int64_t stop);
    static Case span(double start, double stop);
    static Case span(const Thread& start, const Thread& stop);
    static Case where(const Thread& values);
    static Case where(const pluss::Series& values);
  };

  struct Feat {
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
    std::vector<std::string> name_values{};

    static Feat all();
    static Feat at(std::int64_t position);
    static Feat slice(std::int64_t start, std::int64_t stop);
    static Feat take(std::initializer_list<std::int64_t> positions);
    static Feat take(pluss::RangeView<std::int64_t> positions);
    static Feat name(std::string_view value);
    static Feat name(const char* value);
    static Feat names(std::initializer_list<std::string_view> values);
    static Feat names(pluss::RangeView<std::string_view> values);
    static Feat span(std::string_view start, std::string_view stop);
  };

  struct CastSpec {
    std::string feature;
    ThreadCastTarget target{ThreadCastTarget::Series};
    ThreadCastOptions options{};
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

    // Additional names that resolve to the calling frame. `df` is always accepted.
    std::vector<std::string> primary_source_names{};
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
  pluss::kindle::Catalog kindelize() const;
  pluss::kindle::Catalog kindelize(const pluss::kindle::KindelizePolicy& policy) const;
  pluss::scribble::Registry scribbelize() const;
  pluss::scribble::Registry scribbelize(const pluss::kindle::KindelizePolicy& policy) const;
  static DataFrame select(pluss::msharp::RangeView::Of<Thread> conditions,
                          pluss::msharp::RangeView::Of<DataFrame> choices,
                          const DataFrame& default_value);
  static DataFrame select(std::initializer_list<Thread> conditions,
                          std::initializer_list<DataFrame> choices,
                          const DataFrame& default_value) {
    return select(pluss::msharp::RangeView::Of<Thread>(conditions),
                  pluss::msharp::RangeView::Of<DataFrame>(choices),
                  default_value);
  }

  bool defined() const noexcept;
  std::int64_t ncases() const;
  std::int64_t nfeatures() const;
  std::pair<std::int64_t, std::int64_t> shape() const;
  std::int64_t height() const;
  std::int64_t width() const;
  std::int64_t length() const;
  MatrixValues matrix() const;
  const std::vector<std::string>& features() const;
  DTypes ptypes() const;
  DTypes dtypes() const;
  DataFrameLabelAccessor label() const;
  DataFrameFeatureAxisAccessor feataxis() const;
  DataFrame case_study(const Case& rows) const;
  DataFrame case_study(const CasePick& pick) const;
  DataFrame case_study(const Case& rows, const Feat& features) const;
  std::optional<Thread> get(std::string_view feature_name) const;

  std::string title() const;
  DataFrame title(std::string title) const;
  DataFrame& title_(std::string title);

  Thread features(std::string_view name);
  Thread features(std::string_view name) const;
  DataFrame features(pluss::msharp::RangeView::Of<std::string_view> names);
  DataFrame features(pluss::msharp::RangeView::Of<std::string_view> names) const;
  DataFrame features(StackArray<std::string_view> names);
  DataFrame features(StackArray<std::string_view> names) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame features(const NameRange& names) {
    return features(StackArray<std::string_view>(names));
  }
  template <detail::StringViewNameRange NameRange>
  DataFrame features(const NameRange& names) const {
    return features(StackArray<std::string_view>(names));
  }
  DataFrame features(std::initializer_list<std::string_view> names);
  DataFrame features(std::initializer_list<std::string_view> names) const;
  DataFrame features(std::initializer_list<const char*> names);
  DataFrame features(std::initializer_list<const char*> names) const;
  DataFrame astype(pluss::codem::ScalarType dtype) const;
  DataFrame insert(std::int64_t feature_index, std::string name, const Thread& feature) const;
  DataFrame place(std::string name,
                  const Thread& feature,
                  std::string_view anchor_feature,
                  bool after = false) const;
  DataFrame place(std::string_view name,
                  std::string_view anchor_feature,
                  bool after = false) const;
  DataFrame drop(pluss::msharp::RangeView::Of<std::string_view> names) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame drop(const NameRange& names) const {
    auto owned = StackArray<std::string_view>(names);
    return drop(detail::stack_name_view(owned));
  }
  DataFrame reorder(pluss::msharp::RangeView::Of<std::string_view> names) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame reorder(const NameRange& names) const {
    auto owned = StackArray<std::string_view>(names);
    return reorder(detail::stack_name_view(owned));
  }
  DataFrame reorder(const ReorderSpec& spec) const;
  DataFrame reorder(pluss::RangeView<ReorderSpec> specs) const;
  DataFrame reorder(std::initializer_list<ReorderSpec> specs) const {
    return reorder(pluss::RangeView<ReorderSpec>(specs));
  }
  DataFrame bucketize(pluss::msharp::RangeView::Double bins,
                      pluss::msharp::RangeView::Int labels = {},
                      bool right = true,
                      bool include_lowest = true) const;
  DataFrame arrange(pluss::msharp::RangeView::Of<std::string_view> features,
                    bool ascending = true,
                    bool nulls_last = true) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame arrange(const NameRange& features,
                    bool ascending = true,
                    bool nulls_last = true) const {
    auto owned = StackArray<std::string_view>(features);
    return arrange(detail::stack_name_view(owned), ascending, nulls_last);
  }
  DataFrame arrange(pluss::msharp::RangeView::Of<ArrangeSpec> specs) const;
  DataFrameSortBuilder sort(bool ascending = true, bool nulls_last = true) const;
  DataFrame top(std::string_view feature, std::int64_t n) const;
  DataFrame bottom(std::string_view feature, std::int64_t n) const;
  DataFrameView observe(std::int64_t row, bool render = false) const;
  DataFrameView observe(std::int64_t start, std::int64_t stop, bool render = false) const;
  FrameMatch match(const DataFrame& other,
                   const FrameMatchOptions& options = {}) const;
  DataFrame align(const AlignSpec& spec) const;
  DataFrame align(const Thread& new_label) const;
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
  template <detail::StringViewNameRange LeftRange>
  DataFrame unify(const LeftRange& left_on,
                  const DataFrame& right,
                  std::string_view how = "common",
                  std::string right_suffix = "_right",
                  std::string_view validate = "many_to_many") const {
    auto owned_left = StackArray<std::string_view>(left_on);
    return unify(detail::stack_name_view(owned_left), right, {}, how, std::move(right_suffix), validate);
  }
  template <detail::StringViewNameRange LeftRange, detail::StringViewNameRange RightRange>
  DataFrame unify(const LeftRange& left_on,
                  const DataFrame& right,
                  const RightRange& right_on,
                  std::string_view how = "common",
                  std::string right_suffix = "_right",
                  std::string_view validate = "many_to_many") const {
    auto owned_left = StackArray<std::string_view>(left_on);
    auto owned_right = StackArray<std::string_view>(right_on);
    return unify(detail::stack_name_view(owned_left),
                 right,
                 detail::stack_name_view(owned_right),
                 how,
                 std::move(right_suffix),
                 validate);
  }
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
  DataFrameSqlPrepared prep_sql(std::string_view statement) const;
  DataFrameSqlPrepared prep_sql(std::string_view statement, SqlOptions options) const;
  DataFrameTsAccessor ts() const;
  DataFrameTsAccessor ts(const TsOptions& options) const;
  DataFrameCastAccessor cast() const;
  DataFrameCastAccessor cast(const ThreadCastOptions& options) const;
  DataFrameTimelineAccessor tl() const;
  DataFrameNoteAccessor nt() const;
  LetSlot set(std::string_view name);
  LetSlot set(std::string_view name, const AlignOptions& options);
  LetSlots set(pluss::msharp::RangeView::Of<std::string_view> names);
  LetSlots set(pluss::msharp::RangeView::Of<std::string_view> names, const AlignOptions& options);
  template <detail::StringViewNameRange NameRange>
  LetSlots set(const NameRange& names) {
    auto owned = StackArray<std::string_view>(names);
    return set(detail::stack_name_view(owned));
  }
  template <detail::StringViewNameRange NameRange>
  LetSlots set(const NameRange& names, const AlignOptions& options) {
    auto owned = StackArray<std::string_view>(names);
    return set(detail::stack_name_view(owned), options);
  }
  LetSlots set(std::initializer_list<std::string_view> names) {
    return set(pluss::msharp::RangeView::Of<std::string_view>(names));
  }
  LetSlots set(std::initializer_list<std::string_view> names, const AlignOptions& options) {
    return set(pluss::msharp::RangeView::Of<std::string_view>(names), options);
  }
  LetSlots set(std::initializer_list<const char*> names);
  LetSlots set(std::initializer_list<const char*> names, const AlignOptions& options);
  DataFrame& set_(std::string_view name, const Thread& value);
  DataFrame& set_(std::string_view name, const Thread& value, const AlignOptions& options);
  DataFrame& set_(pluss::msharp::RangeView::Of<std::string_view> names,
                  pluss::msharp::RangeView::Of<Thread> values);

  DataFrame rename(std::string_view old_name, std::string new_name) const;
  DataFrame rename(
      pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> renames) const;
  DataFrame rename(std::initializer_list<std::pair<std::string_view, std::string_view>> renames) const {
    return rename(pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>>(renames));
  }
  DataFrame rename(pluss::msharp::RangeView::Of<std::string_view> feature_names) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame rename(const NameRange& feature_names) const {
    auto owned = StackArray<std::string_view>(feature_names);
    return rename(detail::stack_name_view(owned));
  }
  DataFrame rename(std::initializer_list<const char*> feature_names) const;
  DataFrame& rename_(std::string_view old_name, std::string new_name);
  DataFrame conform(const DataFrame& other) const;

  DataFrameView head(std::int64_t n = 5, bool render = false) const;
  DataFrameView tail(std::int64_t n = 5, bool render = false) const;
  DataFrame sample(std::int64_t n) const;
  DataFrame sample(std::int64_t n, bool replace, Axis axis) const;
  DataFrame sample(std::int64_t n, std::uint64_t random_state, bool replace = false, Axis axis = Axis::Case) const;
  template <class Float>
    requires std::floating_point<Float>
  DataFrame sample(Float frac) const {
    return sample_fraction_(static_cast<double>(frac), std::nullopt, false, Axis::Case);
  }
  template <class Float>
    requires std::floating_point<Float>
  DataFrame sample(Float frac, bool replace, Axis axis) const {
    return sample_fraction_(static_cast<double>(frac), std::nullopt, replace, axis);
  }
  template <class Float>
    requires std::floating_point<Float>
  DataFrame sample(Float frac, std::uint64_t random_state, bool replace = false, Axis axis = Axis::Case) const {
    return sample_fraction_(static_cast<double>(frac), random_state, replace, axis);
  }
  DataFrame shuffle(std::optional<std::uint64_t> random_state = std::nullopt) const;
  DataFrame reshuffle(Axis axis = Axis::Case, std::optional<std::uint64_t> random_state = std::nullopt) const;
  DataFrame shift(std::int64_t periods = 1) const;
  DataFrame slice(std::int64_t start, std::int64_t stop) const;
  DataFrame zoom(std::int64_t case_start,
                 std::int64_t case_stop,
                 std::int64_t feature_start,
                 std::int64_t feature_stop) const;
  DataFrame take(pluss::msharp::RangeView::Int indices) const;
  DataFrame take(const Thread& indices) const;
  DataFrame where(const pluss::Series& bool_mask) const;
  DataFrame where(const Thread& bool_mask) const;
  DataFrame where(const Thread& bool_mask, const AlignOptions& options) const;
  DataFrame filter(const pluss::Series& bool_mask) const;
  DataFrame filter(const Thread& bool_mask) const;
  DataFrame filter(const Thread& bool_mask, const AlignOptions& options) const;
  DataFrame query(std::string_view expression) const;
  DataFrame assign(std::string_view name, const Thread& value) const;
  DataFrame assign(std::string_view name, const std::function<Thread(const DataFrame&)>& expr) const;
  DataFrame assign(pluss::msharp::RangeView::Of<std::pair<std::string, Thread>> assignments) const;
  DataFrame assign(pluss::msharp::RangeView::Of<DeriveSpec> specs) const;
  DataFrame map(
      const std::function<ThreadValue(const ThreadValue&)>& fn) const;
  Thread fold(
      const std::function<Thread(const DataFrame&)>& fn,
      ResultShape shape = ResultShape::Reduce) const;
  DataFrame flow(
      const std::function<DataFrame(const DataFrame&)>& fn) const;
  DataFrame transform(const std::function<Thread(const Thread&)>& fn) const;
  DataFrame transform(const std::function<Thread(const Thread&)>& fn,
                      Axis axis) const;
  Thread transform(const std::function<ThreadValue(const DataFrame&)>& fn,
                   Axis axis) const;
  DataFrame transform(const std::function<DataFrame(const DataFrame&)>& fn,
                      Axis axis,
                      ResultShape shape = ResultShape::Infer) const;
  DataFrame transform(pluss::msharp::RangeView::Of<TransformSpec> specs) const;
  DataFrame put(const Thread& bool_mask, const Thread& value) const;
  DataFrame put(const Thread& bool_mask,
                const Thread& value,
                const AlignOptions& options) const;
  DataFrame put(const Thread& bool_mask, double value) const;
  DataFrame put(const Thread& bool_mask,
                double value,
                const AlignOptions& options) const;
  DataFrame put(const Thread& bool_mask, std::string_view value) const;
  DataFrame put(const Thread& bool_mask,
                std::string_view value,
                const AlignOptions& options) const;
  DataFrame put(const Thread& bool_mask, const char* value) const {
    return put(bool_mask, std::string_view{value != nullptr ? value : ""});
  }
  DataFrame put(const Thread& bool_mask,
                const char* value,
                const AlignOptions& options) const {
    return put(bool_mask, std::string_view{value != nullptr ? value : ""}, options);
  }
  DataFrame put(const Thread& bool_mask, pluss::null_t value) const;
  DataFrame put(const Thread& bool_mask,
                pluss::null_t value,
                const AlignOptions& options) const;
  DataFrame put(const pluss::Series& bool_mask, const Thread& value) const {
    return put(Thread(bool_mask), value);
  }
  DataFrame put(const pluss::Series& bool_mask, double value) const {
    return put(Thread(bool_mask), value);
  }
  DataFrame put(const pluss::Series& bool_mask, std::string_view value) const {
    return put(Thread(bool_mask), value);
  }
  DataFrame put(const pluss::Series& bool_mask, const char* value) const {
    return put(Thread(bool_mask), value);
  }
  DataFrame put(const pluss::Series& bool_mask, pluss::null_t value) const {
    return put(Thread(bool_mask), value);
  }
  DataFrame has(double value) const;
  DataFrame has(std::string_view value) const;
  DataFrame has(const char* value) const {
    return has(std::string_view{value != nullptr ? value : ""});
  }
  DataFrame has(const Thread& values) const;
  DataFrame abs() const;
  DataFrame clip(double min_value, double max_value) const;
  DataFrame round(std::int64_t decimals = 0) const;
  DataFrame log() const;
  DataFrame exp() const;
  DataFrameView aggregate(pluss::msharp::RangeView::Of<SummarySpec> specs, bool render = false) const;
  DataFrameGroupBuilder group() const;
  DataFrameGroupBuilder group(const GroupOptions& options) const;
  ContingencyTable contingency(std::string_view row_feature,
                               std::string_view column_feature) const;
  ContingencyTable contingency(
      std::string_view row_feature,
      std::string_view column_feature,
      const ContingencyOptions& options) const;
  ContingencyTable contingency(
      std::initializer_list<std::string_view> features) const;
  ContingencyTable contingency(
      std::initializer_list<std::string_view> features,
      const ContingencyOptions& options) const;
  ContingencyTable contingency(
      std::initializer_list<std::string_view> row_features,
      std::initializer_list<std::string_view> column_features) const;
  ContingencyTable contingency(
      std::initializer_list<std::string_view> row_features,
      std::initializer_list<std::string_view> column_features,
      const ContingencyOptions& options) const;
  ContingencyTable contingency(
      pluss::msharp::RangeView::Of<std::string_view> features) const;
  ContingencyTable contingency(
      pluss::msharp::RangeView::Of<std::string_view> features,
      const ContingencyOptions& options) const;
  ContingencyTable contingency(
      pluss::msharp::RangeView::Of<std::string_view> row_features,
      pluss::msharp::RangeView::Of<std::string_view> column_features) const;
  ContingencyTable contingency(
      pluss::msharp::RangeView::Of<std::string_view> row_features,
      pluss::msharp::RangeView::Of<std::string_view> column_features,
      const ContingencyOptions& options) const;
  template <detail::StringViewNameRange NameRange>
  ContingencyTable contingency(const NameRange& features) const;
  template <detail::StringViewNameRange NameRange>
  ContingencyTable contingency(const NameRange& features,
                               const ContingencyOptions& options) const;
  template <detail::StringViewNameRange RowRange, detail::StringViewNameRange ColumnRange>
  ContingencyTable contingency(const RowRange& row_features,
                               const ColumnRange& column_features) const;
  template <detail::StringViewNameRange RowRange, detail::StringViewNameRange ColumnRange>
  ContingencyTable contingency(const RowRange& row_features,
                               const ColumnRange& column_features,
                               const ContingencyOptions& options) const;
  DataFrameView pivot(std::string_view case_feature,
                      std::string_view feature_feature,
                      std::string_view value_feature,
                      bool dropnull_keys = true,
                      bool render = false) const;
  DataFrameView pivot_table(std::string_view case_feature,
                            std::string_view feature_feature,
                            std::string_view value_feature,
                            GroupOp op = GroupOp::Mean,
                            bool dropnull_keys = true,
                            bool render = false) const;
  DataFrameView pivot_table(const PivotTableOptions& options) const;
  DataFrameView pivot_table(pluss::msharp::RangeView::Of<std::string_view> index_features,
                            pluss::msharp::RangeView::Of<std::string_view> column_features,
                            pluss::msharp::RangeView::Of<std::string_view> value_features,
                            GroupOp op = GroupOp::Mean,
                            bool dropna = true,
                            bool render = false,
                            bool sort = true) const;
  DataFrameView pivot_table(std::initializer_list<std::string_view> index_features,
                            std::initializer_list<std::string_view> column_features,
                            std::initializer_list<std::string_view> value_features,
                            GroupOp op = GroupOp::Mean,
                            bool dropna = true,
                            bool render = false,
                            bool sort = true) const;
  template <detail::StringViewNameRange IndexRange,
            detail::StringViewNameRange ColumnRange,
            detail::StringViewNameRange ValueRange>
  DataFrameView pivot_table(const IndexRange& index_features,
                            const ColumnRange& column_features,
                            const ValueRange& value_features,
                            GroupOp op = GroupOp::Mean,
                            bool dropna = true,
                            bool render = false,
                            bool sort = true) const;
  DataFrame combine(const DataFrame& other) const;
  DataFrame combine(const DataFrame& other, const CombineSpec& spec) const;
  DataFrame stack(std::string feature_name = "feature",
                  std::string value_name = "value") const;
  DataFrame unstack(const UnstackOptions& options) const;
  DataFrame unstack(std::string_view label) const {
    UnstackOptions options;
    options.label = std::string(label);
    return unstack(options);
  }
  DataFrame extend(const DataFrame& other,
                   FrameMatchRule features = FrameMatchRule::Unite,
                   bool fresh_cases = false,
                   bool reject_repeated_cases = false) const;
  Atlas unsqueeze() const;
  Atlas unsqueeze(const Thread& depth_labels) const;
  DataFrame add(double other) const;
  DataFrame add(const DataFrame& other) const;
  DataFrame add(const DataFrame& other, const AlignOptions& options) const;
  DataFrame add(const Thread& other, Axis axis = Axis::Feature) const;
  DataFrame add(const Thread& other, Axis axis, const AlignOptions& options) const;
  DataFrame add(const Thread& other, const AlignOptions& options, Axis axis = Axis::Feature) const;
  DataFrame sub(double other) const;
  DataFrame sub(const DataFrame& other) const;
  DataFrame sub(const DataFrame& other, const AlignOptions& options) const;
  DataFrame sub(const Thread& other, Axis axis = Axis::Feature) const;
  DataFrame sub(const Thread& other, Axis axis, const AlignOptions& options) const;
  DataFrame sub(const Thread& other, const AlignOptions& options, Axis axis = Axis::Feature) const;
  DataFrame mul(double other) const;
  DataFrame mul(const DataFrame& other) const;
  DataFrame mul(const DataFrame& other, const AlignOptions& options) const;
  DataFrame mul(const Thread& other, Axis axis = Axis::Feature) const;
  DataFrame mul(const Thread& other, Axis axis, const AlignOptions& options) const;
  DataFrame mul(const Thread& other, const AlignOptions& options, Axis axis = Axis::Feature) const;
  DataFrame div(double other) const;
  DataFrame div(const DataFrame& other) const;
  DataFrame div(const DataFrame& other, const AlignOptions& options) const;
  DataFrame div(const Thread& other, Axis axis = Axis::Feature) const;
  DataFrame div(const Thread& other, Axis axis, const AlignOptions& options) const;
  DataFrame div(const Thread& other, const AlignOptions& options, Axis axis = Axis::Feature) const;
  DataFrame truediv(double other) const;
  DataFrame truediv(const DataFrame& other) const;
  DataFrame truediv(const DataFrame& other, const AlignOptions& options) const;
  DataFrame truediv(const Thread& other, Axis axis = Axis::Feature) const;
  DataFrame truediv(const Thread& other, Axis axis, const AlignOptions& options) const;
  DataFrame truediv(const Thread& other, const AlignOptions& options, Axis axis = Axis::Feature) const;
  DataFrame floordiv(double other) const;
  DataFrame floordiv(const DataFrame& other) const;
  DataFrame floordiv(const DataFrame& other, const AlignOptions& options) const;
  DataFrame floordiv(const Thread& other, Axis axis = Axis::Feature) const;
  DataFrame floordiv(const Thread& other, Axis axis, const AlignOptions& options) const;
  DataFrame floordiv(const Thread& other, const AlignOptions& options, Axis axis = Axis::Feature) const;
  DataFrame mod(double other) const;
  DataFrame mod(const DataFrame& other) const;
  DataFrame mod(const DataFrame& other, const AlignOptions& options) const;
  DataFrame mod(const Thread& other, Axis axis = Axis::Feature) const;
  DataFrame mod(const Thread& other, Axis axis, const AlignOptions& options) const;
  DataFrame mod(const Thread& other, const AlignOptions& options, Axis axis = Axis::Feature) const;
  DataFrame pow(double other) const;
  DataFrame pow(const DataFrame& other) const;
  DataFrame pow(const DataFrame& other, const AlignOptions& options) const;
  DataFrame pow(const Thread& other, Axis axis = Axis::Feature) const;
  DataFrame pow(const Thread& other, Axis axis, const AlignOptions& options) const;
  DataFrame pow(const Thread& other, const AlignOptions& options, Axis axis = Axis::Feature) const;
  DataFrame radd(double other) const;
  DataFrame rsub(double other) const;
  DataFrame rmul(double other) const;
  DataFrame rdiv(double other) const;
  DataFrame rtruediv(double other) const;
  DataFrame rfloordiv(double other) const;
  DataFrame rmod(double other) const;
  DataFrame rpow(double other) const;
  DataFrame eq(double other) const;
  DataFrame eq(std::string_view other) const;
  DataFrame eq(const char* other) const { return eq(std::string_view{other != nullptr ? other : ""}); }
  DataFrame eq(const DataFrame& other) const;
  DataFrame eq(const DataFrame& other, const AlignOptions& options) const;
  DataFrame eq(const Thread& other, Axis axis = Axis::Feature) const;
  DataFrame eq(const Thread& other, Axis axis, const AlignOptions& options) const;
  DataFrame eq(const Thread& other, const AlignOptions& options, Axis axis = Axis::Feature) const;
  DataFrame ne(double other) const;
  DataFrame ne(std::string_view other) const;
  DataFrame ne(const char* other) const { return ne(std::string_view{other != nullptr ? other : ""}); }
  DataFrame ne(const DataFrame& other) const;
  DataFrame ne(const DataFrame& other, const AlignOptions& options) const;
  DataFrame ne(const Thread& other, Axis axis = Axis::Feature) const;
  DataFrame ne(const Thread& other, Axis axis, const AlignOptions& options) const;
  DataFrame ne(const Thread& other, const AlignOptions& options, Axis axis = Axis::Feature) const;
  DataFrame lt(double other, bool strict = true) const;
  DataFrame lt(std::string_view other, bool strict = true) const;
  DataFrame lt(const char* other, bool strict = true) const {
    return lt(std::string_view{other != nullptr ? other : ""}, strict);
  }
  DataFrame lt(const DataFrame& other, bool strict = true) const;
  DataFrame lt(const DataFrame& other, const AlignOptions& options, bool strict = true) const;
  DataFrame lt(const Thread& other, Axis axis = Axis::Feature, bool strict = true) const;
  DataFrame lt(const Thread& other, Axis axis, const AlignOptions& options, bool strict = true) const;
  DataFrame lt(const Thread& other, const AlignOptions& options, Axis axis = Axis::Feature, bool strict = true) const;
  DataFrame le(double other) const;
  DataFrame le(std::string_view other) const;
  DataFrame le(const char* other) const { return le(std::string_view{other != nullptr ? other : ""}); }
  DataFrame le(const DataFrame& other) const;
  DataFrame le(const DataFrame& other, const AlignOptions& options) const;
  DataFrame le(const Thread& other, Axis axis = Axis::Feature) const;
  DataFrame le(const Thread& other, Axis axis, const AlignOptions& options) const;
  DataFrame le(const Thread& other, const AlignOptions& options, Axis axis = Axis::Feature) const;
  DataFrame gt(double other, bool strict = true) const;
  DataFrame gt(std::string_view other, bool strict = true) const;
  DataFrame gt(const char* other, bool strict = true) const {
    return gt(std::string_view{other != nullptr ? other : ""}, strict);
  }
  DataFrame gt(const DataFrame& other, bool strict = true) const;
  DataFrame gt(const DataFrame& other, const AlignOptions& options, bool strict = true) const;
  DataFrame gt(const Thread& other, Axis axis = Axis::Feature, bool strict = true) const;
  DataFrame gt(const Thread& other, Axis axis, const AlignOptions& options, bool strict = true) const;
  DataFrame gt(const Thread& other, const AlignOptions& options, Axis axis = Axis::Feature, bool strict = true) const;
  DataFrame ge(double other) const;
  DataFrame ge(std::string_view other) const;
  DataFrame ge(const char* other) const { return ge(std::string_view{other != nullptr ? other : ""}); }
  DataFrame ge(const DataFrame& other) const;
  DataFrame ge(const DataFrame& other, const AlignOptions& options) const;
  DataFrame ge(const Thread& other, Axis axis = Axis::Feature) const;
  DataFrame ge(const Thread& other, Axis axis, const AlignOptions& options) const;
  DataFrame ge(const Thread& other, const AlignOptions& options, Axis axis = Axis::Feature) const;
  DataFrame land(const DataFrame& other) const;
  DataFrame land(const Thread& other) const;
  DataFrame rand(const DataFrame& other) const;
  DataFrame rand(const Thread& other) const;
  DataFrame lor(const DataFrame& other) const;
  DataFrame lor(const Thread& other) const;
  DataFrame ror(const DataFrame& other) const;
  DataFrame ror(const Thread& other) const;
  DataFrame lxor(const DataFrame& other) const;
  DataFrame lxor(const Thread& other) const;
  DataFrame rxor(const DataFrame& other) const;
  DataFrame rxor(const Thread& other) const;
  DataFrame compare(double other) const;
  DataFrame compare(const DataFrame& other) const;
  DataFrame compare(const DataFrame& other, const AlignOptions& options) const;
  DataFrame compare(const Thread& other, Axis axis = Axis::Feature) const;
  DataFrame compare(const Thread& other, Axis axis, const AlignOptions& options) const;
  DataFrame compare(const Thread& other, const AlignOptions& options, Axis axis = Axis::Feature) const;
  Thread sum(Axis axis = Axis::Case, bool skipnull = true) const;
  Thread mean(Axis axis = Axis::Case, bool skipnull = true) const;
  Thread median(Axis axis = Axis::Case, bool skipnull = true) const;
  Thread min(Axis axis = Axis::Case, bool skipnull = true) const;
  Thread max(Axis axis = Axis::Case, bool skipnull = true) const;
  Thread std(Axis axis = Axis::Case, bool skipnull = true) const;
  Thread var(Axis axis = Axis::Case, bool skipnull = true) const;
  Thread count(Axis axis = Axis::Case) const;
  Thread prod(Axis axis = Axis::Case, bool skipnull = true) const;
  Thread any(bool skipnull = true) const;
  Thread any(Axis axis, bool skipnull = true) const;
  Thread all(bool skipnull = true) const;
  Thread all(Axis axis, bool skipnull = true) const;
  Thread csmin(Axis axis = Axis::Case, bool skipnull = true) const;
  Thread csmax(Axis axis = Axis::Case, bool skipnull = true) const;
  Thread ndistinct(bool dropnull = true) const;
  Thread freq() const;
  Thread freq(FreqOptions options) const;
  Thread duplicated(bool keep_first = true,
                    bool keep_last = false) const;
  Thread repeated() const;
  DataFrame drop_duplicates(bool keep_first = true,
                            bool keep_last = false) const;
  DataFrame drop_duplicates(pluss::msharp::RangeView::Of<std::string_view> features,
                            bool keep_first = true,
                            bool keep_last = false) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame drop_duplicates(const NameRange& features,
                            bool keep_first = true,
                            bool keep_last = false) const {
    auto owned = StackArray<std::string_view>(features);
    return drop_duplicates(detail::stack_name_view(owned), keep_first, keep_last);
  }
  DataFrameView corr(CorrMethod method, bool render = false) const;
  DataFrameView corr(bool render = false) const;
  DataFrameView covar(const StatisticOptions& options = {}, bool render = false) const;
  DataFrame percentiles(pluss::msharp::RangeView::Double points = {0.25, 0.5, 0.75},
                        std::int64_t min_count = 1) const;
  DataFrame mode(bool drop_nulls = true) const;
  DataFrame rank(RankTieRule ties = RankTieRule::Average,
                 RankNullRule nulls = RankNullRule::Keep,
                 bool ascending = true,
                 bool percentage = false) const;
  Thread skew(Axis axis = Axis::Case, std::int64_t min_count = 3) const;
  Thread kurtosis(Axis axis = Axis::Case, std::int64_t min_count = 4) const;
  Thread var(Axis axis, const StatisticOptions& options) const;
  Thread std(Axis axis, const StatisticOptions& options) const;
  Thread se(Axis axis = Axis::Case,
            ErrorTarget target = ErrorTarget::Mean,
            const StatisticOptions& options = {}) const;
  DataFrame scan(ScanStatistic statistic, bool skip_nulls = true) const;
  DataFrame scan(ScanStatistic statistic, Axis axis, bool skip_nulls = true) const;
  DataFrame cumsum(bool skip_nulls = true) const;
  DataFrame cumsum(Axis axis, bool skip_nulls = true) const;
  DataFrame cumprod(bool skip_nulls = true) const;
  DataFrame cumprod(Axis axis, bool skip_nulls = true) const;
  DataFrame cummin(bool skip_nulls = true) const;
  DataFrame cummin(Axis axis, bool skip_nulls = true) const;
  DataFrame cummax(bool skip_nulls = true) const;
  DataFrame cummax(Axis axis, bool skip_nulls = true) const;
  Thread corr_with(const DataFrame& other,
                   CorrMethod method = CorrMethod::Pearson,
                   std::int64_t min_count = 1) const;
  DataFrame corr_with(pluss::msharp::RangeView::Of<DataFrame> others,
                      CorrMethod method = CorrMethod::Pearson,
                      std::int64_t min_count = 1) const;
  DataFrame matmul(const DataFrame& other) const;
  DataFrame isnull(pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame notnull(pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame invalid(pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame shownull(Axis axis = Axis::Case, bool all = false) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame isnull(const NameRange& features) const {
    return detail::with_stack_name_view(features, [&](auto names) { return isnull(names); });
  }
  template <detail::StringViewNameRange NameRange>
  DataFrame notnull(const NameRange& features) const {
    return detail::with_stack_name_view(features, [&](auto names) { return notnull(names); });
  }
  template <detail::StringViewNameRange NameRange>
  DataFrame invalid(const NameRange& features) const {
    return detail::with_stack_name_view(features, [&](auto names) { return invalid(names); });
  }
  DataFrame astype(
      std::initializer_list<std::pair<std::string_view, pluss::codem::ScalarType>> feature_dtypes) const;
  DataFrame astype(
      pluss::msharp::RangeView::Of<std::pair<std::string_view, pluss::codem::ScalarType>> feature_dtypes) const;
  DataFrame fillnull(double value) const;
  DataFrame fillnull(double value, pluss::msharp::RangeView::Of<std::string_view> features) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame fillnull(double value, const NameRange& features) const {
    return detail::with_stack_name_view(features, [&](auto names) { return fillnull(value, names); });
  }
  DataFrame fillnull(std::string_view value) const;
  DataFrame fillnull(std::string_view value, pluss::msharp::RangeView::Of<std::string_view> features) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame fillnull(std::string_view value, const NameRange& features) const {
    return detail::with_stack_name_view(features, [&](auto names) { return fillnull(value, names); });
  }
  DataFrame fillnull(const char* value) const {
    return fillnull(std::string_view{value != nullptr ? value : ""});
  }
  DataFrame fillnull(const char* value, pluss::msharp::RangeView::Of<std::string_view> features) const {
    return fillnull(std::string_view{value != nullptr ? value : ""}, features);
  }
  template <detail::StringViewNameRange NameRange>
  DataFrame fillnull(const char* value, const NameRange& features) const {
    return fillnull(std::string_view{value != nullptr ? value : ""}, features);
  }
  DataFrame fillnull(const pluss::Timeline& value) const;
  DataFrame fillnull(const pluss::Timeline& value, pluss::msharp::RangeView::Of<std::string_view> features) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame fillnull(const pluss::Timeline& value, const NameRange& features) const {
    return detail::with_stack_name_view(features, [&](auto names) { return fillnull(value, names); });
  }
  DataFrame fillnull(const pluss::TimeIndex& value) const;
  DataFrame fillnull(const pluss::TimeIndex& value, pluss::msharp::RangeView::Of<std::string_view> features) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame fillnull(const pluss::TimeIndex& value, const NameRange& features) const {
    return detail::with_stack_name_view(features, [&](auto names) { return fillnull(value, names); });
  }
  DataFrame fillnull(pluss::msharp::RangeView::Of<FillSpec> specs) const;
  DataFrame fillnull(std::initializer_list<FillSpec> specs) const {
    return fillnull(pluss::msharp::RangeView::Of<FillSpec>(specs));
  }
  DataFrame replace(double from, double to, pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame replace(double from, double to, const NameRange& features) const {
    return detail::with_stack_name_view(features, [&](auto names) { return replace(from, to, names); });
  }
  DataFrame replace(double from, pluss::null_t to, pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame replace(double from, pluss::null_t to, const NameRange& features) const {
    return detail::with_stack_name_view(features, [&](auto names) { return replace(from, to, names); });
  }
  DataFrame replace(pluss::null_t from, double to, pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame replace(pluss::null_t from, double to, const NameRange& features) const {
    return detail::with_stack_name_view(features, [&](auto names) { return replace(from, to, names); });
  }
  DataFrame replace(std::string_view from,
                    std::string_view to,
                    pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame replace(std::string_view from, std::string_view to, const NameRange& features) const {
    return detail::with_stack_name_view(features, [&](auto names) { return replace(from, to, names); });
  }
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
  template <detail::StringViewNameRange NameRange>
  DataFrame replace(std::string_view from, pluss::null_t to, const NameRange& features) const {
    return detail::with_stack_name_view(features, [&](auto names) { return replace(from, to, names); });
  }
  DataFrame replace(pluss::null_t from,
                    std::string_view to,
                    pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame replace(pluss::null_t from, std::string_view to, const NameRange& features) const {
    return detail::with_stack_name_view(features, [&](auto names) { return replace(from, to, names); });
  }
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
  template <detail::StringViewNameRange NameRange>
  DataFrame replace(const pluss::Series& from, const pluss::Series& to, const NameRange& features) const {
    return detail::with_stack_name_view(features, [&](auto names) { return replace(from, to, names); });
  }
  DataFrame replace(const pluss::Series& from,
                    pluss::null_t to,
                    pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame replace(const pluss::Series& from, pluss::null_t to, const NameRange& features) const {
    return detail::with_stack_name_view(features, [&](auto names) { return replace(from, to, names); });
  }
  DataFrame replace(pluss::null_t from,
                    const pluss::Series& to,
                    pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame replace(pluss::null_t from, const pluss::Series& to, const NameRange& features) const {
    return detail::with_stack_name_view(features, [&](auto names) { return replace(from, to, names); });
  }
  DataFrame replace(const pluss::Note& from,
                    const pluss::Note& to,
                    pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame replace(const pluss::Note& from, const pluss::Note& to, const NameRange& features) const {
    return detail::with_stack_name_view(features, [&](auto names) { return replace(from, to, names); });
  }
  DataFrame replace(const pluss::Note& from,
                    pluss::null_t to,
                    pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame replace(const pluss::Note& from, pluss::null_t to, const NameRange& features) const {
    return detail::with_stack_name_view(features, [&](auto names) { return replace(from, to, names); });
  }
  DataFrame replace(pluss::null_t from,
                    const pluss::Note& to,
                    pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame replace(pluss::null_t from, const pluss::Note& to, const NameRange& features) const {
    return detail::with_stack_name_view(features, [&](auto names) { return replace(from, to, names); });
  }
  DataFrame replace(const pluss::Timeline& from,
                    const pluss::Timeline& to,
                    pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame replace(const pluss::Timeline& from, const pluss::Timeline& to, const NameRange& features) const {
    return detail::with_stack_name_view(features, [&](auto names) { return replace(from, to, names); });
  }
  DataFrame replace(const pluss::Timeline& from,
                    pluss::null_t to,
                    pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame replace(const pluss::Timeline& from, pluss::null_t to, const NameRange& features) const {
    return detail::with_stack_name_view(features, [&](auto names) { return replace(from, to, names); });
  }
  DataFrame replace(pluss::null_t from,
                    const pluss::Timeline& to,
                    pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame replace(pluss::null_t from, const pluss::Timeline& to, const NameRange& features) const {
    return detail::with_stack_name_view(features, [&](auto names) { return replace(from, to, names); });
  }
  DataFrame replace(const pluss::TimeIndex& from,
                    const pluss::TimeIndex& to,
                    pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame replace(const pluss::TimeIndex& from, const pluss::TimeIndex& to, const NameRange& features) const {
    return detail::with_stack_name_view(features, [&](auto names) { return replace(from, to, names); });
  }
  DataFrame replace(const pluss::TimeIndex& from,
                    pluss::null_t to,
                    pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame replace(const pluss::TimeIndex& from, pluss::null_t to, const NameRange& features) const {
    return detail::with_stack_name_view(features, [&](auto names) { return replace(from, to, names); });
  }
  DataFrame replace(pluss::null_t from,
                    const pluss::TimeIndex& to,
                    pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame replace(pluss::null_t from, const pluss::TimeIndex& to, const NameRange& features) const {
    return detail::with_stack_name_view(features, [&](auto names) { return replace(from, to, names); });
  }
  DataFrame& fillnull_(double value);
  DataFrame& fillnull_(std::string_view value);
  DataFrame& fillnull_(const char* value) {
    return fillnull_(std::string_view{value != nullptr ? value : ""});
  }
  DataFrame& fillnull_(const pluss::Timeline& value);
  DataFrame& fillnull_(const pluss::TimeIndex& value);
  DataFrame& put_(std::string_view feature, const Thread& bool_mask, const Thread& value);
  DataFrame& put_(std::string_view feature,
                  const Thread& bool_mask,
                  const Thread& value,
                  const AlignOptions& options);
  DataFrame& put_(std::string_view feature, const Thread& bool_mask, double value);
  DataFrame& put_(std::string_view feature,
                  const Thread& bool_mask,
                  double value,
                  const AlignOptions& options);
  DataFrame& put_(std::string_view feature, const Thread& bool_mask, std::string_view value);
  DataFrame& put_(std::string_view feature,
                  const Thread& bool_mask,
                  std::string_view value,
                  const AlignOptions& options);
  DataFrame& put_(std::string_view feature, const Thread& bool_mask, const char* value) {
    return put_(feature, bool_mask, std::string_view{value != nullptr ? value : ""});
  }
  DataFrame& put_(std::string_view feature,
                  const Thread& bool_mask,
                  const char* value,
                  const AlignOptions& options) {
    return put_(feature, bool_mask, std::string_view{value != nullptr ? value : ""}, options);
  }
  DataFrame& put_(std::string_view feature, const Thread& bool_mask, pluss::null_t value);
  DataFrame& put_(std::string_view feature,
                  const Thread& bool_mask,
                  pluss::null_t value,
                  const AlignOptions& options);
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
  DataFrame dropnull(pluss::msharp::RangeView::Of<std::string_view> features = {},
                     bool all = false) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame dropnull(const NameRange& features, bool all = false) const {
    auto owned = StackArray<std::string_view>(features);
    return dropnull(detail::stack_name_view(owned), all);
  }
  DataFrame dropnull(const DropOptions& options) const;
  DataFrame ffill(std::int64_t limit = -1) const;
  DataFrame ffill(const FillOptions& options) const;
  DataFrame bfill(std::int64_t limit = -1) const;
  DataFrame bfill(const FillOptions& options) const;
  DataFrame interp(pluss::msharp::RangeView::Of<std::string_view> features = {},
                   std::int64_t limit = -1) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame interp(const NameRange& features, std::int64_t limit = -1) const {
    return detail::with_stack_name_view(features, [&](auto names) { return interp(names, limit); });
  }
  DataFrame interp(const FillOptions& options) const;
  DataFrame mask(const Thread& bool_mask,
                 pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame mask(const Thread& bool_mask, const NameRange& features) const {
    return detail::with_stack_name_view(features, [&](auto names) { return mask(bool_mask, names); });
  }
  DataFrame mask(const Thread& bool_mask,
                 const AlignOptions& options,
                 pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame mask(const Thread& bool_mask, const AlignOptions& options, const NameRange& features) const {
    return detail::with_stack_name_view(features, [&](auto names) { return mask(bool_mask, options, names); });
  }
  DataFrame mask(const Thread& bool_mask,
                 pluss::msharp::RangeView::Of<std::string_view> features,
                 const AlignOptions& options) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame mask(const Thread& bool_mask, const NameRange& features, const AlignOptions& options) const {
    return detail::with_stack_name_view(features, [&](auto names) { return mask(bool_mask, names, options); });
  }
  DataFrameView describe(bool render = false) const;
  DataFrameView transpose(bool render = false) const;
  DataFrame lengthen(
      pluss::msharp::RangeView::Of<std::string_view> id_features = {},
      pluss::msharp::RangeView::Of<std::string_view> value_features = {},
      std::string feature_name = "feature",
      std::string value_name = "value",
      bool fresh_cases = true) const;
  template <detail::StringViewNameRange IdRange, detail::StringViewNameRange ValueRange>
  DataFrame lengthen(const IdRange& id_features,
                     const ValueRange& value_features,
                     std::string feature_name = "feature",
                     std::string value_name = "value",
                     bool fresh_cases = true) const {
    return detail::with_stack_name_views(id_features, value_features, [&](auto ids, auto values) {
      return lengthen(ids, values, std::move(feature_name), std::move(value_name), fresh_cases);
    });
  }
  CategoryEncoding factor(std::string_view feature,
                          bool ordered = false,
                          bool sort_levels = false) const;
  DataFrame encode(std::string_view feature,
                   EncodeMode mode = EncodeMode::OneHot,
                   const EncodeOptions& options = {}) const;
  DataFrameView tally(std::int64_t top_n = 0, bool dropnull = true, bool render = false) const;
  DataFrameView tally(pluss::msharp::RangeView::Of<std::string_view> features,
                      std::int64_t top_n = 0,
                      bool dropnull = true,
                      bool render = false) const;
  template <detail::StringViewNameRange NameRange>
  DataFrameView tally(const NameRange& features,
                      std::int64_t top_n = 0,
                      bool dropnull = true,
                      bool render = false) const;
  DataFrameView tally(std::string_view feature,
                      std::int64_t top_n = 0,
                      bool dropnull = true,
                      bool render = false) const;
  DataFrameView view(std::int64_t start, std::int64_t stop, bool render = false) const;
  std::pair<DataFrame, DataFrame> split(double frac) const;
  std::pair<DataFrame, DataFrame> split(double frac,
                                        const SplitOptions& options) const;
  std::pair<DataFrame, DataFrame> split(double frac,
                                        Axis axis,
                                        bool shuffle = false) const;
  std::pair<DataFrame, DataFrame> split(double frac,
                                        std::uint64_t random_state,
                                        Axis axis = Axis::Case,
                                        bool shuffle = true) const;
  DataFrame copy() const;
  bool equals(const DataFrame& other) const;
  bool equals(const DataFrame& other, double atol, double rtol) const;

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
                std::string null_rep = "",
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
                  std::string null_rep = "",
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
  friend class DataFrameFeatureAxisAccessor;
  friend class DataFrameLabelAccessor;
  friend class DataFrameSortBuilder;
  explicit DataFrame(pluss::msharp::intrusive_ptr<detail::DataFrameImpl> impl);
  detail::DataFrameImpl& mut_impl_();
  const detail::DataFrameImpl& impl_ref_() const;
  bool unique() const noexcept;
  bool has_feature(std::string_view name) const;
  ColumnRef feature_ref(std::string_view name) const;
  DataFrame project_names_(pluss::msharp::RangeView::Of<std::string_view> names) const;
  DataFrame select_like(std::string_view needle, bool case_sensitive = true) const;
  DataFrame match_features(std::string_view pattern) const;
  DataFrame series_features() const;
  DataFrame note_features() const;
  DataFrame timeline_features() const;
  DataFrame add_prefix(std::string_view prefix) const;
  DataFrame add_suffix(std::string_view suffix) const;
  DataFrame astype(pluss::msharp::RangeView::Of<std::string_view> features,
                   pluss::codem::ScalarType dtype) const;
  DataFrame window(pluss::msharp::RangeView::Of<WindowSpec> specs, bool keep_input = true) const;
  DataFrame rolling(pluss::msharp::RangeView::Of<WindowSpec> specs, bool keep_input = true) const;
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
  DataFrame unique_cases(pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame set_axis(pluss::msharp::RangeView::Of<std::string_view> feature_names) const;
  DataFrame set_features(std::string_view name, const Thread& value) const;
  DataFrame set_features(std::string_view name, const Thread& value, const AlignOptions& options) const;
  DataFrame& set_features_(std::string_view name, const Thread& value);
  DataFrame& set_features_(std::string_view name, const Thread& value, const AlignOptions& options);
  DataFrame set_features(pluss::msharp::RangeView::Of<std::string_view> names,
                         pluss::msharp::RangeView::Of<Thread> values) const;
  DataFrame& set_features_(pluss::msharp::RangeView::Of<std::string_view> names,
                           pluss::msharp::RangeView::Of<Thread> values);
  const Thread& case_labels_() const;
  const Thread& caselbl() const;
  DataFrame caselbl(const Thread& labels) const;
  DataFrame& caselbl_(const Thread& labels);
  bool has_caselbls() const;
  std::vector<std::string> caselbls() const;
  Thread caselbl(std::string_view name) const;
  Thread caselbl(std::int64_t position) const;
  DataFrame sort_caselbl(bool ascending = true, bool nulls_last = true) const;
  DataFrame sort_caselbl(const SortLabelOptions& options) const;
  pluss::Series repeated_caselbl(pluss::msharp::RangeView::Of<std::string_view> features = {}) const;
  DataFrame set_caselbl(std::string_view feature_name, bool drop_feature = true) const;
  DataFrame set_caselbl(std::initializer_list<std::string_view> feature_names,
                        bool drop_features = true,
                        std::string_view separator = "\x1F") const;
  DataFrame set_caselbl(pluss::msharp::RangeView::Of<std::string_view> feature_names,
                        bool drop_features = true,
                        std::string_view separator = "\x1F") const;
  DataFrame set_caselbl(const SetLabelOptions& options) const;
  DataFrame reset_caselbl(std::string case_feature = {}) const;
  DataFrame reset_caselbl(const ResetLabelOptions& options) const;
  DataFrame rename_caselbls(pluss::msharp::RangeView::Of<LabelRename> renames) const;
  DataFrame rename_caselbls(std::initializer_list<LabelRename> renames) const {
    return rename_caselbls(pluss::msharp::RangeView::Of<LabelRename>(renames));
  }
  DataFrame order_caselbls(pluss::msharp::RangeView::Of<std::string_view> names) const;
  DataFrame order_caselbls(std::initializer_list<std::string_view> names) const {
    return order_caselbls(pluss::msharp::RangeView::Of<std::string_view>(names));
  }
  DataFrame drop_caselbls(pluss::msharp::RangeView::Of<std::string_view> names) const;
  DataFrame drop_caselbls(std::initializer_list<std::string_view> names) const {
    return drop_caselbls(pluss::msharp::RangeView::Of<std::string_view>(names));
  }
  DataFrame align_caselbl(const Thread& new_label) const;
  DataFrame align_caselbl(const Thread& new_label, std::optional<double> fill) const;
  DataFrame with_cases(const Thread& cases) const;
  DataFrame& with_cases_(const Thread& cases);
  void suppress_next_stream_() const;
  bool consume_stream_suppression_() const;
  DataFrame query(const pluss::Series& bool_mask) const;
  DataFrame query(const QueryFn& predicate) const;
  DataFrame shift(pluss::msharp::RangeView::Of<std::string_view> features, std::int64_t periods = 1) const;
  DataFrame sample_impl_(std::int64_t n,
                         std::optional<std::uint64_t> random_state,
                         bool replace,
                         Axis axis) const;
  DataFrame sample_fraction_(double frac,
                             std::optional<std::uint64_t> random_state,
                             bool replace,
                             Axis axis) const;
  DataFrame summarize_specs(const std::vector<SummarySpec>& specs, bool render = false) const;
  DataFrameView tally_impl(const std::vector<std::string>& feature_names,
                           std::int64_t top_n,
                           bool dropnull,
                           bool render) const;
  DataFrame map(
      const std::function<ThreadValue(const ThreadValue&)>& fn,
      pluss::msharp::RangeView::Of<std::string_view> features) const;
  DataFrame summarize(pluss::msharp::RangeView::Of<SummarySpec> specs, bool render = false) const;
  DataFrame group(pluss::msharp::RangeView::Of<std::string_view> keys,
                  pluss::msharp::RangeView::Of<GroupSpec> specs,
                  bool dropnull_keys = true,
                  bool render = false,
                  bool sort_groups = false) const;
  DataFrame cast(pluss::msharp::RangeView::Of<CastSpec> specs) const;
  DataFrame cast(
      std::initializer_list<std::pair<std::string_view, pluss::codem::ScalarType>> feature_dtypes) const;
  DataFrame cast(
      pluss::msharp::RangeView::Of<std::pair<std::string_view, pluss::codem::ScalarType>> feature_dtypes) const;
  DataFrame cast(
      std::initializer_list<std::pair<std::string_view, ThreadCastTarget>> feature_targets) const;
  DataFrame cast(
      pluss::msharp::RangeView::Of<std::pair<std::string_view, ThreadCastTarget>> feature_targets) const;
  DataFrame transform(std::string_view feature, const std::function<Thread(const DataFrame&)>& fn) const;
  DataFrame transform(std::string_view feature, const std::function<Thread(const Thread&)>& fn) const;
  DataFrame put(std::string_view feature, const Thread& bool_mask, const Thread& value) const;
  DataFrame put(std::string_view feature,
                const Thread& bool_mask,
                const Thread& value,
                const AlignOptions& options) const;
  DataFrame put(std::string_view feature, const Thread& bool_mask, double value) const;
  DataFrame put(std::string_view feature,
                const Thread& bool_mask,
                double value,
                const AlignOptions& options) const;
  DataFrame put(std::string_view feature, const Thread& bool_mask, std::string_view value) const;
  DataFrame put(std::string_view feature,
                const Thread& bool_mask,
                std::string_view value,
                const AlignOptions& options) const;
  DataFrame put(std::string_view feature, const Thread& bool_mask, pluss::null_t value) const;
  DataFrame put(std::string_view feature,
                const Thread& bool_mask,
                pluss::null_t value,
                const AlignOptions& options) const;
  Thread any(pluss::msharp::RangeView::Of<std::string_view> features, bool skipnull) const;
  Thread any(Axis axis,
             pluss::msharp::RangeView::Of<std::string_view> features,
             bool skipnull) const;
  Thread all(pluss::msharp::RangeView::Of<std::string_view> features, bool skipnull) const;
  Thread all(Axis axis,
             pluss::msharp::RangeView::Of<std::string_view> features,
             bool skipnull) const;
  Thread ndistinct(pluss::msharp::RangeView::Of<std::string_view> features,
                   bool dropnull) const;
  Thread duplicated(pluss::msharp::RangeView::Of<std::string_view> features,
                    bool keep_first,
                    bool keep_last) const;
  Thread repeated(pluss::msharp::RangeView::Of<std::string_view> features) const;
  DataFrameView corr(pluss::msharp::RangeView::Of<std::string_view> features, bool render) const;
  DataFrameView corr(pluss::msharp::RangeView::Of<std::string_view> features,
                     CorrMethod method,
                     bool render) const;
  DataFrame missing(const MissingSpec& spec) const;
  DataFrame missing(MissingOp op,
                    pluss::msharp::RangeView::Of<std::string_view> features = {},
                    std::optional<double> value = std::nullopt) const;

  pluss::msharp::intrusive_ptr<detail::DataFrameImpl> impl_{nullptr};
  friend void output_render(std::ostream& os, const DataFrame& frame);
  friend std::ostream& operator<<(std::ostream& os, const DataFrame& frame);
  friend class DataFrameNoteAccessor;
  friend class DataFrameTimelineAccessor;
  friend class DataFrameTsAccessor;
  friend class DataFrameCastAccessor;
  friend class DataFrameGroupBuilder;
  friend class DataFrameGrouped;
};

struct FrameMatch {
  DataFrame left{};
  DataFrame right{};
};

std::ostream& operator<<(std::ostream& os, const DataFrame& frame);
std::ostream& operator<<(std::ostream& os, const DataFrame::DTypes& dtypes);
void output_render(std::ostream& os, const DataFrame& frame);

class DataFrameLabelAccessor {
public:
  explicit DataFrameLabelAccessor(const DataFrame& frame);

  Thread values() const;
  DataFrame set(const Thread& labels) const;
  bool has() const;
  std::vector<std::string> names() const;
  Thread part(std::string_view name) const;
  Thread part(std::int64_t position) const;

  DataFrame set(std::string_view feature_name, bool drop_feature = true) const;
  DataFrame set(std::initializer_list<std::string_view> feature_names,
                bool drop_features = true,
                std::string_view separator = "\x1F") const;
  DataFrame set(pluss::msharp::RangeView::Of<std::string_view> feature_names,
                bool drop_features = true,
                std::string_view separator = "\x1F") const;
  template <detail::StringViewNameRange NameRange>
  DataFrame set(const NameRange& feature_names,
                bool drop_features = true,
                std::string_view separator = "\x1F") const {
    return detail::with_stack_name_view(feature_names, [&](auto names) {
      return set(names, drop_features, separator);
    });
  }
  DataFrame set(const DataFrame::SetLabelOptions& options) const;

  DataFrame reset(std::string case_feature = {}) const;
  DataFrame reset(const DataFrame::ResetLabelOptions& options) const;
  DataFrame sort(bool ascending = true, bool nulls_last = true) const;
  DataFrame sort(const DataFrame::SortLabelOptions& options) const;
  DataFrame align(const Thread& new_label) const;
  DataFrame align(const Thread& new_label, std::optional<double> fill) const;
  Thread unite(const Thread& other) const;
  Thread intersect(const Thread& other) const;
  Thread diff(const Thread& other) const;
  Thread symdiff(const Thread& other) const;
  Thread find(const Thread& targets) const;
  Thread pos(double value) const;
  Thread pos(std::string_view value) const;
  Thread pos(const char* value) const {
    return pos(std::string_view{value != nullptr ? value : ""});
  }
  Thread slice(std::string_view start, std::string_view stop) const;
  DataFrame rename(pluss::msharp::RangeView::Of<DataFrame::LabelRename> renames) const;
  DataFrame rename(std::initializer_list<DataFrame::LabelRename> renames) const;
  DataFrame order(pluss::msharp::RangeView::Of<std::string_view> names) const;
  DataFrame order(std::initializer_list<std::string_view> names) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame order(const NameRange& names) const {
    return detail::with_stack_name_view(names, [&](auto values) { return order(values); });
  }
  DataFrame names(pluss::msharp::RangeView::Of<std::string_view> names) const;
  DataFrame names(std::initializer_list<std::string_view> names) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame names(const NameRange& names_) const {
    return detail::with_stack_name_view(names_, [&](auto values) { return names(values); });
  }
  DataFrame drop(pluss::msharp::RangeView::Of<std::string_view> names) const;
  DataFrame drop(std::initializer_list<std::string_view> names) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame drop(const NameRange& names) const {
    return detail::with_stack_name_view(names, [&](auto values) { return drop(values); });
  }
  DataFrame drop(const DataFrame::LabelDropOptions& options) const;
  Thread duplicated(pluss::msharp::RangeView::Of<std::string_view> names,
                    bool keep_first = true,
                    bool keep_last = false) const;
  Thread duplicated(std::initializer_list<std::string_view> names,
                    bool keep_first = true,
                    bool keep_last = false) const;
  template <detail::StringViewNameRange NameRange>
  Thread duplicated(const NameRange& names,
                    bool keep_first = true,
                    bool keep_last = false) const {
    return detail::with_stack_name_view(names, [&](auto values) {
      return duplicated(values, keep_first, keep_last);
    });
  }
  Thread duplicated(bool keep_first = true, bool keep_last = false) const;
  Thread repeated(pluss::msharp::RangeView::Of<std::string_view> names) const;
  Thread repeated(std::initializer_list<std::string_view> names) const;
  template <detail::StringViewNameRange NameRange>
  Thread repeated(const NameRange& names) const {
    return detail::with_stack_name_view(names, [&](auto values) { return repeated(values); });
  }
  Thread repeated() const;
  Thread distinct(bool dropnull = true) const;
  Thread freq(std::int64_t top_n = 0, bool dropnull = true) const;
  Thread has(double value) const;
  Thread has(std::string_view value) const;
  Thread has(const char* value) const {
    return has(std::string_view{value != nullptr ? value : ""});
  }
  Thread has(const Thread& values) const;
  Thread fillnull(double value) const;
  Thread fillnull(std::string_view value) const;
  Thread fillnull(const char* value) const {
    return fillnull(std::string_view{value != nullptr ? value : ""});
  }
  Thread fillnull(const pluss::Timeline& value) const;
  Thread fillnull(const pluss::TimeIndex& value) const;
  Thread isnull() const;
  Thread notnull() const;
  DataFrame map(const std::function<ThreadValue(const ThreadValue&)>& fn) const;
  Thread take(pluss::msharp::RangeView::Int indices) const;
  Thread slice(std::int64_t start, std::int64_t stop) const;
  DataFrame swap(std::string_view left, std::string_view right) const;
  bool monotonic(bool ascending = true, bool strict = false, bool nulls_last = true) const;
  std::int64_t depth() const;
  DataFrame codes() const;
  DataFrame levels() const;

private:
  DataFrame frame_{};
};

class DataFrameFeatureAxisAccessor {
public:
  explicit DataFrameFeatureAxisAccessor(const DataFrame& frame);

  DataFrame like(std::string_view needle, bool case_sensitive = true) const;
  DataFrame match(std::string_view pattern) const;
  DataFrame series() const;
  DataFrame note() const;
  DataFrame timeline() const;
  DataFrame prefix(std::string_view prefix) const;
  DataFrame suffix(std::string_view suffix) const;

private:
  DataFrame frame_{};
};

class DataFrameCastAccessor {
public:
  explicit DataFrameCastAccessor(const DataFrame& frame, ThreadCastOptions options = {});

  DataFrame series() const;
  DataFrame note() const;
  DataFrame timeline() const;
  DataFrame timeindex() const;

private:
  DataFrame cast_all_(ThreadCastTarget target) const;

  DataFrame frame_{};
  ThreadCastOptions options_{};
};

class DataFrameGroupBuilder {
public:
  explicit DataFrameGroupBuilder(DataFrame frame, DataFrame::GroupOptions options = {});

  DataFrameGrouped by(std::string_view key) const;
  DataFrameGrouped by(std::initializer_list<std::string_view> keys) const;
  DataFrameGrouped by(pluss::msharp::RangeView::Of<std::string_view> keys) const;
  DataFrameGrouped by(const DataFrame::LabelGroup& keys) const;

  template <detail::StringViewNameRange NameRange>
  DataFrameGrouped by(const NameRange& keys) const;

private:
  DataFrame frame_{};
  DataFrame::GroupOptions options_{};
};

class DataFrameGrouped {
public:
  DataFrameGrouped(DataFrame frame,
                   std::vector<std::string> keys,
                   DataFrame::GroupOptions options = {});
  DataFrameGrouped(DataFrame frame,
                   DataFrame::LabelGroup labels,
                   DataFrame::GroupOptions options = {});

  DataFrame agg(pluss::msharp::RangeView::Of<DataFrame::GroupSpec> specs) const;
  DataFrame agg(std::initializer_list<DataFrame::GroupSpec> specs) const;
  DataFrame transform(pluss::msharp::RangeView::Of<DataFrame::GroupSpec> specs) const;
  DataFrame transform(std::initializer_list<DataFrame::GroupSpec> specs) const;
  DataFrame filter(const pluss::Series& keep) const;
  DataFrame filter(const Thread& keep) const;

  DataFrame count() const;
  DataFrame count(std::string_view feature) const;
  DataFrame count(std::initializer_list<std::string_view> features) const;
  DataFrame count(pluss::msharp::RangeView::Of<std::string_view> features) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame count(const NameRange& features) const;
  DataFrame ndistinct() const;
  DataFrame ndistinct(std::string_view feature) const;
  DataFrame ndistinct(std::initializer_list<std::string_view> features) const;
  DataFrame ndistinct(pluss::msharp::RangeView::Of<std::string_view> features) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame ndistinct(const NameRange& features) const;
  DataFrame sum() const;
  DataFrame sum(std::string_view feature) const;
  DataFrame sum(std::initializer_list<std::string_view> features) const;
  DataFrame sum(pluss::msharp::RangeView::Of<std::string_view> features) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame sum(const NameRange& features) const;
  DataFrame mean() const;
  DataFrame mean(std::string_view feature) const;
  DataFrame mean(std::initializer_list<std::string_view> features) const;
  DataFrame mean(pluss::msharp::RangeView::Of<std::string_view> features) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame mean(const NameRange& features) const;
  DataFrame median() const;
  DataFrame median(std::string_view feature) const;
  DataFrame median(std::initializer_list<std::string_view> features) const;
  DataFrame median(pluss::msharp::RangeView::Of<std::string_view> features) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame median(const NameRange& features) const;
  DataFrame min() const;
  DataFrame min(std::string_view feature) const;
  DataFrame min(std::initializer_list<std::string_view> features) const;
  DataFrame min(pluss::msharp::RangeView::Of<std::string_view> features) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame min(const NameRange& features) const;
  DataFrame max() const;
  DataFrame max(std::string_view feature) const;
  DataFrame max(std::initializer_list<std::string_view> features) const;
  DataFrame max(pluss::msharp::RangeView::Of<std::string_view> features) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame max(const NameRange& features) const;
  DataFrame std() const;
  DataFrame std(std::string_view feature) const;
  DataFrame std(std::initializer_list<std::string_view> features) const;
  DataFrame std(pluss::msharp::RangeView::Of<std::string_view> features) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame std(const NameRange& features) const;
  DataFrame var() const;
  DataFrame var(std::string_view feature) const;
  DataFrame var(std::initializer_list<std::string_view> features) const;
  DataFrame var(pluss::msharp::RangeView::Of<std::string_view> features) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame var(const NameRange& features) const;
  DataFrame mode() const;
  DataFrame mode(std::string_view feature) const;
  DataFrame mode(std::initializer_list<std::string_view> features) const;
  DataFrame mode(pluss::msharp::RangeView::Of<std::string_view> features) const;
  template <detail::StringViewNameRange NameRange>
  DataFrame mode(const NameRange& features) const;
  DataFrame yield() const;  // returns the dataframe unaggregated after grouping

  // Standalone terminal: unlike every op above, a group with a tie can
  // legitimately produce more than one output row, so this cannot be
  // expressed as a GroupSpec/agg() operation (agg()/transform() require
  // exactly one output row per group across every spec in the same call).
  // Ties are broken by returning every tied value, sorted ascending, one
  // row per tied value; a group with zero observed values still produces
  // exactly one row, with a null value, matching every other terminal's
  // one-row-per-group guarantee.
  DataFrame mode_ties(std::string_view feature, std::string_view output = "") const;

private:
  DataFrame keyed_frame_(std::vector<std::string>& key_names) const;
  const DataFrame& keyed_frame_cached_() const;
  const std::vector<std::string>& key_names_cached_() const;
  const detail::GroupStateCache& state_cached_() const;
  DataFrame terminal_(DataFrame::GroupOp op) const;
  DataFrame terminal_(DataFrame::GroupOp op,
                      pluss::msharp::RangeView::Of<std::string_view> features) const;
  std::vector<DataFrame::GroupSpec> value_specs_(DataFrame::GroupOp op) const;
  std::vector<DataFrame::GroupSpec> selected_value_specs_(
      DataFrame::GroupOp op,
      pluss::msharp::RangeView::Of<std::string_view> features) const;

  DataFrame frame_{};
  std::vector<std::string> keys_{};
  std::optional<DataFrame::LabelGroup> labels_{};
  DataFrame::GroupOptions options_{};
  mutable bool keyed_cache_ready_{false};
  mutable DataFrame keyed_frame_cache_{};
  mutable std::vector<std::string> key_names_cache_{};
  mutable std::shared_ptr<const detail::GroupStateCache> state_cache_{};
};

class DataFrameSortBuilder {
public:
  explicit DataFrameSortBuilder(DataFrame frame, bool ascending = true, bool nulls_last = true);

  DataFrameSortBuilder by(std::string_view feature) const;
  DataFrameSortBuilder by(std::initializer_list<std::string_view> features) const;
  DataFrameSortBuilder by(pluss::msharp::RangeView::Of<std::string_view> features) const;
  DataFrameSortBuilder by(pluss::msharp::RangeView::Of<DataFrame::ArrangeSpec> specs) const;

  template <detail::StringViewNameRange NameRange>
  DataFrameSortBuilder by(const NameRange& features) const;

  DataFrameSortBuilder then(std::string_view feature) const;
  DataFrameSortBuilder then(std::initializer_list<std::string_view> features) const;
  DataFrameSortBuilder then(pluss::msharp::RangeView::Of<std::string_view> features) const;
  DataFrameSortBuilder then(pluss::msharp::RangeView::Of<DataFrame::ArrangeSpec> specs) const;

  template <detail::StringViewNameRange NameRange>
  DataFrameSortBuilder then(const NameRange& features) const;

  DataFrameSortBuilder within(std::string_view feature) const;
  DataFrameSortBuilder within(std::initializer_list<std::string_view> features) const;
  DataFrameSortBuilder within(pluss::msharp::RangeView::Of<std::string_view> features) const;

  template <detail::StringViewNameRange NameRange>
  DataFrameSortBuilder within(const NameRange& features) const;

  DataFrameSortBuilder asc() const;
  DataFrameSortBuilder desc() const;
  DataFrameSortBuilder nulls_first() const;
  DataFrameSortBuilder nulls_last() const;

  Thread order() const;
  DataFrame yield() const;
  DataFrame head(std::int64_t n) const;
  DataFrame tail(std::int64_t n) const;

private:
  DataFrameSortBuilder with_specs_(std::vector<DataFrame::ArrangeSpec> specs,
                                   std::optional<std::size_t> active_spec = std::nullopt) const;
  DataFrameSortBuilder mutate_active_(bool ascending, bool nulls_last) const;
  std::vector<std::int64_t> ordered_indices_(std::string_view where) const;
  std::vector<std::int64_t> ranked_indices_(std::int64_t n, bool tail, std::string_view where) const;

  DataFrame frame_{};
  bool ascending_{true};
  bool nulls_last_{true};
  std::vector<DataFrame::ArrangeSpec> specs_{};
  std::vector<std::string> within_{};
  std::optional<std::size_t> active_spec_{};
  mutable std::shared_ptr<const std::vector<std::int64_t>> order_cache_{};
  mutable std::optional<std::int64_t> head_cache_n_{};
  mutable std::vector<std::int64_t> head_cache_{};
  mutable std::optional<std::int64_t> tail_cache_n_{};
  mutable std::vector<std::int64_t> tail_cache_{};
};

class ContingencyTable {
public:
  struct SliceSpec {
    std::string dim{};
    ThreadValue::HostAny value{};

    SliceSpec(std::string_view dim_name, std::string_view level);
    SliceSpec(std::string_view dim_name, const char* level);
    SliceSpec(std::string_view dim_name, bool level);
    SliceSpec(std::string_view dim_name, std::int64_t level);
    SliceSpec(std::string_view dim_name, double level);
  };

  ContingencyTable();
  std::int64_t ndim() const;
  std::vector<std::int64_t> shape() const;
  std::vector<std::string> dims() const;

  ContingencyTable freq() const;
  ContingencyTable share() const;
  ContingencyTable slice(std::string_view dim, std::string_view value) const;
  ContingencyTable slice(std::string_view dim, const char* value) const;
  ContingencyTable slice(std::string_view dim, bool value) const;
  ContingencyTable slice(std::string_view dim, std::int64_t value) const;
  ContingencyTable slice(std::string_view dim, double value) const;
  ContingencyTable slice(std::initializer_list<SliceSpec> specs) const;
  ContingencyTable marginalize(std::string_view dim) const;
  ContingencyTable marginalize(std::initializer_list<std::string_view> dims) const;
  ContingencyTable marginalize(pluss::msharp::RangeView::Of<std::string_view> dims) const;
  ContingencyTable condition(std::string_view dim) const;
  ContingencyTable condition(std::initializer_list<std::string_view> dims) const;
  ContingencyTable condition(pluss::msharp::RangeView::Of<std::string_view> dims) const;

  pluss::Series to_array() const;
  DataFrame to_frame() const;
  DataFrame to_frame(const DataFrame::ContingencyOptions& options) const;
  DataFrame to_frame(std::initializer_list<std::string_view> row_dims,
                     std::initializer_list<std::string_view> column_dims) const;
  DataFrame to_frame(std::initializer_list<std::string_view> row_dims,
                     std::initializer_list<std::string_view> column_dims,
                     const DataFrame::ContingencyOptions& options) const;
  DataFrame to_frame(pluss::msharp::RangeView::Of<std::string_view> row_dims,
                     pluss::msharp::RangeView::Of<std::string_view> column_dims) const;
  DataFrame to_frame(pluss::msharp::RangeView::Of<std::string_view> row_dims,
                     pluss::msharp::RangeView::Of<std::string_view> column_dims,
                     const DataFrame::ContingencyOptions& options) const;
  Atlas facet() const;
  Atlas facet(const DataFrame::ContingencyOptions& options) const;
  Atlas facet(std::initializer_list<std::string_view> row_dims,
              std::initializer_list<std::string_view> column_dims) const;
  Atlas facet(std::initializer_list<std::string_view> row_dims,
              std::initializer_list<std::string_view> column_dims,
              const DataFrame::ContingencyOptions& options) const;
  Atlas facet(pluss::msharp::RangeView::Of<std::string_view> row_dims,
              pluss::msharp::RangeView::Of<std::string_view> column_dims) const;
  Atlas facet(pluss::msharp::RangeView::Of<std::string_view> row_dims,
              pluss::msharp::RangeView::Of<std::string_view> column_dims,
              const DataFrame::ContingencyOptions& options) const;

private:
  enum class Normalize : std::uint8_t {
    None = 0,
    All = 1,
    Conditional = 2,
  };

  ContingencyTable(std::vector<std::string> dims,
                   std::vector<Thread> levels,
                   pluss::Series counts,
                   DataFrame::ContingencyOptions options,
                   std::vector<std::string> default_row_dims,
                   std::vector<std::string> default_column_dims,
                   Normalize normalize = Normalize::None,
                   std::vector<std::string> conditioned_dims = {});

  std::size_t dim_index_(std::string_view dim, std::string_view where) const;
  std::vector<std::size_t> dim_indices_(pluss::msharp::RangeView::Of<std::string_view> dims,
                                        std::string_view where) const;
  std::vector<std::string> default_row_dims_() const;
  std::vector<std::string> default_column_dims_() const;
  std::vector<std::string> render_column_dims_(pluss::msharp::RangeView::Of<std::string_view> row_dims,
                                               pluss::msharp::RangeView::Of<std::string_view> column_dims) const;
  std::int64_t level_index_(std::size_t dim_index,
                            const ThreadValue::HostAny& value,
                            std::string_view where) const;
  ContingencyTable slice_at_(std::size_t dim_index, std::int64_t level_index) const;
  pluss::Series materialized_values_() const;
  std::vector<std::string> conditioned_dims_after_drop_(std::size_t dim_index) const;

  std::vector<std::string> dims_{};
  std::vector<Thread> levels_{};
  pluss::Series counts_{};
  DataFrame::ContingencyOptions options_{};
  std::vector<std::string> default_row_dims_hint_{};
  std::vector<std::string> default_column_dims_hint_{};
  Normalize normalize_{Normalize::None};
  std::vector<std::string> conditioned_dims_{};

  friend class DataFrame;
};

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

class DataFrameNoteAccessor {
 public:
  DataFrameNoteAccessor(DataFrame frame, std::vector<std::string> features);

  DataFrame strip() const;
  DataFrame strip(std::string_view chars) const;
  DataFrame lstrip() const;
  DataFrame lstrip(std::string_view chars) const;
  DataFrame rstrip() const;
  DataFrame rstrip(std::string_view chars) const;
  DataFrame lower() const;
  DataFrame upper() const;
  DataFrame casefold() const;
  DataFrame titlecase() const;
  DataFrame length() const;
  DataFrame starts(std::string_view prefix) const;
  DataFrame ends(std::string_view suffix) const;
  DataFrame has(std::string_view needle,
                const TextOptions& options = {}) const;
  DataFrame count(std::string_view pattern) const;
  DataFrame count(std::string_view pattern,
                  const TextOptions& options) const;
  DataFrame matches(std::string_view pattern, bool full = false) const;
  DataFrame fullmatch(std::string_view pattern) const;
  DataFrame findall(std::string_view pattern) const;
  DataFrame findall(std::string_view pattern,
                    const TextOptions& options) const;
  DataFrame extract(std::string_view pattern,
                    pluss::msharp::RangeView::Of<std::string_view> names = {}) const;
  DataFrame split(std::string_view delimiter,
                  std::int64_t limit = -1,
                  std::string prefix = "part") const;
  DataFrame split(std::string_view pattern,
                  const TextOptions& options) const;
  DataFrame partition(std::string_view separator,
                      std::string prefix = "part") const;
  DataFrame pad(std::int64_t width,
                TextAlign align = TextAlign::Left,
                char fill = ' ') const;
  DataFrame chars(std::int64_t start,
                  std::optional<std::int64_t> stop = std::nullopt,
                  std::int64_t step = 1) const;
  DataFrame normalize(UnicodeForm form = UnicodeForm::NFC) const;
  DataFrame encode(std::string_view encoding = "base64") const;
  DataFrame indicate(std::string_view delimiter = "|",
                     std::string prefix = "") const;
  DataFrame replace(std::string_view from, std::string_view to) const;
  DataFrame replace(std::string_view from,
                    std::string_view to,
                    const TextOptions& options) const;

 private:
  DataFrame frame_{};
  std::vector<std::string> features_{};
};

class DataFrameTimelineAccessor {
 public:
  DataFrameTimelineAccessor(DataFrame frame, std::vector<std::string> features);

  DataFrame isnat() const;
  DataFrame valid() const;
  DataFrame year() const;
  DataFrame month() const;
  DataFrame day() const;
  DataFrame hour() const;
  DataFrame minute() const;
  DataFrame second() const;
  DataFrame weekday() const;
  DataFrame yearday() const;
  DataFrame quarter() const;
  DataFrame week() const;
  DataFrame weekend() const;
  DataFrame month_start() const;
  DataFrame month_end() const;
  DataFrame leap() const;
  DataFrame weekday_name() const;
  DataFrame month_name() const;
  DataFrame fmt(std::string_view pattern) const;
  DataFrame iso() const;
  DataFrame epoch(TimeUnit unit = TimeUnit::Nanos) const;
  DataFrame span(const Thread& other, const TimelineSpanOptions& options = {}) const;
  DataFrame span(const Thread& other, TimeUnit unit) const;
  DataFrame span(const pluss::Timeline& other, const TimelineSpanOptions& options = {}) const;
  DataFrame span(const pluss::Timeline& other, TimeUnit unit) const;
  DataFrame span(const DataFrame& other, const TimelineSpanOptions& options = {}) const;
  DataFrame span(const DataFrame& other, TimeUnit unit) const;
  DataFrame floor(std::string_view unit) const;
  DataFrame ceil(std::string_view unit) const;
  DataFrame round(std::string_view unit) const;
  DataFrame date() const;
  DataFrame time() const;
  DataFrame shift(std::int64_t periods, std::string_view freq) const;
  DataFrame period(std::string_view freq) const;
  DataFrame tz(std::string_view name) const;
  DataFrame localize(const TimelineLocalizeOptions& options = {}) const;
  DataFrame utc() const;
  DataFrame index(bool require_sorted = true) const;

 private:
  DataFrame map_(const std::function<Thread(const Thread&)>& fn, std::string_view op) const;

  DataFrame frame_{};
  std::vector<std::string> features_{};
};

class DataFrameTsAccessor {
 public:
  DataFrameTsAccessor(DataFrame frame, std::vector<std::string> features, TsOptions options = {});

  DataFrame returns(std::int64_t periods = 1) const;
  DataFrame log_returns(std::int64_t periods = 1) const;
  DataFrame diff(std::int64_t periods = 1) const;
  DataFrame lag(std::int64_t periods = 1) const;
  DataFrame lead(std::int64_t periods = 1) const;
  DataFrame rolling(std::int64_t window,
                    WindowStatistic statistic = WindowStatistic::Mean,
                    const WindowOptions& options = {}) const;
  DataFrame rolling(std::string_view duration,
                    WindowStatistic statistic = WindowStatistic::Mean,
                    const StatisticOptions& options = {},
                    WindowClosure closure = WindowClosure::Right) const;
  DataFrame grouped(std::int64_t window,
                    WindowStatistic statistic = WindowStatistic::Mean,
                    const WindowOptions& options = {}) const;
  DataFrame grouped(std::string_view group_feature,
                    std::int64_t window,
                    WindowStatistic statistic = WindowStatistic::Mean,
                    const WindowOptions& options = {}) const;
  DataFrame grouped(const Thread& groups,
                    std::int64_t window,
                    WindowStatistic statistic = WindowStatistic::Mean,
                    const WindowOptions& options = {}) const;
  DataFrameTsWindowAccessor window(std::string_view duration,
                                   WindowClosure closure = WindowClosure::Right) const;
  DataFrame weigh(pluss::msharp::RangeView::Double weights,
                  WindowStatistic statistic = WindowStatistic::Mean,
                  std::int64_t min_count = 1) const;
  DataFrame bounded(const WindowBoundsFn& bounds,
                    WindowStatistic statistic = WindowStatistic::Mean,
                    const WindowOptions& options = {}) const;
  DataFrame window(pluss::msharp::RangeView::Of<DataFrame::WindowSpec> specs,
                   bool keep_input = true) const;
  DataFrame rolling(pluss::msharp::RangeView::Of<DataFrame::WindowSpec> specs,
                    bool keep_input = true) const;
  DataFrame expanding(WindowStatistic statistic = WindowStatistic::Mean,
                      const StatisticOptions& options = {}) const;
  DataFrame ewm(double alpha,
                WindowStatistic statistic = WindowStatistic::Mean,
                bool adjusted = true,
                bool bias_corrected = true,
                std::int64_t min_count = 1,
                bool ignore_nulls = false) const;
  DataFrame timed(std::int64_t duration_nanos,
                  TimeWindowStatistic statistic = TimeWindowStatistic::Mean,
                  const StatisticOptions& options = {},
                  WindowClosure closure = WindowClosure::Right) const;
  DataFrame timed(const Thread& axis,
                  std::int64_t duration_nanos,
                  TimeWindowStatistic statistic = TimeWindowStatistic::Mean,
                  const StatisticOptions& options = {},
                  WindowClosure closure = WindowClosure::Right) const;
  DataFrame zscore_rolling(std::int64_t window, std::int64_t min_periods = 1, double epsilon = 1e-12) const;
  DataFrame fill_gaps(pluss::subsp::stats::ts::GapFillMode mode = pluss::subsp::stats::ts::GapFillMode::Forward,
                      std::optional<double> constant = std::nullopt) const;
  DataFrame sort(bool ascending = true, bool nulls_last = true) const;

  DataFrameTsResampler resample(std::string_view time_feature,
                                std::string_view freq,
                                std::int64_t step = 1) const;
  DataFrameTsResampler resample(std::string_view time_feature,
                                const pluss::TimeIndex::ResampleOptions& options) const;
  DataFrameTsResampler resample(std::string_view freq,
                                std::int64_t step = 1) const;
  DataFrameTsResampler resample(const pluss::TimeIndex::ResampleOptions& options) const;
  DataFrame resample(std::string_view time_feature,
                     std::string_view freq,
                     pluss::subsp::stats::ts::ResampleOp op,
                     std::int64_t step = 1) const;
  DataFrame resample(std::string_view time_feature,
                     const pluss::TimeIndex::ResampleOptions& options,
                     pluss::subsp::stats::ts::ResampleOp op) const;
  DataFrame resample(std::string_view freq,
                     pluss::subsp::stats::ts::ResampleOp op,
                     std::int64_t step = 1) const;
  DataFrame resample(const pluss::TimeIndex::ResampleOptions& options,
                     pluss::subsp::stats::ts::ResampleOp op) const;
  DataFrame asfreq(std::string_view time_feature,
                   std::string_view freq,
                   std::int64_t step = 1,
                   pluss::subsp::stats::ts::GapFillMode fill_mode = pluss::subsp::stats::ts::GapFillMode::Forward,
                   std::optional<double> constant = std::nullopt,
                   pluss::subsp::stats::ts::ResampleOp op = pluss::subsp::stats::ts::ResampleOp::Last) const;
  DataFrame asfreq(std::string_view time_feature,
                   const pluss::TimeIndex::ResampleOptions& options,
                   pluss::subsp::stats::ts::GapFillMode fill_mode = pluss::subsp::stats::ts::GapFillMode::Forward,
                   std::optional<double> constant = std::nullopt,
                   pluss::subsp::stats::ts::ResampleOp op = pluss::subsp::stats::ts::ResampleOp::Last) const;
  DataFrame asfreq(std::string_view freq,
                   std::int64_t step = 1,
                   pluss::subsp::stats::ts::GapFillMode fill_mode = pluss::subsp::stats::ts::GapFillMode::Forward,
                   std::optional<double> constant = std::nullopt,
                   pluss::subsp::stats::ts::ResampleOp op = pluss::subsp::stats::ts::ResampleOp::Last) const;
  DataFrame asfreq(const pluss::TimeIndex::ResampleOptions& options,
                   pluss::subsp::stats::ts::GapFillMode fill_mode = pluss::subsp::stats::ts::GapFillMode::Forward,
                   std::optional<double> constant = std::nullopt,
                   pluss::subsp::stats::ts::ResampleOp op = pluss::subsp::stats::ts::ResampleOp::Last) const;
  DataFrame insert_gaps(std::string_view time_feature,
                        std::string_view freq,
                        std::int64_t step = 1) const;
  DataFrame insert_gaps(std::string_view freq,
                        std::int64_t step = 1) const;
  DataFrame align_to(std::string_view time_feature,
                     const Thread& target_axis,
                     TsAlignMode mode = TsAlignMode::Exact) const;
  DataFrame align_to(std::string_view time_feature,
                     const Thread& target_axis,
                     TsAlignMode mode,
                     TsReindexFill fill,
                     std::optional<double> constant = std::nullopt) const;
  DataFrame align_to(const Thread& target_axis,
                     TsAlignMode mode = TsAlignMode::Exact) const;
  DataFrame align_to(const Thread& target_axis,
                     TsAlignMode mode,
                     TsReindexFill fill,
                     std::optional<double> constant = std::nullopt) const;
  DataFrame add(const DataFrame& other, TsAlignMode mode = TsAlignMode::Exact) const;
  DataFrame sub(const DataFrame& other, TsAlignMode mode = TsAlignMode::Exact) const;
  DataFrame mul(const DataFrame& other, TsAlignMode mode = TsAlignMode::Exact) const;
  DataFrame div(const DataFrame& other, TsAlignMode mode = TsAlignMode::Exact) const;
  DataFrame eq(const DataFrame& other, TsAlignMode mode = TsAlignMode::Exact) const;
  DataFrame ne(const DataFrame& other, TsAlignMode mode = TsAlignMode::Exact) const;
  DataFrame lt(const DataFrame& other, TsAlignMode mode = TsAlignMode::Exact) const;
  DataFrame le(const DataFrame& other, TsAlignMode mode = TsAlignMode::Exact) const;
  DataFrame gt(const DataFrame& other, TsAlignMode mode = TsAlignMode::Exact) const;
  DataFrame ge(const DataFrame& other, TsAlignMode mode = TsAlignMode::Exact) const;

  DataFrame calendar() const;
  DataFrame calendar(std::string_view time_feature) const;
  DataFrame lag_block(pluss::msharp::RangeView::Int lags) const;
  DataFrame rolling_block(pluss::msharp::RangeView::Int windows,
                          const TsRollingBlockOptions& options = {}) const;

 private:
  DataFrame frame_{};
  std::vector<std::string> features_{};
  TsOptions options_{};
};

class DataFrameTsWindowAccessor {
 public:
  DataFrameTsWindowAccessor(DataFrame frame,
                            std::vector<std::string> features,
                            TsOptions options,
                            std::string duration,
                            WindowClosure closure);

  DataFrame sum(std::string_view value_feature = {}) const;
  DataFrame mean(std::string_view value_feature = {}) const;
  DataFrame min(std::string_view value_feature = {}) const;
  DataFrame max(std::string_view value_feature = {}) const;
  DataFrame count(std::string_view value_feature = {}) const;

 private:
  DataFrame frame_{};
  std::vector<std::string> features_{};
  TsOptions options_{};
  std::string duration_{};
  WindowClosure closure_{WindowClosure::Right};
};

class DataFrameTsResampler {
 public:
  DataFrameTsResampler(DataFrame frame,
                       std::vector<std::string> features,
                       std::string time_feature,
                       std::string freq,
                       std::int64_t step,
                       TsOptions options = {});
  DataFrameTsResampler(DataFrame frame,
                       std::vector<std::string> features,
                       std::string time_feature,
                       pluss::TimeIndex::ResampleOptions resample_options,
                       TsOptions options = {});

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
  pluss::TimeIndex::ResampleOptions options_{};
  TsOptions ts_options_{};
};

template <detail::StringViewNameRange NameRange>
inline DataFrameView DataFrame::tally(const NameRange& features,
                                      std::int64_t top_n,
                                      bool dropnull,
                                      bool render) const {
  return detail::with_stack_name_view(features, [&](auto names) {
    return tally(names, top_n, dropnull, render);
  });
}

template <detail::StringViewNameRange NameRange>
inline DataFrameGrouped DataFrameGroupBuilder::by(const NameRange& keys) const {
  return detail::with_stack_name_view(keys, [&](auto values) {
    return by(values);
  });
}

template <detail::StringViewNameRange NameRange>
inline DataFrameSortBuilder DataFrameSortBuilder::by(const NameRange& features) const {
  return detail::with_stack_name_view(features, [&](auto names) {
    return by(names);
  });
}

template <detail::StringViewNameRange NameRange>
inline DataFrameSortBuilder DataFrameSortBuilder::then(const NameRange& features) const {
  return detail::with_stack_name_view(features, [&](auto names) {
    return then(names);
  });
}

template <detail::StringViewNameRange NameRange>
inline DataFrameSortBuilder DataFrameSortBuilder::within(const NameRange& features) const {
  return detail::with_stack_name_view(features, [&](auto names) {
    return within(names);
  });
}

inline ContingencyTable DataFrame::contingency(
    std::initializer_list<std::string_view> features) const {
  return contingency(
      pluss::msharp::RangeView::Of<std::string_view>(features));
}

inline ContingencyTable DataFrame::contingency(
    std::initializer_list<std::string_view> features,
    const ContingencyOptions& options) const {
  return contingency(
      pluss::msharp::RangeView::Of<std::string_view>(features),
      options);
}

inline ContingencyTable DataFrame::contingency(
    std::initializer_list<std::string_view> row_features,
    std::initializer_list<std::string_view> column_features) const {
  return contingency(
      pluss::msharp::RangeView::Of<std::string_view>(row_features),
      pluss::msharp::RangeView::Of<std::string_view>(column_features));
}

inline ContingencyTable DataFrame::contingency(
    std::initializer_list<std::string_view> row_features,
    std::initializer_list<std::string_view> column_features,
    const ContingencyOptions& options) const {
  return contingency(
      pluss::msharp::RangeView::Of<std::string_view>(row_features),
      pluss::msharp::RangeView::Of<std::string_view>(column_features),
      options);
}

template <detail::StringViewNameRange NameRange>
inline ContingencyTable DataFrame::contingency(const NameRange& features) const {
  return detail::with_stack_name_view(features, [&](auto names) {
    return contingency(names);
  });
}

template <detail::StringViewNameRange NameRange>
inline ContingencyTable DataFrame::contingency(const NameRange& features,
                                               const ContingencyOptions& options) const {
  return detail::with_stack_name_view(features, [&](auto names) {
    return contingency(names, options);
  });
}

template <detail::StringViewNameRange RowRange, detail::StringViewNameRange ColumnRange>
inline ContingencyTable DataFrame::contingency(const RowRange& row_features,
                                               const ColumnRange& column_features) const {
  return detail::with_stack_name_views(row_features, column_features, [&](auto rows, auto columns) {
    return contingency(rows, columns);
  });
}

template <detail::StringViewNameRange RowRange, detail::StringViewNameRange ColumnRange>
inline ContingencyTable DataFrame::contingency(const RowRange& row_features,
                                               const ColumnRange& column_features,
                                               const ContingencyOptions& options) const {
  return detail::with_stack_name_views(row_features, column_features, [&](auto rows, auto columns) {
    return contingency(rows, columns, options);
  });
}

template <detail::StringViewNameRange IndexRange,
          detail::StringViewNameRange ColumnRange,
          detail::StringViewNameRange ValueRange>
inline DataFrameView DataFrame::pivot_table(const IndexRange& index_features,
                                            const ColumnRange& column_features,
                                            const ValueRange& value_features,
                                            GroupOp op,
                                            bool dropna,
                                            bool render,
                                            bool sort) const {
  return detail::with_stack_name_views(index_features, column_features, [&](auto rows, auto columns) {
    return detail::with_stack_name_view(value_features, [&](auto values) {
      return pivot_table(rows, columns, values, op, dropna, render, sort);
    });
  });
}

template <detail::StringViewNameRange NameRange>
inline DataFrame DataFrameGrouped::count(const NameRange& features) const {
  return detail::with_stack_name_view(features, [&](auto names) {
    return count(names);
  });
}

template <detail::StringViewNameRange NameRange>
inline DataFrame DataFrameGrouped::ndistinct(const NameRange& features) const {
  return detail::with_stack_name_view(features, [&](auto names) {
    return ndistinct(names);
  });
}

template <detail::StringViewNameRange NameRange>
inline DataFrame DataFrameGrouped::sum(const NameRange& features) const {
  return detail::with_stack_name_view(features, [&](auto names) {
    return sum(names);
  });
}

template <detail::StringViewNameRange NameRange>
inline DataFrame DataFrameGrouped::mean(const NameRange& features) const {
  return detail::with_stack_name_view(features, [&](auto names) {
    return mean(names);
  });
}

template <detail::StringViewNameRange NameRange>
inline DataFrame DataFrameGrouped::median(const NameRange& features) const {
  return detail::with_stack_name_view(features, [&](auto names) {
    return median(names);
  });
}

template <detail::StringViewNameRange NameRange>
inline DataFrame DataFrameGrouped::min(const NameRange& features) const {
  return detail::with_stack_name_view(features, [&](auto names) {
    return min(names);
  });
}

template <detail::StringViewNameRange NameRange>
inline DataFrame DataFrameGrouped::max(const NameRange& features) const {
  return detail::with_stack_name_view(features, [&](auto names) {
    return max(names);
  });
}

template <detail::StringViewNameRange NameRange>
inline DataFrame DataFrameGrouped::std(const NameRange& features) const {
  return detail::with_stack_name_view(features, [&](auto names) {
    return std(names);
  });
}

template <detail::StringViewNameRange NameRange>
inline DataFrame DataFrameGrouped::var(const NameRange& features) const {
  return detail::with_stack_name_view(features, [&](auto names) {
    return var(names);
  });
}

template <detail::StringViewNameRange NameRange>
inline DataFrame DataFrameGrouped::mode(const NameRange& features) const {
  return detail::with_stack_name_view(features, [&](auto names) {
    return mode(names);
  });
}

DataFrame operator+(const DataFrame& lhs, double rhs);
DataFrame operator+(double lhs, const DataFrame& rhs);
DataFrame operator+(const DataFrame& lhs, const DataFrame& rhs);
DataFrame operator+(const DataFrame& lhs, const Thread& rhs);
DataFrame operator+(const Thread& lhs, const DataFrame& rhs);
DataFrame operator-(const DataFrame& lhs, double rhs);
DataFrame operator-(double lhs, const DataFrame& rhs);
DataFrame operator-(const DataFrame& lhs, const DataFrame& rhs);
DataFrame operator-(const DataFrame& lhs, const Thread& rhs);
DataFrame operator-(const Thread& lhs, const DataFrame& rhs);
DataFrame operator*(const DataFrame& lhs, double rhs);
DataFrame operator*(double lhs, const DataFrame& rhs);
DataFrame operator*(const DataFrame& lhs, const DataFrame& rhs);
DataFrame operator*(const DataFrame& lhs, const Thread& rhs);
DataFrame operator*(const Thread& lhs, const DataFrame& rhs);
DataFrame operator/(const DataFrame& lhs, double rhs);
DataFrame operator/(double lhs, const DataFrame& rhs);
DataFrame operator/(const DataFrame& lhs, const DataFrame& rhs);
DataFrame operator/(const DataFrame& lhs, const Thread& rhs);
DataFrame operator/(const Thread& lhs, const DataFrame& rhs);
DataFrame operator%(const DataFrame& lhs, double rhs);
DataFrame operator%(double lhs, const DataFrame& rhs);
DataFrame operator%(const DataFrame& lhs, const DataFrame& rhs);
DataFrame operator%(const DataFrame& lhs, const Thread& rhs);
DataFrame operator%(const Thread& lhs, const DataFrame& rhs);
DataFrame operator==(const DataFrame& lhs, double rhs);
DataFrame operator==(double lhs, const DataFrame& rhs);
DataFrame operator==(const DataFrame& lhs, std::string_view rhs);
DataFrame operator==(std::string_view lhs, const DataFrame& rhs);
DataFrame operator==(const DataFrame& lhs, const char* rhs);
DataFrame operator==(const char* lhs, const DataFrame& rhs);
DataFrame operator==(const DataFrame& lhs, const DataFrame& rhs);
DataFrame operator==(const DataFrame& lhs, const Thread& rhs);
DataFrame operator==(const Thread& lhs, const DataFrame& rhs);
DataFrame operator!=(const DataFrame& lhs, double rhs);
DataFrame operator!=(double lhs, const DataFrame& rhs);
DataFrame operator!=(const DataFrame& lhs, std::string_view rhs);
DataFrame operator!=(std::string_view lhs, const DataFrame& rhs);
DataFrame operator!=(const DataFrame& lhs, const char* rhs);
DataFrame operator!=(const char* lhs, const DataFrame& rhs);
DataFrame operator!=(const DataFrame& lhs, const DataFrame& rhs);
DataFrame operator!=(const DataFrame& lhs, const Thread& rhs);
DataFrame operator!=(const Thread& lhs, const DataFrame& rhs);
DataFrame operator<(const DataFrame& lhs, double rhs);
DataFrame operator<(double lhs, const DataFrame& rhs);
DataFrame operator<(const DataFrame& lhs, std::string_view rhs);
DataFrame operator<(std::string_view lhs, const DataFrame& rhs);
DataFrame operator<(const DataFrame& lhs, const char* rhs);
DataFrame operator<(const char* lhs, const DataFrame& rhs);
DataFrame operator<(const DataFrame& lhs, const DataFrame& rhs);
DataFrame operator<(const DataFrame& lhs, const Thread& rhs);
DataFrame operator<(const Thread& lhs, const DataFrame& rhs);
DataFrame operator<=(const DataFrame& lhs, double rhs);
DataFrame operator<=(double lhs, const DataFrame& rhs);
DataFrame operator<=(const DataFrame& lhs, std::string_view rhs);
DataFrame operator<=(std::string_view lhs, const DataFrame& rhs);
DataFrame operator<=(const DataFrame& lhs, const char* rhs);
DataFrame operator<=(const char* lhs, const DataFrame& rhs);
DataFrame operator<=(const DataFrame& lhs, const DataFrame& rhs);
DataFrame operator<=(const DataFrame& lhs, const Thread& rhs);
DataFrame operator<=(const Thread& lhs, const DataFrame& rhs);
DataFrame operator>(const DataFrame& lhs, double rhs);
DataFrame operator>(double lhs, const DataFrame& rhs);
DataFrame operator>(const DataFrame& lhs, std::string_view rhs);
DataFrame operator>(std::string_view lhs, const DataFrame& rhs);
DataFrame operator>(const DataFrame& lhs, const char* rhs);
DataFrame operator>(const char* lhs, const DataFrame& rhs);
DataFrame operator>(const DataFrame& lhs, const DataFrame& rhs);
DataFrame operator>(const DataFrame& lhs, const Thread& rhs);
DataFrame operator>(const Thread& lhs, const DataFrame& rhs);
DataFrame operator>=(const DataFrame& lhs, double rhs);
DataFrame operator>=(double lhs, const DataFrame& rhs);
DataFrame operator>=(const DataFrame& lhs, std::string_view rhs);
DataFrame operator>=(std::string_view lhs, const DataFrame& rhs);
DataFrame operator>=(const DataFrame& lhs, const char* rhs);
DataFrame operator>=(const char* lhs, const DataFrame& rhs);
DataFrame operator>=(const DataFrame& lhs, const DataFrame& rhs);
DataFrame operator>=(const DataFrame& lhs, const Thread& rhs);
DataFrame operator>=(const Thread& lhs, const DataFrame& rhs);

} // namespace pluss::table

#include "table/plot/DataFramePlot.h"
#include "table/sql/DataFrameSql.h"
