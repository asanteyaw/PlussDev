#pragma once
#include "CodeM/core/SeriesSlim.h"

namespace pluss::codem::native::cpu {

// Minimal kernel-table to accelerate adding ops without editing many files.
// This will expand over time (unary/binary/reduce).
enum class UnaryOpId {
  Neg,
  Abs,
  Sqrt,
  Exp,
  Log,
  Sin,
  Cos,
  Tanh,
  Tan,
  Asin,
  Acos,
  Atan,
  Sinh,
  Cosh,
  Floor,
  Ceil,
  Round,
  Trunc,
  Reciprocal,
  Log1p,
  Expm1,
  Erf,
  Erfc,
  LGamma,
  Asinh,
  Acosh,
  Atanh,
  Rsqrt,
};

using UnaryKernelFn = void(*)(const Series&, Series&);

enum class BinaryOpId {
  Add, Sub, Mul, Div,
  Eq, Ne, Lt, Le, Gt, Ge,
  Pow,
  Maximum,
  Minimum,
};

using BinaryKernelFn = void(*)(const Series&, const Series&, const Series&);

UnaryKernelFn lookup_unary(UnaryOpId op, ScalarType dt);
BinaryKernelFn lookup_binary(BinaryOpId op, ScalarType dt);

} // namespace
