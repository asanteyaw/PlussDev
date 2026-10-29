#include <pluss/ma.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

namespace math = pluss::math;
namespace operators = pluss::operators;

namespace {

double cumulant(const math::Function &cgf, std::int64_t order,
                pluss::codem::Platform platform = pluss::pCPU) {
  const math::Function derivative = operators::differential("t", order)(cgf);
  const pluss::Config config =
      pluss::Config{}
          .with_dtype(platform == pluss::pMPS ? pluss::ScalarType::Float32
                                              : pluss::ScalarType::Float64)
          .with_platform(platform);
  return derivative({pluss::make_series(0.0, config)})
      .to(pluss::pCPU)
      .item<double>();
}

} // namespace

TEST_CASE("Function provenance distinguishes structural and quadrature graphs",
          "[calculus][integration][provenance]") {
  const math::Function polynomial =
      math::function({"x"}, [](math::Function x) { return x.pow(3); });
  const math::Function unresolved =
      math::function({"x"}, [](math::Function x) { return (-x.pow(2)).exp(); });

  CHECK(polynomial.provenance() == math::Provenance::Structural);
  CHECK(operators::differential("x")(polynomial).provenance() ==
        math::Provenance::Structural);
  CHECK(operators::integral("x", 0.0)(polynomial).provenance() ==
        math::Provenance::Structural);
  CHECK(operators::integral("x", 0.0)(unresolved).provenance() ==
        math::Provenance::Quadrature);
}

TEST_CASE("opaque Series callables require explicit autodiff fallback",
          "[calculus][integration][fallback]") {
  const math::Function opaque =
      math::function({"x"}, [](const pluss::Series &x) { return x.sin(); });
  const operators::Operator strict = operators::differential("x");
  const operators::Operator fallback =
      operators::differential("x", 2, {.fallback = math::Fallback::Autodiff});

  CHECK(opaque.provenance() == math::Provenance::Opaque);
  CHECK_THROWS(strict(opaque));

  const math::Function second = fallback(opaque);
  CHECK(second.provenance() == math::Provenance::Autodiff);
  CHECK(second({pluss::make_series(0.7)}).item<double>() ==
        Catch::Approx(-std::sin(0.7)).epsilon(1e-10));
}

TEST_CASE("autodiff fallback differentiates opaque Functions pointwise on arrays",
          "[calculus][integration][fallback][shape]") {
  const math::Function opaque = math::function(
      {"x"}, [](const pluss::Series &x) { return x.sin().mul(x.exp()); });
  const operators::DiffOptions autodiff{.fallback = math::Fallback::Autodiff};
  const pluss::Series x = pluss::make_series({0.5, 1.0, 1.5});
  const pluss::Series expected_first =
      x.cos().mul(x.exp()).add(x.sin().mul(x.exp()));
  const pluss::Series expected_second = x.cos().mul(x.exp()).mul(2.0);

  const pluss::Series first = operators::differential("x", 1, autodiff)(opaque)({x});
  const pluss::Series second = operators::differential("x", 2, autodiff)(opaque)({x});
  CHECK(first.sizes().equals(x.sizes()));
  CHECK(second.sizes().equals(x.sizes()));
  CHECK(first.allclose(expected_first, 1e-9, 1e-9));
  CHECK(second.allclose(expected_second, 1e-9, 1e-9));
}

TEST_CASE("gradient jacobian and hessian honour the autodiff fallback",
          "[calculus][integration][fallback][multivariable]") {
  const math::Function opaque = math::function(
      {"x"}, [](const pluss::Series &x) { return x.sin().mul(x.exp()); });
  const operators::DiffOptions autodiff{.fallback = math::Fallback::Autodiff};
  const pluss::Series point = pluss::make_series(0.5);

  CHECK_THROWS_AS(operators::gradient({"x"})(opaque), pluss::Error);
  CHECK_THROWS_AS(operators::hessian({"x"})(opaque), pluss::Error);
  const pluss::Series gradient =
      operators::gradient({"x"}, autodiff)(opaque)({point});
  const pluss::Series hessian =
      operators::hessian({"x"}, autodiff)(opaque)({point});
  const pluss::Series jacobian =
      operators::jacobian({"x"}, autodiff)(opaque)({point});
  CHECK(gradient.host<double>().front() ==
        Catch::Approx(std::cos(0.5) * std::exp(0.5) +
                      std::sin(0.5) * std::exp(0.5))
            .epsilon(1e-9));
  CHECK(hessian.host<double>().front() ==
        Catch::Approx(2.0 * std::cos(0.5) * std::exp(0.5)).epsilon(1e-9));
  CHECK(jacobian.host<double>().front() == gradient.host<double>().front());
}

TEST_CASE("absolute and covariant differentials remain distinct operators",
          "[calculus][integration][operator]") {
  const math::Function square =
      math::function({"x"}, [](math::Function x) { return x.pow(2); });
  const math::Function connection =
      math::function({"x"}, [](math::Function) { return 3.0; });
  const math::Function absolute = operators::magnitude_differential("x")(square);
  const math::Function covariant =
      operators::covariant("x", connection)(square);
  const pluss::Series x = pluss::make_series({-2.0, 1.0, 3.0});

  CHECK(absolute({x}).allclose(x.mul(2.0).abs()));
  CHECK(covariant({x}).allclose(x.mul(2.0).add(x.pow(2).mul(3.0))));
  CHECK_FALSE(operators::magnitude_differential("x").capabilities().linear);
  CHECK(operators::covariant("x", connection).capabilities().linear);
}

TEST_CASE("generic Function expressions support cumulant workflows",
          "[calculus][integration][cgf]") {
  const math::Function normal = math::function(
      {"t"}, [](math::Function t) { return t * 1.5 + t.pow(2) * 2.0; });
  const math::Function bernoulli = math::function({"t"}, [](math::Function t) {
    return (math::function(0.7) + t.exp() * 0.3).log();
  });
  const math::Function binomial = bernoulli * 7.0;
  const math::Function poisson =
      math::function({"t"}, [](math::Function t) { return t.expm1() * 2.5; });
  const math::Function gamma = math::function({"t"}, [](math::Function t) {
    return t * 0.5 - (-t * 3.0).log1p() * 2.0;
  });
  const math::Function exponential = math::function(
      {"t"}, [](math::Function t) { return (-(t / 4.0)).log1p().mul(-1.0); });
  const math::Function laplace = math::function({"t"}, [](math::Function t) {
    return t - (math::function(1.0) - t.pow(2) * 0.5625).log();
  });

  CHECK(cumulant(normal, 1) == Catch::Approx(1.5));
  CHECK(cumulant(normal, 2) == Catch::Approx(4.0));
  CHECK(cumulant(normal, 3) == Catch::Approx(0.0).margin(1e-12));
  CHECK(cumulant(bernoulli, 2) == Catch::Approx(0.21).epsilon(1e-10));
  CHECK(cumulant(binomial, 2) == Catch::Approx(1.47).epsilon(1e-10));
  CHECK(cumulant(poisson, 3) == Catch::Approx(2.5).epsilon(1e-10));
  CHECK(cumulant(gamma, 3) == Catch::Approx(108.0).epsilon(1e-10));
  CHECK(cumulant(exponential, 3) == Catch::Approx(0.03125).epsilon(1e-10));
  CHECK(cumulant(laplace, 2) == Catch::Approx(1.125).epsilon(1e-10));
}

TEST_CASE("CGF structural cumulants retain MPS execution",
          "[calculus][integration][cgf][mps]") {
  if (!pluss::platform_available(pluss::pMPS)) {
    SKIP("MPS backend is unavailable");
  }

  const math::Function poisson =
      math::function({"t"}, [](math::Function t) { return t.expm1() * 2.5; });
  CHECK(cumulant(poisson, 2, pluss::pMPS) == Catch::Approx(2.5).epsilon(1e-5));
}
