#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <cstdint>
#include <type_traits>
#include <vector>

static_assert(!std::is_default_constructible_v<pluss::Duration>);

TEST_CASE("Duration metadata distinguishes rank from cardinality",
          "[codeh][duration][metadata]") {
  const auto scalar = pluss::duration::seconds(90);
  const auto singleton = pluss::duration::seconds({90});
  const auto matrix = pluss::duration::hours({1, 2, 3, 4}, {2, 2});
  const auto cube = pluss::duration::nanos({1, 2, 3, 4}, {2, 1, 2});
  const auto empty = pluss::duration::zeros({2, 0, 3});

  REQUIRE(scalar.defined());
  REQUIRE(scalar.dim() == 0);
  REQUIRE(scalar.numel() == 1);
  REQUIRE(scalar.sizes().empty());

  REQUIRE(singleton.dim() == 1);
  REQUIRE(singleton.numel() == 1);
  REQUIRE(std::vector<std::int64_t>(singleton.sizes().begin(), singleton.sizes().end()) ==
          std::vector<std::int64_t>{1});

  REQUIRE(matrix.dim() == 2);
  REQUIRE(matrix.numel() == 4);
  REQUIRE(std::vector<std::int64_t>(matrix.sizes().begin(), matrix.sizes().end()) ==
          std::vector<std::int64_t>{2, 2});

  REQUIRE(cube.dim() == 3);
  REQUIRE(cube.numel() == 4);
  REQUIRE(std::vector<std::int64_t>(cube.sizes().begin(), cube.sizes().end()) ==
          std::vector<std::int64_t>{2, 1, 2});

  REQUIRE(empty.defined());
  REQUIRE(empty.dim() == 3);
  REQUIRE(empty.numel() == 0);
  REQUIRE(std::vector<std::int64_t>(empty.sizes().begin(), empty.sizes().end()) ==
          std::vector<std::int64_t>{2, 0, 3});
}

TEST_CASE("Duration host preserves exact nanoseconds and NaT",
          "[codeh][duration][metadata]") {
  const std::vector<std::int64_t> ticks{
      9'007'199'254'740'993LL, -1LL, pluss::Duration::kNaT};
  const auto value = pluss::duration::nanos(ticks);

  REQUIRE(value.host() == ticks);
}

TEST_CASE("flat Duration shortcut forwards to the canonical factory",
          "[codeh][duration][factory]") {
  const auto flat = pluss::make_duration(
      {1, 2, 3, 4}, {2, 2}, pluss::duration::Unit::Seconds);
  const auto canonical = pluss::duration::make(
      {1, 2, 3, 4}, {2, 2}, pluss::duration::Unit::Seconds);

  REQUIRE(flat.host() == canonical.host());
  REQUIRE(std::vector<std::int64_t>(flat.sizes().begin(), flat.sizes().end()) ==
          std::vector<std::int64_t>{2, 2});
}
