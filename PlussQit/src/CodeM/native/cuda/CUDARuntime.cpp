#include "CodeM/native/cuda/CUDARuntime.h"

#include <array>
#include <cctype>
#include <cstdlib>
#include <mutex>
#include <string>

#if defined(__unix__) || defined(__APPLE__)
#include <dlfcn.h>
#define PLUSSQIT_HAS_DLOPEN 1
#else
#define PLUSSQIT_HAS_DLOPEN 0
#endif

#if defined(PLUSS_HAS_CUDA) && PLUSS_HAS_CUDA
#if __has_include(<cuda_runtime_api.h>)
#include <cuda_runtime_api.h>
#define PLUSSQIT_HAS_CUDA_RUNTIME_API 1
#else
#define PLUSSQIT_HAS_CUDA_RUNTIME_API 0
#endif
#endif

namespace pluss::codem::native::cuda {
namespace {

struct RuntimeContext {
  bool ready{false};
  std::int32_t device_count{0};
  std::int32_t runtime_version{0};
  std::int32_t driver_version{0};
  std::string primary_device_name{};
  std::int32_t primary_cc_major{0};
  std::int32_t primary_cc_minor{0};
  std::size_t primary_global_memory_bytes{0};
  std::string error{};
};

struct ProviderContext {
  bool available{false};
  std::string error{};
};

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

RuntimeContext& runtime_ctx_() {
  static RuntimeContext ctx;
  static std::once_flag once;
  std::call_once(once, [&]() {
#if defined(PLUSS_HAS_CUDA) && PLUSS_HAS_CUDA
#if defined(PLUSSQIT_HAS_CUDA_RUNTIME_API) && PLUSSQIT_HAS_CUDA_RUNTIME_API
    int count = 0;
    const cudaError_t err = cudaGetDeviceCount(&count);
    if (err != cudaSuccess) {
      const char* err_msg = cudaGetErrorString(err);
      if (err_msg != nullptr) {
        ctx.error = std::string("CUDA runtime unavailable: ") + err_msg;
      } else {
        ctx.error = "CUDA runtime unavailable: cudaGetDeviceCount failed";
      }
      return;
    }
    ctx.device_count = static_cast<std::int32_t>(count);
    if (count <= 0) {
      ctx.error = "CUDA runtime unavailable: no CUDA devices";
      return;
    }
    int runtime_version = 0;
    if (cudaRuntimeGetVersion(&runtime_version) == cudaSuccess) {
      ctx.runtime_version = static_cast<std::int32_t>(runtime_version);
    }
    int driver_version = 0;
    if (cudaDriverGetVersion(&driver_version) == cudaSuccess) {
      ctx.driver_version = static_cast<std::int32_t>(driver_version);
    }
    cudaDeviceProp props{};
    if (cudaGetDeviceProperties(&props, 0) == cudaSuccess) {
      ctx.primary_device_name = props.name;
      ctx.primary_cc_major = static_cast<std::int32_t>(props.major);
      ctx.primary_cc_minor = static_cast<std::int32_t>(props.minor);
      ctx.primary_global_memory_bytes = static_cast<std::size_t>(props.totalGlobalMem);
    }
    ctx.ready = true;
#else
    ctx.error = "CUDA runtime probe enabled but cuda_runtime_api.h is unavailable";
#endif
#else
    ctx.error = "CUDA runtime probe disabled at build (PLUSSQIT_ENABLE_CUDA=OFF)";
#endif
  });
  return ctx;
}

ProviderContext probe_provider_(
    const std::array<const char*, 4>& libs,
    const char* create_sym,
    const char* destroy_sym,
    const char* provider_name) {
  ProviderContext out{};
  if (!runtime_available()) {
    out.error = std::string(provider_name) + " unavailable: " + runtime_error_cstr();
    return out;
  }

#if PLUSSQIT_HAS_DLOPEN
  auto try_candidate = [&](const char* candidate, std::string* why) -> bool {
    if (candidate == nullptr || *candidate == '\0') return false;
    dlerror(); // clear
    void* handle = dlopen(candidate, RTLD_LAZY | RTLD_LOCAL);
    if (handle == nullptr) {
      if (why != nullptr) {
        const char* dlerr = dlerror();
        *why = std::string(provider_name) + " unavailable: failed to open " + candidate +
            (dlerr != nullptr ? std::string(" (") + dlerr + ")" : std::string{});
      }
      return false;
    }
    void* create_fn = dlsym(handle, create_sym);
    void* destroy_fn = dlsym(handle, destroy_sym);
    if (create_fn != nullptr && destroy_fn != nullptr) {
      out.available = true;
      dlclose(handle);
      return true;
    }
    if (why != nullptr) {
      *why = std::string(provider_name) + " unavailable: missing required symbols in " + candidate;
    }
    dlclose(handle);
    return false;
  };

  if (std::string(provider_name) == "cuDNN") {
    const char* explicit_lib = std::getenv("PLUSS_CUDNN_LIBRARY");
    if (explicit_lib == nullptr || *explicit_lib == '\0') {
      explicit_lib = std::getenv("CUDNN_LIBRARY");
    }
#if defined(PLUSS_CUDNN_LIBRARY_DEFAULT)
    if (explicit_lib == nullptr || *explicit_lib == '\0') {
      explicit_lib = PLUSS_CUDNN_LIBRARY_DEFAULT;
    }
#endif
    if (explicit_lib != nullptr && *explicit_lib != '\0') {
      std::string why{};
      if (try_candidate(explicit_lib, &why)) {
        return out;
      }
      out.error = std::move(why);
    }
  }

  for (const char* lib : libs) {
    if (lib == nullptr || *lib == '\0') {
      continue;
    }
    std::string why{};
    if (try_candidate(lib, &why)) {
      return out;
    }
    out.error = std::move(why);
  }
  if (out.error.empty()) {
    out.error = std::string(provider_name) + " unavailable: provider library not found on loader path";
  }
#else
  (void)libs;
  (void)create_sym;
  (void)destroy_sym;
  out.error = std::string(provider_name) + " probe unavailable on this platform";
#endif
  return out;
}

ProviderContext& cublas_ctx_() {
  static ProviderContext ctx;
  static std::once_flag once;
  std::call_once(once, [&]() {
    ctx = probe_provider_(
        std::array<const char*, 4>{
            "libcublas.so.12",
            "libcublas.so.11",
            "libcublas.so",
            nullptr},
        "cublasCreate_v2",
        "cublasDestroy_v2",
        "cuBLAS");
  });
  return ctx;
}

ProviderContext& cusolver_ctx_() {
  static ProviderContext ctx;
  static std::once_flag once;
  std::call_once(once, [&]() {
    ctx = probe_provider_(
        std::array<const char*, 4>{
            "libcusolver.so.11",
            "libcusolver.so.10",
            "libcusolver.so",
            nullptr},
        "cusolverDnCreate",
        "cusolverDnDestroy",
        "cuSOLVER");
  });
  return ctx;
}

ProviderContext& cudnn_ctx_() {
  static ProviderContext ctx;
  static std::once_flag once;
  std::call_once(once, [&]() {
    ctx = probe_provider_(
        std::array<const char*, 4>{
            "libcudnn.so.9",
            "libcudnn.so.8",
            "libcudnn.so",
            nullptr},
        "cudnnCreate",
        "cudnnDestroy",
        "cuDNN");
  });
  return ctx;
}

} // namespace

bool runtime_available() noexcept {
  return runtime_ctx_().ready;
}

const char* runtime_error_cstr() noexcept {
  return runtime_ctx_().error.c_str();
}

std::int32_t device_count() noexcept {
  return runtime_ctx_().device_count;
}

std::int32_t runtime_version() noexcept {
  return runtime_ctx_().runtime_version;
}

std::int32_t driver_version() noexcept {
  return runtime_ctx_().driver_version;
}

const char* primary_device_name() noexcept {
  return runtime_ctx_().primary_device_name.c_str();
}

std::int32_t primary_device_cc_major() noexcept {
  return runtime_ctx_().primary_cc_major;
}

std::int32_t primary_device_cc_minor() noexcept {
  return runtime_ctx_().primary_cc_minor;
}

std::size_t primary_device_global_memory_bytes() noexcept {
  return runtime_ctx_().primary_global_memory_bytes;
}

bool cublas_provider_available() noexcept {
  return cublas_ctx_().available;
}

const char* cublas_provider_error_cstr() noexcept {
  return cublas_ctx_().error.c_str();
}

bool cusolver_provider_available() noexcept {
  return cusolver_ctx_().available;
}

const char* cusolver_provider_error_cstr() noexcept {
  return cusolver_ctx_().error.c_str();
}

bool cudnn_provider_available() noexcept {
  return cudnn_ctx_().available;
}

const char* cudnn_provider_error_cstr() noexcept {
  return cudnn_ctx_().error.c_str();
}

TransferPolicy transfer_policy() noexcept {
  return parse_bool_env_("PLUSS_CUDA_ALLOW_CPU_BRIDGE", false)
      ? TransferPolicy::AllowCpuBridge
      : TransferPolicy::Strict;
}

bool cpu_bridge_enabled() noexcept {
  return transfer_policy() == TransferPolicy::AllowCpuBridge;
}

const char* transfer_policy_name(TransferPolicy policy) noexcept {
  switch (policy) {
    case TransferPolicy::AllowCpuBridge:
      return "AllowCpuBridge";
    case TransferPolicy::Strict:
    default:
      return "Strict";
  }
}

bool copy_host_to_device(void* dst, const void* src, std::size_t nbytes) noexcept {
  if (nbytes == 0) {
    return true;
  }
  if (dst == nullptr || src == nullptr) {
    return false;
  }
  if (!runtime_available()) {
    return false;
  }
#if defined(PLUSS_HAS_CUDA) && PLUSS_HAS_CUDA && defined(PLUSSQIT_HAS_CUDA_RUNTIME_API) && PLUSSQIT_HAS_CUDA_RUNTIME_API
  return cudaMemcpy(dst, src, nbytes, cudaMemcpyHostToDevice) == cudaSuccess;
#else
  return false;
#endif
}

bool copy_device_to_host(void* dst, const void* src, std::size_t nbytes) noexcept {
  if (nbytes == 0) {
    return true;
  }
  if (dst == nullptr || src == nullptr) {
    return false;
  }
  if (!runtime_available()) {
    return false;
  }
#if defined(PLUSS_HAS_CUDA) && PLUSS_HAS_CUDA && defined(PLUSSQIT_HAS_CUDA_RUNTIME_API) && PLUSSQIT_HAS_CUDA_RUNTIME_API
  return cudaMemcpy(dst, src, nbytes, cudaMemcpyDeviceToHost) == cudaSuccess;
#else
  return false;
#endif
}

} // namespace pluss::codem::native::cuda
