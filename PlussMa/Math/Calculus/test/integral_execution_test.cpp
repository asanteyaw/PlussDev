#include <pluss/ma.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

namespace math = pluss::math;
namespace operators = pluss::operators;

namespace {

bool close(const pluss::Series &actual, const pluss::Series &expected,
           double tolerance = 1e-8) {
  return actual.allclose(expected, tolerance, tolerance);
}

math::Function gaussian_kernel() {
  return math::function({"x"},
                        [](math::Function x) { return (-x.pow(2)).exp(); });
}

} // namespace

TEST_CASE("antiderivative applies exact primitive and structural rules",
          "[calculus][integral][exact]") {
  const math::Function function = math::function(
      {"x"}, [](math::Function x) { return x.pow(3) + x.sin(); });
  const math::Function primitive = operators::antiderivative("x")(function);
  const pluss::Series x = pluss::make_series({0.5, 1.0, 2.0});
  const pluss::Series expected = x.pow(4).div(4.0).sub(x.cos());

  CHECK(close(primitive({x}), expected));
  CHECK(close(operators::differential("x")(primitive)({x}), function({x})));
}

TEST_CASE("anchored and definite integrals preserve exact semantics",
          "[calculus][integral][bounds]") {
  const math::Function square =
      math::function({"x"}, [](math::Function x) { return x.pow(2); });
  const math::Function anchored = operators::integral("x", 1.0)(square);
  const math::Function definite = operators::integral("x", 1.0, 3.0)(square);
  const pluss::Series x = pluss::make_series({1.0, 2.0, 4.0});
  const pluss::Series expected = x.pow(3).sub(1.0).div(3.0);

  CHECK(close(anchored({x}), expected));
  CHECK(definite.arity() == 0);
  CHECK(definite.variables().empty());
  CHECK(definite().item<double>() == Catch::Approx(26.0 / 3.0));
}

TEST_CASE("unresolved integrals delegate to Series-native quadrature",
          "[calculus][integral][quadrature]") {
  const math::Function kernel = gaussian_kernel();
  const math::Function anchored = operators::integral("x", 0.0)(kernel);
  const math::Function definite = operators::integral("x", 0.0, 1.0)(kernel);
  constexpr double expected = 0.7468241328124271;

  CHECK(anchored({pluss::make_series(1.0)}).item<double>() ==
        Catch::Approx(expected).epsilon(1e-8));
  CHECK(definite().item<double>() == Catch::Approx(expected).epsilon(1e-8));
}

TEST_CASE("unresolved definite integration batches remaining parameters",
          "[calculus][integral][batch]") {
  const math::Function function = math::function(
      {"x", "scale"}, [](math::Function x, math::Function scale) {
        return (-x.pow(2)).exp() * scale;
      });
  const math::Function area = operators::integral("x", 0.0, 1.0)(function);
  const pluss::Series scale = pluss::make_series({1.0, 2.0, 4.0});
  CHECK(area.arity() == 1);
  CHECK(area.variables()[0] == "scale");
  const pluss::Series result = area({scale});
  const pluss::Series expected = scale.mul(0.7468241328124271);

  CHECK(close(result, expected, 1e-8));
}

TEST_CASE("anchored quadrature accepts a one-element upper-bound array",
          "[calculus][integral][shape]") {
  const math::Function anchored = operators::integral("x", 0.0)(gaussian_kernel());
  const pluss::Series bound = pluss::make_series({1.0}, {1});
  CHECK(anchored({bound}).host<double>().front() ==
        Catch::Approx(0.7468241328124271).epsilon(1e-8));
}

TEST_CASE("symmetric odd integrals resolve exactly by parity",
          "[calculus][integral][parity]") {
  const math::Function odd =
      math::function({"x", "y"}, [](math::Function x, math::Function y) {
        return x.pow(2) * (x * y).sin() * x.cos().exp();
      });
  const math::Function symmetric = operators::integral("x", -2.0, 2.0)(odd);
  const pluss::Series y = pluss::make_series({0.25, 0.5, 1.0});
  const pluss::Series result = symmetric({y});

  CHECK(close(result, pluss::series::zeros_like(y)));
}

TEST_CASE("Fundamental Theorem differentiates unresolved anchored integrals",
          "[calculus][integral][fundamental-theorem]") {
  const math::Function kernel = gaussian_kernel();
  const math::Function anchored = operators::integral("x", 0.0)(kernel);
  const math::Function recovered = operators::differential("x")(anchored);
  const pluss::Series x = pluss::make_series({0.0, 0.25, 0.75, 1.0});

  CHECK(close(recovered({x}), kernel({x})));
}

TEST_CASE("integral factories validate variables and finite bounds",
          "[calculus][integral][errors]") {
  const math::Function function = gaussian_kernel();

  CHECK_THROWS_AS(operators::antiderivative(""), pluss::Error);
  CHECK_THROWS_AS(operators::integral("", 0.0), pluss::Error);
  CHECK_THROWS_AS(operators::integral("missing", 0.0)(function), pluss::Error);
  CHECK_THROWS_AS(operators::integral("x", INFINITY)(function), pluss::Error);
  CHECK_THROWS_AS(operators::integral("x", 0.0, NAN)(function), pluss::Error);
}

TEST_CASE("unresolved quadrature retains MPS execution",
          "[calculus][integral][mps]") {
  if (!pluss::platform_available(pluss::pMPS)) {
    SKIP("MPS backend is unavailable");
  }

  const math::Function function = math::function(
      {"x", "scale"}, [](math::Function x, math::Function scale) {
        return (-x.pow(2)).exp() * scale;
      });
  const math::Function area = operators::integral("x", 0.0, 1.0)(function);
  const pluss::Config f32 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float32);
  const pluss::Series scale = pluss::make_series(1.0, f32).to(pluss::pMPS);
  const pluss::Series result = area({scale});

  CHECK(result.platform() == pluss::pMPS);
  CHECK(result.to(pluss::pCPU).item<double>() ==
        Catch::Approx(0.7468241328124271).epsilon(1e-5));
}
