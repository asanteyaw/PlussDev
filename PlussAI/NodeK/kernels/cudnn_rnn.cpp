#include "NodeK/kernels/cudnn_rnn.h"

#include <pluss/nn_detail.h>

#include <array>
#include <cctype>
#include <cstdlib>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <vector>

#if defined(__unix__) || defined(__APPLE__)
#include <dlfcn.h>
#define PLUSS_AI_HAS_DLOPEN 1
#else
#define PLUSS_AI_HAS_DLOPEN 0
#endif

#if defined(PLUSS_HAS_CUDA) && PLUSS_HAS_CUDA && __has_include(<cuda_runtime_api.h>) && __has_include(<cudnn.h>)
#include <cuda_runtime_api.h>
#include <cudnn.h>
#define PLUSS_AI_HAS_CUDNN_RNN_KERNELS 1
#else
#define PLUSS_AI_HAS_CUDNN_RNN_KERNELS 0
#endif

namespace pluss::ai::kernels::cudnn {
namespace {

enum class RnnKind : std::uint8_t {
  Vanilla,
  Gru,
  Lstm,
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
  if (raw == nullptr || *raw == '\0') return default_value;
  if (ieq_(raw, "1") || ieq_(raw, "true") || ieq_(raw, "yes") || ieq_(raw, "on")) return true;
  if (ieq_(raw, "0") || ieq_(raw, "false") || ieq_(raw, "no") || ieq_(raw, "off")) return false;
  return default_value;
}

bool in_int_range_(std::int64_t v) noexcept {
  return v >= static_cast<std::int64_t>(std::numeric_limits<int>::min()) &&
      v <= static_cast<std::int64_t>(std::numeric_limits<int>::max());
}

int gate_count_(RnnKind kind) noexcept {
  switch (kind) {
    case RnnKind::Vanilla:
      return 1;
    case RnnKind::Gru:
      return 3;
    case RnnKind::Lstm:
      return 4;
    default:
      return 0;
  }
}

#if PLUSS_AI_HAS_CUDNN_RNN_KERNELS && PLUSS_AI_HAS_DLOPEN

using cudnn_set_rnn_desc_v6_fn = cudnnStatus_t (*)(...);
using cudnn_set_rnn_projection_layers_fn = cudnnStatus_t (*)(...);
using cudnn_get_rnn_workspace_size_fn = cudnnStatus_t (*)(...);
using cudnn_get_rnn_training_reserve_size_fn = cudnnStatus_t (*)(...);
using cudnn_get_rnn_params_size_fn = cudnnStatus_t (*)(...);
using cudnn_get_rnn_lin_layer_matrix_fn = cudnnStatus_t (*)(...);
using cudnn_get_rnn_lin_layer_bias_fn = cudnnStatus_t (*)(...);
using cudnn_rnn_forward_inference_fn = cudnnStatus_t (*)(...);
using cudnn_rnn_forward_training_fn = cudnnStatus_t (*)(...);

struct CudnnApi {
  decltype(&cudnnCreate) create_handle{nullptr};
  decltype(&cudnnDestroy) destroy_handle{nullptr};
  decltype(&cudnnGetErrorString) error_string{nullptr};

  decltype(&cudnnCreateTensorDescriptor) create_tensor_desc{nullptr};
  decltype(&cudnnSetTensorNdDescriptor) set_tensor_nd_desc{nullptr};
  decltype(&cudnnDestroyTensorDescriptor) destroy_tensor_desc{nullptr};

  decltype(&cudnnCreateFilterDescriptor) create_filter_desc{nullptr};
  decltype(&cudnnSetFilterNdDescriptor) set_filter_nd_desc{nullptr};
  decltype(&cudnnDestroyFilterDescriptor) destroy_filter_desc{nullptr};

  decltype(&cudnnCreateDropoutDescriptor) create_dropout_desc{nullptr};
  decltype(&cudnnDropoutGetStatesSize) get_dropout_states_size{nullptr};
  decltype(&cudnnSetDropoutDescriptor) set_dropout_desc{nullptr};
  decltype(&cudnnDestroyDropoutDescriptor) destroy_dropout_desc{nullptr};

  decltype(&cudnnCreateRNNDescriptor) create_rnn_desc{nullptr};
  cudnn_set_rnn_desc_v6_fn set_rnn_desc_v6{nullptr};
  cudnn_set_rnn_projection_layers_fn set_rnn_projection_layers{nullptr};
  decltype(&cudnnDestroyRNNDescriptor) destroy_rnn_desc{nullptr};

  cudnn_get_rnn_workspace_size_fn get_rnn_workspace_size{nullptr};
  cudnn_get_rnn_training_reserve_size_fn get_rnn_training_reserve_size{nullptr};
  cudnn_get_rnn_params_size_fn get_rnn_params_size{nullptr};
  cudnn_get_rnn_lin_layer_matrix_fn get_rnn_lin_layer_matrix{nullptr};
  cudnn_get_rnn_lin_layer_bias_fn get_rnn_lin_layer_bias{nullptr};
  cudnn_rnn_forward_inference_fn rnn_forward_inference{nullptr};
  cudnn_rnn_forward_training_fn rnn_forward_training{nullptr};

  bool load(void* handle, std::string* why) {
    auto resolve_sym = [&](const char* name) -> void* {
      void* sym = nullptr;
      if (handle != nullptr) sym = dlsym(handle, name);
      if (sym == nullptr) sym = dlsym(RTLD_DEFAULT, name);
      return sym;
    };
    auto load_sym = [&](auto& fn, const char* name) -> bool {
      fn = reinterpret_cast<std::remove_reference_t<decltype(fn)>>(resolve_sym(name));
      if (fn == nullptr) {
        if (why != nullptr) *why = std::string("cuDNN symbol missing: ") + name;
        return false;
      }
      return true;
    };
    auto load_sym_opt = [&](auto& fn, const char* name) {
      fn = reinterpret_cast<std::remove_reference_t<decltype(fn)>>(resolve_sym(name));
      (void)name;
    };

    const bool ok = load_sym(create_handle, "cudnnCreate") &&
        load_sym(destroy_handle, "cudnnDestroy") &&
        load_sym(error_string, "cudnnGetErrorString") &&
        load_sym(create_tensor_desc, "cudnnCreateTensorDescriptor") &&
        load_sym(set_tensor_nd_desc, "cudnnSetTensorNdDescriptor") &&
        load_sym(destroy_tensor_desc, "cudnnDestroyTensorDescriptor") &&
        load_sym(create_filter_desc, "cudnnCreateFilterDescriptor") &&
        load_sym(set_filter_nd_desc, "cudnnSetFilterNdDescriptor") &&
        load_sym(destroy_filter_desc, "cudnnDestroyFilterDescriptor") &&
        load_sym(create_dropout_desc, "cudnnCreateDropoutDescriptor") &&
        load_sym(get_dropout_states_size, "cudnnDropoutGetStatesSize") &&
        load_sym(set_dropout_desc, "cudnnSetDropoutDescriptor") &&
        load_sym(destroy_dropout_desc, "cudnnDestroyDropoutDescriptor") &&
        load_sym(create_rnn_desc, "cudnnCreateRNNDescriptor") &&
        load_sym(set_rnn_desc_v6, "cudnnSetRNNDescriptor_v6") &&
        load_sym(destroy_rnn_desc, "cudnnDestroyRNNDescriptor") &&
        load_sym(get_rnn_workspace_size, "cudnnGetRNNWorkspaceSize") &&
        load_sym(get_rnn_params_size, "cudnnGetRNNParamsSize") &&
        load_sym(get_rnn_lin_layer_matrix, "cudnnGetRNNLinLayerMatrixParams") &&
        load_sym(get_rnn_lin_layer_bias, "cudnnGetRNNLinLayerBiasParams") &&
        load_sym(rnn_forward_inference, "cudnnRNNForwardInference");
    if (!ok) return false;
    load_sym_opt(set_rnn_projection_layers, "cudnnSetRNNProjectionLayers");
    load_sym_opt(get_rnn_training_reserve_size, "cudnnGetRNNTrainingReserveSize");
    load_sym_opt(rnn_forward_training, "cudnnRNNForwardTraining");
    return true;
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
    constexpr auto libs = std::to_array<const char*>({
#if defined(__APPLE__)
        "libcudnn.dylib",
#else
        "libcudnn.so.9",
        "libcudnn_ops.so.9",
        "libcudnn_cnn.so.9",
        "libcudnn_adv.so.9",
        "libcudnn_graph.so.9",
        "libcudnn.so.8",
        "libcudnn_ops.so.8",
        "libcudnn_cnn.so.8",
        "libcudnn_adv.so.8",
        "libcudnn_graph.so.8",
        "libcudnn.so",
#endif
    });

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
      void* handle = dlopen(explicit_lib, RTLD_NOW | RTLD_GLOBAL);
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

    std::vector<void*> opened_handles{};
    opened_handles.reserve(libs.size());
    for (const char* lib : libs) {
      if (lib == nullptr || *lib == '\0') continue;
      dlerror(); // clear
      void* handle = dlopen(lib, RTLD_NOW | RTLD_GLOBAL);
      if (handle == nullptr) {
        const char* dlerr = dlerror();
        if (dlerr != nullptr) {
          ctx.error = std::string("failed to open cuDNN library '") + lib + "' (" + dlerr + ")";
        }
        continue;
      }
      opened_handles.push_back(handle);
      std::string why{};
      if (!ctx.api.load(handle, &why)) {
        ctx.error = why;
        continue;
      }
      ctx.handle = handle;
      ctx.ready = true;
      ctx.error.clear();
      return;
    }
    for (void* h : opened_handles) {
      if (h != nullptr) dlclose(h);
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

struct FilterDescGuard {
  const CudnnApi* api{nullptr};
  cudnnFilterDescriptor_t value{nullptr};
  ~FilterDescGuard() {
    if (api != nullptr && value != nullptr && api->destroy_filter_desc != nullptr) {
      (void)api->destroy_filter_desc(value);
    }
  }
};

struct DropoutDescGuard {
  const CudnnApi* api{nullptr};
  cudnnDropoutDescriptor_t value{nullptr};
  ~DropoutDescGuard() {
    if (api != nullptr && value != nullptr && api->destroy_dropout_desc != nullptr) {
      (void)api->destroy_dropout_desc(value);
    }
  }
};

struct RNNDescGuard {
  const CudnnApi* api{nullptr};
  cudnnRNNDescriptor_t value{nullptr};
  ~RNNDescGuard() {
    if (api != nullptr && value != nullptr && api->destroy_rnn_desc != nullptr) {
      (void)api->destroy_rnn_desc(value);
    }
  }
};

struct WorkspaceGuard {
  void* ptr{nullptr};
  ~WorkspaceGuard() {
    if (ptr != nullptr) {
      (void)cudaFree(ptr);
    }
  }
};

bool map_dtype_(ScalarType st, cudnnDataType_t* io_dtype, std::string* reason) {
  if (io_dtype == nullptr) {
    if (reason != nullptr) *reason = "internal: null dtype output";
    return false;
  }
  switch (st) {
    case ScalarType::Float32:
      *io_dtype = CUDNN_DATA_FLOAT;
      return true;
    case ScalarType::Float16:
      *io_dtype = CUDNN_DATA_HALF;
      return true;
    default:
      break;
  }
  if (reason != nullptr) *reason = "cuDNN RNN route currently supports Float32/Float16 only";
  return false;
}

bool set_3d_tensor_desc_(
    const CudnnApi& api,
    cudnnTensorDescriptor_t desc,
    std::int64_t d0,
    std::int64_t d1,
    std::int64_t d2,
    cudnnDataType_t dtype,
    const char* what,
    std::string* reason) {
  if (d0 <= 0 || d1 <= 0 || d2 <= 0 || !in_int_range_(d0) || !in_int_range_(d1) || !in_int_range_(d2)) {
    if (reason != nullptr) *reason = std::string(what) + ": invalid tensor geometry";
    return false;
  }
  const std::array<int, 3> dims{static_cast<int>(d0), static_cast<int>(d1), static_cast<int>(d2)};
  const std::array<int, 3> strides{
      static_cast<int>(d1 * d2),
      static_cast<int>(d2),
      1,
  };
  return check_status_(
      api,
      api.set_tensor_nd_desc(desc, dtype, 3, dims.data(), strides.data()),
      what,
      reason);
}

Series to_cuda_dtype_contiguous_(const Series& x, ScalarType dtype) {
  Series out = x;
  if (out.platform() != pluss::pCUDA || out.scalar_type() != dtype) {
    out = out.to(pluss::pCUDA, dtype);
  }
  if (!out.is_contiguous()) out = out.contiguous();
  return out;
}

Series zeros_cuda_(std::initializer_list<std::int64_t> sizes, ScalarType dtype) {
  const Config cfg = Config{}.with_dtype(dtype).with_platform(pluss::pCUDA);
  return pluss::zeros(sizes, cfg);
}

bool cuda_copy_(void* dst, const Series& src_raw, std::int64_t expected_numel, ScalarType dtype, const char* what, std::string* reason) {
  if (dst == nullptr) {
    if (reason != nullptr) *reason = std::string(what) + ": null destination pointer";
    return false;
  }
  const Series src = to_cuda_dtype_contiguous_(src_raw, dtype);
  if (src.numel() != expected_numel) {
    if (reason != nullptr) {
      *reason = std::string(what) + ": numel mismatch (expected " + std::to_string(expected_numel) +
          ", got " + std::to_string(src.numel()) + ")";
    }
    return false;
  }
  const auto bytes = static_cast<std::size_t>(src.numel()) * static_cast<std::size_t>(src.itemsize());
  if (bytes == 0) return true;
  if (cudaMemcpy(dst, src.raw_data(), bytes, cudaMemcpyDeviceToDevice) != cudaSuccess) {
    if (reason != nullptr) {
      *reason = std::string(what) + ": cudaMemcpy failed";
    }
    return false;
  }
  return true;
}

cudnnRNNMode_t mode_for_kind_(RnnKind kind, const ::pluss::ai::rnn::NetConfig& cfg, std::string* reason) {
  switch (kind) {
    case RnnKind::Vanilla:
      if (cfg.nonlinearity == "relu") return CUDNN_RNN_RELU;
      if (cfg.nonlinearity == "tanh") return CUDNN_RNN_TANH;
      if (reason != nullptr) *reason = "RNN nonlinearity must be 'relu' or 'tanh' for cuDNN route";
      return CUDNN_RNN_TANH;
    case RnnKind::Gru:
      return CUDNN_GRU;
    case RnnKind::Lstm:
      return CUDNN_LSTM;
    default:
      break;
  }
  if (reason != nullptr) *reason = "unsupported RNN kind";
  return CUDNN_RNN_TANH;
}

Series add_bias_maybe_(const Series& a, ScalarType dtype) {
  if (a.defined()) return to_cuda_dtype_contiguous_(a, dtype);
  return Series();
}

void materialize_biases_(
    const ::pluss::ai::rnn::NetWeight& p,
    const ::pluss::ai::rnn::NetConfig& cfg,
    ScalarType dtype,
    std::int64_t bias_size,
    Series* bias_ih,
    Series* bias_hh) {
  const Series zero = zeros_cuda_({bias_size}, dtype);
  if (!cfg.use_bias) {
    *bias_ih = zero;
    *bias_hh = zero.clone();
    return;
  }

  if (cfg.split_bias) {
    *bias_ih = p.bias_ih.defined() ? to_cuda_dtype_contiguous_(p.bias_ih, dtype) : zero;
    *bias_hh = p.bias_hh.defined() ? to_cuda_dtype_contiguous_(p.bias_hh, dtype) : zero.clone();
    return;
  }

  const Series unified = add_bias_maybe_(p.bias, dtype);
  *bias_ih = unified.defined() ? unified : zero;
  *bias_hh = zero.clone();
}

bool supports_kind_(
    RnnKind kind,
    const Series& input,
    const Series& hx,
    const Series* cx,
    pluss::msharp::RangeView::Of<::pluss::ai::rnn::NetWeight> params,
    const ::pluss::ai::rnn::NetConfig& cfg,
    bool require_backward,
    std::string* reason) noexcept {
  if (!rnn_route_enabled()) {
    if (reason != nullptr) *reason = "cuDNN RNN route disabled by environment";
    return false;
  }
  if (require_backward) {
    if (reason != nullptr) *reason = "cuDNN RNN route is currently forward-only";
    return false;
  }
  if (input.platform() != pluss::pCUDA) {
    if (reason != nullptr) *reason = "input is not a CUDA tensor";
    return false;
  }
  if (input.dim() != 3) {
    if (reason != nullptr) *reason = "input must be rank-3 [time,batch,feature]";
    return false;
  }
  if (input.sizes()[0] <= 0) {
    if (reason != nullptr) *reason = "input sequence length must be > 0";
    return false;
  }
  if (cfg.num_layers <= 0) {
    if (reason != nullptr) *reason = "num_layers must be > 0";
    return false;
  }
  const std::int64_t dirs = cfg.bidirectional ? 2 : 1;
  const std::int64_t expected_blocks = cfg.num_layers * dirs;
  if (static_cast<std::int64_t>(params.size()) != expected_blocks) {
    if (reason != nullptr) *reason = "parameter block count does not match num_layers*dirs";
    return false;
  }

  const int gates = gate_count_(kind);
  const auto& p0 = params.front();
  if (!p0.weight_ih.defined() || !p0.weight_hh.defined() || p0.weight_ih.dim() != 2 || p0.weight_hh.dim() != 2) {
    if (reason != nullptr) *reason = "first parameter block is invalid";
    return false;
  }
  const std::int64_t hidden = p0.weight_hh.sizes()[0] / gates;
  const std::int64_t hidden_out = p0.weight_hh.sizes()[1];
  const std::int64_t input_size = input.sizes()[2];
  const std::int64_t batch = input.sizes()[1];
  const std::int64_t gate_hidden = static_cast<std::int64_t>(gates) * hidden;
  const bool projection_enabled = (kind == RnnKind::Lstm) && (cfg.proj_size > 0 || p0.weight_hr.defined());
  if (hidden <= 0 || hidden_out <= 0 || input_size <= 0 || batch <= 0 || p0.weight_hh.sizes()[0] != gate_hidden) {
    if (reason != nullptr) *reason = "invalid hidden/input geometry";
    return false;
  }
  if (kind != RnnKind::Lstm && (cfg.proj_size > 0 || p0.weight_hr.defined())) {
    if (reason != nullptr) *reason = "projection is only supported for LSTM";
    return false;
  }
  if (projection_enabled) {
    if (cfg.proj_size > 0 && cfg.proj_size != hidden_out) {
      if (reason != nullptr) *reason = "proj_size must match LSTM hidden_out geometry";
      return false;
    }
  } else {
    if (cfg.proj_size != 0) {
      if (reason != nullptr) *reason = "proj_size requires LSTM projection weights";
      return false;
    }
    if (hidden_out != hidden) {
      if (reason != nullptr) *reason = "hidden_out must match hidden when projection is disabled";
      return false;
    }
  }
  if (!hx.defined() || hx.dim() != 3 || hx.sizes()[0] != expected_blocks || hx.sizes()[1] != batch || hx.sizes()[2] != hidden_out) {
    if (reason != nullptr) *reason = "hx must have shape [num_layers*dirs,batch,hidden_out]";
    return false;
  }

  if (kind == RnnKind::Lstm) {
    if (cx == nullptr || !cx->defined() || cx->dim() != 3 || cx->sizes()[0] != expected_blocks || cx->sizes()[1] != batch ||
        cx->sizes()[2] != hidden) {
      if (reason != nullptr) *reason = "cx must have shape [num_layers*dirs,batch,hidden]";
      return false;
    }
  }

  for (std::int64_t layer = 0; layer < cfg.num_layers; ++layer) {
    const std::int64_t layer_input = (layer == 0) ? input_size : hidden_out * dirs;
    for (std::int64_t dir = 0; dir < dirs; ++dir) {
      const auto& p = params[static_cast<std::size_t>(layer * dirs + dir)];
      if (!p.weight_ih.defined() || !p.weight_hh.defined() || p.weight_ih.dim() != 2 || p.weight_hh.dim() != 2) {
        if (reason != nullptr) *reason = "parameter block requires rank-2 weight_ih and weight_hh";
        return false;
      }
      if (p.weight_ih.sizes()[0] != gate_hidden || p.weight_hh.sizes()[0] != gate_hidden) {
        if (reason != nullptr) *reason = "gate geometry mismatch between kind and parameter shapes";
        return false;
      }
      if (p.weight_ih.sizes()[1] != layer_input) {
        if (reason != nullptr) *reason = "weight_ih layer input size mismatch";
        return false;
      }
      if (p.weight_hh.sizes()[1] != hidden_out) {
        if (reason != nullptr) *reason = "weight_hh recurrent size mismatch";
        return false;
      }
      const bool has_b = p.bias.defined();
      const bool has_bih = p.bias_ih.defined();
      const bool has_bhh = p.bias_hh.defined();
      if (!cfg.use_bias && (has_b || has_bih || has_bhh)) {
        if (reason != nullptr) *reason = "use_bias=false does not allow bias tensors";
        return false;
      }
      if (cfg.use_bias && cfg.split_bias) {
        if (has_b) {
          if (reason != nullptr) *reason = "split_bias=true does not allow unified bias";
          return false;
        }
        if (has_bih != has_bhh) {
          if (reason != nullptr) *reason = "split_bias=true requires bias_ih and bias_hh together";
          return false;
        }
        if (!has_bih) {
          if (reason != nullptr) *reason = "use_bias=true and split_bias=true require both bias_ih and bias_hh";
          return false;
        }
      }
      if (cfg.use_bias && !cfg.split_bias) {
        if (has_bih || has_bhh) {
          if (reason != nullptr) *reason = "split_bias=false does not allow bias_ih/bias_hh";
          return false;
        }
        if (!has_b) {
          if (reason != nullptr) *reason = "use_bias=true and split_bias=false require unified bias";
          return false;
        }
      }
      if (p.bias.defined() && (p.bias.dim() != 1 || p.bias.sizes()[0] != gate_hidden)) {
        if (reason != nullptr) *reason = "bias shape mismatch";
        return false;
      }
      if (p.bias_ih.defined() && (p.bias_ih.dim() != 1 || p.bias_ih.sizes()[0] != gate_hidden)) {
        if (reason != nullptr) *reason = "bias_ih shape mismatch";
        return false;
      }
      if (p.bias_hh.defined() && (p.bias_hh.dim() != 1 || p.bias_hh.sizes()[0] != gate_hidden)) {
        if (reason != nullptr) *reason = "bias_hh shape mismatch";
        return false;
      }
      if (projection_enabled) {
        if (!p.weight_hr.defined() || p.weight_hr.dim() != 2 || p.weight_hr.sizes()[0] != hidden_out || p.weight_hr.sizes()[1] != hidden) {
          if (reason != nullptr) *reason = "weight_hr shape mismatch for projected LSTM";
          return false;
        }
      } else if (p.weight_hr.defined()) {
        if (reason != nullptr) *reason = "weight_hr requires projected LSTM configuration";
        return false;
      }
    }
  }

  cudnnDataType_t io_dtype{};
  std::string dtype_reason{};
  if (!map_dtype_(input.scalar_type(), &io_dtype, &dtype_reason)) {
    if (reason != nullptr) *reason = dtype_reason;
    return false;
  }
  (void)io_dtype;

  std::string mode_reason{};
  (void)mode_for_kind_(kind, cfg, &mode_reason);
  if (!mode_reason.empty()) {
    if (reason != nullptr) *reason = mode_reason;
    return false;
  }

  auto& ctx = cudnn_ctx_();
  if (!ctx.ready) {
    if (reason != nullptr) *reason = ctx.error;
    return false;
  }
  if (projection_enabled && ctx.api.set_rnn_projection_layers == nullptr) {
    if (reason != nullptr) *reason = "cuDNN runtime is missing LSTM projection symbol";
    return false;
  }
  if (cfg.training && (ctx.api.get_rnn_training_reserve_size == nullptr || ctx.api.rnn_forward_training == nullptr)) {
    if (reason != nullptr) *reason = "cuDNN runtime is missing training RNN symbols";
    return false;
  }
  return true;
}

bool copy_gate_weights_and_biases_(
    const CudnnApi& api,
    cudnnHandle_t handle,
    cudnnRNNDescriptor_t rnn_desc,
    cudnnTensorDescriptor_t x_desc,
    cudnnFilterDescriptor_t w_desc,
    Series* weight_space,
    RnnKind kind,
    pluss::msharp::RangeView::Of<::pluss::ai::rnn::NetWeight> params,
    const ::pluss::ai::rnn::NetConfig& cfg,
    bool projection_enabled,
    ScalarType dtype,
    std::string* reason) {
  const int gates = gate_count_(kind);
  const std::int64_t dirs = cfg.bidirectional ? 2 : 1;
  const std::int64_t hidden = params.front().weight_hh.sizes()[0] / gates;
  for (std::int64_t layer = 0; layer < cfg.num_layers; ++layer) {
    for (std::int64_t dir = 0; dir < dirs; ++dir) {
      const std::int64_t layer_idx = layer * dirs + dir;
      const auto& p = params[static_cast<std::size_t>(layer_idx)];
      const Series w_ih = to_cuda_dtype_contiguous_(p.weight_ih, dtype);
      const Series w_hh = to_cuda_dtype_contiguous_(p.weight_hh, dtype);
      Series b_ih;
      Series b_hh;
      materialize_biases_(p, cfg, dtype, static_cast<std::int64_t>(gates) * hidden, &b_ih, &b_hh);

      for (int g = 0; g < gates; ++g) {
        FilterDescGuard mat_ih_desc{&api};
        FilterDescGuard mat_hh_desc{&api};
        FilterDescGuard bias_ih_desc{&api};
        FilterDescGuard bias_hh_desc{&api};
        if (!check_status_(api, api.create_filter_desc(&mat_ih_desc.value), "cudnnCreateFilterDescriptor(mat_ih)", reason)) return false;
        if (!check_status_(api, api.create_filter_desc(&mat_hh_desc.value), "cudnnCreateFilterDescriptor(mat_hh)", reason)) return false;
        if (!check_status_(api, api.create_filter_desc(&bias_ih_desc.value), "cudnnCreateFilterDescriptor(bias_ih)", reason)) return false;
        if (!check_status_(api, api.create_filter_desc(&bias_hh_desc.value), "cudnnCreateFilterDescriptor(bias_hh)", reason)) return false;

        void* ih_ptr = nullptr;
        void* hh_ptr = nullptr;
        void* bi_ptr = nullptr;
        void* bh_ptr = nullptr;

        if (!check_status_(
                api,
                api.get_rnn_lin_layer_matrix(
                    handle,
                    rnn_desc,
                    static_cast<int>(layer_idx),
                    x_desc,
                    w_desc,
                    weight_space->raw_data(),
                    g,
                    mat_ih_desc.value,
                    &ih_ptr),
                "cudnnGetRNNLinLayerMatrixParams(weight_ih)",
                reason)) {
          return false;
        }
        if (!check_status_(
                api,
                api.get_rnn_lin_layer_matrix(
                    handle,
                    rnn_desc,
                    static_cast<int>(layer_idx),
                    x_desc,
                    w_desc,
                    weight_space->raw_data(),
                    g + gates,
                    mat_hh_desc.value,
                    &hh_ptr),
                "cudnnGetRNNLinLayerMatrixParams(weight_hh)",
                reason)) {
          return false;
        }
        if (!check_status_(
                api,
                api.get_rnn_lin_layer_bias(
                    handle,
                    rnn_desc,
                    static_cast<int>(layer_idx),
                    x_desc,
                    w_desc,
                    weight_space->raw_data(),
                    g,
                    bias_ih_desc.value,
                    &bi_ptr),
                "cudnnGetRNNLinLayerBiasParams(bias_ih)",
                reason)) {
          return false;
        }
        if (!check_status_(
                api,
                api.get_rnn_lin_layer_bias(
                    handle,
                    rnn_desc,
                    static_cast<int>(layer_idx),
                    x_desc,
                    w_desc,
                    weight_space->raw_data(),
                    g + gates,
                    bias_hh_desc.value,
                    &bh_ptr),
                "cudnnGetRNNLinLayerBiasParams(bias_hh)",
                reason)) {
          return false;
        }

        const Series ih_gate = ::pluss::detail::nn::narrow_dim_(w_ih, 0, static_cast<std::int64_t>(g) * hidden, hidden).contiguous();
        const Series hh_gate = ::pluss::detail::nn::narrow_dim_(w_hh, 0, static_cast<std::int64_t>(g) * hidden, hidden).contiguous();
        const Series bi_gate = ::pluss::detail::nn::narrow_dim_(b_ih, 0, static_cast<std::int64_t>(g) * hidden, hidden).contiguous();
        const Series bh_gate = ::pluss::detail::nn::narrow_dim_(b_hh, 0, static_cast<std::int64_t>(g) * hidden, hidden).contiguous();

        if (!cuda_copy_(
                ih_ptr,
                ih_gate,
                ih_gate.numel(),
                dtype,
                "copy weight_ih gate",
                reason)) {
          return false;
        }
        if (!cuda_copy_(
                hh_ptr,
                hh_gate,
                hh_gate.numel(),
                dtype,
                "copy weight_hh gate",
                reason)) {
          return false;
        }
        if (!cuda_copy_(
                bi_ptr,
                bi_gate,
                bi_gate.numel(),
                dtype,
                "copy bias_ih gate",
                reason)) {
          return false;
        }
        if (!cuda_copy_(
                bh_ptr,
                bh_gate,
                bh_gate.numel(),
                dtype,
                "copy bias_hh gate",
                reason)) {
          return false;
        }
      }
      if (projection_enabled) {
        FilterDescGuard proj_desc{&api};
        if (!check_status_(api, api.create_filter_desc(&proj_desc.value), "cudnnCreateFilterDescriptor(weight_hr)", reason)) return false;
        void* proj_ptr = nullptr;
        constexpr int proj_linear_id = 8;
        if (!check_status_(
                api,
                api.get_rnn_lin_layer_matrix(
                    handle,
                    rnn_desc,
                    static_cast<int>(layer_idx),
                    x_desc,
                    w_desc,
                    weight_space->raw_data(),
                    proj_linear_id,
                    proj_desc.value,
                    &proj_ptr),
                "cudnnGetRNNLinLayerMatrixParams(weight_hr)",
                reason)) {
          return false;
        }
        const Series w_hr = to_cuda_dtype_contiguous_(p.weight_hr, dtype);
        if (!cuda_copy_(
                proj_ptr,
                w_hr,
                w_hr.numel(),
                dtype,
                "copy weight_hr",
                reason)) {
          return false;
        }
      }
    }
  }
  return true;
}

bool cudnn_forward_kind_(
    RnnKind kind,
    const Series& input_raw,
    const Series& hx_raw,
    const Series* cx_raw,
    pluss::msharp::RangeView::Of<::pluss::ai::rnn::NetWeight> params,
    const ::pluss::ai::rnn::NetConfig& cfg,
    Series* output,
    Series* hy,
    Series* cy,
    std::string* reason) {
  auto& ctx = cudnn_ctx_();
  if (!ctx.ready) {
    if (reason != nullptr) *reason = ctx.error;
    return false;
  }
  const auto& api = ctx.api;

  const ScalarType dtype = input_raw.scalar_type();
  cudnnDataType_t io_dtype{};
  if (!map_dtype_(dtype, &io_dtype, reason)) return false;

  const Series input = to_cuda_dtype_contiguous_(input_raw, dtype);
  const Series hx = to_cuda_dtype_contiguous_(hx_raw, dtype);
  const Series cx = (kind == RnnKind::Lstm && cx_raw != nullptr) ? to_cuda_dtype_contiguous_(*cx_raw, dtype) : Series();

  const std::int64_t seq_len = input.sizes()[0];
  const std::int64_t batch = input.sizes()[1];
  const std::int64_t input_size = input.sizes()[2];
  const std::int64_t dirs = cfg.bidirectional ? 2 : 1;
  const std::int64_t hidden = params.front().weight_hh.sizes()[0] / gate_count_(kind);
  const std::int64_t hidden_out = params.front().weight_hh.sizes()[1];
  const bool projection_enabled = (kind == RnnKind::Lstm) && (cfg.proj_size > 0 || params.front().weight_hr.defined());
  const std::int64_t stacked = cfg.num_layers * dirs;

  HandleGuard handle{&api};
  if (!check_status_(api, api.create_handle(&handle.value), "cudnnCreate", reason)) return false;

  DropoutDescGuard dropout_desc{&api};
  if (!check_status_(api, api.create_dropout_desc(&dropout_desc.value), "cudnnCreateDropoutDescriptor", reason)) return false;
  size_t dropout_states_size = 0;
  if (!check_status_(api, api.get_dropout_states_size(handle.value, &dropout_states_size), "cudnnDropoutGetStatesSize", reason)) return false;
  WorkspaceGuard dropout_states{};
  if (dropout_states_size > 0) {
    if (cudaMalloc(&dropout_states.ptr, dropout_states_size) != cudaSuccess) {
      if (reason != nullptr) *reason = "cudaMalloc(dropout_states) failed";
      return false;
    }
  }
  const float dropout_p = static_cast<float>(cfg.training ? cfg.dropout : 0.0);
  if (!check_status_(
          api,
          api.set_dropout_desc(
              dropout_desc.value,
              handle.value,
              dropout_p,
              dropout_states.ptr,
              dropout_states_size,
              static_cast<unsigned long long>(cfg.seed)),
          "cudnnSetDropoutDescriptor",
          reason)) {
    return false;
  }
  RNNDescGuard rnn_desc{&api};
  if (!check_status_(api, api.create_rnn_desc(&rnn_desc.value), "cudnnCreateRNNDescriptor", reason)) return false;

  std::string mode_reason{};
  const cudnnRNNMode_t mode = mode_for_kind_(kind, cfg, &mode_reason);
  if (!mode_reason.empty()) {
    if (reason != nullptr) *reason = mode_reason;
    return false;
  }
  if (!check_status_(
          api,
          api.set_rnn_desc_v6(
              handle.value,
              rnn_desc.value,
              static_cast<int>(hidden),
              static_cast<int>(cfg.num_layers),
              dropout_desc.value,
              CUDNN_LINEAR_INPUT,
              cfg.bidirectional ? CUDNN_BIDIRECTIONAL : CUDNN_UNIDIRECTIONAL,
              mode,
              CUDNN_RNN_ALGO_STANDARD,
              io_dtype),
          "cudnnSetRNNDescriptor_v6",
          reason)) {
    return false;
  }
  if (projection_enabled) {
    if (api.set_rnn_projection_layers == nullptr) {
      if (reason != nullptr) *reason = "cuDNN runtime is missing LSTM projection symbol";
      return false;
    }
    if (!check_status_(
            api,
            api.set_rnn_projection_layers(
                handle.value,
                rnn_desc.value,
                static_cast<int>(hidden_out),
                0),
            "cudnnSetRNNProjectionLayers",
            reason)) {
      return false;
    }
  }

  std::vector<TensorDescGuard> x_desc(static_cast<std::size_t>(seq_len));
  std::vector<TensorDescGuard> y_desc(static_cast<std::size_t>(seq_len));
  std::vector<cudnnTensorDescriptor_t> x_desc_raw(static_cast<std::size_t>(seq_len));
  std::vector<cudnnTensorDescriptor_t> y_desc_raw(static_cast<std::size_t>(seq_len));
  for (std::int64_t t = 0; t < seq_len; ++t) {
    x_desc[static_cast<std::size_t>(t)].api = &api;
    y_desc[static_cast<std::size_t>(t)].api = &api;
    if (!check_status_(api, api.create_tensor_desc(&x_desc[static_cast<std::size_t>(t)].value), "cudnnCreateTensorDescriptor(x_t)", reason)) return false;
    if (!check_status_(api, api.create_tensor_desc(&y_desc[static_cast<std::size_t>(t)].value), "cudnnCreateTensorDescriptor(y_t)", reason)) return false;
    if (!set_3d_tensor_desc_(
            api,
            x_desc[static_cast<std::size_t>(t)].value,
            batch,
            input_size,
            1,
            io_dtype,
            "cudnnSetTensorNdDescriptor(x_t)",
            reason)) {
      return false;
    }
    if (!set_3d_tensor_desc_(
            api,
            y_desc[static_cast<std::size_t>(t)].value,
            batch,
            hidden_out * dirs,
            1,
            io_dtype,
            "cudnnSetTensorNdDescriptor(y_t)",
            reason)) {
      return false;
    }
    x_desc_raw[static_cast<std::size_t>(t)] = x_desc[static_cast<std::size_t>(t)].value;
    y_desc_raw[static_cast<std::size_t>(t)] = y_desc[static_cast<std::size_t>(t)].value;
  }

  TensorDescGuard hx_desc{&api};
  TensorDescGuard hy_desc{&api};
  if (!check_status_(api, api.create_tensor_desc(&hx_desc.value), "cudnnCreateTensorDescriptor(hx)", reason)) return false;
  if (!check_status_(api, api.create_tensor_desc(&hy_desc.value), "cudnnCreateTensorDescriptor(hy)", reason)) return false;
  if (!set_3d_tensor_desc_(api, hx_desc.value, stacked, batch, hidden_out, io_dtype, "cudnnSetTensorNdDescriptor(hx)", reason)) return false;
  if (!set_3d_tensor_desc_(api, hy_desc.value, stacked, batch, hidden_out, io_dtype, "cudnnSetTensorNdDescriptor(hy)", reason)) return false;

  TensorDescGuard cx_desc{&api};
  TensorDescGuard cy_desc{&api};
  if (kind == RnnKind::Lstm) {
    if (!check_status_(api, api.create_tensor_desc(&cx_desc.value), "cudnnCreateTensorDescriptor(cx)", reason)) return false;
    if (!check_status_(api, api.create_tensor_desc(&cy_desc.value), "cudnnCreateTensorDescriptor(cy)", reason)) return false;
    if (!set_3d_tensor_desc_(api, cx_desc.value, stacked, batch, hidden, io_dtype, "cudnnSetTensorNdDescriptor(cx)", reason)) return false;
    if (!set_3d_tensor_desc_(api, cy_desc.value, stacked, batch, hidden, io_dtype, "cudnnSetTensorNdDescriptor(cy)", reason)) return false;
  }

  size_t weight_bytes = 0;
  if (!check_status_(
          api,
          api.get_rnn_params_size(handle.value, rnn_desc.value, x_desc_raw.front(), &weight_bytes, io_dtype),
          "cudnnGetRNNParamsSize",
          reason)) {
    return false;
  }
  const std::size_t item_size = pluss::codem::dtype_of(dtype).itemsize;
  if (item_size == 0 || weight_bytes % item_size != 0) {
    if (reason != nullptr) *reason = "cuDNN returned invalid RNN parameter size";
    return false;
  }
  const std::int64_t weight_elems = static_cast<std::int64_t>(weight_bytes / item_size);
  if (!in_int_range_(weight_elems) || weight_elems <= 0) {
    if (reason != nullptr) *reason = "cuDNN parameter size exceeds limits";
    return false;
  }
  Series weight_space = zeros_cuda_({weight_elems}, dtype);

  FilterDescGuard w_desc{&api};
  if (!check_status_(api, api.create_filter_desc(&w_desc.value), "cudnnCreateFilterDescriptor(w)", reason)) return false;
  const std::array<int, 3> w_dims{static_cast<int>(weight_elems), 1, 1};
  if (!check_status_(
          api,
          api.set_filter_nd_desc(w_desc.value, io_dtype, CUDNN_TENSOR_NCHW, 3, w_dims.data()),
          "cudnnSetFilterNdDescriptor(w)",
          reason)) {
    return false;
  }

  if (!copy_gate_weights_and_biases_(
          api,
          handle.value,
          rnn_desc.value,
          x_desc_raw.front(),
          w_desc.value,
          &weight_space,
          kind,
          params,
          cfg,
          projection_enabled,
          dtype,
          reason)) {
    return false;
  }

  *output = zeros_cuda_({seq_len, batch, hidden_out * dirs}, dtype);
  *hy = zeros_cuda_({stacked, batch, hidden_out}, dtype);
  if (kind == RnnKind::Lstm && cy != nullptr) {
    *cy = zeros_cuda_({stacked, batch, hidden}, dtype);
  }

  size_t workspace_bytes = 0;
  if (!check_status_(
          api,
          api.get_rnn_workspace_size(handle.value, rnn_desc.value, static_cast<int>(seq_len), x_desc_raw.data(), &workspace_bytes),
          "cudnnGetRNNWorkspaceSize",
          reason)) {
    return false;
  }
  WorkspaceGuard workspace{};
  if (workspace_bytes > 0) {
    if (cudaMalloc(&workspace.ptr, workspace_bytes) != cudaSuccess) {
      if (reason != nullptr) *reason = "cudaMalloc(RNN workspace) failed";
      return false;
    }
  }
  const bool use_training_path = cfg.training;
  if (use_training_path && (api.get_rnn_training_reserve_size == nullptr || api.rnn_forward_training == nullptr)) {
    if (reason != nullptr) *reason = "cuDNN runtime is missing training RNN symbols";
    return false;
  }
  WorkspaceGuard reserve{};
  size_t reserve_bytes = 0;
  if (use_training_path) {
    if (!check_status_(
            api,
            api.get_rnn_training_reserve_size(
                handle.value,
                rnn_desc.value,
                static_cast<int>(seq_len),
                x_desc_raw.data(),
                &reserve_bytes),
            "cudnnGetRNNTrainingReserveSize",
            reason)) {
      return false;
    }
    if (reserve_bytes > 0) {
      if (cudaMalloc(&reserve.ptr, reserve_bytes) != cudaSuccess) {
        if (reason != nullptr) *reason = "cudaMalloc(RNN reserve) failed";
        return false;
      }
    }
  }

  const auto cx_desc_ptr = kind == RnnKind::Lstm ? cx_desc.value : nullptr;
  const auto cy_desc_ptr = kind == RnnKind::Lstm ? cy_desc.value : nullptr;
  const auto cx_ptr = kind == RnnKind::Lstm ? reinterpret_cast<const void*>(cx.raw_data()) : nullptr;
  auto* cy_ptr = kind == RnnKind::Lstm && cy != nullptr ? reinterpret_cast<void*>(cy->raw_data()) : nullptr;

  if (use_training_path) {
    if (!check_status_(
            api,
            api.rnn_forward_training(
                handle.value,
                rnn_desc.value,
                static_cast<int>(seq_len),
                x_desc_raw.data(),
                input.raw_data(),
                hx_desc.value,
                hx.raw_data(),
                cx_desc_ptr,
                cx_ptr,
                w_desc.value,
                weight_space.raw_data(),
                y_desc_raw.data(),
                output->raw_data(),
                hy_desc.value,
                hy->raw_data(),
                cy_desc_ptr,
                cy_ptr,
                workspace.ptr,
                workspace_bytes,
                reserve.ptr,
                reserve_bytes),
            "cudnnRNNForwardTraining",
            reason)) {
      return false;
    }
  } else {
    if (!check_status_(
            api,
            api.rnn_forward_inference(
                handle.value,
                rnn_desc.value,
                static_cast<int>(seq_len),
                x_desc_raw.data(),
                input.raw_data(),
                hx_desc.value,
                hx.raw_data(),
                cx_desc_ptr,
                cx_ptr,
                w_desc.value,
                weight_space.raw_data(),
                y_desc_raw.data(),
                output->raw_data(),
                hy_desc.value,
                hy->raw_data(),
                cy_desc_ptr,
                cy_ptr,
                workspace.ptr,
                workspace_bytes),
            "cudnnRNNForwardInference",
            reason)) {
      return false;
    }
  }

  return true;
}

#endif // PLUSS_AI_HAS_CUDNN_RNN_KERNELS && PLUSS_AI_HAS_DLOPEN

} // namespace

bool rnn_route_enabled() noexcept {
  return parse_bool_env_("PLUSS_CUDA_NATIVE_CUDNN_RNN", true);
}

bool rnn_route_strict() noexcept {
  return parse_bool_env_("PLUSS_CUDA_NATIVE_CUDNN_RNN_STRICT", false);
}

bool rnn_route_ready() noexcept {
#if PLUSS_AI_HAS_CUDNN_RNN_KERNELS && PLUSS_AI_HAS_DLOPEN
  return rnn_route_enabled() && cudnn_ctx_().ready;
#else
  return false;
#endif
}

bool supports_rnn(
    const Series& input,
    const Series& hx,
    pluss::msharp::RangeView::Of<::pluss::ai::rnn::NetWeight> params,
    const ::pluss::ai::rnn::NetConfig& cfg,
    bool require_backward,
    std::string* reason) noexcept {
#if PLUSS_AI_HAS_CUDNN_RNN_KERNELS && PLUSS_AI_HAS_DLOPEN
  return supports_kind_(RnnKind::Vanilla, input, hx, nullptr, params, cfg, require_backward, reason);
#else
  (void)input;
  (void)hx;
  (void)params;
  (void)cfg;
  (void)require_backward;
  if (reason != nullptr) *reason = "build/runtime does not expose cuDNN RNN headers+dlopen support";
  return false;
#endif
}

bool supports_gru(
    const Series& input,
    const Series& hx,
    pluss::msharp::RangeView::Of<::pluss::ai::rnn::NetWeight> params,
    const ::pluss::ai::rnn::NetConfig& cfg,
    bool require_backward,
    std::string* reason) noexcept {
#if PLUSS_AI_HAS_CUDNN_RNN_KERNELS && PLUSS_AI_HAS_DLOPEN
  return supports_kind_(RnnKind::Gru, input, hx, nullptr, params, cfg, require_backward, reason);
#else
  (void)input;
  (void)hx;
  (void)params;
  (void)cfg;
  (void)require_backward;
  if (reason != nullptr) *reason = "build/runtime does not expose cuDNN RNN headers+dlopen support";
  return false;
#endif
}

bool supports_lstm(
    const Series& input,
    const Series& hx,
    const Series& cx,
    pluss::msharp::RangeView::Of<::pluss::ai::rnn::NetWeight> params,
    const ::pluss::ai::rnn::NetConfig& cfg,
    bool require_backward,
    std::string* reason) noexcept {
#if PLUSS_AI_HAS_CUDNN_RNN_KERNELS && PLUSS_AI_HAS_DLOPEN
  return supports_kind_(RnnKind::Lstm, input, hx, &cx, params, cfg, require_backward, reason);
#else
  (void)input;
  (void)hx;
  (void)cx;
  (void)params;
  (void)cfg;
  (void)require_backward;
  if (reason != nullptr) *reason = "build/runtime does not expose cuDNN RNN headers+dlopen support";
  return false;
#endif
}

std::optional<std::tuple<Series, Series>> try_rnn(
    const Series& input,
    const Series& hx,
    pluss::msharp::RangeView::Of<::pluss::ai::rnn::NetWeight> params,
    const ::pluss::ai::rnn::NetConfig& cfg,
    bool require_backward) {
  if (!rnn_route_enabled()) return std::nullopt;
  if (input.platform() != pluss::pCUDA) return std::nullopt;

  std::string reason{};
  if (!supports_rnn(input, hx, params, cfg, require_backward, &reason)) {
    if (rnn_route_strict()) {
      PLUSS_FAIL(std::string("pluss::ai::rnn: strict cuDNN route unavailable: ") + reason);
    }
    return std::nullopt;
  }

#if PLUSS_AI_HAS_CUDNN_RNN_KERNELS && PLUSS_AI_HAS_DLOPEN
  Series y;
  Series hy;
  if (!cudnn_forward_kind_(RnnKind::Vanilla, input, hx, nullptr, params, cfg, &y, &hy, nullptr, &reason)) {
    if (rnn_route_strict()) {
      PLUSS_FAIL(std::string("pluss::ai::rnn: strict cuDNN forward failed: ") + reason);
    }
    return std::nullopt;
  }
  return std::make_optional(std::make_tuple(y, hy));
#else
  if (rnn_route_strict()) {
    PLUSS_FAIL("pluss::ai::rnn: strict cuDNN route requested but cuDNN RNN kernels are not compiled in this build");
  }
  return std::nullopt;
#endif
}

std::optional<std::tuple<Series, Series>> try_gru(
    const Series& input,
    const Series& hx,
    pluss::msharp::RangeView::Of<::pluss::ai::rnn::NetWeight> params,
    const ::pluss::ai::rnn::NetConfig& cfg,
    bool require_backward) {
  if (!rnn_route_enabled()) return std::nullopt;
  if (input.platform() != pluss::pCUDA) return std::nullopt;

  std::string reason{};
  if (!supports_gru(input, hx, params, cfg, require_backward, &reason)) {
    if (rnn_route_strict()) {
      PLUSS_FAIL(std::string("pluss::ai::gru: strict cuDNN route unavailable: ") + reason);
    }
    return std::nullopt;
  }

#if PLUSS_AI_HAS_CUDNN_RNN_KERNELS && PLUSS_AI_HAS_DLOPEN
  Series y;
  Series hy;
  if (!cudnn_forward_kind_(RnnKind::Gru, input, hx, nullptr, params, cfg, &y, &hy, nullptr, &reason)) {
    if (rnn_route_strict()) {
      PLUSS_FAIL(std::string("pluss::ai::gru: strict cuDNN forward failed: ") + reason);
    }
    return std::nullopt;
  }
  return std::make_optional(std::make_tuple(y, hy));
#else
  if (rnn_route_strict()) {
    PLUSS_FAIL("pluss::ai::gru: strict cuDNN route requested but cuDNN RNN kernels are not compiled in this build");
  }
  return std::nullopt;
#endif
}

std::optional<std::tuple<Series, std::tuple<Series, Series>>> try_lstm(
    const Series& input,
    const Series& hx,
    const Series& cx,
    pluss::msharp::RangeView::Of<::pluss::ai::rnn::NetWeight> params,
    const ::pluss::ai::rnn::NetConfig& cfg,
    bool require_backward) {
  if (!rnn_route_enabled()) return std::nullopt;
  if (input.platform() != pluss::pCUDA) return std::nullopt;

  std::string reason{};
  if (!supports_lstm(input, hx, cx, params, cfg, require_backward, &reason)) {
    if (rnn_route_strict()) {
      PLUSS_FAIL(std::string("pluss::ai::lstm: strict cuDNN route unavailable: ") + reason);
    }
    return std::nullopt;
  }

#if PLUSS_AI_HAS_CUDNN_RNN_KERNELS && PLUSS_AI_HAS_DLOPEN
  Series y;
  Series hy;
  Series cy;
  if (!cudnn_forward_kind_(RnnKind::Lstm, input, hx, &cx, params, cfg, &y, &hy, &cy, &reason)) {
    if (rnn_route_strict()) {
      PLUSS_FAIL(std::string("pluss::ai::lstm: strict cuDNN forward failed: ") + reason);
    }
    return std::nullopt;
  }
  return std::make_optional(std::make_tuple(y, std::make_tuple(hy, cy)));
#else
  if (rnn_route_strict()) {
    PLUSS_FAIL("pluss::ai::lstm: strict cuDNN route requested but cuDNN RNN kernels are not compiled in this build");
  }
  return std::nullopt;
#endif
}

} // namespace pluss::ai::kernels::cudnn
