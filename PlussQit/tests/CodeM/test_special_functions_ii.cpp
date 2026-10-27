#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <cmath>
#include <cstdint>
#include <limits>

using pluss::ScalarType;

TEST_CASE("autodiff recording guards are public pluss types", "[public-api][no-grad]") {
  auto x = pluss::make_series({2.0});
  x.tracks_slope_(true);

  auto untracked = pluss::series::undefined();
  {
    pluss::NoGradGuard guard;
    untracked = x.square();
    REQUIRE_FALSE(untracked.tracks_slope());
  }

  const auto tracked = x.square();
  REQUIRE(tracked.tracks_slope());
}

TEST_CASE("special functions follow endpoint and continuation contracts", "[special-functions-ii]") {
  const auto zero = pluss::make_series({0.0, -0.0}).digamma().host<double>();
  REQUIRE(std::isinf(zero[0]));
  REQUIRE(std::signbit(zero[0]));
  REQUIRE(std::isinf(zero[1]));
  REQUIRE_FALSE(std::signbit(zero[1]));

  const auto inv = pluss::make_series({-1.0, -0.5, 0.0, 0.5, 1.0, 1.1}).erfinv().host<double>();
  REQUIRE(inv[0] == -std::numeric_limits<double>::infinity());
  REQUIRE(std::abs(inv[1] + 0.4769362762044699) < 1e-13);
  REQUIRE(inv[2] == 0.0);
  REQUIRE(std::abs(inv[3] - 0.4769362762044699) < 1e-13);
  REQUIRE(inv[4] == std::numeric_limits<double>::infinity());
  REQUIRE(std::isnan(inv[5]));

  const auto z = pluss::make_series({2.0, 3.0}).zeta(-0.5).host<double>();
  REQUIRE(std::abs(z[0] - 8.934802200544679) < 1e-12);
  REQUIRE(std::abs(z[1] - 0.4143983221171599) < 1e-12);
  REQUIRE(std::isinf(pluss::make_series({2.0}).zeta(0.0).host<double>().front()));

  const auto pg = pluss::make_series({-0.5}).polygamma(1).host<double>().front();
  REQUIRE(std::abs(pg - 8.934802200544679) < 1e-12);
}

TEST_CASE("special functions preserve floating dtype and reject lossy inplace calls", "[special-functions-ii]") {
  const auto f32 = pluss::series::full({2}, 0.5, ScalarType::Float32);
  REQUIRE(f32.digamma().scalar_type() == ScalarType::Float32);
  REQUIRE(f32.erfinv().scalar_type() == ScalarType::Float32);
  REQUIRE(f32.i0().scalar_type() == ScalarType::Float32);
  REQUIRE(f32.sinc().scalar_type() == ScalarType::Float32);
  REQUIRE(f32.mvlgamma(2).scalar_type() == ScalarType::Float32);
  REQUIRE(f32.polygamma(1).scalar_type() == ScalarType::Float32);
  REQUIRE(f32.igamma(1.0).scalar_type() == ScalarType::Float32);
  REQUIRE(f32.zeta(1.0).scalar_type() == ScalarType::Float32);
  REQUIRE(f32.beta(2.0).scalar_type() == ScalarType::Float32);

  auto integers = pluss::make_series({0, 1});
  REQUIRE(integers.sinc().scalar_type() == ScalarType::Float64);
  REQUIRE_THROWS(integers.digamma_());
  REQUIRE_THROWS(integers.erfinv_());
  REQUIRE_THROWS(integers.i0_());
  REQUIRE_THROWS(integers.sinc_());
  REQUIRE_THROWS(integers.mvlgamma_(2));
  REQUIRE_THROWS(integers.polygamma_(1));
  REQUIRE_THROWS(integers.igamma_(1.0));

  auto inplace = pluss::make_series({0.0, 0.5});
  const void* storage = inplace.raw_data();
  inplace.erfinv_();
  REQUIRE(inplace.raw_data() == storage);
  REQUIRE(std::abs(inplace.host<double>()[1] - 0.4769362762044699) < 1e-13);

  auto shapes = pluss::make_series({1.0, 2.0}).reshape({2, 1});
  REQUIRE_THROWS(shapes.igamma_(pluss::make_series({0.5, 1.0, 2.0})));
}

TEST_CASE("i0 and igamma reverse and infinity behavior match their formulas", "[special-functions-ii]") {
  auto x = pluss::make_series({-1.0, 0.0, 1.0});
  x.tracks_slope_(true);
  x.i0().sum().reverse();
  const auto grad = x.sensitivity().host<double>();
  REQUIRE(std::abs(grad[0] + 0.565159103992485) < 2e-7);
  REQUIRE(std::abs(grad[1]) < 1e-15);
  REQUIRE(std::abs(grad[2] - 0.565159103992485) < 2e-7);

  const auto a = pluss::make_series({2.0, 2.0});
  const auto limits = pluss::make_series({0.0, std::numeric_limits<double>::infinity()});
  const auto lower = a.igamma(limits).host<double>();
  const auto upper = a.igammac(limits).host<double>();
  REQUIRE(lower[0] == 0.0);
  REQUIRE(lower[1] == 1.0);
  REQUIRE(upper[0] == 1.0);
  REQUIRE(upper[1] == 0.0);
}
