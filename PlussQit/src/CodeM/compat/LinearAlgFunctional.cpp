#include "pluss/subsp/linearalg/linearalg.h"

#include "CodeM/core/Errors.h"
#include "CodeM/native/cpu/BlasGemm.h"
#include "pluss/codem/series_factory.h"
#include "pluss/subsp/math/linalg.h"

#include <atomic>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace pluss::subsp::linearalg {
namespace {

enum class MpsBridgeOp : std::uint8_t {
  Det = 0,
  Inverse,
  Cholesky,
  Logdet,
  Slogdet,
  Pinverse,
  TriangularSolve,
  CholeskySolve,
  CholeskyInverse,
  LuSolve,
  Qr,
  Svd,
};

struct DispatchCounterState {
  std::atomic<std::uint64_t> total{0};
  std::atomic<std::uint64_t> det{0};
  std::atomic<std::uint64_t> inverse{0};
  std::atomic<std::uint64_t> cholesky{0};
  std::atomic<std::uint64_t> logdet{0};
  std::atomic<std::uint64_t> slogdet{0};
  std::atomic<std::uint64_t> pinverse{0};
  std::atomic<std::uint64_t> triangular_solve{0};
  std::atomic<std::uint64_t> cholesky_solve{0};
  std::atomic<std::uint64_t> cholesky_inverse{0};
  std::atomic<std::uint64_t> lu_solve{0};
  std::atomic<std::uint64_t> qr{0};
  std::atomic<std::uint64_t> svd{0};
};

DispatchCounterState& dispatch_counters_() {
  static DispatchCounterState state{};
  return state;
}

bool parse_bool_env_(const char* name, bool default_value) noexcept {
  const char* raw = std::getenv(name);
  if (raw == nullptr) return default_value;
  const auto eq_lit = [raw](const char* lit) noexcept {
    const unsigned char* a = reinterpret_cast<const unsigned char*>(raw);
    const unsigned char* b = reinterpret_cast<const unsigned char*>(lit);
    while (*a != '\0' && *b != '\0') {
      unsigned char ca = *a;
      unsigned char cb = *b;
      if (ca >= 'A' && ca <= 'Z') ca = static_cast<unsigned char>(ca - 'A' + 'a');
      if (cb >= 'A' && cb <= 'Z') cb = static_cast<unsigned char>(cb - 'A' + 'a');
      if (ca != cb) return false;
      ++a;
      ++b;
    }
    return *a == '\0' && *b == '\0';
  };
  if (eq_lit("1") || eq_lit("true") || eq_lit("yes") || eq_lit("on")) return true;
  if (eq_lit("0") || eq_lit("false") || eq_lit("no") || eq_lit("off")) return false;
  return default_value;
}

void bump_mps_bridge_counter_(MpsBridgeOp op) noexcept {
  auto& c = dispatch_counters_();
  c.total.fetch_add(1, std::memory_order_relaxed);
  switch (op) {
    case MpsBridgeOp::Det: c.det.fetch_add(1, std::memory_order_relaxed); break;
    case MpsBridgeOp::Inverse: c.inverse.fetch_add(1, std::memory_order_relaxed); break;
    case MpsBridgeOp::Cholesky: c.cholesky.fetch_add(1, std::memory_order_relaxed); break;
    case MpsBridgeOp::Logdet: c.logdet.fetch_add(1, std::memory_order_relaxed); break;
    case MpsBridgeOp::Slogdet: c.slogdet.fetch_add(1, std::memory_order_relaxed); break;
    case MpsBridgeOp::Pinverse: c.pinverse.fetch_add(1, std::memory_order_relaxed); break;
    case MpsBridgeOp::TriangularSolve: c.triangular_solve.fetch_add(1, std::memory_order_relaxed); break;
    case MpsBridgeOp::CholeskySolve: c.cholesky_solve.fetch_add(1, std::memory_order_relaxed); break;
    case MpsBridgeOp::CholeskyInverse: c.cholesky_inverse.fetch_add(1, std::memory_order_relaxed); break;
    case MpsBridgeOp::LuSolve: c.lu_solve.fetch_add(1, std::memory_order_relaxed); break;
    case MpsBridgeOp::Qr: c.qr.fetch_add(1, std::memory_order_relaxed); break;
    case MpsBridgeOp::Svd: c.svd.fetch_add(1, std::memory_order_relaxed); break;
    default: break;
  }
}

LinearAlgDenseProvider dense_provider_from_cpu_blas_(pluss::codem::native::cpu::blas::Provider provider) noexcept {
  using BlasProvider = pluss::codem::native::cpu::blas::Provider;
  switch (provider) {
    case BlasProvider::Accelerate: return LinearAlgDenseProvider::Accelerate;
    case BlasProvider::OpenBLAS: return LinearAlgDenseProvider::OpenBLAS;
    case BlasProvider::OneMKL: return LinearAlgDenseProvider::OneMKL;
    case BlasProvider::CBLAS: return LinearAlgDenseProvider::CBLAS;
    case BlasProvider::None:
    default: return LinearAlgDenseProvider::None;
  }
}

constexpr bool cpu_solver_has_lapack_() noexcept {
#if defined(__APPLE__)
  return true;
#else
  return false;
#endif
}

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

inline bool mps_native_triangular_solve_supported_(const Series& a, const Series& b) {
  if (a.platform() != Platform::MPS || b.platform() != Platform::MPS) {
    return false;
  }
  if (a.dim() != 2 || a.sizes()[0] != a.sizes()[1]) {
    return false;
  }
  if (b.dim() == 1) {
    return b.sizes()[0] == a.sizes()[0];
  }
  if (b.dim() == 2) {
    return b.sizes()[0] == a.sizes()[0];
  }
  return false;
}

inline bool mps_native_cholesky_supported_(const Series& a) {
  return a.platform() == Platform::MPS && a.dim() == 2 && a.sizes()[0] == a.sizes()[1];
}

inline Series cholesky_mps_native_(const Series& a, bool upper) {
  PLUSS_CHECK(mps_native_cholesky_supported_(a),
              "subsp::linearalg::cholesky: native MPS path expects square 2D input");
  const auto n = a.sizes()[0];
  Series aa = a.to(ScalarType::Float32).contiguous();
  Series l = Series::zeros({n, n}, ScalarType::Float32).to(Platform::MPS).contiguous();

  const auto idx_range = [](std::int64_t start, std::int64_t end) {
    return Series::arange(
               static_cast<double>(start),
               static_cast<double>(end),
               1.0,
               ScalarType::Int64)
        .to(Platform::MPS)
        .contiguous();
  };

  for (std::int64_t i = 0; i < n; ++i) {
    Series row_i = l.select(0, i).contiguous();
    const Series a_row = aa.select(0, i).contiguous();
    for (std::int64_t j = 0; j <= i; ++j) {
      Series s = a_row.select(0, j);
      if (j > 0) {
        const Series idx = idx_range(0, j);
        const Series li = row_i.take(idx).reshape({1, j}).contiguous();
        const Series row_j = l.select(0, j).contiguous();
        const Series lj = row_j.take(idx).reshape({j, 1}).contiguous();
        const Series dot = li.matmul(lj).reshape({});
        s = s.sub(dot);
      }

      Series value = s;
      if (i == j) {
        const float sv = value.to(Platform::CPU).item<float>();
        PLUSS_CHECK(sv > 0.0f, "subsp::linearalg::cholesky: matrix is not positive definite");
        value = value.sqrt();
      } else {
        const Series ljj = l.select(0, j).select(0, j);
        value = value.div(ljj);
      }
      row_i = row_i.select_scatter(value, 0, j).contiguous();
    }
    l = l.select_scatter(row_i, 0, i).contiguous();
  }
  if (upper) {
    return l.transpose(0, 1).contiguous();
  }
  return l;
}

inline bool mps_native_inverse_supported_(const Series& a) {
  return a.platform() == Platform::MPS && a.dim() == 2 && a.sizes()[0] == a.sizes()[1];
}

inline bool mps_native_det_supported_(const Series& a) {
  return a.platform() == Platform::MPS && a.dim() == 2 && a.sizes()[0] == a.sizes()[1];
}

inline void swap_rows_mps_(Series& x, std::int64_t r0, std::int64_t r1) {
  if (r0 == r1) return;
  const Series row0 = x.select(0, r0).contiguous();
  const Series row1 = x.select(0, r1).contiguous();
  x = x.select_scatter(row1, 0, r0).contiguous();
  x = x.select_scatter(row0, 0, r1).contiguous();
}

struct DetStats {
  float sign = 1.0f;
  float logabs = 0.0f;
  bool singular = false;
};

inline DetStats det_stats_mps_native_(const Series& a) {
  PLUSS_CHECK(
      mps_native_det_supported_(a),
      "subsp::linearalg::det: native MPS path expects square 2D input");
  Series u = a.to(ScalarType::Float32).contiguous();
  const auto n = u.sizes()[0];
  if (n == 0) {
    return DetStats{};
  }

  const auto idx_range = [](std::int64_t start, std::int64_t end) {
    return Series::arange(
               static_cast<double>(start),
               static_cast<double>(end),
               1.0,
               ScalarType::Int64)
        .to(Platform::MPS)
        .contiguous();
  };

  const Series zero = Series::zeros({}, ScalarType::Float32).to(Platform::MPS);
  const float eps = 1e-7f;
  DetStats stats{};

  for (std::int64_t k = 0; k < n; ++k) {
    const Series col_k = u.select(1, k).abs().contiguous();
    const Series tail = col_k.take(idx_range(k, n)).contiguous();
    const std::int64_t piv_rel = tail.argmax().item<std::int64_t>();
    const std::int64_t piv = k + piv_rel;
    const float piv_abs = tail.select(0, piv_rel).item<float>();
    if (!std::isfinite(piv_abs) || piv_abs <= eps) {
      stats.sign = 0.0f;
      stats.logabs = -std::numeric_limits<float>::infinity();
      stats.singular = true;
      return stats;
    }

    if (piv != k) {
      swap_rows_mps_(u, k, piv);
      stats.sign = -stats.sign;
    }

    const Series pivot_row = u.select(0, k).contiguous();
    const float pivot = pivot_row.select(0, k).item<float>();
    if (!std::isfinite(pivot) || std::fabs(pivot) <= eps) {
      stats.sign = 0.0f;
      stats.logabs = -std::numeric_limits<float>::infinity();
      stats.singular = true;
      return stats;
    }

    stats.logabs += std::log(std::fabs(pivot));
    if (pivot < 0.0f) {
      stats.sign = -stats.sign;
    }

    for (std::int64_t i = k + 1; i < n; ++i) {
      Series row_i = u.select(0, i).contiguous();
      const Series factor = row_i.select(0, k).div(pivot_row.select(0, k));
      row_i = row_i.sub(pivot_row.mul(factor)).contiguous();
      row_i = row_i.select_scatter(zero, 0, k).contiguous();
      u = u.select_scatter(row_i, 0, i).contiguous();
    }
  }
  return stats;
}

inline Series inverse_mps_native_(const Series& a) {
  PLUSS_CHECK(mps_native_inverse_supported_(a),
              "subsp::linearalg::inverse: native MPS path expects square 2D input");

  Series left = a.to(ScalarType::Float32).contiguous();
  const auto n = left.sizes()[0];
  Series right = Series::eye(n, n, ScalarType::Float32).to(Platform::MPS).contiguous();
  const float eps = 1e-7f;

  const auto idx_range = [](std::int64_t start, std::int64_t end) {
    return Series::arange(
               static_cast<double>(start),
               static_cast<double>(end),
               1.0,
               ScalarType::Int64)
        .to(Platform::MPS)
        .contiguous();
  };

  for (std::int64_t k = 0; k < n; ++k) {
    const Series col_k = left.select(1, k).abs().contiguous();
    const Series tail = col_k.take(idx_range(k, n)).contiguous();
    const std::int64_t piv_rel = tail.argmax().item<std::int64_t>();
    const std::int64_t piv = k + piv_rel;
    const float piv_abs = tail.select(0, piv_rel).item<float>();
    PLUSS_CHECK(std::isfinite(piv_abs) && piv_abs > eps,
                "subsp::linearalg::inverse: singular or ill-conditioned matrix");

    swap_rows_mps_(left, k, piv);
    swap_rows_mps_(right, k, piv);

    Series pivot_row_l = left.select(0, k).contiguous();
    Series pivot_row_r = right.select(0, k).contiguous();
    const Series diag = pivot_row_l.select(0, k);
    pivot_row_l = pivot_row_l.div(diag).contiguous();
    pivot_row_r = pivot_row_r.div(diag).contiguous();
    left = left.select_scatter(pivot_row_l, 0, k).contiguous();
    right = right.select_scatter(pivot_row_r, 0, k).contiguous();

    for (std::int64_t i = 0; i < n; ++i) {
      if (i == k) continue;
      const Series row_l = left.select(0, i).contiguous();
      const Series row_r = right.select(0, i).contiguous();
      const Series factor = row_l.select(0, k);
      const Series upd_l = row_l.sub(pivot_row_l.mul(factor)).contiguous();
      const Series upd_r = row_r.sub(pivot_row_r.mul(factor)).contiguous();
      left = left.select_scatter(upd_l, 0, i).contiguous();
      right = right.select_scatter(upd_r, 0, i).contiguous();
    }
  }
  return right;
}

inline bool mps_native_lu_solve_supported_(const Series& lu, const Series& b) {
  if (lu.platform() != Platform::MPS || b.platform() != Platform::MPS) return false;
  if (lu.dim() != 2) return false;
  const auto n = lu.sizes()[0];
  if (n != lu.sizes()[1]) return false;
  if (b.dim() == 1) {
    return b.sizes()[0] == n;
  }
  if (b.dim() == 2) {
    return b.sizes()[0] == n;
  }
  return false;
}

inline Series lu_solve_mps_native_(const Series& lu, const Series& b) {
  PLUSS_CHECK(
      mps_native_lu_solve_supported_(lu, b),
      "subsp::linearalg::lu_solve: native MPS path expects square 2D matrix and 1D/2D rhs");
  Series rhs = b.to(ScalarType::Float32).contiguous();
  if (rhs.dim() == 1) {
    rhs = rhs.unsqueeze(1).contiguous();
  }
  const Series inv = inverse_mps_native_(lu);
  return inv.matmul(rhs).contiguous();
}

inline bool mps_native_pinverse_supported_(const Series& a) {
  return a.platform() == Platform::MPS && a.dim() == 2 && a.sizes()[0] > 0 && a.sizes()[1] > 0;
}

inline Series pinverse_mps_native_(const Series& a, double rcond) {
  PLUSS_CHECK(
      mps_native_pinverse_supported_(a),
      "subsp::linearalg::pinverse: native MPS path expects non-empty 2D input");
  const auto m = a.sizes()[0];
  const auto n = a.sizes()[1];
  const double reg = (rcond > 0.0) ? rcond : 0.0;

  Series aa = a.to(ScalarType::Float32).contiguous();
  const Series at = aa.transpose(0, 1).contiguous();
  if (m >= n) {
    const Series eye = Series::eye(n, n, ScalarType::Float32).to(Platform::MPS).contiguous();
    const Series gram = at.matmul(aa).add(eye.mul(reg)).contiguous();
    const Series inv = inverse_mps_native_(gram);
    return inv.matmul(at).contiguous();
  }

  const Series eye = Series::eye(m, m, ScalarType::Float32).to(Platform::MPS).contiguous();
  const Series gram = aa.matmul(at).add(eye.mul(reg)).contiguous();
  const Series inv = inverse_mps_native_(gram);
  return at.matmul(inv).contiguous();
}

struct NativeQrResult {
  Series q{Series::undefined()};
  Series r{Series::undefined()};
};

struct NativeSvdResult {
  Series u{Series::undefined()};
  Series s{Series::undefined()};
  Series vh{Series::undefined()};
};

inline bool mps_native_qr_supported_(const Series& a, bool reduced) {
  return a.platform() == Platform::MPS &&
      a.dim() == 2 &&
      reduced &&
      a.sizes()[0] > 0 &&
      a.sizes()[1] > 0;
}

inline NativeQrResult qr_mps_native_(const Series& a) {
  PLUSS_CHECK(
      mps_native_qr_supported_(a, true),
      "subsp::linearalg::qr: native MPS path expects non-empty 2D input with reduced=true");
  Series work = a.to(ScalarType::Float32).contiguous();
  const auto m = work.sizes()[0];
  const auto n = work.sizes()[1];
  const auto k = (m < n) ? m : n;

  Series q = Series::zeros({m, k}, ScalarType::Float32).to(Platform::MPS).contiguous();
  Series r = Series::zeros({k, n}, ScalarType::Float32).to(Platform::MPS).contiguous();
  const float eps = 1e-7f;

  for (std::int64_t i = 0; i < k; ++i) {
    const Series v_i = work.select(1, i).contiguous();
    const Series r_ii_s = v_i.mul(v_i).sum().sqrt();
    const float r_ii = r_ii_s.item<float>();

    Series q_i = Series::zeros({m}, ScalarType::Float32).to(Platform::MPS).contiguous();
    if (std::isfinite(r_ii) && r_ii > eps) {
      q_i = v_i.div(r_ii_s).contiguous();
    }
    q = q.select_scatter(q_i, 1, i).contiguous();

    Series r_row = r.select(0, i).contiguous();
    r_row = r_row.select_scatter(r_ii_s, 0, i).contiguous();

    for (std::int64_t j = i + 1; j < n; ++j) {
      const Series v_j = work.select(1, j).contiguous();
      const Series r_ij_s = q_i.mul(v_j).sum();
      r_row = r_row.select_scatter(r_ij_s, 0, j).contiguous();
      const Series v_j_new = v_j.sub(q_i.mul(r_ij_s)).contiguous();
      work = work.select_scatter(v_j_new, 1, j).contiguous();
    }
    r = r.select_scatter(r_row, 0, i).contiguous();
  }

  return NativeQrResult{q, r};
}

inline bool mps_native_svd_supported_(const Series& a, bool full_matrices) {
  return a.platform() == Platform::MPS &&
      a.dim() == 2 &&
      !full_matrices &&
      a.sizes()[0] > 0 &&
      a.sizes()[1] > 0;
}

inline NativeSvdResult svd_mps_native_(const Series& a) {
  PLUSS_CHECK(
      mps_native_svd_supported_(a, false),
      "subsp::linearalg::svd: native MPS path expects non-empty 2D input with full_matrices=false");
  Series aa = a.to(ScalarType::Float32).contiguous();
  const auto m = aa.sizes()[0];
  const auto n = aa.sizes()[1];
  const auto k = (m < n) ? m : n;
  const float eps = 1e-6f;
  const std::int64_t power_iters = 48;

  auto arange_vec = [](std::int64_t len) {
    return Series::arange(0.0, static_cast<double>(len), 1.0, ScalarType::Int64)
        .to(Platform::MPS)
        .to(ScalarType::Float32)
        .contiguous();
  };
  auto scalar_mps = [](float v) {
    return Series::full({}, static_cast<double>(v), ScalarType::Float32).to(Platform::MPS).contiguous();
  };

  auto eig_topk_sym = [&](const Series& sym, std::int64_t kk) {
    const auto dim = sym.sizes()[0];
    Series work = sym.to(ScalarType::Float32).contiguous();
    Series vecs = Series::zeros({dim, kk}, ScalarType::Float32).to(Platform::MPS).contiguous();
    Series vals = Series::zeros({kk}, ScalarType::Float32).to(Platform::MPS).contiguous();
    const Series basis = arange_vec(dim);

    for (std::int64_t i = 0; i < kk; ++i) {
      Series x = basis.add(static_cast<double>(i + 1)).contiguous();
      for (std::int64_t j = 0; j < i; ++j) {
        const Series vj = vecs.select(1, j).contiguous();
        const Series coeff = vj.mul(x).sum();
        x = x.sub(vj.mul(coeff)).contiguous();
      }
      Series x_norm_s = x.mul(x).sum().sqrt();
      float x_norm = x_norm_s.item<float>();
      if (!std::isfinite(x_norm) || x_norm <= eps) {
        x = Series::zeros({dim}, ScalarType::Float32).to(Platform::MPS).contiguous();
      } else {
        x = x.div(x_norm_s).contiguous();
      }

      for (std::int64_t t = 0; t < power_iters; ++t) {
        Series y = work.matmul(x.unsqueeze(1)).reshape({dim}).contiguous();
        for (std::int64_t j = 0; j < i; ++j) {
          const Series vj = vecs.select(1, j).contiguous();
          const Series coeff = vj.mul(y).sum();
          y = y.sub(vj.mul(coeff)).contiguous();
        }
        const Series y_norm_s = y.mul(y).sum().sqrt();
        const float y_norm = y_norm_s.item<float>();
        if (!std::isfinite(y_norm) || y_norm <= eps) {
          x = Series::zeros({dim}, ScalarType::Float32).to(Platform::MPS).contiguous();
          break;
        }
        x = y.div(y_norm_s).contiguous();
      }

      const Series wx = work.matmul(x.unsqueeze(1)).reshape({dim}).contiguous();
      float lambda = x.mul(wx).sum().item<float>();
      if (!std::isfinite(lambda) || lambda < 0.0f) lambda = 0.0f;

      vecs = vecs.select_scatter(x, 1, i).contiguous();
      vals = vals.select_scatter(scalar_mps(lambda), 0, i).contiguous();
      if (lambda > eps) {
        const Series outer = x.unsqueeze(1).matmul(x.unsqueeze(0)).mul(lambda);
        work = work.sub(outer).contiguous();
      }
    }

    return std::pair<Series, Series>{vecs, vals};
  };

  if (m >= n) {
    const Series at = aa.transpose(0, 1).contiguous();
    const Series gram = at.matmul(aa).contiguous();
    auto eig = eig_topk_sym(gram, k);
    Series v = eig.first;
    const Series vals = eig.second;
    Series s = Series::zeros({k}, ScalarType::Float32).to(Platform::MPS).contiguous();
    Series u = Series::zeros({m, k}, ScalarType::Float32).to(Platform::MPS).contiguous();

    for (std::int64_t i = 0; i < k; ++i) {
      const float si = std::sqrt(std::max(0.0f, vals.select(0, i).item<float>()));
      s = s.select_scatter(scalar_mps(si), 0, i).contiguous();
      const Series vi = v.select(1, i).contiguous();
      Series ui = Series::zeros({m}, ScalarType::Float32).to(Platform::MPS).contiguous();
      if (std::isfinite(si) && si > eps) {
        ui = aa.matmul(vi.unsqueeze(1)).reshape({m}).div(si).contiguous();
      }
      u = u.select_scatter(ui, 1, i).contiguous();
    }
    return NativeSvdResult{u, s, v.transpose(0, 1).contiguous()};
  }

  const Series at = aa.transpose(0, 1).contiguous();
  const Series gram = aa.matmul(at).contiguous();
  auto eig = eig_topk_sym(gram, k);
  Series u = eig.first;
  const Series vals = eig.second;
  Series s = Series::zeros({k}, ScalarType::Float32).to(Platform::MPS).contiguous();
  Series v = Series::zeros({n, k}, ScalarType::Float32).to(Platform::MPS).contiguous();

  for (std::int64_t i = 0; i < k; ++i) {
    const float si = std::sqrt(std::max(0.0f, vals.select(0, i).item<float>()));
    s = s.select_scatter(scalar_mps(si), 0, i).contiguous();
    const Series ui = u.select(1, i).contiguous();
    Series vi = Series::zeros({n}, ScalarType::Float32).to(Platform::MPS).contiguous();
    if (std::isfinite(si) && si > eps) {
      vi = at.matmul(ui.unsqueeze(1)).reshape({n}).div(si).contiguous();
    }
    v = v.select_scatter(vi, 1, i).contiguous();
  }
  return NativeSvdResult{u, s, v.transpose(0, 1).contiguous()};
}

inline Series triangular_solve_mps_native_(
    const Series& a,
    const Series& b,
    bool upper,
    bool unitriangular) {
  PLUSS_CHECK(mps_native_triangular_solve_supported_(a, b),
              "subsp::linearalg::triangular_solve: native MPS path expects square 2D factor and 1D/2D rhs");
  Series aa = a.to(ScalarType::Float32).contiguous();
  Series bb = b.to(ScalarType::Float32).contiguous();
  bool squeezed = false;
  if (bb.dim() == 1) {
    bb = bb.unsqueeze(1).contiguous();
    squeezed = true;
  }

  const auto n = aa.sizes()[0];
  const auto nrhs = bb.sizes()[1];
  Series solved_stack;

  const auto idx_range = [](std::int64_t start, std::int64_t end) {
    return Series::arange(
               static_cast<double>(start),
               static_cast<double>(end),
               1.0,
               ScalarType::Int64)
        .to(Platform::MPS)
        .contiguous();
  };

  for (std::int64_t ii = 0; ii < n; ++ii) {
    const std::int64_t i = upper ? (n - 1 - ii) : ii;
    Series rhs_i = bb.select(0, i).contiguous();
    const Series a_row = aa.select(0, i).contiguous();

    if (!upper) {
      if (i > 0) {
        const Series coeff = a_row.take(idx_range(0, i)).reshape({1, i}).contiguous();
        const Series prev = solved_stack.contiguous();
        const Series corr = coeff.matmul(prev).reshape({nrhs});
        rhs_i = rhs_i.sub(corr);
      }
    } else {
      const std::int64_t tail = n - i - 1;
      if (tail > 0) {
        const Series coeff = a_row.take(idx_range(i + 1, n)).reshape({1, tail}).contiguous();
        const Series prev = solved_stack.contiguous();
        const Series corr = coeff.matmul(prev).reshape({nrhs});
        rhs_i = rhs_i.sub(corr);
      }
    }

    Series x_i = rhs_i;
    if (!unitriangular) {
      const Series diag = a_row.select(0, i);
      x_i = x_i.div(diag);
    }
    if (!solved_stack.defined()) {
      solved_stack = x_i.unsqueeze(0).contiguous();
    } else if (!upper) {
      solved_stack = solved_stack.unite({x_i.unsqueeze(0)}, 0).contiguous();
    } else {
      solved_stack = x_i.unsqueeze(0).unite({solved_stack}, 0).contiguous();
    }
  }
  if (squeezed) {
    return solved_stack.select(1, 0);
  }
  return solved_stack;
}

inline void require_mps_linalg_bridge_policy_(const char* op) {
  if (linearalg_mps_cpu_bridge_enabled()) {
    return;
  }
  pluss::codem::throw_error(
      std::string("subsp::linearalg::") + op +
      ": MPS CPU bridge disabled by PLUSS_MPS_LINALG_ALLOW_CPU_BRIDGE policy");
}

inline Series bridge_cpu_result_to_mps_(const Series& cpu_result, MpsBridgeOp op, const char* op_name) {
  require_mps_linalg_bridge_policy_(op_name);
  bump_mps_bridge_counter_(op);
  Series out = cpu_result;
  if (out.scalar_type() != ScalarType::Float32) {
    out = out.to(ScalarType::Float32);
  }
  return out.to(Platform::MPS);
}

} // namespace

const char* linearalg_dense_provider_name(LinearAlgDenseProvider provider) noexcept {
  switch (provider) {
    case LinearAlgDenseProvider::Accelerate: return "Accelerate";
    case LinearAlgDenseProvider::OpenBLAS: return "OpenBLAS";
    case LinearAlgDenseProvider::OneMKL: return "oneMKL";
    case LinearAlgDenseProvider::CBLAS: return "CBLAS";
    case LinearAlgDenseProvider::Metal: return "Metal";
    case LinearAlgDenseProvider::None:
    default: return "None";
  }
}

const char* linearalg_solver_provider_name(LinearAlgSolverProvider provider) noexcept {
  switch (provider) {
    case LinearAlgSolverProvider::Accelerate: return "Accelerate";
    case LinearAlgSolverProvider::Reference: return "Reference";
    case LinearAlgSolverProvider::Metal: return "Metal";
    case LinearAlgSolverProvider::None:
    default: return "None";
  }
}

bool linearalg_mps_cpu_bridge_enabled() noexcept {
  static const bool enabled = parse_bool_env_("PLUSS_MPS_LINALG_ALLOW_CPU_BRIDGE", true);
  return enabled;
}

LinearAlgDispatchCounters linearalg_dispatch_counters() noexcept {
  const auto& c = dispatch_counters_();
  LinearAlgDispatchCounters out{};
  out.mps_bridge_total = c.total.load(std::memory_order_relaxed);
  out.mps_bridge_det = c.det.load(std::memory_order_relaxed);
  out.mps_bridge_inverse = c.inverse.load(std::memory_order_relaxed);
  out.mps_bridge_cholesky = c.cholesky.load(std::memory_order_relaxed);
  out.mps_bridge_logdet = c.logdet.load(std::memory_order_relaxed);
  out.mps_bridge_slogdet = c.slogdet.load(std::memory_order_relaxed);
  out.mps_bridge_pinverse = c.pinverse.load(std::memory_order_relaxed);
  out.mps_bridge_triangular_solve = c.triangular_solve.load(std::memory_order_relaxed);
  out.mps_bridge_cholesky_solve = c.cholesky_solve.load(std::memory_order_relaxed);
  out.mps_bridge_cholesky_inverse = c.cholesky_inverse.load(std::memory_order_relaxed);
  out.mps_bridge_lu_solve = c.lu_solve.load(std::memory_order_relaxed);
  out.mps_bridge_qr = c.qr.load(std::memory_order_relaxed);
  out.mps_bridge_svd = c.svd.load(std::memory_order_relaxed);
  return out;
}

void linearalg_reset_dispatch_counters() noexcept {
  auto& c = dispatch_counters_();
  c.total.store(0, std::memory_order_relaxed);
  c.det.store(0, std::memory_order_relaxed);
  c.inverse.store(0, std::memory_order_relaxed);
  c.cholesky.store(0, std::memory_order_relaxed);
  c.logdet.store(0, std::memory_order_relaxed);
  c.slogdet.store(0, std::memory_order_relaxed);
  c.pinverse.store(0, std::memory_order_relaxed);
  c.triangular_solve.store(0, std::memory_order_relaxed);
  c.cholesky_solve.store(0, std::memory_order_relaxed);
  c.cholesky_inverse.store(0, std::memory_order_relaxed);
  c.lu_solve.store(0, std::memory_order_relaxed);
  c.qr.store(0, std::memory_order_relaxed);
  c.svd.store(0, std::memory_order_relaxed);
}

LinearAlgBackendInfo linearalg_backend_info(LinearAlgTarget target) noexcept {
  LinearAlgBackendInfo info{};
  info.target = target;

  switch (target) {
    case LinearAlgTarget::CPU: {
      info.dense_provider =
          dense_provider_from_cpu_blas_(pluss::codem::native::cpu::blas::provider());
      info.solver_provider =
          cpu_solver_has_lapack_() ? LinearAlgSolverProvider::Accelerate : LinearAlgSolverProvider::Reference;
      info.dense_available = (info.dense_provider != LinearAlgDenseProvider::None);
      info.solver_available = true;
      info.supports_matmul = true;
      info.supports_det = true;
      info.supports_inverse = true;
      info.supports_cholesky = true;
      info.supports_qr = cpu_solver_has_lapack_();
      info.supports_svd = cpu_solver_has_lapack_();
      info.supports_triangular_solve = true;
      info.supports_cholesky_solve = true;
      info.supports_lu_solve = true;
      info.supports_matrix_power = true;
      info.supports_matrix_exp = true;
      return info;
    }
    case LinearAlgTarget::MPS: {
      const bool mps_ready = pluss::codem::platform_available(pluss::codem::Platform::MPS);
      info.dense_provider = mps_ready ? LinearAlgDenseProvider::Metal : LinearAlgDenseProvider::None;
      info.solver_provider = mps_ready ? LinearAlgSolverProvider::Reference : LinearAlgSolverProvider::None;
      info.dense_available = mps_ready;
      info.solver_available = mps_ready;
      info.supports_matmul = mps_ready;
      info.supports_det = mps_ready;
      info.supports_inverse = mps_ready;
      info.supports_cholesky = mps_ready;
      info.supports_qr = mps_ready;
      info.supports_svd = mps_ready;
      info.supports_triangular_solve = mps_ready;
      info.supports_cholesky_solve = mps_ready;
      info.supports_lu_solve = mps_ready;
      info.supports_matrix_power = mps_ready;
      info.supports_matrix_exp = mps_ready;
      return info;
    }
    case LinearAlgTarget::CUDA:
    default: {
      info.dense_provider = LinearAlgDenseProvider::None;
      info.solver_provider = LinearAlgSolverProvider::None;
      info.dense_available = false;
      info.solver_available = false;
      info.supports_matmul = false;
      info.supports_det = false;
      info.supports_inverse = false;
      info.supports_cholesky = false;
      info.supports_qr = false;
      info.supports_svd = false;
      info.supports_triangular_solve = false;
      info.supports_cholesky_solve = false;
      info.supports_lu_solve = false;
      info.supports_matrix_power = false;
      info.supports_matrix_exp = false;
      return info;
    }
  }
}

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
  if (a.platform() == Platform::CPU) {
    return pluss::subsp::math::linalg::det(a);
  }
  if (a.platform() == Platform::MPS) {
    if (mps_native_det_supported_(a)) {
      const auto stats = det_stats_mps_native_(a);
      const double value = stats.singular ? 0.0 : static_cast<double>(stats.sign) * std::exp(stats.logabs);
      return Series::full({}, value, ScalarType::Float32).to(Platform::MPS).contiguous();
    }
    return bridge_cpu_result_to_mps_(pluss::subsp::math::linalg::det(a.to(Platform::CPU)), MpsBridgeOp::Det, "det");
  }
  throw_platform_not_ready_("det", a.platform());
}

Series inverse(const Series& a) {
  if (a.platform() == Platform::CPU) {
    return pluss::subsp::math::linalg::inverse(a);
  }
  if (a.platform() == Platform::MPS) {
    if (mps_native_inverse_supported_(a)) {
      return inverse_mps_native_(a);
    }
    return bridge_cpu_result_to_mps_(
        pluss::subsp::math::linalg::inverse(a.to(Platform::CPU)), MpsBridgeOp::Inverse, "inverse");
  }
  throw_platform_not_ready_("inverse", a.platform());
}

Series cholesky(const Series& a, bool upper) {
  if (a.platform() == Platform::CPU) {
    return pluss::subsp::math::linalg::cholesky(a, upper);
  }
  if (a.platform() == Platform::MPS) {
    if (mps_native_cholesky_supported_(a)) {
      return cholesky_mps_native_(a, upper);
    }
    return bridge_cpu_result_to_mps_(
        pluss::subsp::math::linalg::cholesky(a.to(Platform::CPU), upper), MpsBridgeOp::Cholesky, "cholesky");
  }
  throw_platform_not_ready_("cholesky", a.platform());
}

Series logdet(const Series& a) {
  if (a.platform() == Platform::CPU) {
    return pluss::subsp::math::linalg::logdet(a);
  }
  if (a.platform() == Platform::MPS) {
    if (mps_native_det_supported_(a)) {
      const auto stats = det_stats_mps_native_(a);
      const double value =
          stats.singular ? -std::numeric_limits<float>::infinity() : static_cast<double>(stats.logabs);
      return Series::full({}, value, ScalarType::Float32).to(Platform::MPS).contiguous();
    }
    return bridge_cpu_result_to_mps_(
        pluss::subsp::math::linalg::logdet(a.to(Platform::CPU)), MpsBridgeOp::Logdet, "logdet");
  }
  throw_platform_not_ready_("logdet", a.platform());
}

Series slogdet(const Series& a) {
  if (a.platform() == Platform::CPU) {
    return pluss::subsp::math::linalg::slogdet(a);
  }
  if (a.platform() == Platform::MPS) {
    if (mps_native_det_supported_(a)) {
      const auto stats = det_stats_mps_native_(a);
      std::vector<float> out = {
          stats.singular ? 0.0f : stats.sign,
          stats.singular ? -std::numeric_limits<float>::infinity() : stats.logabs};
      return pluss::make_series(out, {2}).to(Platform::MPS);
    }
    return bridge_cpu_result_to_mps_(
        pluss::subsp::math::linalg::slogdet(a.to(Platform::CPU)), MpsBridgeOp::Slogdet, "slogdet");
  }
  throw_platform_not_ready_("slogdet", a.platform());
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
  if (a.platform() == Platform::CPU) {
    return pluss::subsp::math::linalg::pinverse(a, rcond);
  }
  if (a.platform() == Platform::MPS) {
    if (mps_native_pinverse_supported_(a)) {
      try {
        return pinverse_mps_native_(a, rcond);
      } catch (const std::exception&) {
      }
    }
    return bridge_cpu_result_to_mps_(
        pluss::subsp::math::linalg::pinverse(a.to(Platform::CPU), rcond), MpsBridgeOp::Pinverse, "pinverse");
  }
  throw_platform_not_ready_("pinverse", a.platform());
}

Series qr_q(const Series& a, bool reduced) {
  if (a.platform() == Platform::CPU) {
    return pluss::subsp::math::linalg::qr(a, reduced).Q;
  }
  if (a.platform() == Platform::MPS) {
    if (mps_native_qr_supported_(a, reduced)) {
      try {
        return qr_mps_native_(a).q;
      } catch (const std::exception&) {
      }
    }
    require_mps_linalg_bridge_policy_("qr_q");
    bump_mps_bridge_counter_(MpsBridgeOp::Qr);
    auto qr = pluss::subsp::math::linalg::qr(a.to(Platform::CPU), reduced);
    return qr.Q.to(ScalarType::Float32).to(Platform::MPS);
  }
  throw_platform_not_ready_("qr_q", a.platform());
}

Series qr_r(const Series& a, bool reduced) {
  if (a.platform() == Platform::CPU) {
    return pluss::subsp::math::linalg::qr(a, reduced).R;
  }
  if (a.platform() == Platform::MPS) {
    if (mps_native_qr_supported_(a, reduced)) {
      try {
        return qr_mps_native_(a).r;
      } catch (const std::exception&) {
      }
    }
    require_mps_linalg_bridge_policy_("qr_r");
    bump_mps_bridge_counter_(MpsBridgeOp::Qr);
    auto qr = pluss::subsp::math::linalg::qr(a.to(Platform::CPU), reduced);
    return qr.R.to(ScalarType::Float32).to(Platform::MPS);
  }
  throw_platform_not_ready_("qr_r", a.platform());
}

Series svd_u(const Series& a, bool full_matrices) {
  if (a.platform() == Platform::CPU) {
    return pluss::subsp::math::linalg::svd(a, full_matrices).U;
  }
  if (a.platform() == Platform::MPS) {
    if (mps_native_svd_supported_(a, full_matrices)) {
      try {
        return svd_mps_native_(a).u;
      } catch (const std::exception&) {
      }
    }
    require_mps_linalg_bridge_policy_("svd_u");
    bump_mps_bridge_counter_(MpsBridgeOp::Svd);
    auto svd = pluss::subsp::math::linalg::svd(a.to(Platform::CPU), full_matrices);
    return svd.U.to(ScalarType::Float32).to(Platform::MPS);
  }
  throw_platform_not_ready_("svd_u", a.platform());
}

Series svd_s(const Series& a, bool full_matrices) {
  if (a.platform() == Platform::CPU) {
    return pluss::subsp::math::linalg::svd(a, full_matrices).S;
  }
  if (a.platform() == Platform::MPS) {
    if (mps_native_svd_supported_(a, full_matrices)) {
      try {
        return svd_mps_native_(a).s;
      } catch (const std::exception&) {
      }
    }
    require_mps_linalg_bridge_policy_("svd_s");
    bump_mps_bridge_counter_(MpsBridgeOp::Svd);
    auto svd = pluss::subsp::math::linalg::svd(a.to(Platform::CPU), full_matrices);
    return svd.S.to(ScalarType::Float32).to(Platform::MPS);
  }
  throw_platform_not_ready_("svd_s", a.platform());
}

Series svd_vh(const Series& a, bool full_matrices) {
  if (a.platform() == Platform::CPU) {
    return pluss::subsp::math::linalg::svd(a, full_matrices).Vh;
  }
  if (a.platform() == Platform::MPS) {
    if (mps_native_svd_supported_(a, full_matrices)) {
      try {
        return svd_mps_native_(a).vh;
      } catch (const std::exception&) {
      }
    }
    require_mps_linalg_bridge_policy_("svd_vh");
    bump_mps_bridge_counter_(MpsBridgeOp::Svd);
    auto svd = pluss::subsp::math::linalg::svd(a.to(Platform::CPU), full_matrices);
    return svd.Vh.to(ScalarType::Float32).to(Platform::MPS);
  }
  throw_platform_not_ready_("svd_vh", a.platform());
}

Series triangular_solve(const Series& a, const Series& b, bool upper, bool unitriangular) {
  require_same_platform_(a, b, "triangular_solve");
  if (a.platform() == Platform::CPU) {
    return pluss::subsp::math::linalg::triangular_solve(b, a, upper, unitriangular);
  }
  if (a.platform() == Platform::MPS) {
    if (mps_native_triangular_solve_supported_(a, b)) {
      return triangular_solve_mps_native_(a, b, upper, unitriangular);
    }
    return bridge_cpu_result_to_mps_(
        pluss::subsp::math::linalg::triangular_solve(b.to(Platform::CPU), a.to(Platform::CPU), upper, unitriangular),
        MpsBridgeOp::TriangularSolve,
        "triangular_solve");
  }
  throw_platform_not_ready_("triangular_solve", a.platform());
}

Series cholesky_solve(const Series& factor, const Series& b, bool upper) {
  require_same_platform_(factor, b, "cholesky_solve");
  if (factor.platform() == Platform::CPU) {
    return pluss::subsp::math::linalg::cholesky_solve(b, factor, upper);
  }
  if (factor.platform() == Platform::MPS) {
    if (mps_native_triangular_solve_supported_(factor, b)) {
      Series l = factor.to(ScalarType::Float32).contiguous();
      if (upper) {
        l = l.transpose(0, 1).contiguous();
      }
      const Series y = triangular_solve_mps_native_(l, b, /*upper=*/false, /*unitriangular=*/false);
      return triangular_solve_mps_native_(l.transpose(0, 1).contiguous(), y, /*upper=*/true, /*unitriangular=*/false);
    }
    return bridge_cpu_result_to_mps_(
        pluss::subsp::math::linalg::cholesky_solve(b.to(Platform::CPU), factor.to(Platform::CPU), upper),
        MpsBridgeOp::CholeskySolve,
        "cholesky_solve");
  }
  throw_platform_not_ready_("cholesky_solve", factor.platform());
}

Series cholesky_inverse(const Series& factor, bool upper) {
  if (factor.platform() == Platform::CPU) {
    return pluss::subsp::math::linalg::cholesky_inverse(factor, upper);
  }
  if (factor.platform() == Platform::MPS) {
    require_square_2d_(factor, "cholesky_inverse");
    Series l = factor.to(ScalarType::Float32).contiguous();
    if (upper) {
      l = l.transpose(0, 1).contiguous();
    }
    const auto n = l.sizes()[0];
    const Series eye = Series::eye(n, n, ScalarType::Float32).to(Platform::MPS).contiguous();
    const Series y = triangular_solve_mps_native_(l, eye, /*upper=*/false, /*unitriangular=*/false);
    return triangular_solve_mps_native_(l.transpose(0, 1).contiguous(), y, /*upper=*/true, /*unitriangular=*/false);
  }
  throw_platform_not_ready_("cholesky_inverse", factor.platform());
}

Series lu_solve(const Series& lu, const Series& b) {
  require_same_platform_(lu, b, "lu_solve");
  if (lu.platform() == Platform::CPU) {
    return pluss::subsp::math::linalg::lu_solve(b, lu);
  }
  if (lu.platform() == Platform::MPS) {
    if (mps_native_lu_solve_supported_(lu, b)) {
      return lu_solve_mps_native_(lu, b);
    }
    return bridge_cpu_result_to_mps_(
        pluss::subsp::math::linalg::lu_solve(b.to(Platform::CPU), lu.to(Platform::CPU)),
        MpsBridgeOp::LuSolve,
        "lu_solve");
  }
  throw_platform_not_ready_("lu_solve", lu.platform());
}

} // namespace pluss::subsp::linearalg
