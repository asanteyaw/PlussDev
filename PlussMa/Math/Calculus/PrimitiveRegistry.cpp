#include "PrimitiveRegistry.h"

#include "pluss/codem/series_factory.h"
#include "pluss/error.h"

#include <array>

namespace pluss::subsp::math::detail {
namespace {

pluss::Series add(pluss::RangeView<pluss::Series> x) { return x[0].add(x[1]); }
pluss::Series sub(pluss::RangeView<pluss::Series> x) { return x[0].sub(x[1]); }
pluss::Series mul(pluss::RangeView<pluss::Series> x) { return x[0].mul(x[1]); }
pluss::Series div(pluss::RangeView<pluss::Series> x) {
  PLUSS_CHECK(x[1].ne(0.0).all().item<bool>(),
              "math::Function: division domain excludes zero denominators");
  return x[0].div(x[1]);
}
pluss::Series pow(pluss::RangeView<pluss::Series> x) { return x[0].pow(x[1]); }
pluss::Series neg(pluss::RangeView<pluss::Series> x) { return x[0].neg(); }
pluss::Series exp(pluss::RangeView<pluss::Series> x) { return x[0].exp(); }
pluss::Series log(pluss::RangeView<pluss::Series> x) {
  PLUSS_CHECK(x[0].gt(0.0).all().item<bool>(),
              "math::Function: log domain requires positive values");
  return x[0].log();
}
pluss::Series log1p(pluss::RangeView<pluss::Series> x) {
  PLUSS_CHECK(x[0].gt(-1.0).all().item<bool>(),
              "math::Function: log1p domain requires values greater than -1");
  return x[0].log1p();
}
pluss::Series expm1(pluss::RangeView<pluss::Series> x) { return x[0].expm1(); }
pluss::Series sqrt(pluss::RangeView<pluss::Series> x) {
  PLUSS_CHECK(x[0].ge(0.0).all().item<bool>(),
              "math::Function: sqrt domain requires nonnegative values");
  return x[0].sqrt();
}
pluss::Series sin(pluss::RangeView<pluss::Series> x) { return x[0].sin(); }
pluss::Series cos(pluss::RangeView<pluss::Series> x) { return x[0].cos(); }
pluss::Series tan(pluss::RangeView<pluss::Series> x) { return x[0].tan(); }
pluss::Series sinh(pluss::RangeView<pluss::Series> x) { return x[0].sinh(); }
pluss::Series cosh(pluss::RangeView<pluss::Series> x) { return x[0].cosh(); }
pluss::Series tanh(pluss::RangeView<pluss::Series> x) { return x[0].tanh(); }
pluss::Series abs(pluss::RangeView<pluss::Series> x) { return x[0].abs(); }
pluss::Series zeros_like(pluss::RangeView<pluss::Series> x) {
  return pluss::series::zeros_like(x[0]);
}
pluss::Series ones_like(pluss::RangeView<pluss::Series> x) {
  return pluss::series::ones_like(x[0]);
}
pluss::Series stack(pluss::RangeView<pluss::Series> x) {
  PLUSS_CHECK(!x.empty(), "math::Function: stack requires at least one input");
  return x.front().pile(x.drop_front(), x.front().dim());
}
pluss::Series abs_slope_strict(pluss::RangeView<pluss::Series> x) {
  PLUSS_CHECK(x[0].ne(0.0).all().item<bool>(),
              "operators::differential: abs is not differentiable at zero");
  return x[0].div(x[0].abs());
}
pluss::Series abs_slope_zero(pluss::RangeView<pluss::Series> x) {
  const pluss::Series zero = pluss::series::zeros_like(x[0]);
  return x[0].eq(0.0).where(zero, x[0].div(x[0].abs()));
}
pluss::Series abs_slope_piecewise(pluss::RangeView<pluss::Series> x) {
  return x[0].div(x[0].abs());
}

constexpr std::array<PrimitiveSpec, 24> registry{{
    {PrimitiveId::Add, "add", 2, add},
    {PrimitiveId::Sub, "sub", 2, sub},
    {PrimitiveId::Mul, "mul", 2, mul},
    {PrimitiveId::Div, "div", 2, div},
    {PrimitiveId::Pow, "pow", 2, pow},
    {PrimitiveId::Neg, "neg", 1, neg},
    {PrimitiveId::Exp, "exp", 1, exp},
    {PrimitiveId::Log, "log", 1, log},
    {PrimitiveId::Log1p, "log1p", 1, log1p},
    {PrimitiveId::Expm1, "expm1", 1, expm1},
    {PrimitiveId::Sqrt, "sqrt", 1, sqrt},
    {PrimitiveId::Sin, "sin", 1, sin},
    {PrimitiveId::Cos, "cos", 1, cos},
    {PrimitiveId::Tan, "tan", 1, tan},
    {PrimitiveId::Sinh, "sinh", 1, sinh},
    {PrimitiveId::Cosh, "cosh", 1, cosh},
    {PrimitiveId::Tanh, "tanh", 1, tanh},
    {PrimitiveId::Abs, "abs", 1, abs},
    {PrimitiveId::ZerosLike, "zeros_like", 1, zeros_like},
    {PrimitiveId::OnesLike, "ones_like", 1, ones_like},
    {PrimitiveId::Stack, "stack", 0, stack, true},
    {PrimitiveId::AbsSlopeStrict, "abs_slope_strict", 1, abs_slope_strict},
    {PrimitiveId::AbsSlopeZero, "abs_slope_zero", 1, abs_slope_zero},
    {PrimitiveId::AbsSlopePiecewise, "abs_slope_piecewise", 1,
     abs_slope_piecewise},
}};

} // namespace

const PrimitiveSpec &primitive(PrimitiveId id) {
  const std::size_t index = static_cast<std::size_t>(id) - 1;
  PLUSS_CHECK(index < registry.size() && registry[index].id == id,
              "math::Function: unknown primitive operation ID");
  return registry[index];
}

} // namespace pluss::subsp::math::detail
