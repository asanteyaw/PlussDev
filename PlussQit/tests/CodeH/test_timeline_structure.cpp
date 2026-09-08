#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

TEST_CASE("Timeline concat preserves axis-zero geometry and logical order",
          "[timeline-structure]") {
  const auto first = pluss::timeline::date({{"2024-01-01", "2024-01-02"}});
  const auto second = pluss::timeline::date({{"2024-01-03", "2024-01-04"}});
  const auto joined = first.concat(second);
  REQUIRE(joined.sizes().vec() == std::vector<std::int64_t>{2, 2});
  REQUIRE(joined.day().to_vec<std::int32_t>() ==
          std::vector<std::int32_t>{1, 2, 3, 4});

  const auto scalar_join = pluss::timeline::date("2024-01-01")
                               .concat(pluss::timeline::date("2024-01-02"));
  REQUIRE(scalar_join.sizes().vec() == std::vector<std::int64_t>{2});
  REQUIRE(scalar_join.day().to_vec<std::int32_t>() ==
          std::vector<std::int32_t>{1, 2});

  const auto third = pluss::timeline::date({{"2024-01-05", "2024-01-06"}});
  const auto many = first.concat({second, third});
  REQUIRE(many.sizes().vec() == std::vector<std::int64_t>{3, 2});
  REQUIRE(many.day().to_vec<std::int32_t>() ==
          std::vector<std::int32_t>{1, 2, 3, 4, 5, 6});
  REQUIRE(first.concat(std::vector<pluss::Timeline>{}).sizes().vec() ==
          std::vector<std::int64_t>{1, 2});

  const std::vector<std::string> mismatched_values{
      "2024-01-03", "2024-01-04", "2024-01-05"};
  REQUIRE_THROWS(first.concat(
      pluss::timeline::date(mismatched_values, {3, 1})));
  REQUIRE_THROWS(first.concat(pluss::timeline::date("2024-01-03")));
  REQUIRE_THROWS(first.concat(second.tz_convert("America/Edmonton")));

  const auto with_nat = pluss::timeline::make({"2024-01-01", ""})
                            .concat(pluss::timeline::date("2024-01-03"));
  REQUIRE(with_nat.isnat().to_vec<bool>() ==
          std::vector<bool>{false, true, false});
}

TEST_CASE("Timeline concat honors strided operands", "[timeline-structure]") {
  std::array<std::int64_t, 4> backing{
      1'704'067'200'000'000'000LL, 11,
      1'704'153'600'000'000'000LL, 22};
  const std::array<std::int64_t, 1> shape{2};
  const std::array<std::int64_t, 1> strides{2};
  const auto strided = pluss::Timeline::from_buffer(
      backing.data(), shape, strides, {}, "UTC");
  const auto joined = strided.concat(pluss::timeline::date("2024-01-03"));
  REQUIRE(joined.day().to_vec<std::int32_t>() ==
          std::vector<std::int32_t>{1, 2, 3});
}

TEST_CASE("Timeline repeat tiles complete axis-zero blocks",
          "[timeline-structure]") {
  const auto row = pluss::timeline::date({"2024-01-01", "2024-01-02"});
  const auto twice = row.repeat(2);
  REQUIRE(twice.sizes().vec() == std::vector<std::int64_t>{4});
  REQUIRE(twice.day().to_vec<std::int32_t>() ==
          std::vector<std::int32_t>{1, 2, 1, 2});

  const auto matrix = pluss::timeline::date({{"2024-01-01", "2024-01-02"}});
  const auto matrix_twice = matrix.repeat(2);
  REQUIRE(matrix_twice.sizes().vec() == std::vector<std::int64_t>{2, 2});
  REQUIRE(matrix_twice.day().to_vec<std::int32_t>() ==
          std::vector<std::int32_t>{1, 2, 1, 2});

  std::array<std::int64_t, 4> backing{
      1'704'067'200'000'000'000LL, 11,
      1'704'153'600'000'000'000LL, 22};
  const std::array<std::int64_t, 1> shape{2};
  const std::array<std::int64_t, 1> strides{2};
  const auto strided = pluss::Timeline::from_buffer(
      backing.data(), shape, strides, {}, "UTC");
  REQUIRE(strided.repeat(2).day().to_vec<std::int32_t>() ==
          std::vector<std::int32_t>{1, 2, 1, 2});

  REQUIRE(pluss::timeline::date("2024-01-01").repeat(1).sizes().vec() ==
          std::vector<std::int64_t>{1});
  REQUIRE(matrix.repeat(0).sizes().vec() ==
          std::vector<std::int64_t>{0, 2});
  REQUIRE(matrix.repeat(0).numel() == 0);
  REQUIRE_THROWS(matrix.repeat(-1));
  REQUIRE_THROWS(row.repeat(std::numeric_limits<std::int64_t>::max()));
}

TEST_CASE("Timeline empty_like preserves trailing geometry and timezone",
          "[timeline-structure]") {
  const auto scalar = pluss::timeline::date("2024-01-01")
                          .tz_convert("America/Edmonton");
  REQUIRE(scalar.empty_like().sizes().vec() ==
          std::vector<std::int64_t>{0});
  REQUIRE(scalar.empty_like().tz() == "America/Edmonton");

  const auto cube = pluss::timeline::date({
      {{"2024-01-01", "2024-01-02"}},
      {{"2024-01-03", "2024-01-04"}},
  });
  REQUIRE(cube.empty_like().sizes().vec() ==
          std::vector<std::int64_t>{0, 1, 2});
  REQUIRE(cube.empty_like().numel() == 0);
}

TEST_CASE("Timeline fill_like creates receiver-local midnight",
          "[timeline-structure]") {
  const auto like = pluss::timeline::date({{"2024-01-01", "2024-01-02"}})
                        .tz_convert("America/Edmonton");
  const auto numeric = like.fill_like(2035, 7, 8);
  REQUIRE(numeric.sizes().vec() == std::vector<std::int64_t>{1, 2});
  REQUIRE(numeric.tz() == "America/Edmonton");
  REQUIRE(numeric.year().to_vec<std::int32_t>() ==
          std::vector<std::int32_t>{2035, 2035});
  REQUIRE(numeric.month().to_vec<std::int32_t>() ==
          std::vector<std::int32_t>{7, 7});
  REQUIRE(numeric.day().to_vec<std::int32_t>() ==
          std::vector<std::int32_t>{8, 8});
  REQUIRE(numeric.hour().to_vec<std::int32_t>() ==
          std::vector<std::int32_t>{0, 0});

  const auto text = like.fill_like("2036-03-04");
  REQUIRE(text.year().to_vec<std::int32_t>() ==
          std::vector<std::int32_t>{2036, 2036});
  REQUIRE(text.month().to_vec<std::int32_t>() ==
          std::vector<std::int32_t>{3, 3});
  REQUIRE(text.day().to_vec<std::int32_t>() ==
          std::vector<std::int32_t>{4, 4});
  REQUIRE(text.hour().to_vec<std::int32_t>() ==
          std::vector<std::int32_t>{0, 0});

  REQUIRE(like.fill_like("2036/03/04").day().to_vec<std::int32_t>() ==
          std::vector<std::int32_t>{4, 4});

  REQUIRE_THROWS(like.fill_like(2023, 2, 29));
  REQUIRE_THROWS(like.fill_like("2036-03-04T12:00:00"));
  REQUIRE_THROWS(like.fill_like("not-a-date"));
}
