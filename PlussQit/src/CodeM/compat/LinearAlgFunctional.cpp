#include "pluss/subsp/linearalg/linearalg.h"

#include "CodeM/core/Errors.h"
#include "CodeM/native/cpu/BlasGemm.h"
#include "CodeM/native/mps/Linalg.h"
#include "pluss/codem/series_factory.h"
#include "pluss/subsp/math/linalg.h"

#include <atomic>
#include <cmath>
#include <cstddef>
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
  const auto n = a.sizes()[0];
  auto out = Series::eye(n, n, a.scalar_type());
  if (out.platform() != a.platform()) {
    out = out.to(a.platform());
  }
  return out;
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
      a.sizes()[0] == a.sizes()[1];
}

inline bool mps_native_det_supported_(const Series& a) {
  return mps_experimental_native_enabled_() &&
      a.platform() == Platform::MPS &&
      a.dim() == 2 &&
      a.sizes()[0] == a.sizes()[1];
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
  if (pluss::codem::native::mps::supports_lu_solver(native_a)) {
    const auto stats = pluss::codem::native::mps::lu_det_stats(native_a);
    return DetStats{stats.sign, stats.logabs, stats.singular};
  }
  const Series cpu = a.to(Platform::CPU).to(ScalarType::Float32).contiguous();
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

inline Series pinverse_mps_native_(const Series& a, double rcond) {
  PLUSS_CHECK(
      mps_native_pinverse_supported_(a),
      "subsp::linearalg::pinverse: native MPS path expects non-empty 2D input");
  const auto m = a.sizes()[0];
  const auto n = a.sizes()[1];
  const Series at = a.transpose(0, 1).contiguous();
  const double rc = (rcond > 0.0) ? rcond : 0.0;
  // Tikhonov regularization keeps the solve stable on near-rank-deficient
  // matrices while remaining close to Moore-Penrose for well-conditioned ones.
  const double lambda = std::max(1e-7, rc * rc);

  if (m >= n) {
    Series gram = at.matmul(a).to(ScalarType::Float32).contiguous();
    Series ridge = Series::eye(n, n, ScalarType::Float32)
                       .to(Platform::MPS)
                       .mul(lambda)
                       .contiguous();
    gram = gram.add(ridge).contiguous();
    const Series inv = inverse_mps_native_(gram);
    return inv.matmul(at).to(ScalarType::Float32).contiguous();
  }

  Series gram = a.matmul(at).to(ScalarType::Float32).contiguous();
  Series ridge = Series::eye(m, m, ScalarType::Float32)
                     .to(Platform::MPS)
                     .mul(lambda)
                     .contiguous();
  gram = gram.add(ridge).contiguous();
  const Series inv = inverse_mps_native_(gram);
  return at.matmul(inv).to(ScalarType::Float32).contiguous();
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
  return parse_bool_env_("PLUSS_MPS_LINALG_NATIVE_DEVICE_SVD", false);
}

inline float max_abs_diff_scalar_(const Series& a, const Series& b) {
  return a.sub(b).abs().amax().item<float>();
}

inline Series range_index_mps_(std::int64_t n) {
  std::vector<std::int64_t> idx;
  idx.reserve(static_cast<std::size_t>(std::max<std::int64_t>(0, n)));
  for (std::int64_t i = 0; i < n; ++i) {
    idx.push_back(i);
  }
  return pluss::make_series(std::move(idx), {std::max<std::int64_t>(0, n)}).to(Platform::MPS).contiguous();
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
        return out;
      }
    } catch (const std::exception&) {
    }
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
  if (mps_native_device_svd_enabled_()) {
    try {
      const Series x = canonical_mps_native_f32_(a);
      const auto m = x.sizes()[0];
      const auto n = x.sizes()[1];
      const auto k = std::min(m, n);
      PLUSS_CHECK(k > 0, "subsp::linearalg::svd: expected non-empty matrix");

      const Series gram = x.transpose(0, 1).contiguous().matmul(x).contiguous();
      Series v = Series::eye(n, n, ScalarType::Float32).to(Platform::MPS).contiguous();
      for (int iter = 0; iter < 10; ++iter) {
        const Series z = gram.matmul(v).contiguous();
        v = qr_mps_device_mgs_(z).q.contiguous();
      }

      const Series b = x.matmul(v).contiguous();
      std::vector<Series> u_cols;
      u_cols.reserve(static_cast<std::size_t>(k));
      std::vector<Series> s_scalars;
      s_scalars.reserve(static_cast<std::size_t>(k));
      for (std::int64_t j = 0; j < k; ++j) {
        const Series b_j = b.select(1, j).contiguous();
        const Series s_j = b_j.norm(2.0).to(ScalarType::Float32).contiguous();
        const Series u_j = b_j.div(s_j.add(1e-12)).to(ScalarType::Float32).contiguous();
        u_cols.push_back(u_j);
        s_scalars.push_back(s_j);
      }

      const Series u = stack_dim0_(u_cols, "svd_native_u").transpose(0, 1).contiguous();
      const Series s = stack_dim0_(s_scalars, "svd_native_s").reshape({k}).contiguous();
      const Series vh = v.transpose(0, 1).contiguous().index_select(0, range_index_mps_(k)).contiguous();

      const Series recon = u.matmul(vh.mul(s.reshape({k, 1})));
      const float recon_err = max_abs_diff_scalar_(recon, x);
      if (recon_err <= 8.0e-2f) {
        return NativeSvdResult{u, s, vh};
      }
    } catch (const std::exception&) {
    }
  }
  return svd_mps_cpu_reference_(a);
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

    std::vector<Series> solved_rows(static_cast<std::size_t>(n), Series::undefined());
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
  if (a.dim() > 2) {
    require_defined_(a, "det");
    const auto sz = a.sizes();
    const std::int64_t m = sz[static_cast<std::size_t>(a.dim() - 2)];
    const std::int64_t n = sz[static_cast<std::size_t>(a.dim() - 1)];
    PLUSS_CHECK(m == n, "subsp::linearalg::det: expected square matrices on last two dims");
    if (sz[0] == 0) {
      std::vector<std::int64_t> shape(sz.begin(), sz.end() - 2);
      return Series::zeros(shape, decomp_fallback_dtype_(a.platform())).to(a.platform()).contiguous();
    }
    const Series batch_src = a.contiguous();
    std::vector<Series> det_parts;
    det_parts.reserve(static_cast<std::size_t>(sz[0]));
    for (std::int64_t i = 0; i < sz[0]; ++i) {
      Series mat = batch_src.select(0, i).contiguous();
      det_parts.push_back(det(mat));
    }
    return stack_dim0_(det_parts, "det");
  }
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
  if (a.dim() > 2) {
    require_defined_(a, "inverse");
    const auto sz = a.sizes();
    const std::int64_t m = sz[static_cast<std::size_t>(a.dim() - 2)];
    const std::int64_t n = sz[static_cast<std::size_t>(a.dim() - 1)];
    PLUSS_CHECK(m == n, "subsp::linearalg::inverse: expected square matrices on last two dims");
    if (sz[0] == 0) {
      return Series::zeros(sz, decomp_fallback_dtype_(a.platform())).to(a.platform()).contiguous();
    }
    const Series batch_src = a.contiguous();
    std::vector<Series> inv_parts;
    inv_parts.reserve(static_cast<std::size_t>(sz[0]));
    for (std::int64_t i = 0; i < sz[0]; ++i) {
      Series mat = batch_src.select(0, i).contiguous();
      inv_parts.push_back(inverse(mat));
    }
    return stack_dim0_(inv_parts, "inverse");
  }
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
  if (a.dim() > 2) {
    require_defined_(a, "cholesky");
    const auto sz = a.sizes();
    const std::int64_t m = sz[static_cast<std::size_t>(a.dim() - 2)];
    const std::int64_t n = sz[static_cast<std::size_t>(a.dim() - 1)];
    PLUSS_CHECK(m == n, "subsp::linearalg::cholesky: expected square matrices on last two dims");
    if (sz[0] == 0) {
      return Series::zeros(sz, decomp_fallback_dtype_(a.platform())).to(a.platform()).contiguous();
    }
    const Series batch_src = a.contiguous();
    std::vector<Series> chol_parts;
    chol_parts.reserve(static_cast<std::size_t>(sz[0]));
    for (std::int64_t i = 0; i < sz[0]; ++i) {
      Series mat = batch_src.select(0, i).contiguous();
      chol_parts.push_back(cholesky(mat, upper));
    }
    return stack_dim0_(chol_parts, "cholesky");
  }
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
  if (a.dim() > 2) {
    require_defined_(a, "logdet");
    const auto sz = a.sizes();
    const std::int64_t m = sz[static_cast<std::size_t>(a.dim() - 2)];
    const std::int64_t n = sz[static_cast<std::size_t>(a.dim() - 1)];
    PLUSS_CHECK(m == n, "subsp::linearalg::logdet: expected square matrices on last two dims");
    if (sz[0] == 0) {
      std::vector<std::int64_t> shape(sz.begin(), sz.end() - 2);
      return Series::zeros(shape, decomp_fallback_dtype_(a.platform())).to(a.platform()).contiguous();
    }
    const Series batch_src = a.contiguous();
    std::vector<Series> logdet_parts;
    logdet_parts.reserve(static_cast<std::size_t>(sz[0]));
    for (std::int64_t i = 0; i < sz[0]; ++i) {
      Series mat = batch_src.select(0, i).contiguous();
      logdet_parts.push_back(logdet(mat));
    }
    return stack_dim0_(logdet_parts, "logdet");
  }
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
  if (a.dim() > 2) {
    require_defined_(a, "slogdet");
    const auto sz = a.sizes();
    const std::int64_t m = sz[static_cast<std::size_t>(a.dim() - 2)];
    const std::int64_t n = sz[static_cast<std::size_t>(a.dim() - 1)];
    PLUSS_CHECK(m == n, "subsp::linearalg::slogdet: expected square matrices on last two dims");
    if (sz[0] == 0) {
      std::vector<std::int64_t> shape(sz.begin(), sz.end() - 2);
      shape.push_back(2);
      return Series::zeros(shape, decomp_fallback_dtype_(a.platform())).to(a.platform()).contiguous();
    }
    const Series batch_src = a.contiguous();
    std::vector<Series> slogdet_parts;
    slogdet_parts.reserve(static_cast<std::size_t>(sz[0]));
    for (std::int64_t i = 0; i < sz[0]; ++i) {
      Series mat = batch_src.select(0, i).contiguous();
      slogdet_parts.push_back(slogdet(mat));
    }
    return stack_dim0_(slogdet_parts, "slogdet");
  }
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
  if (a.dim() > 2) {
    require_defined_(a, "matrix_power");
    const auto sz = a.sizes();
    const std::int64_t m = sz[static_cast<std::size_t>(a.dim() - 2)];
    const std::int64_t k = sz[static_cast<std::size_t>(a.dim() - 1)];
    PLUSS_CHECK(m == k, "subsp::linearalg::matrix_power: expected square matrices on last two dims");
    if (sz[0] == 0) {
      return Series::zeros(sz, decomp_fallback_dtype_(a.platform())).to(a.platform()).contiguous();
    }
    std::vector<Series> parts;
    parts.reserve(static_cast<std::size_t>(sz[0]));
    for (std::int64_t i = 0; i < sz[0]; ++i) {
      Series mat = a.select(0, i).contiguous();
      parts.push_back(matrix_power(mat, n));
    }
    return stack_dim0_(parts, "matrix_power");
  }
  if (a.platform() == Platform::CPU) {
    return pluss::subsp::math::linalg::matrix_power(a, n);
  }
  if (a.platform() == Platform::MPS) {
    return matrix_power_native_(a, n);
  }
  throw_platform_not_ready_("matrix_power", a.platform());
}

Series matrix_exp(const Series& a) {
  if (a.dim() > 2) {
    require_defined_(a, "matrix_exp");
    const auto sz = a.sizes();
    const std::int64_t m = sz[static_cast<std::size_t>(a.dim() - 2)];
    const std::int64_t k = sz[static_cast<std::size_t>(a.dim() - 1)];
    PLUSS_CHECK(m == k, "subsp::linearalg::matrix_exp: expected square matrices on last two dims");
    if (sz[0] == 0) {
      return Series::zeros(sz, decomp_fallback_dtype_(a.platform())).to(a.platform()).contiguous();
    }
    std::vector<Series> parts;
    parts.reserve(static_cast<std::size_t>(sz[0]));
    for (std::int64_t i = 0; i < sz[0]; ++i) {
      Series mat = a.select(0, i).contiguous();
      parts.push_back(matrix_exp(mat));
    }
    return stack_dim0_(parts, "matrix_exp");
  }
  if (a.platform() == Platform::CPU) {
    return pluss::subsp::math::linalg::matrix_exp(a);
  }
  if (a.platform() == Platform::MPS) {
    return matrix_exp_native_(a);
  }
  throw_platform_not_ready_("matrix_exp", a.platform());
}

Series pinverse(const Series& a, double rcond) {
  if (a.dim() > 2) {
    require_defined_(a, "pinverse");
    const auto sz = a.sizes();
    if (sz[0] == 0) {
      std::vector<std::int64_t> shape(sz.begin(), sz.end());
      const std::int64_t m = sz[shape.size() - 2];
      const std::int64_t n = sz[shape.size() - 1];
      shape[shape.size() - 2] = n;
      shape[shape.size() - 1] = m;
      return Series::zeros(shape, decomp_fallback_dtype_(a.platform())).to(a.platform()).contiguous();
    }
    const Series batch_src = a.contiguous();
    std::vector<Series> pinv_parts;
    pinv_parts.reserve(static_cast<std::size_t>(sz[0]));
    for (std::int64_t i = 0; i < sz[0]; ++i) {
      Series mat = batch_src.select(0, i).contiguous();
      pinv_parts.push_back(pinverse(mat, rcond));
    }
    return stack_dim0_(pinv_parts, "pinverse");
  }
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
  if (a.dim() > 2) {
    require_defined_(a, "qr_q");
    const auto sz = a.sizes();
    const std::int64_t m = sz[static_cast<std::size_t>(a.dim() - 2)];
    const std::int64_t n = sz[static_cast<std::size_t>(a.dim() - 1)];
    const std::int64_t k = std::min(m, n);
    if (sz[0] == 0) {
      std::vector<std::int64_t> shape(sz.begin(), sz.end());
      shape[shape.size() - 2] = m;
      shape[shape.size() - 1] = reduced ? k : m;
      return Series::zeros(shape, decomp_fallback_dtype_(a.platform())).to(a.platform()).contiguous();
    }
    const Series batch_src = a.contiguous();
    std::vector<Series> q_parts;
    q_parts.reserve(static_cast<std::size_t>(sz[0]));
    for (std::int64_t i = 0; i < sz[0]; ++i) {
      Series mat = batch_src.select(0, i).contiguous();
      q_parts.push_back(qr_q(mat, reduced));
    }
    return stack_dim0_(q_parts, "qr_q");
  }
  if (a.platform() == Platform::CPU) {
    return pluss::subsp::math::linalg::qr(a, reduced).Q;
  }
  if (a.platform() == Platform::MPS) {
    const auto& dispatch = cached_mps_qr_dispatch_(a, reduced);
    if (dispatch.bridged) {
      bump_mps_bridge_counter_(MpsBridgeOp::Qr);
    }
    return dispatch.qr.q;
  }
  throw_platform_not_ready_("qr_q", a.platform());
}

Series qr_r(const Series& a, bool reduced) {
  if (a.dim() > 2) {
    require_defined_(a, "qr_r");
    const auto sz = a.sizes();
    const std::int64_t m = sz[static_cast<std::size_t>(a.dim() - 2)];
    const std::int64_t n = sz[static_cast<std::size_t>(a.dim() - 1)];
    const std::int64_t k = std::min(m, n);
    if (sz[0] == 0) {
      std::vector<std::int64_t> shape(sz.begin(), sz.end());
      shape[shape.size() - 2] = reduced ? k : m;
      shape[shape.size() - 1] = n;
      return Series::zeros(shape, decomp_fallback_dtype_(a.platform())).to(a.platform()).contiguous();
    }
    const Series batch_src = a.contiguous();
    std::vector<Series> r_parts;
    r_parts.reserve(static_cast<std::size_t>(sz[0]));
    for (std::int64_t i = 0; i < sz[0]; ++i) {
      Series mat = batch_src.select(0, i).contiguous();
      r_parts.push_back(qr_r(mat, reduced));
    }
    return stack_dim0_(r_parts, "qr_r");
  }
  if (a.platform() == Platform::CPU) {
    return pluss::subsp::math::linalg::qr(a, reduced).R;
  }
  if (a.platform() == Platform::MPS) {
    const auto& dispatch = cached_mps_qr_dispatch_(a, reduced);
    if (dispatch.bridged) {
      bump_mps_bridge_counter_(MpsBridgeOp::Qr);
    }
    return dispatch.qr.r;
  }
  throw_platform_not_ready_("qr_r", a.platform());
}

Series svd_u(const Series& a, bool full_matrices) {
  if (a.dim() > 2) {
    require_defined_(a, "svd_u");
    const auto sz = a.sizes();
    const std::int64_t m = sz[static_cast<std::size_t>(a.dim() - 2)];
    const std::int64_t n = sz[static_cast<std::size_t>(a.dim() - 1)];
    const std::int64_t k = std::min(m, n);
    if (sz[0] == 0) {
      std::vector<std::int64_t> shape(sz.begin(), sz.end());
      shape[shape.size() - 2] = m;
      shape[shape.size() - 1] = full_matrices ? m : k;
      return Series::zeros(shape, decomp_fallback_dtype_(a.platform())).to(a.platform()).contiguous();
    }
    const Series batch_src = a.contiguous();
    std::vector<Series> u_parts;
    u_parts.reserve(static_cast<std::size_t>(sz[0]));
    for (std::int64_t i = 0; i < sz[0]; ++i) {
      Series mat = batch_src.select(0, i).contiguous();
      u_parts.push_back(svd_u(mat, full_matrices));
    }
    return stack_dim0_(u_parts, "svd_u");
  }
  if (a.platform() == Platform::CPU) {
    return pluss::subsp::math::linalg::svd(a, full_matrices).U;
  }
  if (a.platform() == Platform::MPS) {
    const auto& dispatch = cached_mps_svd_dispatch_(a, full_matrices);
    if (dispatch.bridged) {
      bump_mps_bridge_counter_(MpsBridgeOp::Svd);
    }
    return dispatch.svd.u;
  }
  throw_platform_not_ready_("svd_u", a.platform());
}

Series svd_s(const Series& a, bool full_matrices) {
  if (a.dim() > 2) {
    require_defined_(a, "svd_s");
    const auto sz = a.sizes();
    const std::int64_t m = sz[static_cast<std::size_t>(a.dim() - 2)];
    const std::int64_t n = sz[static_cast<std::size_t>(a.dim() - 1)];
    const std::int64_t k = std::min(m, n);
    if (sz[0] == 0) {
      std::vector<std::int64_t> shape(sz.begin(), sz.end() - 1);
      shape.back() = k;
      return Series::zeros(shape, decomp_fallback_dtype_(a.platform())).to(a.platform()).contiguous();
    }
    const Series batch_src = a.contiguous();
    std::vector<Series> s_parts;
    s_parts.reserve(static_cast<std::size_t>(sz[0]));
    for (std::int64_t i = 0; i < sz[0]; ++i) {
      Series mat = batch_src.select(0, i).contiguous();
      s_parts.push_back(svd_s(mat, full_matrices));
    }
    return stack_dim0_(s_parts, "svd_s");
  }
  if (a.platform() == Platform::CPU) {
    return pluss::subsp::math::linalg::svd(a, full_matrices).S;
  }
  if (a.platform() == Platform::MPS) {
    const auto& dispatch = cached_mps_svd_dispatch_(a, full_matrices);
    if (dispatch.bridged) {
      bump_mps_bridge_counter_(MpsBridgeOp::Svd);
    }
    return dispatch.svd.s;
  }
  throw_platform_not_ready_("svd_s", a.platform());
}

Series svd_vh(const Series& a, bool full_matrices) {
  if (a.dim() > 2) {
    require_defined_(a, "svd_vh");
    const auto sz = a.sizes();
    const std::int64_t m = sz[static_cast<std::size_t>(a.dim() - 2)];
    const std::int64_t n = sz[static_cast<std::size_t>(a.dim() - 1)];
    const std::int64_t k = std::min(m, n);
    if (sz[0] == 0) {
      std::vector<std::int64_t> shape(sz.begin(), sz.end());
      shape[shape.size() - 2] = full_matrices ? n : k;
      shape[shape.size() - 1] = n;
      return Series::zeros(shape, decomp_fallback_dtype_(a.platform())).to(a.platform()).contiguous();
    }
    const Series batch_src = a.contiguous();
    std::vector<Series> vh_parts;
    vh_parts.reserve(static_cast<std::size_t>(sz[0]));
    for (std::int64_t i = 0; i < sz[0]; ++i) {
      Series mat = batch_src.select(0, i).contiguous();
      vh_parts.push_back(svd_vh(mat, full_matrices));
    }
    return stack_dim0_(vh_parts, "svd_vh");
  }
  if (a.platform() == Platform::CPU) {
    return pluss::subsp::math::linalg::svd(a, full_matrices).Vh;
  }
  if (a.platform() == Platform::MPS) {
    const auto& dispatch = cached_mps_svd_dispatch_(a, full_matrices);
    if (dispatch.bridged) {
      bump_mps_bridge_counter_(MpsBridgeOp::Svd);
    }
    return dispatch.svd.vh;
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

    Series a_batch_src = (a.platform() == Platform::MPS) ? a.to(Platform::CPU).contiguous() : a;
    Series b_batch_src = (b.platform() == Platform::MPS) ? b.to(Platform::CPU).contiguous() : b;
    a_batch_src = a_batch_src.expand(a_expand_shape, true).contiguous();
    b_batch_src = b_batch_src.expand(b_expand_shape, true).contiguous();

    if (out_batch.empty()) {
      Series ai = a_batch_src;
      Series bi = b_batch_src;
      if (a.platform() == Platform::MPS) ai = ai.to(Platform::MPS);
      if (b.platform() == Platform::MPS) bi = bi.to(Platform::MPS);
      return triangular_solve(ai, bi, upper, unitriangular);
    }
    if (out_batch[0] == 0) {
      std::vector<std::int64_t> out_tail;
      if (rhs_layout == SolverRhsLayout::Matrix) {
        out_tail = b_tail;
      } else {
        out_tail = {a_tail[0], 1};
      }
      return Series::zeros(
                 concat_shape_(out_batch, out_tail),
                 decomp_fallback_dtype_(a.platform()))
          .to(a.platform())
          .contiguous();
    }

    std::vector<Series> parts;
    parts.reserve(static_cast<std::size_t>(out_batch[0]));
    for (std::int64_t i = 0; i < out_batch[0]; ++i) {
      Series ai = a_batch_src.select(0, i).contiguous();
      Series bi = b_batch_src.select(0, i).contiguous();
      if (a.platform() == Platform::MPS) ai = ai.to(Platform::MPS);
      if (b.platform() == Platform::MPS) bi = bi.to(Platform::MPS);
      Series part = triangular_solve(ai, bi, upper, unitriangular);
      if (a.platform() == Platform::MPS && part.platform() == Platform::MPS) {
        part = part.to(Platform::CPU).contiguous();
      }
      parts.push_back(std::move(part));
    }
    Series out = stack_dim0_(parts, op);
    if (a.platform() == Platform::MPS) {
      out = out.to(Platform::MPS).contiguous();
    }
    return out;
  }

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
  if (b.dim() == 1) {
    std::vector<std::int64_t> shape{b.sizes()[0], 1};
    return cholesky_solve(factor, b.reshape(shape), upper);
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

    Series f_batch_src = (factor.platform() == Platform::MPS) ? factor.to(Platform::CPU).contiguous() : factor;
    Series b_batch_src = (b.platform() == Platform::MPS) ? b.to(Platform::CPU).contiguous() : b;
    f_batch_src = f_batch_src.expand(f_expand_shape, true).contiguous();
    b_batch_src = b_batch_src.expand(b_expand_shape, true).contiguous();

    if (out_batch.empty()) {
      Series fi = f_batch_src;
      Series bi = b_batch_src;
      if (factor.platform() == Platform::MPS) fi = fi.to(Platform::MPS);
      if (b.platform() == Platform::MPS) bi = bi.to(Platform::MPS);
      return cholesky_solve(fi, bi, upper);
    }
    if (out_batch[0] == 0) {
      std::vector<std::int64_t> out_tail;
      if (rhs_layout == SolverRhsLayout::Matrix) {
        out_tail = b_tail;
      } else {
        out_tail = {f_tail[0], 1};
      }
      return Series::zeros(
                 concat_shape_(out_batch, out_tail),
                 decomp_fallback_dtype_(factor.platform()))
          .to(factor.platform())
          .contiguous();
    }

    std::vector<Series> parts;
    parts.reserve(static_cast<std::size_t>(out_batch[0]));
    for (std::int64_t i = 0; i < out_batch[0]; ++i) {
      Series fi = f_batch_src.select(0, i).contiguous();
      Series bi = b_batch_src.select(0, i).contiguous();
      if (factor.platform() == Platform::MPS) fi = fi.to(Platform::MPS);
      if (b.platform() == Platform::MPS) bi = bi.to(Platform::MPS);
      Series part = cholesky_solve(fi, bi, upper);
      if (factor.platform() == Platform::MPS && part.platform() == Platform::MPS) {
        part = part.to(Platform::CPU).contiguous();
      }
      parts.push_back(std::move(part));
    }
    Series out = stack_dim0_(parts, op);
    if (factor.platform() == Platform::MPS) {
      out = out.to(Platform::MPS).contiguous();
    }
    return out;
  }

  if (factor.platform() == Platform::CPU) {
    return pluss::subsp::math::linalg::cholesky_solve(b, factor, upper);
  }
  if (factor.platform() == Platform::MPS) {
    if (mps_experimental_native_enabled_()) {
      const Series native_factor = canonical_mps_native_f32_(factor);
      const Series native_b = canonical_mps_native_f32_(b);
      if (pluss::codem::native::mps::supports_cholesky_solve(native_factor, native_b)) {
        return pluss::codem::native::mps::cholesky_solve(native_factor, native_b, upper);
      }
    }
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
  if (factor.dim() > 2) {
    const auto batch = factor.sizes()[0];
    if (batch == 0) {
      return Series::zeros(factor.sizes(), decomp_fallback_dtype_(factor.platform())).to(factor.platform()).contiguous();
    }
    const Series f_batch_src =
        (factor.platform() == Platform::MPS) ? factor.to(Platform::CPU).contiguous() : factor;
    std::vector<Series> parts;
    parts.reserve(static_cast<std::size_t>(batch));
    for (std::int64_t i = 0; i < batch; ++i) {
      Series fi = f_batch_src.select(0, i).contiguous();
      if (factor.platform() == Platform::MPS) {
        fi = fi.to(Platform::MPS);
      }
      parts.push_back(cholesky_inverse(fi, upper));
    }
    return stack_dim0_(parts, "cholesky_inverse");
  }

  if (factor.platform() == Platform::CPU) {
    return pluss::subsp::math::linalg::cholesky_inverse(factor, upper);
  }
  if (factor.platform() == Platform::MPS) {
    if (mps_experimental_native_enabled_()) {
      const Series native_factor = canonical_mps_native_f32_(factor);
      if (pluss::codem::native::mps::supports_cholesky_factor(native_factor)) {
        const auto n = native_factor.sizes()[0];
        const Series eye = Series::eye(n, n, ScalarType::Float32).to(Platform::MPS).contiguous();
        return pluss::codem::native::mps::cholesky_solve(native_factor, eye, upper);
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
      return triangular_solve_mps_native_(l.transpose(0, 1).contiguous(), y, /*upper=*/true, /*unitriangular=*/false);
    }
    return bridge_cpu_result_to_mps_(
        pluss::subsp::math::linalg::cholesky_inverse(factor.to(Platform::CPU), upper),
        MpsBridgeOp::CholeskyInverse,
        "cholesky_inverse");
  }
  throw_platform_not_ready_("cholesky_inverse", factor.platform());
}

Series lu_solve(const Series& lu, const Series& b) {
  require_same_platform_(lu, b, "lu_solve");
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

    Series lu_batch_src = (lu.platform() == Platform::MPS) ? lu.to(Platform::CPU).contiguous() : lu;
    Series b_batch_src = (b.platform() == Platform::MPS) ? b.to(Platform::CPU).contiguous() : b;
    lu_batch_src = lu_batch_src.expand(lu_expand_shape, true).contiguous();
    b_batch_src = b_batch_src.expand(b_expand_shape, true).contiguous();

    if (out_batch.empty()) {
      Series lui = lu_batch_src;
      Series bi = b_batch_src;
      if (lu.platform() == Platform::MPS) lui = lui.to(Platform::MPS);
      if (b.platform() == Platform::MPS) bi = bi.to(Platform::MPS);
      return lu_solve(lui, bi);
    }
    if (out_batch[0] == 0) {
      std::vector<std::int64_t> out_tail;
      if (rhs_layout == SolverRhsLayout::Matrix) {
        out_tail = b_tail;
      } else {
        out_tail = {lu_tail[0], 1};
      }
      return Series::zeros(
                 concat_shape_(out_batch, out_tail),
                 decomp_fallback_dtype_(lu.platform()))
          .to(lu.platform())
          .contiguous();
    }

    std::vector<Series> parts;
    parts.reserve(static_cast<std::size_t>(out_batch[0]));
    for (std::int64_t i = 0; i < out_batch[0]; ++i) {
      Series lui = lu_batch_src.select(0, i).contiguous();
      Series bi = b_batch_src.select(0, i).contiguous();
      if (lu.platform() == Platform::MPS) lui = lui.to(Platform::MPS);
      if (b.platform() == Platform::MPS) bi = bi.to(Platform::MPS);
      Series part = lu_solve(lui, bi);
      if (lu.platform() == Platform::MPS && part.platform() == Platform::MPS) {
        part = part.to(Platform::CPU).contiguous();
      }
      parts.push_back(std::move(part));
    }
    Series out = stack_dim0_(parts, op);
    if (lu.platform() == Platform::MPS) {
      out = out.to(Platform::MPS).contiguous();
    }
    return out;
  }

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
