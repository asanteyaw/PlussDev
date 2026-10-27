#include "../OperatorInternal.h"

#include <pluss/ma.h>

#include <catch2/catch_test_macros.hpp>

#include <functional>
#include <memory>
#include <vector>

namespace math = pluss::math;
namespace operators = pluss::operators;

namespace {

bool close(const pluss::Series &actual, const pluss::Series &expected,
           double tolerance = 1e-10) {
  return actual.allclose(expected, tolerance, tolerance);
}

math::Function square_function() {
  return math::function({"x"}, [](math::Function x) { return x.pow(2); });
}

} // namespace

TEST_CASE("function exposes a coordinate in its own trace",
          "[calculus][function][argument]") {
  const math::Function square = square_function();
  const math::Function x = square.arg("x");
  const pluss::Series values = pluss::make_series({1.0, 2.0, 3.0});

  CHECK(x.arity() == 1);
  CHECK(close(x({values}), values));
  CHECK_THROWS_AS(square.arg("y"), pluss::Error);
}

TEST_CASE("identity shift scale and multiply transform Function graphs",
          "[calculus][operator][builtins]") {
  const math::Function square = square_function();
  const pluss::Series x = pluss::make_series({-1.0, 0.0, 2.0});

  const math::Function unchanged = operators::identity()(square);
  const math::Function shifted = operators::shift("x", 1.5)(square);
  const math::Function scaled = operators::scale(3.0)(square);
  const math::Function multiplied =
      operators::multiply(square.arg("x"))(square);

  CHECK(close(unchanged({x}), x.pow(2)));
  CHECK(close(shifted({x}), x.add(1.5).pow(2)));
  CHECK(close(scaled({x}), x.pow(2).mul(3.0)));
  CHECK(close(multiplied({x}), x.pow(3)));
  CHECK_THROWS_AS(operators::shift("y", 1.0)(square), pluss::Error);
  CHECK_THROWS_AS(operators::shift("", 1.0), pluss::Error);
}

TEST_CASE("composition applies inner first and flattens nested chains",
          "[calculus][operator][compose]") {
  const operators::Operator shift = operators::shift("x", 2.0);
  const operators::Operator twice = operators::scale(2.0);
  const operators::Operator negate = operators::scale(-1.0);
  const operators::Operator first = operators::compose(twice, shift);
  const operators::Operator nested = operators::compose(negate, first);
  const operators::Operator with_identity =
      operators::compose(operators::identity(), nested);
  const math::Function transformed = with_identity(square_function());
  const pluss::Series x = pluss::make_series({0.0, 1.0, 2.0});

  CHECK(close(transformed({x}), x.add(2.0).pow(2).mul(-2.0)));
  CHECK(operators::detail::OperatorAccess::component_count(first) == 2);
  CHECK(operators::detail::OperatorAccess::component_count(nested) == 3);
  CHECK(operators::detail::OperatorAccess::component_count(with_identity) == 3);
}

TEST_CASE("linear combinations support parallel operator algebra",
          "[calculus][operator][linear-combination]") {
  const operators::Operator combination = operators::linear_combination({
      {2.0, operators::shift("x", 1.0)},
      {-0.5, operators::scale(4.0)},
  });
  const math::Function transformed = combination(square_function());
  const pluss::Series x = pluss::make_series({-1.0, 0.0, 2.0});
  const pluss::Series expected =
      x.add(1.0).pow(2).mul(2.0).sub(x.pow(2).mul(2.0));

  CHECK(close(transformed({x}), expected));
  CHECK(combination.capabilities().linear);

  const std::vector<operators::Operator> ops{operators::identity()};
  const std::vector<double> empty;
  CHECK_THROWS_AS(operators::linear_combination(ops, empty), pluss::Error);
  CHECK_THROWS_AS(
      operators::linear_combination(pluss::RangeView<operators::Term>{}),
      pluss::Error);
}

TEST_CASE("commutator preserves operator order",
          "[calculus][operator][commutator]") {
  const math::Function square = square_function();
  const operators::Operator shift = operators::shift("x", 1.0);
  const operators::Operator multiply_x = operators::multiply(square.arg("x"));
  const operators::Operator bracket = operators::commutator(shift, multiply_x);
  const math::Function transformed = bracket(square);
  const pluss::Series x = pluss::make_series({-2.0, 0.0, 3.0});

  // [T_1, M_x]f = T_1(M_x f) - M_x(T_1 f) = T_1 f.
  CHECK(close(transformed({x}), x.add(1.0).pow(2)));
  CHECK(bracket.capabilities().linear);
}

TEST_CASE("custom operators use the same value type and capability contract",
          "[calculus][operator][custom]") {
  std::int64_t applications = 0;
  const operators::Operator custom = operators::define(
      "example.square_output",
      [&applications](const math::Function &input) {
        ++applications;
        return input * input;
      },
      operators::Options{
          .version = 3,
          .structural = true,
          .evaluated_only = false,
          .linear = false,
          .adjoint_known = false,
          .serializable = false,
          .platform_pure = true,
      });
  const math::Function transformed = custom(square_function());
  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0});

  CHECK(applications == 1);
  CHECK(custom.key() == "example.square_output");
  CHECK(custom.version() == 3);
  CHECK(custom.capabilities().structural);
  CHECK_FALSE(custom.capabilities().linear);
  CHECK(custom.capabilities().platform_pure);
  CHECK(close(transformed({x}), x.pow(4)));
}

TEST_CASE("custom operators own move-only callable state",
          "[calculus][operator][custom]") {
  auto factor = std::make_unique<double>(4.0);
  const operators::Operator custom = operators::define(
      [factor = std::move(factor)](const math::Function &input) {
        return input * *factor;
      });
  const math::Function transformed = custom(square_function());
  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0});

  CHECK(close(transformed({x}), x.pow(2).mul(4.0)));
}

TEST_CASE("operator contracts reject undefined values and false promises",
          "[calculus][operator][errors]") {
  const operators::Operator undefined;
  const math::Function square = square_function();
  const std::function<math::Function(const math::Function &)> empty;

  CHECK_THROWS_AS(undefined(square), pluss::Error);
  CHECK_THROWS_AS(operators::identity()(math::Function{}), pluss::Error);
  CHECK_THROWS_AS(operators::multiply(math::Function{}), pluss::Error);
  CHECK_THROWS_AS(operators::define("empty", empty), pluss::Error);
  CHECK_THROWS_AS(
      operators::define(
          "contradictory", [](const math::Function &input) { return input; },
          operators::Options{.structural = true, .evaluated_only = true}),
      pluss::Error);
  CHECK_THROWS_AS(operators::define(
                      "premature.codec",
                      [](const math::Function &input) { return input; },
                      operators::Options{.serializable = true}),
                  pluss::Error);
}

TEST_CASE("operator-transformed functions retain MPS execution",
          "[calculus][operator][mps]") {
  if (!pluss::platform_available(pluss::pMPS)) {
    SKIP("MPS backend is unavailable");
  }

  const operators::Operator plan =
      operators::compose(operators::scale(2.5), operators::shift("x", -0.25));
  const math::Function transformed = plan(square_function());
  const pluss::Config f32 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float32);
  const pluss::Series cpu = pluss::make_series({0.5, 1.0, 2.0}, {3}, f32);
  const pluss::Series mps = cpu.to(pluss::pMPS);
  const pluss::Series result = transformed({mps});
  const pluss::Series expected = mps.sub(0.25).pow(2).mul(2.5);

  CHECK(result.platform() == pluss::pMPS);
  CHECK(close(result.to(pluss::pCPU), expected.to(pluss::pCPU), 1e-6));
}
