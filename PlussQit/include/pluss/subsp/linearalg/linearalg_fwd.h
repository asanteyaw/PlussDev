#pragma once

#include "pluss/export.h"

#include <cstdint>

namespace pluss::codem {
class Series;
}

namespace pluss::subsp::linearalg {

enum class LinearAlgTarget : std::uint8_t {
  CPU = 0,
  MPS = 1,
  CUDA = 2,
};

enum class LinearAlgDenseProvider : std::uint8_t {
  None = 0,
  Accelerate = 1,
  OpenBLAS = 2,
  OneMKL = 3,
  CBLAS = 4,
  Metal = 5,
};

enum class LinearAlgSolverProvider : std::uint8_t {
  None = 0,
  Accelerate = 1,
  Reference = 2,
  Metal = 3,
};

struct LinearAlgBackendInfo {
  LinearAlgTarget target = LinearAlgTarget::CPU;
  LinearAlgDenseProvider dense_provider = LinearAlgDenseProvider::None;
  LinearAlgSolverProvider solver_provider = LinearAlgSolverProvider::None;
  bool dense_available = false;
  bool solver_available = false;
  bool supports_matmul = false;
  bool supports_det = false;
  bool supports_inverse = false;
  bool supports_cholesky = false;
  bool supports_qr = false;
  bool supports_svd = false;
  bool supports_triangular_solve = false;
  bool supports_cholesky_solve = false;
  bool supports_lu_solve = false;
  bool supports_matrix_power = false;
  bool supports_matrix_exp = false;
};

struct LinearAlgDispatchCounters {
  std::uint64_t mps_bridge_total = 0;
  std::uint64_t mps_bridge_det = 0;
  std::uint64_t mps_bridge_inverse = 0;
  std::uint64_t mps_bridge_cholesky = 0;
  std::uint64_t mps_bridge_logdet = 0;
  std::uint64_t mps_bridge_slogdet = 0;
  std::uint64_t mps_bridge_pinverse = 0;
  std::uint64_t mps_bridge_triangular_solve = 0;
  std::uint64_t mps_bridge_cholesky_solve = 0;
  std::uint64_t mps_bridge_cholesky_inverse = 0;
  std::uint64_t mps_bridge_lu_solve = 0;
  std::uint64_t mps_bridge_qr = 0;
  std::uint64_t mps_bridge_svd = 0;
};

CODEM_API const char* linearalg_dense_provider_name(LinearAlgDenseProvider provider) noexcept;
CODEM_API const char* linearalg_solver_provider_name(LinearAlgSolverProvider provider) noexcept;
CODEM_API LinearAlgBackendInfo linearalg_backend_info(LinearAlgTarget target = LinearAlgTarget::CPU) noexcept;
CODEM_API bool linearalg_mps_cpu_bridge_enabled() noexcept;
CODEM_API LinearAlgDispatchCounters linearalg_dispatch_counters() noexcept;
CODEM_API void linearalg_reset_dispatch_counters() noexcept;

CODEM_API pluss::codem::Series matmul(const pluss::codem::Series& a, const pluss::codem::Series& b);
CODEM_API pluss::codem::Series mm(const pluss::codem::Series& a, const pluss::codem::Series& b);
CODEM_API pluss::codem::Series mv(const pluss::codem::Series& a, const pluss::codem::Series& b);
CODEM_API pluss::codem::Series dot(const pluss::codem::Series& a, const pluss::codem::Series& b);
CODEM_API pluss::codem::Series vdot(const pluss::codem::Series& a, const pluss::codem::Series& b);
CODEM_API pluss::codem::Series norm(const pluss::codem::Series& x);

CODEM_API pluss::codem::Series det(const pluss::codem::Series& a);
CODEM_API pluss::codem::Series inverse(const pluss::codem::Series& a);
CODEM_API pluss::codem::Series cholesky(const pluss::codem::Series& a, bool upper = false);
CODEM_API pluss::codem::Series logdet(const pluss::codem::Series& a);
CODEM_API pluss::codem::Series slogdet(const pluss::codem::Series& a);
CODEM_API pluss::codem::Series matrix_power(const pluss::codem::Series& a, std::int64_t n);
CODEM_API pluss::codem::Series matrix_exp(const pluss::codem::Series& a);
CODEM_API pluss::codem::Series pinverse(const pluss::codem::Series& a, double rcond = 1e-12);
CODEM_API pluss::codem::Series qr_q(const pluss::codem::Series& a, bool reduced = true);
CODEM_API pluss::codem::Series qr_r(const pluss::codem::Series& a, bool reduced = true);
CODEM_API pluss::codem::Series svd_u(const pluss::codem::Series& a, bool full_matrices = true);
CODEM_API pluss::codem::Series svd_s(const pluss::codem::Series& a, bool full_matrices = true);
CODEM_API pluss::codem::Series svd_vh(const pluss::codem::Series& a, bool full_matrices = true);

CODEM_API pluss::codem::Series triangular_solve(const pluss::codem::Series& a,
                                                const pluss::codem::Series& b,
                                                bool upper = true,
                                                bool unitriangular = false);
CODEM_API pluss::codem::Series cholesky_solve(const pluss::codem::Series& factor,
                                              const pluss::codem::Series& b,
                                              bool upper = false);
CODEM_API pluss::codem::Series cholesky_inverse(const pluss::codem::Series& factor, bool upper = false);
CODEM_API pluss::codem::Series lu_solve(const pluss::codem::Series& lu, const pluss::codem::Series& b);

} // namespace pluss::subsp::linearalg
