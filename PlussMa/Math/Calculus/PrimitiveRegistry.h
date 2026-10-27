#pragma once

#include "pluss/codem/codem.h"

#include <cstdint>
#include <string_view>

namespace pluss::subsp::math::detail {

enum class PrimitiveId : std::uint16_t {
  Add = 1,
  Sub = 2,
  Mul = 3,
  Div = 4,
  Pow = 5,
  Neg = 6,
  Exp = 7,
  Log = 8,
  Log1p = 9,
  Expm1 = 10,
  Sqrt = 11,
  Sin = 12,
  Cos = 13,
  Tan = 14,
  Sinh = 15,
  Cosh = 16,
  Tanh = 17,
  Abs = 18,
  ZerosLike = 19,
  OnesLike = 20,
  Stack = 21,
  AbsSlopeStrict = 22,
  AbsSlopeZero = 23,
  AbsSlopePiecewise = 24,
};

using PrimitiveEvaluator =
    pluss::Series (*)(pluss::RangeView<pluss::Series> arguments);

struct PrimitiveSpec final {
  PrimitiveId id;
  std::string_view name;
  std::uint8_t arity;
  PrimitiveEvaluator evaluate;
  bool variadic = false;
};

[[nodiscard]] const PrimitiveSpec &primitive(PrimitiveId id);

} // namespace pluss::subsp::math::detail
