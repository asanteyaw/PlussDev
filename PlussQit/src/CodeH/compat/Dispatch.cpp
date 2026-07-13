#include "CodeH/compat/Dispatch.h"

#include <string_view>

#include "CodeH/compat/Invoke.h"
#include "CodeH/compat/Access.h"

namespace pluss::codeh::_compat {

namespace deets = ::pluss::codeh::detail;

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

inline TimeIndex::ResampleOptions arg_resample_options(const Args& args, std::size_t freq_index) {
  return TimeIndex::ResampleOptions{
      .freq = std::string(arg_sv(args, freq_index)),
      .step = arg_i64(args, freq_index + 1),
      .closed = static_cast<TimeIndex::ResampleClosed>(arg_i64(args, freq_index + 2)),
      .label = static_cast<TimeIndex::ResampleLabel>(arg_i64(args, freq_index + 3)),
      .origin = static_cast<TimeIndex::ResampleOrigin>(arg_i64(args, freq_index + 4)),
      .offset_nanos = arg_i64(args, freq_index + 5),
  };
}

} // namespace

Timeline call_timeline(std::string_view op, const Timeline& self, const Args& args) {
  if (op == "add_duration") return deets::impl_add_duration(self, arg_as<Duration>(args, 0));
  if (op == "sub_duration") return deets::impl_sub_duration(self, arg_as<Duration>(args, 0));
  if (op == "add_offset") return deets::impl_add_offset(self, arg_as<CalendarOffset>(args, 0));
  if (op == "sub_offset") return deets::impl_sub_offset(self, arg_as<CalendarOffset>(args, 0));
  if (op == "dt_floor") return deets::impl_dt_round(self, "floor", arg_sv(args, 0));
  if (op == "dt_ceil") return deets::impl_dt_round(self, "ceil", arg_sv(args, 0));
  if (op == "dt_round") return deets::impl_dt_round(self, "round", arg_sv(args, 0));
  if (op == "date_range") {
    const auto& end = arg_as<Timeline>(args, 0);
    const auto freq = arg_sv(args, 1);
    const auto step = arg_i64(args, 2);
    return deets::impl_date_range(self, end, freq, step);
  }
  if (op == "tz_convert") return deets::impl_tz_convert(self, arg_sv(args, 0));
  if (op == "tz_localize") return deets::impl_tz_localize(self, arg_sv(args, 0), (int)arg_i64(args, 1), arg_sv(args, 2));
  if (op == "to_utc") return deets::impl_tz_convert(self, "UTC");

  // Rounding/normalization (Timeline-level convenience)
  if (op == "floor") return deets::impl_dt_round(self, "floor", arg_sv(args, 0));
  if (op == "ceil") return deets::impl_dt_round(self, "ceil", arg_sv(args, 0));
  if (op == "round") return deets::impl_dt_round(self, "round", arg_sv(args, 0));
  if (op == "normalize") return deets::impl_dt_round(self, "floor", "D");
  if (op == "date") return deets::impl_dt_round(self, "floor", "D");
  if (op == "time") return deets::impl_time_of_day(self);
  if (op == "time_of_day") return deets::impl_time_of_day(self);

  if (op == "min") return deets::impl_min(self);
  if (op == "max") return deets::impl_max(self);
  if (op == "sort") return deets::impl_sort(self);
  if (op == "unique") return deets::impl_unique(self);
  if (op == "drop_duplicates") return deets::impl_unique(self);

  if (op == "fill_nat") return deets::impl_fill_nat(self, arg_as<Timeline>(args, 0));
  if (op == "where") return deets::impl_where(self, arg_as<pluss::codem::Series>(args, 0), arg_as<Timeline>(args, 1));

  if (op == "add_nanos") return deets::impl_add_nanos(self, arg_i64(args, 0));
  CODEM_FAIL("CodeH invoke: unknown Timeline op");
}

Duration call_duration(std::string_view op, const Duration& self, const Args& args) {
  if (op == "add") return deets::impl_add_duration(self, arg_as<Duration>(args, 0));
  if (op == "sub") return deets::impl_sub_duration(self, arg_as<Duration>(args, 0));
  if (op == "neg") return deets::impl_neg_duration(self);
  if (op == "range") {
    const auto& end = arg_as<Duration>(args, 0);
    const auto& step = arg_as<Duration>(args, 1);
    return deets::impl_range(self, end, step);
  }
  if (op == "abs") return deets::impl_abs_duration(self);
  if (op == "sign") return deets::impl_sign_duration(self);
  if (op == "clip") return deets::impl_clip_duration(self, arg_as<Duration>(args, 0), arg_as<Duration>(args, 1));
  if (op == "fill_nat") return deets::impl_fill_nat_duration(self, arg_as<Duration>(args, 0));
  if (op == "where") return deets::impl_where_duration(self, arg_as<pluss::codem::Series>(args, 0), arg_as<Duration>(args, 1));
  if (op == "mul_i64") return deets::impl_mul_i64(self, arg_i64(args, 0));
  if (op == "div_i64") return deets::impl_div_i64(self, arg_i64(args, 0));
  if (op == "floordiv_i64") return deets::impl_floordiv_i64(self, arg_i64(args, 0));
  if (op == "mod_i64") return deets::impl_mod_i64(self, arg_i64(args, 0));
  if (op == "floor") return deets::impl_round(self, "floor", arg_sv(args, 0));
  if (op == "ceil") return deets::impl_round(self, "ceil", arg_sv(args, 0));
  if (op == "round") return deets::impl_round(self, "round", arg_sv(args, 0));
  if (op == "min") return deets::impl_min_duration(self);
  if (op == "max") return deets::impl_max_duration(self);
  if (op == "sort") return deets::impl_sort_duration(self);
  if (op == "unique") return deets::impl_unique_duration(self);
  CODEM_FAIL("CodeH invoke: unknown Duration op");
}

pluss::codem::Series call_series_duration(std::string_view op, const Duration& self, const Args& args) {
  if (op == "eq" || op == "ne" || op == "lt" || op == "le" || op == "gt" || op == "ge") {
    return deets::impl_cmp_duration(self, arg_as<Duration>(args, 0), op);
  }
  if (op == "isnat") return deets::impl_isnat_duration(self);
  if (op == "isvalid") return deets::impl_isvalid_duration(self);
  if (op == "total") return deets::impl_total(self, arg_sv(args, 0));
  if (op == "component") return deets::impl_component(self, arg_sv(args, 0));
  if (op == "argsort") return deets::impl_argsort_duration(self);
  CODEM_FAIL("CodeH invoke: unknown Duration->Series op");
}

pluss::coden::Note call_note_duration(std::string_view op, const Duration& self, const Args& args) {
  if (op == "to_note") return deets::impl_duration_to_note(self, "full");
  if (op == "to_note_compact") return deets::impl_duration_to_note(self, "compact");
  if (op == "to_note_iso") return deets::impl_duration_to_note(self, "compact");
  (void)args;
  CODEM_FAIL("CodeH invoke: unknown Duration->Note op");
}

std::vector<std::int64_t> call_i64vec_duration(std::string_view op, const Duration& self, const Args&) {
  if (op == "to_nanos_flat") return deets::impl_to_nanos_flat(self);
  CODEM_FAIL("CodeH invoke: unknown Duration->vector<int64> op");
}

Duration call_duration_from_timeline(std::string_view op, const Timeline& self, const Args& args) {
  if (op == "diff") return deets::impl_diff(self, arg_as<Timeline>(args, 0));
  CODEM_FAIL("CodeH invoke: unknown Timeline->Duration op");
}

pluss::codem::Series call_series_timeline(std::string_view op, const Timeline& self, const Args& args) {
  if (op == "eq" || op == "ne" || op == "lt" || op == "le" || op == "gt" || op == "ge") {
    return deets::impl_cmp(self, arg_as<Timeline>(args, 0), op);
  }

  // Generic dt-field extraction: op names are "dt_<field>".
  if (op.size() > 3 && op.substr(0,3) == "dt_") {
    return deets::impl_dt_field(self, op.substr(3));
  }

  // Generic calendar predicates: op names are "is_<predicate>".
  if (op.size() > 3 && op.substr(0,3) == "is_") {
    return deets::impl_dt_predicate(self, op);
  }

  if (op == "isnat") return deets::impl_isnat(self);
  if (op == "isvalid") return deets::impl_isvalid(self);
  if (op == "argsort") return deets::impl_argsort(self);

  if (op == "searchsorted_left") return deets::impl_searchsorted(self, arg_as<Timeline>(args, 0), true);
  if (op == "searchsorted_right") return deets::impl_searchsorted(self, arg_as<Timeline>(args, 0), false);
  CODEM_FAIL("CodeH invoke: unknown Timeline->Series op");
}

pluss::coden::Note call_note_timeline(std::string_view op, const Timeline& self, const Args& args) {
  if (op == "strftime") return deets::impl_strftime(self, arg_sv(args, 0));
  if (op == "to_note_iso") return deets::impl_strftime(self, "%Y-%m-%dT%H:%M:%S.%f%z");
  if (op == "weekday_name") return deets::impl_weekday_name(self);
  if (op == "month_name") return deets::impl_month_name(self);
  CODEM_FAIL("CodeH invoke: unknown Timeline->Note op");
}

std::string call_string_timeline(std::string_view op, const Timeline& self, const Args&) {
  if (op == "isoformat") return deets::impl_isoformat(self);
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
  if (op == "make_timeindex" || op == "from_timeline") {
    const bool require_sorted = arg_bool(args, 0);
    return ::pluss::codeh::_compat::detail::Access::from_timeline(self, require_sorted);
  }
  CODEM_FAIL("CodeH invoke: unknown Timeline->TimeIndex op");
}

pluss::codem::Series call_series_timeindex(std::string_view op, const TimeIndex& self, const Args& args) {
  if (op == "slice") return ::pluss::codeh::_compat::detail::Access::slice(self, arg_as<Timeline>(args, 0), arg_as<Timeline>(args, 1));
  if (op == "slice_time") return ::pluss::codeh::_compat::detail::Access::slice_time(self, arg_sv(args, 0), arg_sv(args, 1));
  if (op == "resample_count") return ::pluss::codeh::_compat::detail::Access::resample_count(self, arg_sv(args, 0), arg_i64(args, 1));
  if (op == "resample_count_opts") return ::pluss::codeh::_compat::detail::Access::resample_count(self, arg_resample_options(args, 0));
  if (op == "resample_sum") return ::pluss::codeh::_compat::detail::Access::resample_sum(self, arg_as<pluss::codem::Series>(args, 0), arg_sv(args, 1), arg_i64(args, 2));
  if (op == "resample_sum_opts") return ::pluss::codeh::_compat::detail::Access::resample_sum(self, arg_as<pluss::codem::Series>(args, 0), arg_resample_options(args, 1));
  if (op == "resample_mean") return ::pluss::codeh::_compat::detail::Access::resample_mean(self, arg_as<pluss::codem::Series>(args, 0), arg_sv(args, 1), arg_i64(args, 2));
  if (op == "resample_mean_opts") return ::pluss::codeh::_compat::detail::Access::resample_mean(self, arg_as<pluss::codem::Series>(args, 0), arg_resample_options(args, 1));
  if (op == "resample_min") return ::pluss::codeh::_compat::detail::Access::resample_min(self, arg_as<pluss::codem::Series>(args, 0), arg_sv(args, 1), arg_i64(args, 2));
  if (op == "resample_min_opts") return ::pluss::codeh::_compat::detail::Access::resample_min(self, arg_as<pluss::codem::Series>(args, 0), arg_resample_options(args, 1));
  if (op == "resample_max") return ::pluss::codeh::_compat::detail::Access::resample_max(self, arg_as<pluss::codem::Series>(args, 0), arg_sv(args, 1), arg_i64(args, 2));
  if (op == "resample_max_opts") return ::pluss::codeh::_compat::detail::Access::resample_max(self, arg_as<pluss::codem::Series>(args, 0), arg_resample_options(args, 1));
  if (op == "resample_first") return ::pluss::codeh::_compat::detail::Access::resample_first(self, arg_as<pluss::codem::Series>(args, 0), arg_sv(args, 1), arg_i64(args, 2));
  if (op == "resample_first_opts") return ::pluss::codeh::_compat::detail::Access::resample_first(self, arg_as<pluss::codem::Series>(args, 0), arg_resample_options(args, 1));
  if (op == "resample_last") return ::pluss::codeh::_compat::detail::Access::resample_last(self, arg_as<pluss::codem::Series>(args, 0), arg_sv(args, 1), arg_i64(args, 2));
  if (op == "resample_last_opts") return ::pluss::codeh::_compat::detail::Access::resample_last(self, arg_as<pluss::codem::Series>(args, 0), arg_resample_options(args, 1));
  if (op == "resample_ohlc") return ::pluss::codeh::_compat::detail::Access::resample_ohlc(self, arg_as<pluss::codem::Series>(args, 0), arg_sv(args, 1), arg_i64(args, 2));
  if (op == "resample_ohlc_opts") return ::pluss::codeh::_compat::detail::Access::resample_ohlc(self, arg_as<pluss::codem::Series>(args, 0), arg_resample_options(args, 1));
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
  if (op == "resample_agg_opts") {
    return ::pluss::codeh::_compat::detail::Access::resample_agg(self,
      arg_as<pluss::codem::Series>(args, 0), arg_resample_options(args, 1), arg_sv(args, 7));
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
  if (op == "resample_bins_opts") return ::pluss::codeh::_compat::detail::Access::resample_bins(self, arg_resample_options(args, 0));
  CODEM_FAIL("CodeH invoke: unknown TimeIndex->ResampleBins op");
}

TimeIndex::AlignUnion call_alignunion_timeindex(std::string_view op, const TimeIndex& self, const Args& args) {
  if (op == "align_union") return ::pluss::codeh::_compat::detail::Access::align_union(self, arg_as<TimeIndex>(args, 0));
  CODEM_FAIL("CodeH invoke: unknown TimeIndex->AlignUnion op");
}

} // namespace pluss::codeh::_compat
