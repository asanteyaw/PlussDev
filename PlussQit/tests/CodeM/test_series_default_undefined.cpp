#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <pluss/pluss.h>

#include <type_traits>
#include <vector>

static_assert(std::is_default_constructible_v<pluss::Series>);
static_assert(!std::is_constructible_v<pluss::Series, std::initializer_list<double>>);

TEST_CASE("default Series handles are undefined", "[series][undefined]") {
  pluss::Series lb{}, ub{}, scaler{}, pen_val{};

  REQUIRE_FALSE(lb.defined());
  REQUIRE_FALSE(ub.defined());
  REQUIRE_FALSE(scaler.defined());
  REQUIRE_FALSE(pen_val.defined());
}

TEST_CASE("default and factory undefined Series have the same state",
          "[series][undefined]") {
  const pluss::Series shorthand{};
  const auto explicit_form = pluss::series::undefined();

  REQUIRE(shorthand.defined() == explicit_form.defined());
  REQUIRE_FALSE(shorthand.defined());
  REQUIRE_FALSE(explicit_form.defined());
}

TEST_CASE("a default Series can receive and release a defined value",
          "[series][undefined]") {
  pluss::Series value{};
  value = pluss::make_series({2.0, 4.0, 8.0});

  REQUIRE(value.defined());
  REQUIRE(value.host<double>() == std::vector<double>{2.0, 4.0, 8.0});

  value = pluss::Series{};
  REQUIRE_FALSE(value.defined());
}

TEST_CASE("host is the bulk extraction boundary", "[series][host]") {
  const auto matrix = pluss::make_series({1, 2, 3, 4}, {2, 2});
  const auto exported = matrix.host<std::int64_t>();
  const std::vector<std::vector<std::int64_t>> native =
      matrix.host<std::int64_t>();

  REQUIRE(exported.is_array());
  REQUIRE(exported.array().data == std::vector<std::int64_t>{1, 2, 3, 4});
  REQUIRE(exported.array().sizes == std::vector<std::int64_t>{2, 2});
  REQUIRE(exported.array().strides == std::vector<std::int64_t>{2, 1});
  REQUIRE(exported.array().contiguous);
  REQUIRE(native == std::vector<std::vector<std::int64_t>>{{1, 2}, {3, 4}});

  const auto truth = pluss::make_series(true).host<bool>();
  REQUIRE(truth.is_scalar());
  REQUIRE(truth.scalar());
  REQUIRE(truth.array().data == std::vector<bool>{true});

  const auto singleton = pluss::make_series({true}).host<bool>();
  REQUIRE(singleton.is_vector());
  REQUIRE_FALSE(singleton.is_scalar());
}

TEST_CASE("item accepts only rank-zero Series", "[series][host][item]") {
  const auto scalar = pluss::make_series(2.5);
  REQUIRE(scalar.item<double>() == 2.5);

  REQUIRE_THROWS_WITH(pluss::make_series({2.5}).item<double>(),
                      Catch::Matchers::ContainsSubstring("Series::item<T>(): expected a rank-zero Series"));
  REQUIRE_THROWS_WITH(
      pluss::make_series({2.5}, {1, 1}).item<double>(),
                      Catch::Matchers::ContainsSubstring("Series::item<T>(): expected a rank-zero Series"));
}

TEST_CASE("pull accepts only rank-one Series", "[series][host][pull]") {
  const auto values = pluss::make_series({10, 20, 30, 40});
  REQUIRE(values.pull<std::int64_t>() ==
          std::vector<std::int64_t>{10, 20, 30, 40});

  REQUIRE_THROWS_WITH(pluss::make_series(10).pull<std::int64_t>(),
                      Catch::Matchers::ContainsSubstring("Series::pull<T>(): expected a rank-one Series"));
  REQUIRE_THROWS_WITH(
      pluss::make_series({10, 20, 30, 40}, {2, 2}).pull<std::int64_t>(),
                      Catch::Matchers::ContainsSubstring("Series::pull<T>(): expected a rank-one Series"));
}
