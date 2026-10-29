#include <pluss/ma.h>

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <vector>

namespace math = pluss::math;
namespace operators = pluss::operators;
namespace measure = pluss::operators::measure;

namespace {

constexpr double kPi = 3.14159265358979323846;

pluss::Series scalar(double value) { return pluss::make_series(std::vector<double>{value}, {1}).reshape({}); }
double at(const math::Function &f, std::initializer_list<double> point) {
  std::vector<pluss::Series> arguments;
  for (const double v : point) arguments.push_back(scalar(v));
  const pluss::Series out = f(pluss::RangeView<pluss::Series>(arguments)).to(pluss::pCPU, pluss::ScalarType::Float64).contiguous();
  return out.reshape({out.numel()}).host<double>()[0];
}
double at0(const math::Function &f) {
  const pluss::Series out = f().to(pluss::pCPU, pluss::ScalarType::Float64).contiguous();
  return out.reshape({out.numel()}).host<double>()[0];
}

math::Function square() { return math::function({"x"}, [](math::Function x) { return x * x; }); }
math::Function normal_pdf() { return math::function({"x"}, [](math::Function x) { return (-(x * x) * 0.5).exp() / std::sqrt(2.0 * kPi); }); }

} // namespace

TEST_CASE("Lebesgue measure agrees with the Riemann integral", "[calculus][measure]") {
  const math::Function F = measure::lebesgue("x", 0.0, 2.0)(square());
  CHECK(std::abs(at0(F) - 8.0 / 3.0) < 1e-9);
  const math::Function g = math::function({"x", "a"}, [](math::Function x, math::Function a) { return a * x; });
  CHECK(std::abs(at(measure::lebesgue("x", 0.0, 2.0)(g), {3.0}) - 6.0) < 1e-9);  // free variable a survives
}

TEST_CASE("null points never decide the integral", "[calculus][measure]") {
  const double nulls[] = {0.3, 0.7};
  const math::Function F = measure::lebesgue("x", 0.0, 1.0, nulls)(square());
  CHECK(std::abs(at0(F) - 1.0 / 3.0) < 1e-9);
}

TEST_CASE("atomic measures are exact sums", "[calculus][measure]") {
  const double points[] = {0.5, 1.0, 3.0}, weights[] = {0.2, 0.3, 0.5};
  const math::Function F = measure::atomic("x", points, weights)(square());
  CHECK(at0(F) == 0.2 * 0.25 + 0.3 * 1.0 + 0.5 * 9.0);
  const math::Function g = math::function({"x", "a"}, [](math::Function x, math::Function a) { return x * a; });
  CHECK(std::abs(at(measure::atomic("x", points, weights)(g), {2.0}) - 2.0 * (0.1 + 0.3 + 1.5)) < 1e-14);
}

TEST_CASE("expectation against a density and a mixed Stieltjes measure", "[calculus][measure]") {
  const math::Function second_moment = measure::weighted("x", normal_pdf(), -10.0, 10.0)(square());
  CHECK(std::abs(at0(second_moment) - 1.0) < 1e-8);
  // F = 0.3 x (a continuous part with density 0.3) plus atoms 0.25 at -1 and 0.15 at 2
  const math::Function continuous = math::function({"x"}, [](math::Function x) { return x * 0.3; });
  const double jumps[] = {-1.0, 2.0}, sizes[] = {0.25, 0.15};
  const math::Function mixed = measure::stieltjes("x", continuous, 0.0 - 4.0, 4.0, jumps, sizes)(square());
  // int_{-4}^{4} x^2 * 0.3 dx = 0.3 * 128/3 = 12.8; atoms: 0.25 + 0.6
  CHECK(std::abs(at0(mixed) - (12.8 + 0.25 + 0.6)) < 1e-8);
}

TEST_CASE("measure algebra: mixtures, change of measure, product measures", "[calculus][measure]") {
  const double points[] = {1.0}, weights[] = {2.0};
  const auto mix = operators::linear_combination({{0.5, measure::lebesgue("x", 0.0, 1.0)}, {1.0, measure::atomic("x", points, weights)}});
  CHECK(std::abs(at0(mix(square())) - (0.5 / 3.0 + 2.0)) < 1e-9);
  // Radon-Nikodym: int f d(nu) = int f (dnu/dmu) dmu with dnu/dmu = 3x^2 on [0,1] and mu = Lebesgue
  const math::Function dnu = math::function({"x"}, [](math::Function x) { return 3.0 * x * x; });
  const auto nu = operators::compose(measure::lebesgue("x", 0.0, 1.0), operators::multiply(dnu));
  CHECK(std::abs(at0(nu(math::function({"x"}, [](math::Function x) { return x; }))) - 0.75) < 1e-9);
  // product measure (Fubini): int_0^1 int_0^2 x y dx dy = 1
  const math::Function xy = math::function({"x", "y"}, [](math::Function x, math::Function y) { return x * y; });
  const auto product = operators::compose(measure::lebesgue("y", 0.0, 1.0), measure::lebesgue("x", 0.0, 2.0));
  CHECK(std::abs(at0(product(xy)) - 1.0) < 1e-8);
}
