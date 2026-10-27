#include "../FunctionInternal.h"

#include <pluss/ma.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

namespace math = pluss::math;
namespace operators = pluss::operators;

namespace {

bool close(const pluss::Series &actual, const pluss::Series &expected,
           double tolerance = 1e-9) {
  return actual.allclose(expected, tolerance, tolerance);
}

const pluss::subsp::math::detail::FunctionImpl &
impl(const math::Function &function) {
  return pluss::subsp::math::detail::FunctionAccess::get(function);
}

} // namespace

TEST_CASE("function applies shape-safe local simplifications",
          "[calculus][simplification]") {
  const math::Function difference =
      math::function({"x"}, [](math::Function x) { return x - x; });
  const math::Function product =
      math::function({"x"}, [](math::Function x) { return 0.0 * x; });
  const math::Function identity =
      math::function({"x"}, [](math::Function x) { return -(-x); });
  const math::Function composition =
      math::function({"x"}, [](math::Function x) { return x.exp().log(); });
  const math::Function folded = math::function(
      {"x"}, [](math::Function) { return (math::function(2.0) + 3.0) * 4.0; });
  const pluss::Series x = pluss::make_series({-2.0, 0.0, 3.0});

  CHECK(close(difference({x}), pluss::series::zeros_like(x)));
  CHECK(close(product({x}), pluss::series::zeros_like(x)));
  CHECK(close(identity({x}), x));
  CHECK(close(composition({x}), x));
  CHECK(folded({x}).item<double>() == Catch::Approx(20.0));
  CHECK(impl(identity).plan().size() == 1);
  CHECK(impl(composition).plan().size() == 1);
}

TEST_CASE("Series remains the sole tensor value for arbitrary ranks",
          "[calculus][function][tensor]") {
  const pluss::Series tensor =
      pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0}, {2, 2, 2});
  const math::Function constant = math::function(tensor);

  CHECK(constant.rank() == 3);
  CHECK(constant.shape().equals(pluss::RangeView<std::int64_t>({2, 2, 2})));
  CHECK(close(constant(), tensor));
}

TEST_CASE("assumptions enable only mathematically justified rewrites",
          "[calculus][simplification][assumption]") {
  const math::Function root =
      math::function({"x"}, [](math::Function x) { return x.pow(2).sqrt(); });
  const math::Function ratio =
      math::function({"x"}, [](math::Function x) { return x / x; });
  const math::Function nonnegative =
      root.assume("x", math::Assumption::NonNegative);
  const math::Function nonzero = ratio.assume("x", math::Assumption::NonZero);
  const math::Function positive =
      root.assume("x", math::Assumption::NonNegative)
          .assume("x", math::Assumption::NonZero);
  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0});

  CHECK(root.assumption("x") == math::Assumption::Real);
  CHECK(nonnegative.assumption("x") == math::Assumption::NonNegative);
  CHECK(positive.assumption("x") == math::Assumption::Positive);
  CHECK(close(nonnegative({x}), x));
  CHECK(close(nonzero({x}), pluss::series::ones_like(x)));
  CHECK(impl(nonnegative).plan().size() < impl(root).plan().size());
  CHECK_THROWS_AS(root.assume("missing", math::Assumption::Positive),
                  pluss::Error);
  CHECK_THROWS_AS(root.assume("x", math::Assumption::Zero)
                      .assume("x", math::Assumption::NonZero),
                  pluss::Error);
}

TEST_CASE("function evaluation enforces real-valued primitive domains",
          "[calculus][domain]") {
  const math::Function logarithm =
      math::function({"x"}, [](math::Function x) { return x.log(); });
  const math::Function logarithm1p =
      math::function({"x"}, [](math::Function x) { return x.log1p(); });
  const math::Function root =
      math::function({"x"}, [](math::Function x) { return x.sqrt(); });
  const math::Function reciprocal =
      math::function({"x"}, [](math::Function x) { return 1.0 / x; });

  CHECK_THROWS_AS(logarithm({pluss::make_series({1.0, 0.0})}), pluss::Error);
  CHECK_THROWS_AS(logarithm1p({pluss::make_series({0.0, -1.0})}), pluss::Error);
  CHECK_THROWS_AS(root({pluss::make_series({1.0, -1.0})}), pluss::Error);
  CHECK_THROWS_AS(reciprocal({pluss::make_series({1.0, 0.0})}), pluss::Error);
  CHECK(close(logarithm({pluss::make_series({1.0, std::exp(1.0)})}),
              pluss::make_series({0.0, 1.0})));
}

TEST_CASE("absolute-value derivatives expose explicit non-smooth policies",
          "[calculus][differential][nonsmooth]") {
  const math::Function absolute =
      math::function({"x"}, [](math::Function x) { return x.abs(); });
  const math::Function strict = operators::differential("x")(absolute);
  const math::Function piecewise = operators::differential(
      "x", 1, {.nonsmooth = math::NonSmooth::Piecewise})(absolute);
  const math::Function subgradient = operators::differential(
      "x", 1, {.nonsmooth = math::NonSmooth::Subgradient})(absolute);
  const math::Function autodiff = operators::differential(
      "x", 1, {.nonsmooth = math::NonSmooth::Autodiff})(absolute);
  const pluss::Series away = pluss::make_series({-2.0, 3.0});
  const pluss::Series zero = pluss::make_series(0.0);

  CHECK(close(strict({away}), pluss::make_series({-1.0, 1.0})));
  CHECK_THROWS_AS(strict({zero}), pluss::Error);
  CHECK(std::isnan(piecewise({zero}).item<double>()));
  CHECK(subgradient({zero}).item<double>() == Catch::Approx(0.0));
  CHECK(autodiff({zero}).item<double>() == Catch::Approx(0.0));
}

TEST_CASE("derivative graph budgets reject uncontrolled expansion",
          "[calculus][differential][graph-limit]") {
  const math::Function function = math::function(
      {"x"}, [](math::Function x) { return x.pow(5) * x.sin(); });

  CHECK_THROWS_AS(
      operators::differential(
          "x", 1, {.nonsmooth = math::NonSmooth::Strict, .max_nodes = 0}),
      pluss::Error);
  CHECK_THROWS_AS(operators::differential("x", 2,
                                          {.nonsmooth = math::NonSmooth::Strict,
                                           .max_nodes = 3})(function),
                  pluss::Error);
  CHECK_NOTHROW(operators::differential(
      "x", 2,
      {.nonsmooth = math::NonSmooth::Strict, .max_nodes = 1000})(function));
}

TEST_CASE("domain and non-smooth policies execute on MPS",
          "[calculus][domain][mps]") {
  if (!pluss::platform_available(pluss::pMPS)) {
    SKIP("MPS backend is unavailable");
  }

  const math::Function absolute =
      math::function({"x"}, [](math::Function x) { return x.abs(); });
  const math::Function derivative = operators::differential(
      "x", 1, {.nonsmooth = math::NonSmooth::Subgradient})(absolute);
  const pluss::Series x = pluss::make_series({-2.0, 0.0, 3.0}).to(pluss::pMPS);
  const pluss::Series result = derivative({x});

  CHECK(result.platform() == pluss::pMPS);
  CHECK(close(result.to(pluss::pCPU), pluss::make_series({-1.0, 0.0, 1.0})));
}
