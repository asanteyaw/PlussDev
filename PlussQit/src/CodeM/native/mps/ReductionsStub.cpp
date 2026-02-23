#include "CodeM/native/mps/Reductions.h"

#include "CodeM/core/Errors.h"

namespace pluss::codem::native::mps {

bool supports_sum_dim(ScalarType) noexcept {
  return false;
}

void sum_dim(const Series&, std::int64_t, Series&) {
  throw_error("MPS reductions backend is unavailable on this platform/build");
}

bool supports_prod_dim(ScalarType) noexcept {
  return false;
}

void prod_dim(const Series&, std::int64_t, Series&) {
  throw_error("MPS reductions backend is unavailable on this platform/build");
}

bool supports_amax_amin_dim(ScalarType) noexcept {
  return false;
}

void amax_dim(const Series&, std::int64_t, Series&) {
  throw_error("MPS reductions backend is unavailable on this platform/build");
}

void amin_dim(const Series&, std::int64_t, Series&) {
  throw_error("MPS reductions backend is unavailable on this platform/build");
}

bool supports_all_any_dim(ScalarType) noexcept {
  return false;
}

void all_dim(const Series&, std::int64_t, Series&) {
  throw_error("MPS reductions backend is unavailable on this platform/build");
}

void any_dim(const Series&, std::int64_t, Series&) {
  throw_error("MPS reductions backend is unavailable on this platform/build");
}

bool supports_argmax_argmin_dim(ScalarType) noexcept {
  return false;
}

void argmax_dim(const Series&, std::int64_t, Series&) {
  throw_error("MPS reductions backend is unavailable on this platform/build");
}

void argmin_dim(const Series&, std::int64_t, Series&) {
  throw_error("MPS reductions backend is unavailable on this platform/build");
}

} // namespace pluss::codem::native::mps
