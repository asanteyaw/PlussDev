#include "CodeM/native/mps/Linalg.h"

#include "CodeM/core/Errors.h"

namespace pluss::codem::native::mps {

bool supports_lu_solver(const Series&) noexcept {
  return false;
}

bool supports_lu_solve(const Series&, const Series&) noexcept {
  return false;
}

Series lu_solve(const Series&, const Series&) {
  throw_error("MPS linalg backend is unavailable on this platform/build");
}

Series lu_inverse(const Series&) {
  throw_error("MPS linalg backend is unavailable on this platform/build");
}

LuDetStats lu_det_stats(const Series&) {
  throw_error("MPS linalg backend is unavailable on this platform/build");
}

bool supports_cholesky_factor(const Series&) noexcept {
  return false;
}

Series cholesky_factor(const Series&, bool) {
  throw_error("MPS linalg backend is unavailable on this platform/build");
}

bool supports_cholesky_solve(const Series&, const Series&) noexcept {
  return false;
}

Series cholesky_solve(const Series&, const Series&, bool) {
  throw_error("MPS linalg backend is unavailable on this platform/build");
}

} // namespace pluss::codem::native::mps
