#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <pluss/pluss.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

namespace {

void require_close(const std::vector<double>& got,
                   const std::vector<double>& expected,
                   double tolerance = 1.0e-10) {
  REQUIRE(got.size() == expected.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    REQUIRE(std::abs(got[i] - expected[i]) <= tolerance);
  }
}

} // namespace

TEST_CASE("fmax and fmin skip one NaN and split finite ties", "[elementwise-ii][autodiff]") {
  const double nan = std::numeric_limits<double>::quiet_NaN();
  auto a = pluss::make_series({1.0, 4.0, 3.0, nan});
  auto b = pluss::make_series({2.0, 4.0, nan, 5.0});
  a.tracks_slope_(true);
  b.tracks_slope_(true);

  a.fmax(b).sum().reverse();
  require_close(a.sensitivity().to_vec<double>(), {0.0, 0.5, 1.0, 0.0});
  require_close(b.sensitivity().to_vec<double>(), {1.0, 0.5, 0.0, 1.0});

  auto c = pluss::make_series({1.0, 4.0, 3.0, nan});
  auto d = pluss::make_series({2.0, 4.0, nan, 5.0});
  c.tracks_slope_(true);
  d.tracks_slope_(true);
  c.fmin(d).sum().reverse();
  require_close(c.sensitivity().to_vec<double>(), {1.0, 0.5, 1.0, 0.0});
  require_close(d.sensitivity().to_vec<double>(), {0.0, 0.5, 0.0, 1.0});
}

TEST_CASE("heaviside enforces dtype and broadcasts its zero values", "[elementwise-ii]") {
  const auto x = pluss::make_series({{-2.0, 0.0, 3.0}, {0.0, -1.0, 2.0}});
  const auto values = pluss::make_series({{0.25, 0.5, 0.75}});
  require_close(x.heaviside(values).to_vec<double>(), {0.0, 0.5, 1.0, 0.25, 0.0, 1.0});
  REQUIRE_THROWS_WITH(x.heaviside(pluss::make_series({{1, 1, 1}})),
                      Catch::Matchers::ContainsSubstring("same dtype"));
}

TEST_CASE("gcd and lcm broadcast, normalize signs, and promote integer dtypes", "[elementwise-ii]") {
  const auto a = pluss::make_series({{-12, 18, 0}, {20, -27, 14}}).to(pluss::ScalarType::Int32);
  const auto b = pluss::make_series({{8, 6, 5}});
  REQUIRE(a.gcd(b).scalar_type() == pluss::ScalarType::Int64);
  REQUIRE(a.gcd(b).to_vec<std::int64_t>() == std::vector<std::int64_t>{4, 6, 5, 4, 3, 1});
  REQUIRE(a.lcm(b).to_vec<std::int64_t>() == std::vector<std::int64_t>{24, 18, 0, 40, 54, 70});
}

TEST_CASE("frexp round-trips and routes mantissa sensitivity", "[elementwise-ii][autodiff]") {
  auto x = pluss::make_series({1.0, 6.0, 0.75, -8.0});
  x.tracks_slope_(true);
  auto [mantissa, exponent] = x.frexp();
  require_close(mantissa.ldexp(exponent).to_vec<double>(), x.to_vec<double>());
  mantissa.sum().reverse();
  require_close(x.sensitivity().to_vec<double>(), {0.5, 0.125, 1.0, 0.0625});
}

TEST_CASE("ldexp supports continuous exponents and both reverse routes", "[elementwise-ii][autodiff]") {
  auto x = pluss::make_series({2.0, 3.0});
  auto e = pluss::make_series({0.5, -1.0});
  x.tracks_slope_(true);
  e.tracks_slope_(true);
  x.ldexp(e).sum().reverse();
  require_close(x.sensitivity().to_vec<double>(), {std::sqrt(2.0), 0.5});
  require_close(e.sensitivity().to_vec<double>(),
                {2.0 * std::sqrt(2.0) * std::log(2.0), 1.5 * std::log(2.0)});
}

TEST_CASE("float_power promotes and floor_divide floors signed quotients", "[elementwise-ii]") {
  const auto powers = pluss::make_series({2, 4}).float_power(pluss::make_series({-1, 3}));
  REQUIRE(powers.scalar_type() == pluss::ScalarType::Float64);
  require_close(powers.to_vec<double>(), {0.5, 64.0});

  const auto quotients = pluss::make_series({7, -7, 8, -8})
                             .floor_divide(pluss::make_series({2, 2, -3, -3}));
  REQUIRE(quotients.to_vec<std::int64_t>() == std::vector<std::int64_t>{3, -4, -3, 2});
}

TEST_CASE("supported in-place forms preserve receiver storage", "[elementwise-ii][inplace]") {
  auto x = pluss::make_series({1.0, 2.0, 3.0});
  const void* storage = x.raw_data();
  x.ldexp_(pluss::make_series({1, 0, -1}));
  REQUIRE(x.raw_data() == storage);
  require_close(x.to_vec<double>(), {2.0, 2.0, 1.5});

  auto p = pluss::make_series({2.0, 4.0});
  const void* power_storage = p.raw_data();
  p.float_power_(pluss::make_series({-1, 2}));
  REQUIRE(p.raw_data() == power_storage);
  require_close(p.to_vec<double>(), {0.5, 16.0});

  auto integers = pluss::make_series({7, -7});
  const void* divide_storage = integers.raw_data();
  integers.floor_divide_(2.0);
  REQUIRE(integers.raw_data() == divide_storage);
  REQUIRE(integers.to_vec<std::int64_t>() == std::vector<std::int64_t>{3, -4});
}
