#pragma once

#include "CodeM/core/SeriesSlim.h"
#include "CodeM/native/cpu/KernelTable.h"

namespace pluss::codem::native::mps {

bool is_available() noexcept;

bool supports_binary(cpu::BinaryOpId op, ScalarType dt) noexcept;
void binary(cpu::BinaryOpId op, const Series& a, const Series& b, Series& out);
bool supports_maximum_minimum(ScalarType dt) noexcept;
void maximum(const Series& a, const Series& b, Series& out);
void minimum(const Series& a, const Series& b, Series& out);
bool supports_where(ScalarType cond_dt, ScalarType data_dt) noexcept;
void where(const Series& cond, const Series& a, const Series& b, Series& out);

bool supports_unary(cpu::UnaryOpId op, ScalarType dt) noexcept;
void unary(cpu::UnaryOpId op, const Series& a, Series& out);

enum class UnaryMathOp : std::uint8_t {
  Exp,
  Log,
  Sqrt,
  Sin,
  Cos,
  Tanh,
  Tan,
  Asin,
  Acos,
  Atan,
  Sinh,
  Cosh,
  Reciprocal,
  Floor,
  Ceil,
  Round,
  Trunc,
  Relu,
  Sigmoid,
};

bool supports_unary_math(UnaryMathOp op, ScalarType dt) noexcept;
void unary_math(UnaryMathOp op, const Series& a, Series& out);

enum class UnaryPredOp : std::uint8_t {
  IsNan,
  IsInf,
  IsFinite,
};

bool supports_unary_pred(UnaryPredOp op, ScalarType dt) noexcept;
void unary_pred(UnaryPredOp op, const Series& a, Series& out);

} // namespace pluss::codem::native::mps
