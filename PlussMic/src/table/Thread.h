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
namespace pluss::bundle {
class Slot;
}

namespace pluss::table {

class Thread;
enum class LabelKind : std::uint8_t;
enum class ThreadBinaryMathOp : std::uint8_t;
struct DataFramePlotOptions;

namespace detail {
class ThreadImpl;
struct GroupStateCache;
bool has_validity_mask(const Thread& thread);
const std::optional<pluss::Series>& validity_mask(const Thread& thread);
Thread with_validity_mask(const Thread& thread, const pluss::Series& validity_mask);
Thread& with_validity_mask_(Thread& thread, const pluss::Series& validity_mask);
Thread clear_validity_mask(const Thread& thread);
Thread& clear_validity_mask_(Thread& thread);
bool has_display_labels(const Thread& thread);
Thread display_labels(const Thread& thread);
Thread with_display_labels(const Thread& thread, const Thread& labels);
Thread& with_display_labels_(Thread& thread, const Thread& labels);
Thread clear_display_labels(const Thread& thread);
Thread& clear_display_labels_(Thread& thread);
LabelKind label_kind(const Thread& thread);
Thread with_label_kind(const Thread& thread, LabelKind kind);
Thread& with_label_kind_(Thread& thread, LabelKind kind);
bool valid_at(const Thread& thread, std::int64_t row);
int compare_rows(const Thread& thread,
                 std::int64_t lhs_row,
                 std::int64_t rhs_row,
                 bool nulls_last = true);
bool same_payload(const Thread& left, const Thread& right) noexcept;
Thread math(const Thread& thread,
            double other,
            ThreadBinaryMathOp op,
            bool reverse = false);
Thread math(const Thread& left, const Thread& right, ThreadBinaryMathOp op);
Thread freq_count_thread(const Thread& thread,
                         std::int64_t top_n,
                         bool dropnull,
                         std::string_view display_name);
}
class ThreadValue;
class DataFrame;
class DataFrameView;
class Atlas;
class ThreadView;
class ThreadPlotAccessor;
class ThreadCastAccessor;
class ThreadGroupBuilder;
class ThreadGrouped;
class ThreadSortBuilder;
class ThreadNoteAccessor;
class ThreadTimelineAccessor;
class ThreadLabelAccessor;
class TimeSeriesAccessor;
class TimeSeriesResampler;

enum class ThreadKind : std::uint8_t {
  Series = 0,
  Note = 1,
  Timeline = 2,
  TimeIndex = 3,
};

std::string_view thread_kind_name(ThreadKind kind) noexcept;

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

struct ThreadOptions {
  bool rangelbl{false};
};

enum class ThreadBinaryMathOp : std::uint8_t {
  Add = 0,
  Sub = 1,
  Mul = 2,
  Div = 3,
  TrueDiv = 4,
  FloorDiv = 5,
  Mod = 6,
  Pow = 7,
};

enum class ThreadBinaryCompareOp : std::uint8_t {
  Eq = 0,
  Ne = 1,
  Lt = 2,
  Le = 3,
  Gt = 4,
  Ge = 5,
};

enum class ThreadCastTarget : std::uint8_t {
  Series = 0,
  Note = 1,
  Timeline = 2,
  TimeIndex = 3,
};

enum class ParseInvalid : std::uint8_t {
  Raise = 0,
  Null = 1,
};

struct ThreadCastOptions {
  std::optional<pluss::codem::ScalarType> series_dtype{std::nullopt};
  std::string format{};
  ParseInvalid invalid{ParseInvalid::Raise};
};

enum class ThreadWindowOp : std::uint8_t {
  RollingSum = 0,
  RollingMean = 1,
  RollingMin = 2,
  RollingMax = 3,
};

enum class ThreadCorrMethod : std::uint8_t {
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

enum class ThreadFreqKind : std::uint8_t {
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

struct ThreadFreqOptions {
  ThreadFreqKind kind{ThreadFreqKind::Count};
  bool dropnull{true};
  bool sort{true};
  bool ascending{false};
  std::int64_t top{0};
};

struct ThreadGroupOptions {
  bool dropnull{true};
  bool sort{false};
};

struct ThreadGroupSpec {
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

class Thread {
public:
  Thread();
  Thread(const pluss::Series& data, std::optional<pluss::Series> validity_mask = std::nullopt);
  Thread(const pluss::Series& data, const ThreadOptions& options);
  Thread(const pluss::Series& data,
         std::optional<pluss::Series> validity_mask,
         const ThreadOptions& options);
  Thread(const pluss::Note& data, std::optional<pluss::Series> validity_mask = std::nullopt);
  Thread(const pluss::Note& data, const ThreadOptions& options);
  Thread(const pluss::Note& data,
         std::optional<pluss::Series> validity_mask,
         const ThreadOptions& options);
  Thread(const pluss::Timeline& data, std::optional<pluss::Series> validity_mask = std::nullopt);
  Thread(const pluss::Timeline& data, const ThreadOptions& options);
  Thread(const pluss::Timeline& data,
         std::optional<pluss::Series> validity_mask,
         const ThreadOptions& options);
  Thread(const pluss::TimeIndex& data, std::optional<pluss::Series> validity_mask = std::nullopt);
  Thread(const pluss::TimeIndex& data, const ThreadOptions& options);
  Thread(const pluss::TimeIndex& data,
         std::optional<pluss::Series> validity_mask,
         const ThreadOptions& options);
  Thread(const Thread& other);
  Thread(Thread&& other) noexcept;
  Thread& operator=(const Thread& other);
  Thread& operator=(Thread&& other) noexcept;
  ~Thread();

  bool defined() const noexcept;
  std::int64_t size() const;
  std::int64_t ncases() const;
  std::pair<std::int64_t, std::int64_t> shape() const;
  ThreadKind kind() const;
  std::string ptype() const;
  std::string dtype() const;
  ThreadValue value() const;

  // 1D counterparts to DataFrame::kindelize()/bundelize() and
  // Atlas::kindelize()/bundelize().
  pluss::kindle::Stream kindelize() const;
  pluss::bundle::Slot bundelize() const;

  Thread isnull() const;
  Thread notnull() const;
  Thread isnan() const;
  Thread isfinite() const;
  Thread invalid() const;

  // Functional-by-default transforms.
  Thread fillnull(double value) const;
  Thread fillnull(std::string_view value) const;
  Thread fillnull(const char* value) const {
    return fillnull(std::string_view{value != nullptr ? value : ""});
  }
  Thread fillnull(const pluss::Timeline& value) const;
  Thread fillnull(const pluss::TimeIndex& value) const;
  Thread abs() const;
  Thread clip(double min_value, double max_value) const;
  Thread round(std::int64_t decimals = 0) const;
  Thread log() const;
  Thread exp() const;

  // Explicit mutating transforms (copy-on-write when shared).
  Thread& fillnull_(double value);
  Thread& fillnull_(std::string_view value);
  Thread& fillnull_(const char* value) {
    return fillnull_(std::string_view{value != nullptr ? value : ""});
  }
  Thread& fillnull_(const pluss::Timeline& value);
  Thread& fillnull_(const pluss::TimeIndex& value);

  // 1D row-space transforms.
  ThreadView head(std::int64_t n = 5, bool render = false) const;
  ThreadView tail(std::int64_t n = 5, bool render = false) const;
  ThreadView view(std::int64_t start, std::int64_t stop, bool render = false) const;
  ThreadView top(std::int64_t n, bool render = false) const;
  ThreadView bottom(std::int64_t n, bool render = false) const;
  Thread append(const Thread& other) const;
  Thread sample(std::int64_t n) const;
  Thread sample(std::int64_t n, std::uint64_t random_state, bool replace = false) const;
  template <class Float>
    requires std::floating_point<Float>
  Thread sample(Float frac) const {
    return sample_fraction_(static_cast<double>(frac), std::nullopt, false);
  }
  template <class Float>
    requires std::floating_point<Float>
  Thread sample(Float frac, std::uint64_t random_state, bool replace = false) const {
    return sample_fraction_(static_cast<double>(frac), random_state, replace);
  }
  Thread shuffle(std::optional<std::uint64_t> random_state = std::nullopt) const;
  Thread shift(std::int64_t periods = 1) const;
  Thread slice(std::int64_t start, std::int64_t stop) const;
  Thread take(std::span<const std::int64_t> indices) const;
  Thread filter(const pluss::Series& bool_mask) const;
  Thread dropnull() const;
  Thread arrange(bool ascending = true, bool nulls_last = true) const;
  ThreadSortBuilder sort(bool ascending = true, bool nulls_last = true) const;
  Thread ffill(std::int64_t limit = -1) const;
  Thread bfill(std::int64_t limit = -1) const;
  Thread interp(std::int64_t limit = -1) const;
  Thread distinct(bool dropnull = true) const;
  Thread freq() const;
  Thread freq(ThreadFreqOptions options) const;
  ThreadView describe(bool render = false) const;
  Thread bucketize(pluss::msharp::RangeView::Double bins,
                   pluss::msharp::RangeView::Int labels = {},
                   bool right = true,
                   bool include_lowest = true) const;

  ThreadNoteAccessor nt() const;
  ThreadTimelineAccessor tl() const;

  // Reductions return scalar payloads where the operation is valid.
  ThreadValue sum() const;
  ThreadValue prod() const;
  ThreadValue count(bool count_true = true) const;
  ThreadValue any(bool skipnull = true) const;
  ThreadValue all(bool skipnull = true) const;
  ThreadValue mean() const;
  ThreadValue median() const;
  ThreadValue min() const;
  ThreadValue max() const;
  ThreadValue csmin() const;
  ThreadValue csmax() const;
  ThreadValue var(bool unbiased = true) const;
  ThreadValue var(const StatisticOptions& options) const;
  ThreadValue std(bool unbiased = true) const;
  ThreadValue std(const StatisticOptions& options) const;
  ThreadValue se(ErrorTarget target = ErrorTarget::Mean,
                 const StatisticOptions& options = {}) const;
  double corr(const Thread& other, ThreadCorrMethod method = ThreadCorrMethod::Pearson) const;
  ThreadValue covar(const Thread& other, const StatisticOptions& options = {}) const;
  Thread percentiles(pluss::msharp::RangeView::Double points = {0.25, 0.5, 0.75},
                     std::int64_t min_count = 1) const;
  Thread mode(bool drop_nulls = true) const;
  Thread rank(RankTieRule ties = RankTieRule::Average,
              RankNullRule nulls = RankNullRule::Keep,
              bool ascending = true,
              bool percentage = false) const;
  ThreadValue skew(std::int64_t min_count = 3) const;
  ThreadValue kurtosis(std::int64_t min_count = 4) const;
  Thread scan(ScanStatistic statistic, bool skip_nulls = true) const;
  Thread cumsum(bool skip_nulls = true) const;
  Thread cumprod(bool skip_nulls = true) const;
  Thread cummin(bool skip_nulls = true) const;
  Thread cummax(bool skip_nulls = true) const;
  ThreadValue corr_with(const Thread& other,
                        ThreadCorrMethod method = ThreadCorrMethod::Pearson,
                        std::int64_t min_count = 1) const;
  ThreadValue matmul(const Thread& other) const;
  ThreadValue ndistinct(bool dropnull = true) const;
  Thread duplicated(bool keep_first = true, bool keep_last = false) const;
  Thread repeated() const;

  Thread weigh(pluss::msharp::RangeView::Double weights,
               WindowStatistic statistic = WindowStatistic::Mean,
               std::int64_t min_count = 1) const;
  Thread bounded(const WindowBoundsFn& bounds,
                 WindowStatistic statistic = WindowStatistic::Mean,
                 const WindowOptions& options = {}) const;

  Thread map(const std::function<ThreadValue(const ThreadValue&)>& fn) const;
  Thread transform(const std::function<ThreadValue(const ThreadValue&)>& fn) const;
  Thread transform(const std::function<Thread(const Thread&)>& fn,
                   ResultShape shape = ResultShape::Reduce) const;
  Thread fold(const std::function<Thread(const Thread&)>& fn,
              ResultShape shape = ResultShape::Reduce) const;
  Thread flow(const std::function<Thread(const Thread&)>& fn) const;

  // Note payload operations. Prefer note().method() in public code.
  Thread strip() const;
  Thread strip(std::string_view chars) const;
  Thread lstrip() const;
  Thread lstrip(std::string_view chars) const;
  Thread rstrip() const;
  Thread rstrip(std::string_view chars) const;
  Thread lower() const;
  Thread upper() const;
  Thread casefold() const;
  Thread titlecase() const;
  Thread length() const;
  Thread starts(std::string_view prefix) const;
  Thread ends(std::string_view suffix) const;
  Thread matches(std::string_view pattern) const;
  Thread fullmatch(std::string_view pattern) const;
  DataFrame extract(std::string_view pattern,
                    pluss::msharp::RangeView::Of<std::string_view> names = {}) const;
  DataFrame split(std::string_view delimiter,
                  std::int64_t limit = -1,
                  std::string prefix = "part") const;
  DataFrame split(std::string_view pattern,
                  const TextOptions& options) const;
  DataFrame partition(std::string_view separator,
                      std::string prefix = "part") const;
  Thread pad(std::int64_t width,
             TextAlign align = TextAlign::Left,
             char fill = ' ') const;
  Thread chars(std::int64_t start,
               std::optional<std::int64_t> stop = std::nullopt,
               std::int64_t step = 1) const;
  Thread normalize(UnicodeForm form = UnicodeForm::NFC) const;
  Thread encode(std::string_view encoding = "base64") const;
  DataFrame indicate(std::string_view delimiter = "|",
                     std::string prefix = "") const;
  Thread replace(std::string_view from, std::string_view to) const;
  Thread replace(std::string_view from,
                 std::string_view to,
                 const TextOptions& options) const;
  Thread put(const Thread& bool_mask, const Thread& value) const;
  Thread put(const Thread& bool_mask, double value) const;
  Thread put(const Thread& bool_mask, std::string_view value) const;
  Thread put(const Thread& bool_mask, const char* value) const {
    return put(bool_mask, std::string_view{value != nullptr ? value : ""});
  }
  Thread put(const Thread& bool_mask, pluss::null_t value) const;
  Thread put(const pluss::Series& bool_mask, const Thread& value) const {
    return put(Thread(bool_mask), value);
  }
  Thread put(const pluss::Series& bool_mask, double value) const {
    return put(Thread(bool_mask), value);
  }
  Thread put(const pluss::Series& bool_mask, std::string_view value) const {
    return put(Thread(bool_mask), value);
  }
  Thread put(const pluss::Series& bool_mask, const char* value) const {
    return put(Thread(bool_mask), value);
  }
  Thread put(const pluss::Series& bool_mask, pluss::null_t value) const {
    return put(Thread(bool_mask), value);
  }
  Thread replace(double from, double to) const;
  Thread replace(double from, pluss::null_t to) const;
  Thread replace(pluss::null_t from, double to) const;
  Thread replace(const pluss::Series& from, const pluss::Series& to) const;
  Thread replace(const pluss::Series& from, pluss::null_t to) const;
  Thread replace(pluss::null_t from, const pluss::Series& to) const;
  Thread replace(const pluss::Note& from, const pluss::Note& to) const;
  Thread replace(const pluss::Note& from, pluss::null_t to) const;
  Thread replace(pluss::null_t from, const pluss::Note& to) const;
  Thread replace(const pluss::Timeline& from, const pluss::Timeline& to) const;
  Thread replace(const pluss::Timeline& from, pluss::null_t to) const;
  Thread replace(pluss::null_t from, const pluss::Timeline& to) const;
  Thread replace(const pluss::TimeIndex& from, const pluss::TimeIndex& to) const;
  Thread replace(const pluss::TimeIndex& from, pluss::null_t to) const;
  Thread replace(pluss::null_t from, const pluss::TimeIndex& to) const;
  static Thread select(pluss::msharp::RangeView::Of<Thread> conditions,
                       pluss::msharp::RangeView::Of<Thread> choices,
                       const Thread& default_value);
  static Thread select(pluss::msharp::RangeView::Of<Thread> conditions,
                       pluss::msharp::RangeView::Of<Thread> choices,
                       double default_value);
  static Thread select(pluss::msharp::RangeView::Of<Thread> conditions,
                       pluss::msharp::RangeView::Of<Thread> choices,
                       std::string_view default_value);
  static Thread select(pluss::msharp::RangeView::Of<Thread> conditions,
                       pluss::msharp::RangeView::Of<Thread> choices,
                       const char* default_value) {
    return select(
        conditions,
        choices,
        std::string_view{default_value != nullptr ? default_value : ""});
  }
  static Thread select(pluss::msharp::RangeView::Of<Thread> conditions,
                       pluss::msharp::RangeView::Of<Thread> choices,
                       const pluss::Timeline& default_value);
  static Thread select(pluss::msharp::RangeView::Of<Thread> conditions,
                       pluss::msharp::RangeView::Of<Thread> choices,
                       const pluss::TimeIndex& default_value);
  static Thread select(pluss::msharp::RangeView::Of<Thread> conditions,
                       pluss::msharp::RangeView::Of<Thread> choices,
                       pluss::null_t default_value);
  static Thread select(std::initializer_list<Thread> conditions,
                       std::initializer_list<Thread> choices,
                       const Thread& default_value) {
    return select(
        pluss::msharp::RangeView::Of<Thread>(conditions),
        pluss::msharp::RangeView::Of<Thread>(choices),
        default_value);
  }
  static Thread select(std::initializer_list<Thread> conditions,
                       std::initializer_list<Thread> choices,
                       double default_value) {
    return select(
        pluss::msharp::RangeView::Of<Thread>(conditions),
        pluss::msharp::RangeView::Of<Thread>(choices),
        default_value);
  }
  static Thread select(std::initializer_list<Thread> conditions,
                       std::initializer_list<Thread> choices,
                       std::string_view default_value) {
    return select(
        pluss::msharp::RangeView::Of<Thread>(conditions),
        pluss::msharp::RangeView::Of<Thread>(choices),
        default_value);
  }
  static Thread select(std::initializer_list<Thread> conditions,
                       std::initializer_list<Thread> choices,
                       const char* default_value) {
    return select(
        pluss::msharp::RangeView::Of<Thread>(conditions),
        pluss::msharp::RangeView::Of<Thread>(choices),
        default_value);
  }
  static Thread select(std::initializer_list<Thread> conditions,
                       std::initializer_list<Thread> choices,
                       const pluss::Timeline& default_value) {
    return select(
        pluss::msharp::RangeView::Of<Thread>(conditions),
        pluss::msharp::RangeView::Of<Thread>(choices),
        default_value);
  }
  static Thread select(std::initializer_list<Thread> conditions,
                       std::initializer_list<Thread> choices,
                       const pluss::TimeIndex& default_value) {
    return select(
        pluss::msharp::RangeView::Of<Thread>(conditions),
        pluss::msharp::RangeView::Of<Thread>(choices),
        default_value);
  }
  static Thread select(std::initializer_list<Thread> conditions,
                       std::initializer_list<Thread> choices,
                       pluss::null_t default_value) {
    return select(
        pluss::msharp::RangeView::Of<Thread>(conditions),
        pluss::msharp::RangeView::Of<Thread>(choices),
        default_value);
  }
  Thread has(double value) const;
  Thread has(std::string_view value) const;
  Thread has(const char* value) const { return has(std::string_view{value != nullptr ? value : ""}); }
  Thread has(const Thread& values) const;

  Thread add(double other) const;
  Thread add(const Thread& other) const;
  Thread radd(double other) const;
  Thread radd(const Thread& other) const;
  Thread sub(double other) const;
  Thread sub(const Thread& other) const;
  Thread rsub(double other) const;
  Thread rsub(const Thread& other) const;
  Thread mul(double other) const;
  Thread mul(const Thread& other) const;
  Thread rmul(double other) const;
  Thread rmul(const Thread& other) const;
  Thread div(double other) const;
  Thread div(const Thread& other) const;
  Thread rdiv(double other) const;
  Thread rdiv(const Thread& other) const;
  Thread truediv(double other) const;
  Thread truediv(const Thread& other) const;
  Thread rtruediv(double other) const;
  Thread rtruediv(const Thread& other) const;
  Thread floordiv(double other) const;
  Thread floordiv(const Thread& other) const;
  Thread rfloordiv(double other) const;
  Thread rfloordiv(const Thread& other) const;
  Thread mod(double other) const;
  Thread mod(const Thread& other) const;
  Thread rmod(double other) const;
  Thread rmod(const Thread& other) const;
  Thread pow(double other) const;
  Thread pow(const Thread& other) const;
  Thread rpow(double other) const;
  Thread rpow(const Thread& other) const;
  Thread compare(double other, ThreadBinaryCompareOp op) const;
  Thread compare(std::string_view other, ThreadBinaryCompareOp op) const;
  Thread compare(const char* other, ThreadBinaryCompareOp op) const {
    return compare(std::string_view{other != nullptr ? other : ""}, op);
  }
  Thread compare(const pluss::Series& other, ThreadBinaryCompareOp op) const;
  Thread compare(const pluss::Note& other, ThreadBinaryCompareOp op) const;
  Thread compare(const pluss::Timeline& other, ThreadBinaryCompareOp op) const;
  Thread compare(const pluss::TimeIndex& other, ThreadBinaryCompareOp op) const;
  Thread compare(const Thread& other, ThreadBinaryCompareOp op) const;
  Thread eq(double other) const;
  Thread eq(std::string_view other) const;
  Thread eq(const char* other) const { return eq(std::string_view{other != nullptr ? other : ""}); }
  Thread eq(const Thread& other) const;
  Thread ne(double other) const;
  Thread ne(std::string_view other) const;
  Thread ne(const char* other) const { return ne(std::string_view{other != nullptr ? other : ""}); }
  Thread ne(const Thread& other) const;
  Thread lt(double other) const;
  Thread lt(std::string_view other) const;
  Thread lt(const char* other) const { return lt(std::string_view{other != nullptr ? other : ""}); }
  Thread lt(const Thread& other) const;
  Thread le(double other) const;
  Thread le(std::string_view other) const;
  Thread le(const char* other) const { return le(std::string_view{other != nullptr ? other : ""}); }
  Thread le(const Thread& other) const;
  Thread gt(double other) const;
  Thread gt(std::string_view other) const;
  Thread gt(const char* other) const { return gt(std::string_view{other != nullptr ? other : ""}); }
  Thread gt(const Thread& other) const;
  Thread ge(double other) const;
  Thread ge(std::string_view other) const;
  Thread ge(const char* other) const { return ge(std::string_view{other != nullptr ? other : ""}); }
  Thread ge(const Thread& other) const;
  Thread land(const Thread& other) const;
  Thread rand(const Thread& other) const;
  Thread lor(const Thread& other) const;
  Thread ror(const Thread& other) const;
  Thread lxor(const Thread& other) const;
  Thread rxor(const Thread& other) const;
  Thread btw(double a, double b, bool inclusive = false) const;
  Thread btw(double a, double b, bool include_left, bool include_right) const;
  Thread btw(std::string_view lo, std::string_view hi, bool inclusive = true) const;
  Thread btw(std::string_view lo, std::string_view hi, bool include_left, bool include_right) const;
  Thread btw(const Thread& lo, const Thread& hi, bool inclusive = true) const;
  Thread btw(const Thread& lo, const Thread& hi, bool include_left, bool include_right) const;
  Thread between(double a, double b, bool inclusive = false) const { return btw(a, b, inclusive); }
  Thread between(double a, double b, bool include_left, bool include_right) const {
    return btw(a, b, include_left, include_right);
  }
  Thread between(std::string_view lo, std::string_view hi, bool inclusive = true) const {
    return btw(lo, hi, inclusive);
  }
  Thread between(std::string_view lo, std::string_view hi, bool include_left, bool include_right) const {
    return btw(lo, hi, include_left, include_right);
  }
  Thread between(const Thread& lo, const Thread& hi, bool inclusive = true) const {
    return btw(lo, hi, inclusive);
  }
  Thread between(const Thread& lo, const Thread& hi, bool include_left, bool include_right) const {
    return btw(lo, hi, include_left, include_right);
  }
  Thread window(std::int64_t window, ThreadWindowOp op, std::int64_t min_periods = 1) const;
  ThreadCastAccessor cast() const;
  ThreadCastAccessor cast(const ThreadCastOptions& options) const;
  Thread astype(pluss::codem::ScalarType dtype) const;
  ThreadGroupBuilder group() const;
  ThreadGroupBuilder group(const ThreadGroupOptions& options) const;

  std::string to_string(const display::DisplayOptions& options = {}) const;
  std::string info_string() const;
  void show(const display::DisplayOptions& options = {}) const;
  void show(std::ostream& os, const display::DisplayOptions& options = {}) const;
  void info() const;
  void info(std::ostream& os) const;
  ThreadPlotAccessor plot() const;
  ThreadPlotAccessor plot(const DataFramePlotOptions& options) const;
  ThreadLabelAccessor label() const;
  Thread align(const Thread& target_labels, const AlignOptions& options = {}) const;
  bool named() const;
  std::string name() const;
  Thread name(std::string value) const;
  Thread unname() const;

  const pluss::Series& as_series() const;
  const pluss::Note& as_note() const;
  const pluss::Timeline& as_timeline() const;
  const pluss::TimeIndex& as_timeindex() const;

  TimeSeriesAccessor ts() const;

private:
  friend class Atlas;
  friend class DataFrame;
  friend class DataFrameView;
  friend class ThreadCastAccessor;
  friend class ThreadGroupBuilder;
  friend class ThreadGrouped;
  friend class ThreadNoteAccessor;
  friend class ThreadTimelineAccessor;
  friend class ThreadLabelAccessor;
  friend class ThreadValue;
  friend class ThreadView;
  friend class TimeSeriesAccessor;
  friend bool detail::has_validity_mask(const Thread& thread);
  friend const std::optional<pluss::Series>& detail::validity_mask(const Thread& thread);
  friend Thread detail::with_validity_mask(const Thread& thread, const pluss::Series& validity_mask);
  friend Thread& detail::with_validity_mask_(Thread& thread, const pluss::Series& validity_mask);
  friend Thread detail::clear_validity_mask(const Thread& thread);
  friend Thread& detail::clear_validity_mask_(Thread& thread);
  friend bool detail::has_display_labels(const Thread& thread);
  friend Thread detail::display_labels(const Thread& thread);
  friend Thread detail::with_display_labels(const Thread& thread, const Thread& labels);
  friend Thread& detail::with_display_labels_(Thread& thread, const Thread& labels);
  friend Thread detail::clear_display_labels(const Thread& thread);
  friend Thread& detail::clear_display_labels_(Thread& thread);
  friend LabelKind detail::label_kind(const Thread& thread);
  friend Thread detail::with_label_kind(const Thread& thread, LabelKind kind);
  friend Thread& detail::with_label_kind_(Thread& thread, LabelKind kind);
  friend bool detail::valid_at(const Thread& thread, std::int64_t row);
  friend int detail::compare_rows(const Thread& thread,
                                  std::int64_t lhs_row,
                                  std::int64_t rhs_row,
                                  bool nulls_last);
  friend bool detail::same_payload(const Thread& left, const Thread& right) noexcept;
  friend Thread detail::math(const Thread& thread,
                             double other,
                             ThreadBinaryMathOp op,
                             bool reverse);
  friend Thread detail::math(const Thread& left, const Thread& right, ThreadBinaryMathOp op);

  explicit Thread(pluss::msharp::intrusive_ptr<detail::ThreadImpl> impl);
  Thread cast_(ThreadCastTarget target, const ThreadCastOptions& options = {}) const;
  Thread cast_(ThreadCastTarget target,
               std::optional<pluss::codem::ScalarType> series_dtype) const;

  bool unique() const noexcept;
  bool has_validity_mask() const;
  const std::optional<pluss::Series>& validity_mask() const;
  Thread with_validity_mask(const pluss::Series& validity_mask) const;
  Thread& with_validity_mask_(const pluss::Series& validity_mask);
  Thread clear_validity_mask() const;
  Thread& clear_validity_mask_();
  bool has_display_labels() const;
  Thread display_labels() const;
  Thread with_display_labels(const Thread& labels) const;
  Thread& with_display_labels_(const Thread& labels);
  Thread clear_display_labels() const;
  Thread& clear_display_labels_();
  bool has_caselbl() const;
  Thread caselbl() const;
  Thread caselbl(const Thread& labels) const;
  Thread& caselbl_(const Thread& labels);
  Thread rangelbl() const;
  Thread& rangelbl_();
  Thread clear_caselbl() const;
  Thread& clear_caselbl_();

  bool is_valid(std::int64_t row) const;
  int compare_rows(std::int64_t lhs_row, std::int64_t rhs_row, bool nulls_last = true) const;
  Thread year() const;
  Thread month() const;
  Thread day() const;
  Thread hour() const;
  bool same_payload_(const Thread& other) const noexcept;
  Thread math_(double other, ThreadBinaryMathOp op, bool reverse = false) const;
  Thread math_(const Thread& other, ThreadBinaryMathOp op) const;
private:
  Thread sample_fraction_(double frac,
                          std::optional<std::uint64_t> random_state,
                          bool replace) const;
  Thread moving(std::int64_t window,
                WindowStatistic statistic = WindowStatistic::Mean,
                const WindowOptions& options = {}) const;
  Thread growing(WindowStatistic statistic,
                 const StatisticOptions& options = {}) const;
  Thread decay(double alpha,
               WindowStatistic statistic = WindowStatistic::Mean,
               bool adjusted = true,
               bool bias_corrected = true,
               std::int64_t min_count = 1,
               bool ignore_nulls = false) const;
  Thread timed(const Thread& axis,
               std::int64_t duration_nanos,
               TimeWindowStatistic statistic = TimeWindowStatistic::Mean,
               const StatisticOptions& options = {},
               WindowClosure closure = WindowClosure::Right) const;
  Thread grouped(const Thread& groups,
                 std::int64_t window,
                 WindowStatistic statistic = WindowStatistic::Mean,
                 const WindowOptions& options = {}) const;

  detail::ThreadImpl& mut_impl_();
  const detail::ThreadImpl& impl_ref_() const;

  pluss::msharp::intrusive_ptr<detail::ThreadImpl> impl_{nullptr};
};

namespace detail {
inline bool has_validity_mask(const Thread& thread) {
  return thread.has_validity_mask();
}

inline const std::optional<pluss::Series>& validity_mask(const Thread& thread) {
  return thread.validity_mask();
}

inline Thread with_validity_mask(const Thread& thread, const pluss::Series& validity_mask) {
  return thread.with_validity_mask(validity_mask);
}

inline Thread& with_validity_mask_(Thread& thread, const pluss::Series& validity_mask) {
  return thread.with_validity_mask_(validity_mask);
}

inline Thread clear_validity_mask(const Thread& thread) {
  return thread.clear_validity_mask();
}

inline Thread& clear_validity_mask_(Thread& thread) {
  return thread.clear_validity_mask_();
}

inline bool has_display_labels(const Thread& thread) {
  return thread.has_display_labels();
}

inline Thread display_labels(const Thread& thread) {
  return thread.display_labels();
}

inline Thread with_display_labels(const Thread& thread, const Thread& labels) {
  return thread.with_display_labels(labels);
}

inline Thread& with_display_labels_(Thread& thread, const Thread& labels) {
  return thread.with_display_labels_(labels);
}

inline Thread clear_display_labels(const Thread& thread) {
  return thread.clear_display_labels();
}

inline Thread& clear_display_labels_(Thread& thread) {
  return thread.clear_display_labels_();
}

inline bool valid_at(const Thread& thread, std::int64_t row) {
  return thread.is_valid(row);
}

inline int compare_rows(const Thread& thread,
                        std::int64_t lhs_row,
                        std::int64_t rhs_row,
                        bool nulls_last) {
  return thread.compare_rows(lhs_row, rhs_row, nulls_last);
}

inline bool same_payload(const Thread& left, const Thread& right) noexcept {
  return left.same_payload_(right);
}

inline Thread math(const Thread& thread,
                   double other,
                   ThreadBinaryMathOp op,
                   bool reverse) {
  return thread.math_(other, op, reverse);
}

inline Thread math(const Thread& left, const Thread& right, ThreadBinaryMathOp op) {
  return left.math_(right, op);
}

struct GroupStateCache {
  std::vector<Thread> keys{};
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
  pluss::Series included_mask_cpu{};
  pluss::Series included_rows_cpu{};
  pluss::Series group_ids_cpu{};
  pluss::Series included_group_ids_cpu{};
  std::int64_t ngroups{0};
  bool all_included{true};
};
} // namespace detail

struct TimeSeriesResample {
  Thread labels{};
  Thread values{};
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
  explicit TimeSeriesAccessor(Thread base);

  Thread returns(std::int64_t periods = 1) const;
  Thread log_returns(std::int64_t periods = 1) const;
  Thread diff(std::int64_t periods = 1) const;
  Thread lag(std::int64_t periods = 1) const;
  Thread lead(std::int64_t periods = 1) const;
  Thread rolling(std::int64_t window,
                 WindowStatistic statistic = WindowStatistic::Mean,
                 const WindowOptions& options = {}) const;
  Thread rolling(const Thread& axis,
                 std::string_view duration,
                 WindowStatistic statistic = WindowStatistic::Mean,
                 const StatisticOptions& options = {},
                 WindowClosure closure = WindowClosure::Right) const;
  Thread rolling(const Thread& axis,
                 const Thread& groups,
                 std::string_view duration,
                 WindowStatistic statistic = WindowStatistic::Mean,
                 const StatisticOptions& options = {},
                 WindowClosure closure = WindowClosure::Right) const;
  Thread timed(const Thread& axis,
               std::int64_t duration_nanos,
               TimeWindowStatistic statistic = TimeWindowStatistic::Mean,
               const StatisticOptions& options = {},
               WindowClosure closure = WindowClosure::Right) const;
  Thread grouped(const Thread& groups,
                 std::int64_t window,
                 WindowStatistic statistic = WindowStatistic::Mean,
                 const WindowOptions& options = {}) const;
  Thread expanding(WindowStatistic statistic = WindowStatistic::Mean,
                   const StatisticOptions& options = {}) const;
  Thread ewm(double alpha,
             WindowStatistic statistic = WindowStatistic::Mean,
             bool adjusted = true,
             bool bias_corrected = true,
             std::int64_t min_count = 1,
             bool ignore_nulls = false) const;
  Thread zscore_rolling(std::int64_t window,
                        std::int64_t min_periods = 1,
                        double epsilon = 1e-12) const;
  Thread fill_gaps(pluss::subsp::stats::ts::GapFillMode mode =
                       pluss::subsp::stats::ts::GapFillMode::Forward,
                   std::optional<double> constant = std::nullopt) const;
  Thread align_to(const Thread& source_axis,
                  const Thread& target_axis,
                  TsAlignMode mode = TsAlignMode::Exact) const;
  Thread align_to(const Thread& source_axis,
                  const Thread& target_axis,
                  TsAlignMode mode,
                  pluss::subsp::stats::ts::GapFillMode fill_mode,
                  std::optional<double> constant = std::nullopt) const;
  DataFrame lag_block(pluss::msharp::RangeView::Int lags) const;
  DataFrame rolling_block(pluss::msharp::RangeView::Int windows,
                          const TsRollingBlockOptions& options = {}) const;
  TimeSeriesResampler resample(const Thread& axis,
                               std::string_view freq,
                               std::int64_t step = 1) const;
  TimeSeriesResampler resample(const Thread& axis,
                               const pluss::TimeIndex::ResampleOptions& options) const;
  TimeSeriesResample resample(const Thread& axis,
                              std::string_view freq,
                              pluss::subsp::stats::ts::ResampleOp op,
                              std::int64_t step = 1) const;
  TimeSeriesResample resample(
      const Thread& axis,
      const pluss::TimeIndex::ResampleOptions& options,
      pluss::subsp::stats::ts::ResampleOp op) const;
  Thread add(const Thread& other,
             const Thread& left_axis,
             const Thread& right_axis,
             TsAlignMode mode = TsAlignMode::Exact) const;
  Thread sub(const Thread& other,
             const Thread& left_axis,
             const Thread& right_axis,
             TsAlignMode mode = TsAlignMode::Exact) const;
  Thread mul(const Thread& other,
             const Thread& left_axis,
             const Thread& right_axis,
             TsAlignMode mode = TsAlignMode::Exact) const;
  Thread div(const Thread& other,
             const Thread& left_axis,
             const Thread& right_axis,
             TsAlignMode mode = TsAlignMode::Exact) const;
  Thread eq(const Thread& other,
            const Thread& left_axis,
            const Thread& right_axis,
            TsAlignMode mode = TsAlignMode::Exact) const;
  Thread ne(const Thread& other,
            const Thread& left_axis,
            const Thread& right_axis,
            TsAlignMode mode = TsAlignMode::Exact) const;
  Thread lt(const Thread& other,
            const Thread& left_axis,
            const Thread& right_axis,
            TsAlignMode mode = TsAlignMode::Exact) const;
  Thread le(const Thread& other,
            const Thread& left_axis,
            const Thread& right_axis,
            TsAlignMode mode = TsAlignMode::Exact) const;
  Thread gt(const Thread& other,
            const Thread& left_axis,
            const Thread& right_axis,
            TsAlignMode mode = TsAlignMode::Exact) const;
  Thread ge(const Thread& other,
            const Thread& left_axis,
            const Thread& right_axis,
            TsAlignMode mode = TsAlignMode::Exact) const;
  DataFrame calendar(const Thread& axis) const;
  Thread sort(const Thread& axis,
              bool ascending = true,
              bool nulls_last = true) const;

 private:
  const pluss::Series& series_payload_(const char* op) const;
  pluss::Series masked_series_payload_(const char* op) const;
  Thread base_{};
};

class TimeSeriesResampler {
 public:
  TimeSeriesResampler(Thread base,
                      Thread axis,
                      pluss::TimeIndex::ResampleOptions options);

  Thread mean() const;
  Thread sum() const;
  Thread min() const;
  Thread max() const;
  Thread first() const;
  Thread last() const;
  Thread count() const;

  Thread asfreq(std::optional<double> fill_value = std::nullopt) const;
  Thread ffill() const;
  Thread bfill() const;
  Thread nearest() const;

 private:
  Thread aggregate_(pluss::subsp::stats::ts::ResampleOp op) const;

  Thread base_{};
  Thread axis_{};
  pluss::TimeIndex::ResampleOptions options_{};
};

class ThreadLabelAccessor {
public:
  explicit ThreadLabelAccessor(Thread base);

  Thread values() const;
  bool has() const;
  Thread set(const Thread& labels) const;
  Thread range() const;
  Thread reset() const;
  Thread align(const Thread& target_labels, const AlignOptions& options = {}) const;
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
  Thread fillnull(double value) const;
  Thread fillnull(std::string_view value) const;
  Thread fillnull(const char* value) const {
    return fillnull(std::string_view{value != nullptr ? value : ""});
  }
  Thread fillnull(const pluss::Timeline& value) const;
  Thread fillnull(const pluss::TimeIndex& value) const;
  Thread duplicated(bool keep_first = true, bool keep_last = false) const;
  Thread repeated() const;
  Thread distinct(bool dropnull = true) const;
  Thread freq(std::int64_t top_n = 0, bool dropnull = true) const;
  Thread has(const Thread& values) const;
  Thread isnull() const;
  Thread notnull() const;
  Thread map(const std::function<ThreadValue(const ThreadValue&)>& fn) const;
  Thread take(pluss::msharp::RangeView::Int indices) const;
  Thread slice(std::int64_t start, std::int64_t stop) const;
  Thread sort(bool ascending = true, bool nulls_last = true) const;
  bool monotonic(bool ascending = true, bool strict = false, bool nulls_last = true) const;

private:
  Thread base_{};
};

class ThreadSortBuilder {
public:
  explicit ThreadSortBuilder(Thread base, bool ascending = true, bool nulls_last = true);

  Thread asc() const;
  Thread desc() const;
  Thread done() const;
  operator Thread() const;

private:
  Thread base_{};
  bool ascending_{true};
  bool nulls_last_{true};
};

class ThreadView final : public Thread {
 public:
  ThreadView();
  explicit ThreadView(Thread thread,
                      bool auto_render_terminal = false,
                      std::string operation = {});
  ThreadView(const ThreadView& other);
  ThreadView(ThreadView&& other) noexcept;
  ThreadView& operator=(const ThreadView& other);
  ThreadView& operator=(ThreadView&& other) noexcept;
  ~ThreadView();

  void show(const display::DisplayOptions& options = {}) const;
  void show(std::ostream& os, const display::DisplayOptions& options = {}) const;

 private:
  struct State;
  std::shared_ptr<State> state_{};

  void consume_pending_display_() const;
  void maybe_autorender_() const;
  std::string resolved_window_title_() const;

  friend void output_render(std::ostream& os, const ThreadView& thread);
  friend std::ostream& operator<<(std::ostream& os, const ThreadView& thread);
};

class ThreadCastAccessor {
public:
  explicit ThreadCastAccessor(Thread base, ThreadCastOptions options = {});

  Thread series() const;
  Thread note() const;
  Thread timeline() const;
  Thread timeindex() const;

private:
  Thread base_{};
  ThreadCastOptions options_{};
};

class ThreadGroupBuilder {
public:
  explicit ThreadGroupBuilder(Thread value, ThreadGroupOptions options = {});

  ThreadGrouped by(const Thread& key) const;
  ThreadGrouped by(std::initializer_list<Thread> keys) const;
  ThreadGrouped by(pluss::msharp::RangeView::Of<Thread> keys) const;

private:
  Thread value_{};
  ThreadGroupOptions options_{};
};

class ThreadGrouped {
public:
  ThreadGrouped(Thread value,
                std::vector<Thread> keys,
                std::vector<std::string> names,
                ThreadGroupOptions options = {});

  DataFrame agg(pluss::msharp::RangeView::Of<ThreadGroupSpec> specs) const;
  DataFrame agg(std::initializer_list<ThreadGroupSpec> specs) const;
  Thread transform(GroupOp op) const;
  DataFrame transform(pluss::msharp::RangeView::Of<ThreadGroupSpec> specs) const;
  DataFrame transform(std::initializer_list<ThreadGroupSpec> specs) const;
  Thread filter(const pluss::Series& keep) const;
  Thread filter(const Thread& keep) const;

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

  Thread value_{};
  std::vector<Thread> keys_{};
  std::vector<std::string> names_{};
  ThreadGroupOptions options_{};
  mutable std::shared_ptr<const detail::GroupStateCache> state_cache_{};
};

class ThreadNoteAccessor {
public:
  explicit ThreadNoteAccessor(Thread base);

  Thread strip() const;
  Thread strip(std::string_view chars) const;
  Thread lstrip() const;
  Thread lstrip(std::string_view chars) const;
  Thread rstrip() const;
  Thread rstrip(std::string_view chars) const;
  Thread lower() const;
  Thread upper() const;
  Thread casefold() const;
  Thread titlecase() const;
  Thread length() const;
  Thread starts(std::string_view prefix) const;
  Thread ends(std::string_view suffix) const;
  Thread has(std::string_view needle,
             const TextOptions& options = {}) const;
  Thread count(std::string_view pattern) const;
  Thread count(std::string_view pattern,
               const TextOptions& options) const;
  Thread matches(std::string_view pattern, bool full = false) const;
  Thread fullmatch(std::string_view pattern) const;
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
  Thread pad(std::int64_t width,
             TextAlign align = TextAlign::Left,
             char fill = ' ') const;
  Thread chars(std::int64_t start,
               std::optional<std::int64_t> stop = std::nullopt,
               std::int64_t step = 1) const;
  Thread normalize(UnicodeForm form = UnicodeForm::NFC) const;
  Thread encode(std::string_view encoding = "base64") const;
  DataFrame indicate(std::string_view delimiter = "|",
                     std::string prefix = "") const;
  Thread replace(std::string_view from, std::string_view to) const;
  Thread replace(std::string_view from,
                 std::string_view to,
                 const TextOptions& options) const;

private:
  Thread base_{};
};

class ThreadTimelineAccessor {
public:
  explicit ThreadTimelineAccessor(Thread base);

  Thread isnat() const;
  Thread valid() const;
  Thread year() const;
  Thread month() const;
  Thread day() const;
  Thread hour() const;
  Thread minute() const;
  Thread second() const;
  Thread weekday() const;
  Thread yearday() const;
  Thread quarter() const;
  Thread week() const;
  Thread weekend() const;
  Thread month_start() const;
  Thread month_end() const;
  Thread leap() const;
  Thread weekday_name() const;
  Thread month_name() const;
  Thread fmt(std::string_view pattern) const;
  Thread iso() const;
  Thread epoch(TimeUnit unit = TimeUnit::Nanos) const;
  Thread span(const Thread& other, const TimelineSpanOptions& options = {}) const;
  Thread span(const Thread& other, TimeUnit unit) const;
  Thread span(const pluss::Timeline& other, const TimelineSpanOptions& options = {}) const;
  Thread span(const pluss::Timeline& other, TimeUnit unit) const;
  Thread floor(std::string_view unit) const;
  Thread ceil(std::string_view unit) const;
  Thread round(std::string_view unit) const;
  Thread date() const;
  Thread time() const;
  Thread shift(std::int64_t periods, std::string_view freq) const;
  Thread period(std::string_view freq) const;
  Thread tz(std::string_view name) const;
  Thread localize(const TimelineLocalizeOptions& options = {}) const;
  Thread utc() const;
  Thread index(bool require_sorted = true) const;

private:
  Thread base_{};
};

Thread operator+(const Thread& lhs, double rhs);
Thread operator+(double lhs, const Thread& rhs);
Thread operator+(const Thread& lhs, const Thread& rhs);
Thread operator-(const Thread& lhs, double rhs);
Thread operator-(double lhs, const Thread& rhs);
Thread operator-(const Thread& lhs, const Thread& rhs);
Thread operator*(const Thread& lhs, double rhs);
Thread operator*(double lhs, const Thread& rhs);
Thread operator*(const Thread& lhs, const Thread& rhs);
Thread operator/(const Thread& lhs, double rhs);
Thread operator/(double lhs, const Thread& rhs);
Thread operator/(const Thread& lhs, const Thread& rhs);
Thread operator%(const Thread& lhs, double rhs);
Thread operator%(double lhs, const Thread& rhs);
Thread operator%(const Thread& lhs, const Thread& rhs);
Thread operator&(const Thread& lhs, const Thread& rhs);
Thread operator|(const Thread& lhs, const Thread& rhs);
Thread operator^(const Thread& lhs, const Thread& rhs);
Thread operator==(const Thread& lhs, double rhs);
Thread operator==(double lhs, const Thread& rhs);
Thread operator==(const Thread& lhs, std::string_view rhs);
Thread operator==(std::string_view lhs, const Thread& rhs);
Thread operator==(const Thread& lhs, const char* rhs);
Thread operator==(const char* lhs, const Thread& rhs);
Thread operator==(const Thread& lhs, const pluss::Series& rhs);
Thread operator==(const Thread& lhs, const pluss::Note& rhs);
Thread operator==(const Thread& lhs, const pluss::Timeline& rhs);
Thread operator==(const Thread& lhs, const pluss::TimeIndex& rhs);
Thread operator==(const Thread& lhs, const Thread& rhs);
Thread operator!=(const Thread& lhs, double rhs);
Thread operator!=(double lhs, const Thread& rhs);
Thread operator!=(const Thread& lhs, std::string_view rhs);
Thread operator!=(std::string_view lhs, const Thread& rhs);
Thread operator!=(const Thread& lhs, const char* rhs);
Thread operator!=(const char* lhs, const Thread& rhs);
Thread operator!=(const Thread& lhs, const pluss::Series& rhs);
Thread operator!=(const Thread& lhs, const pluss::Note& rhs);
Thread operator!=(const Thread& lhs, const pluss::Timeline& rhs);
Thread operator!=(const Thread& lhs, const pluss::TimeIndex& rhs);
Thread operator!=(const Thread& lhs, const Thread& rhs);
Thread operator<(const Thread& lhs, double rhs);
Thread operator<(double lhs, const Thread& rhs);
Thread operator<(const Thread& lhs, std::string_view rhs);
Thread operator<(std::string_view lhs, const Thread& rhs);
Thread operator<(const Thread& lhs, const char* rhs);
Thread operator<(const char* lhs, const Thread& rhs);
Thread operator<(const Thread& lhs, const pluss::Series& rhs);
Thread operator<(const Thread& lhs, const pluss::Note& rhs);
Thread operator<(const Thread& lhs, const pluss::Timeline& rhs);
Thread operator<(const Thread& lhs, const pluss::TimeIndex& rhs);
Thread operator<(const Thread& lhs, const Thread& rhs);
Thread operator<=(const Thread& lhs, double rhs);
Thread operator<=(double lhs, const Thread& rhs);
Thread operator<=(const Thread& lhs, std::string_view rhs);
Thread operator<=(std::string_view lhs, const Thread& rhs);
Thread operator<=(const Thread& lhs, const char* rhs);
Thread operator<=(const char* lhs, const Thread& rhs);
Thread operator<=(const Thread& lhs, const pluss::Series& rhs);
Thread operator<=(const Thread& lhs, const pluss::Note& rhs);
Thread operator<=(const Thread& lhs, const pluss::Timeline& rhs);
Thread operator<=(const Thread& lhs, const pluss::TimeIndex& rhs);
Thread operator<=(const Thread& lhs, const Thread& rhs);
Thread operator>(const Thread& lhs, double rhs);
Thread operator>(double lhs, const Thread& rhs);
Thread operator>(const Thread& lhs, std::string_view rhs);
Thread operator>(std::string_view lhs, const Thread& rhs);
Thread operator>(const Thread& lhs, const char* rhs);
Thread operator>(const char* lhs, const Thread& rhs);
Thread operator>(const Thread& lhs, const pluss::Series& rhs);
Thread operator>(const Thread& lhs, const pluss::Note& rhs);
Thread operator>(const Thread& lhs, const pluss::Timeline& rhs);
Thread operator>(const Thread& lhs, const pluss::TimeIndex& rhs);
Thread operator>(const Thread& lhs, const Thread& rhs);
Thread operator>=(const Thread& lhs, double rhs);
Thread operator>=(double lhs, const Thread& rhs);
Thread operator>=(const Thread& lhs, std::string_view rhs);
Thread operator>=(std::string_view lhs, const Thread& rhs);
Thread operator>=(const Thread& lhs, const char* rhs);
Thread operator>=(const char* lhs, const Thread& rhs);
Thread operator>=(const Thread& lhs, const pluss::Series& rhs);
Thread operator>=(const Thread& lhs, const pluss::Note& rhs);
Thread operator>=(const Thread& lhs, const pluss::Timeline& rhs);
Thread operator>=(const Thread& lhs, const pluss::TimeIndex& rhs);
Thread operator>=(const Thread& lhs, const Thread& rhs);

class ThreadValue {
public:
  using Payload = std::variant<pluss::Series, pluss::Note, pluss::Timeline, pluss::TimeIndex>;

  class HostAny {
  public:
    enum class Kind : std::uint8_t {
      Bool = 0,
      Int = 1,
      UInt = 2,
      Double = 3,
      String = 4,
      Time = 5,
    };

    using Storage = std::variant<bool, std::int64_t, std::uint64_t, double, std::string>;

    HostAny() = default;
    static HostAny boolean(bool value);
    static HostAny integer(std::int64_t value);
    static HostAny unsigned_integer(std::uint64_t value);
    static HostAny floating(double value);
    static HostAny string(std::string value);
    static HostAny time(std::int64_t epoch_nanos, std::string iso);

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

  ThreadValue() = default;
  explicit ThreadValue(Thread thread);

  bool defined() const noexcept;
  ThreadKind kind() const;
  const Thread& thread() const noexcept;
  Payload payload() const;

  pluss::Series as_series() const;
  pluss::Note as_note() const;
  pluss::Timeline as_timeline() const;
  pluss::TimeIndex as_timeindex() const;
  HostAny host() const;
  double to_dbl() const;
  std::int64_t to_int() const;
  std::string to_str() const;

  operator pluss::Series() const;
  operator pluss::Note() const;
  operator pluss::Timeline() const;
  operator pluss::TimeIndex() const;

private:
  Thread thread_{};
};

std::ostream& operator<<(std::ostream& os, const Thread& thread);
void output_render(std::ostream& os, const Thread& thread);
std::ostream& operator<<(std::ostream& os, const ThreadView& thread);
void output_render(std::ostream& os, const ThreadView& thread);
std::ostream& operator<<(std::ostream& os, const ThreadValue& value);
void output_render(std::ostream& os, const ThreadValue& value);
std::ostream& operator<<(std::ostream& os, const ThreadValue::HostAny& value);

} // namespace pluss::table
