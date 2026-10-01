#include "CodeM/native/cuda/LinalgOps.h"

#include "CodeM/native/cuda/BaselineOps.h"
#include "CodeM/native/cuda/CUDARuntime.h"

#include <array>
#include <cctype>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <vector>

#if defined(__unix__) || defined(__APPLE__)
#include <dlfcn.h>
#define PLUSS_CODEM_HAS_DLOPEN 1
#else
#define PLUSS_CODEM_HAS_DLOPEN 0
#endif

#if defined(PLUSS_HAS_CUDA) && PLUSS_HAS_CUDA && __has_include(<cuda_runtime_api.h>) && __has_include(<cusolverDn.h>) && __has_include(<cublas_v2.h>)
#include <cublas_v2.h>
#include <cuda_runtime_api.h>
#include <cusolverDn.h>
#define PLUSS_CODEM_HAS_CUSOLVER_LINALG 1
#else
#define PLUSS_CODEM_HAS_CUSOLVER_LINALG 0
#endif

namespace pluss::codem::native::cuda {
namespace {

bool ieq_(const char* a, const char* b) noexcept {
  if (a == nullptr || b == nullptr) return false;
  while (*a != '\0' && *b != '\0') {
    if (std::tolower(static_cast<unsigned char>(*a)) != std::tolower(static_cast<unsigned char>(*b))) {
      return false;
    }
    ++a;
    ++b;
  }
  return *a == '\0' && *b == '\0';
}

bool parse_bool_env_(const char* name, bool default_value) noexcept {
  const char* raw = std::getenv(name);
  if (raw == nullptr || *raw == '\0') {
    return default_value;
  }
  if (ieq_(raw, "1") || ieq_(raw, "true") || ieq_(raw, "yes") || ieq_(raw, "on")) {
    return true;
  }
  if (ieq_(raw, "0") || ieq_(raw, "false") || ieq_(raw, "no") || ieq_(raw, "off")) {
    return false;
  }
  return default_value;
}

bool supports_linalg_dtype_(ScalarType dtype) noexcept {
  switch (dtype) {
    case ScalarType::Float16:
    case ScalarType::BFloat16:
    case ScalarType::Float32:
    case ScalarType::Float64:
      return true;
    default:
      return false;
  }
}

bool linalg_baseline_ready_() noexcept {
  return runtime_available() && native_baseline_enabled() && native_linalg_enabled();
}

bool is_cuda_matrix_like_(const Series& a) noexcept {
  return a.platform() == Platform::CUDA && a.dim() >= 2;
}

std::int64_t tail_rows_(const Series& a) noexcept {
  return a.sizes()[static_cast<std::size_t>(a.dim() - 2)];
}

std::int64_t tail_cols_(const Series& a) noexcept {
  return a.sizes()[static_cast<std::size_t>(a.dim() - 1)];
}

bool is_square_cuda_matrix_like_(const Series& a) noexcept {
  return is_cuda_matrix_like_(a) && tail_rows_(a) == tail_cols_(a);
}

bool is_cuda_rhs_for_square_(const Series& b, std::int64_t n) noexcept {
  if (b.platform() != Platform::CUDA || b.dim() < 1) {
    return false;
  }
  if (b.dim() == 1) {
    return b.sizes()[0] == n;
  }
  return b.sizes()[static_cast<std::size_t>(b.dim() - 2)] == n;
}

} // namespace

bool native_linalg_enabled() noexcept {
  return parse_bool_env_("PLUSS_CUDA_NATIVE_LINALG", false);
}

bool native_linalg_strict() noexcept {
  return parse_bool_env_("PLUSS_CUDA_NATIVE_LINALG_STRICT", false);
}

bool supports_det_baseline(ScalarType dtype) noexcept {
  return linalg_baseline_ready_() && supports_linalg_dtype_(dtype);
}

bool supports_inverse_baseline(ScalarType dtype) noexcept {
  return linalg_baseline_ready_() && supports_linalg_dtype_(dtype);
}

bool supports_cholesky_baseline(ScalarType dtype) noexcept {
  return linalg_baseline_ready_() && supports_linalg_dtype_(dtype);
}

bool supports_logdet_baseline(ScalarType dtype) noexcept {
  return supports_det_baseline(dtype);
}

bool supports_slogdet_baseline(ScalarType dtype) noexcept {
  return supports_det_baseline(dtype);
}

bool supports_pinverse_baseline(ScalarType dtype) noexcept {
  return linalg_baseline_ready_() && supports_linalg_dtype_(dtype);
}

bool supports_triangular_solve_baseline(ScalarType a_dtype, ScalarType b_dtype) noexcept {
  return linalg_baseline_ready_() &&
      supports_linalg_dtype_(a_dtype) &&
      supports_linalg_dtype_(b_dtype) &&
      a_dtype == b_dtype;
}

bool supports_cholesky_solve_baseline(ScalarType factor_dtype, ScalarType b_dtype) noexcept {
  return supports_triangular_solve_baseline(factor_dtype, b_dtype);
}

bool supports_cholesky_inverse_baseline(ScalarType factor_dtype) noexcept {
  return linalg_baseline_ready_() && supports_linalg_dtype_(factor_dtype);
}

bool supports_lu_solve_baseline(ScalarType lu_dtype, ScalarType b_dtype) noexcept {
  return supports_triangular_solve_baseline(lu_dtype, b_dtype);
}

bool supports_qr_baseline(ScalarType dtype, bool reduced) noexcept {
  return reduced && linalg_baseline_ready_() && supports_linalg_dtype_(dtype);
}

bool supports_svd_baseline(ScalarType dtype, bool full_matrices) noexcept {
  return !full_matrices && linalg_baseline_ready_() && supports_linalg_dtype_(dtype);
}

bool supports_matrix_power_baseline(ScalarType dtype) noexcept {
  return supports_matmul_lane(dtype, dtype);
}

bool supports_matrix_exp_baseline(ScalarType dtype) noexcept {
  return supports_matmul_lane(dtype, dtype);
}

bool supports_det(const Series& a) noexcept {
  return supports_det_baseline(a.scalar_type()) && is_square_cuda_matrix_like_(a);
}

bool supports_inverse(const Series& a) noexcept {
  return supports_inverse_baseline(a.scalar_type()) && is_square_cuda_matrix_like_(a);
}

bool supports_cholesky(const Series& a) noexcept {
  return supports_cholesky_baseline(a.scalar_type()) && is_square_cuda_matrix_like_(a);
}

bool supports_logdet(const Series& a) noexcept {
  return supports_logdet_baseline(a.scalar_type()) && is_square_cuda_matrix_like_(a);
}

bool supports_slogdet(const Series& a) noexcept {
  return supports_slogdet_baseline(a.scalar_type()) && is_square_cuda_matrix_like_(a);
}

bool supports_pinverse(const Series& a) noexcept {
  return supports_pinverse_baseline(a.scalar_type()) &&
      is_cuda_matrix_like_(a) &&
      tail_rows_(a) > 0 &&
      tail_cols_(a) > 0;
}

bool supports_triangular_solve(const Series& a, const Series& b) noexcept {
  return supports_triangular_solve_baseline(a.scalar_type(), b.scalar_type()) &&
      is_square_cuda_matrix_like_(a) &&
      is_cuda_rhs_for_square_(b, tail_rows_(a));
}

bool supports_cholesky_solve(const Series& factor, const Series& b) noexcept {
  return supports_cholesky_solve_baseline(factor.scalar_type(), b.scalar_type()) &&
      is_square_cuda_matrix_like_(factor) &&
      is_cuda_rhs_for_square_(b, tail_rows_(factor));
}

bool supports_cholesky_inverse(const Series& factor) noexcept {
  return supports_cholesky_inverse_baseline(factor.scalar_type()) && is_square_cuda_matrix_like_(factor);
}

bool supports_lu_solve(const Series& lu, const Series& b) noexcept {
  return supports_lu_solve_baseline(lu.scalar_type(), b.scalar_type()) &&
      is_square_cuda_matrix_like_(lu) &&
      is_cuda_rhs_for_square_(b, tail_rows_(lu));
}

bool supports_qr(const Series& a, bool reduced) noexcept {
  return supports_qr_baseline(a.scalar_type(), reduced) &&
      is_cuda_matrix_like_(a) &&
      tail_rows_(a) > 0 &&
      tail_cols_(a) > 0;
}

bool supports_svd(const Series& a, bool full_matrices) noexcept {
  return supports_svd_baseline(a.scalar_type(), full_matrices) &&
      is_cuda_matrix_like_(a) &&
      tail_rows_(a) > 0 &&
      tail_cols_(a) > 0;
}

bool supports_matrix_power(const Series& a) noexcept {
  return supports_matrix_power_baseline(a.scalar_type()) && is_square_cuda_matrix_like_(a);
}

bool supports_matrix_exp(const Series& a) noexcept {
  return supports_matrix_exp_baseline(a.scalar_type()) && is_square_cuda_matrix_like_(a);
}

#if PLUSS_CODEM_HAS_CUSOLVER_LINALG && PLUSS_CODEM_HAS_DLOPEN

struct CusolverApi {
  decltype(&cusolverDnCreate) create{nullptr};
  decltype(&cusolverDnDestroy) destroy{nullptr};
  decltype(&cusolverDnSgetrf_bufferSize) sgetrf_buffer_size{nullptr};
  decltype(&cusolverDnDgetrf_bufferSize) dgetrf_buffer_size{nullptr};
  decltype(&cusolverDnSgetrf) sgetrf{nullptr};
  decltype(&cusolverDnDgetrf) dgetrf{nullptr};
  decltype(&cusolverDnSgetrs) sgetrs{nullptr};
  decltype(&cusolverDnDgetrs) dgetrs{nullptr};

  bool load(void* handle, std::string* why) {
    auto load_sym = [&](auto& fn, const char* name) -> bool {
      fn = reinterpret_cast<std::remove_reference_t<decltype(fn)>>(dlsym(handle, name));
      if (fn == nullptr) {
        if (why != nullptr) {
          *why = std::string("cuSolver symbol missing: ") + name;
        }
        return false;
      }
      return true;
    };
    return load_sym(create, "cusolverDnCreate") &&
        load_sym(destroy, "cusolverDnDestroy") &&
        load_sym(sgetrf_buffer_size, "cusolverDnSgetrf_bufferSize") &&
        load_sym(dgetrf_buffer_size, "cusolverDnDgetrf_bufferSize") &&
        load_sym(sgetrf, "cusolverDnSgetrf") &&
        load_sym(dgetrf, "cusolverDnDgetrf") &&
        load_sym(sgetrs, "cusolverDnSgetrs") &&
        load_sym(dgetrs, "cusolverDnDgetrs");
  }
};

struct CusolverContext {
  bool ready{false};
  std::string error{};
  void* handle{nullptr};
  CusolverApi api{};
};

CusolverContext& cusolver_ctx_() {
  static CusolverContext ctx;
  static std::once_flag once;
  std::call_once(once, [&]() {
    constexpr const char* libs[] = {
#if defined(__APPLE__)
        "libcusolver.dylib",
#else
        "libcusolver.so.11",
        "libcusolver.so.10",
        "libcusolver.so",
#endif
        nullptr,
    };
    if (!runtime_available()) {
      ctx.error = "CUDA runtime unavailable";
      return;
    }
    if (!cusolver_provider_available()) {
      ctx.error = std::string("cuSolver provider unavailable: ") + cusolver_provider_error_cstr();
      return;
    }
    for (const char* lib : libs) {
      if (lib == nullptr || *lib == '\0') continue;
      void* handle = dlopen(lib, RTLD_LAZY | RTLD_LOCAL);
      if (handle == nullptr) continue;
      std::string why{};
      if (!ctx.api.load(handle, &why)) {
        ctx.error = why;
        dlclose(handle);
        continue;
      }
      ctx.handle = handle;
      ctx.ready = true;
      ctx.error.clear();
      return;
    }
    if (ctx.error.empty()) {
      ctx.error = "cuSolver library not found on loader path";
    }
  });
  return ctx;
}

struct SolverHandleGuard {
  const CusolverApi* api{nullptr};
  cusolverDnHandle_t value{nullptr};
  ~SolverHandleGuard() {
    if (api != nullptr && value != nullptr && api->destroy != nullptr) {
      (void)api->destroy(value);
    }
  }
};

struct DeviceBuffer {
  void* ptr{nullptr};
  std::size_t bytes{0};
  ~DeviceBuffer() {
    if (ptr != nullptr) {
      (void)cudaFree(ptr);
    }
  }
};

std::size_t bytes_of_(const Series& s) noexcept {
  return static_cast<std::size_t>(s.numel()) * static_cast<std::size_t>(s.itemsize());
}

bool alloc_device_buffer_(DeviceBuffer* buf, std::size_t bytes, std::string* reason) {
  if (buf == nullptr) {
    if (reason != nullptr) *reason = "internal: null device buffer";
    return false;
  }
  buf->bytes = bytes;
  if (bytes == 0) return true;
  const cudaError_t st = cudaMalloc(&buf->ptr, bytes);
  if (st == cudaSuccess) return true;
  if (reason != nullptr) {
    *reason = std::string("cudaMalloc failed: ") + cudaGetErrorString(st);
  }
  return false;
}

bool copy_h2d_(void* dst, const void* src, std::size_t bytes, std::string* reason) {
  if (bytes == 0) return true;
  const cudaError_t st = cudaMemcpy(dst, src, bytes, cudaMemcpyHostToDevice);
  if (st == cudaSuccess) return true;
  if (reason != nullptr) {
    *reason = std::string("cudaMemcpy(H2D) failed: ") + cudaGetErrorString(st);
  }
  return false;
}

bool copy_d2h_(void* dst, const void* src, std::size_t bytes, std::string* reason) {
  if (bytes == 0) return true;
  const cudaError_t st = cudaMemcpy(dst, src, bytes, cudaMemcpyDeviceToHost);
  if (st == cudaSuccess) return true;
  if (reason != nullptr) {
    *reason = std::string("cudaMemcpy(D2H) failed: ") + cudaGetErrorString(st);
  }
  return false;
}

bool validate_lu_input_(const Series& a, std::string* reason) {
  if (a.platform() != Platform::CUDA) {
    if (reason != nullptr) *reason = "input is not CUDA";
    return false;
  }
  if (a.dim() != 2) {
    if (reason != nullptr) *reason = "only rank-2 matrices are supported";
    return false;
  }
  if (a.sizes()[0] != a.sizes()[1]) {
    if (reason != nullptr) *reason = "matrix must be square";
    return false;
  }
  if (!a.is_contiguous()) {
    if (reason != nullptr) *reason = "matrix must be contiguous";
    return false;
  }
  if (a.scalar_type() != ScalarType::Float32 && a.scalar_type() != ScalarType::Float64) {
    if (reason != nullptr) *reason = "cuSolver det/inverse currently supports Float32/Float64 only";
    return false;
  }
  return true;
}

bool read_device_info_(DeviceBuffer& d_info, int* info, std::string* reason) {
  if (info == nullptr) {
    if (reason != nullptr) *reason = "internal: null info output";
    return false;
  }
  if (!copy_d2h_(info, d_info.ptr, sizeof(int), reason)) return false;
  return true;
}

bool lu_factorize_(
    const Series& a,
    DeviceBuffer* d_lu,
    DeviceBuffer* d_ipiv,
    int* info,
    std::string* reason) {
  if (d_lu == nullptr || d_ipiv == nullptr || info == nullptr) {
    if (reason != nullptr) *reason = "internal: null LU outputs";
    return false;
  }
  const CusolverContext& ctx = cusolver_ctx_();
  if (!ctx.ready) {
    if (reason != nullptr) *reason = ctx.error;
    return false;
  }
  const int n = static_cast<int>(a.sizes()[0]);

  if (!alloc_device_buffer_(d_lu, bytes_of_(a), reason)) return false;
  if (!copy_h2d_(d_lu->ptr, a.raw_data(), d_lu->bytes, reason)) return false;
  if (!alloc_device_buffer_(d_ipiv, static_cast<std::size_t>(n) * sizeof(int), reason)) return false;

  DeviceBuffer d_info{};
  if (!alloc_device_buffer_(&d_info, sizeof(int), reason)) return false;

  SolverHandleGuard handle{&ctx.api};
  if (ctx.api.create(&handle.value) != CUSOLVER_STATUS_SUCCESS || handle.value == nullptr) {
    if (reason != nullptr) *reason = "cusolverDnCreate failed";
    return false;
  }

  int lwork = 0;
  if (a.scalar_type() == ScalarType::Float32) {
    if (ctx.api.sgetrf_buffer_size(handle.value, n, n, reinterpret_cast<float*>(d_lu->ptr), n, &lwork) != CUSOLVER_STATUS_SUCCESS) {
      if (reason != nullptr) *reason = "cusolverDnSgetrf_bufferSize failed";
      return false;
    }
  } else {
    if (ctx.api.dgetrf_buffer_size(handle.value, n, n, reinterpret_cast<double*>(d_lu->ptr), n, &lwork) != CUSOLVER_STATUS_SUCCESS) {
      if (reason != nullptr) *reason = "cusolverDnDgetrf_bufferSize failed";
      return false;
    }
  }
  if (lwork <= 0) {
    if (reason != nullptr) *reason = "cusolverDn getrf returned non-positive workspace";
    return false;
  }

  DeviceBuffer d_work{};
  if (!alloc_device_buffer_(
          &d_work,
          static_cast<std::size_t>(lwork) * (a.scalar_type() == ScalarType::Float32 ? sizeof(float) : sizeof(double)),
          reason)) {
    return false;
  }

  if (a.scalar_type() == ScalarType::Float32) {
    if (ctx.api.sgetrf(
            handle.value,
            n,
            n,
            reinterpret_cast<float*>(d_lu->ptr),
            n,
            reinterpret_cast<float*>(d_work.ptr),
            reinterpret_cast<int*>(d_ipiv->ptr),
            reinterpret_cast<int*>(d_info.ptr)) != CUSOLVER_STATUS_SUCCESS) {
      if (reason != nullptr) *reason = "cusolverDnSgetrf failed";
      return false;
    }
  } else {
    if (ctx.api.dgetrf(
            handle.value,
            n,
            n,
            reinterpret_cast<double*>(d_lu->ptr),
            n,
            reinterpret_cast<double*>(d_work.ptr),
            reinterpret_cast<int*>(d_ipiv->ptr),
            reinterpret_cast<int*>(d_info.ptr)) != CUSOLVER_STATUS_SUCCESS) {
      if (reason != nullptr) *reason = "cusolverDnDgetrf failed";
      return false;
    }
  }
  return read_device_info_(d_info, info, reason);
}

double determinant_from_lu_(
    const Series& a,
    const DeviceBuffer& d_lu,
    const DeviceBuffer& d_ipiv,
    int info,
    std::string* reason) {
  const int n = static_cast<int>(a.sizes()[0]);
  if (info > 0) {
    return 0.0;
  }
  std::vector<int> piv(static_cast<std::size_t>(n), 0);
  if (!copy_d2h_(piv.data(), d_ipiv.ptr, static_cast<std::size_t>(n) * sizeof(int), reason)) {
    return 0.0;
  }
  int parity = 1;
  for (int i = 0; i < n; ++i) {
    if (piv[static_cast<std::size_t>(i)] != (i + 1)) parity = -parity;
  }
  long double det = static_cast<long double>(parity);
  if (a.scalar_type() == ScalarType::Float32) {
    std::vector<float> lu(static_cast<std::size_t>(n) * static_cast<std::size_t>(n), 0.0f);
    if (!copy_d2h_(lu.data(), d_lu.ptr, lu.size() * sizeof(float), reason)) return 0.0;
    for (int i = 0; i < n; ++i) det *= static_cast<long double>(lu[static_cast<std::size_t>(i) * static_cast<std::size_t>(n) + static_cast<std::size_t>(i)]);
  } else {
    std::vector<double> lu(static_cast<std::size_t>(n) * static_cast<std::size_t>(n), 0.0);
    if (!copy_d2h_(lu.data(), d_lu.ptr, lu.size() * sizeof(double), reason)) return 0.0;
    for (int i = 0; i < n; ++i) det *= static_cast<long double>(lu[static_cast<std::size_t>(i) * static_cast<std::size_t>(n) + static_cast<std::size_t>(i)]);
  }
  return static_cast<double>(det);
}

bool solve_lu_inverse_(
    const Series& a,
    DeviceBuffer& d_lu,
    DeviceBuffer& d_ipiv,
    Series* inverse_out,
    std::string* reason) {
  if (inverse_out == nullptr) {
    if (reason != nullptr) *reason = "internal: null inverse output";
    return false;
  }
  const CusolverContext& ctx = cusolver_ctx_();
  if (!ctx.ready) {
    if (reason != nullptr) *reason = ctx.error;
    return false;
  }
  const int n = static_cast<int>(a.sizes()[0]);
  SolverHandleGuard handle{&ctx.api};
  if (ctx.api.create(&handle.value) != CUSOLVER_STATUS_SUCCESS || handle.value == nullptr) {
    if (reason != nullptr) *reason = "cusolverDnCreate failed";
    return false;
  }

  Series inv = Series::empty({a.sizes()[0], a.sizes()[0]}, a.scalar_type(), Platform::CUDA);
  Series identity = Series::eye(a.sizes()[0], a.sizes()[0], a.scalar_type()).to(Platform::CPU).contiguous();
  DeviceBuffer d_rhs{};
  if (!alloc_device_buffer_(&d_rhs, bytes_of_(identity), reason)) return false;
  if (!copy_h2d_(d_rhs.ptr, identity.raw_data(), d_rhs.bytes, reason)) return false;
  DeviceBuffer d_info{};
  if (!alloc_device_buffer_(&d_info, sizeof(int), reason)) return false;

  cusolverStatus_t st = CUSOLVER_STATUS_INTERNAL_ERROR;
  if (a.scalar_type() == ScalarType::Float32) {
    st = ctx.api.sgetrs(
        handle.value,
        CUBLAS_OP_N,
        n,
        n,
        reinterpret_cast<float*>(d_lu.ptr),
        n,
        reinterpret_cast<int*>(d_ipiv.ptr),
        reinterpret_cast<float*>(d_rhs.ptr),
        n,
        reinterpret_cast<int*>(d_info.ptr));
  } else {
    st = ctx.api.dgetrs(
        handle.value,
        CUBLAS_OP_N,
        n,
        n,
        reinterpret_cast<double*>(d_lu.ptr),
        n,
        reinterpret_cast<int*>(d_ipiv.ptr),
        reinterpret_cast<double*>(d_rhs.ptr),
        n,
        reinterpret_cast<int*>(d_info.ptr));
  }
  if (st != CUSOLVER_STATUS_SUCCESS) {
    if (reason != nullptr) *reason = "cusolverDn getrs failed";
    return false;
  }
  int info = 0;
  if (!read_device_info_(d_info, &info, reason)) return false;
  if (info != 0) {
    if (reason != nullptr) *reason = "cusolverDn getrs returned non-zero info";
    return false;
  }
  if (!copy_d2h_(inv.raw_mutable_data(), d_rhs.ptr, d_rhs.bytes, reason)) return false;
  *inverse_out = std::move(inv);
  return true;
}

#endif // PLUSS_CODEM_HAS_CUSOLVER_LINALG && PLUSS_CODEM_HAS_DLOPEN

bool cusolver_linalg_ready() noexcept {
#if PLUSS_CODEM_HAS_CUSOLVER_LINALG && PLUSS_CODEM_HAS_DLOPEN
  return native_linalg_enabled() && cusolver_ctx_().ready;
#else
  return false;
#endif
}

std::optional<Series> try_det(const Series& a, std::string* reason) {
#if PLUSS_CODEM_HAS_CUSOLVER_LINALG && PLUSS_CODEM_HAS_DLOPEN
  std::string local_reason{};
  std::string* why = (reason != nullptr) ? reason : &local_reason;
  if (!validate_lu_input_(a, why)) return std::nullopt;
  const CusolverContext& ctx = cusolver_ctx_();
  if (!ctx.ready) {
    *why = ctx.error;
    return std::nullopt;
  }
  DeviceBuffer d_lu{};
  DeviceBuffer d_ipiv{};
  int info = 0;
  if (!lu_factorize_(a, &d_lu, &d_ipiv, &info, why)) return std::nullopt;
  if (info < 0) {
    *why = "cusolverDn getrf returned invalid argument info";
    return std::nullopt;
  }
  const double det = determinant_from_lu_(a, d_lu, d_ipiv, info, why);
  if (!why->empty() && det == 0.0 && info <= 0) return std::nullopt;
  return Series::full({}, det, a.scalar_type()).to(Platform::CUDA);
#else
  (void)a;
  if (reason != nullptr) {
    *reason = "build/runtime does not expose cuSolver headers+dlopen support";
  }
  return std::nullopt;
#endif
}

std::optional<Series> try_inverse(const Series& a, std::string* reason) {
#if PLUSS_CODEM_HAS_CUSOLVER_LINALG && PLUSS_CODEM_HAS_DLOPEN
  std::string local_reason{};
  std::string* why = (reason != nullptr) ? reason : &local_reason;
  if (!validate_lu_input_(a, why)) return std::nullopt;
  const CusolverContext& ctx = cusolver_ctx_();
  if (!ctx.ready) {
    *why = ctx.error;
    return std::nullopt;
  }
  DeviceBuffer d_lu{};
  DeviceBuffer d_ipiv{};
  int info = 0;
  if (!lu_factorize_(a, &d_lu, &d_ipiv, &info, why)) return std::nullopt;
  if (info < 0) {
    *why = "cusolverDn getrf returned invalid argument info";
    return std::nullopt;
  }
  if (info > 0) {
    *why = "matrix is singular (LU factorization info > 0)";
    return std::nullopt;
  }
  Series inv = pluss::series::undefined();
  if (!solve_lu_inverse_(a, d_lu, d_ipiv, &inv, why)) return std::nullopt;
  return inv;
#else
  (void)a;
  if (reason != nullptr) {
    *reason = "build/runtime does not expose cuSolver headers+dlopen support";
  }
  return std::nullopt;
#endif
}

} // namespace pluss::codem::native::cuda
