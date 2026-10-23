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
  CuBLAS = 6,
};

enum class LinearAlgSolverProvider : std::uint8_t {
  None = 0,
  Accelerate = 1,
  Reference = 2,
  Metal = 3,
  CuSolver = 4,
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
  bool supports_eigh = false;
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
  std::uint64_t mps_native_qr_device_success = 0;
  std::uint64_t mps_native_qr_device_fallback = 0;
  std::uint64_t mps_native_svd_device_success = 0;
  std::uint64_t mps_native_svd_device_fallback = 0;
};

struct SvdDecomposition;
struct EigenDecomposition;
struct GenEigResult;

struct GenEigOptions {
  std::int64_t max_iterations = 64;
  double relative_tolerance = 1e-8;
  double absolute_tolerance = 1e-10;
};

template <class ApplyA, class ApplyB, class SolveA>
GenEigResult gen_eigh(const pluss::codem::Series &initial, ApplyA &&apply_a,
                      ApplyB &&apply_b, SolveA &&solve_a,
                      const GenEigOptions &options = {});

struct MatrixEqOptions {
  std::int64_t max_iterations = 100;
  std::int64_t check_interval = 2;
  double tolerance = 1e-10;
};

CODEM_API const char *
linearalg_dense_provider_name(LinearAlgDenseProvider provider) noexcept;
CODEM_API const char *
linearalg_solver_provider_name(LinearAlgSolverProvider provider) noexcept;
CODEM_API LinearAlgBackendInfo
linearalg_backend_info(LinearAlgTarget target = LinearAlgTarget::CPU) noexcept;
CODEM_API bool linearalg_mps_cpu_bridge_enabled() noexcept;
CODEM_API bool linearalg_mps_experimental_native_enabled() noexcept;
CODEM_API LinearAlgDispatchCounters linearalg_dispatch_counters() noexcept;
CODEM_API void linearalg_reset_dispatch_counters() noexcept;

CODEM_API pluss::codem::Series matmul(const pluss::codem::Series &a,
                                      const pluss::codem::Series &b);
CODEM_API pluss::codem::Series mm(const pluss::codem::Series &a,
                                  const pluss::codem::Series &b);
CODEM_API pluss::codem::Series mv(const pluss::codem::Series &a,
                                  const pluss::codem::Series &b);
CODEM_API pluss::codem::Series dot(const pluss::codem::Series &a,
                                   const pluss::codem::Series &b);
CODEM_API pluss::codem::Series vdot(const pluss::codem::Series &a,
                                    const pluss::codem::Series &b);
CODEM_API pluss::codem::Series norm(const pluss::codem::Series &x);

CODEM_API pluss::codem::Series det(const pluss::codem::Series &a);
CODEM_API pluss::codem::Series inverse(const pluss::codem::Series &a);
CODEM_API pluss::codem::Series cholesky(const pluss::codem::Series &a,
                                        bool upper = false);
CODEM_API pluss::codem::Series logdet(const pluss::codem::Series &a);
CODEM_API pluss::codem::Series slogdet(const pluss::codem::Series &a);
CODEM_API pluss::codem::Series matrix_power(const pluss::codem::Series &a,
                                            std::int64_t n);
CODEM_API pluss::codem::Series matrix_exp(const pluss::codem::Series &a);
CODEM_API pluss::codem::Series pinverse(const pluss::codem::Series &a,
                                        double rcond = 1e-12);
CODEM_API pluss::codem::Series qr_q(const pluss::codem::Series &a,
                                    bool reduced = true);
CODEM_API pluss::codem::Series qr_r(const pluss::codem::Series &a,
                                    bool reduced = true);
CODEM_API pluss::codem::Series svd_u(const pluss::codem::Series &a,
                                     bool full_matrices = true);
CODEM_API pluss::codem::Series svd_s(const pluss::codem::Series &a,
                                     bool full_matrices = true);
CODEM_API pluss::codem::Series svd_vh(const pluss::codem::Series &a,
                                      bool full_matrices = true);
CODEM_API SvdDecomposition svd(const pluss::codem::Series &a,
                               bool full_matrices = true);
CODEM_API EigenDecomposition eigh(const pluss::codem::Series &a,
                                  bool upper = false);
CODEM_API pluss::codem::Series eigvalsh(const pluss::codem::Series &a,
                                        bool upper = false);

// Matrix equations use the trailing two axes as matrix axes and broadcast all
// leading axes. `sylv` solves A X + X B = C; the remaining functions use the
// standard control-theory signs documented at their public accessors.
CODEM_API pluss::codem::Series sylv(const pluss::codem::Series &a,
                                    const pluss::codem::Series &b,
                                    const pluss::codem::Series &c);
CODEM_API pluss::codem::Series lyap(
    const pluss::codem::Series &a, const pluss::codem::Series &q,
    MatrixEqOptions options = {});
CODEM_API pluss::codem::Series dlyap(
    const pluss::codem::Series &a, const pluss::codem::Series &q,
    MatrixEqOptions options = {});
CODEM_API pluss::codem::Series care(
    const pluss::codem::Series &a, const pluss::codem::Series &g,
    const pluss::codem::Series &q, MatrixEqOptions options = {});
CODEM_API pluss::codem::Series dare(
    const pluss::codem::Series &a, const pluss::codem::Series &g,
    const pluss::codem::Series &q, MatrixEqOptions options = {});

CODEM_API pluss::codem::Series triangular_solve(const pluss::codem::Series &a,
                                                const pluss::codem::Series &b,
                                                bool upper = true,
                                                bool unitriangular = false);
CODEM_API pluss::codem::Series
cholesky_solve(const pluss::codem::Series &factor,
               const pluss::codem::Series &b, bool upper = false);
CODEM_API pluss::codem::Series
cholesky_inverse(const pluss::codem::Series &factor, bool upper = false);
CODEM_API pluss::codem::Series lu_solve(const pluss::codem::Series &lu,
                                        const pluss::codem::Series &b);

} // namespace pluss::subsp::linearalg
