#include "CodeM/native/cuda/MatmulOps.h"

#include "CodeM/native/cuda/BaselineOps.h"
#include "CodeM/native/cuda/CUDARuntime.h"

#include <cctype>
#include <cstdlib>
#include <limits>
#include <mutex>
#include <type_traits>

#if defined(__unix__) || defined(__APPLE__)
#include <dlfcn.h>
#define PLUSS_CODEM_HAS_DLOPEN 1
#else
#define PLUSS_CODEM_HAS_DLOPEN 0
#endif

#if defined(PLUSS_HAS_CUDA) && PLUSS_HAS_CUDA && __has_include(<cublas_v2.h>)
#include <cublas_v2.h>
#if __has_include(<cuda_runtime_api.h>)
#include <cuda_runtime_api.h>
#endif
#define PLUSS_CODEM_HAS_CUBLAS_MATMUL 1
#else
#define PLUSS_CODEM_HAS_CUBLAS_MATMUL 0
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

bool matmul_batch_broadcastable_(const Series& a, const Series& b) noexcept {
  const std::int64_t da = a.dim();
  const std::int64_t db = b.dim();
  const std::int64_t ba = da > 2 ? da - 2 : 0;
  const std::int64_t bb = db > 2 ? db - 2 : 0;
  const std::int64_t rank = ba > bb ? ba : bb;
  for (std::int64_t i = 0; i < rank; ++i) {
    const std::int64_t ai = (da - 3) - i;
    const std::int64_t bi = (db - 3) - i;
    const std::int64_t sa = ai >= 0 ? a.sizes()[static_cast<std::size_t>(ai)] : 1;
    const std::int64_t sb = bi >= 0 ? b.sizes()[static_cast<std::size_t>(bi)] : 1;
    if (sa != sb && sa != 1 && sb != 1) {
      return false;
    }
  }
  return true;
}

bool matmul_core_shapes_compatible_(const Series& a, const Series& b) noexcept {
  if (a.dim() < 1 || b.dim() < 1) {
    return false;
  }
  const std::int64_t k_left = a.sizes().back();
  const std::int64_t k_right = (b.dim() == 1)
      ? b.sizes().front()
      : b.sizes()[static_cast<std::size_t>(b.dim() - 2)];
  if (k_left != k_right) {
    return false;
  }
  return matmul_batch_broadcastable_(a, b);
}

bool map_cublas_dtype_(ScalarType t) noexcept {
  return t == ScalarType::Float32 || t == ScalarType::Float64;
}

#if PLUSS_CODEM_HAS_CUBLAS_MATMUL && PLUSS_CODEM_HAS_DLOPEN

struct CublasApi {
  decltype(&cublasCreate_v2) create{nullptr};
  decltype(&cublasDestroy_v2) destroy{nullptr};
  decltype(&cublasSgemm_v2) sgemm{nullptr};
  decltype(&cublasDgemm_v2) dgemm{nullptr};

  bool load(void* handle, std::string* why) {
    auto load_sym = [&](auto& fn, const char* name) -> bool {
      fn = reinterpret_cast<std::remove_reference_t<decltype(fn)>>(dlsym(handle, name));
      if (fn == nullptr) {
        if (why != nullptr) {
          *why = std::string("cuBLAS symbol missing: ") + name;
        }
        return false;
      }
      return true;
    };

    return load_sym(create, "cublasCreate_v2") &&
        load_sym(destroy, "cublasDestroy_v2") &&
        load_sym(sgemm, "cublasSgemm_v2") &&
        load_sym(dgemm, "cublasDgemm_v2");
  }
};

struct CublasContext {
  bool ready{false};
  std::string error{};
  void* handle{nullptr};
  CublasApi api{};
};

CublasContext& cublas_ctx_() {
  static CublasContext ctx;
  static std::once_flag once;
  std::call_once(once, [&]() {
    constexpr const char* libs[] = {
#if defined(__APPLE__)
        "libcublas.dylib",
#else
        "libcublas.so.12",
        "libcublas.so.11",
        "libcublas.so",
#endif
        nullptr,
    };

    if (!runtime_available()) {
      ctx.error = "CUDA runtime unavailable";
      return;
    }
    if (!cublas_provider_available()) {
      ctx.error = std::string("cuBLAS provider unavailable: ") + cublas_provider_error_cstr();
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
      ctx.error = "cuBLAS library not found on loader path";
    }
  });
  return ctx;
}

bool in_int_range_(std::int64_t v) noexcept {
  return v >= static_cast<std::int64_t>(std::numeric_limits<int>::min()) &&
      v <= static_cast<std::int64_t>(std::numeric_limits<int>::max());
}

std::string cublas_status_string_(const CublasApi& api, cublasStatus_t st) {
  (void)api;
  return std::string("status=") + std::to_string(static_cast<int>(st));
}

std::size_t bytes_of_(const Series& s) noexcept {
  return static_cast<std::size_t>(s.numel()) * static_cast<std::size_t>(s.itemsize());
}

struct DeviceBuffer {
  void* ptr{nullptr};
  std::size_t bytes{0};

  ~DeviceBuffer() {
    if (ptr != nullptr) {
      (void)cudaFree(ptr);
    }
  }
};

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

bool run_cublas_2d_gemm_(
    const Series& a,
    const Series& b,
    Series* out,
    std::string* reason) {
  if (out == nullptr) {
    if (reason != nullptr) *reason = "internal: null output";
    return false;
  }
  const CublasContext& ctx = cublas_ctx_();
  if (!ctx.ready) {
    if (reason != nullptr) *reason = ctx.error;
    return false;
  }

  const std::int64_t m64 = a.sizes()[0];
  const std::int64_t k64 = a.sizes()[1];
  const std::int64_t n64 = b.sizes()[1];
  if (m64 <= 0 || n64 <= 0 || k64 <= 0 || !in_int_range_(m64) || !in_int_range_(n64) || !in_int_range_(k64)) {
    if (reason != nullptr) *reason = "cuBLAS matmul: invalid matrix geometry";
    return false;
  }

  const int m = static_cast<int>(m64);
  const int n = static_cast<int>(n64);
  const int k = static_cast<int>(k64);
  Series y = Series::empty({m64, n64}, a.scalar_type(), Platform::CUDA);

  DeviceBuffer d_a{};
  DeviceBuffer d_b{};
  DeviceBuffer d_c{};
  if (!alloc_device_buffer_(&d_a, bytes_of_(a), reason)) return false;
  if (!alloc_device_buffer_(&d_b, bytes_of_(b), reason)) return false;
  if (!alloc_device_buffer_(&d_c, bytes_of_(y), reason)) return false;
  if (!copy_h2d_(d_a.ptr, a.raw_data(), d_a.bytes, reason)) return false;
  if (!copy_h2d_(d_b.ptr, b.raw_data(), d_b.bytes, reason)) return false;

  cublasHandle_t handle = nullptr;
  cublasStatus_t st = ctx.api.create(&handle);
  if (st != CUBLAS_STATUS_SUCCESS || handle == nullptr) {
    if (reason != nullptr) *reason = std::string("cublasCreate_v2 failed: ") + cublas_status_string_(ctx.api, st);
    return false;
  }

  auto destroy_handle = [&]() {
    if (handle != nullptr) {
      (void)ctx.api.destroy(handle);
      handle = nullptr;
    }
  };

  // Our Series storage is row-major. cuBLAS assumes column-major.
  // Compute C^T = B^T * A^T with opN/opN in cuBLAS:
  // C_col(N,M) = B_col(N,K) * A_col(K,M), then interpret C_col as row-major C(M,N).
  if (a.scalar_type() == ScalarType::Float32) {
    const float alpha = 1.0f;
    const float beta = 0.0f;
    st = ctx.api.sgemm(
        handle,
        CUBLAS_OP_N,
        CUBLAS_OP_N,
        n,
        m,
        k,
        &alpha,
        reinterpret_cast<const float*>(d_b.ptr),
        n,
        reinterpret_cast<const float*>(d_a.ptr),
        k,
        &beta,
        reinterpret_cast<float*>(d_c.ptr),
        n);
  } else {
    const double alpha = 1.0;
    const double beta = 0.0;
    st = ctx.api.dgemm(
        handle,
        CUBLAS_OP_N,
        CUBLAS_OP_N,
        n,
        m,
        k,
        &alpha,
        reinterpret_cast<const double*>(d_b.ptr),
        n,
        reinterpret_cast<const double*>(d_a.ptr),
        k,
        &beta,
        reinterpret_cast<double*>(d_c.ptr),
        n);
  }

  destroy_handle();
  if (st != CUBLAS_STATUS_SUCCESS) {
    if (reason != nullptr) *reason = std::string("cublasGemm failed: ") + cublas_status_string_(ctx.api, st);
    return false;
  }
  if (!copy_d2h_(y.raw_mutable_data(), d_c.ptr, d_c.bytes, reason)) return false;

  *out = std::move(y);
  return true;
}

#endif // PLUSS_CODEM_HAS_CUBLAS_MATMUL && PLUSS_CODEM_HAS_DLOPEN

} // namespace

bool native_matmul_enabled() noexcept {
  return parse_bool_env_("PLUSS_CUDA_NATIVE_MATMUL", false);
}

bool native_matmul_strict() noexcept {
  return parse_bool_env_("PLUSS_CUDA_NATIVE_MATMUL_STRICT", false);
}

bool supports_matmul_baseline(ScalarType lhs_dtype, ScalarType rhs_dtype) noexcept {
  if (!runtime_available() || !native_baseline_enabled() || !native_matmul_enabled()) {
    return false;
  }
  return supports_matmul_lane(lhs_dtype, rhs_dtype);
}

bool supports_matmul(const Series& a, const Series& b) noexcept {
  if (!supports_matmul_baseline(a.scalar_type(), b.scalar_type())) {
    return false;
  }
  if (a.platform() != Platform::CUDA || b.platform() != Platform::CUDA) {
    return false;
  }
  return matmul_core_shapes_compatible_(a, b);
}

bool cublas_matmul_enabled() noexcept {
  return parse_bool_env_("PLUSS_CUBLAS_MATMUL", true);
}

bool cublas_matmul_strict() noexcept {
  return parse_bool_env_("PLUSS_CUBLAS_MATMUL_STRICT", false);
}

bool cublas_matmul_ready() noexcept {
#if PLUSS_CODEM_HAS_CUBLAS_MATMUL && PLUSS_CODEM_HAS_DLOPEN
  if (!cublas_matmul_enabled()) return false;
  return cublas_ctx_().ready;
#else
  return false;
#endif
}

bool supports_cublas_matmul(
    const Series& a,
    const Series& b,
    std::string* reason) noexcept {
  if (!cublas_matmul_enabled()) {
    if (reason != nullptr) *reason = "cuBLAS matmul route disabled by environment";
    return false;
  }
  if (a.platform() != Platform::CUDA || b.platform() != Platform::CUDA) {
    if (reason != nullptr) *reason = "input is not CUDA";
    return false;
  }
  if (a.dim() != 2 || b.dim() != 2) {
    if (reason != nullptr) *reason = "only rank-2 matmul is supported by cuBLAS route";
    return false;
  }
  if (a.sizes()[1] != b.sizes()[0]) {
    if (reason != nullptr) *reason = "matmul inner dimensions must match";
    return false;
  }
  if (a.scalar_type() != b.scalar_type()) {
    if (reason != nullptr) *reason = "lhs/rhs dtypes must match";
    return false;
  }
  if (!map_cublas_dtype_(a.scalar_type())) {
    if (reason != nullptr) *reason = "cuBLAS matmul currently supports Float32/Float64 only";
    return false;
  }
  if (!a.is_contiguous() || !b.is_contiguous()) {
    if (reason != nullptr) *reason = "inputs must be contiguous";
    return false;
  }
#if PLUSS_CODEM_HAS_CUBLAS_MATMUL && PLUSS_CODEM_HAS_DLOPEN
  if (!cublas_ctx_().ready) {
    if (reason != nullptr) *reason = cublas_ctx_().error;
    return false;
  }
  return true;
#else
  if (reason != nullptr) *reason = "build/runtime does not expose cuBLAS headers+dlopen support";
  return false;
#endif
}

std::optional<Series> try_cublas_matmul(const Series& a, const Series& b) {
  if (a.platform() != Platform::CUDA || b.platform() != Platform::CUDA) {
    return std::nullopt;
  }
  const Series a_in = a.is_contiguous() ? a : a.contiguous();
  const Series b_in = b.is_contiguous() ? b : b.contiguous();

  std::string reason{};
  if (!supports_cublas_matmul(a_in, b_in, &reason)) {
    if (cublas_matmul_strict()) {
      PLUSS_FAIL(std::string("matmul: strict cuBLAS route unavailable: ") + reason);
    }
    return std::nullopt;
  }

#if PLUSS_CODEM_HAS_CUBLAS_MATMUL && PLUSS_CODEM_HAS_DLOPEN
  Series out;
  if (!run_cublas_2d_gemm_(a_in, b_in, &out, &reason)) {
    if (cublas_matmul_strict()) {
      PLUSS_FAIL(std::string("matmul: strict cuBLAS forward failed: ") + reason);
    }
    return std::nullopt;
  }
  return out;
#else
  if (cublas_matmul_strict()) {
    PLUSS_FAIL("matmul: strict cuBLAS route requested but cuBLAS kernels are not compiled in this build");
  }
  return std::nullopt;
#endif
}

} // namespace pluss::codem::native::cuda
