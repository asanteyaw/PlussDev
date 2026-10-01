#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <cstdlib>
#include <vector>

TEST_CASE("Timeline index centralizes rank and ordering validation",
          "[timeline][index][timeindex]") {
  const auto ordered = pluss::timeline::make({
      "2025-01-01T00:00:00Z",
      "2025-01-02T00:00:00Z",
      "2025-01-03T00:00:00Z",
  });
  const auto index = ordered.index();

  REQUIRE(index.timeline().ticks() == ordered.ticks());
  REQUIRE(index.is_monotonic_increasing());

  const auto key = pluss::timeline::make("2025-01-02T12:00:00Z");
  REQUIRE(index.searchsorted(key) == 2);
  REQUIRE(index.asof(key) == 1);

  const auto unordered = pluss::timeline::make({
      "2025-01-02T00:00:00Z",
      "2025-01-01T00:00:00Z",
  });
  REQUIRE_THROWS(unordered.index());
  auto unchecked = unordered.index(false);
  REQUIRE_FALSE(unchecked.is_monotonic_increasing());
  REQUIRE_THROWS(unchecked.searchsorted(key));
  REQUIRE_THROWS(unchecked.asof(key));
  REQUIRE_THROWS(unchecked.slice(key, key));

  const auto matrix = pluss::timeline::make({
      {"2025-01-01T00:00:00Z", "2025-01-02T00:00:00Z"},
  });
  REQUIRE_THROWS(matrix.index());
}

TEST_CASE("Timeline index preserves timezone and borrowed storage",
          "[timeline][index][timeindex][storage]") {
  auto* raw = static_cast<std::int64_t*>(
      std::malloc(3 * sizeof(std::int64_t)));
  REQUIRE(raw != nullptr);
  raw[0] = 0;
  raw[1] = 1'000'000'000LL;
  raw[2] = 2'000'000'000LL;

  bool freed = false;
  const std::vector<std::int64_t> sizes{3};
  const std::vector<std::int64_t> strides{1};
  {
    const auto timeline = pluss::Timeline::from_buffer(
        raw,
        sizes,
        strides,
        [&](void* pointer) {
          freed = true;
          std::free(pointer);
        },
        "America/Edmonton");
    const auto index = timeline.index();

    REQUIRE(index.timeline().tz() == "America/Edmonton");
    raw[1] = 1'500'000'000LL;
    REQUIRE(index.host() ==
            std::vector<std::int64_t>{0, 1'500'000'000LL, 2'000'000'000LL});
    REQUIRE_FALSE(freed);
  }
  REQUIRE(freed);
}
