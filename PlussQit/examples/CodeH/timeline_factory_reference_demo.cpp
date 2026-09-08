#include <pluss/pluss.h>

#include <chrono>
#include <ctime>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {
void show(std::string_view label, const pluss::Timeline& value) {
  std::cout << "\n=== " << label << " ===\n" << pluss::timeline::print(value) << '\n';
}

void show(std::string_view label, const pluss::Note& value) {
  std::cout << "\n=== " << label << " ===\n" << pluss::note::print(value) << '\n';
}

void show(std::string_view label, const pluss::Series& value) {
  std::cout << "\n=== " << label << " ===\n" << pluss::series::print(value) << '\n';
}

void show_exact(std::string_view label, const pluss::Timeline& value,
                std::string_view pattern = "%FT%T.%f%z") {
  show(label, pluss::timeline::format(value, pattern));
}

void show_check(std::string_view label, bool value) {
  std::cout << "\n=== " << label << " ===\n" << std::boolalpha << value << '\n';
}

void show_text(std::string_view label, const std::string& value) {
  std::cout << "\n=== " << label << " ===\n" << value << '\n';
}

template <class Fn>
void show_rejection(std::string_view label, Fn&& fn) {
  try {
    fn();
  } catch (const pluss::Error&) {
    std::cout << "\n=== " << label << " ===\nrejected\n";
  }
}
} // namespace

int main() {
  show("make", pluss::timeline::make({{"2024-02-29T23:30:45.123456789Z", "2024-03-01T00:00:00Z"},
                                      {"2024-07-01T06:00:00Z", "1970-01-01T00:00:00Z"}}));
  show("date", pluss::timeline::date({"2024-02-29", "2024-03-01"}));
  show("time", pluss::timeline::time({"09:30:00.250", "17:45:30"}));
  show("from_epoch", pluss::timeline::from_epoch({-1LL, 0LL, 1'234'567'890LL}, "UTC"));

  const auto start = pluss::timeline::make("2024-01-01T00:00:00Z");
  const auto end = pluss::timeline::make("2024-01-01T06:00:00Z");
  show("date_range", pluss::timeline::date_range(start, end, "H", 2));
  show("arange", pluss::timeline::arange(start, end, pluss::duration::hours(2), false));
  show("unite", pluss::timeline::unite({pluss::timeline::date({"2024-01-01", "2024-01-02"}),
                                         pluss::timeline::date("2024-01-03")}));
  show("epoch", pluss::timeline::epoch({2, 2}));
  show("full", pluss::timeline::full({2, 2}, pluss::timeline::make("2024-02-29T12:00:00Z")));
  show("full_like", pluss::timeline::full_like(pluss::timeline::epoch({2, 1}),
                                                pluss::timeline::date("2024-06-01")));
  show("full_time", pluss::timeline::full(
      {2, 2}, pluss::timeline::time("09:30:00")));
  const auto random = pluss::timeline::random(
      {4}, pluss::timeline::from_epoch(0LL), pluss::timeline::from_epoch(10LL), 7);
  show("random", random);
  show("random_epoch_nanos", random.epoch_nanos());
  show("rand_between", pluss::timeline::rand_between("2024-01-01", "2024-01-03", {5}, 7));

  const auto condition = pluss::series::make({true, false, false});
  show("select", pluss::timeline::select(condition, "2024-12-25", "2024-01-01"));

  const auto formatted_source = pluss::timeline::from_epoch({
      1'709'249'445'120'000'000LL, 1'719'813'600'000'000'001LL,
      pluss::Timeline::kNaT});
  auto options = pluss::timeline::FormatOptions{};
  options.pattern = "%A, %F %R %Z (%z), day %j, Q%q, fraction=%f";
  options.timezone = "America/Edmonton";
  options.nat = "not-recorded";
  options.fractional_digits = 6;
  options.trim_fractional_zeros = true;
  show("format", pluss::timeline::format(formatted_source, options));

  show("moment_strict", pluss::timeline::moment(
      "2024-02-29T12:34:56Z",
      {pluss::timeline::MomentMode::Strict, pluss::timeline::MomentClock::UTC}));

  // Pass 1: text, exact epoch, and regular-range factories.
  show_exact("make_scalar_offset",
             pluss::timeline::make("2024-02-29T23:30:45.123456789-07:00"));
  show_exact("make_vector_forms", pluss::timeline::make({
      "2024-02-29", "2024/03/01 06:15", "2024-03-01T08:15:00+0200", ""}));
  show_exact("make_nested_matrix", pluss::timeline::make({
      {"2024-01-01", "2024-01-02T03:04:05Z"},
      {"2024-01-03T06:07Z", "2024-01-04T08:09:10.5Z"}}));
  const std::vector<std::string> make_flat{
      "2024-04-01", "2024-04-02", "2024-04-03", "2024-04-04"};
  show_exact("make_flat_shape", pluss::timeline::make(make_flat, {1, 2, 2}));
  show("make_offsets_same_instant", pluss::timeline::make({
      "2024-01-01T00:00:00Z", "2023-12-31T17:00:00-07:00"}).epoch_nanos());
  show_rejection("make_invalid_date", [] {
    (void)pluss::timeline::make("2023-02-29");
  });
  show_rejection("make_invalid_offset", [] {
    (void)pluss::timeline::make("2024-01-01T00:00:00+24:00");
  });
  show_rejection("make_shape_mismatch", [] {
    const std::vector<std::string> values{"2024-01-01", "2024-01-02"};
    (void)pluss::timeline::make(values, {2, 2});
  });

  show_exact("date_scalar", pluss::timeline::date("2000-02-29"));
  show_exact("date_vector", pluss::timeline::date({"2024-02-29", "2024/03/01"}));
  show_exact("date_nested", pluss::timeline::date({
      {"2024-01-01", "2024-01-02"}, {"2024-02-01", "2024-02-02"}}));
  const std::vector<std::string_view> date_flat{
      "2024-05-01", "2024-05-02", "2024-05-03", "2024-05-04"};
  show_exact("date_flat_shape", pluss::timeline::date(date_flat, {2, 2}));
  show("date_empty_shape", pluss::timeline::date(
      std::vector<std::string_view>{}, {2, 0}));
  show_rejection("date_rejects_clock", [] {
    (void)pluss::timeline::date("2024-06-10T09:30:00Z");
  });

  show_exact("time_scalar_nanos", pluss::timeline::time("00:00:00.000000001"));
  show_exact("time_vector", pluss::timeline::time({"09:30", "17:45:30.250"}));
  show_exact("time_with_offset", pluss::timeline::time("09:30:00-07:00"));
  const std::vector<std::string> time_flat{
      "00:00:00", "06:00:00", "12:00:00", "18:00:00"};
  show_exact("time_flat_shape", pluss::timeline::time(time_flat, {2, 2}));
  show_rejection("time_rejects_date", [] {
    (void)pluss::timeline::time("2024-06-10");
  });
  show_rejection("time_rejects_leap_second", [] {
    (void)pluss::timeline::time("23:59:60");
  });

  constexpr std::int64_t exact_tick = 9'007'199'254'740'993LL;
  show("from_epoch_exact_scalar", pluss::timeline::from_epoch(exact_tick).epoch_nanos());
  show_exact("from_epoch_negative_and_nat",
             pluss::timeline::from_epoch({-1LL, 0LL, pluss::Timeline::kNaT}));
  show("from_epoch_nested", pluss::timeline::from_epoch(
      {{0LL, 1LL}, {2LL, 3LL}}).epoch_nanos());
  show_exact("from_epoch_timezone",
             pluss::timeline::from_epoch(0LL, "America/Edmonton"));
  const std::vector<std::int64_t> epoch_flat{0, 1, 2, 3};
  show("from_epoch_flat_shape",
       pluss::timeline::from_epoch(epoch_flat, {1, 2, 2}).epoch_nanos());
  show_rejection("from_epoch_unknown_timezone", [] {
    (void)pluss::timeline::from_epoch(0LL, "Mars/Olympus_Mons");
  });

  show_exact("date_range_default_step", pluss::timeline::date_range(
      pluss::timeline::make("2024-01-01T00:00:00Z"),
      pluss::timeline::make("2024-01-01T00:00:03Z"), "S"));
  show_exact("date_range_step_two", pluss::timeline::date_range(
      pluss::timeline::date("2024-01-01"),
      pluss::timeline::date("2024-01-06"), "D", 2));
  show_exact("date_range_descending", pluss::timeline::date_range(
      pluss::timeline::date("2024-01-05"),
      pluss::timeline::date("2024-01-01"), "D", -2));
  show("date_range_wrong_direction", pluss::timeline::date_range(
      pluss::timeline::date("2024-01-05"),
      pluss::timeline::date("2024-01-01"), "D", 1));

  const auto ny_day_start = pluss::timeline::make("2024-03-09T14:30:00Z")
                                .tz_convert("America/New_York");
  const auto ny_day_end = pluss::timeline::make("2024-03-11T13:30:00Z")
                              .tz_convert("America/New_York");
  const auto ny_days = pluss::timeline::date_range(ny_day_start, ny_day_end, "D");
  show_exact("date_range_calendar_days", ny_days);
  const auto ny_day_ticks = ny_days.ticks();
  show("date_range_calendar_day_deltas", pluss::make_series(
      std::vector<std::int64_t>{ny_day_ticks[1] - ny_day_ticks[0],
                                ny_day_ticks[2] - ny_day_ticks[1]}));

  const auto ny_hour_start = pluss::timeline::make("2024-03-10T05:00:00Z")
                                 .tz_convert("America/New_York");
  const auto ny_hour_end = pluss::timeline::make("2024-03-10T08:00:00Z")
                               .tz_convert("America/New_York");
  show_exact("date_range_elapsed_hours",
             pluss::timeline::date_range(ny_hour_start, ny_hour_end, "H"));
  show_rejection("date_range_zero_step", [&] {
    (void)pluss::timeline::date_range(start, end, "H", 0);
  });
  show_rejection("date_range_non_scalar", [&] {
    (void)pluss::timeline::date_range(pluss::timeline::date({"2024-01-01"}), end,
                                      "D");
  });

  // Pass 2: exact ranges, shaped allocation, and structured formatting.
  show_exact("arange_timeline_open", pluss::timeline::arange(
      pluss::timeline::from_epoch(0LL), pluss::timeline::from_epoch(7LL),
      pluss::duration::nanos(2), false));
  show_exact("arange_date_integer", pluss::timeline::arange(
      "2024-02-27", "2024-03-02", 2));
  show_exact("arange_time_integer_open", pluss::timeline::arange(
      "09:30:00", "09:30:03", 1, false));
  show_exact("arange_descending", pluss::timeline::arange(
      "2024-03-03", "2024-02-28", -2));
  show("arange_wrong_direction", pluss::timeline::arange(
      "2024-03-03", "2024-02-28", 2));
  show_exact("arange_elapsed_dst", pluss::timeline::arange(
      pluss::timeline::make("2024-03-10T05:00:00Z").tz_convert("America/New_York"),
      pluss::timeline::make("2024-03-10T08:00:00Z").tz_convert("America/New_York"),
      pluss::duration::hours(1)));
  show_rejection("arange_zero_step", [&] {
    (void)pluss::timeline::arange(start, end, pluss::duration::nanos(0));
  });
  show_rejection("arange_nat_step", [&] {
    (void)pluss::timeline::arange(start, end,
                                  pluss::duration::nanos(pluss::Duration::kNaT));
  });
  show_rejection("arange_mixed_text", [] {
    (void)pluss::timeline::arange("2024-01-01", "09:30:00", 1);
  });

  show_exact("epoch_scalar", pluss::timeline::epoch());
  show("epoch_vector_ticks", pluss::timeline::epoch({4}).epoch_nanos());
  show("epoch_cube_ticks", pluss::timeline::epoch({1, 2, 2}).epoch_nanos());
  show("epoch_empty", pluss::timeline::epoch({2, 0}));
  show_rejection("epoch_negative_extent", [] {
    (void)pluss::timeline::epoch({2, -1});
  });
  show_rejection("epoch_rank_four", [] {
    (void)pluss::timeline::epoch({1, 1, 1, 1});
  });

  const auto format_matrix = pluss::timeline::make({
      {"2024-02-29T23:30:45.123456789Z", "2024-03-01T00:00:00Z"},
      {"2024-07-01T06:00:00Z", ""}});
  show("format_default", pluss::timeline::format(format_matrix));
  show("format_pattern_overload", pluss::timeline::format(
      format_matrix, "%a %b %d, day=%j weekday=%u/%w quarter=Q%q"));

  auto zone_options = pluss::timeline::FormatOptions{};
  zone_options.pattern = "%F %T %Z %z";
  zone_options.timezone = "America/New_York";
  show("format_timezone_per_instant", pluss::timeline::format(
      pluss::timeline::make({"2024-01-15T12:00:00Z", "2024-07-15T12:00:00Z"}),
      zone_options));

  auto fraction_options = pluss::timeline::FormatOptions{};
  fraction_options.pattern = "%T.%f | %N";
  fraction_options.fractional_digits = 6;
  fraction_options.trim_fractional_zeros = true;
  show("format_fraction_policy", pluss::timeline::format(
      pluss::timeline::make({"2024-01-01T00:00:00.120000000Z",
                             "2024-01-01T00:00:00.123456789Z"}),
      fraction_options));

  auto nat_options = pluss::timeline::FormatOptions{};
  nat_options.pattern = "observed=%F %% %T";
  nat_options.nat = "not-observed";
  show("format_custom_nat", pluss::timeline::format(
      pluss::timeline::make({"", "2024-01-02T03:04:05Z"}), nat_options));
  show("format_empty_shape", pluss::timeline::format(
      pluss::timeline::from_epoch(std::vector<std::int64_t>{}, {2, 0}), "%F"));
  show_rejection("format_bad_directive", [&] {
    (void)pluss::timeline::format(format_matrix, "%Y-%Q");
  });
  show_rejection("format_bad_precision", [&] {
    auto bad = pluss::timeline::FormatOptions{};
    bad.fractional_digits = 0;
    (void)pluss::timeline::format(format_matrix, bad);
  });

  const auto precise_fill = pluss::timeline::from_epoch(
      1'709'208'000'123'456'789LL, "America/Edmonton");
  show_exact("full_scalar_shape", pluss::timeline::full({}, precise_fill));
  show_exact("full_matrix", pluss::timeline::full({2, 2}, precise_fill));
  show("full_exact_ticks", pluss::timeline::full({3}, precise_fill).epoch_nanos());
  show_exact("full_nat", pluss::timeline::full(
      {2}, pluss::timeline::from_epoch(pluss::Timeline::kNaT)));
  show("full_empty", pluss::timeline::full({2, 0}, precise_fill));
  show_rejection("full_non_scalar_fill", [] {
    (void)pluss::timeline::full({2}, pluss::timeline::epoch({1}));
  });

  const auto shape_only = pluss::timeline::make({
      {"2020-01-01", "2021-01-01"}, {"2022-01-01", "2023-01-01"}});
  show_exact("full_like_values_ignored", pluss::timeline::full_like(
      shape_only, pluss::timeline::make("1999-12-31T23:00:00Z")));
  show_exact("full_like_fill_timezone_wins", pluss::timeline::full_like(
      pluss::timeline::from_epoch({0LL, 1LL}, "UTC"), precise_fill));
  show("full_like_cube_ticks", pluss::timeline::full_like(
      pluss::timeline::epoch({1, 2, 2}), pluss::timeline::from_epoch(7LL))
                                    .epoch_nanos());
  show("full_like_empty", pluss::timeline::full_like(
      pluss::timeline::epoch({0, 3}), precise_fill));
  show_rejection("full_like_non_scalar_fill", [] {
    (void)pluss::timeline::full_like(pluss::timeline::epoch({2}),
                                     pluss::timeline::epoch({1}));
  });

  // Pass 3: flat construction, live boundaries, printing, and closed sampling.
  show_exact("make_timeline_scalar", pluss::make_timeline(
      "2024-02-29T23:30:45.123456789-07:00"));
  const std::vector<std::string> alias_flat{
      "2024-01-01", "2024-01-02", "2024-01-03", "2024-01-04"};
  show("make_timeline_matrix", pluss::timeline::format(
      pluss::make_timeline(alias_flat, {2, 2}), "%F"));

  show_exact("moment_strict_scalar", pluss::timeline::moment(
      "2024-02-29T12:34:56.123456789Z",
      {pluss::timeline::MomentMode::Strict, pluss::timeline::MomentClock::UTC}));
  show_exact("moment_strict_vector", pluss::timeline::moment(
      {"2024-01-01T00:00:00Z", "2024-06-30T23:59:59Z"},
      {pluss::timeline::MomentMode::Strict, pluss::timeline::MomentClock::UTC}));
  const auto utc_today = pluss::timeline::moment(
      "today", {pluss::timeline::MomentMode::ExpressionOnly,
                 pluss::timeline::MomentClock::UTC});
  const auto utc_today_plus = pluss::timeline::moment(
      "today+3h", {pluss::timeline::MomentMode::ExpressionOnly,
                    pluss::timeline::MomentClock::UTC});
  show_check("moment_today_is_utc_midnight",
             utc_today.epoch_nanos().item<std::int64_t>() %
                     (86'400LL * 1'000'000'000LL) ==
                 0);
  show_check("moment_offset_is_exact",
             utc_today_plus.epoch_nanos().item<std::int64_t>() -
                     utc_today.epoch_nanos().item<std::int64_t>() ==
                 3'600LL * 1'000'000'000LL * 3);
  const auto yesterday = pluss::timeline::moment(
      "yesterday", {pluss::timeline::MomentMode::ExpressionOnly,
                     pluss::timeline::MomentClock::UTC});
  const auto tomorrow = pluss::timeline::moment(
      "tomorrow", {pluss::timeline::MomentMode::ExpressionOnly,
                    pluss::timeline::MomentClock::UTC});
  const auto month_start = pluss::timeline::moment(
      "start_of_month", {pluss::timeline::MomentMode::ExpressionOnly,
                          pluss::timeline::MomentClock::UTC});
  const auto month_end = pluss::timeline::moment(
      "end_of_month", {pluss::timeline::MomentMode::ExpressionOnly,
                        pluss::timeline::MomentClock::UTC});
  show_check("moment_calendar_words",
             utc_today.epoch_nanos().item<std::int64_t>() -
                         yesterday.epoch_nanos().item<std::int64_t>() ==
                     86'400LL * 1'000'000'000LL &&
                 tomorrow.epoch_nanos().item<std::int64_t>() -
                         utc_today.epoch_nanos().item<std::int64_t>() ==
                     86'400LL * 1'000'000'000LL &&
                 month_start.day().item<std::int32_t>() == 1 &&
                 (month_end + pluss::duration::days(1)).day().item<std::int32_t>() == 1);
  show_rejection("moment_strict_rejects_expression", [] {
    (void)pluss::timeline::moment(
        "today", {pluss::timeline::MomentMode::Strict,
                   pluss::timeline::MomentClock::UTC});
  });
  show_rejection("moment_expression_rejects_datetime", [] {
    (void)pluss::timeline::moment(
        "2024-01-01", {pluss::timeline::MomentMode::ExpressionOnly,
                        pluss::timeline::MomentClock::UTC});
  });
  show_rejection("moment_rejects_overflow", [] {
    (void)pluss::timeline::moment(
        "today+999999999999999999999999999999999999d",
        {pluss::timeline::MomentMode::ExpressionOnly,
         pluss::timeline::MomentClock::UTC});
  });

  const auto before_now = std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();
  const auto captured_now = pluss::timeline::now();
  const auto after_now = std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();
  const auto captured_tick = captured_now.epoch_nanos().item<std::int64_t>();
  show_check("now_is_scalar_utc",
             captured_now.dim() == 0 && captured_now.numel() == 1 &&
                 captured_now.tz() == "UTC");
  show_check("now_is_bounded",
             before_now <= captured_tick && captured_tick <= after_now);
  const auto reused = captured_now;
  show_check("now_reuse_is_identical",
             reused.epoch_nanos().item<std::int64_t>() == captured_tick);

  const auto print_values = pluss::timeline::make({
      "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z",
      "2024-01-03T00:00:00Z", "2024-01-04T00:00:00Z",
      "2024-01-05T00:00:00Z", "2024-01-06T00:00:00Z"});
  show_text("print_default", pluss::timeline::print(print_values));
  auto print_options = pluss::TimelinePrintOptions{};
  print_options.format = "%F";
  print_options.max_cols = 4;
  print_options.edge_items = 1;
  print_options.show_tz = false;
  show_text("print_truncated", pluss::timeline::print(print_values, print_options));
  auto bare_options = pluss::TimelinePrintOptions{};
  bare_options.format = "%FT%T.%f%z";
  bare_options.show_header = false;
  show_text("print_no_header", pluss::timeline::print(
      pluss::timeline::from_epoch(1LL), bare_options));
  std::ostringstream print_stream;
  auto stream_options = pluss::TimelinePrintOptions{};
  stream_options.format = "%F";
  stream_options.show_header = false;
  const auto stream_values = pluss::timeline::date(
      {"2024-01-01", "2024-01-02"});
  auto& returned_stream = pluss::timeline::print(
      print_stream, stream_values, stream_options);
  returned_stream << "\nrecords=" << stream_values.numel();
  show_check("print_returns_same_stream", &returned_stream == &print_stream);
  show_text("print_stream", print_stream.str());

  show_exact("rand_between_equal_bounds", pluss::timeline::rand_between(
      "2024-02-29", "2024-02-29", {2, 2}, 99));
  const auto closed_ticks = pluss::timeline::rand_between(
      {8}, pluss::timeline::from_epoch(100LL),
      pluss::timeline::from_epoch(102LL), 99);
  show("rand_between_closed_ticks", closed_ticks.epoch_nanos());
  const auto seeded_a = pluss::timeline::rand_between(
      "2024-01-01", "2024-01-03", {5}, 7);
  const auto seeded_b = pluss::timeline::rand_between(
      "2024-01-01", "2024-01-03", {5}, 7);
  show_check("rand_between_seed_repeatable",
             seeded_a.epoch_nanos().equal(seeded_b.epoch_nanos()));
  show_exact("rand_between_timezone", pluss::timeline::rand_between(
      {3},
      pluss::timeline::make("2024-07-01T06:00:00Z")
          .tz_convert("America/Edmonton"),
      pluss::timeline::make("2024-07-01T06:00:00.000000002Z")
          .tz_convert("America/Edmonton"),
      5));
  show_rejection("rand_between_reversed", [] {
    (void)pluss::timeline::rand_between(
        "2024-01-03", "2024-01-01", {2}, 7);
  });

  // Pass 4: half-open sampling, ordered selection, live day boundaries, and joining.
  show("random_scalar_tick", pluss::timeline::random(
      {}, pluss::timeline::from_epoch(0LL),
      pluss::timeline::from_epoch(1LL), 17).epoch_nanos());
  const auto random_matrix = pluss::timeline::random(
      {2, 3}, pluss::timeline::from_epoch(10LL),
      pluss::timeline::from_epoch(15LL), 7);
  show("random_matrix_ticks", random_matrix.epoch_nanos());
  show_check("random_half_open",
             random_matrix.ge(pluss::timeline::from_epoch(10LL)).all().item<bool>() &&
                 random_matrix.lt(pluss::timeline::from_epoch(15LL)).all().item<bool>());
  show_check("random_seed_repeatable",
             random_matrix.epoch_nanos().equal(
                 pluss::timeline::random(
                     {2, 3}, pluss::timeline::from_epoch(10LL),
                     pluss::timeline::from_epoch(15LL), 7).epoch_nanos()));
  show_exact("random_timezone", pluss::timeline::random(
      {3},
      pluss::timeline::make("2024-07-01T06:00:00Z")
          .tz_convert("America/Edmonton"),
      pluss::timeline::make("2024-07-01T06:00:00.000000003Z")
          .tz_convert("America/Edmonton"),
      5));
  show("random_empty", pluss::timeline::random(
      {2, 0}, pluss::timeline::from_epoch(0LL),
      pluss::timeline::from_epoch(1LL), 9));
  show_rejection("random_equal_bounds", [] {
    (void)pluss::timeline::random(
        {1}, pluss::timeline::from_epoch(2LL),
        pluss::timeline::from_epoch(2LL), 7);
  });
  show_rejection("random_non_scalar_bound", [] {
    (void)pluss::timeline::random(
        {1}, pluss::timeline::from_epoch({0LL}),
        pluss::timeline::from_epoch(2LL), 7);
  });
  show_rejection("random_nat_bound", [] {
    (void)pluss::timeline::random(
        {1}, pluss::timeline::from_epoch(pluss::Timeline::kNaT),
        pluss::timeline::from_epoch(2LL), 7);
  });

  const auto text_mask = pluss::make_series({true, false, false});
  show_exact("select_text_shorthand", pluss::timeline::select(
      text_mask, "2024-12-25", "2024-01-01"), "%F");
  const auto matrix_mask = pluss::make_series(
      {{true, false}, {false, true}});
  show_exact("select_broadcast", pluss::timeline::select(
      matrix_mask,
      pluss::timeline::date({"2024-01-10", "2024-01-20"}),
      pluss::timeline::date("2024-01-31")), "%F");
  const auto early = pluss::make_series({true, true, false, false});
  const auto late = pluss::make_series({false, true, true, false});
  show_exact("select_first_true", pluss::timeline::select(
      {early, late},
      {pluss::timeline::date("2024-01-10"),
       pluss::timeline::date("2024-01-20")},
      pluss::timeline::date("2024-01-31")), "%F");
  show_exact("select_nat_branch", pluss::timeline::select(
      pluss::make_series({true, false}),
      pluss::timeline::date("2024-02-29"),
      pluss::timeline::from_epoch(pluss::Timeline::kNaT)), "%F");
  show_rejection("select_count_mismatch", [&] {
    (void)pluss::timeline::select(
        {early, late}, {pluss::timeline::date("2024-01-10")},
        pluss::timeline::date("2024-01-31"));
  });
  show_rejection("select_non_bool_condition", [] {
    (void)pluss::timeline::select(
        pluss::make_series({1, 0}), pluss::timeline::date("2024-01-10"),
        pluss::timeline::date("2024-01-31"));
  });
  show_rejection("select_shape_mismatch", [] {
    (void)pluss::timeline::select(
        pluss::make_series({true, false, true}),
        pluss::timeline::date({"2024-01-10", "2024-01-20"}),
        pluss::timeline::date("2024-01-31"));
  });
  show_rejection("select_timezone_mismatch", [] {
    (void)pluss::timeline::select(
        pluss::make_series({true, false}),
        pluss::timeline::date("2024-01-10").tz_convert("America/Edmonton"),
        pluss::timeline::date("2024-01-31"));
  });

  const auto today_utc = pluss::timeline::today();
  show_check("today_utc_scalar_midnight",
             today_utc.dim() == 0 && today_utc.numel() == 1 &&
                 today_utc.tz() == "UTC" &&
                 today_utc.epoch_nanos().item<std::int64_t>() %
                         (86'400LL * 1'000'000'000LL) == 0);
  const auto today_local = pluss::timeline::today(
      pluss::timeline::MomentClock::Local);
  const auto local_seconds = static_cast<std::time_t>(
      today_local.epoch_nanos().item<std::int64_t>() / 1'000'000'000LL);
  std::tm local_tm{};
#if defined(_WIN32)
  localtime_s(&local_tm, &local_seconds);
#else
  localtime_r(&local_seconds, &local_tm);
#endif
  show_check("today_local_is_local_midnight",
             local_tm.tm_hour == 0 && local_tm.tm_min == 0 && local_tm.tm_sec == 0);
  const auto saved_today = today_utc;
  show_check("today_capture_reuse",
             saved_today.epoch_nanos().item<std::int64_t>() ==
                 today_utc.epoch_nanos().item<std::int64_t>());

  show_exact("unite_sequence", pluss::timeline::unite({
      pluss::timeline::date({"2024-01-01", "2024-01-02"}),
      pluss::timeline::date("2024-01-03")}), "%F");
  const auto unite_matrix = pluss::timeline::date({
      {"2024-01-01", "2024-01-02"},
      {"2024-01-03", "2024-01-04"}});
  show_exact("unite_flattens_multiple", pluss::timeline::unite(
      {unite_matrix, unite_matrix}), "%F");
  const auto unite_identity = pluss::timeline::unite({unite_matrix});
  show_check("unite_one_retains_shape",
             unite_identity.sizes() == unite_matrix.sizes() &&
                 unite_identity.epoch_nanos().equal(unite_matrix.epoch_nanos()));
  show("unite_empty_operands", pluss::timeline::unite({
      pluss::timeline::epoch({0, 2}), pluss::timeline::epoch({3, 0})}));
  show_exact("unite_retains_nat", pluss::timeline::unite({
      pluss::timeline::make({"2024-01-01", ""}),
      pluss::timeline::date("2024-01-02")}), "%F");
  show_rejection("unite_empty_list", [] {
    (void)pluss::timeline::unite(
        std::initializer_list<pluss::Timeline>{});
  });
  show_rejection("unite_timezone_mismatch", [] {
    (void)pluss::timeline::unite({
        pluss::timeline::date("2024-01-01"),
        pluss::timeline::date("2024-01-02")
            .tz_convert("America/Edmonton")});
  });
}
