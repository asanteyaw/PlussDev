#include "CodeH/compat/Dispatch.h"

#include <string_view>

#include "CodeH/compat/Invoke.h"

namespace pluss::codeh::_compat {

using ::pluss::codeh::detail::impl_add_duration;
using ::pluss::codeh::detail::impl_add_offset;
using ::pluss::codeh::detail::impl_cmp;
using ::pluss::codeh::detail::impl_date_range;
using ::pluss::codeh::detail::impl_diff;
using ::pluss::codeh::detail::impl_dt_field;
using ::pluss::codeh::detail::impl_dt_round;
using ::pluss::codeh::detail::impl_isoformat;
using ::pluss::codeh::detail::impl_strftime;
using ::pluss::codeh::detail::impl_sub_duration;
using ::pluss::codeh::detail::impl_sub_offset;
using ::pluss::codeh::detail::impl_tz_convert;
using ::pluss::codeh::detail::impl_tz_localize;
using ::pluss::codeh::detail::impl_abs_duration;
using ::pluss::codeh::detail::impl_sign_duration;
using ::pluss::codeh::detail::impl_clip_duration;
using ::pluss::codeh::detail::impl_fill_nat_duration;
using ::pluss::codeh::detail::impl_where_duration;
using ::pluss::codeh::detail::impl_mul_i64;
using ::pluss::codeh::detail::impl_div_i64;
using ::pluss::codeh::detail::impl_floordiv_i64;
using ::pluss::codeh::detail::impl_mod_i64;
using ::pluss::codeh::detail::impl_total;
using ::pluss::codeh::detail::impl_component;
using ::pluss::codeh::detail::impl_round;
using ::pluss::codeh::detail::impl_min_duration;
using ::pluss::codeh::detail::impl_max_duration;
using ::pluss::codeh::detail::impl_sort_duration;
using ::pluss::codeh::detail::impl_unique_duration;
using ::pluss::codeh::detail::impl_argsort_duration;
using ::pluss::codeh::detail::impl_to_nanos_flat;
using ::pluss::codeh::detail::impl_duration_to_note;

namespace {

template <class T>
const T& arg_as(const Args& args, std::size_t i) {
  CODEM_CHECK(i < args.xs.size(), "CodeH invoke: missing argument");
  const auto& v = args.xs[i].v;
  if constexpr (std::is_same_v<T, Timeline>) {
    auto p = std::get_if<const Timeline*>(&v);
    CODEM_CHECK(p && *p, "CodeH invoke: expected Timeline arg");
    return **p;
  } else if constexpr (std::is_same_v<T, Duration>) {
    auto p = std::get_if<const Duration*>(&v);
    CODEM_CHECK(p && *p, "CodeH invoke: expected Duration arg");
    return **p;
  } else if constexpr (std::is_same_v<T, TimeIndex>) {
    auto p = std::get_if<const TimeIndex*>(&v);
    CODEM_CHECK(p && *p, "CodeH invoke: expected TimeIndex arg");
    return **p;
  } else if constexpr (std::is_same_v<T, pluss::codem::Series>) {
    auto p = std::get_if<const pluss::codem::Series*>(&v);
    CODEM_CHECK(p && *p, "CodeH invoke: expected Series arg");
    return **p;
  } else if constexpr (std::is_same_v<T, CalendarOffset>) {
    auto p = std::get_if<const CalendarOffset*>(&v);
    CODEM_CHECK(p && *p, "CodeH invoke: expected CalendarOffset arg");
    return **p;
  } else {
    static_assert(!sizeof(T), "Unsupported arg type");
  }
}

inline std::string_view arg_sv(const Args& args, std::size_t i) {
  CODEM_CHECK(i < args.xs.size(), "CodeH invoke: missing argument");
  const auto& v = args.xs[i].v;
  auto p = std::get_if<std::string>(&v);
  CODEM_CHECK(p, "CodeH invoke: expected string arg");
  return *p;
}

inline std::int64_t arg_i64(const Args& args, std::size_t i) {
  CODEM_CHECK(i < args.xs.size(), "CodeH invoke: missing argument");
  const auto& v = args.xs[i].v;
  auto p = std::get_if<std::int64_t>(&v);
  CODEM_CHECK(p, "CodeH invoke: expected int64 arg");
  return *p;
}

inline bool arg_bool(const Args& args, std::size_t i) {
  CODEM_CHECK(i < args.xs.size(), "CodeH invoke: missing argument");
  const auto& v = args.xs[i].v;
  auto p = std::get_if<bool>(&v);
  CODEM_CHECK(p, "CodeH invoke: expected bool arg");
  return *p;
}

inline const Duration* arg_optional_duration(const Args& args, std::size_t i) {
  CODEM_CHECK(i < args.xs.size(), "CodeH invoke: missing argument");
  const auto& v = args.xs[i].v;
  if (std::holds_alternative<std::monostate>(v)) return nullptr;
  auto p = std::get_if<const Duration*>(&v);
  CODEM_CHECK(p && *p, "CodeH invoke: expected Duration or null");
  return *p;
}

} // namespace

Timeline call_timeline(std::string_view op, const Timeline& self, const Args& args) {
  if (op == "add_duration") return impl_add_duration(self, arg_as<Duration>(args, 0));
  if (op == "sub_duration") return impl_sub_duration(self, arg_as<Duration>(args, 0));
  if (op == "add_offset") return impl_add_offset(self, arg_as<CalendarOffset>(args, 0));
  if (op == "sub_offset") return impl_sub_offset(self, arg_as<CalendarOffset>(args, 0));
  if (op == "dt_floor") return impl_dt_round(self, "floor", arg_sv(args, 0));
  if (op == "dt_ceil") return impl_dt_round(self, "ceil", arg_sv(args, 0));
  if (op == "dt_round") return impl_dt_round(self, "round", arg_sv(args, 0));
  if (op == "date_range") {
    const auto& end = arg_as<Timeline>(args, 0);
    const auto freq = arg_sv(args, 1);
    const auto step = arg_i64(args, 2);
    return impl_date_range(self, end, freq, step);
  }
  if (op == "tz_convert") return impl_tz_convert(self, arg_sv(args, 0));
  if (op == "tz_localize") return impl_tz_localize(self, arg_sv(args, 0), (int)arg_i64(args, 1), arg_sv(args, 2));
  if (op == "to_utc") return impl_tz_convert(self, "UTC");

  // Rounding/normalization (Timeline-level convenience)
  if (op == "floor") return impl_dt_round(self, "floor", arg_sv(args, 0));
  if (op == "ceil") return impl_dt_round(self, "ceil", arg_sv(args, 0));
  if (op == "round") return impl_dt_round(self, "round", arg_sv(args, 0));
  if (op == "normalize") return impl_dt_round(self, "floor", "D");
  if (op == "date") return impl_dt_round(self, "floor", "D");
  if (op == "time") return ::pluss::codeh::detail::impl_time_of_day(self);

  if (op == "min") return ::pluss::codeh::detail::impl_min(self);
  if (op == "max") return ::pluss::codeh::detail::impl_max(self);
  if (op == "sort") return ::pluss::codeh::detail::impl_sort(self);
  if (op == "unique") return ::pluss::codeh::detail::impl_unique(self);
  if (op == "drop_duplicates") return ::pluss::codeh::detail::impl_unique(self);

  if (op == "fill_nat") return ::pluss::codeh::detail::impl_fill_nat(self, arg_as<Timeline>(args, 0));
  if (op == "where") return ::pluss::codeh::detail::impl_where(self, arg_as<pluss::codem::Series>(args, 0), arg_as<Timeline>(args, 1));

  if (op == "add_nanos") return ::pluss::codeh::detail::impl_add_nanos(self, arg_i64(args, 0));
  CODEM_FAIL("CodeH invoke: unknown Timeline op");
}

Duration call_duration(std::string_view op, const Duration& self, const Args& args) {
  if (op == "add") return ::pluss::codeh::detail::impl_add_duration(self, arg_as<Duration>(args, 0));
  if (op == "sub") return ::pluss::codeh::detail::impl_sub_duration(self, arg_as<Duration>(args, 0));
  if (op == "neg") return ::pluss::codeh::detail::impl_neg_duration(self);
  if (op == "range") {
    const auto& end = arg_as<Duration>(args, 0);
    const auto& step = arg_as<Duration>(args, 1);
    return ::pluss::codeh::detail::impl_range(self, end, step);
  }
  if (op == "abs") return impl_abs_duration(self);
  if (op == "sign") return impl_sign_duration(self);
  if (op == "clip") return impl_clip_duration(self, arg_as<Duration>(args, 0), arg_as<Duration>(args, 1));
  if (op == "fill_nat") return impl_fill_nat_duration(self, arg_as<Duration>(args, 0));
  if (op == "where") return impl_where_duration(self, arg_as<pluss::codem::Series>(args, 0), arg_as<Duration>(args, 1));
  if (op == "mul_i64") return impl_mul_i64(self, arg_i64(args, 0));
  if (op == "div_i64") return impl_div_i64(self, arg_i64(args, 0));
  if (op == "floordiv_i64") return impl_floordiv_i64(self, arg_i64(args, 0));
  if (op == "mod_i64") return impl_mod_i64(self, arg_i64(args, 0));
  if (op == "floor") return impl_round(self, "floor", arg_sv(args, 0));
  if (op == "ceil") return impl_round(self, "ceil", arg_sv(args, 0));
  if (op == "round") return impl_round(self, "round", arg_sv(args, 0));
  if (op == "min") return impl_min_duration(self);
  if (op == "max") return impl_max_duration(self);
  if (op == "sort") return impl_sort_duration(self);
  if (op == "unique") return impl_unique_duration(self);
  CODEM_FAIL("CodeH invoke: unknown Duration op");
}

pluss::codem::Series call_series_duration(std::string_view op, const Duration& self, const Args& args) {
  if (op == "eq" || op == "ne" || op == "lt" || op == "le" || op == "gt" || op == "ge") {
    return ::pluss::codeh::detail::impl_cmp_duration(self, arg_as<Duration>(args, 0), op);
  }
  if (op == "isnat") return ::pluss::codeh::detail::impl_isnat_duration(self);
  if (op == "isvalid") return ::pluss::codeh::detail::impl_isvalid_duration(self);
  if (op == "total") return impl_total(self, arg_sv(args, 0));
  if (op == "component") return impl_component(self, arg_sv(args, 0));
  if (op == "argsort") return impl_argsort_duration(self);
  CODEM_FAIL("CodeH invoke: unknown Duration->Series op");
}

pluss::coden::Note call_note_duration(std::string_view op, const Duration& self, const Args& args) {
  if (op == "to_note") return impl_duration_to_note(self, "full");
  if (op == "to_note_compact") return impl_duration_to_note(self, "compact");
  if (op == "to_note_iso") return impl_duration_to_note(self, "compact");
  (void)args;
  CODEM_FAIL("CodeH invoke: unknown Duration->Note op");
}

std::vector<std::int64_t> call_i64vec_duration(std::string_view op, const Duration& self, const Args&) {
  if (op == "to_nanos_flat") return impl_to_nanos_flat(self);
  CODEM_FAIL("CodeH invoke: unknown Duration->vector<int64> op");
}

Duration call_duration_from_timeline(std::string_view op, const Timeline& self, const Args& args) {
  if (op == "diff") return impl_diff(self, arg_as<Timeline>(args, 0));
  CODEM_FAIL("CodeH invoke: unknown Timeline->Duration op");
}

pluss::codem::Series call_series_timeline(std::string_view op, const Timeline& self, const Args& args) {
  if (op == "eq" || op == "ne" || op == "lt" || op == "le" || op == "gt" || op == "ge") {
    return impl_cmp(self, arg_as<Timeline>(args, 0), op);
  }

  // Generic dt-field extraction: op names are "dt_<field>".
  if (op.size() > 3 && op.substr(0,3) == "dt_") {
    return impl_dt_field(self, op.substr(3));
  }

  // Generic calendar predicates: op names are "is_<predicate>".
  if (op.size() > 3 && op.substr(0,3) == "is_") {
    return ::pluss::codeh::detail::impl_dt_predicate(self, op);
  }

  if (op == "isnat") return ::pluss::codeh::detail::impl_isnat(self);
  if (op == "isvalid") return ::pluss::codeh::detail::impl_isvalid(self);
  if (op == "argsort") return ::pluss::codeh::detail::impl_argsort(self);

  if (op == "searchsorted_left") return ::pluss::codeh::detail::impl_searchsorted(self, arg_as<Timeline>(args, 0), true);
  if (op == "searchsorted_right") return ::pluss::codeh::detail::impl_searchsorted(self, arg_as<Timeline>(args, 0), false);
  CODEM_FAIL("CodeH invoke: unknown Timeline->Series op");
}

pluss::coden::Note call_note_timeline(std::string_view op, const Timeline& self, const Args& args) {
  if (op == "strftime") return impl_strftime(self, arg_sv(args, 0));
  if (op == "to_note_iso") return impl_strftime(self, "%Y-%m-%dT%H:%M:%S.%f%z");
  if (op == "weekday_name") return ::pluss::codeh::detail::impl_weekday_name(self);
  if (op == "month_name") return ::pluss::codeh::detail::impl_month_name(self);
  CODEM_FAIL("CodeH invoke: unknown Timeline->Note op");
}

std::string call_string_timeline(std::string_view op, const Timeline& self, const Args&) {
  if (op == "isoformat") return impl_isoformat(self);
  CODEM_FAIL("CodeH invoke: unknown Timeline->string op");
}

bool call_bool_timeline(std::string_view op, const Timeline& self, const Args&) {
  if (op == "isnat_scalar") return ::pluss::codeh::detail::impl_isnat_scalar(self);
  CODEM_FAIL("CodeH invoke: unknown Timeline->bool op");
}

std::vector<std::int64_t> call_i64vec_timeline(std::string_view op, const Timeline& self, const Args&) {
  if (op == "to_epoch_nanos_flat") return ::pluss::codeh::detail::impl_to_epoch_nanos_flat(self);
  CODEM_FAIL("CodeH invoke: unknown Timeline->vector<int64> op");
}

TimeIndex call_timeindex_from_timeline(std::string_view op, const Timeline& self, const Args& args) {
  if (op == "from_timeline") {
    const bool require_sorted = arg_bool(args, 0);
    return ::pluss::codeh::_compat::detail::Access::from_timeline(self, require_sorted);
  }
  CODEM_FAIL("CodeH invoke: unknown Timeline->TimeIndex op");
}

pluss::codem::Series call_series_timeindex(std::string_view op, const TimeIndex& self, const Args& args) {
  if (op == "slice") return ::pluss::codeh::_compat::detail::Access::slice(self, arg_as<Timeline>(args, 0), arg_as<Timeline>(args, 1));
  if (op == "slice_time") return ::pluss::codeh::_compat::detail::Access::slice_time(self, arg_sv(args, 0), arg_sv(args, 1));
  if (op == "resample_count") return ::pluss::codeh::_compat::detail::Access::resample_count(self, arg_sv(args, 0), arg_i64(args, 1));
  if (op == "resample_sum") return ::pluss::codeh::_compat::detail::Access::resample_sum(self, arg_as<pluss::codem::Series>(args, 0), arg_sv(args, 1), arg_i64(args, 2));
  if (op == "resample_mean") return ::pluss::codeh::_compat::detail::Access::resample_mean(self, arg_as<pluss::codem::Series>(args, 0), arg_sv(args, 1), arg_i64(args, 2));
  if (op == "resample_min") return ::pluss::codeh::_compat::detail::Access::resample_min(self, arg_as<pluss::codem::Series>(args, 0), arg_sv(args, 1), arg_i64(args, 2));
  if (op == "resample_max") return ::pluss::codeh::_compat::detail::Access::resample_max(self, arg_as<pluss::codem::Series>(args, 0), arg_sv(args, 1), arg_i64(args, 2));
  if (op == "resample_first") return ::pluss::codeh::_compat::detail::Access::resample_first(self, arg_as<pluss::codem::Series>(args, 0), arg_sv(args, 1), arg_i64(args, 2));
  if (op == "resample_last") return ::pluss::codeh::_compat::detail::Access::resample_last(self, arg_as<pluss::codem::Series>(args, 0), arg_sv(args, 1), arg_i64(args, 2));
  if (op == "resample_ohlc") return ::pluss::codeh::_compat::detail::Access::resample_ohlc(self, arg_as<pluss::codem::Series>(args, 0), arg_sv(args, 1), arg_i64(args, 2));
  if (op == "rolling_count") return ::pluss::codeh::_compat::detail::Access::rolling_count(self, arg_as<Duration>(args, 0));
  if (op == "rolling_sum") return ::pluss::codeh::_compat::detail::Access::rolling_sum(self, arg_as<pluss::codem::Series>(args, 0), arg_as<Duration>(args, 1));
  if (op == "join_asof") {
    const auto& right = arg_as<TimeIndex>(args, 0);
    const auto dir = static_cast<TimeIndex::AsofDirection>(arg_i64(args, 1));
    const auto* tol = arg_optional_duration(args, 2);
    const bool allow_exact = arg_bool(args, 3);
    return ::pluss::codeh::_compat::detail::Access::join_asof(self, right, dir, tol, allow_exact);
  }
  if (op == "argsort") return ::pluss::codeh::_compat::detail::Access::argsort(self);
  if (op == "resample_agg") {
    return ::pluss::codeh::_compat::detail::Access::resample_agg(self,
      arg_as<pluss::codem::Series>(args, 0), arg_sv(args, 1), arg_sv(args, 2), arg_i64(args, 3));
  }
  if (op == "rolling_agg") {
    return ::pluss::codeh::_compat::detail::Access::rolling_agg(self,
      arg_as<pluss::codem::Series>(args, 0), arg_as<Duration>(args, 1), arg_sv(args, 2));
  }
  CODEM_FAIL("CodeH invoke: unknown TimeIndex->Series op");
}

std::int64_t call_i64_timeindex(std::string_view op, const TimeIndex& self, const Args& args) {
  if (op == "searchsorted") return ::pluss::codeh::_compat::detail::Access::searchsorted(self, arg_as<Timeline>(args, 0), arg_bool(args, 1));
  if (op == "asof") return ::pluss::codeh::_compat::detail::Access::asof(self, arg_as<Timeline>(args, 0));
  CODEM_FAIL("CodeH invoke: unknown TimeIndex->int64 op");
}

bool call_bool_timeindex(std::string_view op, const TimeIndex& self, const Args&) {
  if (op == "is_monotonic_increasing") return ::pluss::codeh::_compat::detail::Access::is_monotonic_increasing(self);
  if (op == "is_monotonic_decreasing") return ::pluss::codeh::_compat::detail::Access::is_monotonic_decreasing(self);
  if (op == "is_unique") return ::pluss::codeh::_compat::detail::Access::is_unique(self);
  CODEM_FAIL("CodeH invoke: unknown TimeIndex->bool op");
}

std::string call_string_timeindex(std::string_view op, const TimeIndex& self, const Args&) {
  if (op == "infer_freq") return ::pluss::codeh::_compat::detail::Access::infer_freq(self);
  CODEM_FAIL("CodeH invoke: unknown TimeIndex->string op");
}

pluss::coden::Note call_note_timeindex(std::string_view op, const TimeIndex& self, const Args&) {
  if (op == "to_note_iso") return ::pluss::codeh::_compat::detail::Access::to_note_iso(self);
  CODEM_FAIL("CodeH invoke: unknown TimeIndex->Note op");
}

Timeline call_timeline_from_timeindex(std::string_view op, const TimeIndex& self, const Args&) {
  if (op == "min") return ::pluss::codeh::_compat::detail::Access::min(self);
  if (op == "max") return ::pluss::codeh::_compat::detail::Access::max(self);
  CODEM_FAIL("CodeH invoke: unknown TimeIndex->Timeline op");
}

TimeIndex call_timeindex(std::string_view op, const TimeIndex& self, const Args& args) {
  if (op == "sort") return ::pluss::codeh::_compat::detail::Access::sort(self);
  if (op == "unique") return ::pluss::codeh::_compat::detail::Access::unique(self);
  if (op == "take") return ::pluss::codeh::_compat::detail::Access::take(self, arg_as<pluss::codem::Series>(args, 0));
  if (op == "slice_index") return ::pluss::codeh::_compat::detail::Access::slice_index(self, arg_i64(args, 0), arg_i64(args, 1));
  if (op == "tail") return ::pluss::codeh::_compat::detail::Access::tail(self, arg_i64(args, 0));
  if (op == "union") return ::pluss::codeh::_compat::detail::Access::union_(self, arg_as<TimeIndex>(args, 0));
  if (op == "intersection") return ::pluss::codeh::_compat::detail::Access::intersection(self, arg_as<TimeIndex>(args, 0));
  if (op == "difference") return ::pluss::codeh::_compat::detail::Access::difference(self, arg_as<TimeIndex>(args, 0));
  if (op == "tz_convert") return ::pluss::codeh::_compat::detail::Access::tz_convert(self, arg_sv(args, 0));
  if (op == "tz_localize") return ::pluss::codeh::_compat::detail::Access::tz_localize(self, arg_sv(args, 0), (int)arg_i64(args, 1), arg_sv(args, 2));
  if (op == "floor") return ::pluss::codeh::_compat::detail::Access::round(self, "floor", arg_sv(args, 0));
  if (op == "ceil") return ::pluss::codeh::_compat::detail::Access::round(self, "ceil", arg_sv(args, 0));
  if (op == "round") return ::pluss::codeh::_compat::detail::Access::round(self, "round", arg_sv(args, 0));
  CODEM_FAIL("CodeH invoke: unknown TimeIndex->TimeIndex op");
}

TimeIndex::ResampleBins call_resamplebins_timeindex(std::string_view op, const TimeIndex& self, const Args& args) {
  if (op == "resample_bins") return ::pluss::codeh::_compat::detail::Access::resample_bins(self, arg_sv(args, 0), arg_i64(args, 1));
  CODEM_FAIL("CodeH invoke: unknown TimeIndex->ResampleBins op");
}

TimeIndex::AlignUnion call_alignunion_timeindex(std::string_view op, const TimeIndex& self, const Args& args) {
  if (op == "align_union") return ::pluss::codeh::_compat::detail::Access::align_union(self, arg_as<TimeIndex>(args, 0));
  CODEM_FAIL("CodeH invoke: unknown TimeIndex->AlignUnion op");
}

} // namespace pluss::codeh::_compat
