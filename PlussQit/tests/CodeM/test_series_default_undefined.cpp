#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <type_traits>

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
  REQUIRE(value.to_vec<double>() == std::vector<double>{2.0, 4.0, 8.0});

  value = pluss::Series{};
  REQUIRE_FALSE(value.defined());
}
