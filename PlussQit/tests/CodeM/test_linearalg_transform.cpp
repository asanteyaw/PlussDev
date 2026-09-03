#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/pluss.h>

#include <cmath>
#include <cstdint>
#include <vector>

namespace {

void require_close(const pluss::Series& actual, const std::vector<double>& expected, double margin = 1e-9) {
  const auto values = actual.to_vec<double>();
  REQUIRE(values.size() == expected.size());
  for (std::size_t i = 0; i < values.size(); ++i) {
    REQUIRE(values[i] == Catch::Approx(expected[i]).margin(margin));
  }
}

} // namespace

TEST_CASE("linearalg matrix transformations preserve their matrix contracts", "[linearalg][transform]") {
  const auto jordan = pluss::make_series({{1.0, 2.0}, {0.0, 1.0}});
  require_close(jordan.linearalg.matrix_power(3), {1.0, 6.0, 0.0, 1.0});
  require_close(jordan.linearalg.matrix_power(0), {1.0, 0.0, 0.0, 1.0});
  require_close(jordan.linearalg.matrix_power(-1), {1.0, -2.0, 0.0, 1.0});

  const auto nilpotent = pluss::make_series({{0.0, 1.0}, {0.0, 0.0}});
  require_close(nilpotent.linearalg.matrix_exp(), {1.0, 1.0, 0.0, 1.0});

  const auto large = pluss::make_series({{20.0, 0.0}, {0.0, -20.0}});
  require_close(
      large.linearalg.matrix_exp(),
      {std::exp(20.0), 0.0, 0.0, std::exp(-20.0)},
      1e-6);
}

TEST_CASE("linearalg pinverse is SVD-defined for rank loss and rcond", "[linearalg][transform]") {
  const auto rank_one = pluss::make_series({{1.0, 2.0}, {2.0, 4.0}});
  require_close(rank_one.linearalg.pinverse(), {0.04, 0.08, 0.08, 0.16});

  const auto separated = pluss::make_series({{1.0, 0.0}, {0.0, 0.001}});
  require_close(separated.linearalg.pinverse(), {1.0, 0.0, 0.0, 1000.0}, 1e-8);
  require_close(separated.linearalg.pinverse(0.01), {1.0, 0.0, 0.0, 0.0});
  REQUIRE_THROWS_AS(separated.linearalg.pinverse(-0.1), pluss::Error);
}

TEST_CASE("linearalg matrix transformation reverse rules include matrix coupling", "[linearalg][transform]") {
  auto power = pluss::make_series({{2.0, 0.0}, {0.0, 3.0}});
  power.tracks_slope_(true);
  power.linearalg.matrix_power(3).sum().reverse();
  require_close(power.sensitivity(), {12.0, 19.0, 19.0, 27.0});

  auto exponential = pluss::make_series({{0.0, 0.0}, {0.0, std::log(2.0)}});
  exponential.tracks_slope_(true);
  exponential.linearalg.matrix_exp().sum().reverse();
  require_close(exponential.sensitivity(), {1.0, 1.0 / std::log(2.0), 1.0 / std::log(2.0), 2.0});

  auto rectangular = pluss::make_series({{1.0, 0.0}, {0.0, 1.0}, {1.0, 1.0}});
  rectangular.tracks_slope_(true);
  rectangular.linearalg.pinverse().sum().reverse();
  require_close(
      rectangular.sensitivity(),
      {-1.0 / 9.0, -1.0 / 9.0, -1.0 / 9.0, -1.0 / 9.0, -5.0 / 9.0, -5.0 / 9.0});
}

TEST_CASE("linearalg matrix transformations preserve batch tails", "[linearalg][transform]") {
  const auto square_batch = pluss::make_series({
      {{1.0, 1.0}, {0.0, 1.0}},
      {{2.0, 0.0}, {0.0, 3.0}},
  });
  REQUIRE(square_batch.linearalg.matrix_power(2).sizes() == std::vector<std::int64_t>{2, 2, 2});
  REQUIRE(square_batch.linearalg.matrix_exp().sizes() == std::vector<std::int64_t>{2, 2, 2});

  const auto rectangular_batch = pluss::make_series({
      {{1.0, 0.0}, {0.0, 1.0}, {1.0, 1.0}},
      {{2.0, 0.0}, {0.0, 1.0}, {0.0, 1.0}},
  });
  REQUIRE(rectangular_batch.linearalg.pinverse().sizes() == std::vector<std::int64_t>{2, 2, 3});
}
