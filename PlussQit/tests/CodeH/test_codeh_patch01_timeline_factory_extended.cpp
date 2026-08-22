#include <catch2/catch_test_macros.hpp>

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

  const auto repeated = pluss::timeline::rep("2025-02-05", {2, 2});
  const auto repeated_sizes = repeated.sizes();
  REQUIRE(repeated.dim() == 2);
  REQUIRE(repeated_sizes.size() == 2);
  REQUIRE(repeated_sizes[0] == 2);
  REQUIRE(repeated_sizes[1] == 2);
  REQUIRE(repeated.day().to_vec<std::int32_t>() ==
          std::vector<std::int32_t>({5, 5, 5, 5}));

  const auto random_between =
      pluss::timeline::rand_between("2024-06-01", "2024-06-03", {12}, 11);
  REQUIRE(random_between.dim() == 1);
  REQUIRE(random_between.numel() == 12);
  const auto random_days = random_between.strftime("%F").vector();
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
  REQUIRE_THROWS(pluss::timeline::arange("2024-06-01T00:00:00Z", "2024-06-01T06:00:00Z", 1, true));
}
