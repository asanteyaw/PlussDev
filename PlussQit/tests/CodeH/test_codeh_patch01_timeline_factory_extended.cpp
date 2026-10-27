#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <ctime>
#include <string>
#include <string_view>
#include <vector>

#include "pluss/codeh/duration_factory.h"
#include "pluss/codeh/timeline_factory.h"

TEST_CASE("CodeH extended Timeline factories cover strict date/time, repetition, random ranges, and arange",
          "[codeh][patch01]") {
  const auto dates = pluss::timeline::date({"2024-06-10", "2024-06-11"});
  REQUIRE(dates.dim() == 1);
  REQUIRE(dates.numel() == 2);
  REQUIRE(dates.strftime("%H:%M:%S").vector() ==
          std::vector<std::string_view>({"00:00:00", "00:00:00"}));

  const auto times = pluss::timeline::time({"09:30:00", "17:45:30"});
  REQUIRE(times.dim() == 1);
  REQUIRE(times.numel() == 2);
  REQUIRE(times.strftime("%F").vector() ==
          std::vector<std::string_view>({"1970-01-01", "1970-01-01"}));
  REQUIRE(times.strftime("%T").vector() ==
          std::vector<std::string_view>({"09:30:00", "17:45:30"}));

  const auto repeated = pluss::timeline::full(
      {2, 2}, pluss::timeline::date("2025-02-05"));
  const auto repeated_sizes = repeated.sizes();
  REQUIRE(repeated.dim() == 2);
  REQUIRE(repeated_sizes.size() == 2);
  REQUIRE(repeated_sizes[0] == 2);
  REQUIRE(repeated_sizes[1] == 2);
  REQUIRE(repeated.day().host<std::int32_t>() ==
          std::vector<std::int32_t>({5, 5, 5, 5}));

  const auto random_between =
      pluss::timeline::rand_between("2024-06-01", "2024-06-03", {12}, 11);
  REQUIRE(random_between.dim() == 1);
  REQUIRE(random_between.numel() == 12);
  const auto random_day_note = random_between.strftime("%F");
  const auto random_days = random_day_note.vector();
  for (const auto& day : random_days) {
    REQUIRE((day == "2024-06-01" || day == "2024-06-02" || day == "2024-06-03"));
  }

  const auto date_range = pluss::timeline::arange("2024-06-01", "2024-06-05", 2, true);
  REQUIRE(date_range.strftime("%F").vector() ==
          std::vector<std::string_view>({"2024-06-01", "2024-06-03", "2024-06-05"}));

  const auto time_range = pluss::timeline::arange("09:00:00", "09:00:03", 1, true);
  REQUIRE(time_range.strftime("%T").vector() ==
          std::vector<std::string_view>({"09:00:00", "09:00:01", "09:00:02", "09:00:03"}));

  const auto datetime_range = pluss::timeline::arange(
      "2024-06-01T00:00:00Z",
      "2024-06-01T06:00:00Z",
      pluss::duration::hours(2),
      true);
  REQUIRE(datetime_range.strftime("%FT%TZ").vector() ==
          std::vector<std::string_view>({
              "2024-06-01T00:00:00Z",
              "2024-06-01T02:00:00Z",
              "2024-06-01T04:00:00Z",
              "2024-06-01T06:00:00Z",
          }));

  REQUIRE_THROWS(pluss::timeline::date("2024-06-10T09:30:00Z"));
  REQUIRE_THROWS(pluss::timeline::time("2024-06-10"));
  REQUIRE_THROWS(pluss::timeline::time("23:59:60"));
  REQUIRE_THROWS(pluss::timeline::make("2024-01-01T00:00:00+24:00"));
  REQUIRE_THROWS(pluss::timeline::make("0001-01-01T00:00:00Z"));
  REQUIRE_THROWS(pluss::timeline::from_epoch(0LL, "Mars/Olympus_Mons"));
  REQUIRE_THROWS(pluss::timeline::arange("2024-06-01T00:00:00Z", "2024-06-01T06:00:00Z", 1, true));
  REQUIRE_THROWS(pluss::timeline::arange(
      pluss::timeline::date("2024-06-01"),
      pluss::timeline::date("2024-06-03"),
      pluss::duration::nanos(pluss::Duration::kNaT)));
  REQUIRE_THROWS(pluss::timeline::epoch({1, 1, 1, 1}));
  REQUIRE_THROWS(pluss::timeline::moment(
      "today+999999999999999999999999999999999999d",
      {pluss::timeline::MomentMode::ExpressionOnly,
       pluss::timeline::MomentClock::UTC}));
  REQUIRE_THROWS(pluss::timeline::moment(
      "som", {pluss::timeline::MomentMode::ExpressionOnly,
               pluss::timeline::MomentClock::UTC}));
}

TEST_CASE("timeline moment resolves local calendar words at local midnight",
          "[codeh][patch01][timeline-moment]") {
  const auto local_today = pluss::timeline::moment(
      "today", {pluss::timeline::MomentMode::ExpressionOnly,
                 pluss::timeline::MomentClock::Local});
  const auto tick = local_today.epoch_nanos().item<std::int64_t>();
  const auto seconds = static_cast<std::time_t>(tick / 1'000'000'000LL);

  std::tm resolved{};
#if defined(_WIN32)
  localtime_s(&resolved, &seconds);
#else
  localtime_r(&seconds, &resolved);
#endif
  REQUIRE(resolved.tm_hour == 0);
  REQUIRE(resolved.tm_min == 0);
  REQUIRE(resolved.tm_sec == 0);

  const auto before = std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();
  const auto captured = pluss::timeline::now();
  const auto after = std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();
  const auto captured_tick = captured.epoch_nanos().item<std::int64_t>();
  REQUIRE(captured.dim() == 0);
  REQUIRE(captured.tz() == "UTC");
  REQUIRE(before <= captured_tick);
  REQUIRE(captured_tick <= after);
}

TEST_CASE("timeline final factories enforce sampling selection and joining contracts",
          "[codeh][patch01][timeline-factory-final]") {
  const auto sampled = pluss::timeline::random(
      {2, 3}, pluss::timeline::from_epoch(10LL),
      pluss::timeline::from_epoch(15LL), 7);
  REQUIRE(sampled.ticks() ==
          std::vector<std::int64_t>({12, 14, 11, 11, 14, 11}));
  REQUIRE(sampled.sizes() == std::vector<std::int64_t>({2, 3}));
  for (const auto tick : sampled.ticks()) {
    REQUIRE(tick >= 10);
    REQUIRE(tick < 15);
  }
  REQUIRE(pluss::timeline::random(
              {}, pluss::timeline::from_epoch(0LL),
              pluss::timeline::from_epoch(1LL), 17)
              .epoch_nanos().item<std::int64_t>() == 0);
  REQUIRE(pluss::timeline::random(
              {2, 0}, pluss::timeline::from_epoch(0LL),
              pluss::timeline::from_epoch(1LL), 9)
              .sizes() == std::vector<std::int64_t>({2, 0}));
  REQUIRE_THROWS(pluss::timeline::random(
      {1}, pluss::timeline::from_epoch(2LL),
      pluss::timeline::from_epoch(2LL), 7));

  const auto early = pluss::make_series({true, true, false, false});
  const auto late = pluss::make_series({false, true, true, false});
  const auto selected = pluss::timeline::select(
      {early, late},
      {pluss::timeline::date("2024-01-10"),
       pluss::timeline::date("2024-01-20")},
      pluss::timeline::date("2024-01-31"));
  REQUIRE(selected.strftime("%F").vector() == std::vector<std::string_view>({
      "2024-01-10", "2024-01-10", "2024-01-20", "2024-01-31"}));

  const auto broadcast_selected = pluss::timeline::select(
      pluss::make_series({{true, false}, {false, true}}),
      pluss::timeline::date({"2024-01-10", "2024-01-20"}),
      pluss::timeline::date("2024-01-31"));
  REQUIRE(broadcast_selected.sizes() == std::vector<std::int64_t>({2, 2}));
  REQUIRE(broadcast_selected.strftime("%F").vector() ==
          std::vector<std::string_view>({
              "2024-01-10", "2024-01-31", "2024-01-31", "2024-01-20"}));
  REQUIRE_THROWS(pluss::timeline::select(
      pluss::make_series({true, false}),
      pluss::timeline::date("2024-01-10").tz_convert("America/Edmonton"),
      pluss::timeline::date("2024-01-31")));

  const auto matrix = pluss::timeline::date({
      {"2024-01-01", "2024-01-02"},
      {"2024-01-03", "2024-01-04"}});
  const auto ticks = matrix.ticks();
  const auto joined = pluss::timeline::unite({matrix, matrix});
  REQUIRE(joined.sizes() == std::vector<std::int64_t>({8}));
  REQUIRE(joined.ticks() ==
          std::vector<std::int64_t>({
              ticks[0], ticks[1], ticks[2], ticks[3],
              ticks[0], ticks[1], ticks[2], ticks[3]}));
  REQUIRE(pluss::timeline::unite({matrix}).sizes() ==
          std::vector<std::int64_t>({2, 2}));
  REQUIRE_THROWS(pluss::timeline::unite(
      std::initializer_list<pluss::Timeline>{}));
  REQUIRE_THROWS(pluss::timeline::unite({
      pluss::timeline::date("2024-01-01"),
      pluss::timeline::date("2024-01-02").tz_convert("America/Edmonton")}));
}

TEST_CASE("timeline date_range separates calendar days from elapsed hours",
          "[codeh][timeline-date-range]") {
  const auto day_start = pluss::timeline::make("2024-03-09T14:30:00Z")
                             .tz_convert("America/New_York");
  const auto day_end = pluss::timeline::make("2024-03-11T13:30:00Z")
                           .tz_convert("America/New_York");
  const auto days = pluss::timeline::date_range(day_start, day_end, "D");
  REQUIRE(days.strftime("%FT%T%z").vector() == std::vector<std::string_view>({
      "2024-03-09T09:30:00-05:00",
      "2024-03-10T09:30:00-04:00",
      "2024-03-11T09:30:00-04:00",
  }));
  const auto ticks = days.ticks();
  REQUIRE(ticks[1] - ticks[0] == 82'800'000'000'000LL);
  REQUIRE(ticks[2] - ticks[1] == 86'400'000'000'000LL);

  const auto hour_start = pluss::timeline::make("2024-03-10T05:00:00Z")
                              .tz_convert("America/New_York");
  const auto hour_end = pluss::timeline::make("2024-03-10T08:00:00Z")
                            .tz_convert("America/New_York");
  REQUIRE(pluss::timeline::date_range(hour_start, hour_end, "H")
              .strftime("%H:%M%z").vector() == std::vector<std::string_view>({
                  "00:00-05:00", "01:00-05:00",
                  "03:00-04:00", "04:00-04:00",
              }));

  REQUIRE(pluss::timeline::date_range(day_end, day_start, "D", 1).numel() == 0);
  REQUIRE_THROWS(pluss::timeline::date_range(day_start, day_end, "D", 0));
  REQUIRE_THROWS(pluss::timeline::date_range(
      pluss::timeline::date({"2024-01-01"}), day_end, "D"));
}

TEST_CASE("timeline format projects instants without changing shape or precision",
          "[codeh][timeline-format]") {
  const auto values = pluss::timeline::from_epoch(
      {1'709'249'445'120'000'000LL, pluss::Timeline::kNaT,
       1'719'813'600'000'000'001LL, 1'704'067'200'000'000'000LL},
      {2, 2});

  auto options = pluss::timeline::FormatOptions{};
  options.pattern = "%F %R | day=%j weekday=%u quarter=%q | %f | %Z %z";
  options.timezone = "America/Edmonton";
  options.nat = "not-recorded";
  options.fractional_digits = 6;
  options.trim_fractional_zeros = true;

  const auto rendered = pluss::timeline::format(values, options);
  REQUIRE(rendered.sizes() == values.sizes().vec());
  REQUIRE(rendered.vector() == std::vector<std::string_view>({
      "2024-02-29 16:30 | day=060 weekday=4 quarter=1 | 12 | America/Edmonton -07:00",
      "not-recorded",
      "2024-07-01 00:00 | day=183 weekday=1 quarter=3 | 0 | America/Edmonton -06:00",
      "2023-12-31 17:00 | day=365 weekday=7 quarter=4 | 0 | America/Edmonton -07:00",
  }));

  REQUIRE(pluss::timeline::format(values, "%F").vector() ==
          std::vector<std::string_view>({"2024-02-29", "NaT", "2024-07-01", "2024-01-01"}));
  REQUIRE_THROWS(pluss::timeline::format(values, "%Q"));
  options.fractional_digits = 0;
  REQUIRE_THROWS(pluss::timeline::format(values, options));

  const auto local = pluss::timeline::make("2024-01-01T00:00:00Z")
                         .tz_convert("America/Edmonton");
  REQUIRE(pluss::timeline::full({2}, local).tz() == "America/Edmonton");
  REQUIRE(pluss::timeline::random(
              {2}, local, pluss::timeline::make("2024-01-02T00:00:00Z"), 3).tz() ==
          "America/Edmonton");
  REQUIRE_THROWS(pluss::timeline::full({-1}, local));
  REQUIRE_THROWS(pluss::timeline::random(
      {1}, pluss::timeline::from_epoch(pluss::Timeline::kNaT),
      pluss::timeline::from_epoch(0LL), 3));
}
