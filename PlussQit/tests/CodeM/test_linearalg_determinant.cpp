#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/pluss.h>

#include <cmath>
#include <cstdint>
#include <vector>

TEST_CASE("linearalg logdet preserves the real logarithm domain", "[linearalg][determinant]") {
  const auto positive = pluss::make_series({{4.0, 7.0}, {2.0, 6.0}});
  const auto negative = pluss::make_series({{2.0, 6.0}, {4.0, 7.0}});
  const auto singular = pluss::make_series({{1.0, 2.0}, {2.0, 4.0}});

  REQUIRE(positive.linearalg.logdet().item<double>() == Catch::Approx(std::log(10.0)));
  REQUIRE(std::isnan(negative.linearalg.logdet().item<double>()));
  REQUIRE(std::isinf(singular.linearalg.logdet().item<double>()));
  REQUIRE(singular.linearalg.logdet().item<double>() < 0.0);

  const auto signed_log = negative.linearalg.slogdet().to_vec<double>();
  REQUIRE(signed_log.size() == 2);
  REQUIRE(signed_log[0] == Catch::Approx(-1.0));
  REQUIRE(signed_log[1] == Catch::Approx(std::log(10.0)));
}

TEST_CASE("linearalg determinant family preserves batch shapes", "[linearalg][determinant]") {
  const auto batch = pluss::make_series({
      {{1.0, 0.0}, {0.0, 3.0}},
      {{2.0, 1.0}, {1.0, 2.0}},
  });

  REQUIRE(batch.linearalg.det().sizes() == std::vector<std::int64_t>{2});
  REQUIRE(batch.linearalg.logdet().sizes() == std::vector<std::int64_t>{2});
  REQUIRE(batch.linearalg.slogdet().sizes() == std::vector<std::int64_t>{2, 2});
  REQUIRE(batch.linearalg.inverse().sizes() == std::vector<std::int64_t>{2, 2, 2});
}
