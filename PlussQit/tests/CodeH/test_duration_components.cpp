#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <cstdint>
#include <vector>

namespace {

constexpr std::int64_t kDay = 86'400'000'000'000LL;
constexpr std::int64_t kHour = 3'600'000'000'000LL;
constexpr std::int64_t kMinute = 60'000'000'000LL;
constexpr std::int64_t kSecond = 1'000'000'000LL;
constexpr std::int64_t kMillisecond = 1'000'000LL;
constexpr std::int64_t kMicrosecond = 1'000LL;

constexpr std::int64_t compose(std::int64_t days,
                               std::int64_t hours,
                               std::int64_t minutes,
                               std::int64_t seconds,
                               std::int64_t milliseconds,
                               std::int64_t microseconds,
                               std::int64_t nanoseconds) {
  return days * kDay + hours * kHour + minutes * kMinute + seconds * kSecond +
         milliseconds * kMillisecond + microseconds * kMicrosecond + nanoseconds;
}

} // namespace

TEST_CASE("Duration components use normalized floor decomposition",
          "[codeh][duration][components]") {
  const auto value = pluss::duration::nanos({
      compose(1, 2, 3, 4, 5, 6, 7),
      -1,
      -kHour,
      -kDay,
      0,
  });

  REQUIRE(value.days().host<std::int64_t>() ==
          std::vector<std::int64_t>{1, -1, -1, -1, 0});
  REQUIRE(value.hours().host<std::int64_t>() ==
          std::vector<std::int64_t>{2, 23, 23, 0, 0});
  REQUIRE(value.minutes().host<std::int64_t>() ==
          std::vector<std::int64_t>{3, 59, 0, 0, 0});
  REQUIRE(value.seconds().host<std::int64_t>() ==
          std::vector<std::int64_t>{4, 59, 0, 0, 0});
  REQUIRE(value.milliseconds().host<std::int64_t>() ==
          std::vector<std::int64_t>{5, 999, 0, 0, 0});
  REQUIRE(value.microseconds().host<std::int64_t>() ==
          std::vector<std::int64_t>{6, 999, 0, 0, 0});
  REQUIRE(value.nanoseconds().host<std::int64_t>() ==
          std::vector<std::int64_t>{7, 999, 0, 0, 0});
}

TEST_CASE("Duration components preserve shape and NaT lane identity",
          "[codeh][duration][components]") {
  const auto value = pluss::duration::nanos(
      {compose(1, 2, 3, 4, 5, 6, 7), pluss::Duration::kNaT,
       -1, 0},
      {2, 1, 2});

  const auto check_shape = [](const pluss::Series& component) {
    REQUIRE(component.scalar_type() == pluss::ScalarType::Int64);
    REQUIRE(component.dim() == 3);
    REQUIRE(component.sizes().vec() == std::vector<std::int64_t>{2, 1, 2});
  };

  check_shape(value.days());
  check_shape(value.hours());
  check_shape(value.minutes());
  check_shape(value.seconds());
  check_shape(value.milliseconds());
  check_shape(value.microseconds());
  check_shape(value.nanoseconds());

  REQUIRE(value.days().host<std::int64_t>()[1] == pluss::Duration::kNaT);
  REQUIRE(value.nanoseconds().host<std::int64_t>()[1] == pluss::Duration::kNaT);
}

TEST_CASE("Duration components preserve zero-element shape",
          "[codeh][duration][components]") {
  const auto empty = pluss::duration::zeros({2, 0, 3});
  const auto component = empty.seconds();

  REQUIRE(component.scalar_type() == pluss::ScalarType::Int64);
  REQUIRE(component.sizes().vec() == std::vector<std::int64_t>{2, 0, 3});
  REQUIRE(component.numel() == 0);
}
