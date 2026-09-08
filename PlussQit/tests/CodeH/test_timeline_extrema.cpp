#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <cstdint>
#include <vector>

TEST_CASE("Timeline min and max reduce globally while ignoring NaT",
          "[timeline-extrema]") {
  const auto observations = pluss::timeline::make({
      {"2025-06-15T08:30:00Z", ""},
      {"2024-01-02T18:45:00Z", "2026-09-30T03:15:00Z"},
  });

  REQUIRE(observations.min().is_scalar());
  REQUIRE(observations.max().is_scalar());
  REQUIRE(observations.min().isoformat() == "2024-01-02T18:45:00.000000000+00:00");
  REQUIRE(observations.max().isoformat() == "2026-09-30T03:15:00.000000000+00:00");
  REQUIRE(observations.min(false).isnat_scalar());
  REQUIRE(observations.max(false).isnat_scalar());
  REQUIRE(observations.sizes().vec() == std::vector<std::int64_t>{2, 2});
}

TEST_CASE("Timeline extrema handle scalar, duplicate, all-NaT, and empty inputs",
          "[timeline-extrema]") {
  const auto duplicated = pluss::timeline::make({
      "2025-06-15T08:30:00Z", "2024-01-02T18:45:00Z",
      "2024-01-02T18:45:00Z", "2026-09-30T03:15:00Z",
      "2026-09-30T03:15:00Z",
  });
  REQUIRE(duplicated.min().isoformat() == "2024-01-02T18:45:00.000000000+00:00");
  REQUIRE(duplicated.max().isoformat() == "2026-09-30T03:15:00.000000000+00:00");

  const auto scalar = pluss::timeline::make("2025-02-03T04:05:06.123456789Z");
  REQUIRE(scalar.min().isoformat() == scalar.isoformat());
  REQUIRE(scalar.max().isoformat() == scalar.isoformat());
  REQUIRE(scalar.min(false).isoformat() == scalar.isoformat());
  REQUIRE(scalar.max(false).isoformat() == scalar.isoformat());

  const auto missing = pluss::timeline::make({"", ""});
  REQUIRE(missing.min().isnat_scalar());
  REQUIRE(missing.max().isnat_scalar());

  const auto empty = pluss::timeline::from_epoch(std::vector<std::int64_t>{});
  REQUIRE(empty.min().isnat_scalar());
  REQUIRE(empty.max().isnat_scalar());
}

TEST_CASE("Timeline extrema retain timezone without changing the selected instant",
          "[timeline-extrema]") {
  const auto utc = pluss::timeline::make({
      "2025-06-15T08:30:00Z", "2024-01-02T18:45:00Z",
      "2026-09-30T03:15:00Z",
  });
  const auto local = utc.tz_convert("America/New_York");

  REQUIRE(local.min().tz() == "America/New_York");
  REQUIRE(local.max().tz() == "America/New_York");
  REQUIRE(local.min().ticks() == utc.min().ticks());
  REQUIRE(local.max().ticks() == utc.max().ticks());
}
