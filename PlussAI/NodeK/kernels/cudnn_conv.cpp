#include "NodeK/kernels/cudnn_conv.h"

#include "CodeM/core/SeriesTape.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <unordered_map>

#if defined(__unix__) || defined(__APPLE__)
#include <dlfcn.h>
#define PLUSS_AI_HAS_DLOPEN 1
#else
#define PLUSS_AI_HAS_DLOPEN 0
#endif

#if defined(PLUSS_HAS_CUDA) && PLUSS_HAS_CUDA && __has_include(<cuda_runtime_api.h>) && __has_include(<cudnn.h>)
#include <cuda_runtime_api.h>
#include <cudnn.h>
#define PLUSS_AI_HAS_CUDNN_CONV_KERNELS 1
#else
#define PLUSS_AI_HAS_CUDNN_CONV_KERNELS 0
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

bool in_int_range_(std::int64_t v) noexcept {
  return v >= static_cast<std::int64_t>(std::numeric_limits<int>::min()) &&
      v <= static_cast<std::int64_t>(std::numeric_limits<int>::max());
}

#if PLUSS_AI_HAS_CUDNN_CONV_KERNELS && PLUSS_AI_HAS_DLOPEN

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
  decltype(&cudnnCreateConvolutionDescriptor) create_conv_desc{nullptr};
  decltype(&cudnnSetConvolution2dDescriptor) set_conv2d_desc{nullptr};
  decltype(&cudnnSetConvolutionGroupCount) set_conv_group_count{nullptr};
  decltype(&cudnnDestroyConvolutionDescriptor) destroy_conv_desc{nullptr};
  decltype(&cudnnGetConvolution2dForwardOutputDim) get_fwd_output_dim{nullptr};
  decltype(&cudnnGetConvolutionForwardWorkspaceSize) get_fwd_workspace{nullptr};
  decltype(&cudnnConvolutionForward) conv_fwd{nullptr};
  decltype(&cudnnGetConvolutionBackwardDataWorkspaceSize) get_bwd_data_workspace{nullptr};
  decltype(&cudnnConvolutionBackwardData) conv_bwd_data{nullptr};
  decltype(&cudnnGetConvolutionBackwardFilterWorkspaceSize) get_bwd_filter_workspace{nullptr};
  decltype(&cudnnConvolutionBackwardFilter) conv_bwd_filter{nullptr};
  decltype(&cudnnConvolutionBackwardBias) conv_bwd_bias{nullptr};

  bool load(void* handle, std::string* why) {
    auto load_sym = [&](auto& fn, const char* name) -> bool {
      fn = reinterpret_cast<std::remove_reference_t<decltype(fn)>>(dlsym(handle, name));
      if (fn == nullptr) {
        if (why != nullptr) {
          *why = std::string("cuDNN symbol missing: ") + name;
        }
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
        load_sym(create_filter_desc, "cudnnCreateFilterDescriptor") &&
        load_sym(set_filter_nd_desc, "cudnnSetFilterNdDescriptor") &&
        load_sym(destroy_filter_desc, "cudnnDestroyFilterDescriptor") &&
        load_sym(create_conv_desc, "cudnnCreateConvolutionDescriptor") &&
        load_sym(set_conv2d_desc, "cudnnSetConvolution2dDescriptor") &&
        load_sym(set_conv_group_count, "cudnnSetConvolutionGroupCount") &&
        load_sym(destroy_conv_desc, "cudnnDestroyConvolutionDescriptor") &&
        load_sym(get_fwd_output_dim, "cudnnGetConvolution2dForwardOutputDim") &&
        load_sym(get_fwd_workspace, "cudnnGetConvolutionForwardWorkspaceSize") &&
        load_sym(conv_fwd, "cudnnConvolutionForward") &&
        load_sym(get_bwd_data_workspace, "cudnnGetConvolutionBackwardDataWorkspaceSize") &&
        load_sym(conv_bwd_data, "cudnnConvolutionBackwardData") &&
        load_sym(get_bwd_filter_workspace, "cudnnGetConvolutionBackwardFilterWorkspaceSize") &&
        load_sym(conv_bwd_filter, "cudnnConvolutionBackwardFilter") &&
        load_sym(conv_bwd_bias, "cudnnConvolutionBackwardBias");
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
      if (lib == nullptr || *lib == '\0') {
        continue;
      }
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
  if (api.error_string == nullptr) {
    return "unknown cuDNN error";
  }
  const char* msg = api.error_string(st);
  return msg == nullptr ? std::string("unknown cuDNN error") : std::string(msg);
}

bool check_status_(const CudnnApi& api, cudnnStatus_t st, const char* what, std::string* reason) {
  if (st == CUDNN_STATUS_SUCCESS) {
    return true;
  }
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

struct ConvDescGuard {
  const CudnnApi* api{nullptr};
  cudnnConvolutionDescriptor_t value{nullptr};
  ~ConvDescGuard() {
    if (api != nullptr && value != nullptr && api->destroy_conv_desc != nullptr) {
      (void)api->destroy_conv_desc(value);
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

bool map_dtype_(
    ScalarType st,
    cudnnDataType_t* io_dtype,
    cudnnDataType_t* compute_dtype,
    std::string* reason) {
  if (io_dtype == nullptr || compute_dtype == nullptr) {
    if (reason != nullptr) *reason = "internal: null dtype outputs";
    return false;
  }
  switch (st) {
    case ScalarType::Float32:
      *io_dtype = CUDNN_DATA_FLOAT;
      *compute_dtype = CUDNN_DATA_FLOAT;
      return true;
    case ScalarType::Float16:
      *io_dtype = CUDNN_DATA_HALF;
      *compute_dtype = CUDNN_DATA_FLOAT;
      return true;
#if defined(CUDNN_DATA_BFLOAT16)
    case ScalarType::BFloat16:
      *io_dtype = CUDNN_DATA_BFLOAT16;
      *compute_dtype = CUDNN_DATA_FLOAT;
      return true;
#endif
    default:
      break;
  }
  if (reason != nullptr) {
    *reason = "only Float32/Float16/BFloat16 cuDNN conv lanes are currently supported";
  }
  return false;
}

bool as_int4_(
    const Series& s,
    std::array<int, 4>* dims,
    std::array<int, 4>* strides,
    std::string* reason) {
  if (s.dim() != 4) {
    if (reason != nullptr) *reason = "expected rank-4 tensor";
    return false;
  }
  for (std::size_t i = 0; i < 4; ++i) {
    const auto d = s.sizes()[i];
    const auto st = s.strides()[i];
    if (!in_int_range_(d) || !in_int_range_(st)) {
      if (reason != nullptr) *reason = "shape/stride exceeds cuDNN int32 limits";
      return false;
    }
    (*dims)[i] = static_cast<int>(d);
    (*strides)[i] = static_cast<int>(st);
  }
  return true;
}

struct ConvAlgoKey {
  std::array<int, 4> input_dims{};
  std::array<int, 4> weight_dims{};
  std::array<int, 2> stride{};
  std::array<int, 2> padding{};
  std::array<int, 2> dilation{};
  int groups{1};
  int dtype_tag{0};

  bool operator==(const ConvAlgoKey& other) const noexcept {
    return input_dims == other.input_dims &&
        weight_dims == other.weight_dims &&
        stride == other.stride &&
        padding == other.padding &&
        dilation == other.dilation &&
        groups == other.groups &&
        dtype_tag == other.dtype_tag;
  }
};

struct ConvAlgoKeyHash {
  std::size_t operator()(const ConvAlgoKey& k) const noexcept {
    auto mix = [](std::size_t seed, std::size_t v) {
      return seed ^ (v + 0x9e3779b9u + (seed << 6) + (seed >> 2));
    };
    std::size_t seed = 0;
    for (const int v : k.input_dims) seed = mix(seed, static_cast<std::size_t>(v));
    for (const int v : k.weight_dims) seed = mix(seed, static_cast<std::size_t>(v));
    for (const int v : k.stride) seed = mix(seed, static_cast<std::size_t>(v));
    for (const int v : k.padding) seed = mix(seed, static_cast<std::size_t>(v));
    for (const int v : k.dilation) seed = mix(seed, static_cast<std::size_t>(v));
    seed = mix(seed, static_cast<std::size_t>(k.groups));
    seed = mix(seed, static_cast<std::size_t>(k.dtype_tag));
    return seed;
  }
};

ConvAlgoKey make_algo_key_(const Series& input, const Series& weight, const Conv2dGeometry& geom) {
  std::array<int, 4> input_dims{
      static_cast<int>(input.sizes()[0]),
      static_cast<int>(input.sizes()[1]),
      static_cast<int>(input.sizes()[2]),
      static_cast<int>(input.sizes()[3]),
  };
  std::array<int, 4> weight_dims{
      static_cast<int>(weight.sizes()[0]),
      static_cast<int>(weight.sizes()[1]),
      static_cast<int>(weight.sizes()[2]),
      static_cast<int>(weight.sizes()[3]),
  };
  return ConvAlgoKey{
      .input_dims = input_dims,
      .weight_dims = weight_dims,
      .stride = {static_cast<int>(geom.stride[0]), static_cast<int>(geom.stride[1])},
      .padding = {static_cast<int>(geom.padding[0]), static_cast<int>(geom.padding[1])},
      .dilation = {static_cast<int>(geom.dilation[0]), static_cast<int>(geom.dilation[1])},
      .groups = static_cast<int>(geom.groups),
      .dtype_tag = static_cast<int>(input.scalar_type()),
  };
}

std::unordered_map<ConvAlgoKey, cudnnConvolutionFwdAlgo_t, ConvAlgoKeyHash>& fwd_algo_cache_() {
  static std::unordered_map<ConvAlgoKey, cudnnConvolutionFwdAlgo_t, ConvAlgoKeyHash> cache;
  return cache;
}

std::unordered_map<ConvAlgoKey, cudnnConvolutionBwdDataAlgo_t, ConvAlgoKeyHash>& bwd_data_algo_cache_() {
  static std::unordered_map<ConvAlgoKey, cudnnConvolutionBwdDataAlgo_t, ConvAlgoKeyHash> cache;
  return cache;
}

std::unordered_map<ConvAlgoKey, cudnnConvolutionBwdFilterAlgo_t, ConvAlgoKeyHash>& bwd_filter_algo_cache_() {
  static std::unordered_map<ConvAlgoKey, cudnnConvolutionBwdFilterAlgo_t, ConvAlgoKeyHash> cache;
  return cache;
}

std::mutex& algo_cache_mutex_() {
  static std::mutex m;
  return m;
}

bool prepare_common_(
    const CudnnApi& api,
    const Series& input,
    const Series& weight,
    const Conv2dGeometry& geom,
    HandleGuard* handle,
    TensorDescGuard* x_desc,
    FilterDescGuard* w_desc,
    ConvDescGuard* conv_desc,
    TensorDescGuard* y_desc,
    Series* output,
    std::string* reason) {
  if (!check_status_(api, api.create_handle(&handle->value), "cudnnCreate", reason)) return false;
  if (!check_status_(api, api.create_tensor_desc(&x_desc->value), "cudnnCreateTensorDescriptor(x)", reason)) return false;
  if (!check_status_(api, api.create_filter_desc(&w_desc->value), "cudnnCreateFilterDescriptor", reason)) return false;
  if (!check_status_(api, api.create_conv_desc(&conv_desc->value), "cudnnCreateConvolutionDescriptor", reason)) return false;
  if (!check_status_(api, api.create_tensor_desc(&y_desc->value), "cudnnCreateTensorDescriptor(y)", reason)) return false;

  std::array<int, 4> x_dims{};
  std::array<int, 4> x_strides{};
  if (!as_int4_(input, &x_dims, &x_strides, reason)) return false;
  std::array<int, 4> w_dims{};
  std::array<int, 4> w_strides{};
  if (!as_int4_(weight, &w_dims, &w_strides, reason)) return false;

  cudnnDataType_t io_dtype{};
  cudnnDataType_t compute_dtype{};
  if (!map_dtype_(input.scalar_type(), &io_dtype, &compute_dtype, reason)) return false;

  if (!check_status_(
          api,
          api.set_tensor_nd_desc(x_desc->value, io_dtype, 4, x_dims.data(), x_strides.data()),
          "cudnnSetTensorNdDescriptor(x)",
          reason)) {
    return false;
  }

  if (!check_status_(
          api,
          api.set_filter_nd_desc(w_desc->value, io_dtype, CUDNN_TENSOR_NCHW, 4, w_dims.data()),
          "cudnnSetFilterNdDescriptor",
          reason)) {
    return false;
  }

  if (!check_status_(
          api,
          api.set_conv2d_desc(
              conv_desc->value,
              static_cast<int>(geom.padding[0]),
              static_cast<int>(geom.padding[1]),
              static_cast<int>(geom.stride[0]),
              static_cast<int>(geom.stride[1]),
              static_cast<int>(geom.dilation[0]),
              static_cast<int>(geom.dilation[1]),
              CUDNN_CROSS_CORRELATION,
              compute_dtype),
          "cudnnSetConvolution2dDescriptor",
          reason)) {
    return false;
  }

  if (!check_status_(
          api,
          api.set_conv_group_count(conv_desc->value, static_cast<int>(geom.groups)),
          "cudnnSetConvolutionGroupCount",
          reason)) {
    return false;
  }

  int on = 0;
  int oc = 0;
  int oh = 0;
  int ow = 0;
  if (!check_status_(
          api,
          api.get_fwd_output_dim(conv_desc->value, x_desc->value, w_desc->value, &on, &oc, &oh, &ow),
          "cudnnGetConvolution2dForwardOutputDim",
          reason)) {
    return false;
  }
  if (on <= 0 || oc <= 0 || oh <= 0 || ow <= 0) {
    if (reason != nullptr) *reason = "cuDNN produced non-positive output shape";
    return false;
  }

  *output = pluss::series::zeros(
      {static_cast<std::int64_t>(on), static_cast<std::int64_t>(oc), static_cast<std::int64_t>(oh), static_cast<std::int64_t>(ow)},
      Config{}.with_dtype(input.scalar_type()).with_platform(input.platform()));

  std::array<int, 4> y_dims{};
  std::array<int, 4> y_strides{};
  if (!as_int4_(*output, &y_dims, &y_strides, reason)) return false;
  if (!check_status_(
          api,
          api.set_tensor_nd_desc(y_desc->value, io_dtype, 4, y_dims.data(), y_strides.data()),
          "cudnnSetTensorNdDescriptor(y)",
          reason)) {
    return false;
  }
  return true;
}

bool try_forward_algo_(
    const CudnnApi& api,
    cudnnHandle_t handle,
    cudnnTensorDescriptor_t x_desc,
    cudnnFilterDescriptor_t w_desc,
    cudnnConvolutionDescriptor_t conv_desc,
    cudnnTensorDescriptor_t y_desc,
    const Series& input,
    const Series& weight,
    Series* output,
    cudnnConvolutionFwdAlgo_t algo,
    std::string* reason) {
  DeviceBuffer d_input{};
  DeviceBuffer d_weight{};
  DeviceBuffer d_output{};
  if (!alloc_device_buffer_(&d_input, bytes_of_(input), reason)) return false;
  if (!alloc_device_buffer_(&d_weight, bytes_of_(weight), reason)) return false;
  if (!alloc_device_buffer_(&d_output, bytes_of_(*output), reason)) return false;
  if (!copy_h2d_(d_input.ptr, input.raw_data(), d_input.bytes, reason)) return false;
  if (!copy_h2d_(d_weight.ptr, weight.raw_data(), d_weight.bytes, reason)) return false;

  size_t workspace_bytes = 0;
  if (api.get_fwd_workspace(handle, x_desc, w_desc, conv_desc, y_desc, algo, &workspace_bytes) != CUDNN_STATUS_SUCCESS) {
    return false;
  }
  WorkspaceGuard workspace{};
  if (workspace_bytes > 0) {
    if (cudaMalloc(&workspace.ptr, workspace_bytes) != cudaSuccess) {
      return false;
    }
  }
  const float alpha = 1.0f;
  const float beta = 0.0f;
  if (!check_status_(
          api,
          api.conv_fwd(
              handle,
              &alpha,
              x_desc,
              d_input.ptr,
              w_desc,
              d_weight.ptr,
              conv_desc,
              algo,
              workspace.ptr,
              workspace_bytes,
              &beta,
              y_desc,
              d_output.ptr),
          "cudnnConvolutionForward",
          reason)) {
    return false;
  }
  if (!copy_d2h_(output->raw_mutable_data(), d_output.ptr, d_output.bytes, reason)) return false;
  return true;
}

bool try_backward_data_algo_(
    const CudnnApi& api,
    cudnnHandle_t handle,
    cudnnFilterDescriptor_t w_desc,
    cudnnTensorDescriptor_t go_desc,
    cudnnConvolutionDescriptor_t conv_desc,
    cudnnTensorDescriptor_t dx_desc,
    const Series& weight,
    const Series& grad_out,
    Series* grad_input,
    cudnnConvolutionBwdDataAlgo_t algo,
    std::string* reason) {
  DeviceBuffer d_weight{};
  DeviceBuffer d_grad_out{};
  DeviceBuffer d_grad_input{};
  if (!alloc_device_buffer_(&d_weight, bytes_of_(weight), reason)) return false;
  if (!alloc_device_buffer_(&d_grad_out, bytes_of_(grad_out), reason)) return false;
  if (!alloc_device_buffer_(&d_grad_input, bytes_of_(*grad_input), reason)) return false;
  if (!copy_h2d_(d_weight.ptr, weight.raw_data(), d_weight.bytes, reason)) return false;
  if (!copy_h2d_(d_grad_out.ptr, grad_out.raw_data(), d_grad_out.bytes, reason)) return false;

  size_t workspace_bytes = 0;
  if (api.get_bwd_data_workspace(handle, w_desc, go_desc, conv_desc, dx_desc, algo, &workspace_bytes) != CUDNN_STATUS_SUCCESS) {
    return false;
  }
  WorkspaceGuard workspace{};
  if (workspace_bytes > 0) {
    if (cudaMalloc(&workspace.ptr, workspace_bytes) != cudaSuccess) {
      return false;
    }
  }
  const float alpha = 1.0f;
  const float beta = 0.0f;
  if (!check_status_(
          api,
          api.conv_bwd_data(
              handle,
              &alpha,
              w_desc,
              d_weight.ptr,
              go_desc,
              d_grad_out.ptr,
              conv_desc,
              algo,
              workspace.ptr,
              workspace_bytes,
              &beta,
              dx_desc,
              d_grad_input.ptr),
          "cudnnConvolutionBackwardData",
          reason)) {
    return false;
  }
  if (!copy_d2h_(grad_input->raw_mutable_data(), d_grad_input.ptr, d_grad_input.bytes, reason)) return false;
  return true;
}

bool try_backward_filter_algo_(
    const CudnnApi& api,
    cudnnHandle_t handle,
    cudnnTensorDescriptor_t x_desc,
    cudnnTensorDescriptor_t go_desc,
    cudnnConvolutionDescriptor_t conv_desc,
    cudnnFilterDescriptor_t dw_desc,
    const Series& input,
    const Series& grad_out,
    Series* grad_weight,
    cudnnConvolutionBwdFilterAlgo_t algo,
    std::string* reason) {
  DeviceBuffer d_input{};
  DeviceBuffer d_grad_out{};
  DeviceBuffer d_grad_weight{};
  if (!alloc_device_buffer_(&d_input, bytes_of_(input), reason)) return false;
  if (!alloc_device_buffer_(&d_grad_out, bytes_of_(grad_out), reason)) return false;
  if (!alloc_device_buffer_(&d_grad_weight, bytes_of_(*grad_weight), reason)) return false;
  if (!copy_h2d_(d_input.ptr, input.raw_data(), d_input.bytes, reason)) return false;
  if (!copy_h2d_(d_grad_out.ptr, grad_out.raw_data(), d_grad_out.bytes, reason)) return false;

  size_t workspace_bytes = 0;
  if (api.get_bwd_filter_workspace(handle, x_desc, go_desc, conv_desc, dw_desc, algo, &workspace_bytes) != CUDNN_STATUS_SUCCESS) {
    return false;
  }
  WorkspaceGuard workspace{};
  if (workspace_bytes > 0) {
    if (cudaMalloc(&workspace.ptr, workspace_bytes) != cudaSuccess) {
      return false;
    }
  }
  const float alpha = 1.0f;
  const float beta = 0.0f;
  if (!check_status_(
          api,
          api.conv_bwd_filter(
              handle,
              &alpha,
              x_desc,
              d_input.ptr,
              go_desc,
              d_grad_out.ptr,
              conv_desc,
              algo,
              workspace.ptr,
              workspace_bytes,
              &beta,
              dw_desc,
              d_grad_weight.ptr),
          "cudnnConvolutionBackwardFilter",
          reason)) {
    return false;
  }
  if (!copy_d2h_(grad_weight->raw_mutable_data(), d_grad_weight.ptr, d_grad_weight.bytes, reason)) return false;
  return true;
}

bool cudnn_forward_(
    const Series& input_raw,
    const Series& weight_raw,
    const Series& bias_raw,
    const Conv2dGeometry& geom,
    Series* out,
    std::string* reason) {
  auto& ctx = cudnn_ctx_();
  if (!ctx.ready) {
    if (reason != nullptr) *reason = ctx.error;
    return false;
  }
  const auto& api = ctx.api;
  const Series input = input_raw.is_contiguous() ? input_raw : input_raw.contiguous();
  const Series weight = weight_raw.is_contiguous() ? weight_raw : weight_raw.contiguous();
  const Series bias = bias_raw.defined() ? (bias_raw.is_contiguous() ? bias_raw : bias_raw.contiguous()) : pluss::series::undefined();

  HandleGuard handle{&api};
  TensorDescGuard x_desc{&api};
  FilterDescGuard w_desc{&api};
  ConvDescGuard conv_desc{&api};
  TensorDescGuard y_desc{&api};
  Series output = pluss::series::undefined();
  if (!prepare_common_(
          api,
          input,
          weight,
          geom,
          &handle,
          &x_desc,
          &w_desc,
          &conv_desc,
          &y_desc,
          &output,
          reason)) {
    return false;
  }

  const ConvAlgoKey key = make_algo_key_(input, weight, geom);
  bool executed = false;

  {
    std::lock_guard<std::mutex> lock(algo_cache_mutex_());
    const auto it = fwd_algo_cache_().find(key);
    if (it != fwd_algo_cache_().end()) {
      executed = try_forward_algo_(
          api,
          handle.value,
          x_desc.value,
          w_desc.value,
          conv_desc.value,
          y_desc.value,
          input,
          weight,
          &output,
          it->second,
          reason);
    }
  }

  if (!executed) {
    constexpr std::array<cudnnConvolutionFwdAlgo_t, 3> candidates{
        CUDNN_CONVOLUTION_FWD_ALGO_IMPLICIT_PRECOMP_GEMM,
        CUDNN_CONVOLUTION_FWD_ALGO_IMPLICIT_GEMM,
        CUDNN_CONVOLUTION_FWD_ALGO_DIRECT,
    };
    for (const auto algo : candidates) {
      if (try_forward_algo_(
              api,
              handle.value,
              x_desc.value,
              w_desc.value,
              conv_desc.value,
              y_desc.value,
              input,
              weight,
              &output,
              algo,
              reason)) {
        std::lock_guard<std::mutex> lock(algo_cache_mutex_());
        fwd_algo_cache_()[key] = algo;
        executed = true;
        break;
      }
    }
  }

  if (!executed) {
    if (reason != nullptr && reason->empty()) {
      *reason = "no valid cuDNN forward convolution algorithm succeeded";
    }
    return false;
  }

  if (bias.defined()) {
    output = output.add(bias.reshape({1, bias.sizes()[0], 1, 1}));
  }

  *out = output;
  return true;
}

bool cudnn_backward_input_(
    const Series& grad_out_raw,
    const Series& weight_raw,
    const Series& input_like_raw,
    const Conv2dGeometry& geom,
    Series* grad_input,
    std::string* reason) {
  auto& ctx = cudnn_ctx_();
  if (!ctx.ready) {
    if (reason != nullptr) *reason = ctx.error;
    return false;
  }
  const auto& api = ctx.api;

  const Series grad_out = grad_out_raw.is_contiguous() ? grad_out_raw : grad_out_raw.contiguous();
  const Series weight = weight_raw.is_contiguous() ? weight_raw : weight_raw.contiguous();
  const Series input_like = input_like_raw.is_contiguous() ? input_like_raw : input_like_raw.contiguous();

  HandleGuard handle{&api};
  TensorDescGuard x_desc{&api};
  FilterDescGuard w_desc{&api};
  ConvDescGuard conv_desc{&api};
  TensorDescGuard y_desc{&api};
  Series output = pluss::series::undefined();
  if (!prepare_common_(
          api,
          input_like,
          weight,
          geom,
          &handle,
          &x_desc,
          &w_desc,
          &conv_desc,
          &y_desc,
          &output,
          reason)) {
    return false;
  }
  (void)output;

  std::array<int, 4> go_dims{};
  std::array<int, 4> go_strides{};
  if (!as_int4_(grad_out, &go_dims, &go_strides, reason)) return false;

  cudnnDataType_t io_dtype{};
  cudnnDataType_t compute_dtype{};
  if (!map_dtype_(grad_out.scalar_type(), &io_dtype, &compute_dtype, reason)) return false;
  (void)compute_dtype;
  if (!check_status_(
          api,
          api.set_tensor_nd_desc(y_desc.value, io_dtype, 4, go_dims.data(), go_strides.data()),
          "cudnnSetTensorNdDescriptor(grad_out)",
          reason)) {
    return false;
  }

  Series dx = pluss::series::zeros(input_like.sizes(), Config{}.with_dtype(input_like.scalar_type()).with_platform(input_like.platform()));
  std::array<int, 4> dx_dims{};
  std::array<int, 4> dx_strides{};
  if (!as_int4_(dx, &dx_dims, &dx_strides, reason)) return false;
  if (!check_status_(
          api,
          api.set_tensor_nd_desc(x_desc.value, io_dtype, 4, dx_dims.data(), dx_strides.data()),
          "cudnnSetTensorNdDescriptor(dx)",
          reason)) {
    return false;
  }

  const ConvAlgoKey key = make_algo_key_(input_like, weight, geom);
  bool executed = false;

  {
    std::lock_guard<std::mutex> lock(algo_cache_mutex_());
    const auto it = bwd_data_algo_cache_().find(key);
    if (it != bwd_data_algo_cache_().end()) {
      executed = try_backward_data_algo_(
          api,
          handle.value,
          w_desc.value,
          y_desc.value,
          conv_desc.value,
          x_desc.value,
          weight,
          grad_out,
          &dx,
          it->second,
          reason);
    }
  }

  if (!executed) {
    constexpr std::array<cudnnConvolutionBwdDataAlgo_t, 2> candidates{
        CUDNN_CONVOLUTION_BWD_DATA_ALGO_1,
        CUDNN_CONVOLUTION_BWD_DATA_ALGO_0,
    };
    for (const auto algo : candidates) {
      if (try_backward_data_algo_(
              api,
              handle.value,
              w_desc.value,
              y_desc.value,
              conv_desc.value,
              x_desc.value,
              weight,
              grad_out,
              &dx,
              algo,
              reason)) {
        std::lock_guard<std::mutex> lock(algo_cache_mutex_());
        bwd_data_algo_cache_()[key] = algo;
        executed = true;
        break;
      }
    }
  }

  if (!executed) {
    if (reason != nullptr && reason->empty()) {
      *reason = "no valid cuDNN backward-data convolution algorithm succeeded";
    }
    return false;
  }

  *grad_input = dx;
  return true;
}

bool cudnn_backward_weight_(
    const Series& grad_out_raw,
    const Series& input_raw,
    const Series& weight_like_raw,
    const Conv2dGeometry& geom,
    Series* grad_weight,
    std::string* reason) {
  auto& ctx = cudnn_ctx_();
  if (!ctx.ready) {
    if (reason != nullptr) *reason = ctx.error;
    return false;
  }
  const auto& api = ctx.api;

  const Series grad_out = grad_out_raw.is_contiguous() ? grad_out_raw : grad_out_raw.contiguous();
  const Series input = input_raw.is_contiguous() ? input_raw : input_raw.contiguous();
  const Series weight_like = weight_like_raw.is_contiguous() ? weight_like_raw : weight_like_raw.contiguous();

  HandleGuard handle{&api};
  TensorDescGuard x_desc{&api};
  FilterDescGuard w_desc{&api};
  ConvDescGuard conv_desc{&api};
  TensorDescGuard y_desc{&api};
  Series output = pluss::series::undefined();
  if (!prepare_common_(
          api,
          input,
          weight_like,
          geom,
          &handle,
          &x_desc,
          &w_desc,
          &conv_desc,
          &y_desc,
          &output,
          reason)) {
    return false;
  }
  (void)output;

  std::array<int, 4> go_dims{};
  std::array<int, 4> go_strides{};
  if (!as_int4_(grad_out, &go_dims, &go_strides, reason)) return false;

  cudnnDataType_t io_dtype{};
  cudnnDataType_t compute_dtype{};
  if (!map_dtype_(grad_out.scalar_type(), &io_dtype, &compute_dtype, reason)) return false;
  (void)compute_dtype;
  if (!check_status_(
          api,
          api.set_tensor_nd_desc(y_desc.value, io_dtype, 4, go_dims.data(), go_strides.data()),
          "cudnnSetTensorNdDescriptor(grad_out)",
          reason)) {
    return false;
  }

  Series dw = pluss::series::zeros(weight_like.sizes(), Config{}.with_dtype(weight_like.scalar_type()).with_platform(weight_like.platform()));
  std::array<int, 4> w_dims{};
  std::array<int, 4> w_strides{};
  if (!as_int4_(dw, &w_dims, &w_strides, reason)) return false;
  if (!check_status_(
          api,
          api.set_filter_nd_desc(w_desc.value, io_dtype, CUDNN_TENSOR_NCHW, 4, w_dims.data()),
          "cudnnSetFilterNdDescriptor(dw)",
          reason)) {
    return false;
  }

  const ConvAlgoKey key = make_algo_key_(input, weight_like, geom);
  bool executed = false;

  {
    std::lock_guard<std::mutex> lock(algo_cache_mutex_());
    const auto it = bwd_filter_algo_cache_().find(key);
    if (it != bwd_filter_algo_cache_().end()) {
      executed = try_backward_filter_algo_(
          api,
          handle.value,
          x_desc.value,
          y_desc.value,
          conv_desc.value,
          w_desc.value,
          input,
          grad_out,
          &dw,
          it->second,
          reason);
    }
  }

  if (!executed) {
    constexpr std::array<cudnnConvolutionBwdFilterAlgo_t, 2> candidates{
        CUDNN_CONVOLUTION_BWD_FILTER_ALGO_1,
        CUDNN_CONVOLUTION_BWD_FILTER_ALGO_0,
    };
    for (const auto algo : candidates) {
      if (try_backward_filter_algo_(
              api,
              handle.value,
              x_desc.value,
              y_desc.value,
              conv_desc.value,
              w_desc.value,
              input,
              grad_out,
              &dw,
              algo,
              reason)) {
        std::lock_guard<std::mutex> lock(algo_cache_mutex_());
        bwd_filter_algo_cache_()[key] = algo;
        executed = true;
        break;
      }
    }
  }

  if (!executed) {
    if (reason != nullptr && reason->empty()) {
      *reason = "no valid cuDNN backward-filter convolution algorithm succeeded";
    }
    return false;
  }

  *grad_weight = dw;
  return true;
}

#endif // PLUSS_AI_HAS_CUDNN_CONV_KERNELS && PLUSS_AI_HAS_DLOPEN

} // namespace

bool route_enabled() noexcept {
  return parse_bool_env_("PLUSS_CUDA_NATIVE_CUDNN_CONV", true);
}

bool route_strict() noexcept {
  return parse_bool_env_("PLUSS_CUDA_NATIVE_CUDNN_CONV_STRICT", false);
}

bool route_ready() noexcept {
#if PLUSS_AI_HAS_CUDNN_CONV_KERNELS && PLUSS_AI_HAS_DLOPEN
  return route_enabled() && cudnn_ctx_().ready;
#else
  return false;
#endif
}

bool supports_conv2d(
    const Series& input,
    const Series& weight,
    const Series& bias,
    const Conv2dGeometry& geom,
    bool require_backward,
    std::string* reason) noexcept {
  if (!route_enabled()) {
    if (reason != nullptr) *reason = "cuDNN conv route disabled by environment";
    return false;
  }
  if (input.platform() != pluss::pCUDA || weight.platform() != pluss::pCUDA) {
    if (reason != nullptr) *reason = "input/weight are not CUDA tensors";
    return false;
  }
  if (bias.defined() && bias.platform() != pluss::pCUDA) {
    if (reason != nullptr) *reason = "bias platform must match CUDA input";
    return false;
  }
  if (input.dim() != 4 || weight.dim() != 4) {
    if (reason != nullptr) *reason = "cuDNN conv route currently supports rank-4 tensors only";
    return false;
  }
  if (geom.groups <= 0 || geom.stride[0] <= 0 || geom.stride[1] <= 0 || geom.dilation[0] <= 0 || geom.dilation[1] <= 0) {
    if (reason != nullptr) *reason = "invalid groups/stride/dilation geometry";
    return false;
  }
  if (geom.padding[0] < 0 || geom.padding[1] < 0) {
    if (reason != nullptr) *reason = "padding must be non-negative";
    return false;
  }
  if (input.scalar_type() != weight.scalar_type()) {
    if (reason != nullptr) *reason = "input/weight dtype mismatch";
    return false;
  }
  if (bias.defined() && bias.scalar_type() != input.scalar_type()) {
    if (reason != nullptr) *reason = "bias dtype mismatch";
    return false;
  }
  if (input.sizes()[1] % geom.groups != 0 || weight.sizes()[0] % geom.groups != 0) {
    if (reason != nullptr) *reason = "channels not divisible by groups";
    return false;
  }
  if (weight.sizes()[1] * geom.groups != input.sizes()[1]) {
    if (reason != nullptr) *reason = "weight channel geometry mismatch";
    return false;
  }
  if (bias.defined() && (bias.dim() != 1 || bias.sizes()[0] != weight.sizes()[0])) {
    if (reason != nullptr) *reason = "bias shape mismatch";
    return false;
  }
  if (require_backward && (!input.is_contiguous() || !weight.is_contiguous() || (bias.defined() && !bias.is_contiguous()))) {
    if (reason != nullptr) *reason = "tracked cuDNN conv route currently requires contiguous tensors";
    return false;
  }

#if PLUSS_AI_HAS_CUDNN_CONV_KERNELS && PLUSS_AI_HAS_DLOPEN
  {
    cudnnDataType_t io_dtype{};
    cudnnDataType_t compute_dtype{};
    std::string dtype_reason{};
    if (!map_dtype_(input.scalar_type(), &io_dtype, &compute_dtype, &dtype_reason)) {
      if (reason != nullptr) *reason = dtype_reason;
      return false;
    }
    (void)io_dtype;
    (void)compute_dtype;
  }
  auto& ctx = cudnn_ctx_();
  if (!ctx.ready) {
    if (reason != nullptr) *reason = ctx.error;
    return false;
  }
  return true;
#else
  if (reason != nullptr) *reason = "build/runtime does not expose cuDNN headers+dlopen support";
  return false;
#endif
}

std::optional<Series> try_conv2d(
    const Series& input,
    const Series& weight,
    const Series& bias,
    const Conv2dGeometry& geom,
    bool require_backward) {
  if (!route_enabled()) {
    return std::nullopt;
  }
  if (input.platform() != pluss::pCUDA) {
    return std::nullopt;
  }

  std::string reason{};
  if (!supports_conv2d(input, weight, bias, geom, require_backward, &reason)) {
    if (route_strict()) {
      PLUSS_FAIL(std::string("pluss::ai::conv: strict cuDNN route unavailable: ") + reason);
    }
    return std::nullopt;
  }

#if PLUSS_AI_HAS_CUDNN_CONV_KERNELS && PLUSS_AI_HAS_DLOPEN
  Series out = pluss::series::undefined();
  if (!cudnn_forward_(input, weight, bias, geom, &out, &reason)) {
    if (route_strict()) {
      PLUSS_FAIL(std::string("pluss::ai::conv: strict cuDNN forward failed: ") + reason);
    }
    return std::nullopt;
  }

  if (require_backward && pluss::codem::series_tape::recording_enabled()) {
    const bool track_input = input.tracks_slope();
    const bool track_weight = weight.tracks_slope();
    const bool track_bias = bias.defined() && bias.tracks_slope();
    if (track_input || track_weight || track_bias) {
      auto link = std::make_shared<pluss::codem::series_tape::SourceLink>();
      link->op_name = "cudnn_conv2d";
      link->parents = track_bias ? std::vector<Series>{input, weight, bias} : std::vector<Series>{input, weight};
      link->propagate = [input, weight, bias, geom, track_input, track_weight, track_bias](const Series& output_sens) {
        pluss::codem::series_tape::PauseRecording pause;
        const Series go = output_sens.platform() == input.platform() && output_sens.scalar_type() == input.scalar_type()
            ? (output_sens.is_contiguous() ? output_sens : output_sens.contiguous())
            : output_sens.to(input.platform(), input.scalar_type()).contiguous();

        if (track_input) {
          Series grad_input = pluss::series::undefined();
          std::string why{};
          if (!cudnn_backward_input_(go, weight, input, geom, &grad_input, &why)) {
            PLUSS_FAIL(std::string("pluss::ai::conv backward(input): ") + why);
          }
          pluss::codem::series_tape::accumulate_sensitivity(input, grad_input);
        }
        if (track_weight) {
          Series grad_weight = pluss::series::undefined();
          std::string why{};
          if (!cudnn_backward_weight_(go, input, weight, geom, &grad_weight, &why)) {
            PLUSS_FAIL(std::string("pluss::ai::conv backward(weight): ") + why);
          }
          pluss::codem::series_tape::accumulate_sensitivity(weight, grad_weight);
        }
        if (track_bias) {
          Series grad_bias = go.sum(0).sum(1).sum(1);
          if (grad_bias.scalar_type() != bias.scalar_type()) {
            grad_bias = grad_bias.to(bias.scalar_type());
          }
          if (grad_bias.platform() != bias.platform()) {
            grad_bias = grad_bias.to(bias.platform());
          }
          pluss::codem::series_tape::accumulate_sensitivity(bias, grad_bias);
        }
      };
      pluss::codem::series_tape::bind_source_link(out, std::move(link));
    }
  }

  return out;
#else
  (void)weight;
  (void)bias;
  (void)geom;
  (void)require_backward;
  if (route_strict()) {
    PLUSS_FAIL("pluss::ai::conv: strict cuDNN route requested but cuDNN kernels are not compiled in this build");
  }
  return std::nullopt;
#endif
}

} // namespace pluss::ai::kernels::cudnn
