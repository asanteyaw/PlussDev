#include <pluss/pluss.h>

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <complex>
#include <cstdint>
#include <limits>
#include <vector>

namespace {

std::vector<std::int64_t> shape_of(const pluss::Series& value) {
  return {value.sizes().begin(), value.sizes().end()};
}

} // namespace

TEST_CASE("bernoulli separates probability input from in-place scalar sampling", "[random-sampling]") {
  const auto probabilities = pluss::make_series({0.0, 1.0, 0.0, 1.0});
  const auto sampled = probabilities.bernoulli();
  REQUIRE(sampled.scalar_type() == probabilities.scalar_type());
  REQUIRE(sampled.host<double>() == std::vector<double>{0.0, 1.0, 0.0, 1.0});
  REQUIRE_THROWS(pluss::make_series({-0.1, 0.5}).bernoulli());
  REQUIRE_THROWS(pluss::make_series({0.5, std::numeric_limits<double>::infinity()}).bernoulli());

  auto destination = pluss::make_series({9.0, 9.0, 9.0});
  auto alias = destination;
  destination.bernoulli_(1.0);
  REQUIRE(alias.host<double>() == std::vector<double>{1.0, 1.0, 1.0});
  REQUIRE(destination.is_set_to(alias));
  REQUIRE_THROWS(destination.bernoulli_(1.1));
}

TEST_CASE("multinomial samples each row and retains batch shape", "[random-sampling]") {
  const auto probabilities = pluss::make_series({{0.0, 1.0, 0.0},
                                                   {1.0, 0.0, 0.0}});
  const auto draws = probabilities.multinomial(4, true);
  REQUIRE(shape_of(draws) == std::vector<std::int64_t>{2, 4});
  REQUIRE(draws.scalar_type() == pluss::ScalarType::Int64);
  REQUIRE(draws.host<std::int64_t>() ==
          std::vector<std::int64_t>{1, 1, 1, 1, 0, 0, 0, 0});

  REQUIRE_THROWS(pluss::Series::ones({2, 2, 2}, pluss::ScalarType::Float64).multinomial(1));
  REQUIRE_THROWS(pluss::make_series({0.0, 0.0}).multinomial(1));
  REQUIRE_THROWS(pluss::make_series({0.5, -0.5}).multinomial(1));
  REQUIRE_THROWS(pluss::make_series({0.5, 0.5}).multinomial(3, false));
  REQUIRE_THROWS(pluss::make_series({1.0, 0.0, 0.0}).multinomial(2, false));
}

TEST_CASE("normal uniform and random obey distinct value contracts", "[random-sampling]") {
  auto normal = pluss::Series::empty({4}, pluss::ScalarType::Float64);
  auto normal_alias = normal;
  normal.normal_(3.5, 0.0);
  REQUIRE(normal_alias.host<double>() == std::vector<double>{3.5, 3.5, 3.5, 3.5});
  REQUIRE(normal.is_set_to(normal_alias));

  auto uniform = pluss::Series::empty({3}, pluss::ScalarType::Float32);
  uniform.uniform_(-2.0, -2.0);
  REQUIRE(uniform.host<float>() == std::vector<float>{-2.0f, -2.0f, -2.0f});
  REQUIRE_THROWS(pluss::Series::empty({2}, pluss::ScalarType::Int64).uniform_());

  auto complex_uniform = pluss::Series::empty({2}, pluss::ScalarType::Complex64);
  complex_uniform.uniform_(-2.0, -2.0);
  REQUIRE(complex_uniform.host<std::complex<float>>() ==
          std::vector<std::complex<float>>{{-2.0f, -2.0f}, {-2.0f, -2.0f}});

  auto complex_normal = pluss::Series::empty({2}, pluss::ScalarType::Complex64);
  complex_normal.normal_(3.5, 0.0);
  REQUIRE(complex_normal.host<std::complex<float>>() ==
          std::vector<std::complex<float>>{{3.5f, 0.0f}, {3.5f, 0.0f}});

  auto discrete = pluss::Series::empty({32}, pluss::ScalarType::Float64);
  discrete.random_(-2, 3);
  for (double value : discrete.host<double>()) {
    REQUIRE(value == std::floor(value));
    REQUIRE(value >= -2.0);
    REQUIRE(value < 3.0);
  }
  REQUIRE_THROWS(pluss::Series::empty({2}, pluss::ScalarType::Complex64).random_());
  REQUIRE_THROWS(pluss::Series::empty({2}, pluss::ScalarType::Int8).random_(0, 200));
}

TEST_CASE("remaining in-place distributions validate domains and mutate aliases", "[random-sampling]") {
  auto values = pluss::Series::empty({64}, pluss::ScalarType::Float64);
  auto alias = values;

  values.exponential_(2.0);
  for (double value : alias.host<double>()) REQUIRE(value >= 0.0);

  values.geometric_(0.25);
  for (double value : alias.host<double>()) {
    REQUIRE(value >= 1.0);
    REQUIRE(value == std::floor(value));
  }

  values.log_normal_(0.0, 0.5);
  for (double value : alias.host<double>()) REQUIRE(value > 0.0);

  values.cauchy_(2.0, 0.5);
  REQUIRE(values.is_set_to(alias));
  REQUIRE_THROWS(values.exponential_(0.0));
  REQUIRE_THROWS(values.geometric_(1.0));
  REQUIRE_THROWS(values.log_normal_(0.0, 0.0));
  REQUIRE_THROWS(values.cauchy_(0.0, 0.0));
}
