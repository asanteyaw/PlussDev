#pragma once

#include <cstdint>
#include <functional>
#include <iosfwd>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "MSharp/IntrusivePtr.h"
#include "MSharp/RangeView.h"
#include "table/display/Display.h"
#include "pluss/null.h"
#include "pluss/codem/codem.h"
#include "pluss/coden/coden.h"
#include "pluss/codeh/codeh.h"
#include "pluss/subsp/stats/ts.h"

namespace pluss::kindle {
class Stream;
}
namespace pluss::scribble {
class Slot;
}

namespace pluss::table {

class Slate;
enum class LabelKind : std::uint8_t;
enum class SlateBinaryMathOp : std::uint8_t;
struct DataFramePlotOptions;

namespace detail {
class SlateImpl;
struct GroupStateCache;
bool has_validity_mask(const Slate& slate);
const std::optional<pluss::Series>& validity_mask(const Slate& slate);
Slate with_validity_mask(const Slate& slate, const pluss::Series& validity_mask);
Slate& with_validity_mask_(Slate& slate, const pluss::Series& validity_mask);
Slate clear_validity_mask(const Slate& slate);
Slate& clear_validity_mask_(Slate& slate);
bool has_display_labels(const Slate& slate);
Slate display_labels(const Slate& slate);
Slate with_display_labels(const Slate& slate, const Slate& labels);
Slate& with_display_labels_(Slate& slate, const Slate& labels);
Slate clear_display_labels(const Slate& slate);
Slate& clear_display_labels_(Slate& slate);
LabelKind label_kind(const Slate& slate);
Slate with_label_kind(const Slate& slate, LabelKind kind);
Slate& with_label_kind_(Slate& slate, LabelKind kind);
bool valid_at(const Slate& slate, std::int64_t row);
int compare_rows(const Slate& slate,
                 std::int64_t lhs_row,
                 std::int64_t rhs_row,
                 bool nulls_last = true);
bool same_payload(const Slate& left, const Slate& right) noexcept;
Slate math(const Slate& slate,
            double other,
            SlateBinaryMathOp op,
            bool reverse = false);
Slate math(const Slate& left, const Slate& right, SlateBinaryMathOp op);
Slate freq_count_slate(const Slate& slate,
                         std::int64_t top_n,
                         bool dropnull,
                         std::string_view display_name);
}
class SlateValue;
class DataFrame;
class DataFrameView;
class Atlas;
class SlateView;
class SlatePlotAccessor;
class SlateCastAccessor;
class SlateGroupBuilder;
class SlateGrouped;
class SlateSortBuilder;
class SlateNoteAccessor;
class SlateTimelineAccessor;
class SlateLabelAccessor;
class TimeSeriesAccessor;
class TimeSeriesResampler;

enum class SlateKind : std::uint8_t {
  Series = 0,
  Note = 1,
  Timeline = 2,
  Duration = 4,
};

std::string_view slate_kind_name(SlateKind kind) noexcept;

enum class LabelKind : std::uint8_t {
  Default = 0,
  Case = 1,
  Feature = 2,
  Custom = 3,
  Time = 4,
};

enum class Axis : std::uint8_t {
  Feature = 0,
  Case = 1,
};

enum class AlignBy : std::uint8_t {
  Pos = 0,
  Caselbl = 1,
  Feature = 2,
  Both = 3,
};

struct AlignOptions {
  AlignBy align{AlignBy::Pos};
  std::optional<double> fill{std::nullopt};
};

enum class TsAlignMode : std::uint8_t {
  Exact = 0,
  Backward = 1,
  Forward = 2,
  Nearest = 3,
};

struct SlateOptions {
  bool rangelbl{false};
};

enum class SlateBinaryMathOp : std::uint8_t {
  Add = 0,
  Sub = 1,
  Mul = 2,
  Div = 3,
  TrueDiv = 4,
  FloorDiv = 5,
  Mod = 6,
  Pow = 7,
};

enum class SlateBinaryCompareOp : std::uint8_t {
  Eq = 0,
  Ne = 1,
  Lt = 2,
  Le = 3,
  Gt = 4,
  Ge = 5,
};

enum class SlateCastTarget : std::uint8_t {
  Series = 0,
  Note = 1,
  Timeline = 2,
  Duration = 4,
};

enum class ParseInvalid : std::uint8_t {
  Raise = 0,
  Null = 1,
};

struct SlateCastOptions {
  std::optional<pluss::codem::ScalarType> series_dtype{std::nullopt};
  std::string format{};
  ParseInvalid invalid{ParseInvalid::Raise};
};

enum class SlateWindowOp : std::uint8_t {
  RollingSum = 0,
  RollingMean = 1,
  RollingMin = 2,
  RollingMax = 3,
};

enum class SlateCorrMethod : std::uint8_t {
  Pearson = 0,
  Spearman = 1,
  KendallTau = 2,
};

enum class RankTieRule : std::uint8_t {
  Average = 0,
  Lowest = 1,
  Highest = 2,
  First = 3,
  Dense = 4,
};

enum class RankNullRule : std::uint8_t {
  Keep = 0,
  Top = 1,
  Bottom = 2,
};

struct StatisticOptions {
  std::int64_t ddof{1};
  std::int64_t min_count{1};
};

enum class WindowClosure : std::uint8_t {
  Right = 0,
  Left = 1,
  Both = 2,
  Neither = 3,
};

struct WindowOptions {
  std::int64_t min_count{1};
  std::int64_t ddof{1};
  bool centered{false};
  WindowClosure closure{WindowClosure::Right};
};

enum class WindowStatistic : std::uint8_t {
  Sum = 0,
  Mean = 1,
  Minimum = 2,
  Maximum = 3,
  Var = 4,
  Std = 5,
  Sem = 6,
  Skew = 7,
  Kurtosis = 8,
};

enum class TimeWindowStatistic : std::uint8_t {
  Sum = 0,
  Mean = 1,
  Var = 2,
  Std = 3,
};

enum class ScanStatistic : std::uint8_t {
  Sum = 0,
  Product = 1,
  Minimum = 2,
  Maximum = 3,
};

enum class ErrorTarget : std::uint8_t {
  Mean = 0,
  Median = 1,
};

enum class SlateFreqKind : std::uint8_t {
  Count = 0,
  Prop = 1,
};

enum class GroupOp : std::uint8_t {
  Count = 0,
  Ndistinct = 1,
  Sum = 2,
  Mean = 3,
  Median = 4,
  Min = 5,
  Max = 6,
  Std = 7,
  Var = 8,
  Mode = 9,
};

struct SlateFreqOptions {
  SlateFreqKind kind{SlateFreqKind::Count};
  bool dropnull{true};
  bool sort{true};
  bool ascending{false};
  std::int64_t top{0};
};

struct SlateGroupOptions {
  bool dropnull{true};
  bool sort{false};
};

struct SlateGroupSpec {
  std::string output{};
  GroupOp op{GroupOp::Count};
};

enum class UnicodeForm : std::uint8_t {
  NFC = 0,
  NFD = 1,
  NFKC = 2,
  NFKD = 3,
};

enum class TextAlign : std::uint8_t {
  Left = 0,
  Right = 1,
  Center = 2,
};

struct TextOptions {
  bool regex{false};
  bool case_sensitive{true};
  std::int64_t limit{-1};
  std::string prefix{"part"};
};

enum class ResultShape : std::uint8_t {
  Infer = 0,
  Reduce = 1,
  Preserve = 2,
  Expand = 3,
};

enum class TimeUnit : std::uint8_t {
  Nanos = 0,
  Micros = 1,
  Millis = 2,
  Seconds = 3,
  Days = 4,
  Weeks = 5,
  Months = 6,
  Years = 7,
  BusinessDays = 8,
};

using TimelineSpanMode = pluss::codeh::TimelineSpanMode;
using TimelineSpanClosed = pluss::codeh::TimelineSpanClosed;
using TimelineSpanAnchor = pluss::codeh::TimelineSpanAnchor;
using BusinessCalendar = pluss::codeh::BusinessCalendar;

struct TimelineSpanOptions {
  TimeUnit unit{TimeUnit::Days};
  TimelineSpanMode mode{TimelineSpanMode::Complete};
  TimelineSpanAnchor anchor{TimelineSpanAnchor::DayOfMonth};
  bool signed_result{true};
  BusinessCalendar calendar{};
  TimelineSpanClosed closed{TimelineSpanClosed::Right};
};

struct TimelineLocalizeOptions {
  std::string tz{"UTC"};
  int fold{0};
  std::string nonexistent{"raise"};
};

using WindowBoundsFn =
    std::function<std::pair<std::vector<std::int64_t>, std::vector<std::int64_t>>(
        std::int64_t size,
        const WindowOptions& options)>;

class Slate {
public:
  Slate();
  Slate(const pluss::Series& data, std::optional<pluss::Series> validity_mask = std::nullopt);
  Slate(const pluss::Series& data, const SlateOptions& options);
  Slate(const pluss::Series& data,
         std::optional<pluss::Series> validity_mask,
         const SlateOptions& options);
  Slate(const pluss::Note& data, std::optional<pluss::Series> validity_mask = std::nullopt);
  Slate(const pluss::Note& data, const SlateOptions& options);
  Slate(const pluss::Note& data,
         std::optional<pluss::Series> validity_mask,
         const SlateOptions& options);
  Slate(const pluss::Timeline& data, std::optional<pluss::Series> validity_mask = std::nullopt);
  Slate(const pluss::Timeline& data, const SlateOptions& options);
  Slate(const pluss::Timeline& data,
         std::optional<pluss::Series> validity_mask,
         const SlateOptions& options);
  Slate(const pluss::Duration& data, std::optional<pluss::Series> validity_mask = std::nullopt);
  Slate(const pluss::Duration& data, const SlateOptions& options);
  Slate(const pluss::Duration& data,
         std::optional<pluss::Series> validity_mask,
         const SlateOptions& options);
  Slate(const Slate& other);
  Slate(Slate&& other) noexcept;
  Slate& operator=(const Slate& other);
  Slate& operator=(Slate&& other) noexcept;
  ~Slate();

  bool defined() const noexcept;
  std::int64_t size() const;
  std::int64_t ncases() const;
  std::pair<std::int64_t, std::int64_t> shape() const;
  SlateKind kind() const;
  std::string ptype() const;
  std::string dtype() const;
  SlateValue value() const;

  // 1D counterparts to DataFrame::kindelize()/scribbelize() and
  // Atlas::kindelize()/scribbelize().
  pluss::kindle::Stream kindelize() const;
  pluss::scribble::Slot scribbelize() const;

  Slate isnull() const;
  Slate notnull() const;
  Slate isnan() const;
  Slate isfinite() const;
  Slate invalid() const;

  // Functional-by-default transforms.
  Slate fillnull(double value) const;
  Slate fillnull(std::string_view value) const;
  Slate fillnull(const char* value) const {
    return fillnull(std::string_view{value != nullptr ? value : ""});
  }
  Slate fillnull(const pluss::Timeline& value) const;
  Slate fillnull(const pluss::Duration& value) const;
  Slate abs() const;
  Slate clip(double min_value, double max_value) const;
  Slate round(std::int64_t decimals = 0) const;
  Slate log() const;
  Slate exp() const;

  // Explicit mutating transforms (copy-on-write when shared).
  Slate& fillnull_(double value);
  Slate& fillnull_(std::string_view value);
  Slate& fillnull_(const char* value) {
    return fillnull_(std::string_view{value != nullptr ? value : ""});
  }
  Slate& fillnull_(const pluss::Timeline& value);
  Slate& fillnull_(const pluss::Duration& value);

  // 1D row-space transforms.
  SlateView head(std::int64_t n = 5, bool render = false) const;
  SlateView tail(std::int64_t n = 5, bool render = false) const;
  SlateView view(std::int64_t start, std::int64_t stop, bool render = false) const;
  SlateView top(std::int64_t n, bool render = false) const;
  SlateView bottom(std::int64_t n, bool render = false) const;
  Slate append(const Slate& other) const;
  Slate sample(std::int64_t n) const;
  Slate sample(std::int64_t n, std::uint64_t random_state, bool replace = false) const;
  template <class Float>
    requires std::floating_point<Float>
  Slate sample(Float frac) const {
    return sample_fraction_(static_cast<double>(frac), std::nullopt, false);
  }
  template <class Float>
    requires std::floating_point<Float>
  Slate sample(Float frac, std::uint64_t random_state, bool replace = false) const {
    return sample_fraction_(static_cast<double>(frac), random_state, replace);
  }
  Slate shuffle(std::optional<std::uint64_t> random_state = std::nullopt) const;
  Slate shift(std::int64_t periods = 1) const;
  Slate slice(std::int64_t start, std::int64_t stop) const;
  Slate take(std::span<const std::int64_t> indices) const;
  Slate filter(const pluss::Series& bool_mask) const;
  Slate filter(const Slate& bool_mask) const;
  Slate dropnull() const;
  Slate arrange(bool ascending = true, bool nulls_last = true) const;
  SlateSortBuilder sort(bool ascending = true, bool nulls_last = true) const;
  Slate ffill(std::int64_t limit = -1) const;
  Slate bfill(std::int64_t limit = -1) const;
  Slate interp(std::int64_t limit = -1) const;
  Slate distinct(bool dropnull = true) const;
  Slate freq() const;
  Slate freq(SlateFreqOptions options) const;
  SlateView describe(bool render = false) const;
  Slate bucketize(pluss::msharp::RangeView::Double bins,
                   pluss::msharp::RangeView::Int labels = {},
                   bool right = true,
                   bool include_lowest = true) const;

  // Reshape this Slate into a one-column DataFrame -- the 1-D -> 2-D
  // counterpart of pandas `Series.to_frame`. The column takes `name`; when
  // `name` is empty it falls back to this Slate's own name, or "x" if it has
  // none. This Slate's display labels (its index) become the frame's cases.
  DataFrame framerize(std::string_view name = {}) const;

  SlateNoteAccessor nt() const;
  SlateTimelineAccessor tl() const;

  // Reductions return scalar payloads where the operation is valid.
  SlateValue sum() const;
  SlateValue prod() const;
  SlateValue count(bool count_true = true) const;
  SlateValue any(bool skipnull = true) const;
  SlateValue all(bool skipnull = true) const;
  SlateValue mean() const;
  SlateValue median() const;
  SlateValue min() const;
  SlateValue max() const;
  SlateValue csmin() const;
  SlateValue csmax() const;
  SlateValue var(bool unbiased = true) const;
  SlateValue var(const StatisticOptions& options) const;
  SlateValue std(bool unbiased = true) const;
  SlateValue std(const StatisticOptions& options) const;
  SlateValue se(ErrorTarget target = ErrorTarget::Mean,
                 const StatisticOptions& options = {}) const;
  double corr(const Slate& other, SlateCorrMethod method = SlateCorrMethod::Pearson) const;
  SlateValue covar(const Slate& other, const StatisticOptions& options = {}) const;
  Slate percentiles(pluss::msharp::RangeView::Double points = {0.25, 0.5, 0.75},
                     std::int64_t min_count = 1) const;
  Slate mode(bool drop_nulls = true) const;
  Slate rank(RankTieRule ties = RankTieRule::Average,
              RankNullRule nulls = RankNullRule::Keep,
              bool ascending = true,
              bool percentage = false) const;
  SlateValue skew(std::int64_t min_count = 3) const;
  SlateValue kurtosis(std::int64_t min_count = 4) const;
  Slate scan(ScanStatistic statistic, bool skip_nulls = true) const;
  Slate cumsum(bool skip_nulls = true) const;
  Slate cumprod(bool skip_nulls = true) const;
  Slate cummin(bool skip_nulls = true) const;
  Slate cummax(bool skip_nulls = true) const;
  SlateValue corr_with(const Slate& other,
                        SlateCorrMethod method = SlateCorrMethod::Pearson,
                        std::int64_t min_count = 1) const;
  SlateValue matmul(const Slate& other) const;
  SlateValue ndistinct(bool dropnull = true) const;
  Slate duplicated(bool keep_first = true, bool keep_last = false) const;
  Slate repeated() const;

  Slate weigh(pluss::msharp::RangeView::Double weights,
               WindowStatistic statistic = WindowStatistic::Mean,
               std::int64_t min_count = 1) const;
  Slate bounded(const WindowBoundsFn& bounds,
                 WindowStatistic statistic = WindowStatistic::Mean,
                 const WindowOptions& options = {}) const;

  Slate map(const std::function<SlateValue(const SlateValue&)>& fn) const;
  Slate transform(const std::function<SlateValue(const SlateValue&)>& fn) const;
  Slate transform(const std::function<Slate(const Slate&)>& fn,
                   ResultShape shape = ResultShape::Preserve) const;
  Slate fold(const std::function<Slate(const Slate&)>& fn,
              ResultShape shape = ResultShape::Reduce) const;
  Slate flow(const std::function<Slate(const Slate&)>& fn) const;

  // Note payload operations. Prefer note().method() in public code.
  Slate strip() const;
  Slate strip(std::string_view chars) const;
  Slate lstrip() const;
  Slate lstrip(std::string_view chars) const;
  Slate rstrip() const;
  Slate rstrip(std::string_view chars) const;
  Slate lower() const;
  Slate upper() const;
  Slate casefold() const;
  Slate titlecase() const;
  Slate length() const;
  Slate starts(std::string_view prefix) const;
  Slate ends(std::string_view suffix) const;
  Slate matches(std::string_view pattern) const;
  Slate fullmatch(std::string_view pattern) const;
  DataFrame extract(std::string_view pattern,
                    pluss::msharp::RangeView::Of<std::string_view> names = {}) const;
  DataFrame split(std::string_view delimiter,
                  std::int64_t limit = -1,
                  std::string prefix = "part") const;
  DataFrame split(std::string_view pattern,
                  const TextOptions& options) const;
  DataFrame partition(std::string_view separator,
                      std::string prefix = "part") const;
  Slate pad(std::int64_t width,
             TextAlign align = TextAlign::Left,
             char fill = ' ') const;
  Slate chars(std::int64_t start,
               std::optional<std::int64_t> stop = std::nullopt,
               std::int64_t step = 1) const;
  Slate normalize(UnicodeForm form = UnicodeForm::NFC) const;
  Slate encode(std::string_view encoding = "base64") const;
  DataFrame indicate(std::string_view delimiter = "|",
                     std::string prefix = "") const;
  Slate replace(std::string_view from, std::string_view to) const;
  Slate replace(std::string_view from,
                 std::string_view to,
                 const TextOptions& options) const;
  Slate put(const Slate& bool_mask, const Slate& value) const;
  Slate put(const Slate& bool_mask, double value) const;
  Slate put(const Slate& bool_mask, std::string_view value) const;
  Slate put(const Slate& bool_mask, const char* value) const {
    return put(bool_mask, std::string_view{value != nullptr ? value : ""});
  }
  Slate put(const Slate& bool_mask, pluss::null_t value) const;
  Slate put(const pluss::Series& bool_mask, const Slate& value) const {
    return put(Slate(bool_mask), value);
  }
  Slate put(const pluss::Series& bool_mask, double value) const {
    return put(Slate(bool_mask), value);
  }
  Slate put(const pluss::Series& bool_mask, std::string_view value) const {
    return put(Slate(bool_mask), value);
  }
  Slate put(const pluss::Series& bool_mask, const char* value) const {
    return put(Slate(bool_mask), value);
  }
  Slate put(const pluss::Series& bool_mask, pluss::null_t value) const {
    return put(Slate(bool_mask), value);
  }
  Slate replace(double from, double to) const;
  Slate replace(double from, pluss::null_t to) const;
  Slate replace(pluss::null_t from, double to) const;
  Slate replace(const pluss::Series& from, const pluss::Series& to) const;
  Slate replace(const pluss::Series& from, pluss::null_t to) const;
  Slate replace(pluss::null_t from, const pluss::Series& to) const;
  Slate replace(const pluss::Note& from, const pluss::Note& to) const;
  Slate replace(const pluss::Note& from, pluss::null_t to) const;
  Slate replace(pluss::null_t from, const pluss::Note& to) const;
  Slate replace(const pluss::Timeline& from, const pluss::Timeline& to) const;
  Slate replace(const pluss::Timeline& from, pluss::null_t to) const;
  Slate replace(pluss::null_t from, const pluss::Timeline& to) const;
  Slate replace(const pluss::Duration& from, const pluss::Duration& to) const;
  Slate replace(const pluss::Duration& from, pluss::null_t to) const;
  Slate replace(pluss::null_t from, const pluss::Duration& to) const;
  static Slate select(pluss::msharp::RangeView::Of<Slate> conditions,
                       pluss::msharp::RangeView::Of<Slate> choices,
                       const Slate& default_value);
  static Slate select(pluss::msharp::RangeView::Of<Slate> conditions,
                       pluss::msharp::RangeView::Of<Slate> choices,
                       double default_value);
  static Slate select(pluss::msharp::RangeView::Of<Slate> conditions,
                       pluss::msharp::RangeView::Of<Slate> choices,
                       std::string_view default_value);
  static Slate select(pluss::msharp::RangeView::Of<Slate> conditions,
                       pluss::msharp::RangeView::Of<Slate> choices,
                       const char* default_value) {
    return select(
        conditions,
        choices,
        std::string_view{default_value != nullptr ? default_value : ""});
  }
  static Slate select(pluss::msharp::RangeView::Of<Slate> conditions,
                       pluss::msharp::RangeView::Of<Slate> choices,
                       const pluss::Timeline& default_value);
  static Slate select(pluss::msharp::RangeView::Of<Slate> conditions,
                       pluss::msharp::RangeView::Of<Slate> choices,
                       pluss::null_t default_value);
  static Slate select(std::initializer_list<Slate> conditions,
                       std::initializer_list<Slate> choices,
                       const Slate& default_value) {
    return select(
        pluss::msharp::RangeView::Of<Slate>(conditions),
        pluss::msharp::RangeView::Of<Slate>(choices),
        default_value);
  }
  static Slate select(std::initializer_list<Slate> conditions,
                       std::initializer_list<Slate> choices,
                       double default_value) {
    return select(
        pluss::msharp::RangeView::Of<Slate>(conditions),
        pluss::msharp::RangeView::Of<Slate>(choices),
        default_value);
  }
  static Slate select(std::initializer_list<Slate> conditions,
                       std::initializer_list<Slate> choices,
                       std::string_view default_value) {
    return select(
        pluss::msharp::RangeView::Of<Slate>(conditions),
        pluss::msharp::RangeView::Of<Slate>(choices),
        default_value);
  }
  static Slate select(std::initializer_list<Slate> conditions,
                       std::initializer_list<Slate> choices,
                       const char* default_value) {
    return select(
        pluss::msharp::RangeView::Of<Slate>(conditions),
        pluss::msharp::RangeView::Of<Slate>(choices),
        default_value);
  }
  static Slate select(std::initializer_list<Slate> conditions,
                       std::initializer_list<Slate> choices,
                       const pluss::Timeline& default_value) {
    return select(
        pluss::msharp::RangeView::Of<Slate>(conditions),
        pluss::msharp::RangeView::Of<Slate>(choices),
        default_value);
  }
  static Slate select(std::initializer_list<Slate> conditions,
                       std::initializer_list<Slate> choices,
                       pluss::null_t default_value) {
    return select(
        pluss::msharp::RangeView::Of<Slate>(conditions),
        pluss::msharp::RangeView::Of<Slate>(choices),
        default_value);
  }
  Slate has(double value) const;
  Slate has(std::string_view value) const;
  Slate has(const char* value) const { return has(std::string_view{value != nullptr ? value : ""}); }
  Slate has(const Slate& values) const;

  Slate add(double other) const;
  Slate add(const Slate& other) const;
  Slate radd(double other) const;
  Slate radd(const Slate& other) const;
  Slate sub(double other) const;
  Slate sub(const Slate& other) const;
  Slate rsub(double other) const;
  Slate rsub(const Slate& other) const;
  Slate mul(double other) const;
  Slate mul(const Slate& other) const;
  Slate rmul(double other) const;
  Slate rmul(const Slate& other) const;
  Slate div(double other) const;
  Slate div(const Slate& other) const;
  Slate rdiv(double other) const;
  Slate rdiv(const Slate& other) const;
  Slate truediv(double other) const;
  Slate truediv(const Slate& other) const;
  Slate rtruediv(double other) const;
  Slate rtruediv(const Slate& other) const;
  Slate floordiv(double other) const;
  Slate floordiv(const Slate& other) const;
  Slate rfloordiv(double other) const;
  Slate rfloordiv(const Slate& other) const;
  Slate mod(double other) const;
  Slate mod(const Slate& other) const;
  Slate rmod(double other) const;
  Slate rmod(const Slate& other) const;
  Slate pow(double other) const;
  Slate pow(const Slate& other) const;
  Slate rpow(double other) const;
  Slate rpow(const Slate& other) const;
  Slate compare(double other, SlateBinaryCompareOp op) const;
  Slate compare(std::string_view other, SlateBinaryCompareOp op) const;
  Slate compare(const char* other, SlateBinaryCompareOp op) const {
    return compare(std::string_view{other != nullptr ? other : ""}, op);
  }
  Slate compare(const pluss::Series& other, SlateBinaryCompareOp op) const;
  Slate compare(const pluss::Note& other, SlateBinaryCompareOp op) const;
  Slate compare(const pluss::Timeline& other, SlateBinaryCompareOp op) const;
  Slate compare(const pluss::Duration& other, SlateBinaryCompareOp op) const;
  Slate compare(const Slate& other, SlateBinaryCompareOp op) const;
  Slate eq(double other) const;
  Slate eq(std::string_view other) const;
  Slate eq(const char* other) const { return eq(std::string_view{other != nullptr ? other : ""}); }
  Slate eq(const Slate& other) const;
  Slate ne(double other) const;
  Slate ne(std::string_view other) const;
  Slate ne(const char* other) const { return ne(std::string_view{other != nullptr ? other : ""}); }
  Slate ne(const Slate& other) const;
  Slate lt(double other) const;
  Slate lt(std::string_view other) const;
  Slate lt(const char* other) const { return lt(std::string_view{other != nullptr ? other : ""}); }
  Slate lt(const Slate& other) const;
  Slate le(double other) const;
  Slate le(std::string_view other) const;
  Slate le(const char* other) const { return le(std::string_view{other != nullptr ? other : ""}); }
  Slate le(const Slate& other) const;
  Slate gt(double other) const;
  Slate gt(std::string_view other) const;
  Slate gt(const char* other) const { return gt(std::string_view{other != nullptr ? other : ""}); }
  Slate gt(const Slate& other) const;
  Slate ge(double other) const;
  Slate ge(std::string_view other) const;
  Slate ge(const char* other) const { return ge(std::string_view{other != nullptr ? other : ""}); }
  Slate ge(const Slate& other) const;
  Slate land(const Slate& other) const;
  Slate rand(const Slate& other) const;
  Slate lor(const Slate& other) const;
  Slate ror(const Slate& other) const;
  Slate lxor(const Slate& other) const;
  Slate rxor(const Slate& other) const;
  Slate between(double a, double b, bool inclusive = false) const;
  Slate between(double a, double b, bool include_left, bool include_right) const;
  Slate between(std::string_view lo, std::string_view hi, bool inclusive = true) const;
  Slate between(std::string_view lo, std::string_view hi, bool include_left, bool include_right) const;
  Slate between(const Slate& lo, const Slate& hi, bool inclusive = true) const;
  Slate between(const Slate& lo, const Slate& hi, bool include_left, bool include_right) const;
  Slate window(std::int64_t window, SlateWindowOp op, std::int64_t min_periods = 1) const;
  SlateCastAccessor cast() const;
  SlateCastAccessor cast(const SlateCastOptions& options) const;
  Slate astype(pluss::codem::ScalarType dtype) const;
  SlateGroupBuilder group() const;
  SlateGroupBuilder group(const SlateGroupOptions& options) const;

  std::string to_string(const display::DisplayOptions& options = {}) const;
  std::string info_string() const;
  // A raw Slate has no show(): its terse column view is rarely what you want.
  // Reach for `framerize(name).show()` for the aligned table view, `describe()`
  // / `view()` for a SlateView you can `.show()`, or `to_string()` for a
  // string. `operator<<` still prints a Slate.
  void info() const;
  void info(std::ostream& os) const;
  SlatePlotAccessor plot() const;
  SlatePlotAccessor plot(const DataFramePlotOptions& options) const;
  SlateLabelAccessor label() const;
  Slate align(const Slate& target_labels, const AlignOptions& options = {}) const;
  bool named() const;
  std::string name() const;
  Slate name(std::string value) const;
  Slate unname() const;

  const pluss::Series& as_series() const;
  const pluss::Note& as_note() const;
  const pluss::Timeline& as_timeline() const;
  const pluss::Duration& as_duration() const;

  TimeSeriesAccessor ts() const;

private:
  friend class Atlas;
  friend class DataFrame;
  friend class DataFrameView;
  friend class SlateCastAccessor;
  friend class SlateGroupBuilder;
  friend class SlateGrouped;
  friend class SlateNoteAccessor;
  friend class SlateTimelineAccessor;
  friend class SlateLabelAccessor;
  friend class SlateValue;
  friend class SlateView;
  friend class TimeSeriesAccessor;
  friend bool detail::has_validity_mask(const Slate& slate);
  friend const std::optional<pluss::Series>& detail::validity_mask(const Slate& slate);
  friend Slate detail::with_validity_mask(const Slate& slate, const pluss::Series& validity_mask);
  friend Slate& detail::with_validity_mask_(Slate& slate, const pluss::Series& validity_mask);
  friend Slate detail::clear_validity_mask(const Slate& slate);
  friend Slate& detail::clear_validity_mask_(Slate& slate);
  friend bool detail::has_display_labels(const Slate& slate);
  friend Slate detail::display_labels(const Slate& slate);
  friend Slate detail::with_display_labels(const Slate& slate, const Slate& labels);
  friend Slate& detail::with_display_labels_(Slate& slate, const Slate& labels);
  friend Slate detail::clear_display_labels(const Slate& slate);
  friend Slate& detail::clear_display_labels_(Slate& slate);
  friend LabelKind detail::label_kind(const Slate& slate);
  friend Slate detail::with_label_kind(const Slate& slate, LabelKind kind);
  friend Slate& detail::with_label_kind_(Slate& slate, LabelKind kind);
  friend bool detail::valid_at(const Slate& slate, std::int64_t row);
  friend int detail::compare_rows(const Slate& slate,
                                  std::int64_t lhs_row,
                                  std::int64_t rhs_row,
                                  bool nulls_last);
  friend bool detail::same_payload(const Slate& left, const Slate& right) noexcept;
  friend Slate detail::math(const Slate& slate,
                             double other,
                             SlateBinaryMathOp op,
                             bool reverse);
  friend Slate detail::math(const Slate& left, const Slate& right, SlateBinaryMathOp op);

  explicit Slate(pluss::msharp::intrusive_ptr<detail::SlateImpl> impl);
  Slate cast_(SlateCastTarget target, const SlateCastOptions& options = {}) const;
  Slate cast_(SlateCastTarget target,
               std::optional<pluss::codem::ScalarType> series_dtype) const;

  bool unique() const noexcept;
  bool has_validity_mask() const;
  const std::optional<pluss::Series>& validity_mask() const;
  Slate with_validity_mask(const pluss::Series& validity_mask) const;
  Slate& with_validity_mask_(const pluss::Series& validity_mask);
  Slate clear_validity_mask() const;
  Slate& clear_validity_mask_();
  bool has_display_labels() const;
  Slate display_labels() const;
  Slate with_display_labels(const Slate& labels) const;
  Slate& with_display_labels_(const Slate& labels);
  Slate clear_display_labels() const;
  Slate& clear_display_labels_();
  bool has_caselbl() const;
  Slate caselbl() const;
  Slate caselbl(const Slate& labels) const;
  Slate& caselbl_(const Slate& labels);
  Slate rangelbl() const;
  Slate& rangelbl_();
  Slate clear_caselbl() const;
  Slate& clear_caselbl_();

  bool is_valid(std::int64_t row) const;
  int compare_rows(std::int64_t lhs_row, std::int64_t rhs_row, bool nulls_last = true) const;
  Slate year() const;
  Slate month() const;
  Slate day() const;
  Slate hour() const;
  bool same_payload_(const Slate& other) const noexcept;
  Slate math_(double other, SlateBinaryMathOp op, bool reverse = false) const;
  Slate math_(const Slate& other, SlateBinaryMathOp op) const;
private:
  Slate sample_fraction_(double frac,
                          std::optional<std::uint64_t> random_state,
                          bool replace) const;
  Slate moving(std::int64_t window,
                WindowStatistic statistic = WindowStatistic::Mean,
                const WindowOptions& options = {}) const;
  Slate growing(WindowStatistic statistic,
                 const StatisticOptions& options = {}) const;
  Slate decay(double alpha,
               WindowStatistic statistic = WindowStatistic::Mean,
               bool adjusted = true,
               bool bias_corrected = true,
               std::int64_t min_count = 1,
               bool ignore_nulls = false) const;
  Slate timed(const Slate& axis,
               std::int64_t duration_nanos,
               TimeWindowStatistic statistic = TimeWindowStatistic::Mean,
               const StatisticOptions& options = {},
               WindowClosure closure = WindowClosure::Right) const;
  Slate grouped(const Slate& groups,
                 std::int64_t window,
                 WindowStatistic statistic = WindowStatistic::Mean,
                 const WindowOptions& options = {}) const;

  detail::SlateImpl& mut_impl_();
  const detail::SlateImpl& impl_ref_() const;

  pluss::msharp::intrusive_ptr<detail::SlateImpl> impl_{nullptr};
};

namespace detail {
inline bool has_validity_mask(const Slate& slate) {
  return slate.has_validity_mask();
}

inline const std::optional<pluss::Series>& validity_mask(const Slate& slate) {
  return slate.validity_mask();
}

inline Slate with_validity_mask(const Slate& slate, const pluss::Series& validity_mask) {
  return slate.with_validity_mask(validity_mask);
}

inline Slate& with_validity_mask_(Slate& slate, const pluss::Series& validity_mask) {
  return slate.with_validity_mask_(validity_mask);
}

inline Slate clear_validity_mask(const Slate& slate) {
  return slate.clear_validity_mask();
}

inline Slate& clear_validity_mask_(Slate& slate) {
  return slate.clear_validity_mask_();
}

inline bool has_display_labels(const Slate& slate) {
  return slate.has_display_labels();
}

inline Slate display_labels(const Slate& slate) {
  return slate.display_labels();
}

inline Slate with_display_labels(const Slate& slate, const Slate& labels) {
  return slate.with_display_labels(labels);
}

inline Slate& with_display_labels_(Slate& slate, const Slate& labels) {
  return slate.with_display_labels_(labels);
}

inline Slate clear_display_labels(const Slate& slate) {
  return slate.clear_display_labels();
}

inline Slate& clear_display_labels_(Slate& slate) {
  return slate.clear_display_labels_();
}

inline bool valid_at(const Slate& slate, std::int64_t row) {
  return slate.is_valid(row);
}

inline int compare_rows(const Slate& slate,
                        std::int64_t lhs_row,
                        std::int64_t rhs_row,
                        bool nulls_last) {
  return slate.compare_rows(lhs_row, rhs_row, nulls_last);
}

inline bool same_payload(const Slate& left, const Slate& right) noexcept {
  return left.same_payload_(right);
}

inline Slate math(const Slate& slate,
                   double other,
                   SlateBinaryMathOp op,
                   bool reverse) {
  return slate.math_(other, op, reverse);
}

inline Slate math(const Slate& left, const Slate& right, SlateBinaryMathOp op) {
  return left.math_(right, op);
}

struct GroupStateCache {
  std::vector<Slate> keys{};
  std::vector<std::string> names{};
  std::vector<bool> included{};
  std::vector<std::int64_t> included_rows{};
  std::vector<std::int64_t> group_ids{};
  std::vector<std::int64_t> included_group_ids{};
  std::vector<std::int64_t> first_rows{};
  std::vector<std::int64_t> order{};
  std::vector<std::int64_t> order_positions{};
  std::vector<std::int64_t> ordered_first_rows{};
  std::vector<std::int64_t> group_offsets{};
  std::vector<std::int64_t> broadcast_positions{};
  std::vector<std::int64_t> grouped_rows{};
  pluss::Series included_mask_cpu = pluss::series::undefined();
  pluss::Series included_rows_cpu = pluss::series::undefined();
  pluss::Series group_ids_cpu = pluss::series::undefined();
  pluss::Series included_group_ids_cpu = pluss::series::undefined();
  std::int64_t ngroups{0};
  bool all_included{true};
};
} // namespace detail

struct TimeSeriesResample {
  Slate labels{};
  Slate values{};
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

class TimeSeriesAccessor {
 public:
  explicit TimeSeriesAccessor(Slate base);

  Slate returns(std::int64_t periods = 1) const;
  Slate log_returns(std::int64_t periods = 1) const;
  Slate diff(std::int64_t periods = 1) const;
  Slate lag(std::int64_t periods = 1) const;
  Slate lead(std::int64_t periods = 1) const;
  Slate rolling(std::int64_t window,
                 WindowStatistic statistic = WindowStatistic::Mean,
                 const WindowOptions& options = {}) const;
  Slate rolling(const Slate& axis,
                 std::string_view duration,
                 WindowStatistic statistic = WindowStatistic::Mean,
                 const StatisticOptions& options = {},
                 WindowClosure closure = WindowClosure::Right) const;
  Slate rolling(const Slate& axis,
                 const Slate& groups,
                 std::string_view duration,
                 WindowStatistic statistic = WindowStatistic::Mean,
                 const StatisticOptions& options = {},
                 WindowClosure closure = WindowClosure::Right) const;
  Slate timed(const Slate& axis,
               std::int64_t duration_nanos,
               TimeWindowStatistic statistic = TimeWindowStatistic::Mean,
               const StatisticOptions& options = {},
               WindowClosure closure = WindowClosure::Right) const;
  Slate grouped(const Slate& groups,
                 std::int64_t window,
                 WindowStatistic statistic = WindowStatistic::Mean,
                 const WindowOptions& options = {}) const;
  Slate expanding(WindowStatistic statistic = WindowStatistic::Mean,
                   const StatisticOptions& options = {}) const;
  Slate ewm(double alpha,
             WindowStatistic statistic = WindowStatistic::Mean,
             bool adjusted = true,
             bool bias_corrected = true,
             std::int64_t min_count = 1,
             bool ignore_nulls = false) const;
  Slate zscore_rolling(std::int64_t window,
                        std::int64_t min_periods = 1,
                        double epsilon = 1e-12) const;
  Slate fill_gaps(pluss::subsp::stats::ts::GapFillMode mode =
                       pluss::subsp::stats::ts::GapFillMode::Forward,
                   std::optional<double> constant = std::nullopt) const;
  Slate align_to(const Slate& source_axis,
                  const Slate& target_axis,
                  TsAlignMode mode = TsAlignMode::Exact) const;
  Slate align_to(const Slate& source_axis,
                  const Slate& target_axis,
                  TsAlignMode mode,
                  pluss::subsp::stats::ts::GapFillMode fill_mode,
                  std::optional<double> constant = std::nullopt) const;
  DataFrame lag_block(pluss::msharp::RangeView::Int lags) const;
  DataFrame rolling_block(pluss::msharp::RangeView::Int windows,
                          const TsRollingBlockOptions& options = {}) const;
  TimeSeriesResampler resample(const Slate& axis,
                               std::string_view freq,
                               std::int64_t step = 1) const;
  TimeSeriesResampler resample(const Slate& axis,
                               const pluss::Timeline::ResampleOptions& options) const;
  TimeSeriesResample resample(const Slate& axis,
                              std::string_view freq,
                              pluss::subsp::stats::ts::ResampleOp op,
                              std::int64_t step = 1) const;
  TimeSeriesResample resample(
      const Slate& axis,
      const pluss::Timeline::ResampleOptions& options,
      pluss::subsp::stats::ts::ResampleOp op) const;
  Slate add(const Slate& other,
             const Slate& left_axis,
             const Slate& right_axis,
             TsAlignMode mode = TsAlignMode::Exact) const;
  Slate sub(const Slate& other,
             const Slate& left_axis,
             const Slate& right_axis,
             TsAlignMode mode = TsAlignMode::Exact) const;
  Slate mul(const Slate& other,
             const Slate& left_axis,
             const Slate& right_axis,
             TsAlignMode mode = TsAlignMode::Exact) const;
  Slate div(const Slate& other,
             const Slate& left_axis,
             const Slate& right_axis,
             TsAlignMode mode = TsAlignMode::Exact) const;
  Slate eq(const Slate& other,
            const Slate& left_axis,
            const Slate& right_axis,
            TsAlignMode mode = TsAlignMode::Exact) const;
  Slate ne(const Slate& other,
            const Slate& left_axis,
            const Slate& right_axis,
            TsAlignMode mode = TsAlignMode::Exact) const;
  Slate lt(const Slate& other,
            const Slate& left_axis,
            const Slate& right_axis,
            TsAlignMode mode = TsAlignMode::Exact) const;
  Slate le(const Slate& other,
            const Slate& left_axis,
            const Slate& right_axis,
            TsAlignMode mode = TsAlignMode::Exact) const;
  Slate gt(const Slate& other,
            const Slate& left_axis,
            const Slate& right_axis,
            TsAlignMode mode = TsAlignMode::Exact) const;
  Slate ge(const Slate& other,
            const Slate& left_axis,
            const Slate& right_axis,
            TsAlignMode mode = TsAlignMode::Exact) const;
  DataFrame calendar(const Slate& axis) const;
  Slate sort(const Slate& axis,
              bool ascending = true,
              bool nulls_last = true) const;

 private:
  friend class DataFrameTsAccessor;
  const pluss::Series& series_payload_(const char* op) const;
  pluss::Series masked_series_payload_(const char* op) const;
  TimeSeriesResample resample_index_(
      const pluss::Timeline::TimelineIndexAccessor& index,
      const pluss::Timeline::ResampleOptions& options,
      pluss::subsp::stats::ts::ResampleOp op) const;
  Slate align_index_(
      const Slate& other,
      const Slate& left_axis,
      const Slate& right_axis,
      TsAlignMode mode,
      const pluss::Timeline::TimelineIndexAccessor* left_index,
      const pluss::Timeline::TimelineIndexAccessor* right_index) const;
  Slate base_{};
};

class TimeSeriesResampler {
 public:
  TimeSeriesResampler(Slate base,
                      Slate axis,
                      pluss::Timeline::ResampleOptions options);

  Slate mean() const;
  Slate sum() const;
  Slate min() const;
  Slate max() const;
  Slate first() const;
  Slate last() const;
  Slate count() const;

  Slate asfreq(std::optional<double> fill_value = std::nullopt) const;
  Slate ffill() const;
  Slate bfill() const;
  Slate nearest() const;

 private:
  Slate aggregate_(pluss::subsp::stats::ts::ResampleOp op) const;

  Slate base_{};
  Slate axis_{};
  pluss::Timeline::ResampleOptions options_{};
};

class SlateLabelAccessor {
public:
  explicit SlateLabelAccessor(Slate base);

  Slate values() const;
  bool has() const;
  Slate set(const Slate& labels) const;
  Slate range() const;
  Slate reset() const;
  Slate align(const Slate& target_labels, const AlignOptions& options = {}) const;
  Slate unite(const Slate& other) const;
  Slate intersect(const Slate& other) const;
  Slate diff(const Slate& other) const;
  Slate symdiff(const Slate& other) const;
  Slate find(const Slate& targets) const;
  Slate pos(double value) const;
  Slate pos(std::string_view value) const;
  Slate pos(const char* value) const {
    return pos(std::string_view{value != nullptr ? value : ""});
  }
  Slate slice(std::string_view start, std::string_view stop) const;
  Slate fillnull(double value) const;
  Slate fillnull(std::string_view value) const;
  Slate fillnull(const char* value) const {
    return fillnull(std::string_view{value != nullptr ? value : ""});
  }
  Slate fillnull(const pluss::Timeline& value) const;
  Slate fillnull(const pluss::Duration& value) const;
  Slate duplicated(bool keep_first = true, bool keep_last = false) const;
  Slate repeated() const;
  Slate distinct(bool dropnull = true) const;
  Slate freq(std::int64_t top_n = 0, bool dropnull = true) const;
  Slate has(const Slate& values) const;
  Slate isnull() const;
  Slate notnull() const;
  Slate map(const std::function<SlateValue(const SlateValue&)>& fn) const;
  Slate take(pluss::msharp::RangeView::Int indices) const;
  Slate slice(std::int64_t start, std::int64_t stop) const;
  Slate sort(bool ascending = true, bool nulls_last = true) const;
  bool monotonic(bool ascending = true, bool strict = false, bool nulls_last = true) const;

private:
  Slate base_{};
};

class SlateSortBuilder {
public:
  explicit SlateSortBuilder(Slate base, bool ascending = true, bool nulls_last = true);

  Slate asc() const;
  Slate desc() const;
  Slate done() const;
  operator Slate() const;

private:
  Slate base_{};
  bool ascending_{true};
  bool nulls_last_{true};
};

class SlateView final : public Slate {
 public:
  SlateView();
  explicit SlateView(Slate slate,
                      bool auto_render_terminal = false,
                      std::string operation = {});
  SlateView(const SlateView& other);
  SlateView(SlateView&& other) noexcept;
  SlateView& operator=(const SlateView& other);
  SlateView& operator=(SlateView&& other) noexcept;
  ~SlateView();

  void show(const display::DisplayOptions& options = {}) const;
  void show(std::ostream& os, const display::DisplayOptions& options = {}) const;

 private:
  struct State;
  std::shared_ptr<State> state_{};

  void consume_pending_display_() const;
  void maybe_autorender_() const;
  std::string resolved_window_title_() const;

  friend void output_render(std::ostream& os, const SlateView& slate);
  friend std::ostream& operator<<(std::ostream& os, const SlateView& slate);
};

class SlateCastAccessor {
public:
  explicit SlateCastAccessor(Slate base, SlateCastOptions options = {});

  Slate series() const;
  Slate note() const;
  Slate timeline() const;
  Slate duration() const;

private:
  Slate base_{};
  SlateCastOptions options_{};
};

class SlateGroupBuilder {
public:
  explicit SlateGroupBuilder(Slate value, SlateGroupOptions options = {});

  SlateGrouped by(const Slate& key) const;
  SlateGrouped by(std::initializer_list<Slate> keys) const;
  SlateGrouped by(pluss::msharp::RangeView::Of<Slate> keys) const;

private:
  Slate value_{};
  SlateGroupOptions options_{};
};

class SlateGrouped {
public:
  SlateGrouped(Slate value,
                std::vector<Slate> keys,
                std::vector<std::string> names,
                SlateGroupOptions options = {});

  DataFrame agg(pluss::msharp::RangeView::Of<SlateGroupSpec> specs) const;
  DataFrame agg(std::initializer_list<SlateGroupSpec> specs) const;
  Slate transform(GroupOp op) const;
  DataFrame transform(pluss::msharp::RangeView::Of<SlateGroupSpec> specs) const;
  DataFrame transform(std::initializer_list<SlateGroupSpec> specs) const;
  Slate filter(const pluss::Series& keep) const;
  Slate filter(const Slate& keep) const;

  DataFrame count() const;
  DataFrame ndistinct() const;
  DataFrame sum() const;
  DataFrame mean() const;
  DataFrame median() const;
  DataFrame min() const;
  DataFrame max() const;
  DataFrame std() const;
  DataFrame var() const;
  DataFrame mode() const;

private:
  const detail::GroupStateCache& state_cached_() const;

  DataFrame terminal_(GroupOp op) const;

  Slate value_{};
  std::vector<Slate> keys_{};
  std::vector<std::string> names_{};
  SlateGroupOptions options_{};
  mutable std::shared_ptr<const detail::GroupStateCache> state_cache_{};
};

class SlateNoteAccessor {
public:
  explicit SlateNoteAccessor(Slate base);

  Slate strip() const;
  Slate strip(std::string_view chars) const;
  Slate lstrip() const;
  Slate lstrip(std::string_view chars) const;
  Slate rstrip() const;
  Slate rstrip(std::string_view chars) const;
  Slate lower() const;
  Slate upper() const;
  Slate casefold() const;
  Slate titlecase() const;
  Slate length() const;
  Slate starts(std::string_view prefix) const;
  Slate ends(std::string_view suffix) const;
  Slate has(std::string_view needle,
             const TextOptions& options = {}) const;
  Slate count(std::string_view pattern) const;
  Slate count(std::string_view pattern,
               const TextOptions& options) const;
  Slate matches(std::string_view pattern, bool full = false) const;
  Slate fullmatch(std::string_view pattern) const;
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
  Slate pad(std::int64_t width,
             TextAlign align = TextAlign::Left,
             char fill = ' ') const;
  Slate chars(std::int64_t start,
               std::optional<std::int64_t> stop = std::nullopt,
               std::int64_t step = 1) const;
  Slate normalize(UnicodeForm form = UnicodeForm::NFC) const;
  Slate encode(std::string_view encoding = "base64") const;
  DataFrame indicate(std::string_view delimiter = "|",
                     std::string prefix = "") const;
  Slate replace(std::string_view from, std::string_view to) const;
  Slate replace(std::string_view from,
                 std::string_view to,
                 const TextOptions& options) const;

private:
  Slate base_{};
};

class SlateTimelineAccessor {
public:
  explicit SlateTimelineAccessor(Slate base);

  Slate isnat() const;
  Slate valid() const;
  Slate year() const;
  Slate month() const;
  Slate day() const;
  Slate hour() const;
  Slate minute() const;
  Slate second() const;
  Slate weekday() const;
  Slate yearday() const;
  Slate quarter() const;
  Slate week() const;
  Slate weekend() const;
  Slate month_start() const;
  Slate month_end() const;
  Slate leap() const;
  Slate weekday_name() const;
  Slate month_name() const;
  Slate fmt(std::string_view pattern) const;
  Slate iso() const;
  Slate epoch(TimeUnit unit = TimeUnit::Nanos) const;
  Slate span(const Slate& other, const TimelineSpanOptions& options = {}) const;
  Slate span(const Slate& other, TimeUnit unit) const;
  Slate span(const pluss::Timeline& other, const TimelineSpanOptions& options = {}) const;
  Slate span(const pluss::Timeline& other, TimeUnit unit) const;
  Slate floor(std::string_view unit) const;
  Slate ceil(std::string_view unit) const;
  Slate round(std::string_view unit) const;
  Slate date() const;
  Slate time() const;
  Slate shift(std::int64_t periods, std::string_view freq) const;
  Slate period(std::string_view freq) const;
  Slate tz(std::string_view name) const;
  Slate localize(const TimelineLocalizeOptions& options = {}) const;
  Slate utc() const;

private:
  Slate base_{};
};

Slate operator+(const Slate& lhs, double rhs);
Slate operator+(double lhs, const Slate& rhs);
Slate operator+(const Slate& lhs, const Slate& rhs);
Slate operator-(const Slate& lhs, double rhs);
Slate operator-(double lhs, const Slate& rhs);
Slate operator-(const Slate& lhs, const Slate& rhs);
Slate operator*(const Slate& lhs, double rhs);
Slate operator*(double lhs, const Slate& rhs);
Slate operator*(const Slate& lhs, const Slate& rhs);
Slate operator/(const Slate& lhs, double rhs);
Slate operator/(double lhs, const Slate& rhs);
Slate operator/(const Slate& lhs, const Slate& rhs);
Slate operator%(const Slate& lhs, double rhs);
Slate operator%(double lhs, const Slate& rhs);
Slate operator%(const Slate& lhs, const Slate& rhs);
Slate operator&(const Slate& lhs, const Slate& rhs);
Slate operator|(const Slate& lhs, const Slate& rhs);
Slate operator^(const Slate& lhs, const Slate& rhs);
Slate operator==(const Slate& lhs, double rhs);
Slate operator==(double lhs, const Slate& rhs);
Slate operator==(const Slate& lhs, std::string_view rhs);
Slate operator==(std::string_view lhs, const Slate& rhs);
Slate operator==(const Slate& lhs, const char* rhs);
Slate operator==(const char* lhs, const Slate& rhs);
Slate operator==(const Slate& lhs, const pluss::Series& rhs);
Slate operator==(const Slate& lhs, const pluss::Note& rhs);
Slate operator==(const Slate& lhs, const pluss::Timeline& rhs);
Slate operator==(const Slate& lhs, const Slate& rhs);
Slate operator!=(const Slate& lhs, double rhs);
Slate operator!=(double lhs, const Slate& rhs);
Slate operator!=(const Slate& lhs, std::string_view rhs);
Slate operator!=(std::string_view lhs, const Slate& rhs);
Slate operator!=(const Slate& lhs, const char* rhs);
Slate operator!=(const char* lhs, const Slate& rhs);
Slate operator!=(const Slate& lhs, const pluss::Series& rhs);
Slate operator!=(const Slate& lhs, const pluss::Note& rhs);
Slate operator!=(const Slate& lhs, const pluss::Timeline& rhs);
Slate operator!=(const Slate& lhs, const Slate& rhs);
Slate operator<(const Slate& lhs, double rhs);
Slate operator<(double lhs, const Slate& rhs);
Slate operator<(const Slate& lhs, std::string_view rhs);
Slate operator<(std::string_view lhs, const Slate& rhs);
Slate operator<(const Slate& lhs, const char* rhs);
Slate operator<(const char* lhs, const Slate& rhs);
Slate operator<(const Slate& lhs, const pluss::Series& rhs);
Slate operator<(const Slate& lhs, const pluss::Note& rhs);
Slate operator<(const Slate& lhs, const pluss::Timeline& rhs);
Slate operator<(const Slate& lhs, const Slate& rhs);
Slate operator<=(const Slate& lhs, double rhs);
Slate operator<=(double lhs, const Slate& rhs);
Slate operator<=(const Slate& lhs, std::string_view rhs);
Slate operator<=(std::string_view lhs, const Slate& rhs);
Slate operator<=(const Slate& lhs, const char* rhs);
Slate operator<=(const char* lhs, const Slate& rhs);
Slate operator<=(const Slate& lhs, const pluss::Series& rhs);
Slate operator<=(const Slate& lhs, const pluss::Note& rhs);
Slate operator<=(const Slate& lhs, const pluss::Timeline& rhs);
Slate operator<=(const Slate& lhs, const Slate& rhs);
Slate operator>(const Slate& lhs, double rhs);
Slate operator>(double lhs, const Slate& rhs);
Slate operator>(const Slate& lhs, std::string_view rhs);
Slate operator>(std::string_view lhs, const Slate& rhs);
Slate operator>(const Slate& lhs, const char* rhs);
Slate operator>(const char* lhs, const Slate& rhs);
Slate operator>(const Slate& lhs, const pluss::Series& rhs);
Slate operator>(const Slate& lhs, const pluss::Note& rhs);
Slate operator>(const Slate& lhs, const pluss::Timeline& rhs);
Slate operator>(const Slate& lhs, const Slate& rhs);
Slate operator>=(const Slate& lhs, double rhs);
Slate operator>=(double lhs, const Slate& rhs);
Slate operator>=(const Slate& lhs, std::string_view rhs);
Slate operator>=(std::string_view lhs, const Slate& rhs);
Slate operator>=(const Slate& lhs, const char* rhs);
Slate operator>=(const char* lhs, const Slate& rhs);
Slate operator>=(const Slate& lhs, const pluss::Series& rhs);
Slate operator>=(const Slate& lhs, const pluss::Note& rhs);
Slate operator>=(const Slate& lhs, const pluss::Timeline& rhs);
Slate operator>=(const Slate& lhs, const Slate& rhs);

class SlateValue {
public:
  using Payload = std::variant<pluss::Series, pluss::Note, pluss::Timeline, pluss::Duration>;

  class HostAny {
  public:
    enum class Kind : std::uint8_t {
      Bool = 0,
      Int = 1,
      UInt = 2,
      Double = 3,
      String = 4,
      Time = 5,
      Duration = 6,
    };

    using Storage = std::variant<bool, std::int64_t, std::uint64_t, double, std::string>;

    HostAny() = default;
    static HostAny boolean(bool value);
    static HostAny integer(std::int64_t value);
    static HostAny unsigned_integer(std::uint64_t value);
    static HostAny floating(double value);
    static HostAny string(std::string value);
    static HostAny time(std::int64_t epoch_nanos, std::string iso);
    static HostAny duration(std::int64_t nanos, std::string text);

    Kind kind() const noexcept;
    const Storage& storage() const noexcept;
    double to_dbl() const;
    std::int64_t to_int() const;
    std::string to_str() const;

  private:
    Kind kind_{Kind::Int};
    Storage storage_{std::int64_t{0}};
    std::string display_{};
  };

  SlateValue() = default;
  explicit SlateValue(Slate slate);

  bool defined() const noexcept;
  SlateKind kind() const;
  const Slate& slate() const noexcept;
  Payload payload() const;

  pluss::Series as_series() const;
  pluss::Note as_note() const;
  pluss::Timeline as_timeline() const;
  pluss::Duration as_duration() const;
  HostAny host() const;
  double to_dbl() const;
  std::int64_t to_int() const;
  std::string to_str() const;

  operator pluss::Series() const;
  operator pluss::Note() const;
  operator pluss::Timeline() const;
  operator pluss::Duration() const;

private:
  Slate slate_{};
};

std::ostream& operator<<(std::ostream& os, const Slate& slate);
void output_render(std::ostream& os, const Slate& slate);
std::ostream& operator<<(std::ostream& os, const SlateView& slate);
void output_render(std::ostream& os, const SlateView& slate);
std::ostream& operator<<(std::ostream& os, const SlateValue& value);
void output_render(std::ostream& os, const SlateValue& value);
std::ostream& operator<<(std::ostream& os, const SlateValue::HostAny& value);

} // namespace pluss::table
