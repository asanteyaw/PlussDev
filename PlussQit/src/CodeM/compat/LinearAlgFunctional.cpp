#include "pluss/subsp/linearalg/linearalg.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"
#include "pluss/subsp/math/linalg.h"

#include <string>
#include <utility>

namespace pluss::subsp::linearalg {
namespace {

inline void require_defined_(const Series& x, const char* op) {
  PLUSS_CHECK(x.defined(), std::string("subsp::linearalg::") + op + ": expected defined Series");
}

inline void require_same_platform_(const Series& a, const Series& b, const char* op) {
  PLUSS_CHECK(
      a.platform() == b.platform(),
      std::string("subsp::linearalg::") + op + ": expected matching platforms");
}

inline Series invoke_pair_(std::string_view op, const char* opname, const Series& a, const Series& b) {
  require_defined_(a, opname);
  require_defined_(b, opname);
  require_same_platform_(a, b, opname);
  return pluss::codem::_compat::invoke(op, a, b);
}

inline void require_square_2d_(const Series& a, const char* op) {
  require_defined_(a, op);
  PLUSS_CHECK(a.dim() == 2, std::string("subsp::linearalg::") + op + ": expected 2D input");
  PLUSS_CHECK(
      a.sizes()[0] == a.sizes()[1],
      std::string("subsp::linearalg::") + op + ": expected square matrix");
}

inline Series eye_like_(const Series& a) {
  const auto n = a.sizes()[0];
  auto out = Series::eye(n, n, a.scalar_type());
  if (out.platform() != a.platform()) {
    out = out.to(a.platform());
  }
  return out;
}

[[noreturn]] inline void throw_platform_not_ready_(const char* op, Platform p) {
  const char* pstr = (p == Platform::CPU) ? "CPU" : (p == Platform::MPS) ? "MPS" : "Unknown";
  pluss::codem::throw_error(
      std::string("subsp::linearalg::") + op +
      ": no native implementation on " + pstr +
      " yet; use CPU input for now");
}

inline Series matrix_power_native_(const Series& a, std::int64_t exp) {
  require_square_2d_(a, "matrix_power");
  if (exp == 0) {
    return eye_like_(a);
  }
  PLUSS_CHECK(exp > 0, "subsp::linearalg::matrix_power: negative powers require CPU inverse path");

  std::uint64_t p = static_cast<std::uint64_t>(exp);
  Series out = eye_like_(a);
  Series base = a;
  while (p > 0) {
    if ((p & 1ULL) != 0ULL) out = out.matmul(base);
    p >>= 1ULL;
    if (p) base = base.matmul(base);
  }
  return out;
}

inline Series matrix_exp_native_(const Series& a) {
  require_square_2d_(a, "matrix_exp");
  Series out = eye_like_(a);
  Series term = out;
  for (std::int64_t k = 1; k <= 18; ++k) {
    term = term.matmul(a).div(static_cast<double>(k));
    out = out.add(term);
  }
  return out;
}

} // namespace

Series matmul(const Series& a, const Series& b) {
  return invoke_pair_("matmul", "matmul", a, b);
}

Series mm(const Series& a, const Series& b) {
  return invoke_pair_("mm", "mm", a, b);
}

Series mv(const Series& a, const Series& b) {
  return invoke_pair_("mv", "mv", a, b);
}

Series dot(const Series& a, const Series& b) {
  return invoke_pair_("dot", "dot", a, b);
}

Series vdot(const Series& a, const Series& b) {
  return invoke_pair_("vdot", "vdot", a, b);
}

Series norm(const Series& x) {
  require_defined_(x, "norm");
  return x.norm(2.0);
}

Series det(const Series& a) {
  if (a.platform() != Platform::CPU) {
    throw_platform_not_ready_("det", a.platform());
  }
  return pluss::subsp::math::linalg::det(a);
}

Series inverse(const Series& a) {
  if (a.platform() != Platform::CPU) {
    throw_platform_not_ready_("inverse", a.platform());
  }
  return pluss::subsp::math::linalg::inverse(a);
}

Series cholesky(const Series& a, bool upper) {
  if (a.platform() != Platform::CPU) {
    throw_platform_not_ready_("cholesky", a.platform());
  }
  return pluss::subsp::math::linalg::cholesky(a, upper);
}

Series logdet(const Series& a) {
  if (a.platform() != Platform::CPU) {
    throw_platform_not_ready_("logdet", a.platform());
  }
  return pluss::subsp::math::linalg::logdet(a);
}

Series slogdet(const Series& a) {
  if (a.platform() != Platform::CPU) {
    throw_platform_not_ready_("slogdet", a.platform());
  }
  return pluss::subsp::math::linalg::slogdet(a);
}

Series matrix_power(const Series& a, std::int64_t n) {
  if (a.platform() == Platform::CPU) {
    return pluss::subsp::math::linalg::matrix_power(a, n);
  }
  if (a.platform() == Platform::MPS) {
    return matrix_power_native_(a, n);
  }
  throw_platform_not_ready_("matrix_power", a.platform());
}

Series matrix_exp(const Series& a) {
  if (a.platform() == Platform::CPU) {
    return pluss::subsp::math::linalg::matrix_exp(a);
  }
  if (a.platform() == Platform::MPS) {
    return matrix_exp_native_(a);
  }
  throw_platform_not_ready_("matrix_exp", a.platform());
}

Series pinverse(const Series& a, double rcond) {
  if (a.platform() != Platform::CPU) {
    throw_platform_not_ready_("pinverse", a.platform());
  }
  return pluss::subsp::math::linalg::pinverse(a, rcond);
}

Series triangular_solve(const Series& a, const Series& b, bool upper, bool unitriangular) {
  require_same_platform_(a, b, "triangular_solve");
  if (a.platform() != Platform::CPU) {
    throw_platform_not_ready_("triangular_solve", a.platform());
  }
  return pluss::subsp::math::linalg::triangular_solve(b, a, upper, unitriangular);
}

Series cholesky_solve(const Series& factor, const Series& b, bool upper) {
  require_same_platform_(factor, b, "cholesky_solve");
  if (factor.platform() != Platform::CPU) {
    throw_platform_not_ready_("cholesky_solve", factor.platform());
  }
  return pluss::subsp::math::linalg::cholesky_solve(b, factor, upper);
}

Series cholesky_inverse(const Series& factor, bool upper) {
  if (factor.platform() != Platform::CPU) {
    throw_platform_not_ready_("cholesky_inverse", factor.platform());
  }
  return pluss::subsp::math::linalg::cholesky_inverse(factor, upper);
}

Series lu_solve(const Series& lu, const Series& b) {
  require_same_platform_(lu, b, "lu_solve");
  if (lu.platform() != Platform::CPU) {
    throw_platform_not_ready_("lu_solve", lu.platform());
  }
  return pluss::subsp::math::linalg::lu_solve(b, lu);
}

} // namespace pluss::subsp::linearalg
