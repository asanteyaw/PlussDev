#pragma once

#include "CodeM/core/SeriesSlim.h"
#include "CodeM/native/cpu/KernelTable.h"

namespace pluss::codem::native::mps {

bool is_available() noexcept;

bool supports_binary(cpu::BinaryOpId op, ScalarType dt) noexcept;
void binary(cpu::BinaryOpId op, const Series& a, const Series& b, Series& out);

bool supports_unary(cpu::UnaryOpId op, ScalarType dt) noexcept;
void unary(cpu::UnaryOpId op, const Series& a, Series& out);

enum class UnaryMathOp : std::uint8_t {
  Exp,
  Log,
  Sqrt,
};

bool supports_unary_math(UnaryMathOp op, ScalarType dt) noexcept;
void unary_math(UnaryMathOp op, const Series& a, Series& out);

} // namespace pluss::codem::native::mps
