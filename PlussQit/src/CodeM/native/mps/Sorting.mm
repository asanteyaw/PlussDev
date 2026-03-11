#include "CodeM/native/mps/Sorting.h"

#include "CodeM/core/Errors.h"
#include "CodeM/native/mps/BufferCache.h"
#include "CodeM/native/mps/BufferCacheMM.h"
#include "CodeM/native/mps/MPSRuntimeMM.h"

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShadersGraph/MetalPerformanceShadersGraph.h>
#import <MetalPerformanceShadersGraph/MPSGraphSortOps.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace pluss::codem::native::mps {
namespace {

[[noreturn]] void fail_(const std::string& msg) {
  throw_error(msg);
}

bool sort_runtime_available_() noexcept {
  if (!runtime_available()) {
    return false;
  }
  if (@available(macOS 13.0, *)) {
    return [MPSGraph instancesRespondToSelector:@selector(sortWithTensor:axis:descending:name:)] &&
        [MPSGraph instancesRespondToSelector:@selector(argSortWithTensor:axis:descending:name:)];
  }
  return false;
}

bool mps_dtype_for_(ScalarType st, MPSDataType& out) noexcept {
  switch (st) {
    case ScalarType::Float16:
      out = MPSDataTypeFloat16;
      return true;
    case ScalarType::Float32:
      out = MPSDataTypeFloat32;
      return true;
    case ScalarType::Int32:
      out = MPSDataTypeInt32;
      return true;
    case ScalarType::Int64:
      out = MPSDataTypeInt64;
      return true;
    default:
      return false;
  }
}

bool is_float_dtype_(ScalarType st) noexcept {
  switch (st) {
    case ScalarType::Float16:
    case ScalarType::BFloat16:
    case ScalarType::Float32:
    case ScalarType::Float64:
    case ScalarType::Float8E4M3FN:
    case ScalarType::Float8E5M2:
      return true;
    default:
      return false;
  }
}

ScalarType selection_compute_dtype_(ScalarType st) noexcept {
  switch (st) {
    case ScalarType::Float16:
    case ScalarType::Float32:
    case ScalarType::Int32:
      return st;
    case ScalarType::Complex64:
    case ScalarType::Complex128:
      return ScalarType::Complex64;
    default:
      return is_float_dtype_(st) ? ScalarType::Float32 : ScalarType::Int32;
  }
}

MPSShape* mps_shape_for_(std::span<const std::int64_t> shape) {
  if (shape.empty()) {
    return @[ @1 ];
  }
  NSMutableArray<NSNumber*>* arr = [NSMutableArray arrayWithCapacity:shape.size()];
  for (const auto v : shape) {
    [arr addObject:@(static_cast<NSInteger>(v))];
  }
  return arr;
}

std::size_t nbytes_(const Series& x) {
  return static_cast<std::size_t>(std::max<std::int64_t>(x.numel(), 0)) * static_cast<std::size_t>(x.itemsize());
}

std::pair<Series, Series> sort_or_topk_impl_(
    const Series& a,
    std::int64_t dim,
    bool descending,
    std::int64_t k_or_neg1) {
  if (!sort_runtime_available_()) {
    fail_(std::string("mps::sorting: MPSGraph sort/argsort is unavailable (") + runtime_error_cstr() + ")");
  }
  if (a.platform() != Platform::MPS) {
    fail_("mps::sorting: expected MPS input");
  }
  if (a.dim() <= 0) {
    fail_("mps::sorting: expected rank >= 1 input");
  }
  if (dtype_of(a.scalar_type()).is_complex()) {
    fail_("mps::sorting: complex dtype is unsupported");
  }

  const ScalarType compute_dtype = selection_compute_dtype_(a.scalar_type());
  if (compute_dtype == ScalarType::Complex64) {
    fail_("mps::sorting: unsupported compute dtype");
  }

  Series input = a.is_contiguous() ? a : a.contiguous();
  if (input.scalar_type() != compute_dtype) {
    input = input.to(compute_dtype).contiguous();
  }

  const auto nd = input.dim();
  if (dim < 0) {
    dim += nd;
  }
  if (dim < 0 || dim >= nd) {
    fail_("mps::sorting: dim out of range");
  }

  std::vector<std::int64_t> out_shape(input.sizes().begin(), input.sizes().end());
  if (k_or_neg1 >= 0) {
    const auto dsz = out_shape[static_cast<std::size_t>(dim)];
    if (k_or_neg1 < 0 || k_or_neg1 > dsz) {
      fail_("mps::sorting: k out of range");
    }
    out_shape[static_cast<std::size_t>(dim)] = k_or_neg1;
  }

  Series values = Series::zeros(out_shape, compute_dtype).to(Platform::MPS).contiguous();
  Series indices = Series::zeros(out_shape, ScalarType::Int64).to(Platform::MPS).contiguous();

  @autoreleasepool {
    id<MTLDevice> device = shared_device_mm();
    id<MTLCommandQueue> queue = shared_queue_mm();
    if (device == nil || queue == nil) {
      fail_("mps::sorting: missing shared Metal runtime");
    }

    MPSDataType compute_mps_dtype = MPSDataTypeInvalid;
    if (!mps_dtype_for_(compute_dtype, compute_mps_dtype)) {
      fail_("mps::sorting: unsupported compute dtype for graph feed");
    }

    MPSShape* input_shape = mps_shape_for_(input.sizes());
    MPSShape* output_shape = mps_shape_for_(out_shape);

    MPSGraph* graph = [[MPSGraph alloc] init];
    MPSGraphTensor* input_tensor = [graph placeholderWithShape:input_shape dataType:compute_mps_dtype name:nil];
    MPSGraphTensor* sorted_tensor =
        [graph sortWithTensor:input_tensor axis:static_cast<NSInteger>(dim) descending:descending name:@"sort_out"];
    MPSGraphTensor* argsort_tensor =
        [graph argSortWithTensor:input_tensor axis:static_cast<NSInteger>(dim) descending:descending name:@"argsort_out"];

    if (k_or_neg1 >= 0) {
      sorted_tensor = [graph sliceTensor:sorted_tensor
                               dimension:static_cast<NSUInteger>(dim)
                                   start:0
                                  length:static_cast<NSUInteger>(k_or_neg1)
                                    name:@"topk_values"];
      argsort_tensor = [graph sliceTensor:argsort_tensor
                                dimension:static_cast<NSUInteger>(dim)
                                    start:0
                                   length:static_cast<NSUInteger>(k_or_neg1)
                                     name:@"topk_indices"];
    }

    MPSGraphTensor* argsort_i64 = [graph castTensor:argsort_tensor toType:MPSDataTypeInt64 name:@"argsort_i64"];

    id<MTLBuffer> in_buf = ensure_device_buffer_for_read_mm(input, device, nbytes_(input));
    id<MTLBuffer> values_buf = ensure_device_buffer_for_write_mm(values, device, nbytes_(values));
    id<MTLBuffer> indices_buf = ensure_device_buffer_for_write_mm(indices, device, nbytes_(indices));
    if (in_buf == nil || values_buf == nil || indices_buf == nil) {
      fail_("mps::sorting: failed to acquire device buffers");
    }

    MPSGraphTensorData* in_data =
        [[MPSGraphTensorData alloc] initWithMTLBuffer:in_buf shape:input_shape dataType:compute_mps_dtype];
    MPSGraphTensorData* values_data =
        [[MPSGraphTensorData alloc] initWithMTLBuffer:values_buf shape:output_shape dataType:compute_mps_dtype];
    MPSGraphTensorData* indices_data =
        [[MPSGraphTensorData alloc] initWithMTLBuffer:indices_buf shape:output_shape dataType:MPSDataTypeInt64];
    if (in_data == nil || values_data == nil || indices_data == nil) {
      fail_("mps::sorting: failed to create graph tensor data bindings");
    }

    NSDictionary* feeds = @{input_tensor : in_data};
    NSDictionary* results = @{sorted_tensor : values_data, argsort_i64 : indices_data};
    [graph runWithMTLCommandQueue:queue feeds:feeds targetOperations:nil resultsDictionary:results];
  }

  mark_device_data_dirty(values, nullptr);
  mark_device_data_dirty(indices, nullptr);

  if (values.scalar_type() != a.scalar_type()) {
    values = values.to(a.scalar_type()).contiguous();
  }
  return {values, indices};
}

} // namespace

bool supports_sort_selection(const Series& a, std::int64_t dim, bool stable) noexcept {
  if (stable) {
    return false;
  }
  if (!sort_runtime_available_()) {
    return false;
  }
  if (!a.defined() || a.platform() != Platform::MPS || a.dim() <= 0 || a.numel() == 0) {
    return false;
  }
  if (dtype_of(a.scalar_type()).is_complex()) {
    return false;
  }
  const auto nd = a.dim();
  if (dim < -nd || dim >= nd) {
    return false;
  }
  const auto normalized_dim = (dim < 0) ? (dim + nd) : dim;
  if (normalized_dim != (nd - 1)) {
    // Keep native path on the fastest/most-stable lane first.
    return false;
  }
  return true;
}

std::pair<Series, Series> sort_with_indices(const Series& a, std::int64_t dim, bool descending) {
  return sort_or_topk_impl_(a, dim, descending, /*k_or_neg1=*/-1);
}

std::pair<Series, Series> topk_with_indices(const Series& a, std::int64_t k, std::int64_t dim, bool largest) {
  return sort_or_topk_impl_(a, dim, largest, k);
}

Series argsort(const Series& a, std::int64_t dim, bool descending) {
  return sort_or_topk_impl_(a, dim, descending, /*k_or_neg1=*/-1).second;
}

Series kthvalue(const Series& a, std::int64_t k, std::int64_t dim, bool keepdim) {
  if (!sort_runtime_available_()) {
    fail_(std::string("mps::sorting::kthvalue: MPSGraph sort/argsort is unavailable (") + runtime_error_cstr() + ")");
  }
  if (a.platform() != Platform::MPS) {
    fail_("mps::sorting::kthvalue: expected MPS input");
  }
  if (a.dim() <= 0) {
    fail_("mps::sorting::kthvalue: expected rank >= 1 input");
  }
  if (dtype_of(a.scalar_type()).is_complex()) {
    fail_("mps::sorting::kthvalue: complex dtype is unsupported");
  }

  const auto nd = a.dim();
  if (dim < 0) {
    dim += nd;
  }
  if (dim < 0 || dim >= nd) {
    fail_("mps::sorting::kthvalue: dim out of range");
  }
  const auto dim_size = a.sizes()[static_cast<std::size_t>(dim)];
  if (k < 1 || k > dim_size) {
    fail_("mps::sorting::kthvalue: k out of range");
  }

  const ScalarType compute_dtype = selection_compute_dtype_(a.scalar_type());
  if (compute_dtype == ScalarType::Complex64) {
    fail_("mps::sorting::kthvalue: unsupported compute dtype");
  }

  Series input = a.is_contiguous() ? a : a.contiguous();
  if (input.scalar_type() != compute_dtype) {
    input = input.to(compute_dtype).contiguous();
  }

  std::vector<std::int64_t> out_keepdim(input.sizes().begin(), input.sizes().end());
  out_keepdim[static_cast<std::size_t>(dim)] = 1;
  Series out_keep = Series::zeros(out_keepdim, compute_dtype).to(Platform::MPS).contiguous();

  @autoreleasepool {
    id<MTLDevice> device = shared_device_mm();
    id<MTLCommandQueue> queue = shared_queue_mm();
    if (device == nil || queue == nil) {
      fail_("mps::sorting::kthvalue: missing shared Metal runtime");
    }

    MPSDataType compute_mps_dtype = MPSDataTypeInvalid;
    if (!mps_dtype_for_(compute_dtype, compute_mps_dtype)) {
      fail_("mps::sorting::kthvalue: unsupported compute dtype for graph feed");
    }

    MPSShape* input_shape = mps_shape_for_(input.sizes());
    MPSShape* output_shape = mps_shape_for_(out_keepdim);

    MPSGraph* graph = [[MPSGraph alloc] init];
    MPSGraphTensor* input_tensor = [graph placeholderWithShape:input_shape dataType:compute_mps_dtype name:nil];
    MPSGraphTensor* sorted_tensor =
        [graph sortWithTensor:input_tensor axis:static_cast<NSInteger>(dim) descending:NO name:@"sort_out"];
    MPSGraphTensor* kth_tensor = [graph sliceTensor:sorted_tensor
                                          dimension:static_cast<NSUInteger>(dim)
                                              start:static_cast<NSUInteger>(k - 1)
                                             length:1
                                               name:@"kth_value"];

    id<MTLBuffer> in_buf = ensure_device_buffer_for_read_mm(input, device, nbytes_(input));
    id<MTLBuffer> out_buf = ensure_device_buffer_for_write_mm(out_keep, device, nbytes_(out_keep));
    if (in_buf == nil || out_buf == nil) {
      fail_("mps::sorting::kthvalue: failed to acquire device buffers");
    }

    MPSGraphTensorData* in_data =
        [[MPSGraphTensorData alloc] initWithMTLBuffer:in_buf shape:input_shape dataType:compute_mps_dtype];
    MPSGraphTensorData* out_data =
        [[MPSGraphTensorData alloc] initWithMTLBuffer:out_buf shape:output_shape dataType:compute_mps_dtype];
    if (in_data == nil || out_data == nil) {
      fail_("mps::sorting::kthvalue: failed to create graph tensor data bindings");
    }

    NSDictionary* feeds = @{input_tensor : in_data};
    NSDictionary* results = @{kth_tensor : out_data};
    [graph runWithMTLCommandQueue:queue feeds:feeds targetOperations:nil resultsDictionary:results];
  }

  mark_device_data_dirty(out_keep, nullptr);

  Series out = out_keep;
  if (!keepdim) {
    std::vector<std::int64_t> out_shape;
    out_shape.reserve(static_cast<std::size_t>(nd - 1));
    for (std::int64_t i = 0; i < nd; ++i) {
      if (i == dim) continue;
      out_shape.push_back(a.sizes()[static_cast<std::size_t>(i)]);
    }
    out = out.reshape(out_shape).contiguous();
  }

  if (out.scalar_type() != a.scalar_type()) {
    out = out.to(a.scalar_type()).contiguous();
  }
  return out;
}

} // namespace pluss::codem::native::mps
