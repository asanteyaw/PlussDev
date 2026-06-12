#include "CodeM/native/cpu/KernelTable.h"
#include "CodeM/native/cpu/UnaryMoreKernels.h"
#include "CodeM/native/cpu/UnaryExtraMathKernels.h"
#include "CodeM/native/cpu/ElementwiseOps.h"
#include "CodeM/native/cpu/CompareOps.h"
#include "CodeM/native/cpu/UnaryBasicOps.h"
#include "CodeM/native/cpu/UnaryMathKernels.h"
#include "CodeM/native/cpu/PowOps.h"
#include "CodeM/native/cpu/BinaryExtraMathKernels.h"

namespace pluss::codem::native::cpu {

UnaryKernelFn lookup_unary(UnaryOpId op, ScalarType dt) {
  // for now, FloatOnly kernels (Float16/BFloat16/Float32/Float64) are dispatched inside the kernel,
  // so dtype isn't used yet. We'll evolve this to per-dtype tables as we add vectorized kernels.
  (void)dt;
  switch (op) {
    case UnaryOpId::Neg: return &neg;
    case UnaryOpId::Abs: return &abs;
    case UnaryOpId::Sqrt: return &sqrt;
    case UnaryOpId::Exp: return &exp;
    case UnaryOpId::Log: return &log;
    case UnaryOpId::Sin: return &sin;
    case UnaryOpId::Cos: return &cos;
    case UnaryOpId::Tanh: return &tanh;
    case UnaryOpId::Tan: return &tan;
    case UnaryOpId::Asin: return &asin;
    case UnaryOpId::Acos: return &acos;
    case UnaryOpId::Atan: return &atan;
    case UnaryOpId::Sinh: return &sinh;
    case UnaryOpId::Cosh: return &cosh;
    case UnaryOpId::Floor: return &floor;
    case UnaryOpId::Ceil: return &ceil;
    case UnaryOpId::Round: return &round;
    case UnaryOpId::Trunc: return &trunc;
    case UnaryOpId::Reciprocal: return &reciprocal;
    case UnaryOpId::Log1p: return &log1p;
    case UnaryOpId::Expm1: return &expm1;
    case UnaryOpId::Erf: return &erf;
    case UnaryOpId::Erfc: return &erfc;
    case UnaryOpId::LGamma: return &lgamma;
    case UnaryOpId::Asinh: return &asinh;
    case UnaryOpId::Acosh: return &acosh;
    case UnaryOpId::Atanh: return &atanh;
    case UnaryOpId::Rsqrt: return &rsqrt;
  }
  return nullptr;
}

BinaryKernelFn lookup_binary(BinaryOpId op, ScalarType dt) {
  (void)dt;
  switch (op) {
    case BinaryOpId::Add: return &add;
    case BinaryOpId::Sub: return &sub;
    case BinaryOpId::Mul: return &mul;
    case BinaryOpId::Div: return &div;
    case BinaryOpId::Eq:  return &eq;
    case BinaryOpId::Ne:  return &ne;
    case BinaryOpId::Lt:  return &lt;
    case BinaryOpId::Le:  return &le;
    case BinaryOpId::Gt:  return &gt;
    case BinaryOpId::Ge:  return &ge;
    case BinaryOpId::Pow: return &pow;
    case BinaryOpId::Maximum: return &maximum;
    case BinaryOpId::Minimum: return &minimum;
  }
  return nullptr;
}

} // namespace
