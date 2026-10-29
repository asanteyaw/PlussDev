#pragma once

#include "pluss/codem/codem.h"

#include <cstdint>
#include <span>
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
  // tensor primitives (attribute-carrying; output axes are the trailing axes of the value)
  Transpose = 25,
  Contract = 26,
  Inverse = 27,
  Det = 28,
  Trace = 29,
  Select = 30,
  // path functionals: reductions along a sampled path axis of the arguments (stochastic family)
  PathIntegral = 31,
  PathCovariation = 32,
  // complex structural primitives. These IDs are appended so persisted graphs
  // produced by the original real-valued calculus retain their meaning.
  Conj = 33,
  Real = 34,
  Imag = 35,
  Angle = 36,
  SafeDivStrict = 37,
  SafeDivZero = 38,
  SafeDivPiecewise = 39,
};

using PrimitiveEvaluator =
    pluss::Series (*)(pluss::RangeView<pluss::Series> arguments);
// Tensor primitives also receive the integer attributes stored on their node.
using AttributeEvaluator =
    pluss::Series (*)(pluss::RangeView<pluss::Series> arguments,
                      std::span<const std::int64_t> attributes);

struct PrimitiveSpec final {
  PrimitiveId id;
  std::string_view name;
  std::uint8_t arity;
  PrimitiveEvaluator evaluate;
  bool variadic = false;
  AttributeEvaluator evaluate_with_attributes = nullptr;
};

[[nodiscard]] constexpr bool carries_attributes(PrimitiveId id) noexcept {
  return id >= PrimitiveId::Transpose && id <= PrimitiveId::PathCovariation;
}

[[nodiscard]] const PrimitiveSpec &primitive(PrimitiveId id);

} // namespace pluss::subsp::math::detail
