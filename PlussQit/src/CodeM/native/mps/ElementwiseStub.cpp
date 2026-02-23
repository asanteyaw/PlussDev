#include "CodeM/native/mps/Elementwise.h"

#include "CodeM/core/Errors.h"

namespace pluss::codem::native::mps {

bool is_available() noexcept {
  return false;
}

bool supports_binary(cpu::BinaryOpId, ScalarType) noexcept {
  return false;
}

void binary(cpu::BinaryOpId, const Series&, const Series&, Series&) {
  throw_error("MPS backend is unavailable on this platform/build");
}

bool supports_where(ScalarType, ScalarType) noexcept {
  return false;
}

void where(const Series&, const Series&, const Series&, Series&) {
  throw_error("MPS backend is unavailable on this platform/build");
}

bool supports_unary(cpu::UnaryOpId, ScalarType) noexcept {
  return false;
}

void unary(cpu::UnaryOpId, const Series&, Series&) {
  throw_error("MPS backend is unavailable on this platform/build");
}

bool supports_unary_math(UnaryMathOp, ScalarType) noexcept {
  return false;
}

void unary_math(UnaryMathOp, const Series&, Series&) {
  throw_error("MPS backend is unavailable on this platform/build");
}

bool supports_unary_pred(UnaryPredOp, ScalarType) noexcept {
  return false;
}

void unary_pred(UnaryPredOp, const Series&, Series&) {
  throw_error("MPS backend is unavailable on this platform/build");
}

} // namespace pluss::codem::native::mps
