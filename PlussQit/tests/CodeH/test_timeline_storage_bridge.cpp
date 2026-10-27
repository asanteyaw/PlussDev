#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <span>
#include <vector>

TEST_CASE("Timeline epoch projections floor negative instants", "[timeline-storage]") {
  const auto t = pluss::timeline::from_epoch(
      {{-1LL, 0LL}, {1'999'999'999LL, pluss::Timeline::kNaT}});

  REQUIRE(t.epoch_nanos().host<std::int64_t>() ==
          std::vector<std::int64_t>{-1, 0, 1'999'999'999LL, pluss::Timeline::kNaT});
  REQUIRE(t.epoch_micros().host<std::int64_t>() ==
          std::vector<std::int64_t>{-1, 0, 1'999'999LL, pluss::Timeline::kNaT});
  REQUIRE(t.epoch_millis().host<std::int64_t>() ==
          std::vector<std::int64_t>{-1, 0, 1'999LL, pluss::Timeline::kNaT});
  REQUIRE(t.epoch_seconds().host<std::int64_t>() ==
          std::vector<std::int64_t>{-1, 0, 1, pluss::Timeline::kNaT});
  REQUIRE(t.epoch_days().host<std::int64_t>() ==
          std::vector<std::int64_t>{-1, 0, 0, pluss::Timeline::kNaT});
  REQUIRE(t.epoch_seconds().sizes().vec() == std::vector<std::int64_t>{2, 2});
}

TEST_CASE("Timeline coarse epoch constructors preserve shape and reject overflow",
          "[timeline-storage]") {
  const auto seconds = pluss::make_series(
      {{-1LL, 0LL}, {1LL, pluss::Timeline::kNaT}});
  const auto rebuilt = pluss::Timeline::from_epoch_seconds(seconds, "America/New_York");
  REQUIRE(rebuilt.ticks() ==
          std::vector<std::int64_t>{-1'000'000'000LL, 0, 1'000'000'000LL,
                                    pluss::Timeline::kNaT});
  REQUIRE(rebuilt.sizes().vec() == std::vector<std::int64_t>{2, 2});
  REQUIRE(rebuilt.tz() == "America/New_York");

  const auto too_large = pluss::make_series({std::int64_t{9'223'372'037}});
  REQUIRE_THROWS(pluss::Timeline::from_epoch_seconds(too_large));

  const auto millis = pluss::make_series({-1LL, 0LL, 1'999LL});
  REQUIRE(pluss::Timeline::from_epoch_millis(millis).ticks() ==
          std::vector<std::int64_t>{-1'000'000LL, 0, 1'999'000'000LL});
  const auto micros = pluss::make_series({-1LL, 0LL, 1'999'999LL});
  REQUIRE(pluss::Timeline::from_epoch_micros(micros).ticks() ==
          std::vector<std::int64_t>{-1'000LL, 0, 1'999'999'000LL});
}

TEST_CASE("Timeline component constructors broadcast scalars and reject invalid fields",
          "[timeline-storage]") {
  const auto year = pluss::make_series(2024LL);
  const auto month = pluss::make_series({2LL, 3LL});
  const auto day = pluss::make_series({29LL, pluss::Timeline::kNaT});
  const auto dates = pluss::Timeline::from_ymd(year, month, day, "UTC");
  REQUIRE(dates.sizes().vec() == std::vector<std::int64_t>{2});
  REQUIRE(dates.year().host<std::int32_t>() ==
          std::vector<std::int32_t>{2024, -1});

  const auto hour = pluss::make_series({6LL, 7LL});
  const auto minute = pluss::make_series(30LL);
  const auto second = pluss::make_series(45LL);
  const auto times = pluss::Timeline::from_ymdhms(
      year, month, pluss::make_series({29LL, 1LL}), hour, minute, second, "UTC");
  REQUIRE(times.hour().host<std::int32_t>() ==
          std::vector<std::int32_t>{6, 7});
  REQUIRE(times.minute().host<std::int32_t>() ==
          std::vector<std::int32_t>{30, 30});

  REQUIRE_THROWS(pluss::Timeline::from_ymd(
      year, pluss::make_series(2LL), pluss::make_series(30LL)));
  REQUIRE_THROWS(pluss::Timeline::from_ymdhms(
      year, pluss::make_series(1LL), pluss::make_series(1LL),
      pluss::make_series(24LL), minute, second));
}

TEST_CASE("Timeline buffer bridge honors strides and owns through its deleter",
          "[timeline-storage]") {
  bool released = false;
  {
    auto* raw = static_cast<std::int64_t*>(std::malloc(3 * sizeof(std::int64_t)));
    raw[0] = 0;
    raw[1] = 111;
    raw[2] = 2'000'000'000LL;
    const std::array<std::int64_t, 1> shape{2};
    const std::array<std::int64_t, 1> strides{2};
    const auto t = pluss::Timeline::from_buffer(
        raw, shape, strides,
        [&](void* p) { released = true; std::free(p); }, "UTC");
    REQUIRE(t.ticks() == std::vector<std::int64_t>{0, 2'000'000'000LL});
    REQUIRE(t.min().ticks() == std::vector<std::int64_t>{0});
    REQUIRE(t.max().ticks() ==
            std::vector<std::int64_t>{2'000'000'000LL});
    REQUIRE(t.unique().ticks() ==
            std::vector<std::int64_t>{0, 2'000'000'000LL});
    const auto query = pluss::timeline::from_epoch({2'000'000'000LL});
    REQUIRE(t.searchsorted_left(query).host<std::int64_t>() ==
            std::vector<std::int64_t>{1});

    std::vector<std::int64_t> copied;
    t.copy_ticks(std::back_inserter(copied));
    REQUIRE(copied == std::vector<std::int64_t>{0, 2'000'000'000LL});
    REQUIRE_FALSE(released);
  }
  REQUIRE(released);
}

TEST_CASE("Timeline promotion to TimelineIndexAccessor enforces the requested invariant",
          "[timeline-storage]") {
  const auto sorted = pluss::timeline::make({
      "2024-01-01T00:00:00Z", "2024-01-01T01:00:00Z",
  });
  REQUIRE(sorted.index().size() == 2);

  const auto unsorted = pluss::timeline::make({
      "2024-01-01T01:00:00Z", "2024-01-01T00:00:00Z",
  });
  REQUIRE_THROWS(unsorted.index());
  REQUIRE(unsorted.index(false).size() == 2);
  REQUIRE_THROWS(pluss::timeline::make({""}).index());
  REQUIRE_THROWS(pluss::timeline::make("2024-01-01T00:00:00Z").index());
}

TEST_CASE("Timeline operators preserve temporal types and elementwise comparison semantics",
          "[timeline-operators]") {
  const auto base = pluss::timeline::make({
      "2024-01-01T00:00:00Z", "2024-01-01T02:00:00Z"});
  const auto delta = pluss::duration::hours({1, -2});

  REQUIRE((base + delta).ticks() ==
          (delta + base).ticks());
  REQUIRE(((base + delta) - base).host() == delta.host());

  auto changed = base;
  changed += pluss::duration::hours(1);
  changed -= pluss::duration::minutes(30);
  REQUIRE(changed.ticks() ==
          (base + pluss::duration::minutes(30)).ticks());

  const auto boundary = pluss::timeline::make("2024-01-01T01:00:00Z");
  REQUIRE((base < boundary).host<bool>() == std::vector<bool>{true, false});
  REQUIRE((base >= boundary).host<bool>() == std::vector<bool>{false, true});

  const auto with_nat = pluss::timeline::make({"", "2024-01-01T00:00:00Z"});
  REQUIRE((with_nat == with_nat).host<bool>() == std::vector<bool>{false, true});
  REQUIRE((with_nat != with_nat).host<bool>() == std::vector<bool>{true, false});
  REQUIRE((with_nat < with_nat).host<bool>() == std::vector<bool>{false, false});
  REQUIRE((with_nat <= with_nat).host<bool>() == std::vector<bool>{false, true});
  REQUIRE((with_nat > with_nat).host<bool>() == std::vector<bool>{false, false});
  REQUIRE((with_nat >= with_nat).host<bool>() == std::vector<bool>{false, true});

  const std::vector<std::string> row_time_values{
      "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z"};
  const auto row_times = pluss::timeline::make(row_time_values, {2, 1});
  const auto column_offsets = pluss::duration::minutes({{0, 30, 60}});
  REQUIRE((row_times + column_offsets).sizes() == std::vector<std::int64_t>{2, 3});

  const auto precise = pluss::timeline::from_epoch(
      {100LL, pluss::Timeline::kNaT});
  REQUIRE((precise + pluss::duration::nanos({7, 9})).ticks() ==
          std::vector<std::int64_t>{107, pluss::Timeline::kNaT});

  const std::vector<std::int64_t> no_ticks;
  const std::vector<std::int64_t> empty_shape{0, 2};
  const auto empty = pluss::timeline::from_epoch(no_ticks, empty_shape);
  REQUIRE((empty + pluss::duration::hours(1)).sizes() ==
          std::vector<std::int64_t>{0, 2});
  REQUIRE((empty - pluss::duration::hours(1)).sizes() ==
          std::vector<std::int64_t>{0, 2});
  REQUIRE((empty - pluss::timeline::make("2024-01-01T00:00:00Z")).sizes() ==
          std::vector<std::int64_t>{0, 2});
  REQUIRE((empty == pluss::timeline::make("2024-01-01T00:00:00Z")).sizes() ==
          std::vector<std::int64_t>{0, 2});

  const auto matrix = pluss::timeline::make({
      {"2024-01-01T00:00:00Z", "2024-01-01T01:00:00Z"},
      {"2024-01-01T02:00:00Z", "2024-01-01T03:00:00Z"},
  });
  REQUIRE_THROWS(matrix == pluss::timeline::make({
      "2024-01-01T00:00:00Z", "2024-01-01T01:00:00Z",
      "2024-01-01T02:00:00Z"}));

  const auto latest = pluss::timeline::from_epoch(
      {std::numeric_limits<std::int64_t>::max()});
  const auto earliest = pluss::timeline::from_epoch(
      {std::numeric_limits<std::int64_t>::min() + 1});
  REQUIRE_THROWS(latest + pluss::duration::nanos(1));
  REQUIRE_THROWS(earliest - pluss::duration::nanos(1));
  REQUIRE_THROWS(latest - earliest);
}
