#include "NodeK/kernels/cudnn_attention.h"

#include "CodeM/core/SeriesTape.h"

#include <array>
#include <cctype>
#include <cstdlib>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>

#if defined(__unix__) || defined(__APPLE__)
#include <dlfcn.h>
#define PLUSS_AI_HAS_DLOPEN 1
#else
#define PLUSS_AI_HAS_DLOPEN 0
#endif

#if defined(PLUSS_HAS_CUDA) && PLUSS_HAS_CUDA && __has_include(<cuda_runtime_api.h>) && __has_include(<cudnn.h>)
#include <cuda_runtime_api.h>
#include <cudnn.h>
#define PLUSS_AI_HAS_CUDNN_ATTN_KERNELS 1
#else
#define PLUSS_AI_HAS_CUDNN_ATTN_KERNELS 0
#endif

namespace pluss::ai::kernels::cudnn {
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
  if (raw == nullptr || *raw == '\0') return default_value;
  if (ieq_(raw, "1") || ieq_(raw, "true") || ieq_(raw, "yes") || ieq_(raw, "on")) return true;
  if (ieq_(raw, "0") || ieq_(raw, "false") || ieq_(raw, "no") || ieq_(raw, "off")) return false;
  return default_value;
}

bool in_int_range_(std::int64_t v) noexcept {
  return v >= static_cast<std::int64_t>(std::numeric_limits<int>::min()) &&
      v <= static_cast<std::int64_t>(std::numeric_limits<int>::max());
}

#if PLUSS_AI_HAS_CUDNN_ATTN_KERNELS && PLUSS_AI_HAS_DLOPEN

struct CudnnApi {
  decltype(&cudnnCreate) create_handle{nullptr};
  decltype(&cudnnDestroy) destroy_handle{nullptr};
  decltype(&cudnnGetErrorString) error_string{nullptr};
  decltype(&cudnnCreateTensorDescriptor) create_tensor_desc{nullptr};
  decltype(&cudnnSetTensorNdDescriptor) set_tensor_nd_desc{nullptr};
  decltype(&cudnnDestroyTensorDescriptor) destroy_tensor_desc{nullptr};
  decltype(&cudnnSoftmaxForward) softmax_fwd{nullptr};

  bool load(void* handle, std::string* why) {
    auto load_sym = [&](auto& fn, const char* name) -> bool {
      fn = reinterpret_cast<std::remove_reference_t<decltype(fn)>>(dlsym(handle, name));
      if (fn == nullptr) {
        if (why != nullptr) *why = std::string("cuDNN symbol missing: ") + name;
        return false;
      }
      return true;
    };
    return load_sym(create_handle, "cudnnCreate") &&
        load_sym(destroy_handle, "cudnnDestroy") &&
        load_sym(error_string, "cudnnGetErrorString") &&
        load_sym(create_tensor_desc, "cudnnCreateTensorDescriptor") &&
        load_sym(set_tensor_nd_desc, "cudnnSetTensorNdDescriptor") &&
        load_sym(destroy_tensor_desc, "cudnnDestroyTensorDescriptor") &&
        load_sym(softmax_fwd, "cudnnSoftmaxForward");
  }
};

struct CudnnContext {
  bool ready{false};
  std::string error{};
  void* handle{nullptr};
  CudnnApi api{};
};

CudnnContext& cudnn_ctx_() {
  static CudnnContext ctx;
  static std::once_flag once;
  std::call_once(once, [&]() {
    constexpr std::array<const char*, 5> libs{
#if defined(__APPLE__)
        "libcudnn.dylib",
#else
        "libcudnn.so.9",
        "libcudnn.so.8",
        "libcudnn.so",
#endif
        nullptr,
        nullptr,
    };

    if (!pluss::cuda::is_ready()) {
      ctx.error = "CUDA runtime unavailable";
      return;
    }
    if (!pluss::cuda::cudnn_ready()) {
      ctx.error = std::string("cuDNN provider unavailable: ") + pluss::cuda::cudnn_error();
      return;
    }

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
      dlerror(); // clear
      void* handle = dlopen(explicit_lib, RTLD_LAZY | RTLD_LOCAL);
      if (handle != nullptr) {
        std::string why{};
        if (ctx.api.load(handle, &why)) {
          ctx.handle = handle;
          ctx.ready = true;
          ctx.error.clear();
          return;
        }
        ctx.error = why;
        dlclose(handle);
      } else {
        const char* dlerr = dlerror();
        ctx.error = std::string("failed to open explicit cuDNN library '") + explicit_lib + "'" +
            (dlerr != nullptr ? std::string(" (") + dlerr + ")" : std::string{});
      }
    }

    for (const char* lib : libs) {
      if (lib == nullptr || *lib == '\0') continue;
      dlerror(); // clear
      void* handle = dlopen(lib, RTLD_LAZY | RTLD_LOCAL);
      if (handle == nullptr) {
        const char* dlerr = dlerror();
        if (dlerr != nullptr) {
          ctx.error = std::string("failed to open cuDNN library '") + lib + "' (" + dlerr + ")";
        }
        continue;
      }
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
      ctx.error = "cuDNN library not found on loader path";
    }
  });
  return ctx;
}

std::string cudnn_status_string_(const CudnnApi& api, cudnnStatus_t st) {
  if (api.error_string == nullptr) return "unknown cuDNN error";
  const char* msg = api.error_string(st);
  return msg == nullptr ? std::string("unknown cuDNN error") : std::string(msg);
}

bool check_status_(const CudnnApi& api, cudnnStatus_t st, const char* what, std::string* reason) {
  if (st == CUDNN_STATUS_SUCCESS) return true;
  if (reason != nullptr) {
    *reason = std::string(what) + " failed: " + cudnn_status_string_(api, st);
  }
  return false;
}

struct HandleGuard {
  const CudnnApi* api{nullptr};
  cudnnHandle_t value{nullptr};
  ~HandleGuard() {
    if (api != nullptr && value != nullptr && api->destroy_handle != nullptr) {
      (void)api->destroy_handle(value);
    }
  }
};

struct TensorDescGuard {
  const CudnnApi* api{nullptr};
  cudnnTensorDescriptor_t value{nullptr};
  ~TensorDescGuard() {
    if (api != nullptr && value != nullptr && api->destroy_tensor_desc != nullptr) {
      (void)api->destroy_tensor_desc(value);
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

bool set_softmax_desc_(
    const CudnnApi& api,
    cudnnTensorDescriptor_t desc,
    std::int64_t rows,
    std::int64_t cols,
    cudnnDataType_t dtype,
    std::string* reason) {
  if (rows <= 0 || cols <= 0 || !in_int_range_(rows) || !in_int_range_(cols)) {
    if (reason != nullptr) *reason = "invalid attention softmax geometry";
    return false;
  }
  const std::array<int, 4> dims{
      static_cast<int>(rows),
      static_cast<int>(cols),
      1,
      1,
  };
  const std::array<int, 4> strides{
      static_cast<int>(cols),
      1,
      1,
      1,
  };
  return check_status_(
      api,
      api.set_tensor_nd_desc(desc, dtype, 4, dims.data(), strides.data()),
      "cudnnSetTensorNdDescriptor(softmax)",
      reason);
}

bool cudnn_softmax_forward_(
    const Series& logits,
    Series* out,
    std::string* reason) {
  if (out == nullptr) {
    if (reason != nullptr) *reason = "internal: null output";
    return false;
  }

  const CudnnContext& ctx = cudnn_ctx_();
  if (!ctx.ready) {
    if (reason != nullptr) *reason = ctx.error;
    return false;
  }

  const Series in = logits.is_contiguous() ? logits : logits.contiguous();
  const std::int64_t cols = in.sizes().back();
  const std::int64_t rows = in.numel() / cols;
  if (rows <= 0 || cols <= 0) {
    if (reason != nullptr) *reason = "attention softmax requires non-empty trailing dimension";
    return false;
  }

  HandleGuard handle{&ctx.api};
  if (!check_status_(ctx.api, ctx.api.create_handle(&handle.value), "cudnnCreate", reason)) return false;

  TensorDescGuard desc{&ctx.api};
  if (!check_status_(ctx.api, ctx.api.create_tensor_desc(&desc.value), "cudnnCreateTensorDescriptor", reason)) return false;
  if (!set_softmax_desc_(ctx.api, desc.value, rows, cols, CUDNN_DATA_FLOAT, reason)) return false;

  Series out_series = pluss::series::empty(
      in.sizes(),
      Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCUDA));
  DeviceBuffer d_in{};
  DeviceBuffer d_out{};
  if (!alloc_device_buffer_(&d_in, bytes_of_(in), reason)) return false;
  if (!alloc_device_buffer_(&d_out, bytes_of_(out_series), reason)) return false;
  if (!copy_h2d_(d_in.ptr, in.raw_data(), d_in.bytes, reason)) return false;

  const float alpha = 1.0f;
  const float beta = 0.0f;
  if (!check_status_(
          ctx.api,
          ctx.api.softmax_fwd(
              handle.value,
              CUDNN_SOFTMAX_ACCURATE,
              CUDNN_SOFTMAX_MODE_CHANNEL,
              &alpha,
              desc.value,
              d_in.ptr,
              &beta,
              desc.value,
              d_out.ptr),
          "cudnnSoftmaxForward",
          reason)) {
    return false;
  }
  if (!copy_d2h_(out_series.raw_mutable_data(), d_out.ptr, d_out.bytes, reason)) return false;

  *out = std::move(out_series);
  return true;
}

#endif

} // namespace

bool attention_route_enabled() noexcept {
  return parse_bool_env_("PLUSS_CUDNN_ATTENTION", true);
}

bool attention_route_strict() noexcept {
  return parse_bool_env_("PLUSS_CUDNN_ATTENTION_STRICT", false);
}

bool attention_route_ready() noexcept {
#if PLUSS_AI_HAS_CUDNN_ATTN_KERNELS && PLUSS_AI_HAS_DLOPEN
  if (!attention_route_enabled()) return false;
  return cudnn_ctx_().ready;
#else
  return false;
#endif
}

bool supports_scaled_attention_softmax(
    const Series& logits,
    bool require_backward,
    std::string* reason) noexcept {
  if (!attention_route_enabled()) {
    if (reason != nullptr) *reason = "cuDNN attention route disabled by environment";
    return false;
  }
  (void)require_backward;
  if (logits.platform() != pluss::pCUDA) {
    if (reason != nullptr) *reason = "attention logits are not CUDA";
    return false;
  }
  if (logits.dim() < 2) {
    if (reason != nullptr) *reason = "attention logits must be rank >= 2";
    return false;
  }
  if (logits.sizes().back() <= 0) {
    if (reason != nullptr) *reason = "attention logits trailing dim must be > 0";
    return false;
  }
  if (logits.scalar_type() != ScalarType::Float32) {
    if (reason != nullptr) *reason = "cuDNN attention softmax currently supports Float32 only";
    return false;
  }
#if PLUSS_AI_HAS_CUDNN_ATTN_KERNELS && PLUSS_AI_HAS_DLOPEN
  if (!cudnn_ctx_().ready) {
    if (reason != nullptr) *reason = cudnn_ctx_().error;
    return false;
  }
  return true;
#else
  if (reason != nullptr) *reason = "build/runtime does not expose cuDNN attention headers+dlopen support";
  return false;
#endif
}

std::optional<Series> try_scaled_attention_softmax(
    const Series& logits,
    bool require_backward) {
  if (!attention_route_enabled()) return std::nullopt;
  if (logits.platform() != pluss::pCUDA) return std::nullopt;

  std::string reason{};
  if (!supports_scaled_attention_softmax(logits, require_backward, &reason)) {
    if (attention_route_strict()) {
      PLUSS_FAIL(std::string("pluss::ai::scaled_attention: strict cuDNN route unavailable: ") + reason);
    }
    return std::nullopt;
  }

#if PLUSS_AI_HAS_CUDNN_ATTN_KERNELS && PLUSS_AI_HAS_DLOPEN
  Series out;
  if (!cudnn_softmax_forward_(logits, &out, &reason)) {
    if (attention_route_strict()) {
      PLUSS_FAIL(std::string("pluss::ai::scaled_attention: strict cuDNN softmax failed: ") + reason);
    }
    return std::nullopt;
  }

  if (require_backward && pluss::codem::series_tape::recording_enabled() && logits.tracks_slope()) {
    auto link = std::make_shared<pluss::codem::series_tape::SourceLink>();
    link->op_name = "cudnn_attention_softmax";
    link->parents = {logits};
    link->propagate = [logits, out](const Series& output_sens) {
      pluss::codem::series_tape::PauseRecording pause;
      const Series go = (output_sens.platform() == out.platform() && output_sens.scalar_type() == out.scalar_type())
          ? (output_sens.is_contiguous() ? output_sens : output_sens.contiguous())
          : output_sens.to(out.platform(), out.scalar_type()).contiguous();
      const Series dot = go.mul(out).sum(-1, true);
      const Series grad_logits = out.mul(go.sub(dot));
      pluss::codem::series_tape::accumulate_sensitivity(logits, grad_logits);
    };
    pluss::codem::series_tape::bind_source_link(out, std::move(link));
  }

  return out;
#else
  if (attention_route_strict()) {
    PLUSS_FAIL("pluss::ai::scaled_attention: strict cuDNN route requested but cuDNN attention kernels are not compiled in this build");
  }
  return std::nullopt;
#endif
}

} // namespace pluss::ai::kernels::cudnn
