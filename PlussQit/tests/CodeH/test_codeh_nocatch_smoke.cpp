#include <pluss/pluss.h>

#include <any>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

int g_failures = 0;

static_assert(!std::is_constructible_v<pluss::codeh::Timeline, pluss::codem::Series, std::string>,
              "Timeline should not be publicly constructible from raw core storage");
static_assert(!std::is_constructible_v<pluss::codeh::TimeIndex, pluss::codeh::Timeline>,
              "TimeIndex should not be publicly constructible from Timeline");

void expect_true(bool cond, const std::string& msg) {
  if (!cond) {
    ++g_failures;
    std::cerr << "[FAIL] " << msg << "\n";
  }
}

template <class T>
void expect_eq_vec(const std::vector<T>& got, const std::vector<T>& exp,
                   const std::string& msg) {
  if (got != exp) {
    ++g_failures;
    std::cerr << "[FAIL] " << msg << " (size=" << got.size()
              << " expected=" << exp.size() << ")\n";
  }
}

template <class Fn>
void expect_throw(Fn&& fn, const std::string& msg) {
  bool threw = false;
  try {
    fn();
  } catch (const pluss::Error&) {
    threw = true;
  } catch (...) {
    threw = true;
  }
  expect_true(threw, msg);
}

void test_generated_timeline_surface() {
  using pluss::codeh::Timeline;
  std::cout << "[RUN] codeh:generated_surface\n";

  const Timeline t = pluss::timeline::make("2026-02-15T13:04:05Z");
  expect_true(t.defined(), "moment scalar is defined");
  expect_true(t.year().to_vec<std::int32_t>(0) == 2026, "year() generated method");
  expect_true(t.month().to_vec<std::int32_t>(0) == 2, "month() generated method");
  expect_true(t.day().to_vec<std::int32_t>(0) == 15, "day() generated method");
  expect_true(t.hour().to_vec<std::int32_t>(0) == 13, "hour() generated method");
  expect_true(t.minute().to_vec<std::int32_t>(0) == 4, "minute() generated method");
  expect_true(t.second().to_vec<std::int32_t>(0) == 5, "second() generated method");

  const auto iso = t.isoformat();
  expect_true(iso.find("2026-02-15T13:04:05") != std::string::npos,
              "isoformat contains expected timestamp");

  const auto d = t.floor("D");
  expect_true(d.hour().to_vec<std::int32_t>(0) == 0, "floor(D) zeros hour");
  expect_true(d.minute().to_vec<std::int32_t>(0) == 0, "floor(D) zeros minute");
  expect_true(d.second().to_vec<std::int32_t>(0) == 0, "floor(D) zeros second");

  const auto plus2s = t.add_nanos(2'000'000'000LL);
  expect_true(t.seconds_between(plus2s) == 2,
              "add_nanos + base seconds_between");

  const auto weekdays = t.strftime("%A");
  expect_true(weekdays.defined(), "strftime() generated Note");
}

void test_base_static_and_legacy_surface() {
  using pluss::codeh::Timeline;
  std::cout << "[RUN] codeh:base_surface\n";

  const Timeline now = Timeline::now();
  const Timeline today = Timeline::today();
  const Timeline yesterday = Timeline::yesterday();
  const Timeline tomorrow = Timeline::tomorrow();

  expect_true(now.defined() && today.defined(), "Timeline::now/today defined");
  expect_true(today.days_between(tomorrow) == 1, "today->tomorrow day distance");
  expect_true(yesterday.days_between(today) == 1, "yesterday->today day distance");

  const Timeline leap =
      Timeline::createFromDateTimeInts(2024, 2, 29, 12, 0, 0);
  expect_true(leap.is_leap_year_scalar(), "scalar leap year predicate");
  expect_true(leap.year_scalar() == 2024, "year_scalar");
  expect_true(leap.month_scalar() == 2, "month_scalar");
  expect_true(leap.day_scalar() == 29, "day_scalar");
  expect_true(!leap.weekday_name(true).empty(), "base weekday_name(abbrev)");
  expect_true(!leap.month_name(true).empty(), "base month_name(abbrev)");

  const Timeline adjusted =
      leap.set_year(2025).set_month(3).set_day(1).set_hour(8).set_minute(9).set_second(10);
  expect_true(adjusted.year_scalar() == 2025, "set_year");
  expect_true(adjusted.month_scalar() == 3, "set_month");
  expect_true(adjusted.day_scalar() == 1, "set_day");
  expect_true(adjusted.hour_scalar() == 8, "set_hour");
  expect_true(adjusted.minute_scalar() == 9, "set_minute");
  expect_true(adjusted.second_scalar() == 10, "set_second");
  expect_true(adjusted.toISOString().find("2025-03-01T08:09:10") != std::string::npos,
              "toISOString base helper");

  const auto dr = Timeline::date_range(today, today.add_days(2), "D", 1);
  expect_true(dr.size() == 3, "base Timeline::date_range");
  const auto dr_desc = Timeline::date_range(today.add_days(2), today, "D", -1);
  expect_true(dr_desc.size() == 3, "base Timeline::date_range descending");

  const auto rep = today.repeat(3);
  expect_true(rep.numel() == 3, "repeat(3)");
  const auto cat = today.concat(tomorrow);
  expect_true(cat.numel() == 2, "concat scalar->1D");
  const auto filled = rep.fill_like(2030, 1, 2);
  expect_true(filled.numel() == 3, "fill_like keeps shape");

  const auto from_ts = Timeline::fromTimestamp(0);
  expect_true(from_ts.year_scalar() == 1970, "fromTimestamp epoch");

  const auto from_date = Timeline::createFromDateInts(2025, 12, 31);
  expect_true(from_date.year_scalar() == 2025, "createFromDateInts year");
  expect_true(from_date.hour_scalar() == 0, "createFromDateInts hour reset");

  const auto from_time = Timeline::createFromTimeInts(23, 59, 58);
  expect_true(from_time.hour_scalar() == 23, "createFromTimeInts hour");
  expect_true(from_time.minute_scalar() == 59, "createFromTimeInts minute");
  expect_true(from_time.second_scalar() == 58, "createFromTimeInts second");

  const auto e = pluss::make_series(std::vector<std::int64_t>{0, 1'000'000'000LL}, {2});
  const auto from_series = Timeline::fromSeries(e);
  expect_eq_vec(from_series.to_epoch_nanos_flat(), std::vector<std::int64_t>{0, 1'000'000'000LL},
                "fromSeries epoch nanos");
}

void test_dataframe_datetime_tranche() {
  using pluss::codeh::Timeline;
  std::cout << "[RUN] codeh:datetime_tranche\n";

  const auto years = pluss::make_series(std::vector<std::int64_t>{2024, 2024, 2024}, {3});
  const auto months = pluss::make_series(std::vector<std::int64_t>{1, 1, 1}, {3});
  const auto days = pluss::make_series(std::vector<std::int64_t>{2, 3, 4}, {3});
  const auto hours = pluss::make_series(std::vector<std::int64_t>{10, 11, 12}, {3});
  const auto mins = pluss::make_series(std::vector<std::int64_t>{5, 6, 7}, {3});
  const auto secs = pluss::make_series(std::vector<std::int64_t>{8, 9, 10}, {3});

  const Timeline ymd = Timeline::from_ymd(years, months, days, "UTC");
  expect_true(ymd.numel() == 3, "from_ymd builds 1D timeline");
  expect_eq_vec(ymd.year().to_vec<std::int32_t>(), {2024, 2024, 2024},
                "from_ymd year fields");
  expect_eq_vec(ymd.day().to_vec<std::int32_t>(), {2, 3, 4},
                "from_ymd day fields");

  const Timeline ymdhms =
      Timeline::from_ymdhms(years, months, days, hours, mins, secs, "UTC");
  expect_eq_vec(ymdhms.hour().to_vec<std::int32_t>(), {10, 11, 12},
                "from_ymdhms hour fields");
  expect_eq_vec(ymdhms.minute().to_vec<std::int32_t>(), {5, 6, 7},
                "from_ymdhms minute fields");
  expect_eq_vec(ymdhms.second().to_vec<std::int32_t>(), {8, 9, 10},
                "from_ymdhms second fields");

  const auto epoch_s = ymdhms.epoch_seconds();
  const auto epoch_ms = ymdhms.epoch_millis();
  const auto epoch_us = ymdhms.epoch_micros();
  const auto epoch_ns = ymdhms.epoch_nanos();
  expect_true(epoch_s.numel() == 3 && epoch_ns.numel() == 3,
              "epoch_* conversions preserve shape");

  const auto rt_s = Timeline::from_epoch_seconds(epoch_s, "UTC");
  const auto rt_ms = Timeline::from_epoch_millis(epoch_ms, "UTC");
  const auto rt_us = Timeline::from_epoch_micros(epoch_us, "UTC");
  expect_eq_vec(rt_s.to_epoch_nanos_flat(), ymdhms.to_epoch_nanos_flat(),
                "from_epoch_seconds roundtrip");
  expect_eq_vec(rt_ms.to_epoch_nanos_flat(), ymdhms.to_epoch_nanos_flat(),
                "from_epoch_millis roundtrip");
  expect_eq_vec(rt_us.to_epoch_nanos_flat(), ymdhms.to_epoch_nanos_flat(),
                "from_epoch_micros roundtrip");

  const auto idx = ymdhms.sort().to_timeindex(true);
  expect_true(idx.size() == 3, "to_timeindex bridge");
  const auto key = pluss::timeline::make("2024-01-03T11:06:09Z");
  const auto pos = idx.searchsorted(key, false);
  expect_true(pos >= 1 && pos <= 2, "TimeIndex searchsorted on Timeline-built index");
}

void test_factory_rank_and_printing() {
  using pluss::codeh::Timeline;
  std::cout << "[RUN] codeh:factory_rank_print\n";

  const auto t2 = pluss::timeline::make({
      {"2024-01-01T00:00:00Z", "2024-01-01T01:00:00Z"},
      {"2024-01-02T00:00:00Z", "2024-01-02T01:00:00Z"},
  });
  expect_true(t2.dim() == 2, "moment 2D rank");
  expect_eq_vec(t2.day().to_vec<std::int32_t>(), std::vector<std::int32_t>{1, 1, 2, 2},
                "moment 2D flatten order");

  const auto t3 = pluss::timeline::make({
      {
          {"2024-01-01T00:00:00Z", "2024-01-01T01:00:00Z"},
          {"2024-01-01T02:00:00Z", "2024-01-01T03:00:00Z"},
      },
      {
          {"2024-01-02T00:00:00Z", "2024-01-02T01:00:00Z"},
          {"2024-01-02T02:00:00Z", "2024-01-02T03:00:00Z"},
      },
  });
  expect_true(t3.dim() == 3, "moment 3D rank");
  expect_true(t3.numel() == 8, "moment 3D numel");

  const std::vector<std::string> lvalue_flat{
      "2024-02-01T00:00:00Z", "2024-02-01T01:00:00Z",
      "2024-02-02T00:00:00Z", "2024-02-02T01:00:00Z"};
  const std::vector<std::int64_t> lvalue_shape{2, 2};
  const auto t_lvalue = pluss::timeline::make(lvalue_flat, lvalue_shape);
  expect_true(t_lvalue.dim() == 2, "make_timeline lvalue flat+shape rank");
  expect_true(t_lvalue.month().to_vec<std::int32_t>(0) == 2, "make_timeline lvalue parse");

  const std::vector<std::int64_t> epoch_flat{
      0, 1'000'000'000LL, 2'000'000'000LL, 3'000'000'000LL,
      4'000'000'000LL, 5'000'000'000LL
  };
  const std::vector<std::int64_t> epoch_shape{1, 2, 3};
  const auto t_epoch = pluss::timeline::from_epoch(epoch_flat, epoch_shape);
  expect_true(t_epoch.dim() == 3, "from_epoch_nanos lvalue rank");
  expect_eq_vec(t_epoch.to_epoch_nanos_flat(), epoch_flat,
                "from_epoch_nanos lvalue preserves data");

  bool timeline_freed = false;
  {
    auto* raw = static_cast<std::int64_t*>(std::malloc(sizeof(std::int64_t) * epoch_flat.size()));
    std::memcpy(raw, epoch_flat.data(), sizeof(std::int64_t) * epoch_flat.size());
    const std::array<std::int64_t, 3> epoch_strides{6, 3, 1};
    const auto t_epoch_buf = pluss::codeh::Timeline::from_buffer_epoch_nanos(
        raw,
        std::span<const std::int64_t>(epoch_shape.data(), epoch_shape.size()),
        std::span<const std::int64_t>(epoch_strides.data(), epoch_strides.size()),
        [&](void* p) {
          timeline_freed = true;
          std::free(p);
        });
    expect_true(t_epoch_buf.dim() == 3, "Timeline::from_buffer_epoch_nanos rank");
    expect_eq_vec(t_epoch_buf.to_epoch_nanos_vector(), epoch_flat,
                  "Timeline::from_buffer_epoch_nanos preserves data");
  }
  expect_true(timeline_freed, "Timeline::from_buffer_epoch_nanos deleter called");

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
  expect_true(t_epoch_nested.dim() == 3, "from_epoch_nanos nested rank");
  expect_true(t_epoch_nested.numel() == 8, "from_epoch_nanos nested numel");

  const auto idx = pluss::timeindex::make({
      "2024-01-01T00:00:00Z",
      "2024-01-01T01:00:00Z",
      "2024-01-01T02:00:00Z",
  });
  expect_true(idx.size() == 3, "make_timeindex 1D literal factory");

  bool timeindex_freed = false;
  {
    auto* raw = static_cast<std::int64_t*>(std::malloc(sizeof(std::int64_t) * 3));
    raw[0] = 0;
    raw[1] = 1'000'000'000LL;
    raw[2] = 2'000'000'000LL;
    const auto idx_buf = pluss::codeh::TimeIndex::from_buffer_epoch_nanos(
        raw,
        3,
        1,
        [&](void* p) {
          timeindex_freed = true;
          std::free(p);
        },
        true);
    expect_true(idx_buf.size() == 3, "TimeIndex::from_buffer_epoch_nanos size");
    expect_eq_vec(idx_buf.timeline().to_epoch_nanos_vector(),
                  std::vector<std::int64_t>{0, 1'000'000'000LL, 2'000'000'000LL},
                  "TimeIndex::from_buffer_epoch_nanos preserves data");
  }
  expect_true(timeindex_freed, "TimeIndex::from_buffer_epoch_nanos deleter called");

  pluss::codeh::TimelinePrintOptions popt;
  popt.max_rows = 4;
  popt.max_cols = 4;
  popt.max_slices = 4;
  const auto s2 = pluss::timeline::print(t2, popt);
  expect_true(s2.find("Timeline(shape=[2, 2]") != std::string::npos, "print_timeline 2D header");
  expect_true(s2.find("2024-01-02 01:00:00") != std::string::npos, "print_timeline 2D value");
  const auto s3 = pluss::timeline::print(t3, popt);
  expect_true(s3.find("values[0, :, :]") != std::string::npos, "print_timeline 3D slice header");
  expect_true(s3.find("2024-01-02 03:00:00") != std::string::npos, "print_timeline 3D value");

  expect_throw(
      [] {
        (void)pluss::timeline::make({
            {"2024-01-01T00:00:00Z"},
            {"2024-01-01T01:00:00Z", "2024-01-01T02:00:00Z"},
        });
      },
      "moment 2D rejects ragged input");
  expect_throw(
      [] {
        (void)pluss::timeindex::make({
            {"2024-01-01T00:00:00Z", "2024-01-01T01:00:00Z"},
            {"2024-01-02T00:00:00Z", "2024-01-02T01:00:00Z"},
        });
      },
      "make_timeindex rejects 2D nested input");
}

void test_generated_timeindex_dataframe_family() {
  using pluss::codeh::Timeline;
  std::cout << "[RUN] codeh:timeindex_generated_df\n";

  const auto t = pluss::timeline::make({
      "2024-01-01T00:00:00Z",
      "2024-01-01T00:30:00Z",
      "2024-01-01T01:00:00Z",
      "2024-01-01T01:30:00Z",
  });
  const auto idx = t.to_timeindex(true);

  expect_true(idx.is_monotonic_increasing(), "generated is_monotonic_increasing");
  expect_true(idx.is_unique(), "generated is_unique");
  expect_true(!idx.has_duplicates(), "generated has_duplicates");
  expect_true(idx.freq() == "min", "generated infer_freq alias");

  const auto values = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4});
  const auto rs = idx.resample_agg(values, "H", "sum", 1);
  expect_eq_vec(rs.to_vec<double>(), std::vector<double>{3.0, 7.0},
                "generated resample_agg(sum)");

  const auto window = pluss::duration::hours(1);
  const auto roll = idx.rolling_agg(values, window, "sum");
  expect_eq_vec(roll.to_vec<double>(), std::vector<double>{1.0, 3.0, 6.0, 9.0},
                "generated rolling_agg(sum)");

  const auto sorted = idx.sort_values();
  expect_true(sorted.size() == idx.size(), "generated sort_values alias");

  const auto ticks_with_nat = pluss::make_series(
      std::vector<std::int64_t>{
          30'000'000'000LL,
          Timeline::kNaT,
          10'000'000'000LL,
          Timeline::kNaT,
          20'000'000'000LL,
      },
      {5});
  const auto t_nat = Timeline::fromSeries(ticks_with_nat);
  expect_eq_vec(
      t_nat.argsort().to_vec<std::int64_t>(),
      std::vector<std::int64_t>{2, 4, 0, 1, 3},
      "Timeline argsort keeps NaT last and preserves NaT stability");
  expect_eq_vec(
      t_nat.sort().to_epoch_nanos_flat(),
      std::vector<std::int64_t>{
          10'000'000'000LL,
          20'000'000'000LL,
          30'000'000'000LL,
          Timeline::kNaT,
          Timeline::kNaT,
      },
      "Timeline sort keeps NaT last");
}

void test_3d_non_generated_timeline_support() {
  std::cout << "[RUN] codeh:timeline_3d_non_generated\n";

  const auto t3 = pluss::timeline::make({
      {
          {"2024-01-01T00:00:00Z", "2024-01-01T01:00:00Z"},
          {"2024-01-01T02:00:00Z", "2024-01-01T03:00:00Z"},
      },
      {
          {"2024-01-02T00:00:00Z", "2024-01-02T01:00:00Z"},
          {"2024-01-02T02:00:00Z", "2024-01-02T03:00:00Z"},
      },
  });
  expect_true(t3.dim() == 3, "3D fixture rank");

  const auto y = t3.set_year(2030);
  const auto mo = t3.set_month(12);
  const auto d = t3.set_day(15);
  const auto h = t3.set_hour(7);
  const auto mi = t3.set_minute(8);
  const auto s = t3.set_second(9);
  expect_true(y.dim() == 3 && mo.dim() == 3 && d.dim() == 3 &&
              h.dim() == 3 && mi.dim() == 3 && s.dim() == 3,
              "set_* keep 3D rank");
  expect_true(y.year().to_vec<std::int32_t>(0) == 2030, "set_year(3D)");
  expect_true(mo.month().to_vec<std::int32_t>(0) == 12, "set_month(3D)");
  expect_true(d.day().to_vec<std::int32_t>(0) == 15, "set_day(3D)");
  expect_true(h.hour().to_vec<std::int32_t>(0) == 7, "set_hour(3D)");
  expect_true(mi.minute().to_vec<std::int32_t>(0) == 8, "set_minute(3D)");
  expect_true(s.second().to_vec<std::int32_t>(0) == 9, "set_second(3D)");

  const auto y_period = t3.to_period("year");
  const auto m_period = t3.to_period("month");
  expect_true(y_period.dim() == 3 && m_period.dim() == 3, "to_period keeps 3D rank");
  expect_true(y_period.month().to_vec<std::int32_t>(0) == 1 &&
              y_period.day().to_vec<std::int32_t>(0) == 1,
              "to_period(year) normalized components");
  expect_true(m_period.day().to_vec<std::int32_t>(0) == 1, "to_period(month) normalized day");

  const auto t3_plus = t3.add_seconds(1);
  expect_true(t3 < t3_plus, "operator< supports 3D");
  expect_true(t3 <= t3_plus, "operator<= supports 3D");
  expect_true(t3_plus > t3, "operator> supports 3D");
  expect_true(t3_plus >= t3, "operator>= supports 3D");
}

void test_queue_dynamic_dispatch() {
  std::cout << "[RUN] codeh:queue_dynamic_dispatch\n";

  pluss::Type qt = pluss::timeline::make("2024-01-15T10:11:12Z");
  expect_true(qt.as_timeline().year().to_vec<std::int32_t>(0) == 2024,
              "Type.as_timeline() exposes Timeline surface");

  const auto qf = qt.invoke("floor", {"D"});
  expect_true(qf.kind() == pluss::Type::Kind::Timeline, "Type floor returns Timeline");
  expect_true(qf.as<pluss::codeh::Timeline>().hour().to_vec<std::int32_t>(0) == 0,
              "Type floor(D) zeroes hour");

  const auto qy = qt.invoke("year");
  expect_true(qy.kind() == pluss::Type::Kind::Series, "Type year returns Series");
  expect_true(qy.as<pluss::Series>().to_vec<std::int32_t>(0) == 2024,
              "Type year value");

  const auto qn = qt.invoke("strftime", {"%Y-%m-%d"});
  expect_true(qn.kind() == pluss::Type::Kind::Note, "Type strftime returns Note");
  expect_true(qn.as<pluss::Note>().at(0) == "2024-01-15", "Type strftime formatted value");

  pluss::Type qt1d = pluss::timeline::make({
      "2024-01-15T10:11:12Z",
      "2024-01-16T10:11:12Z",
  });
  const auto qidx = qt1d.invoke("to_timeindex", {true});
  expect_true(qidx.kind() == pluss::Type::Kind::TimeIndex, "Type to_timeindex returns TimeIndex");
  expect_true(qidx.as<pluss::codeh::TimeIndex>().size() == 2, "Type to_timeindex size");
  expect_true(qidx.as_timeindex().size() == 2, "Type.as_timeindex() exposes TimeIndex surface");

  const auto qidx_sorted = qidx.invoke("sort_values");
  expect_true(qidx_sorted.kind() == pluss::Type::Kind::TimeIndex, "Type TimeIndex sort_values returns TimeIndex");
}

void test_dynamic_dispense_utility() {
  std::cout << "[RUN] codeh:dynamic_dispense_utility\n";

  auto s = pluss::make_series({-1.0, 2.0, -3.0});
  const auto s_numel = pluss::dispense(s, &pluss::Series::numel, std::make_tuple());
  expect_true(s_numel == 3,
              "dispense(Series,&Series::numel)");

  auto n = pluss::make_note({"apple", "banana", "apricot"});
  const auto starts_with_a = pluss::dispense(n, &pluss::Note::starts_with, std::make_tuple("a"));
  expect_true(starts_with_a.size() == 3 && starts_with_a[0] == 1 && starts_with_a[1] == 0 && starts_with_a[2] == 1,
              "dispense(Note,&Note::starts_with)");

  auto t = pluss::timeline::make("2024-01-15T10:11:12Z");
  const auto floor_d = pluss::dispense(t, &pluss::codeh::Timeline::floor, std::make_tuple("D"));
  expect_true(floor_d.hour().to_vec<std::int32_t>(0) == 0,
              "dispense(Timeline,&Timeline::floor)");

  const auto year_series = pluss::dispense(t, &pluss::codeh::Timeline::year, std::make_tuple());
  expect_true(year_series.to_vec<std::int32_t>(0) == 2024,
              "dispense(Timeline,&Timeline::year)");

  const std::any any_year = pluss::dispense(t, &pluss::codeh::Timeline::year, pluss::args{});
  const auto& any_year_series = std::any_cast<const pluss::Series&>(any_year);
  expect_true(any_year_series.to_vec<std::int32_t>(0) == 2024,
              "dispense(any Timeline::year) value");

  const std::any any_lower = pluss::dispense(n, &pluss::Note::lower, pluss::args{});
  const auto& any_lower_note = std::any_cast<const pluss::Note&>(any_lower);
  expect_true(any_lower_note.at(0) == "apple",
              "dispense(any Note::lower) payload");
}

} // namespace

int main() {
  test_generated_timeline_surface();
  test_base_static_and_legacy_surface();
  test_dataframe_datetime_tranche();
  test_factory_rank_and_printing();
  test_generated_timeindex_dataframe_family();
  test_3d_non_generated_timeline_support();
  test_queue_dynamic_dispatch();
  test_dynamic_dispense_utility();

  if (g_failures != 0) {
    std::cerr << "[CodeH no-catch] failures=" << g_failures << "\n";
    return 1;
  }
  std::cout << "[CodeH no-catch] all good\n";
  return 0;
}
