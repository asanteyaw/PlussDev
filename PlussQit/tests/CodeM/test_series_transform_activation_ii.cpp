#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/pluss.h>

#include <complex>
#include <limits>
#include <vector>

namespace {

using Catch::Approx;
using Complex = std::complex<double>;

}  // namespace

TEST_CASE("cplxpair groups conjugates and preserves the authored shape", "[series][cplxpair]") {
  const auto input = pluss::make_series(std::vector<Complex>{
      {3.0, 0.0}, {2.0, 4.0}, {1.0, -2.0}, {2.0, -4.0},
      {-1.0, 0.0}, {1.0, 2.0}}, {2, 3});
  const auto output = input.cplxpair();
  const auto values = output.host<Complex>();

  REQUIRE(output.sizes() == input.sizes());
  REQUIRE(values == std::vector<Complex>{
      {1.0, -2.0}, {1.0, 2.0}, {2.0, -4.0}, {2.0, 4.0},
      {-1.0, 0.0}, {3.0, 0.0}});
  REQUIRE_THROWS(
      pluss::make_series(std::vector<Complex>{{1.0, 2.0}, {3.0, 0.0}}, {2}).cplxpair());
}

TEST_CASE("dct and dst transform a selected axis and round trip", "[series][dct][dst]") {
  const auto input = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});

  REQUIRE(input.dct(-1).allclose(input.dct(1), 1e-12, 1e-12));
  REQUIRE(input.dst(-1).allclose(input.dst(1), 1e-12, 1e-12));
  REQUIRE(input.dct().idct().allclose(input, 1e-10, 1e-10));
  REQUIRE(input.dst().idst().allclose(input, 1e-10, 1e-10));
  REQUIRE_THROWS(input.dct(2));
  REQUIRE_THROWS(input.dst(-3));
  REQUIRE_THROWS(pluss::make_series(std::vector<Complex>{{1.0, 1.0}}, {1}).dct());
}

TEST_CASE("ediff1d preserves integer and complex value domains", "[series][ediff1d]") {
  const auto integers = pluss::make_series({{1, 4, 9}, {16, 25, 36}}).ediff1d();
  REQUIRE(integers.scalar_type() == pluss::ScalarType::Int64);
  REQUIRE(integers.host<std::int64_t>() == std::vector<std::int64_t>{3, 5, 7, 9, 11});

  const auto complex = pluss::make_series(
      std::vector<Complex>{{1.0, 1.0}, {4.0, -1.0}, {2.0, 3.0}}, {3}).ediff1d();
  REQUIRE(complex.host<Complex>() ==
          std::vector<Complex>{{3.0, -2.0}, {-2.0, 4.0}});
}

TEST_CASE("elu and hardsigmoid promote integral receivers and preserve boundary rules",
          "[series][elu][hardsigmoid]") {
  const auto elu = pluss::make_series({-1, 0, 1}).elu();
  REQUIRE(elu.scalar_type() == pluss::ScalarType::Float32);
  REQUIRE(elu.select(0, 0).item<float>() == Approx(-0.63212055f));
  REQUIRE(elu.select(0, 1).item<float>() == Approx(0.0f));
  REQUIRE(elu.select(0, 2).item<float>() == Approx(1.0f));

  const auto hard = pluss::make_series({-4, -3, 0, 3, 4}).hardsigmoid();
  REQUIRE(hard.scalar_type() == pluss::ScalarType::Float32);
  REQUIRE(hard.host<float>() ==
          std::vector<float>{0.0f, 0.0f, 0.5f, 1.0f, 1.0f});
}

TEST_CASE("fftshift supports all axes and an explicit negative axis", "[series][fftshift]") {
  const auto input = pluss::series::arange(0, 12, 1, pluss::ScalarType::Int64).reshape({3, 4});
  const std::vector<std::int64_t> last{-1};

  REQUIRE(input.fftshift().ifftshift().equal(input));
  REQUIRE(input.fftshift(last).host<std::int64_t>() ==
          std::vector<std::int64_t>{2, 3, 0, 1, 6, 7, 4, 5, 10, 11, 8, 9});
  const std::vector<std::int64_t> invalid{2};
  REQUIRE_THROWS(input.fftshift(invalid));
}

TEST_CASE("hardswish preserves its piecewise contract and reverse rule", "[series][hardswish]") {
  const auto input = pluss::make_series({-4.0, -3.0, -1.5, 0.0, 1.5, 3.0, 4.0});
  REQUIRE(input.hardswish().host<double>() ==
          std::vector<double>{0.0, 0.0, -0.375, 0.0, 1.125, 3.0, 4.0});

  const auto integers = pluss::make_series({-4, 0, 4}).hardswish();
  REQUIRE(integers.scalar_type() == pluss::ScalarType::Float32);
  REQUIRE(integers.host<float>() == std::vector<float>{0.0f, 0.0f, 4.0f});

  auto tracked = input.clone();
  tracked.tracks_slope_(true);
  tracked.hardswish().sum().reverse();
  REQUIRE(tracked.sensitivity().host<double>() ==
          std::vector<double>{0.0, 0.0, 0.0, 0.5, 1.0, 1.0, 1.0});
  REQUIRE_THROWS(pluss::make_series(std::vector<Complex>{{1.0, 1.0}}, {1}).hardswish());
}

TEST_CASE("hilbert builds the analytic signal along a selected axis", "[series][hilbert]") {
  const auto input = pluss::make_series({1.0, 2.0, 3.0, 4.0});
  const auto output = input.hilbert();
  const auto values = output.host<Complex>();
  REQUIRE(output.scalar_type() == pluss::ScalarType::Complex128);
  REQUIRE(output.sizes() == input.sizes());
  REQUIRE(values[0].real() == Approx(1.0));
  REQUIRE(values[0].imag() == Approx(1.0));
  REQUIRE(values[1].real() == Approx(2.0));
  REQUIRE(values[1].imag() == Approx(-1.0));
  REQUIRE(values[2].real() == Approx(3.0));
  REQUIRE(values[2].imag() == Approx(-1.0));
  REQUIRE(values[3].real() == Approx(4.0));
  REQUIRE(values[3].imag() == Approx(1.0));

  const auto matrix = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
  REQUIRE(matrix.hilbert(-1).allclose(matrix.hilbert(1), 1e-12, 1e-12));
  REQUIRE(matrix.hilbert(0).sizes() == matrix.sizes());
  REQUIRE_THROWS(matrix.hilbert(2));
}

TEST_CASE("idct and idst invert their matching real transforms", "[series][idct][idst]") {
  const auto input = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
  REQUIRE(input.dct(0).idct(0).allclose(input, 1e-10, 1e-10));
  REQUIRE(input.dst(0).idst(0).allclose(input, 1e-10, 1e-10));
  REQUIRE(input.dct(-1).idct(-1).allclose(input, 1e-10, 1e-10));
  REQUIRE(input.dst(-1).idst(-1).allclose(input, 1e-10, 1e-10));
  REQUIRE(pluss::make_series({1, 2, 3}).idct().scalar_type() == pluss::ScalarType::Float64);
  REQUIRE(pluss::make_series({1, 2, 3}).idst().scalar_type() == pluss::ScalarType::Float64);
  REQUIRE_THROWS(input.idct(2));
  REQUIRE_THROWS(input.idst(-3));
}

TEST_CASE("ifftshift is the exact complementary permutation", "[series][ifftshift]") {
  const auto even = pluss::series::arange(0, 8, 1, pluss::ScalarType::Int64);
  const auto odd = pluss::series::arange(0, 7, 1, pluss::ScalarType::Int64);
  REQUIRE(even.ifftshift().host<std::int64_t>() ==
          std::vector<std::int64_t>{4, 5, 6, 7, 0, 1, 2, 3});
  REQUIRE(odd.ifftshift().host<std::int64_t>() ==
          std::vector<std::int64_t>{3, 4, 5, 6, 0, 1, 2});
  REQUIRE(even.ifftshift().fftshift().equal(even));
  REQUIRE(odd.ifftshift().fftshift().equal(odd));
}

TEST_CASE("directional infinity predicates return shape-preserving booleans",
          "[series][isneginf][isposinf]") {
  const double inf = std::numeric_limits<double>::infinity();
  const auto input = pluss::make_series({-inf, -1.0, 0.0, inf,
                                         std::numeric_limits<double>::quiet_NaN()});
  REQUIRE(input.isneginf().scalar_type() == pluss::ScalarType::Bool);
  REQUIRE(input.isneginf().host<bool>() ==
          std::vector<bool>{true, false, false, false, false});
  REQUIRE(input.isposinf().host<bool>() ==
          std::vector<bool>{false, false, false, true, false});
  REQUIRE(pluss::make_series({1, -2, 3}).isneginf().host<bool>() ==
          std::vector<bool>{false, false, false});
  REQUIRE_THROWS(pluss::make_series(std::vector<Complex>{{inf, 0.0}}, {1}).isposinf());
}
