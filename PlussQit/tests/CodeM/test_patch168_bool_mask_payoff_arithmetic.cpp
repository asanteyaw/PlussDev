#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>
#include <vector>

using pluss::codem::ScalarType;
using pluss::codem::Series;

TEST_CASE("bool mask payoff arithmetic works without explicit casts", "[arith][promotion]") {
  const Series is_call =
      pluss::make_series(std::vector<std::int64_t>{1, 0, 1}, {3, 1}).to(ScalarType::Bool);
  const Series s_t =
      pluss::make_series(std::vector<float>{10.0f, 20.0f, 30.0f, 40.0f, 50.0f, 60.0f}, {3, 2});
  const Series strike = pluss::make_series(std::vector<float>{15.0f, 25.0f, 35.0f}, {3, 1});

  const Series put_mask = pluss::series::ones_like(is_call) - is_call;
  REQUIRE(put_mask.scalar_type() == ScalarType::Int64);
  REQUIRE(put_mask.to_vec<std::int64_t>() == std::vector<std::int64_t>{0, 1, 0});

  const Series payoffs = is_call * (s_t - strike).relu() +
                         (pluss::series::ones_like(is_call) - is_call) * (strike - s_t).relu();
  REQUIRE(payoffs.scalar_type() == ScalarType::Float32);

  const auto got = payoffs.to_vec<float>();
  REQUIRE(got.size() == 6);
  REQUIRE(got[0] == Catch::Approx(0.0f));
  REQUIRE(got[1] == Catch::Approx(5.0f));
  REQUIRE(got[2] == Catch::Approx(0.0f));
  REQUIRE(got[3] == Catch::Approx(0.0f));
  REQUIRE(got[4] == Catch::Approx(15.0f));
  REQUIRE(got[5] == Catch::Approx(25.0f));
}
