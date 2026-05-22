#include "NodeK/kernels/mps_conv.h"

#if defined(__APPLE__)

#include "CodeM/core/Errors.h"
#include "CodeM/core/SeriesTape.h"
#include "CodeM/native/mps/BufferCacheMM.h"
#include "CodeM/native/mps/MPSRuntimeMM.h"

#import <Metal/Metal.h>
#import <MetalPerformanceShadersGraph/MPSGraph.h>
#import <MetalPerformanceShadersGraph/MPSGraphArithmeticOps.h>
#import <MetalPerformanceShadersGraph/MPSGraphConvolutionOps.h>
#import <MetalPerformanceShadersGraph/MPSGraphMemoryOps.h>
#import <MetalPerformanceShadersGraph/MPSGraphTensorData.h>

#include <atomic>
#include <cctype>
#include <cstdlib>
#include <mutex>
#include <string>

namespace pluss::ai::kernels::mps {
namespace {

struct Context {
  bool ready{false};
  id<MTLDevice> device{nil};
  id<MTLCommandQueue> queue{nil};
  std::string error{};
};

struct Conv3dGeometry {
  std::array<std::int64_t, 3> stride{1, 1, 1};
  std::array<std::int64_t, 3> padding{0, 0, 0};
  std::array<std::int64_t, 3> dilation{1, 1, 1};
  std::int64_t groups{1};
};

std::atomic<std::uint64_t>& conv3d_calls_() {
  static std::atomic<std::uint64_t> v{0};
  return v;
}

bool ieq_(const char* a, const char* b) noexcept {
  if (a == nullptr || b == nullptr) return false;
  while (*a != '\0' && *b != '\0') {
    if (std::tolower(static_cast<unsigned char>(*a)) != std::tolower(static_cast<unsigned char>(*b))) return false;
    ++a;
    ++b;
  }
  return *a == '\0' && *b == '\0';
}

bool parse_bool_env_(const char* key, bool fallback) noexcept {
  const char* raw = std::getenv(key);
  if (raw == nullptr || *raw == '\0') return fallback;
  if (ieq_(raw, "1") || ieq_(raw, "true") || ieq_(raw, "yes") || ieq_(raw, "on")) return true;
  if (ieq_(raw, "0") || ieq_(raw, "false") || ieq_(raw, "no") || ieq_(raw, "off")) return false;
  return fallback;
}

Context& ctx_() {
  static Context ctx;
  static std::once_flag once;
  std::call_once(once, [&]() {
    @autoreleasepool {
      ctx.device = ::pluss::codem::native::mps::shared_device_mm();
      if (ctx.device == nil) {
        ctx.error = "MPS Conv3d init failed: ";
        ctx.error += ::pluss::codem::native::mps::runtime_error_cstr();
        return;
      }
      ctx.queue = ::pluss::codem::native::mps::shared_queue_mm();
      if (ctx.queue == nil) {
        ctx.error = "MPS Conv3d init failed: no shared Metal command queue";
        return;
      }
      if (![MPSGraph instancesRespondToSelector:@selector(convolution3DWithSourceTensor:weightsTensor:descriptor:name:)]) {
        ctx.error = "MPSGraph Conv3d operator is unavailable on this OS";
        return;
      }
      ctx.ready = true;
    }
  });
  return ctx;
}

void set_reason_(std::string* reason, std::string msg) noexcept {
  if (reason) *reason = std::move(msg);
}

std::string shape_string_(const Series& s) {
  std::string out = "[";
  for (std::size_t i = 0; i < s.sizes().size(); ++i) {
    if (i) out += ",";
    out += std::to_string(s.sizes()[i]);
  }
  out += "]";
  return out;
}

bool is_zero_offset_contiguous_f32_mps_(const Series& s) noexcept {
  return s.defined() &&
      s.platform() == Platform::MPS &&
      s.scalar_type() == ScalarType::Float32 &&
      s.is_contiguous() &&
      s.storage_offset() == 0;
}

bool geometry_valid_(const Conv3dGeometry& g, std::string* reason) noexcept {
  for (int i = 0; i < 3; ++i) {
    if (g.stride[static_cast<std::size_t>(i)] <= 0) {
      set_reason_(reason, "native MPS Conv3d route requires positive stride");
      return false;
    }
    if (g.padding[static_cast<std::size_t>(i)] < 0) {
      set_reason_(reason, "native MPS Conv3d route requires non-negative padding");
      return false;
    }
    if (g.dilation[static_cast<std::size_t>(i)] <= 0) {
      set_reason_(reason, "native MPS Conv3d route requires positive dilation");
      return false;
    }
  }
  if (g.groups <= 0) {
    set_reason_(reason, "native MPS Conv3d route requires positive groups");
    return false;
  }
  return true;
}

bool validate_conv3d_(const Series& input,
                      const Series& weight,
                      const Series& bias,
                      const Conv3dGeometry& g,
                      bool require_backward,
                      std::string* reason) noexcept {
  if (!conv3d_route_enabled()) {
    set_reason_(reason, "native MPS Conv3d route is disabled");
    return false;
  }
  if (!ctx_().ready) {
    set_reason_(reason, ctx_().error);
    return false;
  }
  (void)require_backward;
  if (!geometry_valid_(g, reason)) return false;
  if (!is_zero_offset_contiguous_f32_mps_(input) || !is_zero_offset_contiguous_f32_mps_(weight)) {
    set_reason_(reason, "native MPS Conv3d route expects zero-offset contiguous float32 MPS input and weight");
    return false;
  }
  if (input.dim() != 5 || weight.dim() != 5) {
    set_reason_(reason, "native MPS Conv3d route expects input [N,C,D,H,W] and weight [O,I,D,H,W]");
    return false;
  }
  if (bias.defined()) {
    if (!is_zero_offset_contiguous_f32_mps_(bias) || bias.dim() != 1) {
      set_reason_(reason, "native MPS Conv3d route expects zero-offset contiguous float32 MPS bias [O]");
      return false;
    }
  }
  const std::int64_t in_channels = input.sizes()[1];
  const std::int64_t out_channels = weight.sizes()[0];
  const std::int64_t channels_per_group = weight.sizes()[1];
  if (in_channels % g.groups != 0 || out_channels % g.groups != 0) {
    set_reason_(reason, "native MPS Conv3d route channel count must be divisible by groups");
    return false;
  }
  if (channels_per_group * g.groups != in_channels) {
    set_reason_(reason, "native MPS Conv3d route weight input channel mismatch");
    return false;
  }
  if (bias.defined() && bias.sizes()[0] != out_channels) {
    set_reason_(reason, "native MPS Conv3d route bias shape mismatch");
    return false;
  }
  for (int i = 0; i < 3; ++i) {
    const std::int64_t input_extent = input.sizes()[static_cast<std::size_t>(i + 2)];
    const std::int64_t kernel_extent = weight.sizes()[static_cast<std::size_t>(i + 2)];
    const std::int64_t padded = input_extent + 2 * g.padding[static_cast<std::size_t>(i)];
    const std::int64_t effective = g.dilation[static_cast<std::size_t>(i)] * (kernel_extent - 1) + 1;
    if (kernel_extent <= 0 || padded < effective) {
      set_reason_(reason, "native MPS Conv3d route kernel larger than padded input");
      return false;
    }
  }
  return true;
}

NSUInteger byte_count_(const Series& s) {
  return static_cast<NSUInteger>(s.numel() * static_cast<std::int64_t>(sizeof(float)));
}

MPSShape* mps_shape_(pluss::msharp::RangeView::Int sizes) {
  NSMutableArray<NSNumber*>* shape = [NSMutableArray arrayWithCapacity:sizes.size()];
  for (const auto d : sizes) {
    [shape addObject:@(static_cast<NSInteger>(d))];
  }
  return shape;
}

std::vector<std::int64_t> sizes_vector_(const Series& s) {
  std::vector<std::int64_t> out;
  out.reserve(s.sizes().size());
  for (const auto v : s.sizes()) out.push_back(v);
  return out;
}

MPSGraphTensorData* tensor_data_for_read_(const Series& s, MPSShape* shape, id<MTLDevice> device, const char* op) {
  id<MTLBuffer> buf = ::pluss::codem::native::mps::ensure_device_buffer_for_read_mm(s, device, byte_count_(s));
  if (buf == nil) {
    ::pluss::throw_error(std::string(op) + ": failed to acquire input Metal buffer");
  }
  return [[MPSGraphTensorData alloc] initWithMTLBuffer:buf shape:shape dataType:MPSDataTypeFloat32];
}

MPSGraphTensorData* tensor_data_for_write_(const Series& s, MPSShape* shape, id<MTLDevice> device, const char* op) {
  id<MTLBuffer> buf = ::pluss::codem::native::mps::ensure_device_buffer_for_write_mm(s, device, byte_count_(s));
  if (buf == nil) {
    ::pluss::throw_error(std::string(op) + ": failed to acquire output Metal buffer");
  }
  return [[MPSGraphTensorData alloc] initWithMTLBuffer:buf shape:shape dataType:MPSDataTypeFloat32];
}

void fill_conv3d_desc_(MPSGraphConvolution3DOpDescriptor* desc, const Conv3dGeometry& g) {
  desc.strideInX = static_cast<NSUInteger>(g.stride[2]);
  desc.strideInY = static_cast<NSUInteger>(g.stride[1]);
  desc.strideInZ = static_cast<NSUInteger>(g.stride[0]);
  desc.dilationRateInX = static_cast<NSUInteger>(g.dilation[2]);
  desc.dilationRateInY = static_cast<NSUInteger>(g.dilation[1]);
  desc.dilationRateInZ = static_cast<NSUInteger>(g.dilation[0]);
  desc.paddingStyle = MPSGraphPaddingStyleExplicit;
  desc.paddingLeft = static_cast<NSUInteger>(g.padding[2]);
  desc.paddingRight = static_cast<NSUInteger>(g.padding[2]);
  desc.paddingTop = static_cast<NSUInteger>(g.padding[1]);
  desc.paddingBottom = static_cast<NSUInteger>(g.padding[1]);
  desc.paddingFront = static_cast<NSUInteger>(g.padding[0]);
  desc.paddingBack = static_cast<NSUInteger>(g.padding[0]);
  desc.dataLayout = static_cast<MPSGraphTensorNamedDataLayout>(7L);
  desc.weightsLayout = static_cast<MPSGraphTensorNamedDataLayout>(9L);
  desc.groups = static_cast<NSUInteger>(g.groups);
}

std::vector<std::int64_t> conv3d_output_sizes_(const Series& input, const Series& weight, const Conv3dGeometry& g) {
  std::vector<std::int64_t> out{input.sizes()[0], weight.sizes()[0], 0, 0, 0};
  for (int i = 0; i < 3; ++i) {
    const std::int64_t input_extent = input.sizes()[static_cast<std::size_t>(i + 2)];
    const std::int64_t kernel_extent = weight.sizes()[static_cast<std::size_t>(i + 2)];
    const std::int64_t effective = g.dilation[static_cast<std::size_t>(i)] * (kernel_extent - 1) + 1;
    out[static_cast<std::size_t>(i + 2)] =
        (input_extent + 2 * g.padding[static_cast<std::size_t>(i)] - effective) / g.stride[static_cast<std::size_t>(i)] + 1;
  }
  return out;
}

Series run_conv3d_backward_input_(const std::vector<std::int64_t>& input_sizes,
                                  const Series& grad_output,
                                  const Series& weight,
                                  const Conv3dGeometry& g) {
  auto& c = ctx_();
  Series grad_input = Series::empty(input_sizes, ScalarType::Float32, Platform::MPS).contiguous();
  @autoreleasepool {
    MPSGraph* graph = [[MPSGraph alloc] init];
    MPSShape* grad_shape = mps_shape_(grad_output.sizes());
    MPSShape* weight_shape = mps_shape_(weight.sizes());
    MPSShape* input_shape = mps_shape_(input_sizes);
    MPSGraphTensor* grad_t = [graph placeholderWithShape:grad_shape dataType:MPSDataTypeFloat32 name:@"grad_output"];
    MPSGraphTensor* weight_t = [graph placeholderWithShape:weight_shape dataType:MPSDataTypeFloat32 name:@"weight"];
    MPSGraphConvolution3DOpDescriptor* desc = [MPSGraphConvolution3DOpDescriptor new];
    fill_conv3d_desc_(desc, g);
    MPSGraphTensor* grad_input_t =
        [graph convolution3DDataGradientWithIncomingGradientTensor:grad_t
                                                     weightsTensor:weight_t
                                                       outputShape:input_shape
                                      forwardConvolutionDescriptor:desc
                                                              name:@"pluss_conv3d_grad_input"];
    NSDictionary<MPSGraphTensor*, MPSGraphTensorData*>* feeds = @{
      grad_t : tensor_data_for_read_(grad_output, grad_shape, c.device, "mps::conv3d_backward_input"),
      weight_t : tensor_data_for_read_(weight, weight_shape, c.device, "mps::conv3d_backward_input"),
    };
    NSDictionary<MPSGraphTensor*, MPSGraphTensorData*>* results = @{
      grad_input_t : tensor_data_for_write_(grad_input, input_shape, c.device, "mps::conv3d_backward_input"),
    };
    [graph runWithMTLCommandQueue:c.queue feeds:feeds targetOperations:nil resultsDictionary:results];
  }
  ::pluss::codem::native::mps::mark_device_data_dirty(grad_input, nullptr);
  return grad_input;
}

Series run_conv3d_backward_weight_(const std::vector<std::int64_t>& weight_sizes,
                                   const Series& grad_output,
                                   const Series& input,
                                   const Conv3dGeometry& g) {
  auto& c = ctx_();
  Series grad_weight = Series::empty(weight_sizes, ScalarType::Float32, Platform::MPS).contiguous();
  @autoreleasepool {
    MPSGraph* graph = [[MPSGraph alloc] init];
    MPSShape* grad_shape = mps_shape_(grad_output.sizes());
    MPSShape* input_shape = mps_shape_(input.sizes());
    MPSShape* weight_shape = mps_shape_(weight_sizes);
    MPSGraphTensor* grad_t = [graph placeholderWithShape:grad_shape dataType:MPSDataTypeFloat32 name:@"grad_output"];
    MPSGraphTensor* input_t = [graph placeholderWithShape:input_shape dataType:MPSDataTypeFloat32 name:@"input"];
    MPSGraphConvolution3DOpDescriptor* desc = [MPSGraphConvolution3DOpDescriptor new];
    fill_conv3d_desc_(desc, g);
    MPSGraphTensor* grad_weight_t =
        [graph convolution3DWeightsGradientWithIncomingGradientTensor:grad_t
                                                         sourceTensor:input_t
                                                          outputShape:weight_shape
                                         forwardConvolutionDescriptor:desc
                                                                 name:@"pluss_conv3d_grad_weight"];
    NSDictionary<MPSGraphTensor*, MPSGraphTensorData*>* feeds = @{
      grad_t : tensor_data_for_read_(grad_output, grad_shape, c.device, "mps::conv3d_backward_weight"),
      input_t : tensor_data_for_read_(input, input_shape, c.device, "mps::conv3d_backward_weight"),
    };
    NSDictionary<MPSGraphTensor*, MPSGraphTensorData*>* results = @{
      grad_weight_t : tensor_data_for_write_(grad_weight, weight_shape, c.device, "mps::conv3d_backward_weight"),
    };
    [graph runWithMTLCommandQueue:c.queue feeds:feeds targetOperations:nil resultsDictionary:results];
  }
  ::pluss::codem::native::mps::mark_device_data_dirty(grad_weight, nullptr);
  return grad_weight;
}

Series run_conv3d_(const Series& input, const Series& weight, const Series& bias, const Conv3dGeometry& g) {
  auto& c = ctx_();
  const std::vector<std::int64_t> out_sizes = conv3d_output_sizes_(input, weight, g);
  Series output = Series::empty(out_sizes, ScalarType::Float32, Platform::MPS).contiguous();
  @autoreleasepool {
    MPSGraph* graph = [[MPSGraph alloc] init];
    MPSShape* input_shape = mps_shape_(input.sizes());
    MPSShape* weight_shape = mps_shape_(weight.sizes());
    MPSShape* output_shape = mps_shape_(out_sizes);

    MPSGraphTensor* input_t = [graph placeholderWithShape:input_shape dataType:MPSDataTypeFloat32 name:@"input"];
    MPSGraphTensor* weight_t = [graph placeholderWithShape:weight_shape dataType:MPSDataTypeFloat32 name:@"weight"];
    MPSGraphConvolution3DOpDescriptor* desc = [MPSGraphConvolution3DOpDescriptor new];
    fill_conv3d_desc_(desc, g);
    MPSGraphTensor* output_t = [graph convolution3DWithSourceTensor:input_t
                                                      weightsTensor:weight_t
                                                         descriptor:desc
                                                               name:@"pluss_conv3d"];

    MPSGraphTensor* bias_t = nil;
    Series bias_shaped = Series::undefined();
    MPSShape* bias_shape = nil;
    if (bias.defined()) {
      bias_shaped = bias.reshape({1, bias.sizes()[0], 1, 1, 1}).contiguous();
      bias_shape = mps_shape_(bias_shaped.sizes());
      bias_t = [graph placeholderWithShape:bias_shape dataType:MPSDataTypeFloat32 name:@"bias"];
      output_t = [graph additionWithPrimaryTensor:output_t secondaryTensor:bias_t name:@"pluss_conv3d_bias"];
    }

    NSMutableDictionary<MPSGraphTensor*, MPSGraphTensorData*>* feeds = [@{
      input_t : tensor_data_for_read_(input, input_shape, c.device, "mps::conv3d"),
      weight_t : tensor_data_for_read_(weight, weight_shape, c.device, "mps::conv3d"),
    } mutableCopy];
    if (bias.defined()) {
      [feeds setObject:tensor_data_for_read_(bias_shaped, bias_shape, c.device, "mps::conv3d") forKey:bias_t];
    }
    NSDictionary<MPSGraphTensor*, MPSGraphTensorData*>* results = @{
      output_t : tensor_data_for_write_(output, output_shape, c.device, "mps::conv3d"),
    };
    [graph runWithMTLCommandQueue:c.queue feeds:feeds targetOperations:nil resultsDictionary:results];
  }
  ::pluss::codem::native::mps::mark_device_data_dirty(output, nullptr);
  conv3d_calls_().fetch_add(1, std::memory_order_relaxed);

  const bool should_link =
      ::pluss::codem::series_tape::recording_enabled() &&
      (::pluss::codem::series_tape::tracks_slope(input) ||
       ::pluss::codem::series_tape::tracks_slope(weight) ||
       (bias.defined() && ::pluss::codem::series_tape::tracks_slope(bias)));
  if (should_link) {
    auto link = std::make_shared<::pluss::codem::series_tape::SourceLink>();
    link->op_name = "mps_conv3d";
    link->parents = {input, weight};
    if (bias.defined()) link->parents.push_back(bias);
    link->propagate = [input, weight, bias, g](const Series& sens) {
      const Series grad_output = sens.contiguous();
      if (::pluss::codem::series_tape::tracks_slope(input)) {
        ::pluss::codem::series_tape::accumulate_sensitivity(
            input,
            run_conv3d_backward_input_(sizes_vector_(input), grad_output, weight, g));
      }
      if (::pluss::codem::series_tape::tracks_slope(weight)) {
        ::pluss::codem::series_tape::accumulate_sensitivity(
            weight,
            run_conv3d_backward_weight_(sizes_vector_(weight), grad_output, input, g));
      }
      if (bias.defined() && ::pluss::codem::series_tape::tracks_slope(bias)) {
        Series grad_bias = grad_output.sum(0).sum(1).sum(1).sum(1).contiguous();
        ::pluss::codem::series_tape::accumulate_sensitivity(bias, grad_bias);
      }
    };
    ::pluss::codem::series_tape::bind_source_link(output, std::move(link));
  }
  return output;
}

} // namespace

bool conv3d_route_enabled() noexcept {
  return parse_bool_env_("PLUSS_MPS_CONV3D_ROUTE", true);
}

bool conv3d_route_strict() noexcept {
  return parse_bool_env_("PLUSS_MPS_CONV3D_ROUTE_STRICT", false);
}

bool conv3d_route_ready() noexcept {
  return ctx_().ready;
}

std::uint64_t conv3d_native_call_count() noexcept {
  return conv3d_calls_().load(std::memory_order_relaxed);
}

bool supports_conv3d(const Series& input,
                     const Series& weight,
                     const Series& bias,
                     std::array<std::int64_t, 3> stride,
                     std::array<std::int64_t, 3> padding,
                     std::array<std::int64_t, 3> dilation,
                     std::int64_t groups,
                     bool require_backward,
                     std::string* reason) noexcept {
  return validate_conv3d_(input, weight, bias, Conv3dGeometry{stride, padding, dilation, groups}, require_backward, reason);
}

std::optional<Series> try_conv3d(const Series& input,
                                 const Series& weight,
                                 const Series& bias,
                                 std::array<std::int64_t, 3> stride,
                                 std::array<std::int64_t, 3> padding,
                                 std::array<std::int64_t, 3> dilation,
                                 std::int64_t groups,
                                 bool require_backward) {
  const Conv3dGeometry g{stride, padding, dilation, groups};
  std::string reason;
  if (!validate_conv3d_(input, weight, bias, g, require_backward, &reason)) {
    if (conv3d_route_strict() && input.defined() && input.platform() == Platform::MPS) {
      ::pluss::throw_error("mps::conv3d: native route unavailable: " + reason);
    }
    return std::nullopt;
  }
  return run_conv3d_(input, weight, bias, g);
}

} // namespace pluss::ai::kernels::mps

#endif
