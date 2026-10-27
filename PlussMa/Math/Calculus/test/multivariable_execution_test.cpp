#include "../FunctionInternal.h"

#include <pluss/ma.h>

#include <catch2/catch_test_macros.hpp>

namespace math = pluss::math;
namespace operators = pluss::operators;

namespace {

bool close(const pluss::Series &actual, const pluss::Series &expected,
           double tolerance = 1e-9) {
  return actual.allclose(expected, tolerance, tolerance);
}

math::Function potential() {
  return math::function({"x", "y"}, [](math::Function x, math::Function y) {
    return x.pow(3) + x * y + y.pow(2);
  });
}

pluss::Series trailing_stack(const pluss::Series &first,
                             const pluss::Series &second) {
  return first.pile({second}, first.dim());
}

} // namespace

TEST_CASE("gradient uses canonical and selected variable order",
          "[calculus][multivariable][gradient]") {
  const math::Function function = potential();
  const math::Function canonical = operators::gradient()(function);
  const math::Function selected = operators::gradient({"y", "x"})(function);
  const pluss::Series x = pluss::make_series({0.5, 1.0, 2.0});
  const pluss::Series y = pluss::make_series({1.5, 2.0, 3.0});
  const pluss::Series dx = x.pow(2).mul(3.0).add(y);
  const pluss::Series dy = x.add(y.mul(2.0));

  REQUIRE(canonical.rank() == 1);
  REQUIRE(canonical.shape().size() == 1);
  CHECK(canonical.shape()[0] == 2);
  CHECK(close(canonical({x, y}), trailing_stack(dx, dy)));
  CHECK(close(selected({x, y}), trailing_stack(dy, dx)));
}

TEST_CASE("hessian produces trailing selector-ordered derivative axes",
          "[calculus][multivariable][hessian]") {
  const math::Function function = potential();
  const math::Function hessian = operators::hessian()(function);
  const math::Function reversed = operators::hessian({"y", "x"})(function);
  const pluss::Series x = pluss::make_series({0.5, 1.0, 2.0});
  const pluss::Series y = pluss::make_series({1.5, 2.0, 3.0});
  const pluss::Series one = pluss::series::ones_like(x);
  const pluss::Series row_x = trailing_stack(x.mul(6.0), one);
  const pluss::Series row_y = trailing_stack(one, one.mul(2.0));
  const pluss::Series expected = trailing_stack(row_x, row_y);
  const pluss::Series reversed_row_y = trailing_stack(one.mul(2.0), one);
  const pluss::Series reversed_row_x = trailing_stack(one, x.mul(6.0));
  const pluss::Series expected_reversed =
      trailing_stack(reversed_row_y, reversed_row_x);

  REQUIRE(hessian.rank() == 2);
  REQUIRE(hessian.shape().size() == 2);
  CHECK(hessian.shape()[0] == 2);
  CHECK(hessian.shape()[1] == 2);
  CHECK(close(hessian({x, y}), expected));
  CHECK(close(reversed({x, y}), expected_reversed));
}

TEST_CASE("jacobian differentiates vector-valued structural functions",
          "[calculus][multivariable][jacobian]") {
  const math::Function function = potential();
  const math::Function vector = operators::gradient()(function);
  const math::Function jacobian = operators::jacobian()(vector);
  const math::Function hessian = operators::hessian()(function);
  const pluss::Series x = pluss::make_series({0.5, 1.0, 2.0});
  const pluss::Series y = pluss::make_series({1.5, 2.0, 3.0});

  CHECK(vector.rank() == 1);
  CHECK(jacobian.rank() == 2);
  CHECK(jacobian.shape()[0] == 2);
  CHECK(jacobian.shape()[1] == 2);
  CHECK(close(jacobian({x, y}), hessian({x, y})));
}

TEST_CASE("multivariable outputs retain arbitrary batch dimensions",
          "[calculus][multivariable][batch]") {
  const math::Function gradient = operators::gradient()(potential());
  const pluss::Series x = pluss::make_series({0.5, 1.0, 2.0, 3.0}, {2, 2});
  const pluss::Series y = pluss::make_series({1.5, 2.0, 3.0, 4.0}, {2, 2});
  const pluss::Series result = gradient({x, y});
  const pluss::Series expected =
      trailing_stack(x.pow(2).mul(3.0).add(y), x.add(y.mul(2.0)));

  CHECK(result.sizes().equals(pluss::RangeView<std::int64_t>({2, 2, 2})));
  CHECK(close(result, expected));
}

TEST_CASE("gradient supports arbitrary function arity and subsets",
          "[calculus][multivariable][arity]") {
  const math::Function function =
      math::function({"w", "x", "y", "z"},
                     [](math::Function w, math::Function x, math::Function y,
                        math::Function z) { return w * x + y * z + w.pow(2); });
  const math::Function full = operators::gradient()(function);
  const math::Function subset = operators::gradient({"z", "w", "x"})(function);
  const pluss::Series w = pluss::make_series({1.0, 2.0});
  const pluss::Series x = pluss::make_series({3.0, 4.0});
  const pluss::Series y = pluss::make_series({5.0, 6.0});
  const pluss::Series z = pluss::make_series({7.0, 8.0});
  const pluss::Series dw = x.add(w.mul(2.0));
  const pluss::Series dx = w;
  const pluss::Series dy = z;
  const pluss::Series dz = y;
  const pluss::Series full_expected = dw.pile({dx, dy, dz}, dw.dim());
  const pluss::Series subset_expected = dz.pile({dw, dx}, dz.dim());

  CHECK(full.shape()[0] == 4);
  CHECK(subset.shape()[0] == 3);
  CHECK(close(full({w, x, y, z}), full_expected));
  CHECK(close(subset({w, x, y, z}), subset_expected));
}

TEST_CASE("multivariable operators validate selectors and output rank",
          "[calculus][multivariable][errors]") {
  const math::Function scalar = potential();
  const math::Function vector = operators::gradient()(scalar);
  const math::Function matrix = operators::hessian()(scalar);
  const math::Function constant = math::function(3.0);

  CHECK_THROWS_AS(operators::gradient({"x", "x"})(scalar), pluss::Error);
  CHECK_THROWS_AS(operators::gradient({""})(scalar), pluss::Error);
  CHECK_THROWS_AS(operators::jacobian({"z"})(scalar), pluss::Error);
  CHECK_THROWS_AS(operators::gradient()(vector), pluss::Error);
  CHECK_THROWS_AS(operators::hessian()(vector), pluss::Error);
  CHECK_THROWS_AS(operators::jacobian()(matrix), pluss::Error);
  CHECK_THROWS_AS(operators::gradient()(constant), pluss::Error);
}

TEST_CASE("multivariable derivative graphs preserve shared components",
          "[calculus][multivariable][memo]") {
  const math::Function function =
      math::function({"x", "y"}, [](math::Function x, math::Function y) {
        const math::Function shared = x * y;
        return shared + shared;
      });
  const math::Function gradient = operators::gradient()(function);
  const auto &impl = pluss::subsp::math::detail::FunctionAccess::get(gradient);

  REQUIRE(impl.root().primitive_id() ==
          pluss::subsp::math::detail::PrimitiveId::Stack);
  REQUIRE(impl.root().children().size() == 2);
  const pluss::Series x = pluss::make_series({1.0, 2.0});
  const pluss::Series y = pluss::make_series({3.0, 4.0});
  CHECK(close(gradient({x, y}), trailing_stack(y.mul(2.0), x.mul(2.0))));
}

TEST_CASE("gradient hessian and jacobian execute natively on MPS",
          "[calculus][multivariable][mps]") {
  if (!pluss::platform_available(pluss::pMPS)) {
    SKIP("MPS backend is unavailable");
  }

  const math::Function function = potential();
  const math::Function gradient = operators::gradient()(function);
  const math::Function hessian = operators::hessian()(function);
  const math::Function jacobian = operators::jacobian()(gradient);
  const pluss::Config f32 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float32);
  const pluss::Series x =
      pluss::make_series({0.5, 1.0, 2.0}, {3}, f32).to(pluss::pMPS);
  const pluss::Series y =
      pluss::make_series({1.5, 2.0, 3.0}, {3}, f32).to(pluss::pMPS);
  const pluss::Series gradient_result = gradient({x, y});
  const pluss::Series hessian_result = hessian({x, y});
  const pluss::Series jacobian_result = jacobian({x, y});

  CHECK(gradient_result.platform() == pluss::pMPS);
  CHECK(hessian_result.platform() == pluss::pMPS);
  CHECK(jacobian_result.platform() == pluss::pMPS);
  CHECK(close(jacobian_result.to(pluss::pCPU), hessian_result.to(pluss::pCPU),
              1e-5));
}
