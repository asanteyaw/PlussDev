#include "../FunctionInternal.h"

#include <pluss/ma.h>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <string_view>
#include <vector>

namespace math = pluss::math;
namespace operators = pluss::operators;

namespace {

bool close(const pluss::Series &actual, const pluss::Series &expected,
           double tolerance = 1e-10) {
  return actual.allclose(expected, tolerance, tolerance);
}

} // namespace

TEST_CASE("function evaluates the motivating multivariable formula",
          "[calculus][function][evaluation]") {
  const math::Function formula =
      math::function({"x", "y"}, [](math::Function x, math::Function y) {
        return x.pow(2) * (x * y).sin() * x.cos().exp();
      });
  const pluss::Series x = pluss::make_series({0.25, 0.5, 1.0, 1.5});
  const pluss::Series y = pluss::make_series(0.75);

  const pluss::Series expected =
      x.pow(2).mul(x.mul(y).sin()).mul(x.cos().exp());
  const pluss::Series named = formula({{"x", x}, {"y", y}});
  const pluss::Series ordered = formula({x, y});

  CHECK(close(named, expected));
  CHECK(close(ordered, expected));
  CHECK(named.platform() == x.platform());
}

TEST_CASE("univariate functions accept native scalar arguments",
          "[calculus][function][evaluation][scalar]") {
  const math::Function twice = math::function(
      {"x"}, [](math::Function x) { return x + x; });

  const pluss::Series f64 = twice(3.0);
  const pluss::Series f32 = twice(3.0F);
  const pluss::Series integer = twice(3);
  const pluss::Series i64 = twice(std::int64_t{3});

  CHECK(f64.scalar_type() == pluss::ScalarType::Float64);
  CHECK(f32.scalar_type() == pluss::ScalarType::Float32);
  CHECK(integer.scalar_type() == pluss::ScalarType::Int64);
  CHECK(i64.scalar_type() == pluss::ScalarType::Int64);
  CHECK(f64.item<double>() == 6.0);
  CHECK(f32.item<float>() == 6.0F);
  CHECK(integer.item<std::int64_t>() == 6);
  CHECK(i64.item<std::int64_t>() == 6);
}

TEST_CASE("function primitive methods delegate to Series operations",
          "[calculus][function][registry]") {
  const pluss::Series positive = pluss::make_series({0.2, 0.5, 1.25, 2.0});
  const math::Function x =
      math::function({"x"}, [](math::Function value) { return value; });

  const std::array<math::Function, 13> functions{
      x.exp(), x.log(),  x.log1p(), x.expm1(), x.sqrt(), x.sin(), x.cos(),
      x.tan(), x.sinh(), x.cosh(),  x.tanh(),  x.abs(),  -x,
  };
  const std::array<pluss::Series, 13> expected{
      positive.exp(),  positive.log(),  positive.log1p(), positive.expm1(),
      positive.sqrt(), positive.sin(),  positive.cos(),   positive.tan(),
      positive.sinh(), positive.cosh(), positive.tanh(),  positive.abs(),
      positive.neg(),
  };

  for (std::size_t i = 0; i < functions.size(); ++i) {
    CAPTURE(i);
    CHECK(close(functions[i]({positive}), expected[i]));
  }
}

TEST_CASE("function arithmetic delegates with scalar and Function operands",
          "[calculus][function][registry]") {
  const math::Function arithmetic =
      math::function({"x", "y"}, [](math::Function x, math::Function y) {
        return ((2.0 + x) - y).mul(x + 1.0).div(3.0).pow(y);
      });
  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0});
  const pluss::Series y = pluss::make_series({1.0, 2.0, 0.5});
  const pluss::Series expected = pluss::codem::make_scalar_like(x, 2.0)
                                     .add(x)
                                     .sub(y)
                                     .mul(x.add(1.0))
                                     .div(3.0)
                                     .pow(y);

  CHECK(close(arithmetic({{"y", y}, {"x", x}}), expected));
}

TEST_CASE("function evaluation preserves Series broadcasting and float dtype",
          "[calculus][function][broadcast]") {
  const math::Function add = math::function(
      {"left", "right"},
      [](math::Function left, math::Function right) { return left + right; });
  const pluss::Config f32 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float32);
  const pluss::Series left = pluss::make_series({1.0, 2.0}, {2, 1}, f32);
  const pluss::Series right =
      pluss::make_series({10.0, 20.0, 30.0}, {1, 3}, f32);

  const pluss::Series result = add({{"left", left}, {"right", right}});
  const pluss::Series expected = left.add(right);

  CHECK(result.scalar_type() == pluss::ScalarType::Float32);
  CHECK(result.sizes().equals(pluss::RangeView<std::int64_t>({2, 3})));
  CHECK(close(result, expected, 1e-6));
}

TEST_CASE("function constants are isolated from source and result mutation",
          "[calculus][function][constant]") {
  pluss::Series source = pluss::make_series({1.0, 2.0, 3.0});
  const math::Function constant = math::function(source);

  source.add_(10.0, 1.0);
  pluss::Series first = constant();
  CHECK(close(first, pluss::make_series({1.0, 2.0, 3.0})));

  first.mul_(5.0);
  const pluss::Series second = constant();
  CHECK(close(second, pluss::make_series({1.0, 2.0, 3.0})));
}

TEST_CASE("function plan preserves shared expression nodes",
          "[calculus][function][plan]") {
  const math::Function function = math::function({"x"}, [](math::Function x) {
    const math::Function square = x * x;
    return square + square;
  });
  const auto &impl = pluss::subsp::math::detail::FunctionAccess::get(function);

  REQUIRE(impl.plan().size() == 3);
  REQUIRE(impl.plan()[1].releases == std::vector<std::size_t>{0});
  REQUIRE(impl.plan()[2].releases == std::vector<std::size_t>{1});
  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0});
  CHECK(close(function({x}), x.mul(x).add(x.mul(x))));
}

TEST_CASE("integral execution plans cache their integrand subplan",
          "[calculus][function][plan][integral]") {
  const math::Function function = math::function(
      {"x"}, [](math::Function x) { return x.pow(3).exp(); });
  const math::Function integrated = operators::integral("x", 0.0)(function);
  const auto &impl =
      pluss::subsp::math::detail::FunctionAccess::get(integrated);

  const auto integral_step = std::ranges::find_if(
      impl.plan(), [](const auto &step) {
        return step.node->kind() ==
               pluss::subsp::math::detail::NodeKind::Integral;
      });
  REQUIRE(integral_step != impl.plan().end());
  REQUIRE_FALSE(integral_step->integral_plan.empty());
  CHECK(integral_step->integral_plan.size() ==
        pluss::subsp::math::detail::FunctionAccess::get(function).plan().size());
}

TEST_CASE("function validates named and ordered evaluation bindings",
          "[calculus][function][binding]") {
  const math::Function add = math::function(
      {"x", "y"}, [](math::Function x, math::Function y) { return x + y; });
  const pluss::Series values = pluss::make_series({1.0, 2.0});

  CHECK_THROWS_AS(add(), pluss::Error);
  CHECK_THROWS_AS(add({{"x", values}}), pluss::Error);
  CHECK_THROWS_AS(add({{"x", values}, {"z", values}}), pluss::Error);
  CHECK_THROWS_AS(add({{"x", values}, {"x", values}, {"y", values}}),
                  pluss::Error);
  CHECK_THROWS_AS(add({values}), pluss::Error);
  CHECK_THROWS_AS(add({values, pluss::Series{}}), pluss::Error);
}

TEST_CASE("function rejects incompatible structural output shapes",
          "[calculus][function][shape]") {
  const math::Function left =
      math::function(pluss::make_series({1.0, 2.0}, {2}));
  const math::Function right =
      math::function(pluss::make_series({1.0, 2.0, 3.0}, {3}));
  CHECK_THROWS_AS(left + right, pluss::Error);

  const math::Function matrix =
      math::function(pluss::make_series({1.0, 2.0, 3.0, 4.0}, {2, 2}));
  const math::Function scalar = math::function(2.0);
  const math::Function broadcast = matrix * scalar;
  CHECK(broadcast.rank() == 2);
  CHECK(broadcast.shape()[0] == 2);
  CHECK(broadcast.shape()[1] == 2);
}

TEST_CASE("function rejects direct combination of independent variable traces",
          "[calculus][function][binding]") {
  const math::Function first =
      math::function({"x"}, [](math::Function x) { return x; });
  const math::Function second =
      math::function({"x"}, [](math::Function x) { return x; });
  CHECK_THROWS_AS(first + second, pluss::Error);
}

TEST_CASE("function evaluation follows the MPS Series backend when available",
          "[calculus][function][mps]") {
  if (!pluss::platform_available(pluss::pMPS)) {
    SKIP("MPS backend is unavailable");
  }

  const math::Function function = math::function(
      {"x"}, [](math::Function x) { return x.sin() * x.exp() + 2.0; });
  const pluss::Config f32 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float32);
  const pluss::Series cpu = pluss::make_series({0.2, 0.5, 1.0}, {3}, f32);
  const pluss::Series mps = cpu.to(pluss::pMPS);
  const pluss::Series result = function({mps});
  const pluss::Series expected = mps.sin().mul(mps.exp()).add(2.0);

  CHECK(result.platform() == pluss::pMPS);
  CHECK(close(result.to(pluss::pCPU), expected.to(pluss::pCPU), 1e-6));
}
