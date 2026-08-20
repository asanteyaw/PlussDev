#include <pluss/pluss.h>

#include <array>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::string shape_string(std::span<const std::int64_t> sizes) {
  std::string out = "[";
  for (std::size_t i = 0; i < sizes.size(); ++i) {
    if (i != 0) out += ", ";
    out += std::to_string(sizes[i]);
  }
  out += "]";
  return out;
}

void show_timeline(std::string_view label, const pluss::codeh::Timeline& t) {
  std::cout << "\n== " << label << " ==\n";
  std::cout << "rank=" << t.dim() << " shape=" << shape_string(t.sizes())
            << " tz=" << t.tz() << "\n";
  std::cout << pluss::timeline::print(t) << "\n";
}

void show_series(std::string_view label, const pluss::Series& s) {
  std::cout << "\n== " << label << " ==\n";
  std::cout << pluss::print_series(s) << "\n";
}

} // namespace

int main() {
  using pluss::codeh::TimeIndex;
  using pluss::codeh::Timeline;

  // --------------------------------------------------------------------------
  // Factory methods: Timeline
  // --------------------------------------------------------------------------
  const auto t_scalar_sv = pluss::timeline::make(std::string_view{"2024-01-01T00:00:00Z"});
  const char* scalar_c = "2024-01-01T00:00:01Z";
  const auto t_scalar_c = pluss::timeline::make(scalar_c);

  const std::array<std::string_view, 3> flat_sv_arr{
      "2024-01-01T00:00:00Z",
      "2024-01-01T01:00:00Z",
      "2024-01-01T02:00:00Z"};
  const auto t_1d_span = pluss::timeline::make(std::span<const std::string_view>(flat_sv_arr));

  const std::vector<std::string_view> flat_sv_vec{
      "2024-01-02T00:00:00Z",
      "2024-01-02T01:00:00Z",
      "2024-01-02T02:00:00Z"};
  const auto t_1d_vecsv = pluss::timeline::make(flat_sv_vec);

  const auto t_1d_init = pluss::timeline::make({
      "2024-01-03T00:00:00Z",
      "2024-01-03T01:00:00Z",
      "2024-01-03T02:00:00Z",
  });

  const auto t_2d_nested = pluss::timeline::make({
      {"2024-02-01T00:00:00Z", "2024-02-01T01:00:00Z"},
      {"2024-02-02T00:00:00Z", "2024-02-02T01:00:00Z"},
  });

  const auto t_3d_nested = pluss::timeline::make({
      {
          {"2024-03-01T00:00:00Z", "2024-03-01T01:00:00Z"},
          {"2024-03-01T02:00:00Z", "2024-03-01T03:00:00Z"},
      },
      {
          {"2024-03-02T00:00:00Z", "2024-03-02T01:00:00Z"},
          {"2024-03-02T02:00:00Z", "2024-03-02T03:00:00Z"},
      },
  });

  const std::vector<std::string> flat_str{
      "2024-04-01T00:00:00Z", "2024-04-01T01:00:00Z",
      "2024-04-02T00:00:00Z", "2024-04-02T01:00:00Z"};
  const std::vector<std::string_view> flat_sv{
      "2024-05-01T00:00:00Z", "2024-05-01T01:00:00Z",
      "2024-05-02T00:00:00Z", "2024-05-02T01:00:00Z"};
  const std::array<std::int64_t, 2> shape_2d{2, 2};
  const auto t_make_vecstr = pluss::timeline::make(flat_str, {2, 2});
  const auto t_make_vecsv = pluss::timeline::make(flat_sv, {2, 2});
  const auto t_make_span = pluss::timeline::make(
      std::span<const std::string_view>(flat_sv),
      std::span<const std::int64_t>(shape_2d));

  const auto t_epoch_scalar = pluss::timeline::from_epoch(0LL);
  const auto t_epoch_nested = pluss::timeline::from_epoch({
      {
          {0LL, 1'000'000'000LL},
          {2'000'000'000LL, 3'000'000'000LL},
      },
      {
          {4'000'000'000LL, 5'000'000'000LL},
          {6'000'000'000LL, 7'000'000'000LL},
      },
  });
  const std::vector<std::int64_t> epoch_flat{
      10'000'000'000LL, 11'000'000'000LL, 12'000'000'000LL, 13'000'000'000LL};
  const std::array<std::int64_t, 2> epoch_shape{2, 2};
  const auto t_epoch_span = pluss::timeline::from_epoch(
      std::span<const std::int64_t>(epoch_flat),
      std::span<const std::int64_t>(epoch_shape));
  const auto t_epoch_vec = pluss::timeline::from_epoch(epoch_flat, {2, 2});

  show_timeline("factory moment(string_view)", t_scalar_sv);
  show_timeline("factory moment(const char*)", t_scalar_c);
  show_timeline("factory moment(span<string_view>)", t_1d_span);
  show_timeline("factory moment(vector<string_view>)", t_1d_vecsv);
  show_timeline("factory moment(initializer_list<const char*>)", t_1d_init);
  show_timeline("factory moment(nested literal 2D)", t_2d_nested);
  show_timeline("factory moment(nested literal 3D)", t_3d_nested);
  show_timeline("factory make_timeline(vector<string>, shape)", t_make_vecstr);
  show_timeline("factory make_timeline(vector<string_view>, shape)", t_make_vecsv);
  show_timeline("factory make_timeline(span, span)", t_make_span);
  show_timeline("factory from_epoch_nanos(scalar)", t_epoch_scalar);
  show_timeline("factory from_epoch_nanos(nested literal 3D)", t_epoch_nested);
  show_timeline("factory from_epoch_nanos(span, span)", t_epoch_span);
  show_timeline("factory from_epoch_nanos(vector, vector)", t_epoch_vec);

  const auto d_nanos = pluss::duration::nanos(500);
  const auto d_seconds = pluss::duration::seconds(1);
  const auto d_minutes = pluss::duration::minutes(1);
  const auto d_hours = pluss::duration::hours(1);
  const auto d_days = pluss::duration::days(1);
  const auto d_range = pluss::duration::range(pluss::duration::seconds(0),
                                              pluss::duration::seconds(5),
                                              pluss::duration::seconds(1));
  std::cout << "\n== factory Duration helpers ==\n";
  std::cout << "duration_nanos dim=" << d_nanos.dim() << " numel=" << d_nanos.numel() << "\n";
  std::cout << "duration_seconds dim=" << d_seconds.dim() << " numel=" << d_seconds.numel() << "\n";
  std::cout << "duration_minutes dim=" << d_minutes.dim() << " numel=" << d_minutes.numel() << "\n";
  std::cout << "duration_hours dim=" << d_hours.dim() << " numel=" << d_hours.numel() << "\n";
  std::cout << "duration_days dim=" << d_days.dim() << " numel=" << d_days.numel() << "\n";
  std::cout << "timedelta_range dim=" << d_range.dim() << " numel=" << d_range.numel() << "\n";

  const auto t_date_range_factory = pluss::timeline::date_range(
      pluss::timeline::make("2024-06-01T00:00:00Z"),
      pluss::timeline::make("2024-06-04T00:00:00Z"),
      "D",
      1);
  show_timeline("factory date_range(start, end, freq, step)", t_date_range_factory);
  const auto t_date_only = pluss::timeline::date({"2024-06-10", "2024-06-11"});
  const auto t_time_only = pluss::timeline::time({"09:30:00", "17:45:30"});
  const auto t_datetime = pluss::timeline::datetime(t_date_only, t_time_only);
  const auto t_repeated = pluss::timeline::rep("2025-02-05", {2, 2});
  const auto t_rand_between = pluss::timeline::rand_between(
      "2024-06-01", "2024-06-03", {5}, 11);
  const auto t_arange_dates = pluss::timeline::arange(
      "2024-06-01", "2024-06-05", 1, true);
  const auto t_arange_times = pluss::timeline::arange(
      "09:00:00", "09:00:05", 1, true);
  const auto t_arange_datetimes = pluss::timeline::arange(
      "2024-06-01T00:00:00Z",
      "2024-06-01T06:00:00Z",
      pluss::duration::hours(2),
      true);
  show_timeline("factory date(strict)", t_date_only);
  show_timeline("factory time(strict)", t_time_only);
  show_timeline("factory datetime(date, time)", t_datetime);
  show_timeline("factory rep(value, sizes)", t_repeated);
  show_timeline("factory rand_between(lo, hi, sizes)", t_rand_between);
  show_timeline("factory arange(date, date, step)", t_arange_dates);
  show_timeline("factory arange(time, time, step)", t_arange_times);
  show_timeline("factory arange(datetime, datetime, duration_step)", t_arange_datetimes);

  // --------------------------------------------------------------------------
  // Factory methods: TimeIndex (1D axis only)
  // --------------------------------------------------------------------------
  const auto idx_from_timeline = pluss::timeindex::make(t_1d_init, true);
  const auto idx_scalar_sv = pluss::timeindex::make(std::string_view{"2024-07-01T00:00:00Z"});
  const auto idx_scalar_c = pluss::timeindex::make("2024-07-01T00:00:01Z");
  const auto idx_span = pluss::timeindex::make(std::span<const std::string_view>(flat_sv_arr), true);
  const auto idx_vecsv = pluss::timeindex::make(flat_sv_vec, true);
  const std::vector<std::string> idx_vecstr_data{
      "2024-08-01T00:00:00Z", "2024-08-01T01:00:00Z", "2024-08-01T02:00:00Z"};
  const auto idx_vecstr = pluss::timeindex::make(idx_vecstr_data, true);
  const auto idx_init = pluss::timeindex::make({
      "2024-09-01T00:00:00Z",
      "2024-09-01T01:00:00Z",
      "2024-09-01T02:00:00Z",
  });
  const auto idx_nested = pluss::timeindex::make({
      pluss::detail::NestedTimelineLiteral("2024-10-01T00:00:00Z"),
      pluss::detail::NestedTimelineLiteral("2024-10-01T01:00:00Z"),
      pluss::detail::NestedTimelineLiteral("2024-10-01T02:00:00Z"),
  });
  const std::vector<std::int64_t> idx_epoch{
      0LL, 1'000'000'000LL, 2'000'000'000LL, 3'000'000'000LL};
  const auto idx_epoch_span = pluss::timeindex::make(std::span<const std::int64_t>(idx_epoch), true);
  const auto idx_epoch_vec = pluss::timeindex::make(idx_epoch, true);

  show_timeline("factory make_timeindex(Timeline)", idx_from_timeline.timeline());
  show_timeline("factory make_timeindex(string_view)", idx_scalar_sv.timeline());
  show_timeline("factory make_timeindex(const char*)", idx_scalar_c.timeline());
  show_timeline("factory make_timeindex(span<string_view>)", idx_span.timeline());
  show_timeline("factory make_timeindex(vector<string_view>)", idx_vecsv.timeline());
  show_timeline("factory make_timeindex(vector<string>)", idx_vecstr.timeline());
  show_timeline("factory make_timeindex(init_list<const char*>)", idx_init.timeline());
  show_timeline("factory make_timeindex(nested literal 1D)", idx_nested.timeline());
  show_timeline("factory make_timeindex(span<epoch_nanos>)", idx_epoch_span.timeline());
  show_timeline("factory make_timeindex(vector<epoch_nanos>)", idx_epoch_vec.timeline());

  // --------------------------------------------------------------------------
  // Non-generated Timeline methods (TimelineBase + manual surface)
  // --------------------------------------------------------------------------
  const auto scalar = Timeline::createFromDateTimeInts(2024, 2, 29, 12, 34, 56);
  const auto scalar2 = Timeline::createFromDateTimeInts(2025, 3, 1, 13, 35, 57);

  std::cout << "\n== Timeline scalar base helpers ==\n";
  std::cout << "has_date=" << scalar.has_date() << " has_time=" << scalar.has_time() << "\n";
  std::cout << "year=" << scalar.year_scalar()
            << " month=" << scalar.month_scalar()
            << " day=" << scalar.day_scalar()
            << " hour=" << scalar.hour_scalar()
            << " minute=" << scalar.minute_scalar()
            << " second=" << scalar.second_scalar() << "\n";
  std::cout << "dow=" << scalar.day_of_week_scalar()
            << " doy=" << scalar.day_of_year_scalar()
            << " quarter=" << scalar.quarter_scalar()
            << " week=" << scalar.week_of_year_scalar() << "\n";
  std::cout << "is_leap=" << scalar.is_leap_year_scalar()
            << " is_month_start=" << scalar.is_month_start_scalar()
            << " is_month_end=" << scalar.is_month_end_scalar()
            << " is_quarter_start=" << scalar.is_quarter_start_scalar()
            << " is_quarter_end=" << scalar.is_quarter_end_scalar()
            << " is_weekend=" << scalar.is_weekend_scalar() << "\n";
  std::cout << "weekday_name=" << scalar.weekday_name(false)
            << " month_name=" << scalar.month_name(false) << "\n";
  std::cout << "weekday_name(short)=" << scalar.weekday_name(true)
            << " month_name(short)=" << scalar.month_name(true) << "\n";
  std::cout << "format=%Y/%m/%d %H:%M:%S => " << scalar.format("%Y/%m/%d %H:%M:%S") << "\n";
  std::cout << "toISOString => " << scalar.toISOString() << "\n";
  std::cout << "to_string => " << scalar.to_string() << "\n";
  show_series("to_Series()", scalar.to_Series());

  show_timeline("set_year", scalar.set_year(2030));
  show_timeline("set_month", scalar.set_month(12));
  show_timeline("set_day", scalar.set_day(15));
  show_timeline("set_hour", scalar.set_hour(8));
  show_timeline("set_minute", scalar.set_minute(9));
  show_timeline("set_second", scalar.set_second(10));

  show_timeline("add_years", scalar.add_years(1));
  show_timeline("add_months", scalar.add_months(2));
  show_timeline("add_days", scalar.add_days(3));
  show_timeline("add_hours", scalar.add_hours(4));
  show_timeline("add_minutes", scalar.add_minutes(5));
  show_timeline("add_seconds", scalar.add_seconds(6));

  std::cout << "\n== Timeline scalar between helpers ==\n";
  std::cout << "years_between=" << scalar.years_between(scalar2)
            << " months_between=" << scalar.months_between(scalar2)
            << " days_between=" << scalar.days_between(scalar2)
            << " hours_between=" << scalar.hours_between(scalar2)
            << " minutes_between=" << scalar.minutes_between(scalar2)
            << " seconds_between=" << scalar.seconds_between(scalar2) << "\n";

  show_timeline("concat(Timeline)", scalar.concat(scalar2));
  show_timeline("concat(vector<Timeline>)", scalar.concat({scalar2, scalar}));
  show_timeline("repeat", scalar.repeat(3));
  show_timeline("empty_like", t_3d_nested.empty_like());
  show_timeline("fill_like(ymd)", t_3d_nested.fill_like(2035, 1, 2));
  show_timeline("fill_like(date_str)", t_3d_nested.fill_like("2036-03-04"));

  std::cout << "\n== Timeline comparisons ==\n";
  std::cout << "scalar == scalar: " << (scalar == scalar) << "\n";
  std::cout << "scalar != scalar2: " << (scalar != scalar2) << "\n";
  std::cout << "scalar < scalar2: " << (scalar < scalar2) << "\n";
  std::cout << "scalar <= scalar2: " << (scalar <= scalar2) << "\n";
  std::cout << "scalar2 > scalar: " << (scalar2 > scalar) << "\n";
  std::cout << "scalar2 >= scalar: " << (scalar2 >= scalar) << "\n";
  std::cout << "3D < 3D+1s: " << (t_3d_nested < t_3d_nested.add_seconds(1)) << "\n";

  show_timeline("shift(day)", t_3d_nested.shift(1, "day"));
  show_timeline("to_period(year)", t_3d_nested.to_period("year"));
  show_timeline("to_period(month)", t_3d_nested.to_period("month"));
  show_timeline("to_period(day)", t_3d_nested.to_period("day"));
  show_timeline("to_period(hour)", t_3d_nested.to_period("hour"));
  show_timeline("to_period(min)", t_3d_nested.to_period("min"));
  show_timeline("to_period(second)", t_3d_nested.to_period("second"));

  const auto idx_from_method = t_1d_init.to_timeindex(true);
  show_timeline("to_timeindex()", idx_from_method.timeline());

  // Static non-generated constructors/helpers.
  const auto t_now = Timeline::now();
  const auto t_today = Timeline::today();
  const auto t_yesterday = Timeline::yesterday();
  const auto t_tomorrow = Timeline::tomorrow();
  const auto t_from_ts = Timeline::fromTimestamp(0);
  const auto t_from_series = Timeline::fromSeries(t_epoch_vec.to_Series());
  const auto t_from_date = Timeline::createFromDateInts(2024, 12, 31);
  const auto t_from_time = Timeline::createFromTimeInts(23, 59, 58);
  const auto t_from_dt = Timeline::createFromDateTimeInts(2024, 12, 31, 23, 59, 58);
  const auto t_static_range = Timeline::date_range(
      Timeline::createFromDateInts(2024, 1, 1),
      Timeline::createFromDateInts(2024, 1, 3),
      "D",
      1);
  show_timeline("Timeline::now()", t_now);
  show_timeline("Timeline::today()", t_today);
  show_timeline("Timeline::yesterday()", t_yesterday);
  show_timeline("Timeline::tomorrow()", t_tomorrow);
  show_timeline("Timeline::fromTimestamp", t_from_ts);
  show_timeline("Timeline::fromSeries", t_from_series);
  show_timeline("Timeline::createFromDateInts", t_from_date);
  show_timeline("Timeline::createFromTimeInts", t_from_time);
  show_timeline("Timeline::createFromDateTimeInts", t_from_dt);
  std::cout << "\nTimeline::date_range count=" << t_static_range.size() << "\n";

  // Static dataframe-oriented constructors with 3D input.
  const auto y = pluss::make_series(std::vector<std::int64_t>{2024, 2024, 2024, 2024}, {1, 2, 2});
  const auto mo = pluss::make_series(std::vector<std::int64_t>{1, 1, 1, 1}, {1, 2, 2});
  const auto d = pluss::make_series(std::vector<std::int64_t>{10, 11, 12, 13}, {1, 2, 2});
  const auto h = pluss::make_series(std::vector<std::int64_t>{8, 9, 10, 11}, {1, 2, 2});
  const auto mi = pluss::make_series(std::vector<std::int64_t>{1, 2, 3, 4}, {1, 2, 2});
  const auto se = pluss::make_series(std::vector<std::int64_t>{5, 6, 7, 8}, {1, 2, 2});
  const auto t_from_ymd = Timeline::from_ymd(y, mo, d, "UTC");
  const auto t_from_ymdhms = Timeline::from_ymdhms(y, mo, d, h, mi, se, "UTC");
  show_timeline("Timeline::from_ymd(3D)", t_from_ymd);
  show_timeline("Timeline::from_ymdhms(3D)", t_from_ymdhms);

  const auto e_ns = t_from_ymdhms.epoch_nanos();
  const auto e_us = t_from_ymdhms.epoch_micros();
  const auto e_ms = t_from_ymdhms.epoch_millis();
  const auto e_s = t_from_ymdhms.epoch_seconds();
  show_series("epoch_nanos()", e_ns);
  show_series("epoch_micros()", e_us);
  show_series("epoch_millis()", e_ms);
  show_series("epoch_seconds()", e_s);

  show_timeline("from_epoch_micros()", Timeline::from_epoch_micros(e_us, "UTC"));
  show_timeline("from_epoch_millis()", Timeline::from_epoch_millis(e_ms, "UTC"));
  show_timeline("from_epoch_seconds()", Timeline::from_epoch_seconds(e_s, "UTC"));

  std::vector<std::int64_t> copied_nanos;
  t_from_ymdhms.copy_epoch_nanos_to(std::back_inserter(copied_nanos));
  const auto copied_vec = t_from_ymdhms.to_epoch_nanos_vector();
  std::cout << "\ncopy_epoch_nanos_to size=" << copied_nanos.size()
            << " to_epoch_nanos_vector size=" << copied_vec.size() << "\n";

  // --------------------------------------------------------------------------
  // Non-generated TimeIndex methods
  // --------------------------------------------------------------------------
  const auto idx = pluss::timeindex::make({
      "2024-11-01T00:00:00Z",
      "2024-11-01T00:20:00Z",
      "2024-11-01T00:40:00Z",
      "2024-11-01T01:00:00Z",
      "2024-11-01T01:20:00Z",
  });
  std::cout << "\n== TimeIndex non-generated methods ==\n";
  std::cout << "size=" << idx.size() << "\n";
  show_timeline("timeline()", idx.timeline());

  const auto key = pluss::timeline::make("2024-11-01T00:41:00Z");
  std::cout << "searchsorted(left)=" << idx.searchsorted(key, false)
            << " searchsorted(right)=" << idx.searchsorted(key, true) << "\n";
  std::cout << "asof(key)=" << idx.asof(key) << "\n";

  show_series("slice(start, end)",
              idx.slice(pluss::timeline::make("2024-11-01T00:20:00Z"),
                        pluss::timeline::make("2024-11-01T01:00:00Z")));
  show_series("slice_time(start, end)",
              idx.slice_time("2024-11-01T00:20:00Z", "2024-11-01T01:00:00Z"));

  const auto bins = idx.resample_bins("H", 1);
  show_timeline("resample_bins.labels", bins.labels);
  show_series("resample_bins.bin_id", bins.bin_id);
  show_series("resample_count", idx.resample_count("H", 1));

  const auto values = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5}, {5});
  show_series("resample_sum", idx.resample_sum(values, "H", 1));
  show_series("resample_mean", idx.resample_mean(values, "H", 1));
  show_series("resample_min", idx.resample_min(values, "H", 1));
  show_series("resample_max", idx.resample_max(values, "H", 1));
  show_series("resample_first", idx.resample_first(values, "H", 1));
  show_series("resample_last", idx.resample_last(values, "H", 1));
  show_series("resample_ohlc", idx.resample_ohlc(values, "H", 1));

  const auto win = pluss::duration::minutes(45);
  show_series("rolling_count", idx.rolling_count(win));
  show_series("rolling_sum", idx.rolling_sum(values, win));

  const auto idx_right = pluss::timeindex::make({
      "2024-11-01T00:05:00Z",
      "2024-11-01T00:35:00Z",
      "2024-11-01T01:05:00Z",
      "2024-11-01T01:35:00Z",
  });
  const auto join_back = idx.join_asof(idx_right, TimeIndex::AsofDirection::Backward, nullptr, true);
  const auto tol = pluss::duration::minutes(25);
  const auto join_near = idx.join_asof(idx_right, TimeIndex::AsofDirection::Nearest, &tol, true);
  show_series("join_asof(backward)", join_back);
  show_series("join_asof(nearest, tolerance=25m)", join_near);

  const auto aligned = idx.align_union(idx_right);
  show_timeline("align_union.index", aligned.index);
  show_series("align_union.left_indexer", aligned.left_indexer);
  show_series("align_union.right_indexer", aligned.right_indexer);

  return 0;
}
