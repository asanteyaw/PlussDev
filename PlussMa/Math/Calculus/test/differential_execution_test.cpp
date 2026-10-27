#include "../FunctionInternal.h"

#include <pluss/ma.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cmath>

namespace math = pluss::math;
namespace operators = pluss::operators;

namespace {

bool close(const pluss::Series &actual, const pluss::Series &expected,
           double tolerance = 1e-9) {
  return actual.allclose(expected, tolerance, tolerance);
}

math::Function motivating_function() {
  return math::function({"x", "y"}, [](math::Function x, math::Function y) {
    return x.pow(2) * (x * y).sin() * x.cos().exp();
  });
}

} // namespace

TEST_CASE("differential transforms the motivating multivariable function",
          "[calculus][differential][partial]") {
  const math::Function function = motivating_function();
  const math::Function dx = operators::differential("x")(function);
  const math::Function dy = operators::differential("y")(function);
  const pluss::Series x = pluss::make_series({0.25, 0.5, 1.0, 1.5});
  const pluss::Series y = pluss::make_series({0.4, 0.7, 1.1, 1.4});
  const pluss::Series xy = x.mul(y);
  const pluss::Series envelope = x.cos().exp();
  const pluss::Series expected_x =
      envelope.mul(x.mul(2.0)
                       .mul(xy.sin())
                       .add(x.pow(2).mul(y).mul(xy.cos()))
                       .sub(x.pow(2).mul(x.sin()).mul(xy.sin())));
  const pluss::Series expected_y = x.pow(3).mul(xy.cos()).mul(envelope);

  CHECK(close(dx({x, y}), expected_x));
  CHECK(close(dy({x, y}), expected_y));
  CHECK(dx.variables()[0] == "x");
  CHECK(dx.variables()[1] == "y");
}

TEST_CASE("differential computes direct higher orders",
          "[calculus][differential][higher-order]") {
  const math::Function polynomial = math::function(
      {"x"}, [](math::Function x) { return x.pow(5) - 2.0 * x.pow(3) + x; });
  const math::Function third = operators::differential("x", 3)(polynomial);
  const math::Function fifth = operators::differential("x", 5)(polynomial);
  const pluss::Series x = pluss::make_series({-2.0, -0.5, 0.0, 1.5});

  CHECK(close(third({x}), x.pow(2).mul(60.0).sub(12.0)));
  CHECK(close(fifth({x}), pluss::series::ones_like(x).mul(120.0)));
}

TEST_CASE("differential covers every unary primitive rule",
          "[calculus][differential][primitive]") {
  const math::Function x =
      math::function({"x"}, [](math::Function value) { return value; });
  const pluss::Series positive = pluss::make_series({0.2, 0.5, 1.25, 2.0});
  const std::array<math::Function, 13> derivatives{
      operators::differential("x")(x.exp()),
      operators::differential("x")(x.log()),
      operators::differential("x")(x.log1p()),
      operators::differential("x")(x.expm1()),
      operators::differential("x")(x.sqrt()),
      operators::differential("x")(x.sin()),
      operators::differential("x")(x.cos()),
      operators::differential("x")(x.tan()),
      operators::differential("x")(x.sinh()),
      operators::differential("x")(x.cosh()),
      operators::differential("x")(x.tanh()),
      operators::differential("x")(x.abs()),
      operators::differential("x")(-x),
  };
  const std::array<pluss::Series, 13> expected{
      positive.exp(),
      pluss::series::ones_like(positive).div(positive),
      pluss::series::ones_like(positive).div(positive.add(1.0)),
      positive.exp(),
      pluss::series::ones_like(positive).div(positive.sqrt().mul(2.0)),
      positive.cos(),
      positive.sin().neg(),
      pluss::series::ones_like(positive).div(positive.cos().pow(2)),
      positive.cosh(),
      positive.sinh(),
      pluss::series::ones_like(positive).div(positive.cosh().pow(2)),
      positive.div(positive.abs()),
      pluss::series::ones_like(positive).neg(),
  };

  for (std::size_t i = 0; i < derivatives.size(); ++i) {
    CAPTURE(i);
    CHECK(close(derivatives[i]({positive}), expected[i]));
  }
}

TEST_CASE("differential covers binary and variable power rules",
          "[calculus][differential][primitive]") {
  const math::Function power =
      math::function({"x", "y"}, [](math::Function x, math::Function y) {
        return (x + y) * (x - y) / y + x.pow(y) + 2.0 / x;
      });
  const math::Function dx = operators::differential("x")(power);
  const math::Function dy = operators::differential("y")(power);
  const pluss::Series x = pluss::make_series({1.2, 1.8, 2.5});
  const pluss::Series y = pluss::make_series({0.7, 1.1, 1.6});
  const pluss::Series expected_x =
      x.mul(2.0).div(y).add(y.mul(x.pow(y.sub(1.0)))).sub(2.0 / x.pow(2));
  const pluss::Series expected_y =
      x.pow(2).div(y.pow(2)).neg().sub(1.0).add(x.pow(y).mul(x.log()));

  CHECK(close(dx({x, y}), expected_x));
  CHECK(close(dy({x, y}), expected_y));

  const math::Function exponential =
      math::function({"x"}, [](math::Function value) {
        return math::function(2.0).pow(value);
      });
  const math::Function derivative = operators::differential("x")(exponential);
  CHECK(close(derivative({x}), x.mul(0.0).add(2.0).pow(x).mul(std::log(2.0))));
}

TEST_CASE("differential preserves zero and one runtime shapes",
          "[calculus][differential][shape]") {
  const math::Function projection = math::function(
      {"x", "y"}, [](math::Function, math::Function y) { return y; });
  const math::Function dx = operators::differential("x")(projection);
  const math::Function dy = operators::differential("y")(projection);
  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0});
  const pluss::Series y = pluss::make_series({4.0, 5.0, 6.0});

  const pluss::Series zero = dx({x, y});
  const pluss::Series one = dy({x, y});
  CHECK(zero.sizes().equals(y.sizes()));
  CHECK(one.sizes().equals(y.sizes()));
  CHECK(close(zero, pluss::series::zeros_like(y)));
  CHECK(close(one, pluss::series::ones_like(y)));
}

TEST_CASE("structural derivatives match Series calc derivatives",
          "[calculus][differential][parity]") {
  const math::Function function = math::function({"x"}, [](math::Function x) {
    return x.pow(2) * x.sin() * x.cos().exp();
  });
  const math::Function first = operators::differential("x")(function);
  const math::Function second = operators::differential("x", 2)(function);

  for (const double value : std::array{0.3, 0.8, 1.4}) {
    const pluss::Series point = pluss::make_series(value);
    const auto callback = [](const pluss::Series &x) {
      return x.pow(2).mul(x.sin()).mul(x.cos().exp());
    };
    const double eager_first = point.calc.diff(callback, 1).item<double>();
    const double eager_second = point.calc.diff(callback, 2).item<double>();

    CHECK(first({point}).item<double>() ==
          Catch::Approx(eager_first).epsilon(1e-8));
    CHECK(second({point}).item<double>() ==
          Catch::Approx(eager_second).epsilon(1e-8));
  }
}

TEST_CASE("differential memoizes shared subexpressions",
          "[calculus][differential][memo]") {
  const math::Function function = math::function({"x"}, [](math::Function x) {
    const math::Function square = x * x;
    return square + square;
  });
  const math::Function derivative = operators::differential("x")(function);
  const auto &impl =
      pluss::subsp::math::detail::FunctionAccess::get(derivative);

  CHECK(impl.plan().size() == 6);
  REQUIRE(impl.root().children().size() == 2);
  CHECK(impl.root().children()[0].get() == impl.root().children()[1].get());
  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0});
  CHECK(close(derivative({x}), x.mul(4.0)));
}

TEST_CASE("differential validates variable and order",
          "[calculus][differential][errors]") {
  const math::Function function = motivating_function();

  CHECK_THROWS_AS(operators::differential("", 1), pluss::Error);
  CHECK_THROWS_AS(operators::differential("x", 0), pluss::Error);
  CHECK_THROWS_AS(operators::differential("x", -1), pluss::Error);
  CHECK_THROWS_AS(operators::differential("z")(function), pluss::Error);
}

TEST_CASE("structural derivatives retain MPS execution",
          "[calculus][differential][mps]") {
  if (!pluss::platform_available(pluss::pMPS)) {
    SKIP("MPS backend is unavailable");
  }

  const math::Function function = math::function(
      {"x"}, [](math::Function x) { return x.pow(3) * x.sin(); });
  const math::Function derivative = operators::differential("x")(function);
  const pluss::Config f32 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float32);
  const pluss::Series cpu = pluss::make_series({0.25, 0.75, 1.25}, {3}, f32);
  const pluss::Series mps = cpu.to(pluss::pMPS);
  const pluss::Series result = derivative({mps});
  const pluss::Series expected =
      mps.pow(2).mul(3.0).mul(mps.sin()).add(mps.pow(3).mul(mps.cos()));

  CHECK(result.platform() == pluss::pMPS);
  CHECK(close(result.to(pluss::pCPU), expected.to(pluss::pCPU), 1e-5));
}
