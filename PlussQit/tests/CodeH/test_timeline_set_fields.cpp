#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <array>
#include <cstdint>
#include <cstdlib>
#include <vector>

TEST_CASE("Timeline field setters preserve every unnamed field", "[timeline-set-fields]") {
  const auto base = pluss::timeline::make({
      {"2024-01-15T23:58:59.123456789Z", "2024-06-20T04:05:06.000000004Z"},
      {"2024-09-28T12:30:45.900000000Z", "2024-12-01T00:00:00.000000001Z"},
  });
  const auto changed = base.set_year(2030).set_month(2).set_day(1)
                           .set_hour(6).set_minute(15).set_second(0);

  REQUIRE(changed.sizes().vec() == std::vector<std::int64_t>{2, 2});
  REQUIRE(changed.year().to_vec<std::int32_t>() == std::vector<std::int32_t>(4, 2030));
  REQUIRE(changed.month().to_vec<std::int32_t>() == std::vector<std::int32_t>(4, 2));
  REQUIRE(changed.day().to_vec<std::int32_t>() == std::vector<std::int32_t>(4, 1));
  REQUIRE(changed.hour().to_vec<std::int32_t>() == std::vector<std::int32_t>(4, 6));
  REQUIRE(changed.minute().to_vec<std::int32_t>() == std::vector<std::int32_t>(4, 15));
  REQUIRE(changed.second().to_vec<std::int32_t>() == std::vector<std::int32_t>(4, 0));
  REQUIRE(changed.nanosecond().to_vec<std::int32_t>() ==
          std::vector<std::int32_t>{123456789, 4, 900000000, 1});
  REQUIRE(base.year().to_vec<std::int32_t>() == std::vector<std::int32_t>(4, 2024));
}

TEST_CASE("Timeline field setters validate arguments and reconstructed dates",
          "[timeline-set-fields]") {
  REQUIRE_THROWS(pluss::timeline::make("2024-02-29T12:00:00Z").set_year(2023));
  REQUIRE_THROWS(pluss::timeline::make("2024-05-31T12:00:00Z").set_month(2));
  REQUIRE_THROWS(pluss::timeline::make({"2024-01-01", "2024-02-01"}).set_day(30));

  const auto empty = pluss::timeline::epoch({0});
  REQUIRE_THROWS(empty.set_month(13));
  REQUIRE_THROWS(empty.set_day(0));
  REQUIRE_THROWS(empty.set_hour(24));
  REQUIRE_THROWS(empty.set_minute(-1));
  REQUIRE_THROWS(empty.set_second(60));

  const auto with_nat = pluss::timeline::make({"2024-01-01T00:00:00Z", ""});
  REQUIRE(with_nat.set_hour(8).isnat().to_vec<bool>() == std::vector<bool>{false, true});

  const auto gap = pluss::timeline::make("2024-03-10T06:30:00Z")
                       .tz_convert("America/New_York");
  REQUIRE_THROWS(gap.set_hour(2));
}

TEST_CASE("Timeline field setters follow logical strides and retain timezone",
          "[timeline-set-fields]") {
  bool released = false;
  {
    auto* raw = static_cast<std::int64_t*>(std::malloc(3 * sizeof(std::int64_t)));
    raw[0] = 1'704'067'200'123'456'789LL;
    raw[1] = 99;
    raw[2] = 1'709'337'600'000'000'004LL;
    const std::array<std::int64_t, 1> shape{2};
    const std::array<std::int64_t, 1> strides{2};
    const auto strided = pluss::Timeline::from_buffer(
        raw, shape, strides, [&](void* p) {
          released = true;
          std::free(p);
        }, "America/New_York");

    const auto changed = strided.set_year(2030);
    REQUIRE(changed.year().to_vec<std::int32_t>() == std::vector<std::int32_t>{2030, 2030});
    REQUIRE(changed.tz() == "America/New_York");
  }
  REQUIRE(released);
}
