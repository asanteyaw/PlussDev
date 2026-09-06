#include <pluss/pluss.h>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <vector>

namespace {

std::vector<std::int64_t> shape_of(const pluss::Timeline& value) {
  return {value.sizes().begin(), value.sizes().end()};
}

} // namespace

TEST_CASE("Timeline metadata distinguishes scalar vector and matrix payloads", "[timeline-foundations]") {
  const auto scalar = pluss::timeline::make("2024-02-29T12:30:00Z");
  const auto vector = pluss::timeline::make({"2024-02-29T12:30:00Z", "2024-03-01T00:00:00Z"});
  const auto matrix = pluss::timeline::make({
      {"2024-02-29T12:30:00Z", "2024-03-01T00:00:00Z"},
      {"2024-03-02T06:15:00Z", ""},
  });

  REQUIRE(scalar.defined());
  REQUIRE(scalar.dim() == 0);
  REQUIRE(scalar.numel() == 1);
  REQUIRE(scalar.is_scalar());
  REQUIRE(shape_of(scalar).empty());

  REQUIRE(vector.dim() == 1);
  REQUIRE(vector.numel() == 2);
  REQUIRE_FALSE(vector.is_scalar());
  REQUIRE(shape_of(vector) == std::vector<std::int64_t>{2});

  REQUIRE(matrix.dim() == 2);
  REQUIRE(matrix.numel() == 4);
  REQUIRE(shape_of(matrix) == std::vector<std::int64_t>{2, 2});

  const auto empty = pluss::timeline::epoch({2, 0, 4});
  REQUIRE(empty.defined());
  REQUIRE(empty.dim() == 3);
  REQUIRE(empty.numel() == 0);
  REQUIRE(shape_of(empty) == std::vector<std::int64_t>{2, 0, 4});
}

TEST_CASE("Timeline host materialization distinguishes cardinality and rank", "[timeline-foundations]") {
  const auto scalar = pluss::timeline::make("2024-02-29T12:30:00Z");
  const auto vector = pluss::timeline::make({"2024-02-29T12:30:00Z", "2024-03-01T00:00:00Z"});
  const auto one_vector = pluss::timeline::make({"2024-02-29T12:30:00Z"});
  const auto matrix = pluss::timeline::make({
      {"2024-02-29T12:30:00Z", "2024-03-01T00:00:00Z"},
      {"2024-03-02T06:15:00Z", ""},
  });

  REQUIRE(scalar.host().is_scalar());
  REQUIRE(vector.host().is_vector());
  REQUIRE(one_vector.dim() == 1);
  REQUIRE_FALSE(one_vector.is_scalar());
  REQUIRE(one_vector.host().is_scalar());
  REQUIRE(matrix.host().is_array());
  REQUIRE(matrix.host().array().sizes == std::vector<std::int64_t>{2, 2});
  REQUIRE(matrix.host().array().data == matrix.to_epoch_nanos_vector());
  REQUIRE(matrix.host().array().contiguous);
  REQUIRE(matrix.to_epoch_nanos_flat() == matrix.to_epoch_nanos_vector());
  REQUIRE(matrix.epoch_nanos().to_vec<std::int64_t>() == matrix.to_epoch_nanos_vector());

  const auto shaped = pluss::timeline::epoch({2, 3});
  const auto ticks = shaped.to_epoch_nanos_vector();
  const std::vector<std::int64_t> sizes(shaped.sizes().begin(), shaped.sizes().end());
  const auto rebuilt = pluss::timeline::from_epoch(ticks, sizes);
  REQUIRE(shape_of(rebuilt) == sizes);
  REQUIRE(rebuilt.to_epoch_nanos_vector() == ticks);
}

TEST_CASE("timezone conversion changes presentation metadata not the instant", "[timeline-foundations]") {
  const auto utc = pluss::timeline::make("2024-02-29T12:30:00Z");
  const auto local = utc.tz_convert("America/New_York");
  REQUIRE(utc.tz() == "UTC");
  REQUIRE(local.tz() == "America/New_York");
  REQUIRE(utc.to_epoch_nanos_vector() == local.to_epoch_nanos_vector());
  REQUIRE(utc.isoformat() == "2024-02-29T12:30:00.000000000+00:00");
  REQUIRE(local.isoformat() == "2024-02-29T07:30:00.000000000-05:00");
}
