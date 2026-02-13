#include "CodeM/native/cpu/KernelTable.h"
#include "CodeM/native/cpu/UnaryMoreKernels.h"
#include "CodeM/native/cpu/ElementwiseOps.h"
#include "CodeM/native/cpu/CompareOps.h"
#include "CodeM/native/cpu/UnaryBasicOps.h"
#include "CodeM/native/cpu/PowOps.h"

namespace pluss::codem::native::cpu {

UnaryKernelFn lookup_unary(UnaryOpId op, ScalarType dt) {
  // for now, FloatOnly kernels (Float16/BFloat16/Float32/Float64) are dispatched inside the kernel,
  // so dtype isn't used yet. We'll evolve this to per-dtype tables as we add vectorized kernels.
  (void)dt;
  switch (op) {
    case UnaryOpId::Neg: return &neg;
    case UnaryOpId::Abs: return &abs;
    case UnaryOpId::Log1p: return &log1p;
    case UnaryOpId::Expm1: return &expm1;
    case UnaryOpId::Erf: return &erf;
    case UnaryOpId::Erfc: return &erfc;
    case UnaryOpId::LGamma: return &lgamma;
    case UnaryOpId::Asinh: return &asinh;
    case UnaryOpId::Acosh: return &acosh;
    case UnaryOpId::Atanh: return &atanh;
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
  }
  return nullptr;
}

} // namespace
