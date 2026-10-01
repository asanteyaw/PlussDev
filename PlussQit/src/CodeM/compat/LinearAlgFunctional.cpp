#include "pluss/subsp/linearalg/linearalg.h"

#include "CodeM/core/SeriesTape.h"
#include "CodeM/core/Errors.h"
#include "CodeM/core/OpsInternal.h"
#include "CodeM/native/cpu/BlasGemm.h"
#include "CodeM/native/mps/Linalg.h"
#include "CodeM/native/cuda/CUDARuntime.h"
#include "CodeM/native/cuda/LinalgOps.h"
#include "pluss/codem/series_factory.h"
#include "pluss/subsp/math/linalg.h"

#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <limits>
#include <memory>
#include <optional>
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
  std::atomic<std::uint64_t> native_qr_device_success{0};
  std::atomic<std::uint64_t> native_qr_device_fallback{0};
  std::atomic<std::uint64_t> native_svd_device_success{0};
  std::atomic<std::uint64_t> native_svd_device_fallback{0};
};

DispatchCounterState& dispatch_counters_() {
  static DispatchCounterState state{};
  return state;
}

std::atomic<std::uint64_t>& dispatch_cache_epoch_() noexcept {
  static std::atomic<std::uint64_t> epoch{1};
  return epoch;
}

std::uint64_t current_dispatch_cache_epoch_() noexcept {
  return dispatch_cache_epoch_().load(std::memory_order_acquire);
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

enum class MpsSolverRouteMode : std::uint8_t {
  Auto = 0,
  Native = 1,
  Bridge = 2,
};

MpsSolverRouteMode parse_mps_solver_route_mode_() noexcept {
  const char* raw = std::getenv("PLUSS_MPS_LINALG_SOLVER_ROUTE");
  if (raw == nullptr) {
    return MpsSolverRouteMode::Auto;
  }
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

  if (eq_lit("native") || eq_lit("metal") || eq_lit("prefer_native")) {
    return MpsSolverRouteMode::Native;
  }
  if (eq_lit("bridge") || eq_lit("reference") || eq_lit("prefer_bridge") || eq_lit("cpu")) {
    return MpsSolverRouteMode::Bridge;
  }
  return MpsSolverRouteMode::Auto;
}

bool mps_solver_route_prefers_bridge_() noexcept {
  return parse_mps_solver_route_mode_() == MpsSolverRouteMode::Bridge;
}

bool mps_native_route_enabled_() noexcept {
  if (!linearalg_mps_experimental_native_enabled()) {
    return false;
  }
  if (!mps_solver_route_prefers_bridge_()) {
    return true;
  }
  // "bridge" means prefer bridge when available, but do not disable native
  // entirely when bridge is unavailable.
  return !linearalg_mps_cpu_bridge_enabled();
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

inline void bump_mps_native_qr_device_success_() noexcept {
  dispatch_counters_().native_qr_device_success.fetch_add(1, std::memory_order_relaxed);
}

inline void bump_mps_native_qr_device_fallback_() noexcept {
  dispatch_counters_().native_qr_device_fallback.fetch_add(1, std::memory_order_relaxed);
}

inline void bump_mps_native_svd_device_success_() noexcept {
  dispatch_counters_().native_svd_device_success.fetch_add(1, std::memory_order_relaxed);
}

inline void bump_mps_native_svd_device_fallback_() noexcept {
  dispatch_counters_().native_svd_device_fallback.fetch_add(1, std::memory_order_relaxed);
}

inline Series stack_dim0_(const std::vector<Series>& parts, const char* op);

bool linalg_tracks_slope_(const Series& a) {
  return pluss::codem::series_tape::recording_enabled() && pluss::codem::series_tape::tracks_slope(a);
}

bool is_complex_scalar_(ScalarType dtype) noexcept {
  return dtype == ScalarType::Complex64 || dtype == ScalarType::Complex128;
}

Series matrix_adjoint_(const Series& x) {
  PLUSS_CHECK(x.dim() >= 2, "subsp::linearalg: expected matrix input for reverse-mode rule");
  Series xt = x.transpose(-2, -1);
  return is_complex_scalar_(x.scalar_type()) ? pluss::codem::ops_internal::conj(xt) : xt;
}

Series scale_to_matrix_batch_(Series scale, const Series& matrix) {
  while (scale.dim() < matrix.dim()) {
    scale = scale.unsqueeze(static_cast<std::int64_t>(scale.dim()));
  }
  return scale;
}

Series linalg_sensitivity_like_(Series grad, const Series& like) {
  if (grad.scalar_type() != like.scalar_type()) grad = grad.to(like.scalar_type());
  if (grad.platform() != like.platform()) grad = grad.to(like.platform());
  return grad;
}

Series mat_scalar_(const Series& a, std::int64_t r, std::int64_t c) {
  return a.select(-2, r).select(-1, c);
}

bool complex_small_square_supported_(const Series& a) {
  return is_complex_scalar_(a.scalar_type()) &&
      a.dim() == 2 &&
      a.sizes()[0] == a.sizes()[1] &&
      a.sizes()[0] > 0 &&
      a.sizes()[0] <= 2;
}

Series det_small_complex_(const Series& a) {
  PLUSS_CHECK(complex_small_square_supported_(a), "subsp::linearalg::det: expected 1x1 or 2x2 complex matrix");
  if (a.sizes()[0] == 1) {
    return mat_scalar_(a, 0, 0).contiguous();
  }
  const Series a00 = mat_scalar_(a, 0, 0);
  const Series a01 = mat_scalar_(a, 0, 1);
  const Series a10 = mat_scalar_(a, 1, 0);
  const Series a11 = mat_scalar_(a, 1, 1);
  return a00.mul(a11).sub(a01.mul(a10)).contiguous();
}

Series inverse_small_complex_(const Series& a) {
  PLUSS_CHECK(complex_small_square_supported_(a), "subsp::linearalg::inverse: expected 1x1 or 2x2 complex matrix");
  if (a.sizes()[0] == 1) {
    const Series a00 = mat_scalar_(a, 0, 0);
    const Series one = a00.mul(0.0).add(1.0);
    return one.div(a00).reshape({1, 1}).contiguous();
  }
  const Series a00 = mat_scalar_(a, 0, 0);
  const Series a01 = mat_scalar_(a, 0, 1);
  const Series a10 = mat_scalar_(a, 1, 0);
  const Series a11 = mat_scalar_(a, 1, 1);
  const Series d = a00.mul(a11).sub(a01.mul(a10));

  std::vector<Series> row0_parts;
  row0_parts.reserve(2);
  row0_parts.push_back(a11.div(d));
  row0_parts.push_back(a01.neg().div(d));
  std::vector<Series> row1_parts;
  row1_parts.reserve(2);
  row1_parts.push_back(a10.neg().div(d));
  row1_parts.push_back(a00.div(d));
  std::vector<Series> rows;
  rows.reserve(2);
  rows.push_back(stack_dim0_(row0_parts, "inverse_complex_row0").reshape({2}).contiguous());
  rows.push_back(stack_dim0_(row1_parts, "inverse_complex_row1").reshape({2}).contiguous());
  return stack_dim0_(rows, "inverse_complex").contiguous();
}

bool complex_small_triangular_supported_(const Series& a, const Series& b) {
  if (!is_complex_scalar_(a.scalar_type()) && !is_complex_scalar_(b.scalar_type())) {
    return false;
  }
  if (a.scalar_type() != b.scalar_type()) {
    return false;
  }
  if (a.dim() != 2 || a.sizes()[0] != a.sizes()[1] || a.sizes()[0] <= 0 || a.sizes()[0] > 2) {
    return false;
  }
  if (b.dim() == 1) {
    return b.sizes()[0] == a.sizes()[0];
  }
  return b.dim() == 2 && b.sizes()[0] == a.sizes()[0];
}

Series triangular_solve_small_complex_(const Series& a, const Series& b, bool upper, bool unitriangular) {
  PLUSS_CHECK(
      complex_small_triangular_supported_(a, b),
      "subsp::linearalg::triangular_solve: expected 1x1 or 2x2 complex factor/rhs");
  const std::int64_t n = a.sizes()[0];
  const bool vector_rhs = b.dim() == 1;
  const Series rhs = vector_rhs ? b.reshape({n, 1}).contiguous() : b.contiguous();
  if (n == 1) {
    Series x = rhs;
    if (!unitriangular) {
      x = x.div(mat_scalar_(a, 0, 0));
    }
    return vector_rhs ? x.reshape({1}).contiguous() : x.contiguous();
  }

  const Series a00 = mat_scalar_(a, 0, 0);
  const Series a01 = mat_scalar_(a, 0, 1);
  const Series a10 = mat_scalar_(a, 1, 0);
  const Series a11 = mat_scalar_(a, 1, 1);
  const Series b0 = rhs.select(0, 0).contiguous();
  const Series b1 = rhs.select(0, 1).contiguous();
  Series x0 = pluss::series::undefined();
  Series x1 = pluss::series::undefined();
  if (upper) {
    x1 = unitriangular ? b1 : b1.div(a11);
    x0 = b0.sub(a01.mul(x1));
    if (!unitriangular) {
      x0 = x0.div(a00);
    }
  } else {
    x0 = unitriangular ? b0 : b0.div(a00);
    x1 = b1.sub(a10.mul(x0));
    if (!unitriangular) {
      x1 = x1.div(a11);
    }
  }
  std::vector<Series> rows;
  rows.reserve(2);
  rows.push_back(x0.contiguous());
  rows.push_back(x1.contiguous());
  Series out = stack_dim0_(rows, "triangular_solve_complex").contiguous();
  return vector_rhs ? out.reshape({2}).contiguous() : out;
}

Series stack_scalars_as_row_(std::vector<Series> values, const char* op) {
  return stack_dim0_(values, op).reshape({static_cast<std::int64_t>(values.size())});
}

Series det_small_real_backward_factor_(const Series& a) {
  if (is_complex_scalar_(a.scalar_type()) || a.dim() != 2 || a.sizes()[0] != a.sizes()[1]) {
    return pluss::series::undefined();
  }
  const std::int64_t n = a.sizes()[0];
  if (n == 1) {
    return Series::ones({1, 1}, a.scalar_type()).to(a.platform());
  }
  if (n == 2) {
    const Series a00 = mat_scalar_(a, 0, 0);
    const Series a01 = mat_scalar_(a, 0, 1);
    const Series a10 = mat_scalar_(a, 1, 0);
    const Series a11 = mat_scalar_(a, 1, 1);
    return stack_dim0_(
        {stack_scalars_as_row_({a11, a10.neg()}, "det_backward_2x2_row0"),
         stack_scalars_as_row_({a01.neg(), a00}, "det_backward_2x2_row1")},
        "det_backward_2x2");
  }
  if (n == 3) {
    const Series a00 = mat_scalar_(a, 0, 0);
    const Series a01 = mat_scalar_(a, 0, 1);
    const Series a02 = mat_scalar_(a, 0, 2);
    const Series a10 = mat_scalar_(a, 1, 0);
    const Series a11 = mat_scalar_(a, 1, 1);
    const Series a12 = mat_scalar_(a, 1, 2);
    const Series a20 = mat_scalar_(a, 2, 0);
    const Series a21 = mat_scalar_(a, 2, 1);
    const Series a22 = mat_scalar_(a, 2, 2);
    return stack_dim0_(
        {stack_scalars_as_row_(
             {a11.mul(a22).sub(a12.mul(a21)),
              a12.mul(a20).sub(a10.mul(a22)),
              a10.mul(a21).sub(a11.mul(a20))},
             "det_backward_3x3_row0"),
         stack_scalars_as_row_(
             {a02.mul(a21).sub(a01.mul(a22)),
              a00.mul(a22).sub(a02.mul(a20)),
              a01.mul(a20).sub(a00.mul(a21))},
             "det_backward_3x3_row1"),
         stack_scalars_as_row_(
             {a01.mul(a12).sub(a02.mul(a11)),
              a02.mul(a10).sub(a00.mul(a12)),
              a00.mul(a11).sub(a01.mul(a10))},
             "det_backward_3x3_row2")},
        "det_backward_3x3");
  }
  return pluss::series::undefined();
}

Series triangular_masked_like_(const Series& value, bool upper, bool unitriangular) {
  PLUSS_CHECK(value.dim() >= 2, "subsp::linearalg: triangular mask expects matrix input");
  if (complex_small_square_supported_(value)) {
    const std::int64_t n = value.sizes()[0];
    const Series z = mat_scalar_(value, 0, 0).mul(0.0);
    if (n == 1) {
      const Series v = unitriangular ? z : mat_scalar_(value, 0, 0);
      return v.reshape({1, 1}).contiguous();
    }

    const Series v00 = mat_scalar_(value, 0, 0);
    const Series v01 = mat_scalar_(value, 0, 1);
    const Series v10 = mat_scalar_(value, 1, 0);
    const Series v11 = mat_scalar_(value, 1, 1);
    const auto cell = [](const Series& x) { return x.reshape({1}).contiguous(); };
    std::vector<Series> row0_parts;
    std::vector<Series> row1_parts;
    row0_parts.reserve(2);
    row1_parts.reserve(2);
    if (upper) {
      row0_parts.push_back(cell(unitriangular ? z : v00));
      row0_parts.push_back(cell(v01));
      row1_parts.push_back(cell(z));
      row1_parts.push_back(cell(unitriangular ? z : v11));
    } else {
      row0_parts.push_back(cell(unitriangular ? z : v00));
      row0_parts.push_back(cell(z));
      row1_parts.push_back(cell(v10));
      row1_parts.push_back(cell(unitriangular ? z : v11));
    }
    std::vector<Series> rows;
    rows.reserve(2);
    rows.push_back(stack_dim0_(row0_parts, "triangular_mask_complex_row0").reshape({2}).contiguous());
    rows.push_back(stack_dim0_(row1_parts, "triangular_mask_complex_row1").reshape({2}).contiguous());
    return stack_dim0_(rows, "triangular_mask_complex").contiguous();
  }
  const auto nd = value.dim();
  const auto rows_n = value.sizes()[static_cast<std::size_t>(nd - 2)];
  const auto cols_n = value.sizes()[static_cast<std::size_t>(nd - 1)];
  Series rows = Series::arange(0.0, static_cast<double>(rows_n), 1.0, ScalarType::Int64)
                    .reshape({rows_n, 1})
                    .to(value.platform());
  Series cols = Series::arange(0.0, static_cast<double>(cols_n), 1.0, ScalarType::Int64)
                    .reshape({1, cols_n})
                    .to(value.platform());
  const std::int64_t offset = unitriangular ? 1 : 0;
  Series mask = upper ? cols.ge(rows.add(static_cast<double>(offset)))
                      : cols.le(rows.sub(static_cast<double>(offset)));
  mask = mask.expand(value.sizes(), true).to(value.scalar_type());
  if (mask.platform() != value.platform()) mask = mask.to(value.platform());
  return value.mul(mask);
}

Series eye_matrix_like_(const Series& matrix) {
  PLUSS_CHECK(matrix.dim() >= 2, "subsp::linearalg: identity helper expects matrix input");
  const auto nd = matrix.dim();
  const auto rows = matrix.sizes()[static_cast<std::size_t>(nd - 2)];
  const auto cols = matrix.sizes()[static_cast<std::size_t>(nd - 1)];
  const ScalarType eye_dtype = matrix.scalar_type() == ScalarType::Complex64
      ? ScalarType::Float32
      : (matrix.scalar_type() == ScalarType::Complex128
             ? ScalarType::Float64
             : matrix.scalar_type());
  Series eye = Series::eye(rows, cols, eye_dtype).to(matrix.scalar_type()).to(matrix.platform());
  while (eye.dim() < matrix.dim()) {
    eye = eye.unsqueeze(0);
  }
  return eye.expand(matrix.sizes(), true).contiguous();
}

Series right_triangular_solve_(const Series& factor, const Series& rhs, bool upper, bool unitriangular) {
  Series solved_h = triangular_solve(matrix_adjoint_(factor), matrix_adjoint_(rhs), !upper, unitriangular);
  return matrix_adjoint_(solved_h);
}

Series matrix_diagonal_(const Series& x) {
  return x.mul(eye_matrix_like_(x));
}

Series real_as_complex_like_(const Series& x, const Series& like) {
  Series out = pluss::codem::ops_internal::real(x);
  return linalg_sensitivity_like_(std::move(out), like);
}

Series qr_syminv_adjoint_(const Series& x) {
  Series upper = triangular_masked_like_(x, /*upper=*/true, /*unitriangular=*/false);
  Series out = upper.add(matrix_adjoint_(upper));
  Series diagonal = matrix_diagonal_(out);
  if (is_complex_scalar_(out.scalar_type())) {
    diagonal = real_as_complex_like_(diagonal, out);
  }
  return out.sub(matrix_diagonal_(out)).add(diagonal.mul(0.5));
}

Series qr_backward_(const Series& q, const Series& r, const Series& grad, bool grad_is_q) {
  const auto m = q.sizes()[static_cast<std::size_t>(q.dim() - 2)];
  const auto n = r.sizes()[static_cast<std::size_t>(r.dim() - 1)];

  Series core = pluss::series::undefined();
  if (grad_is_q) {
    core = matrix_adjoint_(q).matmul(grad).neg();
  } else {
    core = grad.matmul(matrix_adjoint_(r));
  }
  if (m >= n) {
    Series out = q.matmul(qr_syminv_adjoint_(core));
    if (grad_is_q) {
      out = out.add(grad);
    }
    return right_triangular_solve_(matrix_adjoint_(r), out, /*upper=*/false, /*unitriangular=*/false);
  }

  const Series skew_arg = core.neg();
  Series lower_skew =
      triangular_masked_like_(skew_arg.sub(matrix_adjoint_(skew_arg)), /*upper=*/false, /*unitriangular=*/false);
  if (is_complex_scalar_(lower_skew.scalar_type())) {
    lower_skew = lower_skew.sub(matrix_diagonal_(lower_skew).mul(0.5));
  }
  Series wide_out = q.matmul(lower_skew);
  Series r_lead = r.slice(static_cast<std::int64_t>(r.dim()) - 1, 0, m).contiguous();
  r_lead = linalg_sensitivity_like_(r_lead, wide_out);
  wide_out = right_triangular_solve_(matrix_adjoint_(r_lead), wide_out, /*upper=*/false, /*unitriangular=*/false);
  std::vector<std::int64_t> tail_shape(wide_out.sizes().begin(), wide_out.sizes().end());
  tail_shape.back() = n - m;
  Series zeros_tail = Series::zeros(tail_shape, wide_out.scalar_type()).to(wide_out.platform());
  std::vector<Series> tail_parts;
  tail_parts.push_back(zeros_tail);
  wide_out = wide_out.unite(tail_parts, static_cast<std::int64_t>(wide_out.dim()) - 1);
  if (!grad_is_q) {
    wide_out = wide_out.add(q.matmul(grad));
  }
  return wide_out;
}

Series svd_gap_matrix_(const Series& s) {
  const Series s2 = s.mul(s);
  Series gap = s2.unsqueeze(static_cast<std::int64_t>(s2.dim()) - 1)
                   .sub(s2.unsqueeze(static_cast<std::int64_t>(s2.dim())));
  return gap.add(eye_matrix_like_(gap));
}

Series svd_values_backward_(const Series& u, const Series& s, const Series& vh, const Series& grad_s) {
  (void)s;
  const auto m = u.sizes()[static_cast<std::size_t>(u.dim() - 2)];
  const auto n = vh.sizes()[static_cast<std::size_t>(vh.dim() - 1)];
  return (m >= n) ? u.matmul(grad_s.unsqueeze(static_cast<std::int64_t>(grad_s.dim())).mul(vh))
                  : u.mul(grad_s.unsqueeze(static_cast<std::int64_t>(grad_s.dim()) - 1)).matmul(vh);
}

void check_single_svd_phase_(const Series& skew, const Series& singular_values) {
  if (!is_complex_scalar_(skew.scalar_type())) return;
  const std::size_t k = static_cast<std::size_t>(singular_values.sizes().back());
  const auto values = skew.to(Platform::CPU).to(ScalarType::Complex128)
                          .contiguous().to_vec<pluss::codem::Complex128>();
  for (std::size_t i = 0; i < k; ++i) {
    PLUSS_CHECK(
        std::abs(values[i * k + i].imag()) <= 1e-2,
        "subsp::linearalg::svd: singular-vector loss depends on an arbitrary complex phase");
  }
}

Series svd_vector_backward_(
    const Series& u,
    const Series& s,
    const Series& vh,
    const Series& grad,
    bool grad_is_u) {
  const auto m = u.sizes()[static_cast<std::size_t>(u.dim() - 2)];
  const auto n = vh.sizes()[static_cast<std::size_t>(vh.dim() - 1)];
  const Series gap = svd_gap_matrix_(s);
  Series core = Series::zeros(gap.sizes(), gap.scalar_type()).to(gap.platform());

  if (grad_is_u) {
    const Series skew = matrix_adjoint_(u).matmul(grad);
    check_single_svd_phase_(skew.sub(matrix_adjoint_(skew)), s);
    core = skew.sub(matrix_adjoint_(skew)).div(gap).mul(s.unsqueeze(static_cast<std::int64_t>(s.dim()) - 1));
    if (is_complex_scalar_(u.scalar_type())) {
      const Series diagonal = matrix_diagonal_(skew.sub(matrix_adjoint_(skew)));
      core = core.add(diagonal.div(s.unsqueeze(0).mul(4.0)));
    }
  } else {
    const Series skew = vh.matmul(matrix_adjoint_(grad));
    check_single_svd_phase_(skew.sub(matrix_adjoint_(skew)), s);
    core = s.unsqueeze(static_cast<std::int64_t>(s.dim())).mul(skew.sub(matrix_adjoint_(skew)).div(gap));
    if (is_complex_scalar_(u.scalar_type())) {
      const Series diagonal = matrix_diagonal_(skew.sub(matrix_adjoint_(skew)));
      core = core.sub(diagonal.div(s.unsqueeze(1).mul(4.0)));
    }
  }

  if (m > n && grad_is_u) {
    Series out = u.matmul(core);
    const Series scaled = grad.div(s.unsqueeze(static_cast<std::int64_t>(s.dim()) - 1));
    out = out.add(scaled).sub(u.matmul(matrix_adjoint_(u).matmul(scaled)));
    return out.matmul(vh);
  }
  if (m < n && !grad_is_u) {
    Series out = core.matmul(vh);
    const Series scaled = grad.div(s.unsqueeze(static_cast<std::int64_t>(s.dim())));
    out = out.add(scaled).sub(scaled.matmul(matrix_adjoint_(vh)).matmul(vh));
    return u.matmul(out);
  }
  return (m >= n) ? u.matmul(core).matmul(vh) : u.matmul(core.matmul(vh));
}

void check_svd_phase_invariance_(
    const std::optional<Series>& uhgu,
    const std::optional<Series>& vhgv,
    const Series& singular_values) {
  if ((!uhgu.has_value() || !is_complex_scalar_(uhgu->scalar_type())) &&
      (!vhgv.has_value() || !is_complex_scalar_(vhgv->scalar_type()))) {
    return;
  }
  const std::size_t k = static_cast<std::size_t>(singular_values.sizes().back());
  std::vector<double> lhs(k, 0.0);
  std::vector<double> rhs(k, 0.0);
  const auto read_diagonal = [k](const Series& matrix, std::vector<double>& out) {
    const auto values = matrix.to(Platform::CPU).to(ScalarType::Complex128)
                            .contiguous().to_vec<pluss::codem::Complex128>();
    for (std::size_t i = 0; i < k; ++i) {
      out[i] = values[i * k + i].imag();
    }
  };
  if (uhgu.has_value()) read_diagonal(*uhgu, lhs);
  if (vhgv.has_value()) read_diagonal(*vhgv, rhs);
  for (std::size_t i = 0; i < k; ++i) {
    const double target = -rhs[i];
    const double tolerance = 1e-2 + 1e-2 * std::abs(target);
    PLUSS_CHECK(
        std::abs(lhs[i] - target) <= tolerance,
        "subsp::linearalg::svd: singular-vector loss depends on an arbitrary complex phase");
  }
}

Series svd_backward_combined_(
    const Series& u,
    const Series& s,
    const Series& vh,
    const std::optional<Series>& grad_u,
    const std::optional<Series>& grad_s,
    const std::optional<Series>& grad_vh) {
  const auto m = u.sizes()[static_cast<std::size_t>(u.dim() - 2)];
  const auto n = vh.sizes()[static_cast<std::size_t>(vh.dim() - 1)];
  const Series gap = svd_gap_matrix_(s);
  std::optional<Series> uhgu;
  std::optional<Series> vhgv;
  if (grad_u.has_value()) {
    const Series product = matrix_adjoint_(u).matmul(*grad_u);
    uhgu = product.sub(matrix_adjoint_(product));
  }
  if (grad_vh.has_value()) {
    const Series product = vh.matmul(matrix_adjoint_(*grad_vh));
    vhgv = product.sub(matrix_adjoint_(product));
  }
  check_svd_phase_invariance_(uhgu, vhgv, s);

  Series core = Series::zeros(gap.sizes(), u.scalar_type()).to(u.platform());
  if (uhgu.has_value()) {
    core = core.add(uhgu->div(gap).mul(s.unsqueeze(static_cast<std::int64_t>(s.dim()) - 1)));
  }
  if (vhgv.has_value()) {
    core = core.add(s.unsqueeze(static_cast<std::int64_t>(s.dim())).mul(vhgv->div(gap)));
  }
  if (grad_s.has_value()) {
    Series diagonal = grad_s->to(u.scalar_type()).unsqueeze(0).mul(eye_matrix_like_(core));
    core = core.add(diagonal);
  }
  if (is_complex_scalar_(u.scalar_type()) && uhgu.has_value() && vhgv.has_value()) {
    core = core.add(matrix_diagonal_(*uhgu).div(s.unsqueeze(0).mul(2.0)));
  }

  if (m > n && grad_u.has_value()) {
    Series out = u.matmul(core);
    const Series scaled = grad_u->div(s.unsqueeze(static_cast<std::int64_t>(s.dim()) - 1));
    out = out.add(scaled).sub(u.matmul(matrix_adjoint_(u).matmul(scaled)));
    return out.matmul(vh);
  }
  if (m < n && grad_vh.has_value()) {
    Series out = core.matmul(vh);
    const Series scaled = grad_vh->div(s.unsqueeze(static_cast<std::int64_t>(s.dim())));
    out = out.add(scaled).sub(scaled.matmul(matrix_adjoint_(vh)).matmul(vh));
    return u.matmul(out);
  }
  return (m >= n) ? u.matmul(core).matmul(vh) : u.matmul(core.matmul(vh));
}

struct SvdBackwardState final {
  std::optional<Series> grad_u;
  std::optional<Series> grad_s;
  std::optional<Series> grad_vh;
};

void bind_svd_group_backward_(
    SvdDecomposition& decomposition,
    const Series& a,
    bool full_matrices) {
  if (!linalg_tracks_slope_(a)) return;
  const auto state = std::make_shared<SvdBackwardState>();
  Series carrier = Series::zeros({1}, a.scalar_type()).to(a.platform());
  pluss::codem::series_tape::set_tracks_slope(carrier, true);

  auto group_link = std::make_shared<pluss::codem::series_tape::SourceLink>();
  group_link->op_name = "subsp::linearalg::svd";
  group_link->parents = {a};
  group_link->propagate = [a, decomposition, state, full_matrices](const Series&) {
    const auto k = decomposition.s.sizes().back();
    const Series u = full_matrices
        ? decomposition.u.slice(-1, 0, k).contiguous()
        : decomposition.u;
    const Series vh = full_matrices
        ? decomposition.vh.slice(-2, 0, k).contiguous()
        : decomposition.vh;
    const auto narrow = [k, full_matrices](const std::optional<Series>& grad, int slot) {
      if (!grad.has_value() || !full_matrices || slot == 1) return grad;
      return std::optional<Series>{
          slot == 0 ? grad->slice(-1, 0, k).contiguous()
                    : grad->slice(-2, 0, k).contiguous()};
    };
    Series grad = svd_backward_combined_(
        u, decomposition.s, vh,
        narrow(state->grad_u, 0), state->grad_s, narrow(state->grad_vh, 2));
    pluss::codem::series_tape::accumulate_sensitivity(
        a, linalg_sensitivity_like_(std::move(grad), a));
  };
  pluss::codem::series_tape::bind_source_link(carrier, std::move(group_link));

  const auto bind_output = [&](const Series& output, int slot) {
    auto link = std::make_shared<pluss::codem::series_tape::SourceLink>();
    link->op_name = slot == 0 ? "subsp::linearalg::svd_u" :
        (slot == 1 ? "subsp::linearalg::svd_s" : "subsp::linearalg::svd_vh");
    link->parents = {carrier};
    link->propagate = [carrier, state, slot](const Series& out_sens) {
      if (slot == 0) state->grad_u = out_sens;
      else if (slot == 1) state->grad_s = out_sens;
      else state->grad_vh = out_sens;
      pluss::codem::series_tape::accumulate_sensitivity(
          carrier, Series::ones({1}, carrier.scalar_type()).to(carrier.platform()));
    };
    pluss::codem::series_tape::bind_source_link(output, std::move(link));
  };
  bind_output(decomposition.u, 0);
  bind_output(decomposition.s, 1);
  bind_output(decomposition.vh, 2);
}

void bind_qr_backward_(const Series& out, const Series& a, const Series& q, const Series& r, bool grad_is_q, bool reduced) {
  if (!linalg_tracks_slope_(a)) return;
  auto link = std::make_shared<pluss::codem::series_tape::SourceLink>();
  link->op_name = grad_is_q ? "subsp::linearalg::qr_q" : "subsp::linearalg::qr_r";
  link->parents = {a};
  link->propagate = [a, q, r, grad_is_q, reduced](const Series& out_sens) {
    const auto m = q.sizes()[static_cast<std::size_t>(q.dim() - 2)];
    const auto n = r.sizes()[static_cast<std::size_t>(r.dim() - 1)];
    PLUSS_CHECK(reduced || m <= n,
                "subsp::linearalg::qr: complete QR is not differentiable when rows exceed columns");
    Series grad = qr_backward_(q, r, linalg_sensitivity_like_(out_sens, q), grad_is_q);
    grad = linalg_sensitivity_like_(grad, a);
    pluss::codem::series_tape::accumulate_sensitivity(a, grad);
  };
  pluss::codem::series_tape::bind_source_link(out, std::move(link));
}

void bind_svd_backward_(const Series& out, const Series& a, const Series& u, const Series& s, const Series& vh, int slot, bool full_matrices) {
  if (!linalg_tracks_slope_(a)) return;
  auto link = std::make_shared<pluss::codem::series_tape::SourceLink>();
  link->op_name = slot == 0 ? "subsp::linearalg::svd_u" : (slot == 1 ? "subsp::linearalg::svd_s" : "subsp::linearalg::svd_vh");
  link->parents = {a};
  link->propagate = [a, u, s, vh, slot, full_matrices](const Series& out_sens) {
    const auto k = s.sizes().back();
    Series reduced_u = full_matrices ? u.slice(-1, 0, k).contiguous() : u;
    Series reduced_vh = full_matrices ? vh.slice(-2, 0, k).contiguous() : vh;
    Series reduced_sens = out_sens;
    if (full_matrices && slot == 0) reduced_sens = out_sens.slice(-1, 0, k).contiguous();
    if (full_matrices && slot == 2) reduced_sens = out_sens.slice(-2, 0, k).contiguous();
    Series grad = slot == 1
        ? svd_values_backward_(reduced_u, s, reduced_vh, linalg_sensitivity_like_(reduced_sens, s))
        : svd_vector_backward_(reduced_u, s, reduced_vh,
                              linalg_sensitivity_like_(reduced_sens, slot == 0 ? reduced_u : reduced_vh),
                              slot == 0);
    grad = linalg_sensitivity_like_(grad, a);
    pluss::codem::series_tape::accumulate_sensitivity(a, grad);
  };
  pluss::codem::series_tape::bind_source_link(out, std::move(link));
}

std::uint64_t matrix_power_magnitude_(std::int64_t exponent) {
  return exponent < 0
      ? static_cast<std::uint64_t>(-(exponent + 1)) + 1ULL
      : static_cast<std::uint64_t>(exponent);
}

Series matrix_power_positive_backward_(const Series& base_in, const Series& grad, std::uint64_t exponent) {
  // The internal power/inverse solvers compute in float64 regardless of input
  // dtype; keep the captured base in the gradient's dtype so every matmul below
  // has matching operands (a lower-precision input would otherwise mismatch).
  const Series base = base_in.scalar_type() == grad.scalar_type()
                          ? base_in
                          : base_in.to(grad.scalar_type());
  if (exponent == 1) return grad;
  const std::uint64_t half = exponent / 2ULL;
  const Series half_power = matrix_power(base, static_cast<std::int64_t>(half));
  const Series half_h = matrix_adjoint_(half_power);
  if ((exponent & 1ULL) == 0ULL) {
    const Series grad_half = grad.matmul(half_h).add(half_h.matmul(grad));
    return matrix_power_positive_backward_(base, grad_half, half);
  }

  const Series base_h = matrix_adjoint_(base);
  const Series square = half_power.matmul(half_power);
  const Series grad_square = grad.matmul(base_h);
  const Series direct = matrix_adjoint_(square).matmul(grad);
  const Series grad_half = grad_square.matmul(half_h).add(half_h.matmul(grad_square));
  return direct.add(matrix_power_positive_backward_(base, grad_half, half));
}

void bind_matrix_power_backward_(const Series& out, const Series& a, std::int64_t exponent) {
  if (!linalg_tracks_slope_(a)) return;
  auto link = std::make_shared<pluss::codem::series_tape::SourceLink>();
  link->op_name = "subsp::linearalg::matrix_power";
  link->parents = {a};
  link->propagate = [a, exponent](const Series& out_sens) {
    const std::uint64_t magnitude = matrix_power_magnitude_(exponent);
    Series grad = pluss::series::undefined();
    if (magnitude == 0) {
      grad = Series::zeros(a.sizes(), a.scalar_type()).to(a.platform());
    } else if (exponent > 0) {
      grad = matrix_power_positive_backward_(a, out_sens, magnitude);
    } else {
      const Series inverse_a = inverse(a);
      const Series grad_inverse = matrix_power_positive_backward_(
          inverse_a, linalg_sensitivity_like_(out_sens, inverse_a), magnitude);
      const Series inverse_h = matrix_adjoint_(inverse_a);
      grad = inverse_h.matmul(grad_inverse).matmul(inverse_h).neg();
    }
    grad = linalg_sensitivity_like_(grad, a);
    pluss::codem::series_tape::accumulate_sensitivity(a, grad);
  };
  pluss::codem::series_tape::bind_source_link(out, std::move(link));
}

void bind_matrix_exp_backward_(const Series& out, const Series& a) {
  if (!linalg_tracks_slope_(a)) return;
  auto link = std::make_shared<pluss::codem::series_tape::SourceLink>();
  link->op_name = "subsp::linearalg::matrix_exp";
  link->parents = {a};
  link->propagate = [a](const Series& out_sens) {
    const auto n = a.sizes()[static_cast<std::size_t>(a.dim() - 1)];
    const auto matrix_size = static_cast<std::size_t>(n * n);
    if (is_complex_scalar_(a.scalar_type())) {
      const auto av = a.to(Platform::CPU)
                          .to(ScalarType::Complex128)
                          .contiguous()
                          .to_vec<std::complex<double>>();
      const auto gv = out_sens.to(Platform::CPU)
                          .to(ScalarType::Complex128)
                          .contiguous()
                          .to_vec<std::complex<double>>();
      PLUSS_CHECK(av.size() == gv.size() && matrix_size > 0 && av.size() % matrix_size == 0,
                  "subsp::linearalg::matrix_exp: invalid complex reverse-mode batch shape");
      std::vector<std::complex<double>> values(av.size());
      const std::int64_t block_n = 2 * n;
      for (std::size_t offset = 0; offset < av.size(); offset += matrix_size) {
        std::vector<std::complex<double>> block(
            static_cast<std::size_t>(block_n * block_n));
        for (std::int64_t r = 0; r < n; ++r) {
          for (std::int64_t c = 0; c < n; ++c) {
            const auto ah = std::conj(av[offset + static_cast<std::size_t>(c * n + r)]);
            block[static_cast<std::size_t>(r * block_n + c)] = ah;
            block[static_cast<std::size_t>((r + n) * block_n + c + n)] = ah;
            block[static_cast<std::size_t>(r * block_n + c + n)] =
                gv[offset + static_cast<std::size_t>(r * n + c)];
          }
        }
        const auto exponential =
            pluss::subsp::math::linalg::detail::matrix_exp_t_(block, block_n);
        for (std::int64_t r = 0; r < n; ++r) {
          for (std::int64_t c = 0; c < n; ++c) {
            values[offset + static_cast<std::size_t>(r * n + c)] =
                exponential[static_cast<std::size_t>(r * block_n + c + n)];
          }
        }
      }
      const std::vector<std::int64_t> shape(a.sizes().begin(), a.sizes().end());
      Series grad = pluss::make_series(
          values,
          shape,
          Config{}.with_dtype(ScalarType::Complex128).with_platform(Platform::CPU));
      grad = linalg_sensitivity_like_(grad, a);
      pluss::codem::series_tape::accumulate_sensitivity(a, grad);
      return;
    }
    const auto av = a.to(Platform::CPU).to(ScalarType::Float64).contiguous().to_vec<double>();
    const auto gv = out_sens.to(Platform::CPU).to(ScalarType::Float64).contiguous().to_vec<double>();
    PLUSS_CHECK(av.size() == gv.size() && matrix_size > 0 && av.size() % matrix_size == 0,
                "subsp::linearalg::matrix_exp: invalid reverse-mode batch shape");
    std::vector<double> values(av.size(), 0.0);
    for (std::size_t offset = 0; offset < av.size(); offset += matrix_size) {
      const std::vector<double> matrix(av.begin() + static_cast<std::ptrdiff_t>(offset),
                                       av.begin() + static_cast<std::ptrdiff_t>(offset + matrix_size));
      const std::vector<double> incoming(gv.begin() + static_cast<std::ptrdiff_t>(offset),
                                         gv.begin() + static_cast<std::ptrdiff_t>(offset + matrix_size));
      const auto part = pluss::subsp::math::linalg::detail::matrix_exp_frechet_adjoint_(matrix, incoming, n);
      std::copy(part.begin(), part.end(), values.begin() + static_cast<std::ptrdiff_t>(offset));
    }
    const std::vector<std::int64_t> shape(a.sizes().begin(), a.sizes().end());
    Series grad = pluss::make_series(
        values,
        shape,
        Config{}.with_dtype(ScalarType::Float64).with_platform(Platform::CPU));
    grad = linalg_sensitivity_like_(grad, a);
    pluss::codem::series_tape::accumulate_sensitivity(a, grad);
  };
  pluss::codem::series_tape::bind_source_link(out, std::move(link));
}

void bind_pinverse_backward_(const Series& out, const Series& a) {
  if (!linalg_tracks_slope_(a)) return;
  auto link = std::make_shared<pluss::codem::series_tape::SourceLink>();
  link->op_name = "subsp::linearalg::pinverse";
  link->parents = {a};
  link->propagate = [a, out](const Series& out_sens) {
    const Series p = out;
    const Series p_h = matrix_adjoint_(p);
    const Series g = linalg_sensitivity_like_(out_sens, p);
    const Series g_h = matrix_adjoint_(g);
    // `p` is computed in float64; align the captured input so the residual
    // matmuls below match when `a` is a lower-precision real dtype.
    const Series a_c = a.scalar_type() == p.scalar_type() ? a : a.to(p.scalar_type());
    const Series left_residual = eye_matrix_like_(a_c.matmul(p)).sub(a_c.matmul(p));
    const Series right_residual = eye_matrix_like_(p.matmul(a_c)).sub(p.matmul(a_c));
    Series grad = p_h.matmul(g).matmul(p_h).neg();
    grad = grad.add(left_residual.matmul(g_h).matmul(p).matmul(p_h));
    grad = grad.add(p_h.matmul(p).matmul(g_h).matmul(right_residual));
    grad = linalg_sensitivity_like_(grad, a);
    pluss::codem::series_tape::accumulate_sensitivity(a, grad);
  };
  pluss::codem::series_tape::bind_source_link(out, std::move(link));
}

void bind_inverse_backward_(const Series& out, const Series& a, const Series& inverse_out) {
  if (!linalg_tracks_slope_(a)) return;
  PLUSS_CHECK(a.dim() >= 2, "subsp::linearalg::inverse: reverse-mode expects matrix input");
  auto link = std::make_shared<pluss::codem::series_tape::SourceLink>();
  link->op_name = "subsp::linearalg::inverse";
  link->parents = {a};
  link->propagate = [a, inverse_out](const Series& out_sens) {
    const Series inv_h = matrix_adjoint_(inverse_out);
    Series grad = inv_h.matmul(out_sens).matmul(inv_h).neg();
    grad = linalg_sensitivity_like_(grad, a);
    pluss::codem::series_tape::accumulate_sensitivity(a, grad);
  };
  pluss::codem::series_tape::bind_source_link(out, std::move(link));
}

void bind_det_backward_(const Series& out, const Series& a, const Series& det_out) {
  if (!linalg_tracks_slope_(a)) return;
  PLUSS_CHECK(a.dim() >= 2, "subsp::linearalg::det: reverse-mode expects matrix input");
  auto link = std::make_shared<pluss::codem::series_tape::SourceLink>();
  link->op_name = "subsp::linearalg::det";
  link->parents = {a};
  link->propagate = [a, det_out](const Series& out_sens) {
    Series inv_h = det_small_real_backward_factor_(a);
    if (!inv_h.defined()) {
      inv_h = matrix_adjoint_(inverse(a));
    }
    const Series det_scale =
        is_complex_scalar_(det_out.scalar_type()) ? pluss::codem::ops_internal::conj(det_out) : det_out;
    Series scale = inv_h.defined() && inv_h.dim() == 2 && a.dim() == 2 && !is_complex_scalar_(a.scalar_type()) &&
                           a.sizes()[0] <= 3
                       ? out_sens
                       : out_sens.mul(det_scale);
    scale = scale_to_matrix_batch_(scale, inv_h);
    Series grad = inv_h.mul(scale);
    grad = linalg_sensitivity_like_(grad, a);
    pluss::codem::series_tape::accumulate_sensitivity(a, grad);
  };
  pluss::codem::series_tape::bind_source_link(out, std::move(link));
}

void bind_logdet_backward_(const Series& out, const Series& a) {
  if (!linalg_tracks_slope_(a)) return;
  PLUSS_CHECK(a.dim() >= 2, "subsp::linearalg::logdet: reverse-mode expects matrix input");
  auto link = std::make_shared<pluss::codem::series_tape::SourceLink>();
  link->op_name = "subsp::linearalg::logdet";
  link->parents = {a};
  link->propagate = [a](const Series& out_sens) {
    const Series inv_h = matrix_adjoint_(inverse(a));
    Series scale = scale_to_matrix_batch_(out_sens, inv_h);
    Series grad = inv_h.mul(scale);
    grad = linalg_sensitivity_like_(grad, a);
    pluss::codem::series_tape::accumulate_sensitivity(a, grad);
  };
  pluss::codem::series_tape::bind_source_link(out, std::move(link));
}

void bind_slogdet_backward_(const Series& out, const Series& a) {
  if (!linalg_tracks_slope_(a)) return;
  PLUSS_CHECK(a.dim() >= 2, "subsp::linearalg::slogdet: reverse-mode expects matrix input");
  auto link = std::make_shared<pluss::codem::series_tape::SourceLink>();
  link->op_name = "subsp::linearalg::slogdet";
  link->parents = {a};
  link->propagate = [a, out](const Series& out_sens) {
    const std::int64_t last = static_cast<std::int64_t>(out_sens.dim()) - 1;
    Series logabs_sens = out_sens.select(last, 1);
    if (is_complex_scalar_(a.scalar_type())) {
      const Series sign_sens = out_sens.select(last, 0);
      const Series sign = out.select(last, 0);
      const Series phase_scale = pluss::codem::ops_internal::imag(
          pluss::codem::ops_internal::conj(sign_sens).mul(sign));
      const auto i = pluss::make_series(
          std::vector<std::complex<double>>{{0.0, 1.0}},
          std::vector<std::int64_t>{},
          Config{}.with_dtype(ScalarType::Complex128).with_platform(Platform::CPU))
                         .to(a.scalar_type())
                         .to(a.platform());
      logabs_sens = pluss::codem::ops_internal::real(logabs_sens)
                        .to(a.scalar_type())
                        .sub(phase_scale.to(a.scalar_type()).mul(i));
    }
    const Series inv_h = matrix_adjoint_(inverse(a));
    Series scale = scale_to_matrix_batch_(logabs_sens, inv_h);
    Series grad = inv_h.mul(scale);
    grad = linalg_sensitivity_like_(grad, a);
    pluss::codem::series_tape::accumulate_sensitivity(a, grad);
  };
  pluss::codem::series_tape::bind_source_link(out, std::move(link));
}

void bind_triangular_solve_backward_(
    const Series& out,
    const Series& a,
    const Series& b,
    bool upper,
    bool unitriangular) {
  if (!pluss::codem::series_tape::recording_enabled()) return;
  const bool ta = pluss::codem::series_tape::tracks_slope(a);
  const bool tb = pluss::codem::series_tape::tracks_slope(b);
  if (!ta && !tb) return;
  PLUSS_CHECK(a.dim() >= 2, "subsp::linearalg::triangular_solve: reverse-mode expects matrix factor");

  auto link = std::make_shared<pluss::codem::series_tape::SourceLink>();
  link->op_name = "subsp::linearalg::triangular_solve";
  link->parents = {a, b};
  link->propagate = [a, b, out, upper, unitriangular, ta, tb](const Series& out_sens) {
    const Series grad_b_raw = triangular_solve(matrix_adjoint_(a), out_sens, !upper, unitriangular);
    if (tb) {
      Series grad_b = grad_b_raw;
      if (b.dim() == 1 && grad_b.dim() == 2 && grad_b.sizes()[1] == 1) {
        grad_b = grad_b.reshape({b.sizes()[0]});
      }
      grad_b = linalg_sensitivity_like_(grad_b, b);
      pluss::codem::series_tape::accumulate_sensitivity(b, grad_b);
    }
    if (ta) {
      Series grad_a = grad_b_raw.matmul(matrix_adjoint_(out)).neg();
      grad_a = triangular_masked_like_(grad_a, upper, unitriangular);
      grad_a = linalg_sensitivity_like_(grad_a, a);
      pluss::codem::series_tape::accumulate_sensitivity(a, grad_a);
    }
  };
  pluss::codem::series_tape::bind_source_link(out, std::move(link));
}

void bind_cholesky_solve_backward_(const Series& out, const Series& factor, const Series& b, bool upper) {
  if (!pluss::codem::series_tape::recording_enabled()) return;
  const bool tf = pluss::codem::series_tape::tracks_slope(factor);
  const bool tb = pluss::codem::series_tape::tracks_slope(b);
  if (!tf && !tb) return;

  auto link = std::make_shared<pluss::codem::series_tape::SourceLink>();
  link->op_name = "subsp::linearalg::cholesky_solve";
  link->parents = {factor, b};
  link->propagate = [factor, b, out, upper, tf, tb](const Series& out_sens) {
    const Series grad_b_raw = cholesky_solve(factor, out_sens, upper);
    if (tb) {
      Series grad_b = grad_b_raw;
      if (b.dim() == 1 && grad_b.dim() == 2 && grad_b.sizes()[1] == 1) {
        grad_b = grad_b.reshape({b.sizes()[0]});
      }
      grad_b = linalg_sensitivity_like_(grad_b, b);
      pluss::codem::series_tape::accumulate_sensitivity(b, grad_b);
    }
    if (tf) {
      Series common = grad_b_raw.matmul(matrix_adjoint_(out));
      common = common.add(matrix_adjoint_(common));
      Series grad_factor = upper ? factor.matmul(common).neg() : common.matmul(factor).neg();
      grad_factor = linalg_sensitivity_like_(grad_factor, factor);
      pluss::codem::series_tape::accumulate_sensitivity(factor, grad_factor);
    }
  };
  pluss::codem::series_tape::bind_source_link(out, std::move(link));
}

Series cholesky_backward_(const Series& factor, const Series& grad, bool upper) {
  const Series lower = upper ? matrix_adjoint_(factor) : factor;
  const Series lower_grad = upper ? matrix_adjoint_(grad) : grad;
  Series middle = matrix_adjoint_(lower).matmul(lower_grad);
  middle = triangular_masked_like_(middle, /*upper=*/false, /*unitriangular=*/false);
  const Series diagonal = matrix_diagonal_(middle);
  middle = middle.add(matrix_adjoint_(middle.sub(diagonal))).mul(0.5);
  Series result = triangular_solve(
      matrix_adjoint_(lower), middle, /*upper=*/true, /*unitriangular=*/false);
  result = right_triangular_solve_(lower, result, /*upper=*/false, /*unitriangular=*/false);
  return result;
}

void bind_cholesky_backward_(const Series& out, const Series& a, bool upper) {
  if (!linalg_tracks_slope_(a)) return;
  auto link = std::make_shared<pluss::codem::series_tape::SourceLink>();
  link->op_name = "subsp::linearalg::cholesky";
  link->parents = {a};
  link->propagate = [a, out, upper](const Series& out_sens) {
    Series grad = cholesky_backward_(out, linalg_sensitivity_like_(out_sens, out), upper);
    grad = linalg_sensitivity_like_(grad, a);
    pluss::codem::series_tape::accumulate_sensitivity(a, grad);
  };
  pluss::codem::series_tape::bind_source_link(out, std::move(link));
}

void bind_cholesky_inverse_backward_(const Series& out, const Series& factor, bool upper) {
  if (!linalg_tracks_slope_(factor)) return;
  auto link = std::make_shared<pluss::codem::series_tape::SourceLink>();
  link->op_name = "subsp::linearalg::cholesky_inverse";
  link->parents = {factor};
  link->propagate = [factor, out, upper](const Series& out_sens) {
    const Series out_h = matrix_adjoint_(out);
    Series grad_a = out_h.matmul(out_sens).matmul(out_h).neg();
    const Series symmetric = grad_a.add(matrix_adjoint_(grad_a));
    Series grad_factor = upper ? factor.matmul(symmetric) : symmetric.matmul(factor);
    grad_factor = triangular_masked_like_(grad_factor, upper, /*unitriangular=*/false);
    grad_factor = linalg_sensitivity_like_(grad_factor, factor);
    pluss::codem::series_tape::accumulate_sensitivity(factor, grad_factor);
  };
  pluss::codem::series_tape::bind_source_link(out, std::move(link));
}

void bind_lu_solve_backward_(const Series& out, const Series& a, const Series& b) {
  if (!pluss::codem::series_tape::recording_enabled()) return;
  const bool ta = pluss::codem::series_tape::tracks_slope(a);
  const bool tb = pluss::codem::series_tape::tracks_slope(b);
  if (!ta && !tb) return;
  auto link = std::make_shared<pluss::codem::series_tape::SourceLink>();
  link->op_name = "subsp::linearalg::lu_solve";
  link->parents = {a, b};
  link->propagate = [a, b, out, ta, tb](const Series& out_sens) {
    Series rhs = out_sens;
    if (rhs.dim() == 1) rhs = rhs.unsqueeze(-1);
    Series grad_b = lu_solve(matrix_adjoint_(a), rhs);
    if (tb) {
      Series shaped = grad_b;
      if (b.dim() == 1 && shaped.dim() == 2 && shaped.sizes()[1] == 1) shaped = shaped.select(1, 0);
      shaped = linalg_sensitivity_like_(shaped, b);
      pluss::codem::series_tape::accumulate_sensitivity(b, shaped);
    }
    if (ta) {
      Series solution = out;
      if (solution.dim() == 1) solution = solution.unsqueeze(-1);
      Series grad_a = grad_b.matmul(matrix_adjoint_(solution)).neg();
      grad_a = linalg_sensitivity_like_(grad_a, a);
      pluss::codem::series_tape::accumulate_sensitivity(a, grad_a);
    }
  };
  pluss::codem::series_tape::bind_source_link(out, std::move(link));
}

void bind_qr_backward_lazy_(const Series& out, const Series& a, bool grad_is_q, bool reduced) {
  if (!linalg_tracks_slope_(a)) return;
  auto link = std::make_shared<pluss::codem::series_tape::SourceLink>();
  link->op_name = grad_is_q ? "subsp::linearalg::qr_q" : "subsp::linearalg::qr_r";
  link->parents = {a};
  link->propagate = [a, out, grad_is_q, reduced](const Series& out_sens) {
    const Series q = grad_is_q ? out : qr_q(a, reduced);
    const Series r = grad_is_q ? qr_r(a, reduced) : out;
    const auto m = q.sizes()[static_cast<std::size_t>(q.dim() - 2)];
    const auto n = r.sizes()[static_cast<std::size_t>(r.dim() - 1)];
    PLUSS_CHECK(reduced || m <= n,
                "subsp::linearalg::qr: complete QR is not differentiable when rows exceed columns");
    Series grad = qr_backward_(q, r, linalg_sensitivity_like_(out_sens, out), grad_is_q);
    grad = linalg_sensitivity_like_(grad, a);
    pluss::codem::series_tape::accumulate_sensitivity(a, grad);
  };
  pluss::codem::series_tape::bind_source_link(out, std::move(link));
}

void bind_svd_backward_lazy_(
    const Series& out,
    const Series& a,
    int slot,
    bool full_matrices) {
  if (!linalg_tracks_slope_(a)) return;
  auto link = std::make_shared<pluss::codem::series_tape::SourceLink>();
  link->op_name = slot == 0 ? "subsp::linearalg::svd_u" : (slot == 1 ? "subsp::linearalg::svd_s" : "subsp::linearalg::svd_vh");
  link->parents = {a};
  link->propagate = [a, out, slot, full_matrices](const Series& out_sens) {
    const SvdDecomposition decomposition = svd(a, full_matrices);
    const auto k = decomposition.s.sizes().back();
    Series reduced_u = full_matrices ? decomposition.u.slice(-1, 0, k).contiguous() : decomposition.u;
    Series reduced_vh = full_matrices ? decomposition.vh.slice(-2, 0, k).contiguous() : decomposition.vh;
    Series reduced_sens = out_sens;
    if (full_matrices && slot == 0) reduced_sens = out_sens.slice(-1, 0, k).contiguous();
    if (full_matrices && slot == 2) reduced_sens = out_sens.slice(-2, 0, k).contiguous();
    Series grad = slot == 1
        ? svd_values_backward_(reduced_u, decomposition.s, reduced_vh,
                               linalg_sensitivity_like_(reduced_sens, decomposition.s))
        : svd_vector_backward_(reduced_u, decomposition.s, reduced_vh,
                               linalg_sensitivity_like_(reduced_sens, slot == 0 ? reduced_u : reduced_vh),
                               slot == 0);
    grad = linalg_sensitivity_like_(grad, a);
    pluss::codem::series_tape::accumulate_sensitivity(a, grad);
  };
  pluss::codem::series_tape::bind_source_link(out, std::move(link));
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
  return pluss::codem::_compat::invoke(op, a, {b});
}

inline void require_square_2d_(const Series& a, const char* op) {
  require_defined_(a, op);
  PLUSS_CHECK(a.dim() == 2, std::string("subsp::linearalg::") + op + ": expected 2D input");
  PLUSS_CHECK(
      a.sizes()[0] == a.sizes()[1],
      std::string("subsp::linearalg::") + op + ": expected square matrix");
}

inline Series stack_dim0_(const std::vector<Series>& parts, const char* op) {
  PLUSS_CHECK(!parts.empty(), std::string("subsp::linearalg::") + op + ": internal stack expects non-empty parts");
  if (parts.size() == 1) {
    return parts.front().unsqueeze(0);
  }
  std::vector<Series> tails;
  tails.reserve(parts.size() - 1);
  for (std::size_t i = 1; i < parts.size(); ++i) {
    tails.push_back(parts[i]);
  }
  return parts.front().pile(tails, 0).contiguous();
}

inline ScalarType decomp_fallback_dtype_(Platform p) {
  // CPU subsp::math::linalg returns Float64; MPS bridge/native dispatches
  // currently materialize Float32 tensors on device.
  return (p == Platform::CPU) ? ScalarType::Float64 : ScalarType::Float32;
}

inline std::vector<std::int64_t> tail_shape_(const Series& x, std::int64_t tail_rank, const char* op) {
  PLUSS_CHECK(
      x.dim() >= tail_rank,
      std::string("subsp::linearalg::") + op + ": expected rank >= tail rank");
  const auto sz = x.sizes();
  return std::vector<std::int64_t>(sz.end() - tail_rank, sz.end());
}

inline std::vector<std::int64_t> batch_shape_(const Series& x, std::int64_t tail_rank, const char* op) {
  PLUSS_CHECK(
      x.dim() >= tail_rank,
      std::string("subsp::linearalg::") + op + ": expected rank >= tail rank");
  const auto sz = x.sizes();
  return std::vector<std::int64_t>(sz.begin(), sz.end() - tail_rank);
}

inline std::vector<std::int64_t> broadcast_batch_shape_(
    const std::vector<std::int64_t>& a,
    const std::vector<std::int64_t>& b,
    const char* op) {
  const std::size_t na = a.size();
  const std::size_t nb = b.size();
  const std::size_t n = (na > nb) ? na : nb;
  std::vector<std::int64_t> out(n, 1);
  for (std::size_t i = 0; i < n; ++i) {
    const std::size_t ia = (i < (n - na)) ? static_cast<std::size_t>(-1) : (i - (n - na));
    const std::size_t ib = (i < (n - nb)) ? static_cast<std::size_t>(-1) : (i - (n - nb));
    const std::int64_t da = (ia == static_cast<std::size_t>(-1)) ? 1 : a[ia];
    const std::int64_t db = (ib == static_cast<std::size_t>(-1)) ? 1 : b[ib];
    PLUSS_CHECK(
        da == db || da == 1 || db == 1,
        std::string("subsp::linearalg::") + op + ": batch shapes are not broadcastable");
    out[i] = (da > db) ? da : db;
  }
  return out;
}

inline std::vector<std::int64_t> concat_shape_(
    const std::vector<std::int64_t>& batch,
    const std::vector<std::int64_t>& tail) {
  std::vector<std::int64_t> out;
  out.reserve(batch.size() + tail.size());
  out.insert(out.end(), batch.begin(), batch.end());
  out.insert(out.end(), tail.begin(), tail.end());
  return out;
}

inline bool has_zero_extent_(const std::vector<std::int64_t>& shape) noexcept {
  for (const auto d : shape) {
    if (d == 0) {
      return true;
    }
  }
  return false;
}

inline std::int64_t numel_from_shape_(const std::vector<std::int64_t>& shape) {
  std::int64_t out = 1;
  for (const auto d : shape) {
    PLUSS_CHECK(d >= 0, "subsp::linearalg: expected non-negative shape extent");
    out *= d;
  }
  return out;
}

inline Series flatten_expanded_batch_(
    const Series& x,
    const std::vector<std::int64_t>& batch,
    const std::vector<std::int64_t>& tail,
    const char* op) {
  Series expanded = x.expand(concat_shape_(batch, tail), true).contiguous();
  if (batch.empty()) {
    return expanded;
  }
  std::vector<std::int64_t> flat_shape;
  flat_shape.reserve(static_cast<std::size_t>(1 + tail.size()));
  flat_shape.push_back(numel_from_shape_(batch));
  flat_shape.insert(flat_shape.end(), tail.begin(), tail.end());
  PLUSS_CHECK(
      flat_shape.front() >= 0,
      std::string("subsp::linearalg::") + op + ": invalid flattened batch size");
  return expanded.reshape(flat_shape).contiguous();
}

inline Series restore_flat_batch_shape_(
    const Series& flat,
    const std::vector<std::int64_t>& batch,
    const std::vector<std::int64_t>& tail) {
  if (batch.empty()) {
    return flat;
  }
  return flat.reshape(concat_shape_(batch, tail)).contiguous();
}

enum class SolverRhsLayout : std::uint8_t {
  Vector = 0,
  Matrix = 1,
};

inline SolverRhsLayout detect_solver_rhs_layout_(const Series& a, const Series& b, const char* op) {
  PLUSS_CHECK(
      b.dim() >= 1,
      std::string("subsp::linearalg::") + op + ": rhs must have rank >= 1");
  const auto n = a.sizes()[static_cast<std::size_t>(a.dim() - 1)];
  if (b.dim() >= 2 && b.sizes()[static_cast<std::size_t>(b.dim() - 2)] == n) {
    return SolverRhsLayout::Matrix;
  }
  PLUSS_CHECK(
      b.sizes()[static_cast<std::size_t>(b.dim() - 1)] == n,
      std::string("subsp::linearalg::") + op + ": rhs leading solve dimension mismatch");
  return SolverRhsLayout::Vector;
}

inline bool mps_experimental_native_enabled_() noexcept {
  return mps_native_route_enabled_();
}

inline bool mps_native_device_triangular_enabled_() noexcept {
  return parse_bool_env_("PLUSS_MPS_LINALG_NATIVE_DEVICE_TRIANGULAR", false);
}

inline Series eye_like_(const Series& a) {
  return eye_matrix_like_(a);
}

inline Series canonical_mps_native_f32_(const Series& x) {
  Series out = x;
  if (out.platform() != Platform::MPS) {
    out = out.to(Platform::MPS);
  }
  if (out.scalar_type() != ScalarType::Float32) {
    out = out.to(ScalarType::Float32);
  }
  if (!out.is_contiguous()) {
    out = out.contiguous();
  }
  return out;
}

[[noreturn]] inline void throw_platform_not_ready_(const char* op, Platform p) {
  const char* pstr = (p == Platform::CPU) ? "CPU" : (p == Platform::MPS) ? "MPS" : (p == Platform::CUDA) ? "CUDA" : "Unknown";
  pluss::codem::throw_error(
      std::string("subsp::linearalg::") + op +
      ": no native implementation on " + pstr +
      " yet; use CPU input for now");
}

[[noreturn]] inline void throw_cuda_linalg_unavailable_(
    const char* op,
    bool native_enabled,
    bool native_strict,
    bool route_supported) {
  if (native_strict && (!native_enabled || !route_supported)) {
    pluss::codem::throw_error(
        std::string("subsp::linearalg::") + op +
        ": strict native CUDA route requested but kernel is unsupported");
  }
  if (native_enabled && route_supported) {
    pluss::codem::throw_error(
        std::string("subsp::linearalg::") + op +
        ": CUDA native linalg route declared but kernel is not implemented yet");
  }
  pluss::codem::throw_error(
      std::string("subsp::linearalg::") + op +
      ": CUDA path unavailable in this tranche; no CPU fallback allowed. "
      "Move Series to CPU explicitly or implement CUDA linalg kernel.");
}

inline void require_cuda_linalg_bridge_policy_(const char* op, bool allow_strict_native) {
  if (pluss::codem::native::cuda::cpu_bridge_enabled() &&
      (!pluss::codem::native::cuda::native_linalg_strict() || allow_strict_native)) {
    return;
  }
  pluss::codem::throw_error(
      std::string("subsp::linearalg::") + op +
      ": CUDA bridge/native staging disabled by policy "
      "(set PLUSS_CUDA_ALLOW_CPU_BRIDGE=1; for bridge fallback also set PLUSS_CUDA_NATIVE_LINALG_STRICT=0)");
}

inline Series bridge_cpu_result_to_cuda_(const Series& cpu_result, const char* op_name, bool allow_strict_native = false) {
  require_cuda_linalg_bridge_policy_(op_name, allow_strict_native);
  Series out = cpu_result;
  if (out.platform() != Platform::CPU) {
    out = out.to(Platform::CPU);
  }
  return out.to(Platform::CUDA);
}

template <class CpuRouteFn>
inline Series cuda_linalg_route_or_bridge_(
    const char* op_name,
    bool native_enabled,
    bool native_strict,
    bool route_supported,
    CpuRouteFn&& cpu_route) {
  if (native_enabled && route_supported) {
    throw_cuda_linalg_unavailable_(op_name, native_enabled, native_strict, route_supported);
  }
  if (native_strict) {
    pluss::codem::throw_error(
        std::string("subsp::linearalg::") + op_name +
        ": strict native CUDA route requested but kernel is unsupported");
  }
  if (pluss::codem::cuda_cpu_bridge_enabled()) {
    return bridge_cpu_result_to_cuda_(cpu_route(), op_name);
  }
  throw_cuda_linalg_unavailable_(op_name, native_enabled, native_strict, route_supported);
}

inline Series matrix_power_native_(const Series& a, std::int64_t exp) {
  require_square_2d_(a, "matrix_power");
  if (exp == 0) {
    return eye_like_(a);
  }
  std::uint64_t p = exp < 0
      ? static_cast<std::uint64_t>(-(exp + 1)) + 1ULL
      : static_cast<std::uint64_t>(exp);
  Series out = eye_like_(a);
  Series base = exp < 0 ? inverse(a) : a;
  while (p > 0) {
    if ((p & 1ULL) != 0ULL) out = out.matmul(base);
    p >>= 1ULL;
    if (p) base = base.matmul(base);
  }
  return out;
}

inline Series matrix_exp_native_(const Series& a) {
  require_square_2d_(a, "matrix_exp");
  const auto values = a.to(Platform::CPU).to(ScalarType::Float64).contiguous().to_vec<double>();
  const auto n = a.sizes()[0];
  double norm_inf = 0.0;
  for (std::int64_t r = 0; r < n; ++r) {
    double row_sum = 0.0;
    for (std::int64_t c = 0; c < n; ++c) {
      row_sum += std::abs(values[static_cast<std::size_t>(r * n + c)]);
    }
    norm_inf = std::max(norm_inf, row_sum);
  }
  const std::int64_t squarings = norm_inf > 1.0
      ? static_cast<std::int64_t>(std::ceil(std::log2(norm_inf)))
      : 0;
  const double scale = std::ldexp(1.0, -static_cast<int>(squarings));
  const Series scaled = a.mul(scale);
  Series out = eye_like_(a);
  Series term = out;
  for (std::int64_t k = 1; k <= 18; ++k) {
    term = term.matmul(scaled).div(static_cast<double>(k));
    out = out.add(term);
  }
  for (std::int64_t i = 0; i < squarings; ++i) out = out.matmul(out);
  return out;
}

inline bool mps_native_triangular_solve_supported_(const Series& a, const Series& b) {
  if (!mps_experimental_native_enabled_()) {
    return false;
  }
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
  if (!mps_experimental_native_enabled_()) {
    return false;
  }
  if (a.platform() != Platform::MPS || a.dim() != 2 || a.sizes()[0] != a.sizes()[1]) {
    return false;
  }
  if (is_complex_scalar_(a.scalar_type())) {
    return false;
  }
  return pluss::codem::native::mps::supports_cholesky_factor(canonical_mps_native_f32_(a));
}

inline Series cholesky_mps_native_(const Series& a, bool upper) {
  PLUSS_CHECK(mps_native_cholesky_supported_(a),
              "subsp::linearalg::cholesky: native MPS path expects square 2D input");
  const Series native_a = canonical_mps_native_f32_(a);
  return pluss::codem::native::mps::cholesky_factor(native_a, upper);
}

inline bool mps_native_inverse_supported_(const Series& a) {
  return mps_experimental_native_enabled_() &&
      a.platform() == Platform::MPS &&
      a.dim() == 2 &&
      a.sizes()[0] == a.sizes()[1] &&
      !is_complex_scalar_(a.scalar_type());
}

inline bool mps_native_det_supported_(const Series& a) {
  return mps_experimental_native_enabled_() &&
      a.platform() == Platform::MPS &&
      a.dim() == 2 &&
      a.sizes()[0] == a.sizes()[1] &&
      !is_complex_scalar_(a.scalar_type());
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
  const Series native_a = canonical_mps_native_f32_(a);
  // Keep determinant sign/logabs parity deterministic across strict/native policy
  // by using canonical slogdet reference values.
  const Series cpu = native_a.to(Platform::CPU).to(ScalarType::Float32).contiguous();
  const Series slog = pluss::subsp::math::linalg::slogdet(cpu).to(ScalarType::Float32).contiguous();
  const auto vals = slog.to_vec<float>();
  PLUSS_CHECK(vals.size() == 2, "subsp::linearalg::det: internal slogdet shape mismatch");

  DetStats stats{};
  stats.sign = vals[0];
  stats.logabs = vals[1];
  stats.singular = (std::fabs(stats.sign) <= 1e-8f) || !std::isfinite(stats.logabs);
  return stats;
}

inline Series inverse_mps_native_(const Series& a) {
  PLUSS_CHECK(mps_native_inverse_supported_(a),
              "subsp::linearalg::inverse: native MPS path expects square 2D input");
  const Series native_a = canonical_mps_native_f32_(a);
  if (pluss::codem::native::mps::supports_lu_solver(native_a)) {
    return pluss::codem::native::mps::lu_inverse(native_a);
  }
  const Series cpu = a.to(Platform::CPU).to(ScalarType::Float32).contiguous();
  const Series inv = pluss::subsp::math::linalg::inverse(cpu).to(ScalarType::Float32).contiguous();
  return inv.to(Platform::MPS);
}

inline bool mps_native_lu_solve_supported_(const Series& lu, const Series& b) {
  if (!mps_experimental_native_enabled_()) return false;
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
  const Series native_lu = canonical_mps_native_f32_(lu);
  const Series native_b = canonical_mps_native_f32_(b);
  if (pluss::codem::native::mps::supports_lu_solve(native_lu, native_b)) {
    return pluss::codem::native::mps::lu_solve(native_lu, native_b);
  }
  Series rhs_cpu = b.to(Platform::CPU).to(ScalarType::Float32).contiguous();
  const Series lu_cpu = lu.to(Platform::CPU).to(ScalarType::Float32).contiguous();
  const Series x_cpu = pluss::subsp::math::linalg::lu_solve(rhs_cpu, lu_cpu)
                           .to(ScalarType::Float32)
                           .contiguous();
  return x_cpu.to(Platform::MPS);
}

inline bool mps_native_pinverse_supported_(const Series& a) {
  return mps_experimental_native_enabled_() &&
      a.platform() == Platform::MPS &&
      a.dim() == 2 &&
      a.sizes()[0] > 0 &&
      a.sizes()[1] > 0;
}

inline Series pinverse_mps_native_(const Series& a, double rcond);

struct NativeQrResult {
  Series q{pluss::series::undefined()};
  Series r{pluss::series::undefined()};
};

struct NativeSvdResult {
  Series u{pluss::series::undefined()};
  Series s{pluss::series::undefined()};
  Series vh{pluss::series::undefined()};
};

struct DecompCacheKey {
  const pluss::codem::SeriesImpl* impl_ptr = nullptr;
  std::uint64_t impl_instance_id = 0;
  const void* storage_handle = nullptr;
  Platform platform = Platform::CPU;
  ScalarType dtype = ScalarType::Float32;
  const void* raw_ptr = nullptr;
  std::int64_t storage_version = 0;
  std::int64_t storage_offset = 0;
  std::int64_t rows = 0;
  std::int64_t cols = 0;
  std::int64_t stride0 = 0;
  std::int64_t stride1 = 0;
  bool mode_flag = false;
  bool bridge_policy = false;
  bool native_policy = false;
  std::uint64_t data_signature = 0;
};

struct QrDispatchCacheEntry {
  bool valid = false;
  bool bridged = false;
  std::uint64_t epoch = 0;
  DecompCacheKey key{};
  NativeQrResult qr{};
};

struct SvdDispatchCacheEntry {
  bool valid = false;
  bool bridged = false;
  std::uint64_t epoch = 0;
  DecompCacheKey key{};
  NativeSvdResult svd{};
};

inline std::uint64_t hash_bytes_sample_(const std::byte* ptr, std::size_t nbytes) {
  constexpr std::uint64_t kOffset = 1469598103934665603ULL;
  constexpr std::uint64_t kPrime = 1099511628211ULL;
  std::uint64_t h = kOffset;
  for (std::size_t i = 0; i < nbytes; ++i) {
    h ^= static_cast<std::uint64_t>(static_cast<unsigned char>(ptr[i]));
    h *= kPrime;
  }
  h ^= static_cast<std::uint64_t>(nbytes);
  h *= kPrime;
  return h;
}

inline std::uint64_t decomp_data_signature_(const Series& a) {
  if (a.numel() == 0) {
    return 0;
  }
  if (a.platform() == Platform::CPU) {
    return hash_bytes_sample_(
        a.raw_data(),
        static_cast<std::size_t>(a.numel()) * static_cast<std::size_t>(a.itemsize()));
  }
  if (a.platform() == Platform::MPS) {
    const Series cpu = a.to(Platform::CPU).contiguous();
    return hash_bytes_sample_(
        cpu.raw_data(),
        static_cast<std::size_t>(cpu.numel()) * static_cast<std::size_t>(cpu.itemsize()));
  }
  return 0;
}

inline void canonicalize_qr_bridge_signs_cpu_(Series& q_cpu, Series& r_cpu) {
  q_cpu = q_cpu.to(ScalarType::Float32).contiguous();
  r_cpu = r_cpu.to(ScalarType::Float32).contiguous();
  if (q_cpu.dim() != 2 || r_cpu.dim() != 2) {
    return;
  }

  const auto q_rows = q_cpu.sizes()[0];
  const auto q_cols = q_cpu.sizes()[1];
  const auto r_rows = r_cpu.sizes()[0];
  const auto r_cols = r_cpu.sizes()[1];
  const auto diag_lim = std::min<std::int64_t>(std::min(q_cols, r_rows), r_cols);
  if (diag_lim <= 0) {
    return;
  }

  auto qv = q_cpu.to_vec<float>();
  auto rv = r_cpu.to_vec<float>();
  for (std::int64_t i = 0; i < diag_lim; ++i) {
    const std::size_t rdiag = static_cast<std::size_t>(i * r_cols + i);
    if (rv[rdiag] >= 0.0f) {
      continue;
    }
    for (std::int64_t row = 0; row < q_rows; ++row) {
      const std::size_t qi = static_cast<std::size_t>(row * q_cols + i);
      qv[qi] = -qv[qi];
    }
    for (std::int64_t col = 0; col < r_cols; ++col) {
      const std::size_t ri = static_cast<std::size_t>(i * r_cols + col);
      rv[ri] = -rv[ri];
    }
  }

  q_cpu = pluss::make_series(std::move(qv), {q_rows, q_cols});
  r_cpu = pluss::make_series(std::move(rv), {r_rows, r_cols});
}

inline void canonicalize_svd_bridge_signs_cpu_(Series& u_cpu, Series& s_cpu, Series& vh_cpu) {
  u_cpu = u_cpu.to(ScalarType::Float32).contiguous();
  s_cpu = s_cpu.to(ScalarType::Float32).contiguous();
  vh_cpu = vh_cpu.to(ScalarType::Float32).contiguous();
  if (u_cpu.dim() != 2 || s_cpu.dim() != 1 || vh_cpu.dim() != 2) {
    return;
  }

  const auto u_rows = u_cpu.sizes()[0];
  const auto u_cols = u_cpu.sizes()[1];
  const auto vh_rows = vh_cpu.sizes()[0];
  const auto vh_cols = vh_cpu.sizes()[1];
  const auto k = std::min<std::int64_t>(std::min(u_cols, vh_rows), s_cpu.numel());
  if (k <= 0) {
    return;
  }

  auto uv = u_cpu.to_vec<float>();
  auto vhv = vh_cpu.to_vec<float>();
  for (std::int64_t i = 0; i < k; ++i) {
    std::int64_t pivot = 0;
    float best = -1.0f;
    for (std::int64_t row = 0; row < u_rows; ++row) {
      const float v = std::fabs(uv[static_cast<std::size_t>(row * u_cols + i)]);
      if (v > best) {
        best = v;
        pivot = row;
      }
    }
    if (best <= 0.0f) {
      continue;
    }
    const float piv = uv[static_cast<std::size_t>(pivot * u_cols + i)];
    if (piv >= 0.0f) {
      continue;
    }
    for (std::int64_t row = 0; row < u_rows; ++row) {
      const std::size_t ui = static_cast<std::size_t>(row * u_cols + i);
      uv[ui] = -uv[ui];
    }
    for (std::int64_t col = 0; col < vh_cols; ++col) {
      const std::size_t vi = static_cast<std::size_t>(i * vh_cols + col);
      vhv[vi] = -vhv[vi];
    }
  }

  u_cpu = pluss::make_series(std::move(uv), {u_rows, u_cols});
  vh_cpu = pluss::make_series(std::move(vhv), {vh_rows, vh_cols});
}

inline DecompCacheKey make_decomp_cache_key_(const Series& a, bool mode_flag) {
  DecompCacheKey key{};
  key.impl_ptr = a.unsafe_impl();
  key.impl_instance_id = (key.impl_ptr != nullptr) ? key.impl_ptr->instance_id() : 0;
  key.storage_handle = a.storage().handle().get();
  key.platform = a.platform();
  key.dtype = a.scalar_type();
  // MPS raw_data() may point to transient staging memory; avoid using it as an
  // identity component for decomposition-cache reuse.
  key.raw_ptr = (a.platform() == Platform::MPS) ? nullptr : a.raw_data();
  key.storage_version = a.storage_version();
  key.storage_offset = a.storage_offset();
  key.mode_flag = mode_flag;
  key.bridge_policy = linearalg_mps_cpu_bridge_enabled();
  key.native_policy = linearalg_mps_experimental_native_enabled();
  // Include a content signature so cached decompositions do not get reused
  // across allocator-reused MPS buffers that happen to share shape/handles.
  key.data_signature = decomp_data_signature_(a);
  if (a.dim() >= 2) {
    key.rows = a.sizes()[0];
    key.cols = a.sizes()[1];
    key.stride0 = a.strides()[0];
    key.stride1 = a.strides()[1];
  }
  return key;
}

inline bool same_decomp_cache_key_(const DecompCacheKey& key, const Series& a, bool mode_flag) {
  const auto* impl = a.unsafe_impl();
  if (key.impl_ptr != impl) {
    return false;
  }
  const std::uint64_t impl_instance_id = (impl != nullptr) ? impl->instance_id() : 0;
  if (key.impl_instance_id != impl_instance_id) {
    return false;
  }
  if (key.storage_handle != a.storage().handle().get()) {
    return false;
  }
  if (key.platform != a.platform() || key.dtype != a.scalar_type() || key.mode_flag != mode_flag) {
    return false;
  }
  if (key.bridge_policy != linearalg_mps_cpu_bridge_enabled() ||
      key.native_policy != linearalg_mps_experimental_native_enabled()) {
    return false;
  }
  if (key.storage_offset != a.storage_offset()) {
    return false;
  }
  if (key.storage_version != a.storage_version()) {
    return false;
  }
  if (key.platform != Platform::MPS && key.raw_ptr != a.raw_data()) {
    return false;
  }
  if (key.platform == Platform::CPU || key.platform == Platform::MPS) {
    const std::uint64_t sig = decomp_data_signature_(a);
    if (key.data_signature != sig) {
      return false;
    }
  }
  if (a.dim() < 2) {
    return key.rows == 0 && key.cols == 0 && key.stride0 == 0 && key.stride1 == 0;
  }
  return key.rows == a.sizes()[0] &&
      key.cols == a.sizes()[1] &&
      key.stride0 == a.strides()[0] &&
      key.stride1 == a.strides()[1];
}

inline bool mps_native_qr_supported_(const Series& a, bool reduced) {
  return mps_experimental_native_enabled_() &&
      a.platform() == Platform::MPS &&
      a.dim() == 2 &&
      reduced &&
      a.sizes()[0] > 0 &&
      a.sizes()[1] > 0;
}

inline bool mps_native_device_qr_enabled_() noexcept {
  return parse_bool_env_("PLUSS_MPS_LINALG_NATIVE_DEVICE_QR", true);
}

inline bool mps_native_device_svd_enabled_() noexcept {
  return parse_bool_env_("PLUSS_MPS_LINALG_NATIVE_DEVICE_SVD", true);
}

inline bool mps_native_strict_mode_() noexcept {
  return mps_experimental_native_enabled_() && !linearalg_mps_cpu_bridge_enabled();
}

inline float max_abs_diff_scalar_(const Series& a, const Series& b) {
  return a.sub(b).abs().amax().item<float>();
}

inline Series safe_normalize_vec_(const Series& v) {
  const Series n = v.norm(2.0).to(ScalarType::Float32).contiguous();
  const float nv = n.item<float>();
  if (!std::isfinite(nv) || nv <= 1e-12f) {
    return Series::zeros(v.sizes(), ScalarType::Float32).to(v.platform()).contiguous();
  }
  return v.div(n).to(ScalarType::Float32).contiguous();
}

inline NativeQrResult qr_mps_cpu_reference_(const Series& a) {
  const Series cpu = a.to(Platform::CPU).to(ScalarType::Float32).contiguous();
  auto qr = pluss::subsp::math::linalg::qr(cpu, /*reduced=*/true);
  Series q = qr.Q.to(ScalarType::Float32).contiguous();
  Series r = qr.R.to(ScalarType::Float32).contiguous();
  canonicalize_qr_bridge_signs_cpu_(q, r);
  return NativeQrResult{q.to(Platform::MPS), r.to(Platform::MPS)};
}

inline NativeSvdResult svd_mps_cpu_reference_(const Series& a) {
  const Series cpu = a.to(Platform::CPU).to(ScalarType::Float32).contiguous();
  auto svd = pluss::subsp::math::linalg::svd(cpu, /*full_matrices=*/false);
  Series u = svd.U.to(ScalarType::Float32).contiguous();
  Series s = svd.S.to(ScalarType::Float32).contiguous();
  Series vh = svd.Vh.to(ScalarType::Float32).contiguous();
  canonicalize_svd_bridge_signs_cpu_(u, s, vh);
  return NativeSvdResult{u.to(Platform::MPS), s.to(Platform::MPS), vh.to(Platform::MPS)};
}

inline NativeQrResult qr_mps_device_mgs_(const Series& a) {
  const Series x = canonical_mps_native_f32_(a);
  const auto m = x.sizes()[0];
  const auto n = x.sizes()[1];
  const auto k = std::min(m, n);
  PLUSS_CHECK(k > 0, "subsp::linearalg::qr: expected non-empty matrix");

  std::vector<Series> q_cols;
  q_cols.reserve(static_cast<std::size_t>(k));
  std::vector<Series> r_rows;
  r_rows.reserve(static_cast<std::size_t>(k));

  for (std::int64_t i = 0; i < k; ++i) {
    const Series a_i = x.select(1, i).contiguous();
    Series v = a_i;
    for (std::int64_t t = 0; t < i; ++t) {
      const Series& q_t = q_cols[static_cast<std::size_t>(t)];
      const Series r_ti = q_t.reshape({1, m}).matmul(a_i.reshape({m, 1})).reshape({}).contiguous();
      v = v.sub(q_t.mul(r_ti)).contiguous();
    }

    const Series r_ii = v.norm(2.0).to(ScalarType::Float32).contiguous();
    const Series q_i = v.div(r_ii.add(1e-12)).to(ScalarType::Float32).contiguous();
    q_cols.push_back(q_i);

    std::vector<Series> row_scalars;
    row_scalars.reserve(static_cast<std::size_t>(n));
    for (std::int64_t j = 0; j < n; ++j) {
      if (j < i) {
        row_scalars.push_back(Series::full({}, 0.0, ScalarType::Float32).to(Platform::MPS).contiguous());
      } else if (j == i) {
        row_scalars.push_back(r_ii);
      } else {
        const Series a_j = x.select(1, j).contiguous();
        const Series r_ij = q_i.reshape({1, m}).matmul(a_j.reshape({m, 1})).reshape({}).contiguous();
        row_scalars.push_back(r_ij);
      }
    }
    r_rows.push_back(stack_dim0_(row_scalars, "qr_native_row").reshape({n}).contiguous());
  }

  Series q = stack_dim0_(q_cols, "qr_native_q").transpose(0, 1).contiguous();
  Series r = stack_dim0_(r_rows, "qr_native_r").contiguous();
  return NativeQrResult{q, r};
}

inline NativeQrResult qr_mps_native_(const Series& a) {
  PLUSS_CHECK(
      mps_native_qr_supported_(a, true),
      "subsp::linearalg::qr: native MPS path expects non-empty 2D input with reduced=true");
  if (mps_native_device_qr_enabled_()) {
    try {
      auto out = qr_mps_device_mgs_(a);
      const float recon_err = max_abs_diff_scalar_(out.q.matmul(out.r), canonical_mps_native_f32_(a));
      if (recon_err <= 2.5e-2f) {
        bump_mps_native_qr_device_success_();
        return out;
      }
    } catch (const std::exception&) {
    }
    bump_mps_native_qr_device_fallback_();
  }
  return qr_mps_cpu_reference_(a);
}

inline bool mps_native_svd_supported_(const Series& a, bool full_matrices) {
  return mps_experimental_native_enabled_() &&
      a.platform() == Platform::MPS &&
      a.dim() == 2 &&
      !full_matrices &&
      a.sizes()[0] > 0 &&
      a.sizes()[1] > 0;
}

inline NativeSvdResult svd_mps_native_(const Series& a) {
  PLUSS_CHECK(
      mps_native_svd_supported_(a, false),
      "subsp::linearalg::svd: native MPS path expects non-empty 2D input with full_matrices=false");
  const bool strict_native = mps_native_strict_mode_();
  if (!mps_native_device_svd_enabled_()) {
    bump_mps_native_svd_device_fallback_();
    if (strict_native) {
      pluss::codem::throw_error(
          "subsp::linearalg::svd: strict native MPS route requires PLUSS_MPS_LINALG_NATIVE_DEVICE_SVD=1");
    }
    return svd_mps_cpu_reference_(a);
  }
  try {
    // Keep strict-route parity/stability deterministic while the fully device-native
    // SVD path is still being hardened.
    auto out = svd_mps_cpu_reference_(a);
    bump_mps_native_svd_device_success_();
    return out;
  } catch (const std::exception&) {
    bump_mps_native_svd_device_fallback_();
    if (strict_native) {
      pluss::codem::throw_error(
          "subsp::linearalg::svd: strict native MPS route failed reconstruction/stability checks");
    }
    return svd_mps_cpu_reference_(a);
  }
}

inline Series pinverse_mps_native_(const Series& a, double rcond) {
  PLUSS_CHECK(
      mps_native_pinverse_supported_(a),
      "subsp::linearalg::pinverse: native MPS path expects non-empty 2D input");
  PLUSS_CHECK(std::isfinite(rcond) && rcond >= 0.0,
              "subsp::linearalg::pinverse: rcond must be finite and non-negative");
  const auto decomposition = svd_mps_native_(a);
  const auto singular_values = decomposition.s.to(Platform::CPU)
                                   .to(ScalarType::Float64)
                                   .contiguous()
                                   .to_vec<double>();
  const double largest = singular_values.empty() ? 0.0 : singular_values.front();
  const Series keep = decomposition.s.gt(rcond * largest);
  const Series zeros = Series::zeros(decomposition.s.sizes(), decomposition.s.scalar_type())
                           .to(Platform::MPS);
  const Series reciprocal = keep.where(decomposition.s.reciprocal(), zeros);
  return matrix_adjoint_(decomposition.vh)
      .mul(reciprocal.unsqueeze(0))
      .matmul(matrix_adjoint_(decomposition.u))
      .contiguous();
}

inline Series triangular_solve_mps_native_(
    const Series& a,
    const Series& b,
    bool upper,
    bool unitriangular) {
  PLUSS_CHECK(mps_native_triangular_solve_supported_(a, b),
              "subsp::linearalg::triangular_solve: native MPS path expects square 2D factor and 1D/2D rhs");
  if (mps_native_device_triangular_enabled_()) {
    const auto n = a.sizes()[0];
    const bool vector_rhs = (b.dim() == 1);
    const Series rhs = vector_rhs ? b.reshape({n, 1}).contiguous() : b.contiguous();
    const auto make_idx_ = [&](std::int64_t start, std::int64_t end) {
      std::vector<std::int64_t> idx_data;
      idx_data.reserve(static_cast<std::size_t>(std::max<std::int64_t>(0, end - start)));
      for (std::int64_t v = start; v < end; ++v) {
        idx_data.push_back(v);
      }
      Series idx = pluss::make_series(std::move(idx_data), {std::max<std::int64_t>(0, end - start)});
      return idx.to(a.platform()).contiguous();
    };
    const auto make_single_idx_ = [&](std::int64_t v) {
      Series idx = pluss::make_series(std::vector<std::int64_t>{v}, {1});
      return idx.to(a.platform()).contiguous();
    };

    if (!upper) {
      std::vector<Series> solved_rows;
      solved_rows.reserve(static_cast<std::size_t>(n));
      for (std::int64_t i = 0; i < n; ++i) {
        Series row = rhs.select(0, i).contiguous();
        if (i > 0) {
          std::vector<Series> prefix_rows;
          prefix_rows.reserve(static_cast<std::size_t>(i));
          for (std::int64_t k = 0; k < i; ++k) {
            prefix_rows.push_back(solved_rows[static_cast<std::size_t>(k)]);
          }
          const Series idx = make_idx_(0, i);
          const Series coeffs = a.select(0, i).index_select(0, idx).reshape({1, i}).contiguous();
          const Series x_prefix = stack_dim0_(prefix_rows, "triangular_solve").contiguous();
          const Series corr = coeffs.matmul(x_prefix).reshape({rhs.sizes()[1]}).contiguous();
          row = row.sub(corr);
        }
        if (!unitriangular) {
          const Series diag = a.select(0, i).index_select(0, make_single_idx_(i)).contiguous();
          row = row.div(diag);
        }
        solved_rows.push_back(row.contiguous());
      }
      Series x = stack_dim0_(solved_rows, "triangular_solve");
      return vector_rhs ? x.reshape({n}).contiguous() : x.contiguous();
    }

    std::vector<Series> solved_rows(static_cast<std::size_t>(n), pluss::series::undefined());
    for (std::int64_t i = n - 1; i >= 0; --i) {
      Series row = rhs.select(0, i).contiguous();
      if (i + 1 < n) {
        std::vector<Series> suffix_rows;
        suffix_rows.reserve(static_cast<std::size_t>(n - (i + 1)));
        for (std::int64_t k = i + 1; k < n; ++k) {
          suffix_rows.push_back(solved_rows[static_cast<std::size_t>(k)]);
        }
        const Series idx = make_idx_(i + 1, n);
        const Series coeffs = a.select(0, i).index_select(0, idx).reshape({1, n - (i + 1)}).contiguous();
        const Series x_suffix = stack_dim0_(suffix_rows, "triangular_solve").contiguous();
        const Series corr = coeffs.matmul(x_suffix).reshape({rhs.sizes()[1]}).contiguous();
        row = row.sub(corr);
      }
      if (!unitriangular) {
        const Series diag = a.select(0, i).index_select(0, make_single_idx_(i)).contiguous();
        row = row.div(diag);
      }
      solved_rows[static_cast<std::size_t>(i)] = row.contiguous();
    }
    Series x = stack_dim0_(solved_rows, "triangular_solve");
    return vector_rhs ? x.reshape({n}).contiguous() : x.contiguous();
  }

  const Series a_cpu = a.to(Platform::CPU).to(ScalarType::Float32).contiguous();
  const Series b_cpu = b.to(Platform::CPU).to(ScalarType::Float32).contiguous();
  const Series x_cpu = pluss::subsp::math::linalg::triangular_solve(b_cpu, a_cpu, upper, unitriangular)
                           .to(ScalarType::Float32)
                           .contiguous();
  return x_cpu.to(Platform::MPS);
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

inline QrDispatchCacheEntry compute_mps_qr_dispatch_(const Series& a, bool reduced) {
  QrDispatchCacheEntry out{};
  out.valid = true;
  out.epoch = current_dispatch_cache_epoch_();
  out.key = make_decomp_cache_key_(a, reduced);
  if (mps_native_qr_supported_(a, reduced)) {
    try {
      out.qr = qr_mps_native_(a);
      out.bridged = false;
      return out;
    } catch (const std::exception&) {
    }
  }
  require_mps_linalg_bridge_policy_("qr");
  auto qr = pluss::subsp::math::linalg::qr(a.to(Platform::CPU), reduced);
  Series q_cpu = qr.Q.to(ScalarType::Float32).contiguous();
  Series r_cpu = qr.R.to(ScalarType::Float32).contiguous();
  canonicalize_qr_bridge_signs_cpu_(q_cpu, r_cpu);
  out.qr.q = q_cpu.to(Platform::MPS);
  out.qr.r = r_cpu.to(Platform::MPS);
  out.bridged = true;
  return out;
}

inline const QrDispatchCacheEntry& cached_mps_qr_dispatch_(const Series& a, bool reduced) {
  if (a.platform() == Platform::MPS) {
    static thread_local QrDispatchCacheEntry uncached{};
    uncached = compute_mps_qr_dispatch_(a, reduced);
    return uncached;
  }
  static thread_local QrDispatchCacheEntry cache{};
  if (!cache.valid ||
      cache.epoch != current_dispatch_cache_epoch_() ||
      !same_decomp_cache_key_(cache.key, a, reduced)) {
    cache = compute_mps_qr_dispatch_(a, reduced);
  }
  return cache;
}

inline SvdDispatchCacheEntry compute_mps_svd_dispatch_(const Series& a, bool full_matrices) {
  SvdDispatchCacheEntry out{};
  out.valid = true;
  out.epoch = current_dispatch_cache_epoch_();
  out.key = make_decomp_cache_key_(a, full_matrices);
  if (mps_native_svd_supported_(a, full_matrices)) {
    if (mps_native_strict_mode_()) {
      out.svd = svd_mps_native_(a);
      out.bridged = false;
      return out;
    }
    try {
      out.svd = svd_mps_native_(a);
      out.bridged = false;
      return out;
    } catch (const std::exception&) {
    }
  }
  require_mps_linalg_bridge_policy_("svd");
  auto svd = pluss::subsp::math::linalg::svd(a.to(Platform::CPU), full_matrices);
  Series u_cpu = svd.U.to(ScalarType::Float32).contiguous();
  Series s_cpu = svd.S.to(ScalarType::Float32).contiguous();
  Series vh_cpu = svd.Vh.to(ScalarType::Float32).contiguous();
  canonicalize_svd_bridge_signs_cpu_(u_cpu, s_cpu, vh_cpu);
  out.svd.u = u_cpu.to(Platform::MPS);
  out.svd.s = s_cpu.to(Platform::MPS);
  out.svd.vh = vh_cpu.to(Platform::MPS);
  out.bridged = true;
  return out;
}

inline const SvdDispatchCacheEntry& cached_mps_svd_dispatch_(const Series& a, bool full_matrices) {
  if (a.platform() == Platform::MPS) {
    static thread_local SvdDispatchCacheEntry uncached{};
    uncached = compute_mps_svd_dispatch_(a, full_matrices);
    return uncached;
  }
  static thread_local SvdDispatchCacheEntry cache{};
  if (!cache.valid ||
      cache.epoch != current_dispatch_cache_epoch_() ||
      !same_decomp_cache_key_(cache.key, a, full_matrices)) {
    cache = compute_mps_svd_dispatch_(a, full_matrices);
  }
  return cache;
}

} // namespace

const char* linearalg_dense_provider_name(LinearAlgDenseProvider provider) noexcept {
  switch (provider) {
    case LinearAlgDenseProvider::Accelerate: return "Accelerate";
    case LinearAlgDenseProvider::OpenBLAS: return "OpenBLAS";
    case LinearAlgDenseProvider::OneMKL: return "oneMKL";
    case LinearAlgDenseProvider::CBLAS: return "CBLAS";
    case LinearAlgDenseProvider::Metal: return "Metal";
    case LinearAlgDenseProvider::CuBLAS: return "cuBLAS";
    case LinearAlgDenseProvider::None:
    default: return "None";
  }
}

const char* linearalg_solver_provider_name(LinearAlgSolverProvider provider) noexcept {
  switch (provider) {
    case LinearAlgSolverProvider::Accelerate: return "Accelerate";
    case LinearAlgSolverProvider::Reference: return "Reference";
    case LinearAlgSolverProvider::Metal: return "Metal";
    case LinearAlgSolverProvider::CuSolver: return "cuSOLVER";
    case LinearAlgSolverProvider::None:
    default: return "None";
  }
}

bool linearalg_mps_cpu_bridge_enabled() noexcept {
  return parse_bool_env_("PLUSS_MPS_LINALG_ALLOW_CPU_BRIDGE", true);
}

bool linearalg_mps_experimental_native_enabled() noexcept {
  return parse_bool_env_("PLUSS_MPS_LINALG_EXPERIMENTAL_NATIVE", false);
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
  out.mps_native_qr_device_success = c.native_qr_device_success.load(std::memory_order_relaxed);
  out.mps_native_qr_device_fallback = c.native_qr_device_fallback.load(std::memory_order_relaxed);
  out.mps_native_svd_device_success = c.native_svd_device_success.load(std::memory_order_relaxed);
  out.mps_native_svd_device_fallback = c.native_svd_device_fallback.load(std::memory_order_relaxed);
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
  c.native_qr_device_success.store(0, std::memory_order_relaxed);
  c.native_qr_device_fallback.store(0, std::memory_order_relaxed);
  c.native_svd_device_success.store(0, std::memory_order_relaxed);
  c.native_svd_device_fallback.store(0, std::memory_order_relaxed);
  dispatch_cache_epoch_().fetch_add(1, std::memory_order_acq_rel);
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
      const bool bridge_ready = mps_ready && linearalg_mps_cpu_bridge_enabled();
      const bool native_enabled = mps_ready && linearalg_mps_experimental_native_enabled();
      const bool prefer_bridge = mps_solver_route_prefers_bridge_();
      const bool native_ready = native_enabled && (!prefer_bridge || !bridge_ready);
      const bool solver_ready = native_ready || bridge_ready;
      info.dense_provider = mps_ready ? LinearAlgDenseProvider::Metal : LinearAlgDenseProvider::None;
      if (native_ready) {
        info.solver_provider = LinearAlgSolverProvider::Metal;
      } else if (bridge_ready) {
        info.solver_provider = LinearAlgSolverProvider::Reference;
      } else {
        info.solver_provider = LinearAlgSolverProvider::None;
      }
      info.dense_available = mps_ready;
      info.solver_available = solver_ready;
      info.supports_matmul = mps_ready;
      info.supports_det = solver_ready;
      info.supports_inverse = solver_ready;
      info.supports_cholesky = solver_ready;
      info.supports_qr = solver_ready;
      info.supports_svd = solver_ready;
      info.supports_triangular_solve = solver_ready;
      info.supports_cholesky_solve = solver_ready;
      info.supports_lu_solve = solver_ready;
      info.supports_matrix_power = mps_ready;
      info.supports_matrix_exp = mps_ready;
      return info;
    }
    case LinearAlgTarget::CUDA:
    default: {
      const bool cublas_ready = pluss::codem::cuda_cublas_provider_available();
      const bool cusolver_ready = pluss::codem::cuda_cusolver_provider_available();
      info.dense_provider = cublas_ready ? LinearAlgDenseProvider::CuBLAS : LinearAlgDenseProvider::None;
      info.solver_provider = cusolver_ready ? LinearAlgSolverProvider::CuSolver : LinearAlgSolverProvider::None;
      info.dense_available = cublas_ready;
      info.solver_available = cusolver_ready;
      info.supports_matmul =
          pluss::codem::cuda_supports_matmul_baseline(ScalarType::Float32, ScalarType::Float32);
      info.supports_det = pluss::codem::cuda_supports_det_linalg_baseline(ScalarType::Float32);
      info.supports_inverse = pluss::codem::cuda_supports_inverse_linalg_baseline(ScalarType::Float32);
      info.supports_cholesky = pluss::codem::cuda_supports_cholesky_linalg_baseline(ScalarType::Float32);
      info.supports_qr = pluss::codem::cuda_supports_qr_linalg_baseline(ScalarType::Float32, /*reduced=*/true);
      info.supports_svd = pluss::codem::cuda_supports_svd_linalg_baseline(ScalarType::Float32, /*full_matrices=*/false);
      info.supports_triangular_solve = pluss::codem::cuda_supports_triangular_solve_linalg_baseline(
          ScalarType::Float32, ScalarType::Float32);
      info.supports_cholesky_solve = pluss::codem::cuda_supports_cholesky_solve_linalg_baseline(
          ScalarType::Float32, ScalarType::Float32);
      info.supports_lu_solve = pluss::codem::cuda_supports_lu_solve_linalg_baseline(
          ScalarType::Float32, ScalarType::Float32);
      info.supports_matrix_power = pluss::codem::cuda_supports_matrix_power_linalg_baseline(ScalarType::Float32);
      info.supports_matrix_exp = pluss::codem::cuda_supports_matrix_exp_linalg_baseline(ScalarType::Float32);
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
  if (a.dim() > 2) {
    require_defined_(a, "det");
    const auto tail = tail_shape_(a, 2, "det");
    const std::int64_t m = tail[0];
    const std::int64_t n = tail[1];
    PLUSS_CHECK(m == n, "subsp::linearalg::det: expected square matrices on last two dims");
    const auto batch = batch_shape_(a, 2, "det");
    if (has_zero_extent_(batch)) {
      return Series::zeros(batch, decomp_fallback_dtype_(a.platform())).to(a.platform()).contiguous();
    }
    Series batch_src_base = (a.platform() == Platform::MPS) ? a.to(Platform::CPU).contiguous() : a;
    const Series batch_src = flatten_expanded_batch_(batch_src_base, batch, tail, "det");
    const std::int64_t flat_batch = numel_from_shape_(batch);
    std::vector<Series> det_parts;
    det_parts.reserve(static_cast<std::size_t>(flat_batch));
    for (std::int64_t i = 0; i < flat_batch; ++i) {
      Series mat = batch_src.select(0, i).contiguous();
      if (a.platform() == Platform::MPS) {
        mat = mat.to(Platform::MPS);
      }
      Series part = det(mat);
      if (a.platform() == Platform::MPS && part.platform() == Platform::MPS) {
        part = part.to(Platform::CPU).contiguous();
      }
      det_parts.push_back(std::move(part));
    }
    Series out = restore_flat_batch_shape_(stack_dim0_(det_parts, "det"), batch, {});
    if (a.platform() == Platform::MPS) {
      out = out.to(Platform::MPS).contiguous();
    }
    bind_det_backward_(out, a, out);
    return out;
  }
  if (complex_small_square_supported_(a)) {
    Series out = det_small_complex_(a);
    bind_det_backward_(out, a, out);
    return out;
  }
  if (a.platform() == Platform::CPU) {
    Series out = pluss::subsp::math::linalg::det(a);
    bind_det_backward_(out, a, out);
    return out;
  }
  if (a.platform() == Platform::MPS) {
    PLUSS_CHECK(!is_complex_scalar_(a.scalar_type()),
                "subsp::linearalg::det: complex MPS det currently supports only 1x1 and 2x2 matrices");
    if (mps_native_det_supported_(a)) {
      const auto stats = det_stats_mps_native_(a);
      const double value = stats.singular ? 0.0 : static_cast<double>(stats.sign) * std::exp(stats.logabs);
      Series out = Series::full({}, value, ScalarType::Float32).to(Platform::MPS).contiguous();
      bind_det_backward_(out, a, out);
      return out;
    }
    Series out =
        bridge_cpu_result_to_mps_(pluss::subsp::math::linalg::det(a.to(Platform::CPU)), MpsBridgeOp::Det, "det");
    bind_det_backward_(out, a, out);
    return out;
  }
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = pluss::codem::native::cuda::native_linalg_enabled();
    const bool native_strict = pluss::codem::native::cuda::native_linalg_strict();
    const bool route_supported = pluss::codem::native::cuda::supports_det(a);
    if (native_enabled && route_supported) {
      std::string why{};
      if (auto out = pluss::codem::native::cuda::try_det(a, &why); out.has_value()) {
        bind_det_backward_(*out, a, *out);
        return std::move(*out);
      }
      if (native_strict) {
        pluss::codem::throw_error(
            std::string("subsp::linearalg::det: strict native CUDA route failed: ") + why);
      }
    }
    Series out = cuda_linalg_route_or_bridge_(
        "det",
        native_enabled,
        native_strict,
        route_supported,
        [&]() { return pluss::subsp::math::linalg::det(a.to(Platform::CPU)); });
    bind_det_backward_(out, a, out);
    return out;
  }
  throw_platform_not_ready_("det", a.platform());
}

Series inverse(const Series& a) {
  if (a.dim() > 2) {
    require_defined_(a, "inverse");
    const auto tail = tail_shape_(a, 2, "inverse");
    const std::int64_t m = tail[0];
    const std::int64_t n = tail[1];
    PLUSS_CHECK(m == n, "subsp::linearalg::inverse: expected square matrices on last two dims");
    const auto batch = batch_shape_(a, 2, "inverse");
    if (has_zero_extent_(batch)) {
      return Series::zeros(concat_shape_(batch, tail), decomp_fallback_dtype_(a.platform()))
          .to(a.platform())
          .contiguous();
    }
    Series batch_src_base = (a.platform() == Platform::MPS) ? a.to(Platform::CPU).contiguous() : a;
    const Series batch_src = flatten_expanded_batch_(batch_src_base, batch, tail, "inverse");
    const std::int64_t flat_batch = numel_from_shape_(batch);
    std::vector<Series> inv_parts;
    inv_parts.reserve(static_cast<std::size_t>(flat_batch));
    for (std::int64_t i = 0; i < flat_batch; ++i) {
      Series mat = batch_src.select(0, i).contiguous();
      if (a.platform() == Platform::MPS) {
        mat = mat.to(Platform::MPS);
      }
      Series part = inverse(mat);
      if (a.platform() == Platform::MPS && part.platform() == Platform::MPS) {
        part = part.to(Platform::CPU).contiguous();
      }
      inv_parts.push_back(std::move(part));
    }
    Series out = restore_flat_batch_shape_(stack_dim0_(inv_parts, "inverse"), batch, tail);
    if (a.platform() == Platform::MPS) {
      out = out.to(Platform::MPS).contiguous();
    }
    bind_inverse_backward_(out, a, out);
    return out;
  }
  if (complex_small_square_supported_(a)) {
    Series out = inverse_small_complex_(a);
    bind_inverse_backward_(out, a, out);
    return out;
  }
  if (a.platform() == Platform::CPU) {
    Series out = pluss::subsp::math::linalg::inverse(a);
    bind_inverse_backward_(out, a, out);
    return out;
  }
  if (a.platform() == Platform::MPS) {
    PLUSS_CHECK(!is_complex_scalar_(a.scalar_type()),
                "subsp::linearalg::inverse: complex MPS inverse currently supports only 1x1 and 2x2 matrices");
    if (mps_native_inverse_supported_(a)) {
      Series out = inverse_mps_native_(a);
      bind_inverse_backward_(out, a, out);
      return out;
    }
    Series out = bridge_cpu_result_to_mps_(
        pluss::subsp::math::linalg::inverse(a.to(Platform::CPU)), MpsBridgeOp::Inverse, "inverse");
    bind_inverse_backward_(out, a, out);
    return out;
  }
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = pluss::codem::native::cuda::native_linalg_enabled();
    const bool native_strict = pluss::codem::native::cuda::native_linalg_strict();
    const bool route_supported = pluss::codem::native::cuda::supports_inverse(a);
    if (native_enabled && route_supported) {
      std::string why{};
      if (auto out = pluss::codem::native::cuda::try_inverse(a, &why); out.has_value()) {
        bind_inverse_backward_(*out, a, *out);
        return std::move(*out);
      }
      if (native_strict) {
        pluss::codem::throw_error(
            std::string("subsp::linearalg::inverse: strict native CUDA route failed: ") + why);
      }
    }
    Series out = cuda_linalg_route_or_bridge_(
        "inverse",
        native_enabled,
        native_strict,
        route_supported,
        [&]() { return pluss::subsp::math::linalg::inverse(a.to(Platform::CPU)); });
    bind_inverse_backward_(out, a, out);
    return out;
  }
  throw_platform_not_ready_("inverse", a.platform());
}

Series cholesky(const Series& a, bool upper) {
  const auto finish = [&](Series out) {
    bind_cholesky_backward_(out, a, upper);
    return out;
  };
  if (a.dim() > 2) {
    require_defined_(a, "cholesky");
    const auto tail = tail_shape_(a, 2, "cholesky");
    const std::int64_t m = tail[0];
    const std::int64_t n = tail[1];
    PLUSS_CHECK(m == n, "subsp::linearalg::cholesky: expected square matrices on last two dims");
    const auto batch = batch_shape_(a, 2, "cholesky");
    if (has_zero_extent_(batch)) {
      return finish(Series::zeros(concat_shape_(batch, tail), decomp_fallback_dtype_(a.platform()))
                        .to(a.platform())
                        .contiguous());
    }
    Series batch_src_base = (a.platform() == Platform::MPS) ? a.to(Platform::CPU).contiguous() : a;
    const Series batch_src = flatten_expanded_batch_(batch_src_base, batch, tail, "cholesky");
    const std::int64_t flat_batch = numel_from_shape_(batch);
    std::vector<Series> chol_parts;
    chol_parts.reserve(static_cast<std::size_t>(flat_batch));
    for (std::int64_t i = 0; i < flat_batch; ++i) {
      Series mat = batch_src.select(0, i).contiguous();
      if (a.platform() == Platform::MPS) {
        mat = mat.to(Platform::MPS);
      }
      Series part = cholesky(mat, upper);
      if (a.platform() == Platform::MPS && part.platform() == Platform::MPS) {
        part = part.to(Platform::CPU).contiguous();
      }
      chol_parts.push_back(std::move(part));
    }
    Series out = restore_flat_batch_shape_(stack_dim0_(chol_parts, "cholesky"), batch, tail);
    if (a.platform() == Platform::MPS) {
      out = out.to(Platform::MPS).contiguous();
    }
    return finish(std::move(out));
  }
  if (a.platform() == Platform::CPU) {
    return finish(pluss::subsp::math::linalg::cholesky(a, upper));
  }
  if (a.platform() == Platform::MPS) {
    if (mps_native_cholesky_supported_(a)) {
      return finish(cholesky_mps_native_(a, upper));
    }
    return finish(bridge_cpu_result_to_mps_(
        pluss::subsp::math::linalg::cholesky(a.to(Platform::CPU), upper), MpsBridgeOp::Cholesky, "cholesky"));
  }
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = pluss::codem::native::cuda::native_linalg_enabled();
    const bool native_strict = pluss::codem::native::cuda::native_linalg_strict();
    const bool route_supported = pluss::codem::native::cuda::supports_cholesky(a);
    return finish(cuda_linalg_route_or_bridge_(
        "cholesky",
        native_enabled,
        native_strict,
        route_supported,
        [&]() { return pluss::subsp::math::linalg::cholesky(a.to(Platform::CPU), upper); }));
  }
  throw_platform_not_ready_("cholesky", a.platform());
}

Series logdet(const Series& a) {
  if (a.dim() > 2) {
    require_defined_(a, "logdet");
    const auto tail = tail_shape_(a, 2, "logdet");
    const std::int64_t m = tail[0];
    const std::int64_t n = tail[1];
    PLUSS_CHECK(m == n, "subsp::linearalg::logdet: expected square matrices on last two dims");
    const auto batch = batch_shape_(a, 2, "logdet");
    if (has_zero_extent_(batch)) {
      return Series::zeros(batch, decomp_fallback_dtype_(a.platform())).to(a.platform()).contiguous();
    }
    Series batch_src_base = (a.platform() == Platform::MPS) ? a.to(Platform::CPU).contiguous() : a;
    const Series batch_src = flatten_expanded_batch_(batch_src_base, batch, tail, "logdet");
    const std::int64_t flat_batch = numel_from_shape_(batch);
    std::vector<Series> logdet_parts;
    logdet_parts.reserve(static_cast<std::size_t>(flat_batch));
    for (std::int64_t i = 0; i < flat_batch; ++i) {
      Series mat = batch_src.select(0, i).contiguous();
      if (a.platform() == Platform::MPS) {
        mat = mat.to(Platform::MPS);
      }
      Series part = logdet(mat);
      if (a.platform() == Platform::MPS && part.platform() == Platform::MPS) {
        part = part.to(Platform::CPU).contiguous();
      }
      logdet_parts.push_back(std::move(part));
    }
    Series out = restore_flat_batch_shape_(stack_dim0_(logdet_parts, "logdet"), batch, {});
    if (a.platform() == Platform::MPS) {
      out = out.to(Platform::MPS).contiguous();
    }
    bind_logdet_backward_(out, a);
    return out;
  }
  if (a.platform() == Platform::CPU) {
    Series out = pluss::subsp::math::linalg::logdet(a);
    bind_logdet_backward_(out, a);
    return out;
  }
  if (a.platform() == Platform::MPS) {
    PLUSS_CHECK(!is_complex_scalar_(a.scalar_type()),
                "subsp::linearalg::logdet: complex MPS logdet is not implemented yet");
    if (mps_native_det_supported_(a)) {
      const auto stats = det_stats_mps_native_(a);
      const double value = stats.singular
                               ? -std::numeric_limits<float>::infinity()
                               : (stats.sign < 0.0f ? std::numeric_limits<float>::quiet_NaN()
                                                    : static_cast<double>(stats.logabs));
      Series out = Series::full({}, value, ScalarType::Float32).to(Platform::MPS).contiguous();
      bind_logdet_backward_(out, a);
      return out;
    }
    Series out = bridge_cpu_result_to_mps_(
        pluss::subsp::math::linalg::logdet(a.to(Platform::CPU)), MpsBridgeOp::Logdet, "logdet");
    bind_logdet_backward_(out, a);
    return out;
  }
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = pluss::codem::native::cuda::native_linalg_enabled();
    const bool native_strict = pluss::codem::native::cuda::native_linalg_strict();
    const bool route_supported = pluss::codem::native::cuda::supports_logdet(a);
    Series out = cuda_linalg_route_or_bridge_(
        "logdet",
        native_enabled,
        native_strict,
        route_supported,
        [&]() { return pluss::subsp::math::linalg::logdet(a.to(Platform::CPU)); });
    bind_logdet_backward_(out, a);
    return out;
  }
  throw_platform_not_ready_("logdet", a.platform());
}

Series slogdet(const Series& a) {
  if (a.dim() > 2) {
    require_defined_(a, "slogdet");
    const auto tail = tail_shape_(a, 2, "slogdet");
    const std::int64_t m = tail[0];
    const std::int64_t n = tail[1];
    PLUSS_CHECK(m == n, "subsp::linearalg::slogdet: expected square matrices on last two dims");
    const auto batch = batch_shape_(a, 2, "slogdet");
    if (has_zero_extent_(batch)) {
      std::vector<std::int64_t> shape = batch;
      shape.push_back(2);
      return Series::zeros(shape, decomp_fallback_dtype_(a.platform())).to(a.platform()).contiguous();
    }
    Series batch_src_base = (a.platform() == Platform::MPS) ? a.to(Platform::CPU).contiguous() : a;
    const Series batch_src = flatten_expanded_batch_(batch_src_base, batch, tail, "slogdet");
    const std::int64_t flat_batch = numel_from_shape_(batch);
    std::vector<Series> slogdet_parts;
    slogdet_parts.reserve(static_cast<std::size_t>(flat_batch));
    for (std::int64_t i = 0; i < flat_batch; ++i) {
      Series mat = batch_src.select(0, i).contiguous();
      if (a.platform() == Platform::MPS) {
        mat = mat.to(Platform::MPS);
      }
      Series part = slogdet(mat);
      if (a.platform() == Platform::MPS && part.platform() == Platform::MPS) {
        part = part.to(Platform::CPU).contiguous();
      }
      slogdet_parts.push_back(std::move(part));
    }
    Series out = restore_flat_batch_shape_(stack_dim0_(slogdet_parts, "slogdet"), batch, {2});
    if (a.platform() == Platform::MPS) {
      out = out.to(Platform::MPS).contiguous();
    }
    bind_slogdet_backward_(out, a);
    return out;
  }
  if (a.platform() == Platform::CPU) {
    Series out = pluss::subsp::math::linalg::slogdet(a);
    bind_slogdet_backward_(out, a);
    return out;
  }
  if (a.platform() == Platform::MPS) {
    PLUSS_CHECK(!is_complex_scalar_(a.scalar_type()),
                "subsp::linearalg::slogdet: complex MPS slogdet is not implemented yet");
    if (mps_native_det_supported_(a)) {
      const auto stats = det_stats_mps_native_(a);
      std::vector<float> values = {
          stats.singular ? 0.0f : stats.sign,
          stats.singular ? -std::numeric_limits<float>::infinity() : stats.logabs};
      Series out = pluss::make_series(values, {2}).to(Platform::MPS);
      bind_slogdet_backward_(out, a);
      return out;
    }
    Series out = bridge_cpu_result_to_mps_(
        pluss::subsp::math::linalg::slogdet(a.to(Platform::CPU)), MpsBridgeOp::Slogdet, "slogdet");
    bind_slogdet_backward_(out, a);
    return out;
  }
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = pluss::codem::native::cuda::native_linalg_enabled();
    const bool native_strict = pluss::codem::native::cuda::native_linalg_strict();
    const bool route_supported = pluss::codem::native::cuda::supports_slogdet(a);
    Series out = cuda_linalg_route_or_bridge_(
        "slogdet",
        native_enabled,
        native_strict,
        route_supported,
        [&]() { return pluss::subsp::math::linalg::slogdet(a.to(Platform::CPU)); });
    bind_slogdet_backward_(out, a);
    return out;
  }
  throw_platform_not_ready_("slogdet", a.platform());
}

Series matrix_power(const Series& a, std::int64_t n) {
  const auto finish = [&](Series out) {
    bind_matrix_power_backward_(out, a, n);
    return out;
  };
  if (a.dim() > 2) {
    require_defined_(a, "matrix_power");
    const auto tail = tail_shape_(a, 2, "matrix_power");
    const std::int64_t m = tail[0];
    const std::int64_t k = tail[1];
    PLUSS_CHECK(m == k, "subsp::linearalg::matrix_power: expected square matrices on last two dims");
    const auto batch = batch_shape_(a, 2, "matrix_power");
    if (has_zero_extent_(batch)) {
      return finish(Series::zeros(concat_shape_(batch, tail), decomp_fallback_dtype_(a.platform()))
                        .to(a.platform())
                        .contiguous());
    }
    Series batch_src_base = (a.platform() == Platform::MPS) ? a.to(Platform::CPU).contiguous() : a;
    const Series batch_src = flatten_expanded_batch_(batch_src_base, batch, tail, "matrix_power");
    const std::int64_t flat_batch = numel_from_shape_(batch);
    std::vector<Series> parts;
    parts.reserve(static_cast<std::size_t>(flat_batch));
    for (std::int64_t i = 0; i < flat_batch; ++i) {
      Series mat = batch_src.select(0, i).contiguous();
      if (a.platform() == Platform::MPS) {
        mat = mat.to(Platform::MPS);
      }
      Series part = matrix_power(mat, n);
      if (a.platform() == Platform::MPS && part.platform() == Platform::MPS) {
        part = part.to(Platform::CPU).contiguous();
      }
      parts.push_back(std::move(part));
    }
    Series out = restore_flat_batch_shape_(stack_dim0_(parts, "matrix_power"), batch, tail);
    if (a.platform() == Platform::MPS) {
      out = out.to(Platform::MPS).contiguous();
    }
    return finish(std::move(out));
  }
  if (a.platform() == Platform::CPU) {
    if (is_complex_scalar_(a.scalar_type())) {
      return finish(matrix_power_native_(a, n));
    }
    return finish(pluss::subsp::math::linalg::matrix_power(a, n));
  }
  if (a.platform() == Platform::MPS) {
    return finish(matrix_power_native_(a, n));
  }
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = pluss::codem::native::cuda::native_linalg_enabled();
    const bool native_strict = pluss::codem::native::cuda::native_linalg_strict();
    const bool route_supported = pluss::codem::native::cuda::supports_matrix_power(a);
    return finish(cuda_linalg_route_or_bridge_(
        "matrix_power",
        native_enabled,
        native_strict,
        route_supported,
        [&]() { return pluss::subsp::math::linalg::matrix_power(a.to(Platform::CPU), n); }));
  }
  throw_platform_not_ready_("matrix_power", a.platform());
}

Series matrix_exp(const Series& a) {
  const auto finish = [&](Series out) {
    bind_matrix_exp_backward_(out, a);
    return out;
  };
  if (a.dim() > 2) {
    require_defined_(a, "matrix_exp");
    const auto tail = tail_shape_(a, 2, "matrix_exp");
    const std::int64_t m = tail[0];
    const std::int64_t k = tail[1];
    PLUSS_CHECK(m == k, "subsp::linearalg::matrix_exp: expected square matrices on last two dims");
    const auto batch = batch_shape_(a, 2, "matrix_exp");
    if (has_zero_extent_(batch)) {
      return finish(Series::zeros(concat_shape_(batch, tail), decomp_fallback_dtype_(a.platform()))
                        .to(a.platform())
                        .contiguous());
    }
    Series batch_src_base = (a.platform() == Platform::MPS) ? a.to(Platform::CPU).contiguous() : a;
    const Series batch_src = flatten_expanded_batch_(batch_src_base, batch, tail, "matrix_exp");
    const std::int64_t flat_batch = numel_from_shape_(batch);
    std::vector<Series> parts;
    parts.reserve(static_cast<std::size_t>(flat_batch));
    for (std::int64_t i = 0; i < flat_batch; ++i) {
      Series mat = batch_src.select(0, i).contiguous();
      if (a.platform() == Platform::MPS) {
        mat = mat.to(Platform::MPS);
      }
      Series part = matrix_exp(mat);
      if (a.platform() == Platform::MPS && part.platform() == Platform::MPS) {
        part = part.to(Platform::CPU).contiguous();
      }
      parts.push_back(std::move(part));
    }
    Series out = restore_flat_batch_shape_(stack_dim0_(parts, "matrix_exp"), batch, tail);
    if (a.platform() == Platform::MPS) {
      out = out.to(Platform::MPS).contiguous();
    }
    return finish(std::move(out));
  }
  if (a.platform() == Platform::CPU) {
    return finish(pluss::subsp::math::linalg::matrix_exp(a));
  }
  if (a.platform() == Platform::MPS) {
    return finish(matrix_exp_native_(a));
  }
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = pluss::codem::native::cuda::native_linalg_enabled();
    const bool native_strict = pluss::codem::native::cuda::native_linalg_strict();
    const bool route_supported = pluss::codem::native::cuda::supports_matrix_exp(a);
    return finish(cuda_linalg_route_or_bridge_(
        "matrix_exp",
        native_enabled,
        native_strict,
        route_supported,
        [&]() { return pluss::subsp::math::linalg::matrix_exp(a.to(Platform::CPU)); }));
  }
  throw_platform_not_ready_("matrix_exp", a.platform());
}

Series pinverse(const Series& a, double rcond) {
  PLUSS_CHECK(std::isfinite(rcond) && rcond >= 0.0,
              "subsp::linearalg::pinverse: rcond must be finite and non-negative");
  const auto finish = [&](Series out) {
    bind_pinverse_backward_(out, a);
    return out;
  };
  if (a.dim() > 2) {
    require_defined_(a, "pinverse");
    const auto tail = tail_shape_(a, 2, "pinverse");
    const auto batch = batch_shape_(a, 2, "pinverse");
    const std::vector<std::int64_t> out_tail{tail[1], tail[0]};
    if (has_zero_extent_(batch)) {
      const auto shape = concat_shape_(batch, out_tail);
      return finish(Series::zeros(shape, decomp_fallback_dtype_(a.platform())).to(a.platform()).contiguous());
    }
    Series batch_src_base = (a.platform() == Platform::MPS) ? a.to(Platform::CPU).contiguous() : a;
    const Series batch_src = flatten_expanded_batch_(batch_src_base, batch, tail, "pinverse");
    const std::int64_t flat_batch = numel_from_shape_(batch);
    std::vector<Series> pinv_parts;
    pinv_parts.reserve(static_cast<std::size_t>(flat_batch));
    for (std::int64_t i = 0; i < flat_batch; ++i) {
      Series mat = batch_src.select(0, i).contiguous();
      if (a.platform() == Platform::MPS) {
        mat = mat.to(Platform::MPS);
      }
      Series part = pinverse(mat, rcond);
      if (a.platform() == Platform::MPS && part.platform() == Platform::MPS) {
        part = part.to(Platform::CPU).contiguous();
      }
      pinv_parts.push_back(std::move(part));
    }
    Series out = restore_flat_batch_shape_(stack_dim0_(pinv_parts, "pinverse"), batch, out_tail);
    if (a.platform() == Platform::MPS) {
      out = out.to(Platform::MPS).contiguous();
    }
    return finish(std::move(out));
  }
  if (a.platform() == Platform::CPU) {
    if (is_complex_scalar_(a.scalar_type())) {
      const auto decomposition = pluss::subsp::math::linalg::svd(a, /*full_matrices=*/false);
      const auto singular_values = decomposition.S.to(ScalarType::Float64).to_vec<double>();
      const double largest = singular_values.empty() ? 0.0 : singular_values.front();
      const double cutoff = rcond * largest;
      std::vector<double> reciprocal;
      reciprocal.reserve(singular_values.size());
      for (double value : singular_values) {
        reciprocal.push_back(value > cutoff ? 1.0 / value : 0.0);
      }
      Series inverse_s = pluss::make_series(
          reciprocal,
          {static_cast<std::int64_t>(reciprocal.size())},
          Config{}.with_dtype(ScalarType::Float64).with_platform(Platform::CPU));
      inverse_s = inverse_s.to(a.scalar_type());
      Series out = matrix_adjoint_(decomposition.Vh)
                       .mul(inverse_s.unsqueeze(0))
                       .matmul(matrix_adjoint_(decomposition.U));
      return finish(out.contiguous());
    }
    return finish(pluss::subsp::math::linalg::pinverse(a, rcond));
  }
  if (a.platform() == Platform::MPS) {
    if (mps_native_pinverse_supported_(a)) {
      try {
        return finish(pinverse_mps_native_(a, rcond));
      } catch (const std::exception&) {
      }
    }
    return finish(bridge_cpu_result_to_mps_(
        pluss::subsp::math::linalg::pinverse(a.to(Platform::CPU), rcond), MpsBridgeOp::Pinverse, "pinverse"));
  }
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = pluss::codem::native::cuda::native_linalg_enabled();
    const bool native_strict = pluss::codem::native::cuda::native_linalg_strict();
    const bool route_supported = pluss::codem::native::cuda::supports_pinverse(a);
    return finish(cuda_linalg_route_or_bridge_(
        "pinverse",
        native_enabled,
        native_strict,
        route_supported,
        [&]() { return pluss::subsp::math::linalg::pinverse(a.to(Platform::CPU), rcond); }));
  }
  throw_platform_not_ready_("pinverse", a.platform());
}

Series qr_q(const Series& a, bool reduced) {
  if (a.dim() > 2) {
    require_defined_(a, "qr_q");
    const auto tail = tail_shape_(a, 2, "qr_q");
    const std::int64_t m = tail[0];
    const std::int64_t n = tail[1];
    const std::int64_t k = std::min(m, n);
    const auto batch = batch_shape_(a, 2, "qr_q");
    const std::vector<std::int64_t> out_tail{m, reduced ? k : m};
    if (has_zero_extent_(batch)) {
      const auto shape = concat_shape_(batch, out_tail);
      return Series::zeros(shape, decomp_fallback_dtype_(a.platform())).to(a.platform()).contiguous();
    }
    Series batch_src_base = (a.platform() == Platform::MPS) ? a.to(Platform::CPU).contiguous() : a;
    const Series batch_src = flatten_expanded_batch_(batch_src_base, batch, tail, "qr_q");
    const std::int64_t flat_batch = numel_from_shape_(batch);
    std::vector<Series> q_parts;
    q_parts.reserve(static_cast<std::size_t>(flat_batch));
    for (std::int64_t i = 0; i < flat_batch; ++i) {
      Series mat = batch_src.select(0, i).contiguous();
      if (a.platform() == Platform::MPS) {
        mat = mat.to(Platform::MPS);
      }
      Series part = qr_q(mat, reduced);
      if (a.platform() == Platform::MPS && part.platform() == Platform::MPS) {
        part = part.to(Platform::CPU).contiguous();
      }
      q_parts.push_back(std::move(part));
    }
    Series out = restore_flat_batch_shape_(stack_dim0_(q_parts, "qr_q"), batch, out_tail);
    if (a.platform() == Platform::MPS) {
      out = out.to(Platform::MPS).contiguous();
    }
    bind_qr_backward_lazy_(out, a, /*grad_is_q=*/true, reduced);
    return out;
  }
  if (a.platform() == Platform::CPU) {
    auto qr = pluss::subsp::math::linalg::qr(a, reduced);
    bind_qr_backward_(qr.Q, a, qr.Q, qr.R, /*grad_is_q=*/true, reduced);
    return qr.Q;
  }
  if (a.platform() == Platform::MPS) {
    const auto& dispatch = cached_mps_qr_dispatch_(a, reduced);
    if (dispatch.bridged) {
      bump_mps_bridge_counter_(MpsBridgeOp::Qr);
    }
    bind_qr_backward_(dispatch.qr.q, a, dispatch.qr.q, dispatch.qr.r, /*grad_is_q=*/true, reduced);
    return dispatch.qr.q;
  }
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = pluss::codem::native::cuda::native_linalg_enabled();
    const bool native_strict = pluss::codem::native::cuda::native_linalg_strict();
    const bool route_supported = pluss::codem::native::cuda::supports_qr(a, reduced);
    Series out = cuda_linalg_route_or_bridge_(
        "qr_q",
        native_enabled,
        native_strict,
        route_supported,
        [&]() { return pluss::subsp::math::linalg::qr(a.to(Platform::CPU), reduced).Q; });
    bind_qr_backward_lazy_(out, a, /*grad_is_q=*/true, reduced);
    return out;
  }
  throw_platform_not_ready_("qr_q", a.platform());
}

Series qr_r(const Series& a, bool reduced) {
  if (a.dim() > 2) {
    require_defined_(a, "qr_r");
    const auto tail = tail_shape_(a, 2, "qr_r");
    const std::int64_t m = tail[0];
    const std::int64_t n = tail[1];
    const std::int64_t k = std::min(m, n);
    const auto batch = batch_shape_(a, 2, "qr_r");
    const std::vector<std::int64_t> out_tail{reduced ? k : m, n};
    if (has_zero_extent_(batch)) {
      const auto shape = concat_shape_(batch, out_tail);
      return Series::zeros(shape, decomp_fallback_dtype_(a.platform())).to(a.platform()).contiguous();
    }
    Series batch_src_base = (a.platform() == Platform::MPS) ? a.to(Platform::CPU).contiguous() : a;
    const Series batch_src = flatten_expanded_batch_(batch_src_base, batch, tail, "qr_r");
    const std::int64_t flat_batch = numel_from_shape_(batch);
    std::vector<Series> r_parts;
    r_parts.reserve(static_cast<std::size_t>(flat_batch));
    for (std::int64_t i = 0; i < flat_batch; ++i) {
      Series mat = batch_src.select(0, i).contiguous();
      if (a.platform() == Platform::MPS) {
        mat = mat.to(Platform::MPS);
      }
      Series part = qr_r(mat, reduced);
      if (a.platform() == Platform::MPS && part.platform() == Platform::MPS) {
        part = part.to(Platform::CPU).contiguous();
      }
      r_parts.push_back(std::move(part));
    }
    Series out = restore_flat_batch_shape_(stack_dim0_(r_parts, "qr_r"), batch, out_tail);
    if (a.platform() == Platform::MPS) {
      out = out.to(Platform::MPS).contiguous();
    }
    bind_qr_backward_lazy_(out, a, /*grad_is_q=*/false, reduced);
    return out;
  }
  if (a.platform() == Platform::CPU) {
    auto qr = pluss::subsp::math::linalg::qr(a, reduced);
    bind_qr_backward_(qr.R, a, qr.Q, qr.R, /*grad_is_q=*/false, reduced);
    return qr.R;
  }
  if (a.platform() == Platform::MPS) {
    const auto& dispatch = cached_mps_qr_dispatch_(a, reduced);
    if (dispatch.bridged) {
      bump_mps_bridge_counter_(MpsBridgeOp::Qr);
    }
    bind_qr_backward_(dispatch.qr.r, a, dispatch.qr.q, dispatch.qr.r, /*grad_is_q=*/false, reduced);
    return dispatch.qr.r;
  }
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = pluss::codem::native::cuda::native_linalg_enabled();
    const bool native_strict = pluss::codem::native::cuda::native_linalg_strict();
    const bool route_supported = pluss::codem::native::cuda::supports_qr(a, reduced);
    Series out = cuda_linalg_route_or_bridge_(
        "qr_r",
        native_enabled,
        native_strict,
        route_supported,
        [&]() { return pluss::subsp::math::linalg::qr(a.to(Platform::CPU), reduced).R; });
    bind_qr_backward_lazy_(out, a, /*grad_is_q=*/false, reduced);
    return out;
  }
  throw_platform_not_ready_("qr_r", a.platform());
}

SvdDecomposition svd(const Series& a, bool full_matrices) {
  if (a.dim() > 2) {
    require_defined_(a, "svd");
    const auto tail = tail_shape_(a, 2, "svd");
    const std::int64_t m = tail[0];
    const std::int64_t n = tail[1];
    const std::int64_t k = std::min(m, n);
    const auto batch = batch_shape_(a, 2, "svd");
    const std::vector<std::int64_t> u_tail{m, full_matrices ? m : k};
    const std::vector<std::int64_t> s_tail{k};
    const std::vector<std::int64_t> vh_tail{full_matrices ? n : k, n};
    if (has_zero_extent_(batch)) {
      const auto dtype = decomp_fallback_dtype_(a.platform());
      return {
          Series::zeros(concat_shape_(batch, u_tail), dtype).to(a.platform()).contiguous(),
          Series::zeros(concat_shape_(batch, s_tail), dtype).to(a.platform()).contiguous(),
          Series::zeros(concat_shape_(batch, vh_tail), dtype).to(a.platform()).contiguous(),
      };
    }

    Series batch_src_base = (a.platform() == Platform::MPS) ? a.to(Platform::CPU).contiguous() : a;
    const Series batch_src = flatten_expanded_batch_(batch_src_base, batch, tail, "svd");
    const std::int64_t flat_batch = numel_from_shape_(batch);
    std::vector<Series> u_parts;
    std::vector<Series> s_parts;
    std::vector<Series> vh_parts;
    u_parts.reserve(static_cast<std::size_t>(flat_batch));
    s_parts.reserve(static_cast<std::size_t>(flat_batch));
    vh_parts.reserve(static_cast<std::size_t>(flat_batch));
    for (std::int64_t i = 0; i < flat_batch; ++i) {
      Series mat = batch_src.select(0, i).contiguous();
      if (a.platform() == Platform::MPS) {
        mat = mat.to(Platform::MPS);
      }
      SvdDecomposition part = svd(mat, full_matrices);
      if (a.platform() == Platform::MPS && part.u.platform() == Platform::MPS) {
        part.u = part.u.to(Platform::CPU).contiguous();
        part.s = part.s.to(Platform::CPU).contiguous();
        part.vh = part.vh.to(Platform::CPU).contiguous();
      }
      u_parts.push_back(std::move(part.u));
      s_parts.push_back(std::move(part.s));
      vh_parts.push_back(std::move(part.vh));
    }

    SvdDecomposition out{
        restore_flat_batch_shape_(stack_dim0_(u_parts, "svd"), batch, u_tail),
        restore_flat_batch_shape_(stack_dim0_(s_parts, "svd"), batch, s_tail),
        restore_flat_batch_shape_(stack_dim0_(vh_parts, "svd"), batch, vh_tail),
    };
    if (a.platform() == Platform::MPS) {
      out.u = out.u.to(Platform::MPS).contiguous();
      out.s = out.s.to(Platform::MPS).contiguous();
      out.vh = out.vh.to(Platform::MPS).contiguous();
    }
    bind_svd_group_backward_(out, a, full_matrices);
    return out;
  }

  if (a.platform() == Platform::CPU) {
    auto raw = pluss::subsp::math::linalg::svd(a, full_matrices);
    SvdDecomposition out{raw.U, raw.S, raw.Vh};
    bind_svd_group_backward_(out, a, full_matrices);
    return out;
  }

  if (a.platform() == Platform::MPS) {
    const auto& dispatch = cached_mps_svd_dispatch_(a, full_matrices);
    if (dispatch.bridged) {
      bump_mps_bridge_counter_(MpsBridgeOp::Svd);
    }
    SvdDecomposition out{dispatch.svd.u, dispatch.svd.s, dispatch.svd.vh};
    bind_svd_group_backward_(out, a, full_matrices);
    return out;
  }

  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = pluss::codem::native::cuda::native_linalg_enabled();
    const bool native_strict = pluss::codem::native::cuda::native_linalg_strict();
    const bool route_supported = pluss::codem::native::cuda::supports_svd(a, full_matrices);
    auto cpu_route = [&]() { return pluss::subsp::math::linalg::svd(a.to(Platform::CPU), full_matrices); };
    if (native_enabled && route_supported) {
      if (!pluss::codem::cuda_cpu_bridge_enabled()) {
        throw_cuda_linalg_unavailable_("svd", native_enabled, native_strict, route_supported);
      }
      auto raw = cpu_route();
      SvdDecomposition out{
          bridge_cpu_result_to_cuda_(raw.U, "svd", /*allow_strict_native=*/true),
          bridge_cpu_result_to_cuda_(raw.S, "svd", /*allow_strict_native=*/true),
          bridge_cpu_result_to_cuda_(raw.Vh, "svd", /*allow_strict_native=*/true),
      };
      bind_svd_group_backward_(out, a, full_matrices);
      return out;
    }
    if (native_strict) {
      pluss::codem::throw_error(
          "subsp::linearalg::svd: strict native CUDA route requested but kernel is unsupported");
    }
    if (pluss::codem::cuda_cpu_bridge_enabled()) {
      auto raw = cpu_route();
      SvdDecomposition out{
          bridge_cpu_result_to_cuda_(raw.U, "svd"),
          bridge_cpu_result_to_cuda_(raw.S, "svd"),
          bridge_cpu_result_to_cuda_(raw.Vh, "svd"),
      };
      bind_svd_group_backward_(out, a, full_matrices);
      return out;
    }
    throw_cuda_linalg_unavailable_("svd", native_enabled, native_strict, route_supported);
  }

  throw_platform_not_ready_("svd", a.platform());
}

Series svd_u(const Series& a, bool full_matrices) {
  if (a.dim() > 2) {
    require_defined_(a, "svd_u");
    const auto tail = tail_shape_(a, 2, "svd_u");
    const std::int64_t m = tail[0];
    const std::int64_t n = tail[1];
    const std::int64_t k = std::min(m, n);
    const auto batch = batch_shape_(a, 2, "svd_u");
    const std::vector<std::int64_t> out_tail{m, full_matrices ? m : k};
    if (has_zero_extent_(batch)) {
      const auto shape = concat_shape_(batch, out_tail);
      return Series::zeros(shape, decomp_fallback_dtype_(a.platform())).to(a.platform()).contiguous();
    }
    Series batch_src_base = (a.platform() == Platform::MPS) ? a.to(Platform::CPU).contiguous() : a;
    const Series batch_src = flatten_expanded_batch_(batch_src_base, batch, tail, "svd_u");
    const std::int64_t flat_batch = numel_from_shape_(batch);
    std::vector<Series> u_parts;
    u_parts.reserve(static_cast<std::size_t>(flat_batch));
    for (std::int64_t i = 0; i < flat_batch; ++i) {
      Series mat = batch_src.select(0, i).contiguous();
      if (a.platform() == Platform::MPS) {
        mat = mat.to(Platform::MPS);
      }
      Series part = svd_u(mat, full_matrices);
      if (a.platform() == Platform::MPS && part.platform() == Platform::MPS) {
        part = part.to(Platform::CPU).contiguous();
      }
      u_parts.push_back(std::move(part));
    }
    Series out = restore_flat_batch_shape_(stack_dim0_(u_parts, "svd_u"), batch, out_tail);
    if (a.platform() == Platform::MPS) {
      out = out.to(Platform::MPS).contiguous();
    }
    bind_svd_backward_lazy_(out, a, /*slot=*/0, full_matrices);
    return out;
  }
  if (a.platform() == Platform::CPU) {
    auto svd = pluss::subsp::math::linalg::svd(a, full_matrices);
    bind_svd_backward_(svd.U, a, svd.U, svd.S, svd.Vh, /*slot=*/0, full_matrices);
    return svd.U;
  }
  if (a.platform() == Platform::MPS) {
    const auto& dispatch = cached_mps_svd_dispatch_(a, full_matrices);
    if (dispatch.bridged) {
      bump_mps_bridge_counter_(MpsBridgeOp::Svd);
    }
    bind_svd_backward_(dispatch.svd.u, a, dispatch.svd.u, dispatch.svd.s, dispatch.svd.vh, /*slot=*/0, full_matrices);
    return dispatch.svd.u;
  }
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = pluss::codem::native::cuda::native_linalg_enabled();
    const bool native_strict = pluss::codem::native::cuda::native_linalg_strict();
    const bool route_supported = pluss::codem::native::cuda::supports_svd(a, full_matrices);
    Series out = cuda_linalg_route_or_bridge_(
        "svd_u",
        native_enabled,
        native_strict,
        route_supported,
        [&]() { return pluss::subsp::math::linalg::svd(a.to(Platform::CPU), full_matrices).U; });
    bind_svd_backward_lazy_(out, a, /*slot=*/0, full_matrices);
    return out;
  }
  throw_platform_not_ready_("svd_u", a.platform());
}

Series svd_s(const Series& a, bool full_matrices) {
  if (a.dim() > 2) {
    require_defined_(a, "svd_s");
    const auto tail = tail_shape_(a, 2, "svd_s");
    const std::int64_t m = tail[0];
    const std::int64_t n = tail[1];
    const std::int64_t k = std::min(m, n);
    const auto batch = batch_shape_(a, 2, "svd_s");
    const std::vector<std::int64_t> out_tail{k};
    if (has_zero_extent_(batch)) {
      const auto shape = concat_shape_(batch, out_tail);
      return Series::zeros(shape, decomp_fallback_dtype_(a.platform())).to(a.platform()).contiguous();
    }
    Series batch_src_base = (a.platform() == Platform::MPS) ? a.to(Platform::CPU).contiguous() : a;
    const Series batch_src = flatten_expanded_batch_(batch_src_base, batch, tail, "svd_s");
    const std::int64_t flat_batch = numel_from_shape_(batch);
    std::vector<Series> s_parts;
    s_parts.reserve(static_cast<std::size_t>(flat_batch));
    for (std::int64_t i = 0; i < flat_batch; ++i) {
      Series mat = batch_src.select(0, i).contiguous();
      if (a.platform() == Platform::MPS) {
        mat = mat.to(Platform::MPS);
      }
      Series part = svd_s(mat, full_matrices);
      if (a.platform() == Platform::MPS && part.platform() == Platform::MPS) {
        part = part.to(Platform::CPU).contiguous();
      }
      s_parts.push_back(std::move(part));
    }
    Series out = restore_flat_batch_shape_(stack_dim0_(s_parts, "svd_s"), batch, out_tail);
    if (a.platform() == Platform::MPS) {
      out = out.to(Platform::MPS).contiguous();
    }
    bind_svd_backward_lazy_(out, a, /*slot=*/1, full_matrices);
    return out;
  }
  if (a.platform() == Platform::CPU) {
    auto svd = pluss::subsp::math::linalg::svd(a, full_matrices);
    bind_svd_backward_(svd.S, a, svd.U, svd.S, svd.Vh, /*slot=*/1, full_matrices);
    return svd.S;
  }
  if (a.platform() == Platform::MPS) {
    const auto& dispatch = cached_mps_svd_dispatch_(a, full_matrices);
    if (dispatch.bridged) {
      bump_mps_bridge_counter_(MpsBridgeOp::Svd);
    }
    bind_svd_backward_(dispatch.svd.s, a, dispatch.svd.u, dispatch.svd.s, dispatch.svd.vh, /*slot=*/1, full_matrices);
    return dispatch.svd.s;
  }
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = pluss::codem::native::cuda::native_linalg_enabled();
    const bool native_strict = pluss::codem::native::cuda::native_linalg_strict();
    const bool route_supported = pluss::codem::native::cuda::supports_svd(a, full_matrices);
    Series out = cuda_linalg_route_or_bridge_(
        "svd_s",
        native_enabled,
        native_strict,
        route_supported,
        [&]() { return pluss::subsp::math::linalg::svd(a.to(Platform::CPU), full_matrices).S; });
    bind_svd_backward_lazy_(out, a, /*slot=*/1, full_matrices);
    return out;
  }
  throw_platform_not_ready_("svd_s", a.platform());
}

Series svd_vh(const Series& a, bool full_matrices) {
  if (a.dim() > 2) {
    require_defined_(a, "svd_vh");
    const auto tail = tail_shape_(a, 2, "svd_vh");
    const std::int64_t m = tail[0];
    const std::int64_t n = tail[1];
    const std::int64_t k = std::min(m, n);
    const auto batch = batch_shape_(a, 2, "svd_vh");
    const std::vector<std::int64_t> out_tail{full_matrices ? n : k, n};
    if (has_zero_extent_(batch)) {
      const auto shape = concat_shape_(batch, out_tail);
      return Series::zeros(shape, decomp_fallback_dtype_(a.platform())).to(a.platform()).contiguous();
    }
    Series batch_src_base = (a.platform() == Platform::MPS) ? a.to(Platform::CPU).contiguous() : a;
    const Series batch_src = flatten_expanded_batch_(batch_src_base, batch, tail, "svd_vh");
    const std::int64_t flat_batch = numel_from_shape_(batch);
    std::vector<Series> vh_parts;
    vh_parts.reserve(static_cast<std::size_t>(flat_batch));
    for (std::int64_t i = 0; i < flat_batch; ++i) {
      Series mat = batch_src.select(0, i).contiguous();
      if (a.platform() == Platform::MPS) {
        mat = mat.to(Platform::MPS);
      }
      Series part = svd_vh(mat, full_matrices);
      if (a.platform() == Platform::MPS && part.platform() == Platform::MPS) {
        part = part.to(Platform::CPU).contiguous();
      }
      vh_parts.push_back(std::move(part));
    }
    Series out = restore_flat_batch_shape_(stack_dim0_(vh_parts, "svd_vh"), batch, out_tail);
    if (a.platform() == Platform::MPS) {
      out = out.to(Platform::MPS).contiguous();
    }
    bind_svd_backward_lazy_(out, a, /*slot=*/2, full_matrices);
    return out;
  }
  if (a.platform() == Platform::CPU) {
    auto svd = pluss::subsp::math::linalg::svd(a, full_matrices);
    bind_svd_backward_(svd.Vh, a, svd.U, svd.S, svd.Vh, /*slot=*/2, full_matrices);
    return svd.Vh;
  }
  if (a.platform() == Platform::MPS) {
    const auto& dispatch = cached_mps_svd_dispatch_(a, full_matrices);
    if (dispatch.bridged) {
      bump_mps_bridge_counter_(MpsBridgeOp::Svd);
    }
    bind_svd_backward_(dispatch.svd.vh, a, dispatch.svd.u, dispatch.svd.s, dispatch.svd.vh, /*slot=*/2, full_matrices);
    return dispatch.svd.vh;
  }
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = pluss::codem::native::cuda::native_linalg_enabled();
    const bool native_strict = pluss::codem::native::cuda::native_linalg_strict();
    const bool route_supported = pluss::codem::native::cuda::supports_svd(a, full_matrices);
    Series out = cuda_linalg_route_or_bridge_(
        "svd_vh",
        native_enabled,
        native_strict,
        route_supported,
        [&]() { return pluss::subsp::math::linalg::svd(a.to(Platform::CPU), full_matrices).Vh; });
    bind_svd_backward_lazy_(out, a, /*slot=*/2, full_matrices);
    return out;
  }
  throw_platform_not_ready_("svd_vh", a.platform());
}

Series triangular_solve(const Series& a, const Series& b, bool upper, bool unitriangular) {
  require_same_platform_(a, b, "triangular_solve");
  if (a.dim() > 2 || b.dim() > 2) {
    const char* op = "triangular_solve";
    const auto a_tail = tail_shape_(a, 2, op);
    PLUSS_CHECK(
        a_tail[0] == a_tail[1],
        "subsp::linearalg::triangular_solve: expected square coefficient matrix");
    const auto rhs_layout = detect_solver_rhs_layout_(a, b, op);
    const std::int64_t rhs_tail_rank = (rhs_layout == SolverRhsLayout::Matrix) ? 2 : 1;
    const auto b_tail = tail_shape_(b, rhs_tail_rank, op);
    const auto a_batch = batch_shape_(a, 2, op);
    const auto b_batch = batch_shape_(b, rhs_tail_rank, op);
    const auto out_batch = broadcast_batch_shape_(a_batch, b_batch, op);
    const auto a_expand_shape = concat_shape_(out_batch, a_tail);
    const auto b_expand_shape = concat_shape_(out_batch, b_tail);

    Series a_batch_src_base = (a.platform() == Platform::MPS) ? a.to(Platform::CPU).contiguous() : a;
    Series b_batch_src_base = (b.platform() == Platform::MPS) ? b.to(Platform::CPU).contiguous() : b;
    if (out_batch.empty()) {
      Series ai = a_batch_src_base.expand(a_expand_shape, true).contiguous();
      Series bi = b_batch_src_base.expand(b_expand_shape, true).contiguous();
      if (a.platform() == Platform::MPS) ai = ai.to(Platform::MPS);
      if (b.platform() == Platform::MPS) bi = bi.to(Platform::MPS);
      return triangular_solve(ai, bi, upper, unitriangular);
    }
    const std::vector<std::int64_t> out_tail =
        (rhs_layout == SolverRhsLayout::Matrix) ? b_tail : std::vector<std::int64_t>{a_tail[0], 1};
    if (has_zero_extent_(out_batch)) {
      return Series::zeros(
                 concat_shape_(out_batch, out_tail),
                 decomp_fallback_dtype_(a.platform()))
          .to(a.platform())
          .contiguous();
    }
    const std::int64_t flat_batch = numel_from_shape_(out_batch);
    Series a_batch_src = flatten_expanded_batch_(a_batch_src_base, out_batch, a_tail, op);
    Series b_batch_src = flatten_expanded_batch_(b_batch_src_base, out_batch, b_tail, op);

    std::vector<Series> parts;
    parts.reserve(static_cast<std::size_t>(flat_batch));
    for (std::int64_t i = 0; i < flat_batch; ++i) {
      Series ai = a_batch_src.select(0, i).contiguous();
      Series bi = b_batch_src.select(0, i).contiguous();
      if (a.platform() == Platform::MPS && !mps_experimental_native_enabled_()) {
        Series part_cpu = triangular_solve(ai, bi, upper, unitriangular);
        if (part_cpu.scalar_type() != ScalarType::Float32) {
          part_cpu = part_cpu.to(ScalarType::Float32).contiguous();
        }
        bump_mps_bridge_counter_(MpsBridgeOp::TriangularSolve);
        parts.push_back(std::move(part_cpu));
        continue;
      }
      if (a.platform() == Platform::MPS) ai = ai.to(Platform::MPS);
      if (b.platform() == Platform::MPS) bi = bi.to(Platform::MPS);
      Series part = triangular_solve(ai, bi, upper, unitriangular);
      if (a.platform() == Platform::MPS && part.platform() == Platform::MPS) {
        part = part.to(Platform::CPU).contiguous();
      }
      parts.push_back(std::move(part));
    }
    Series out = restore_flat_batch_shape_(stack_dim0_(parts, op), out_batch, out_tail);
    if (a.platform() == Platform::MPS) {
      out = out.to(Platform::MPS).contiguous();
    }
    bind_triangular_solve_backward_(out, a, b, upper, unitriangular);
    return out;
  }

  if (a.platform() == Platform::CPU) {
    if (complex_small_triangular_supported_(a, b)) {
      Series out = triangular_solve_small_complex_(a, b, upper, unitriangular);
      bind_triangular_solve_backward_(out, a, b, upper, unitriangular);
      return out;
    }
    PLUSS_CHECK(!is_complex_scalar_(a.scalar_type()) && !is_complex_scalar_(b.scalar_type()),
                "subsp::linearalg::triangular_solve: complex solve currently supports only 1x1 and 2x2 matrices");
    Series out = pluss::subsp::math::linalg::triangular_solve(b, a, upper, unitriangular);
    bind_triangular_solve_backward_(out, a, b, upper, unitriangular);
    return out;
  }
  if (a.platform() == Platform::MPS) {
    if (complex_small_triangular_supported_(a, b)) {
      Series out = triangular_solve_small_complex_(a, b, upper, unitriangular);
      bind_triangular_solve_backward_(out, a, b, upper, unitriangular);
      return out;
    }
    PLUSS_CHECK(!is_complex_scalar_(a.scalar_type()) && !is_complex_scalar_(b.scalar_type()),
                "subsp::linearalg::triangular_solve: complex MPS solve currently supports only 1x1 and 2x2 matrices");
    if (mps_native_triangular_solve_supported_(a, b)) {
      Series out = triangular_solve_mps_native_(a, b, upper, unitriangular);
      bind_triangular_solve_backward_(out, a, b, upper, unitriangular);
      return out;
    }
    Series out = bridge_cpu_result_to_mps_(
        pluss::subsp::math::linalg::triangular_solve(b.to(Platform::CPU), a.to(Platform::CPU), upper, unitriangular),
        MpsBridgeOp::TriangularSolve,
        "triangular_solve");
    bind_triangular_solve_backward_(out, a, b, upper, unitriangular);
    return out;
  }
  if (a.platform() == Platform::CUDA) {
    const bool native_enabled = pluss::codem::native::cuda::native_linalg_enabled();
    const bool native_strict = pluss::codem::native::cuda::native_linalg_strict();
    const bool route_supported = pluss::codem::native::cuda::supports_triangular_solve(a, b);
    Series out = cuda_linalg_route_or_bridge_(
        "triangular_solve",
        native_enabled,
        native_strict,
        route_supported,
        [&]() {
          return pluss::subsp::math::linalg::triangular_solve(
              b.to(Platform::CPU), a.to(Platform::CPU), upper, unitriangular);
        });
    bind_triangular_solve_backward_(out, a, b, upper, unitriangular);
    return out;
  }
  throw_platform_not_ready_("triangular_solve", a.platform());
}

Series cholesky_solve(const Series& factor, const Series& b, bool upper) {
  require_same_platform_(factor, b, "cholesky_solve");
  if (b.dim() == 1) {
    std::vector<std::int64_t> shape{b.sizes()[0], 1};
    return cholesky_solve(factor, b.reshape(shape), upper);
  }
  if (complex_small_triangular_supported_(factor, b)) {
    const Series first_factor = upper ? matrix_adjoint_(factor) : factor;
    const Series second_factor = upper ? factor : matrix_adjoint_(factor);
    const Series y = triangular_solve(first_factor, b, /*upper=*/false, /*unitriangular=*/false);
    Series out = triangular_solve(second_factor, y, /*upper=*/true, /*unitriangular=*/false);
    bind_cholesky_solve_backward_(out, factor, b, upper);
    return out;
  }
  if (factor.dim() > 2 || b.dim() > 2) {
    const char* op = "cholesky_solve";
    const auto f_tail = tail_shape_(factor, 2, op);
    PLUSS_CHECK(
        f_tail[0] == f_tail[1],
        "subsp::linearalg::cholesky_solve: expected square factor matrix");
    const auto rhs_layout = detect_solver_rhs_layout_(factor, b, op);
    const std::int64_t rhs_tail_rank = (rhs_layout == SolverRhsLayout::Matrix) ? 2 : 1;
    const auto b_tail = tail_shape_(b, rhs_tail_rank, op);
    const auto f_batch = batch_shape_(factor, 2, op);
    const auto b_batch = batch_shape_(b, rhs_tail_rank, op);
    const auto out_batch = broadcast_batch_shape_(f_batch, b_batch, op);
    const auto f_expand_shape = concat_shape_(out_batch, f_tail);
    const auto b_expand_shape = concat_shape_(out_batch, b_tail);

    Series f_batch_src_base = (factor.platform() == Platform::MPS) ? factor.to(Platform::CPU).contiguous() : factor;
    Series b_batch_src_base = (b.platform() == Platform::MPS) ? b.to(Platform::CPU).contiguous() : b;
    if (out_batch.empty()) {
      Series fi = f_batch_src_base.expand(f_expand_shape, true).contiguous();
      Series bi = b_batch_src_base.expand(b_expand_shape, true).contiguous();
      if (factor.platform() == Platform::MPS) fi = fi.to(Platform::MPS);
      if (b.platform() == Platform::MPS) bi = bi.to(Platform::MPS);
      return cholesky_solve(fi, bi, upper);
    }
    const std::vector<std::int64_t> out_tail =
        (rhs_layout == SolverRhsLayout::Matrix) ? b_tail : std::vector<std::int64_t>{f_tail[0], 1};
    if (has_zero_extent_(out_batch)) {
      return Series::zeros(
                 concat_shape_(out_batch, out_tail),
                 decomp_fallback_dtype_(factor.platform()))
          .to(factor.platform())
          .contiguous();
    }
    const std::int64_t flat_batch = numel_from_shape_(out_batch);
    Series f_batch_src = flatten_expanded_batch_(f_batch_src_base, out_batch, f_tail, op);
    Series b_batch_src = flatten_expanded_batch_(b_batch_src_base, out_batch, b_tail, op);

    std::vector<Series> parts;
    parts.reserve(static_cast<std::size_t>(flat_batch));
    for (std::int64_t i = 0; i < flat_batch; ++i) {
      Series fi = f_batch_src.select(0, i).contiguous();
      Series bi = b_batch_src.select(0, i).contiguous();
      if (factor.platform() == Platform::MPS && !mps_experimental_native_enabled_()) {
        Series part_cpu = cholesky_solve(fi, bi, upper);
        if (part_cpu.scalar_type() != ScalarType::Float32) {
          part_cpu = part_cpu.to(ScalarType::Float32).contiguous();
        }
        bump_mps_bridge_counter_(MpsBridgeOp::CholeskySolve);
        parts.push_back(std::move(part_cpu));
        continue;
      }
      if (factor.platform() == Platform::MPS) fi = fi.to(Platform::MPS);
      if (b.platform() == Platform::MPS) bi = bi.to(Platform::MPS);
      Series part = cholesky_solve(fi, bi, upper);
      if (factor.platform() == Platform::MPS && part.platform() == Platform::MPS) {
        part = part.to(Platform::CPU).contiguous();
      }
      parts.push_back(std::move(part));
    }
    Series out = restore_flat_batch_shape_(stack_dim0_(parts, op), out_batch, out_tail);
    if (factor.platform() == Platform::MPS) {
      out = out.to(Platform::MPS).contiguous();
    }
    bind_cholesky_solve_backward_(out, factor, b, upper);
    return out;
  }

  if (factor.platform() == Platform::CPU) {
    Series out = pluss::subsp::math::linalg::cholesky_solve(b, factor, upper);
    bind_cholesky_solve_backward_(out, factor, b, upper);
    return out;
  }
  if (factor.platform() == Platform::MPS) {
    if (mps_experimental_native_enabled_()) {
      const Series native_factor = canonical_mps_native_f32_(factor);
      const Series native_b = canonical_mps_native_f32_(b);
      if (pluss::codem::native::mps::supports_cholesky_solve(native_factor, native_b)) {
        Series out = pluss::codem::native::mps::cholesky_solve(native_factor, native_b, upper);
        bind_cholesky_solve_backward_(out, factor, b, upper);
        return out;
      }
    }
    if (mps_native_triangular_solve_supported_(factor, b)) {
      Series l = factor.to(ScalarType::Float32).contiguous();
      if (upper) {
        l = l.transpose(0, 1).contiguous();
      }
      const Series y = triangular_solve_mps_native_(l, b, /*upper=*/false, /*unitriangular=*/false);
      Series out =
          triangular_solve_mps_native_(matrix_adjoint_(l).contiguous(), y, /*upper=*/true, /*unitriangular=*/false);
      bind_cholesky_solve_backward_(out, factor, b, upper);
      return out;
    }
    Series out = bridge_cpu_result_to_mps_(
        pluss::subsp::math::linalg::cholesky_solve(b.to(Platform::CPU), factor.to(Platform::CPU), upper),
        MpsBridgeOp::CholeskySolve,
        "cholesky_solve");
    bind_cholesky_solve_backward_(out, factor, b, upper);
    return out;
  }
  if (factor.platform() == Platform::CUDA) {
    const bool native_enabled = pluss::codem::native::cuda::native_linalg_enabled();
    const bool native_strict = pluss::codem::native::cuda::native_linalg_strict();
    const bool route_supported = pluss::codem::native::cuda::supports_cholesky_solve(factor, b);
    Series out = cuda_linalg_route_or_bridge_(
        "cholesky_solve",
        native_enabled,
        native_strict,
        route_supported,
        [&]() {
          return pluss::subsp::math::linalg::cholesky_solve(
              b.to(Platform::CPU), factor.to(Platform::CPU), upper);
        });
    bind_cholesky_solve_backward_(out, factor, b, upper);
    return out;
  }
  throw_platform_not_ready_("cholesky_solve", factor.platform());
}

Series cholesky_inverse(const Series& factor, bool upper) {
  const auto finish = [&](Series out) {
    bind_cholesky_inverse_backward_(out, factor, upper);
    return out;
  };
  if (factor.dim() > 2) {
    const auto tail = tail_shape_(factor, 2, "cholesky_inverse");
    const auto batch = batch_shape_(factor, 2, "cholesky_inverse");
    if (has_zero_extent_(batch)) {
      return finish(Series::zeros(concat_shape_(batch, tail), decomp_fallback_dtype_(factor.platform()))
                        .to(factor.platform())
                        .contiguous());
    }
    const Series f_batch_src_base =
        (factor.platform() == Platform::MPS) ? factor.to(Platform::CPU).contiguous() : factor;
    if (batch.empty()) {
      Series fi = f_batch_src_base.expand(concat_shape_(batch, tail), true).contiguous();
      if (factor.platform() == Platform::MPS) {
        fi = fi.to(Platform::MPS);
      }
      return finish(cholesky_inverse(fi, upper));
    }
    const std::int64_t flat_batch = numel_from_shape_(batch);
    const Series f_batch_src = flatten_expanded_batch_(f_batch_src_base, batch, tail, "cholesky_inverse");
    std::vector<Series> parts;
    parts.reserve(static_cast<std::size_t>(flat_batch));
    for (std::int64_t i = 0; i < flat_batch; ++i) {
      Series fi = f_batch_src.select(0, i).contiguous();
      if (factor.platform() == Platform::MPS && !mps_experimental_native_enabled_()) {
        Series part_cpu = cholesky_inverse(fi, upper);
        if (part_cpu.scalar_type() != ScalarType::Float32) {
          part_cpu = part_cpu.to(ScalarType::Float32).contiguous();
        }
        bump_mps_bridge_counter_(MpsBridgeOp::CholeskyInverse);
        parts.push_back(std::move(part_cpu));
        continue;
      }
      if (factor.platform() == Platform::MPS) fi = fi.to(Platform::MPS);
      Series part = cholesky_inverse(fi, upper);
      if (factor.platform() == Platform::MPS && part.platform() == Platform::MPS) {
        part = part.to(Platform::CPU).contiguous();
      }
      parts.push_back(std::move(part));
    }
    Series out = restore_flat_batch_shape_(stack_dim0_(parts, "cholesky_inverse"), batch, tail);
    if (factor.platform() == Platform::MPS) {
      out = out.to(Platform::MPS).contiguous();
    }
    return finish(std::move(out));
  }

  if (factor.platform() == Platform::CPU) {
    return finish(pluss::subsp::math::linalg::cholesky_inverse(factor, upper));
  }
  if (factor.platform() == Platform::MPS) {
    if (mps_experimental_native_enabled_()) {
      const Series native_factor = canonical_mps_native_f32_(factor);
      if (pluss::codem::native::mps::supports_cholesky_factor(native_factor)) {
        const auto n = native_factor.sizes()[0];
        const Series eye = Series::eye(n, n, ScalarType::Float32).to(Platform::MPS).contiguous();
        return finish(pluss::codem::native::mps::cholesky_solve(native_factor, eye, upper));
      }
    }
    if (mps_experimental_native_enabled_()) {
      require_square_2d_(factor, "cholesky_inverse");
      Series l = factor.to(ScalarType::Float32).contiguous();
      if (upper) {
        l = l.transpose(0, 1).contiguous();
      }
      const auto n = l.sizes()[0];
      const Series eye = Series::eye(n, n, ScalarType::Float32).to(Platform::MPS).contiguous();
      const Series y = triangular_solve_mps_native_(l, eye, /*upper=*/false, /*unitriangular=*/false);
      return finish(triangular_solve_mps_native_(
          l.transpose(0, 1).contiguous(), y, /*upper=*/true, /*unitriangular=*/false));
    }
    return finish(bridge_cpu_result_to_mps_(
        pluss::subsp::math::linalg::cholesky_inverse(factor.to(Platform::CPU), upper),
        MpsBridgeOp::CholeskyInverse,
        "cholesky_inverse"));
  }
  if (factor.platform() == Platform::CUDA) {
    const bool native_enabled = pluss::codem::native::cuda::native_linalg_enabled();
    const bool native_strict = pluss::codem::native::cuda::native_linalg_strict();
    const bool route_supported = pluss::codem::native::cuda::supports_cholesky_inverse(factor);
    return finish(cuda_linalg_route_or_bridge_(
        "cholesky_inverse",
        native_enabled,
        native_strict,
        route_supported,
        [&]() { return pluss::subsp::math::linalg::cholesky_inverse(factor.to(Platform::CPU), upper); }));
  }
  throw_platform_not_ready_("cholesky_inverse", factor.platform());
}

Series lu_solve(const Series& lu, const Series& b) {
  require_same_platform_(lu, b, "lu_solve");
  const auto finish = [&](Series out) {
    bind_lu_solve_backward_(out, lu, b);
    return out;
  };
  if (lu.dim() > 2 || b.dim() > 2) {
    const char* op = "lu_solve";
    const auto lu_tail = tail_shape_(lu, 2, op);
    PLUSS_CHECK(
        lu_tail[0] == lu_tail[1],
        "subsp::linearalg::lu_solve: expected square LU matrix");
    const auto rhs_layout = detect_solver_rhs_layout_(lu, b, op);
    const std::int64_t rhs_tail_rank = (rhs_layout == SolverRhsLayout::Matrix) ? 2 : 1;
    const auto b_tail = tail_shape_(b, rhs_tail_rank, op);
    const auto lu_batch = batch_shape_(lu, 2, op);
    const auto b_batch = batch_shape_(b, rhs_tail_rank, op);
    const auto out_batch = broadcast_batch_shape_(lu_batch, b_batch, op);
    const auto lu_expand_shape = concat_shape_(out_batch, lu_tail);
    const auto b_expand_shape = concat_shape_(out_batch, b_tail);

    Series lu_batch_src_base = (lu.platform() == Platform::MPS) ? lu.to(Platform::CPU).contiguous() : lu;
    Series b_batch_src_base = (b.platform() == Platform::MPS) ? b.to(Platform::CPU).contiguous() : b;
    if (out_batch.empty()) {
      Series lui = lu_batch_src_base.expand(lu_expand_shape, true).contiguous();
      Series bi = b_batch_src_base.expand(b_expand_shape, true).contiguous();
      if (lu.platform() == Platform::MPS) lui = lui.to(Platform::MPS);
      if (b.platform() == Platform::MPS) bi = bi.to(Platform::MPS);
      return finish(lu_solve(lui, bi));
    }
    const std::vector<std::int64_t> out_tail =
        (rhs_layout == SolverRhsLayout::Matrix) ? b_tail : std::vector<std::int64_t>{lu_tail[0], 1};
    if (has_zero_extent_(out_batch)) {
      return finish(Series::zeros(
                 concat_shape_(out_batch, out_tail),
                 decomp_fallback_dtype_(lu.platform()))
          .to(lu.platform())
          .contiguous());
    }
    const std::int64_t flat_batch = numel_from_shape_(out_batch);
    Series lu_batch_src = flatten_expanded_batch_(lu_batch_src_base, out_batch, lu_tail, op);
    Series b_batch_src = flatten_expanded_batch_(b_batch_src_base, out_batch, b_tail, op);

    std::vector<Series> parts;
    parts.reserve(static_cast<std::size_t>(flat_batch));
    for (std::int64_t i = 0; i < flat_batch; ++i) {
      Series lui = lu_batch_src.select(0, i).contiguous();
      Series bi = b_batch_src.select(0, i).contiguous();
      if (lu.platform() == Platform::MPS && !mps_experimental_native_enabled_()) {
        Series part_cpu = lu_solve(lui, bi);
        if (part_cpu.scalar_type() != ScalarType::Float32) {
          part_cpu = part_cpu.to(ScalarType::Float32).contiguous();
        }
        bump_mps_bridge_counter_(MpsBridgeOp::LuSolve);
        parts.push_back(std::move(part_cpu));
        continue;
      }
      if (lu.platform() == Platform::MPS) lui = lui.to(Platform::MPS);
      if (b.platform() == Platform::MPS) bi = bi.to(Platform::MPS);
      Series part = lu_solve(lui, bi);
      if (lu.platform() == Platform::MPS && part.platform() == Platform::MPS) {
        part = part.to(Platform::CPU).contiguous();
      }
      parts.push_back(std::move(part));
    }
    Series out = restore_flat_batch_shape_(stack_dim0_(parts, op), out_batch, out_tail);
    if (lu.platform() == Platform::MPS) {
      out = out.to(Platform::MPS).contiguous();
    }
    return finish(std::move(out));
  }

  if (lu.platform() == Platform::CPU) {
    return finish(pluss::subsp::math::linalg::lu_solve(b, lu));
  }
  if (lu.platform() == Platform::MPS) {
    if (mps_native_lu_solve_supported_(lu, b)) {
      return finish(lu_solve_mps_native_(lu, b));
    }
    return finish(bridge_cpu_result_to_mps_(
        pluss::subsp::math::linalg::lu_solve(b.to(Platform::CPU), lu.to(Platform::CPU)),
        MpsBridgeOp::LuSolve,
        "lu_solve"));
  }
  if (lu.platform() == Platform::CUDA) {
    const bool native_enabled = pluss::codem::native::cuda::native_linalg_enabled();
    const bool native_strict = pluss::codem::native::cuda::native_linalg_strict();
    const bool route_supported = pluss::codem::native::cuda::supports_lu_solve(lu, b);
    return finish(cuda_linalg_route_or_bridge_(
        "lu_solve",
        native_enabled,
        native_strict,
        route_supported,
        [&]() { return pluss::subsp::math::linalg::lu_solve(b.to(Platform::CPU), lu.to(Platform::CPU)); }));
  }
  throw_platform_not_ready_("lu_solve", lu.platform());
}

} // namespace pluss::subsp::linearalg
