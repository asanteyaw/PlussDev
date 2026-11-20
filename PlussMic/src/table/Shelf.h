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

class Shelf;
enum class LabelKind : std::uint8_t;
enum class ShelfBinaryMathOp : std::uint8_t;
struct DataFramePlotOptions;

namespace detail {
class ShelfImpl;
struct GroupStateCache;
bool has_validity_mask(const Shelf& shelf);
const std::optional<pluss::Series>& validity_mask(const Shelf& shelf);
Shelf with_validity_mask(const Shelf& shelf, const pluss::Series& validity_mask);
Shelf& with_validity_mask_(Shelf& shelf, const pluss::Series& validity_mask);
Shelf clear_validity_mask(const Shelf& shelf);
Shelf& clear_validity_mask_(Shelf& shelf);
bool has_display_labels(const Shelf& shelf);
Shelf display_labels(const Shelf& shelf);
Shelf with_display_labels(const Shelf& shelf, const Shelf& labels);
Shelf& with_display_labels_(Shelf& shelf, const Shelf& labels);
Shelf clear_display_labels(const Shelf& shelf);
Shelf& clear_display_labels_(Shelf& shelf);
LabelKind label_kind(const Shelf& shelf);
Shelf with_label_kind(const Shelf& shelf, LabelKind kind);
Shelf& with_label_kind_(Shelf& shelf, LabelKind kind);
bool valid_at(const Shelf& shelf, std::int64_t row);
int compare_rows(const Shelf& shelf,
                 std::int64_t lhs_row,
                 std::int64_t rhs_row,
                 bool nulls_last = true);
bool same_payload(const Shelf& left, const Shelf& right) noexcept;
Shelf math(const Shelf& shelf,
            double other,
            ShelfBinaryMathOp op,
            bool reverse = false);
Shelf math(const Shelf& left, const Shelf& right, ShelfBinaryMathOp op);
Shelf freq_count_shelf(const Shelf& shelf,
                         std::int64_t top_n,
                         bool dropnull,
                         std::string_view display_name);
}
class ShelfValue;
class DataFrame;
class DataFrameView;
class Atlas;
class ShelfView;
class ShelfPlotAccessor;
class ShelfCastAccessor;
class ShelfGroupBuilder;
class ShelfGrouped;
class ShelfSortBuilder;
class ShelfNoteAccessor;
class ShelfTimelineAccessor;
class ShelfLabelAccessor;
class TimeSeriesAccessor;
class TimeSeriesResampler;

enum class ShelfKind : std::uint8_t {
  Series = 0,
  Note = 1,
  Timeline = 2,
  Duration = 4,
};

std::string_view shelf_kind_name(ShelfKind kind) noexcept;

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

struct ShelfOptions {
  bool rangelbl{false};
};

enum class ShelfBinaryMathOp : std::uint8_t {
  Add = 0,
  Sub = 1,
  Mul = 2,
  Div = 3,
  TrueDiv = 4,
  FloorDiv = 5,
  Mod = 6,
  Pow = 7,
};

enum class ShelfBinaryCompareOp : std::uint8_t {
  Eq = 0,
  Ne = 1,
  Lt = 2,
  Le = 3,
  Gt = 4,
  Ge = 5,
};

enum class ShelfCastTarget : std::uint8_t {
  Series = 0,
  Note = 1,
  Timeline = 2,
  Duration = 4,
};

enum class ParseInvalid : std::uint8_t {
  Raise = 0,
  Null = 1,
};

struct ShelfCastOptions {
  std::optional<pluss::codem::ScalarType> series_dtype{std::nullopt};
  std::string format{};
  ParseInvalid invalid{ParseInvalid::Raise};
};

enum class ShelfWindowOp : std::uint8_t {
  RollingSum = 0,
  RollingMean = 1,
  RollingMin = 2,
  RollingMax = 3,
};

enum class ShelfCorrMethod : std::uint8_t {
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

enum class ShelfFreqKind : std::uint8_t {
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

struct ShelfFreqOptions {
  ShelfFreqKind kind{ShelfFreqKind::Count};
  bool dropnull{true};
  bool sort{true};
  bool ascending{false};
  std::int64_t top{0};
};

struct ShelfGroupOptions {
  bool dropnull{true};
  bool sort{false};
};

struct ShelfGroupSpec {
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

class Shelf {
public:
  Shelf();
  Shelf(const pluss::Series& data, std::optional<pluss::Series> validity_mask = std::nullopt);
  Shelf(const pluss::Series& data, const ShelfOptions& options);
  Shelf(const pluss::Series& data,
         std::optional<pluss::Series> validity_mask,
         const ShelfOptions& options);
  Shelf(const pluss::Note& data, std::optional<pluss::Series> validity_mask = std::nullopt);
  Shelf(const pluss::Note& data, const ShelfOptions& options);
  Shelf(const pluss::Note& data,
         std::optional<pluss::Series> validity_mask,
         const ShelfOptions& options);
  Shelf(const pluss::Timeline& data, std::optional<pluss::Series> validity_mask = std::nullopt);
  Shelf(const pluss::Timeline& data, const ShelfOptions& options);
  Shelf(const pluss::Timeline& data,
         std::optional<pluss::Series> validity_mask,
         const ShelfOptions& options);
  Shelf(const pluss::Duration& data, std::optional<pluss::Series> validity_mask = std::nullopt);
  Shelf(const pluss::Duration& data, const ShelfOptions& options);
  Shelf(const pluss::Duration& data,
         std::optional<pluss::Series> validity_mask,
         const ShelfOptions& options);
  Shelf(const Shelf& other);
  Shelf(Shelf&& other) noexcept;
  Shelf& operator=(const Shelf& other);
  Shelf& operator=(Shelf&& other) noexcept;
  ~Shelf();

  bool defined() const noexcept;
  std::int64_t size() const;
  std::int64_t ncases() const;
  std::pair<std::int64_t, std::int64_t> shape() const;
  ShelfKind kind() const;
  std::string ptype() const;
  std::string dtype() const;
  ShelfValue value() const;

  // 1D counterparts to DataFrame::kindelize()/scribbelize() and
  // Atlas::kindelize()/scribbelize().
  pluss::kindle::Stream kindelize() const;
  pluss::scribble::Slot scribbelize() const;

  Shelf isnull() const;
  Shelf notnull() const;
  // NaN and infinity are values, not missing cells; see NonFinite.h. A null cell is never NaN or infinite, and a payload that
  // cannot hold them (integers, bool, text, time) has none, so isnan / isinf are all false there and notnan / notinf all true.
  Shelf isnan() const;
  Shelf notnan() const;
  Shelf isinf() const;
  Shelf isposinf() const;
  Shelf isneginf() const;
  Shelf notinf() const;
  Shelf dropnan() const;
  Shelf dropinf() const;
  Shelf dropposinf() const;
  Shelf dropneginf() const;
  // The catch-all: drop / fill every cell invalid() flags (NULL, NaN and +-inf together).
  Shelf dropinvalid() const;
  Shelf fillinvalid(double value) const;
  Shelf fillnan(double value) const;
  Shelf fillinf(double value) const;
  Shelf fillposinf(double value) const;
  Shelf fillneginf(double value) const;
  Shelf isfinite() const;
  // How many cells are of each kind. NULL has no value; NaN, +inf and -inf are values; nnull + nnan + nposinf + nneginf never
  // double count a cell, and a column that cannot hold NaN or infinity reports 0 for them.
  std::int64_t nnull() const;
  std::int64_t nnan() const;
  std::int64_t nposinf() const;
  std::int64_t nneginf() const;
  Shelf invalid() const;

  // Functional-by-default transforms.
  Shelf fillnull(double value) const;
  Shelf fillnull(std::string_view value) const;
  Shelf fillnull(const char* value) const {
    return fillnull(std::string_view{value != nullptr ? value : ""});
  }
  Shelf fillnull(const pluss::Timeline& value) const;
  Shelf fillnull(const pluss::Duration& value) const;
  Shelf abs() const;
  Shelf clip(double min_value, double max_value) const;
  Shelf round(std::int64_t decimals = 0) const;
  Shelf log() const;
  Shelf exp() const;

  // Explicit mutating transforms (copy-on-write when shared).
  Shelf& fillnull_(double value);
  Shelf& fillnull_(std::string_view value);
  Shelf& fillnull_(const char* value) {
    return fillnull_(std::string_view{value != nullptr ? value : ""});
  }
  Shelf& fillnull_(const pluss::Timeline& value);
  Shelf& fillnull_(const pluss::Duration& value);

  // 1D row-space transforms.
  ShelfView head(std::int64_t n = 5, bool render = false) const;
  ShelfView tail(std::int64_t n = 5, bool render = false) const;
  ShelfView view(std::int64_t start, std::int64_t stop, bool render = false) const;
  ShelfView top(std::int64_t n, bool render = false) const;
  ShelfView bottom(std::int64_t n, bool render = false) const;
  Shelf append(const Shelf& other) const;
  Shelf sample(std::int64_t n) const;
  Shelf sample(std::int64_t n, std::uint64_t random_state, bool replace = false) const;
  template <class Float>
    requires std::floating_point<Float>
  Shelf sample(Float frac) const {
    return sample_fraction_(static_cast<double>(frac), std::nullopt, false);
  }
  template <class Float>
    requires std::floating_point<Float>
  Shelf sample(Float frac, std::uint64_t random_state, bool replace = false) const {
    return sample_fraction_(static_cast<double>(frac), random_state, replace);
  }
  Shelf shuffle(std::optional<std::uint64_t> random_state = std::nullopt) const;
  Shelf shift(std::int64_t periods = 1) const;
  Shelf slice(std::int64_t start, std::int64_t stop) const;
  Shelf take(std::span<const std::int64_t> indices) const;
  Shelf filter(const pluss::Series& bool_mask) const;
  Shelf filter(const Shelf& bool_mask) const;
  Shelf dropnull() const;
  Shelf arrange(bool ascending = true, bool nulls_last = true) const;
  ShelfSortBuilder sort(bool ascending = true, bool nulls_last = true) const;
  Shelf ffill(std::int64_t limit = -1) const;
  Shelf bfill(std::int64_t limit = -1) const;
  Shelf interp(std::int64_t limit = -1) const;
  Shelf distinct(bool dropnull = true) const;
  Shelf freq() const;
  Shelf freq(ShelfFreqOptions options) const;
  ShelfView describe(bool render = false) const;
  Shelf bucketize(pluss::msharp::RangeView::Double bins,
                   pluss::msharp::RangeView::Int labels = {},
                   bool right = true,
                   bool include_lowest = true) const;

  // Reshape this Shelf into a one-column DataFrame -- the 1-D -> 2-D
  // counterpart of pandas `Series.to_frame`. The column takes `name`; when
  // `name` is empty it falls back to this Shelf's own name, or "x" if it has
  // none. This Shelf's display labels (its index) become the frame's cases.
  DataFrame framerize(std::string_view name = {}) const;

  ShelfNoteAccessor nt() const;
  ShelfTimelineAccessor tl() const;

  // Reductions return scalar payloads where the operation is valid.
  ShelfValue sum() const;
  ShelfValue prod() const;
  ShelfValue count(bool count_true = true) const;
  ShelfValue any(bool skipnull = true) const;
  ShelfValue all(bool skipnull = true) const;
  ShelfValue mean() const;
  ShelfValue median() const;
  ShelfValue min() const;
  ShelfValue max() const;
  ShelfValue csmin() const;
  ShelfValue csmax() const;
  ShelfValue var(bool unbiased = true) const;
  ShelfValue var(const StatisticOptions& options) const;
  ShelfValue std(bool unbiased = true) const;
  ShelfValue std(const StatisticOptions& options) const;
  ShelfValue se(ErrorTarget target = ErrorTarget::Mean,
                 const StatisticOptions& options = {}) const;
  double corr(const Shelf& other, ShelfCorrMethod method = ShelfCorrMethod::Pearson) const;
  ShelfValue covar(const Shelf& other, const StatisticOptions& options = {}) const;
  Shelf percentiles(pluss::msharp::RangeView::Double points = {0.25, 0.5, 0.75},
                     std::int64_t min_count = 1) const;
  Shelf mode(bool drop_nulls = true) const;
  Shelf rank(RankTieRule ties = RankTieRule::Average,
              RankNullRule nulls = RankNullRule::Keep,
              bool ascending = true,
              bool percentage = false) const;
  ShelfValue skew(std::int64_t min_count = 3) const;
  ShelfValue kurtosis(std::int64_t min_count = 4) const;
  Shelf scan(ScanStatistic statistic, bool skip_nulls = true) const;
  Shelf cumsum(bool skip_nulls = true) const;
  Shelf cumprod(bool skip_nulls = true) const;
  Shelf cummin(bool skip_nulls = true) const;
  Shelf cummax(bool skip_nulls = true) const;
  ShelfValue corr_with(const Shelf& other,
                        ShelfCorrMethod method = ShelfCorrMethod::Pearson,
                        std::int64_t min_count = 1) const;
  ShelfValue matmul(const Shelf& other) const;
  ShelfValue ndistinct(bool dropnull = true) const;
  Shelf duplicated(bool keep_first = true, bool keep_last = false) const;
  Shelf repeated() const;

  Shelf weigh(pluss::msharp::RangeView::Double weights,
               WindowStatistic statistic = WindowStatistic::Mean,
               std::int64_t min_count = 1) const;
  Shelf bounded(const WindowBoundsFn& bounds,
                 WindowStatistic statistic = WindowStatistic::Mean,
                 const WindowOptions& options = {}) const;

  Shelf map(const std::function<ShelfValue(const ShelfValue&)>& fn) const;
  Shelf transform(const std::function<ShelfValue(const ShelfValue&)>& fn) const;
  Shelf transform(const std::function<Shelf(const Shelf&)>& fn,
                   ResultShape shape = ResultShape::Preserve) const;
  Shelf fold(const std::function<Shelf(const Shelf&)>& fn,
              ResultShape shape = ResultShape::Reduce) const;
  Shelf flow(const std::function<Shelf(const Shelf&)>& fn) const;

  // Note payload operations. Prefer note().method() in public code.
  Shelf strip() const;
  Shelf strip(std::string_view chars) const;
  Shelf lstrip() const;
  Shelf lstrip(std::string_view chars) const;
  Shelf rstrip() const;
  Shelf rstrip(std::string_view chars) const;
  Shelf lower() const;
  Shelf upper() const;
  Shelf casefold() const;
  Shelf titlecase() const;
  Shelf length() const;
  Shelf starts(std::string_view prefix) const;
  Shelf ends(std::string_view suffix) const;
  Shelf matches(std::string_view pattern) const;
  Shelf fullmatch(std::string_view pattern) const;
  DataFrame extract(std::string_view pattern,
                    pluss::msharp::RangeView::Of<std::string_view> names = {}) const;
  DataFrame split(std::string_view delimiter,
                  std::int64_t limit = -1,
                  std::string prefix = "part") const;
  DataFrame split(std::string_view pattern,
                  const TextOptions& options) const;
  DataFrame partition(std::string_view separator,
                      std::string prefix = "part") const;
  Shelf pad(std::int64_t width,
             TextAlign align = TextAlign::Left,
             char fill = ' ') const;
  Shelf chars(std::int64_t start,
               std::optional<std::int64_t> stop = std::nullopt,
               std::int64_t step = 1) const;
  Shelf normalize(UnicodeForm form = UnicodeForm::NFC) const;
  Shelf encode(std::string_view encoding = "base64") const;
  DataFrame indicate(std::string_view delimiter = "|",
                     std::string prefix = "") const;
  Shelf replace(std::string_view from, std::string_view to) const;
  Shelf replace(std::string_view from,
                 std::string_view to,
                 const TextOptions& options) const;
  Shelf put(const Shelf& bool_mask, const Shelf& value) const;
  Shelf put(const Shelf& bool_mask, double value) const;
  Shelf put(const Shelf& bool_mask, std::string_view value) const;
  Shelf put(const Shelf& bool_mask, const char* value) const {
    return put(bool_mask, std::string_view{value != nullptr ? value : ""});
  }
  Shelf put(const Shelf& bool_mask, pluss::null_t value) const;
  Shelf put(const pluss::Series& bool_mask, const Shelf& value) const {
    return put(Shelf(bool_mask), value);
  }
  Shelf put(const pluss::Series& bool_mask, double value) const {
    return put(Shelf(bool_mask), value);
  }
  Shelf put(const pluss::Series& bool_mask, std::string_view value) const {
    return put(Shelf(bool_mask), value);
  }
  Shelf put(const pluss::Series& bool_mask, const char* value) const {
    return put(Shelf(bool_mask), value);
  }
  Shelf put(const pluss::Series& bool_mask, pluss::null_t value) const {
    return put(Shelf(bool_mask), value);
  }
  Shelf replace(double from, double to) const;
  Shelf replace(double from, pluss::null_t to) const;
  Shelf replace(pluss::null_t from, double to) const;
  Shelf replace(const pluss::Series& from, const pluss::Series& to) const;
  Shelf replace(const pluss::Series& from, pluss::null_t to) const;
  Shelf replace(pluss::null_t from, const pluss::Series& to) const;
  Shelf replace(const pluss::Note& from, const pluss::Note& to) const;
  Shelf replace(const pluss::Note& from, pluss::null_t to) const;
  Shelf replace(pluss::null_t from, const pluss::Note& to) const;
  Shelf replace(const pluss::Timeline& from, const pluss::Timeline& to) const;
  Shelf replace(const pluss::Timeline& from, pluss::null_t to) const;
  Shelf replace(pluss::null_t from, const pluss::Timeline& to) const;
  Shelf replace(const pluss::Duration& from, const pluss::Duration& to) const;
  Shelf replace(const pluss::Duration& from, pluss::null_t to) const;
  Shelf replace(pluss::null_t from, const pluss::Duration& to) const;
  static Shelf select(pluss::msharp::RangeView::Of<Shelf> conditions,
                       pluss::msharp::RangeView::Of<Shelf> choices,
                       const Shelf& default_value);
  static Shelf select(pluss::msharp::RangeView::Of<Shelf> conditions,
                       pluss::msharp::RangeView::Of<Shelf> choices,
                       double default_value);
  static Shelf select(pluss::msharp::RangeView::Of<Shelf> conditions,
                       pluss::msharp::RangeView::Of<Shelf> choices,
                       std::string_view default_value);
  static Shelf select(pluss::msharp::RangeView::Of<Shelf> conditions,
                       pluss::msharp::RangeView::Of<Shelf> choices,
                       const char* default_value) {
    return select(
        conditions,
        choices,
        std::string_view{default_value != nullptr ? default_value : ""});
  }
  static Shelf select(pluss::msharp::RangeView::Of<Shelf> conditions,
                       pluss::msharp::RangeView::Of<Shelf> choices,
                       const pluss::Timeline& default_value);
  static Shelf select(pluss::msharp::RangeView::Of<Shelf> conditions,
                       pluss::msharp::RangeView::Of<Shelf> choices,
                       pluss::null_t default_value);
  static Shelf select(std::initializer_list<Shelf> conditions,
                       std::initializer_list<Shelf> choices,
                       const Shelf& default_value) {
    return select(
        pluss::msharp::RangeView::Of<Shelf>(conditions),
        pluss::msharp::RangeView::Of<Shelf>(choices),
        default_value);
  }
  static Shelf select(std::initializer_list<Shelf> conditions,
                       std::initializer_list<Shelf> choices,
                       double default_value) {
    return select(
        pluss::msharp::RangeView::Of<Shelf>(conditions),
        pluss::msharp::RangeView::Of<Shelf>(choices),
        default_value);
  }
  static Shelf select(std::initializer_list<Shelf> conditions,
                       std::initializer_list<Shelf> choices,
                       std::string_view default_value) {
    return select(
        pluss::msharp::RangeView::Of<Shelf>(conditions),
        pluss::msharp::RangeView::Of<Shelf>(choices),
        default_value);
  }
  static Shelf select(std::initializer_list<Shelf> conditions,
                       std::initializer_list<Shelf> choices,
                       const char* default_value) {
    return select(
        pluss::msharp::RangeView::Of<Shelf>(conditions),
        pluss::msharp::RangeView::Of<Shelf>(choices),
        default_value);
  }
  static Shelf select(std::initializer_list<Shelf> conditions,
                       std::initializer_list<Shelf> choices,
                       const pluss::Timeline& default_value) {
    return select(
        pluss::msharp::RangeView::Of<Shelf>(conditions),
        pluss::msharp::RangeView::Of<Shelf>(choices),
        default_value);
  }
  static Shelf select(std::initializer_list<Shelf> conditions,
                       std::initializer_list<Shelf> choices,
                       pluss::null_t default_value) {
    return select(
        pluss::msharp::RangeView::Of<Shelf>(conditions),
        pluss::msharp::RangeView::Of<Shelf>(choices),
        default_value);
  }
  Shelf has(double value) const;
  Shelf has(std::string_view value) const;
  Shelf has(const char* value) const { return has(std::string_view{value != nullptr ? value : ""}); }
  Shelf has(const Shelf& values) const;

  Shelf add(double other) const;
  Shelf add(const Shelf& other) const;
  Shelf radd(double other) const;
  Shelf radd(const Shelf& other) const;
  Shelf sub(double other) const;
  Shelf sub(const Shelf& other) const;
  Shelf rsub(double other) const;
  Shelf rsub(const Shelf& other) const;
  Shelf mul(double other) const;
  Shelf mul(const Shelf& other) const;
  Shelf rmul(double other) const;
  Shelf rmul(const Shelf& other) const;
  Shelf div(double other) const;
  Shelf div(const Shelf& other) const;
  Shelf rdiv(double other) const;
  Shelf rdiv(const Shelf& other) const;
  Shelf truediv(double other) const;
  Shelf truediv(const Shelf& other) const;
  Shelf rtruediv(double other) const;
  Shelf rtruediv(const Shelf& other) const;
  Shelf floordiv(double other) const;
  Shelf floordiv(const Shelf& other) const;
  Shelf rfloordiv(double other) const;
  Shelf rfloordiv(const Shelf& other) const;
  Shelf mod(double other) const;
  Shelf mod(const Shelf& other) const;
  Shelf rmod(double other) const;
  Shelf rmod(const Shelf& other) const;
  Shelf pow(double other) const;
  Shelf pow(const Shelf& other) const;
  Shelf rpow(double other) const;
  Shelf rpow(const Shelf& other) const;
  Shelf compare(double other, ShelfBinaryCompareOp op) const;
  Shelf compare(std::string_view other, ShelfBinaryCompareOp op) const;
  Shelf compare(const char* other, ShelfBinaryCompareOp op) const {
    return compare(std::string_view{other != nullptr ? other : ""}, op);
  }
  Shelf compare(const pluss::Series& other, ShelfBinaryCompareOp op) const;
  Shelf compare(const pluss::Note& other, ShelfBinaryCompareOp op) const;
  Shelf compare(const pluss::Timeline& other, ShelfBinaryCompareOp op) const;
  Shelf compare(const pluss::Duration& other, ShelfBinaryCompareOp op) const;
  Shelf compare(const Shelf& other, ShelfBinaryCompareOp op) const;
  Shelf eq(double other) const;
  Shelf eq(std::string_view other) const;
  Shelf eq(const char* other) const { return eq(std::string_view{other != nullptr ? other : ""}); }
  Shelf eq(const Shelf& other) const;
  Shelf ne(double other) const;
  Shelf ne(std::string_view other) const;
  Shelf ne(const char* other) const { return ne(std::string_view{other != nullptr ? other : ""}); }
  Shelf ne(const Shelf& other) const;
  Shelf lt(double other) const;
  Shelf lt(std::string_view other) const;
  Shelf lt(const char* other) const { return lt(std::string_view{other != nullptr ? other : ""}); }
  Shelf lt(const Shelf& other) const;
  Shelf le(double other) const;
  Shelf le(std::string_view other) const;
  Shelf le(const char* other) const { return le(std::string_view{other != nullptr ? other : ""}); }
  Shelf le(const Shelf& other) const;
  Shelf gt(double other) const;
  Shelf gt(std::string_view other) const;
  Shelf gt(const char* other) const { return gt(std::string_view{other != nullptr ? other : ""}); }
  Shelf gt(const Shelf& other) const;
  Shelf ge(double other) const;
  Shelf ge(std::string_view other) const;
  Shelf ge(const char* other) const { return ge(std::string_view{other != nullptr ? other : ""}); }
  Shelf ge(const Shelf& other) const;
  Shelf land(const Shelf& other) const;
  Shelf rand(const Shelf& other) const;
  Shelf lor(const Shelf& other) const;
  Shelf ror(const Shelf& other) const;
  Shelf lxor(const Shelf& other) const;
  Shelf rxor(const Shelf& other) const;
  Shelf between(double a, double b, bool inclusive = false) const;
  Shelf between(double a, double b, bool include_left, bool include_right) const;
  Shelf between(std::string_view lo, std::string_view hi, bool inclusive = true) const;
  Shelf between(std::string_view lo, std::string_view hi, bool include_left, bool include_right) const;
  Shelf between(const Shelf& lo, const Shelf& hi, bool inclusive = true) const;
  Shelf between(const Shelf& lo, const Shelf& hi, bool include_left, bool include_right) const;
  Shelf window(std::int64_t window, ShelfWindowOp op, std::int64_t min_periods = 1) const;
  ShelfCastAccessor cast() const;
  ShelfCastAccessor cast(const ShelfCastOptions& options) const;
  Shelf astype(pluss::codem::ScalarType dtype) const;
  ShelfGroupBuilder group() const;
  ShelfGroupBuilder group(const ShelfGroupOptions& options) const;

  std::string to_string(const display::DisplayOptions& options = {}) const;
  std::string info_string() const;
  // A raw Shelf has no show(): its terse column view is rarely what you want.
  // Reach for `framerize(name).show()` for the aligned table view, `describe()`
  // / `view()` for a ShelfView you can `.show()`, or `to_string()` for a
  // string. `operator<<` still prints a Shelf.
  void info() const;
  void info(std::ostream& os) const;
  ShelfPlotAccessor plot() const;
  ShelfPlotAccessor plot(const DataFramePlotOptions& options) const;
  ShelfLabelAccessor label() const;
  Shelf align(const Shelf& target_labels, const AlignOptions& options = {}) const;
  bool named() const;
  std::string name() const;
  Shelf name(std::string value) const;
  Shelf unname() const;

  const pluss::Series& as_series() const;
  const pluss::Note& as_note() const;
  const pluss::Timeline& as_timeline() const;
  const pluss::Duration& as_duration() const;

  TimeSeriesAccessor ts() const;

private:
  friend class Atlas;
  friend class DataFrame;
  friend class DataFrameView;
  friend class ShelfCastAccessor;
  friend class ShelfGroupBuilder;
  friend class ShelfGrouped;
  friend class ShelfNoteAccessor;
  friend class ShelfTimelineAccessor;
  friend class ShelfLabelAccessor;
  friend class ShelfValue;
  friend class ShelfView;
  friend class TimeSeriesAccessor;
  friend bool detail::has_validity_mask(const Shelf& shelf);
  friend const std::optional<pluss::Series>& detail::validity_mask(const Shelf& shelf);
  friend Shelf detail::with_validity_mask(const Shelf& shelf, const pluss::Series& validity_mask);
  friend Shelf& detail::with_validity_mask_(Shelf& shelf, const pluss::Series& validity_mask);
  friend Shelf detail::clear_validity_mask(const Shelf& shelf);
  friend Shelf& detail::clear_validity_mask_(Shelf& shelf);
  friend bool detail::has_display_labels(const Shelf& shelf);
  friend Shelf detail::display_labels(const Shelf& shelf);
  friend Shelf detail::with_display_labels(const Shelf& shelf, const Shelf& labels);
  friend Shelf& detail::with_display_labels_(Shelf& shelf, const Shelf& labels);
  friend Shelf detail::clear_display_labels(const Shelf& shelf);
  friend Shelf& detail::clear_display_labels_(Shelf& shelf);
  friend LabelKind detail::label_kind(const Shelf& shelf);
  friend Shelf detail::with_label_kind(const Shelf& shelf, LabelKind kind);
  friend Shelf& detail::with_label_kind_(Shelf& shelf, LabelKind kind);
  friend bool detail::valid_at(const Shelf& shelf, std::int64_t row);
  friend int detail::compare_rows(const Shelf& shelf,
                                  std::int64_t lhs_row,
                                  std::int64_t rhs_row,
                                  bool nulls_last);
  friend bool detail::same_payload(const Shelf& left, const Shelf& right) noexcept;
  friend Shelf detail::math(const Shelf& shelf,
                             double other,
                             ShelfBinaryMathOp op,
                             bool reverse);
  friend Shelf detail::math(const Shelf& left, const Shelf& right, ShelfBinaryMathOp op);

  explicit Shelf(pluss::msharp::intrusive_ptr<detail::ShelfImpl> impl);
  Shelf cast_(ShelfCastTarget target, const ShelfCastOptions& options = {}) const;
  Shelf cast_(ShelfCastTarget target,
               std::optional<pluss::codem::ScalarType> series_dtype) const;

  bool unique() const noexcept;
  bool has_validity_mask() const;
  const std::optional<pluss::Series>& validity_mask() const;
  Shelf with_validity_mask(const pluss::Series& validity_mask) const;
  Shelf& with_validity_mask_(const pluss::Series& validity_mask);
  Shelf clear_validity_mask() const;
  Shelf& clear_validity_mask_();
  bool has_display_labels() const;
  Shelf display_labels() const;
  Shelf with_display_labels(const Shelf& labels) const;
  Shelf& with_display_labels_(const Shelf& labels);
  Shelf clear_display_labels() const;
  Shelf& clear_display_labels_();
  bool has_caselbl() const;
  Shelf caselbl() const;
  Shelf caselbl(const Shelf& labels) const;
  Shelf& caselbl_(const Shelf& labels);
  Shelf rangelbl() const;
  Shelf& rangelbl_();
  Shelf clear_caselbl() const;
  Shelf& clear_caselbl_();

  bool is_valid(std::int64_t row) const;
  int compare_rows(std::int64_t lhs_row, std::int64_t rhs_row, bool nulls_last = true) const;
  Shelf year() const;
  Shelf month() const;
  Shelf day() const;
  Shelf hour() const;
  bool same_payload_(const Shelf& other) const noexcept;
  Shelf math_(double other, ShelfBinaryMathOp op, bool reverse = false) const;
  Shelf math_(const Shelf& other, ShelfBinaryMathOp op) const;
private:
  Shelf sample_fraction_(double frac,
                          std::optional<std::uint64_t> random_state,
                          bool replace) const;
  Shelf moving(std::int64_t window,
                WindowStatistic statistic = WindowStatistic::Mean,
                const WindowOptions& options = {}) const;
  Shelf growing(WindowStatistic statistic,
                 const StatisticOptions& options = {}) const;
  Shelf decay(double alpha,
               WindowStatistic statistic = WindowStatistic::Mean,
               bool adjusted = true,
               bool bias_corrected = true,
               std::int64_t min_count = 1,
               bool ignore_nulls = false) const;
  Shelf timed(const Shelf& axis,
               std::int64_t duration_nanos,
               TimeWindowStatistic statistic = TimeWindowStatistic::Mean,
               const StatisticOptions& options = {},
               WindowClosure closure = WindowClosure::Right) const;
  Shelf grouped(const Shelf& groups,
                 std::int64_t window,
                 WindowStatistic statistic = WindowStatistic::Mean,
                 const WindowOptions& options = {}) const;

  detail::ShelfImpl& mut_impl_();
  const detail::ShelfImpl& impl_ref_() const;

  pluss::msharp::intrusive_ptr<detail::ShelfImpl> impl_{nullptr};
};

namespace detail {
inline bool has_validity_mask(const Shelf& shelf) {
  return shelf.has_validity_mask();
}

inline const std::optional<pluss::Series>& validity_mask(const Shelf& shelf) {
  return shelf.validity_mask();
}

inline Shelf with_validity_mask(const Shelf& shelf, const pluss::Series& validity_mask) {
  return shelf.with_validity_mask(validity_mask);
}

inline Shelf& with_validity_mask_(Shelf& shelf, const pluss::Series& validity_mask) {
  return shelf.with_validity_mask_(validity_mask);
}

inline Shelf clear_validity_mask(const Shelf& shelf) {
  return shelf.clear_validity_mask();
}

inline Shelf& clear_validity_mask_(Shelf& shelf) {
  return shelf.clear_validity_mask_();
}

inline bool has_display_labels(const Shelf& shelf) {
  return shelf.has_display_labels();
}

inline Shelf display_labels(const Shelf& shelf) {
  return shelf.display_labels();
}

inline Shelf with_display_labels(const Shelf& shelf, const Shelf& labels) {
  return shelf.with_display_labels(labels);
}

inline Shelf& with_display_labels_(Shelf& shelf, const Shelf& labels) {
  return shelf.with_display_labels_(labels);
}

inline Shelf clear_display_labels(const Shelf& shelf) {
  return shelf.clear_display_labels();
}

inline Shelf& clear_display_labels_(Shelf& shelf) {
  return shelf.clear_display_labels_();
}

inline bool valid_at(const Shelf& shelf, std::int64_t row) {
  return shelf.is_valid(row);
}

inline int compare_rows(const Shelf& shelf,
                        std::int64_t lhs_row,
                        std::int64_t rhs_row,
                        bool nulls_last) {
  return shelf.compare_rows(lhs_row, rhs_row, nulls_last);
}

inline bool same_payload(const Shelf& left, const Shelf& right) noexcept {
  return left.same_payload_(right);
}

inline Shelf math(const Shelf& shelf,
                   double other,
                   ShelfBinaryMathOp op,
                   bool reverse) {
  return shelf.math_(other, op, reverse);
}

inline Shelf math(const Shelf& left, const Shelf& right, ShelfBinaryMathOp op) {
  return left.math_(right, op);
}

struct GroupStateCache {
  std::vector<Shelf> keys{};
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
  Shelf labels{};
  Shelf values{};
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
  explicit TimeSeriesAccessor(Shelf base);

  Shelf returns(std::int64_t periods = 1) const;
  Shelf log_returns(std::int64_t periods = 1) const;
  Shelf diff(std::int64_t periods = 1) const;
  Shelf lag(std::int64_t periods = 1) const;
  Shelf lead(std::int64_t periods = 1) const;
  Shelf rolling(std::int64_t window,
                 WindowStatistic statistic = WindowStatistic::Mean,
                 const WindowOptions& options = {}) const;
  Shelf rolling(const Shelf& axis,
                 std::string_view duration,
                 WindowStatistic statistic = WindowStatistic::Mean,
                 const StatisticOptions& options = {},
                 WindowClosure closure = WindowClosure::Right) const;
  Shelf rolling(const Shelf& axis,
                 const Shelf& groups,
                 std::string_view duration,
                 WindowStatistic statistic = WindowStatistic::Mean,
                 const StatisticOptions& options = {},
                 WindowClosure closure = WindowClosure::Right) const;
  Shelf timed(const Shelf& axis,
               std::int64_t duration_nanos,
               TimeWindowStatistic statistic = TimeWindowStatistic::Mean,
               const StatisticOptions& options = {},
               WindowClosure closure = WindowClosure::Right) const;
  Shelf grouped(const Shelf& groups,
                 std::int64_t window,
                 WindowStatistic statistic = WindowStatistic::Mean,
                 const WindowOptions& options = {}) const;
  Shelf expanding(WindowStatistic statistic = WindowStatistic::Mean,
                   const StatisticOptions& options = {}) const;
  Shelf ewm(double alpha,
             WindowStatistic statistic = WindowStatistic::Mean,
             bool adjusted = true,
             bool bias_corrected = true,
             std::int64_t min_count = 1,
             bool ignore_nulls = false) const;
  Shelf zscore_rolling(std::int64_t window,
                        std::int64_t min_periods = 1,
                        double epsilon = 1e-12) const;
  Shelf fill_gaps(pluss::subsp::stats::ts::GapFillMode mode =
                       pluss::subsp::stats::ts::GapFillMode::Forward,
                   std::optional<double> constant = std::nullopt) const;
  Shelf align_to(const Shelf& source_axis,
                  const Shelf& target_axis,
                  TsAlignMode mode = TsAlignMode::Exact) const;
  Shelf align_to(const Shelf& source_axis,
                  const Shelf& target_axis,
                  TsAlignMode mode,
                  pluss::subsp::stats::ts::GapFillMode fill_mode,
                  std::optional<double> constant = std::nullopt) const;
  DataFrame lag_block(pluss::msharp::RangeView::Int lags) const;
  DataFrame rolling_block(pluss::msharp::RangeView::Int windows,
                          const TsRollingBlockOptions& options = {}) const;
  TimeSeriesResampler resample(const Shelf& axis,
                               std::string_view freq,
                               std::int64_t step = 1) const;
  TimeSeriesResampler resample(const Shelf& axis,
                               const pluss::Timeline::ResampleOptions& options) const;
  TimeSeriesResample resample(const Shelf& axis,
                              std::string_view freq,
                              pluss::subsp::stats::ts::ResampleOp op,
                              std::int64_t step = 1) const;
  TimeSeriesResample resample(
      const Shelf& axis,
      const pluss::Timeline::ResampleOptions& options,
      pluss::subsp::stats::ts::ResampleOp op) const;
  Shelf add(const Shelf& other,
             const Shelf& left_axis,
             const Shelf& right_axis,
             TsAlignMode mode = TsAlignMode::Exact) const;
  Shelf sub(const Shelf& other,
             const Shelf& left_axis,
             const Shelf& right_axis,
             TsAlignMode mode = TsAlignMode::Exact) const;
  Shelf mul(const Shelf& other,
             const Shelf& left_axis,
             const Shelf& right_axis,
             TsAlignMode mode = TsAlignMode::Exact) const;
  Shelf div(const Shelf& other,
             const Shelf& left_axis,
             const Shelf& right_axis,
             TsAlignMode mode = TsAlignMode::Exact) const;
  Shelf eq(const Shelf& other,
            const Shelf& left_axis,
            const Shelf& right_axis,
            TsAlignMode mode = TsAlignMode::Exact) const;
  Shelf ne(const Shelf& other,
            const Shelf& left_axis,
            const Shelf& right_axis,
            TsAlignMode mode = TsAlignMode::Exact) const;
  Shelf lt(const Shelf& other,
            const Shelf& left_axis,
            const Shelf& right_axis,
            TsAlignMode mode = TsAlignMode::Exact) const;
  Shelf le(const Shelf& other,
            const Shelf& left_axis,
            const Shelf& right_axis,
            TsAlignMode mode = TsAlignMode::Exact) const;
  Shelf gt(const Shelf& other,
            const Shelf& left_axis,
            const Shelf& right_axis,
            TsAlignMode mode = TsAlignMode::Exact) const;
  Shelf ge(const Shelf& other,
            const Shelf& left_axis,
            const Shelf& right_axis,
            TsAlignMode mode = TsAlignMode::Exact) const;
  DataFrame calendar(const Shelf& axis) const;
  Shelf sort(const Shelf& axis,
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
  Shelf align_index_(
      const Shelf& other,
      const Shelf& left_axis,
      const Shelf& right_axis,
      TsAlignMode mode,
      const pluss::Timeline::TimelineIndexAccessor* left_index,
      const pluss::Timeline::TimelineIndexAccessor* right_index) const;
  Shelf base_{};
};

class TimeSeriesResampler {
 public:
  TimeSeriesResampler(Shelf base,
                      Shelf axis,
                      pluss::Timeline::ResampleOptions options);

  Shelf mean() const;
  Shelf sum() const;
  Shelf min() const;
  Shelf max() const;
  Shelf first() const;
  Shelf last() const;
  Shelf count() const;

  Shelf asfreq(std::optional<double> fill_value = std::nullopt) const;
  Shelf ffill() const;
  Shelf bfill() const;
  Shelf nearest() const;

 private:
  Shelf aggregate_(pluss::subsp::stats::ts::ResampleOp op) const;

  Shelf base_{};
  Shelf axis_{};
  pluss::Timeline::ResampleOptions options_{};
};

class ShelfLabelAccessor {
public:
  explicit ShelfLabelAccessor(Shelf base);

  Shelf values() const;
  bool has() const;
  Shelf set(const Shelf& labels) const;
  Shelf range() const;
  Shelf reset() const;
  Shelf align(const Shelf& target_labels, const AlignOptions& options = {}) const;
  Shelf unite(const Shelf& other) const;
  Shelf intersect(const Shelf& other) const;
  Shelf diff(const Shelf& other) const;
  Shelf symdiff(const Shelf& other) const;
  Shelf find(const Shelf& targets) const;
  Shelf pos(double value) const;
  Shelf pos(std::string_view value) const;
  Shelf pos(const char* value) const {
    return pos(std::string_view{value != nullptr ? value : ""});
  }
  Shelf slice(std::string_view start, std::string_view stop) const;
  Shelf fillnull(double value) const;
  Shelf fillnull(std::string_view value) const;
  Shelf fillnull(const char* value) const {
    return fillnull(std::string_view{value != nullptr ? value : ""});
  }
  Shelf fillnull(const pluss::Timeline& value) const;
  Shelf fillnull(const pluss::Duration& value) const;
  Shelf duplicated(bool keep_first = true, bool keep_last = false) const;
  Shelf repeated() const;
  Shelf distinct(bool dropnull = true) const;
  Shelf freq(std::int64_t top_n = 0, bool dropnull = true) const;
  Shelf has(const Shelf& values) const;
  Shelf isnull() const;
  Shelf notnull() const;
  Shelf map(const std::function<ShelfValue(const ShelfValue&)>& fn) const;
  Shelf take(pluss::msharp::RangeView::Int indices) const;
  Shelf slice(std::int64_t start, std::int64_t stop) const;
  Shelf sort(bool ascending = true, bool nulls_last = true) const;
  bool monotonic(bool ascending = true, bool strict = false, bool nulls_last = true) const;

private:
  Shelf base_{};
};

class ShelfSortBuilder {
public:
  explicit ShelfSortBuilder(Shelf base, bool ascending = true, bool nulls_last = true);

  Shelf asc() const;
  Shelf desc() const;
  Shelf done() const;
  operator Shelf() const;

private:
  Shelf base_{};
  bool ascending_{true};
  bool nulls_last_{true};
};

class ShelfView final : public Shelf {
 public:
  ShelfView();
  explicit ShelfView(Shelf shelf,
                      bool auto_render_terminal = false,
                      std::string operation = {});
  ShelfView(const ShelfView& other);
  ShelfView(ShelfView&& other) noexcept;
  ShelfView& operator=(const ShelfView& other);
  ShelfView& operator=(ShelfView&& other) noexcept;
  ~ShelfView();

  void show(const display::DisplayOptions& options = {}) const;
  void show(std::ostream& os, const display::DisplayOptions& options = {}) const;

 private:
  struct State;
  std::shared_ptr<State> state_{};

  void consume_pending_display_() const;
  void maybe_autorender_() const;
  std::string resolved_window_title_() const;

  friend void output_render(std::ostream& os, const ShelfView& shelf);
  friend std::ostream& operator<<(std::ostream& os, const ShelfView& shelf);
};

class ShelfCastAccessor {
public:
  explicit ShelfCastAccessor(Shelf base, ShelfCastOptions options = {});

  Shelf series() const;
  Shelf note() const;
  Shelf timeline() const;
  Shelf duration() const;

private:
  Shelf base_{};
  ShelfCastOptions options_{};
};

class ShelfGroupBuilder {
public:
  explicit ShelfGroupBuilder(Shelf value, ShelfGroupOptions options = {});

  ShelfGrouped by(const Shelf& key) const;
  ShelfGrouped by(std::initializer_list<Shelf> keys) const;
  ShelfGrouped by(pluss::msharp::RangeView::Of<Shelf> keys) const;

private:
  Shelf value_{};
  ShelfGroupOptions options_{};
};

class ShelfGrouped {
public:
  ShelfGrouped(Shelf value,
                std::vector<Shelf> keys,
                std::vector<std::string> names,
                ShelfGroupOptions options = {});

  DataFrame agg(pluss::msharp::RangeView::Of<ShelfGroupSpec> specs) const;
  DataFrame agg(std::initializer_list<ShelfGroupSpec> specs) const;
  Shelf transform(GroupOp op) const;
  DataFrame transform(pluss::msharp::RangeView::Of<ShelfGroupSpec> specs) const;
  DataFrame transform(std::initializer_list<ShelfGroupSpec> specs) const;
  Shelf filter(const pluss::Series& keep) const;
  Shelf filter(const Shelf& keep) const;

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

  Shelf value_{};
  std::vector<Shelf> keys_{};
  std::vector<std::string> names_{};
  ShelfGroupOptions options_{};
  mutable std::shared_ptr<const detail::GroupStateCache> state_cache_{};
};

class ShelfNoteAccessor {
public:
  explicit ShelfNoteAccessor(Shelf base);

  Shelf strip() const;
  Shelf strip(std::string_view chars) const;
  Shelf lstrip() const;
  Shelf lstrip(std::string_view chars) const;
  Shelf rstrip() const;
  Shelf rstrip(std::string_view chars) const;
  Shelf lower() const;
  Shelf upper() const;
  Shelf casefold() const;
  Shelf titlecase() const;
  Shelf length() const;
  Shelf starts(std::string_view prefix) const;
  Shelf ends(std::string_view suffix) const;
  Shelf has(std::string_view needle,
             const TextOptions& options = {}) const;
  Shelf count(std::string_view pattern) const;
  Shelf count(std::string_view pattern,
               const TextOptions& options) const;
  Shelf matches(std::string_view pattern, bool full = false) const;
  Shelf fullmatch(std::string_view pattern) const;
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
  Shelf pad(std::int64_t width,
             TextAlign align = TextAlign::Left,
             char fill = ' ') const;
  Shelf chars(std::int64_t start,
               std::optional<std::int64_t> stop = std::nullopt,
               std::int64_t step = 1) const;
  Shelf normalize(UnicodeForm form = UnicodeForm::NFC) const;
  Shelf encode(std::string_view encoding = "base64") const;
  DataFrame indicate(std::string_view delimiter = "|",
                     std::string prefix = "") const;
  Shelf replace(std::string_view from, std::string_view to) const;
  Shelf replace(std::string_view from,
                 std::string_view to,
                 const TextOptions& options) const;

private:
  Shelf base_{};
};

class ShelfTimelineAccessor {
public:
  explicit ShelfTimelineAccessor(Shelf base);

  Shelf isnat() const;
  Shelf valid() const;
  Shelf year() const;
  Shelf month() const;
  Shelf day() const;
  Shelf hour() const;
  Shelf minute() const;
  Shelf second() const;
  Shelf weekday() const;
  Shelf yearday() const;
  Shelf quarter() const;
  Shelf week() const;
  Shelf weekend() const;
  Shelf month_start() const;
  Shelf month_end() const;
  Shelf leap() const;
  Shelf weekday_name() const;
  Shelf month_name() const;
  Shelf fmt(std::string_view pattern) const;
  Shelf iso() const;
  Shelf epoch(TimeUnit unit = TimeUnit::Nanos) const;
  Shelf span(const Shelf& other, const TimelineSpanOptions& options = {}) const;
  Shelf span(const Shelf& other, TimeUnit unit) const;
  Shelf span(const pluss::Timeline& other, const TimelineSpanOptions& options = {}) const;
  Shelf span(const pluss::Timeline& other, TimeUnit unit) const;
  Shelf floor(std::string_view unit) const;
  Shelf ceil(std::string_view unit) const;
  Shelf round(std::string_view unit) const;
  Shelf date() const;
  Shelf time() const;
  Shelf shift(std::int64_t periods, std::string_view freq) const;
  Shelf period(std::string_view freq) const;
  Shelf tz(std::string_view name) const;
  Shelf localize(const TimelineLocalizeOptions& options = {}) const;
  Shelf utc() const;

private:
  Shelf base_{};
};

Shelf operator+(const Shelf& lhs, double rhs);
Shelf operator+(double lhs, const Shelf& rhs);
Shelf operator+(const Shelf& lhs, const Shelf& rhs);
Shelf operator-(const Shelf& lhs, double rhs);
Shelf operator-(double lhs, const Shelf& rhs);
Shelf operator-(const Shelf& lhs, const Shelf& rhs);
Shelf operator*(const Shelf& lhs, double rhs);
Shelf operator*(double lhs, const Shelf& rhs);
Shelf operator*(const Shelf& lhs, const Shelf& rhs);
Shelf operator/(const Shelf& lhs, double rhs);
Shelf operator/(double lhs, const Shelf& rhs);
Shelf operator/(const Shelf& lhs, const Shelf& rhs);
Shelf operator%(const Shelf& lhs, double rhs);
Shelf operator%(double lhs, const Shelf& rhs);
Shelf operator%(const Shelf& lhs, const Shelf& rhs);
Shelf operator&(const Shelf& lhs, const Shelf& rhs);
Shelf operator|(const Shelf& lhs, const Shelf& rhs);
Shelf operator^(const Shelf& lhs, const Shelf& rhs);
Shelf operator==(const Shelf& lhs, double rhs);
Shelf operator==(double lhs, const Shelf& rhs);
Shelf operator==(const Shelf& lhs, std::string_view rhs);
Shelf operator==(std::string_view lhs, const Shelf& rhs);
Shelf operator==(const Shelf& lhs, const char* rhs);
Shelf operator==(const char* lhs, const Shelf& rhs);
Shelf operator==(const Shelf& lhs, const pluss::Series& rhs);
Shelf operator==(const Shelf& lhs, const pluss::Note& rhs);
Shelf operator==(const Shelf& lhs, const pluss::Timeline& rhs);
Shelf operator==(const Shelf& lhs, const Shelf& rhs);
Shelf operator!=(const Shelf& lhs, double rhs);
Shelf operator!=(double lhs, const Shelf& rhs);
Shelf operator!=(const Shelf& lhs, std::string_view rhs);
Shelf operator!=(std::string_view lhs, const Shelf& rhs);
Shelf operator!=(const Shelf& lhs, const char* rhs);
Shelf operator!=(const char* lhs, const Shelf& rhs);
Shelf operator!=(const Shelf& lhs, const pluss::Series& rhs);
Shelf operator!=(const Shelf& lhs, const pluss::Note& rhs);
Shelf operator!=(const Shelf& lhs, const pluss::Timeline& rhs);
Shelf operator!=(const Shelf& lhs, const Shelf& rhs);
Shelf operator<(const Shelf& lhs, double rhs);
Shelf operator<(double lhs, const Shelf& rhs);
Shelf operator<(const Shelf& lhs, std::string_view rhs);
Shelf operator<(std::string_view lhs, const Shelf& rhs);
Shelf operator<(const Shelf& lhs, const char* rhs);
Shelf operator<(const char* lhs, const Shelf& rhs);
Shelf operator<(const Shelf& lhs, const pluss::Series& rhs);
Shelf operator<(const Shelf& lhs, const pluss::Note& rhs);
Shelf operator<(const Shelf& lhs, const pluss::Timeline& rhs);
Shelf operator<(const Shelf& lhs, const Shelf& rhs);
Shelf operator<=(const Shelf& lhs, double rhs);
Shelf operator<=(double lhs, const Shelf& rhs);
Shelf operator<=(const Shelf& lhs, std::string_view rhs);
Shelf operator<=(std::string_view lhs, const Shelf& rhs);
Shelf operator<=(const Shelf& lhs, const char* rhs);
Shelf operator<=(const char* lhs, const Shelf& rhs);
Shelf operator<=(const Shelf& lhs, const pluss::Series& rhs);
Shelf operator<=(const Shelf& lhs, const pluss::Note& rhs);
Shelf operator<=(const Shelf& lhs, const pluss::Timeline& rhs);
Shelf operator<=(const Shelf& lhs, const Shelf& rhs);
Shelf operator>(const Shelf& lhs, double rhs);
Shelf operator>(double lhs, const Shelf& rhs);
Shelf operator>(const Shelf& lhs, std::string_view rhs);
Shelf operator>(std::string_view lhs, const Shelf& rhs);
Shelf operator>(const Shelf& lhs, const char* rhs);
Shelf operator>(const char* lhs, const Shelf& rhs);
Shelf operator>(const Shelf& lhs, const pluss::Series& rhs);
Shelf operator>(const Shelf& lhs, const pluss::Note& rhs);
Shelf operator>(const Shelf& lhs, const pluss::Timeline& rhs);
Shelf operator>(const Shelf& lhs, const Shelf& rhs);
Shelf operator>=(const Shelf& lhs, double rhs);
Shelf operator>=(double lhs, const Shelf& rhs);
Shelf operator>=(const Shelf& lhs, std::string_view rhs);
Shelf operator>=(std::string_view lhs, const Shelf& rhs);
Shelf operator>=(const Shelf& lhs, const char* rhs);
Shelf operator>=(const char* lhs, const Shelf& rhs);
Shelf operator>=(const Shelf& lhs, const pluss::Series& rhs);
Shelf operator>=(const Shelf& lhs, const pluss::Note& rhs);
Shelf operator>=(const Shelf& lhs, const pluss::Timeline& rhs);
Shelf operator>=(const Shelf& lhs, const Shelf& rhs);

class ShelfValue {
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

  ShelfValue() = default;
  explicit ShelfValue(Shelf shelf);

  bool defined() const noexcept;
  ShelfKind kind() const;
  const Shelf& shelf() const noexcept;
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
  Shelf shelf_{};
};

std::ostream& operator<<(std::ostream& os, const Shelf& shelf);
void output_render(std::ostream& os, const Shelf& shelf);
std::ostream& operator<<(std::ostream& os, const ShelfView& shelf);
void output_render(std::ostream& os, const ShelfView& shelf);
std::ostream& operator<<(std::ostream& os, const ShelfValue& value);
void output_render(std::ostream& os, const ShelfValue& value);
std::ostream& operator<<(std::ostream& os, const ShelfValue::HostAny& value);

} // namespace pluss::table
