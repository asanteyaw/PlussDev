#include "CodeM/native/mps/Stochastic.h"

#include "CodeM/core/Errors.h"
#include "CodeM/native/mps/BufferCache.h"
#include "CodeM/native/mps/BufferCacheMM.h"
#include "CodeM/native/mps/MPSRuntimeMM.h"

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShadersGraph/MetalPerformanceShadersGraph.h>
#import <MetalPerformanceShadersGraph/MPSGraphRandomOps.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace pluss::codem::native::mps {
namespace {

[[noreturn]] void fail_(const std::string& msg) {
  throw_error(msg);
}

bool stochastic_runtime_available_() noexcept {
  if (!runtime_available()) {
    return false;
  }
  if (@available(macOS 12.3, *)) {
    return [MPSGraph instancesRespondToSelector:@selector(randomTensorWithShape:descriptor:seed:name:)];
  }
  return false;
}

bool stochastic_dtype_supported_(ScalarType dtype) noexcept {
  return dtype == ScalarType::Float32;
}

MPSDataType mps_dtype_for_(ScalarType dtype) {
  switch (dtype) {
    case ScalarType::Float32:
      return MPSDataTypeFloat32;
    default:
      fail_("mps::stochastic: unsupported dtype");
  }
}

MPSShape* mps_shape_for_(std::span<const std::int64_t> shape) {
  NSMutableArray<NSNumber*>* arr = [NSMutableArray arrayWithCapacity:shape.size()];
  for (const auto v : shape) {
    [arr addObject:@(static_cast<NSInteger>(v))];
  }
  return arr;
}

std::size_t nbytes_(const Series& x) {
  return static_cast<std::size_t>(std::max<std::int64_t>(x.numel(), 0)) * static_cast<std::size_t>(x.itemsize());
}

Series random_stochastic_(pluss::msharp::RangeView::Int sizes,
                          ScalarType dtype,
                          MPSGraphRandomDistribution distribution,
                          double a,
                          double b,
                          std::uint64_t seed,
                          const char* op_name) {
  if (!stochastic_runtime_available_()) {
    fail_(std::string(op_name) + ": MPSGraph random is unavailable");
  }
  if (!stochastic_dtype_supported_(dtype)) {
    fail_(std::string(op_name) + ": currently supports float32 only");
  }
  Series out = Series::empty(sizes, dtype, Platform::MPS).contiguous();
  if (out.numel() == 0) {
    return out;
  }

  @autoreleasepool {
    id<MTLDevice> device = shared_device_mm();
    id<MTLCommandQueue> queue = shared_queue_mm();
    if (device == nil || queue == nil) {
      fail_(std::string(op_name) + ": missing shared Metal runtime");
    }

    const MPSDataType mps_dtype = mps_dtype_for_(dtype);
    MPSShape* shape = mps_shape_for_(out.sizes());

    MPSGraphRandomOpDescriptor* descriptor =
        [MPSGraphRandomOpDescriptor descriptorWithDistribution:distribution dataType:mps_dtype];
    if (descriptor == nil) {
      fail_(std::string(op_name) + ": failed to create MPSGraph random descriptor");
    }
    if (distribution == MPSGraphRandomDistributionUniform) {
      descriptor.min = static_cast<float>(a);
      descriptor.max = static_cast<float>(b);
    } else if (distribution == MPSGraphRandomDistributionNormal) {
      descriptor.mean = static_cast<float>(a);
      descriptor.standardDeviation = static_cast<float>(b);
    }

    MPSGraph* graph = [[MPSGraph alloc] init];
    MPSGraphTensor* result = [graph randomTensorWithShape:shape
                                               descriptor:descriptor
                                                     seed:static_cast<NSUInteger>(seed)
                                                     name:nil];
    if (result == nil) {
      fail_(std::string(op_name) + ": failed to create random tensor");
    }

    id<MTLBuffer> out_buf = ensure_device_buffer_for_write_mm(out, device, nbytes_(out));
    if (out_buf == nil) {
      fail_(std::string(op_name) + ": failed to acquire output device buffer");
    }

    MPSGraphTensorData* out_data = [[MPSGraphTensorData alloc] initWithMTLBuffer:out_buf
                                                                           shape:shape
                                                                        dataType:mps_dtype];
    if (out_data == nil) {
      fail_(std::string(op_name) + ": failed to bind output tensor data");
    }

    NSDictionary* results = @{result : out_data};
    [graph runWithMTLCommandQueue:queue feeds:@{} targetOperations:nil resultsDictionary:results];
  }

  mark_device_data_dirty(out, nullptr);
  return out;
}

} // namespace

bool supports_uniform_stochastic(ScalarType dtype) noexcept {
  return stochastic_runtime_available_() && stochastic_dtype_supported_(dtype);
}

bool supports_normal_stochastic(ScalarType dtype) noexcept {
  return stochastic_runtime_available_() && stochastic_dtype_supported_(dtype);
}

Series uniform_stochastic(pluss::msharp::RangeView::Int sizes,
                          ScalarType dtype,
                          double from,
                          double to,
                          std::uint64_t seed) {
  PLUSS_CHECK(to > from, "mps::uniform_stochastic: expected to > from");
  return random_stochastic_(sizes,
                            dtype,
                            MPSGraphRandomDistributionUniform,
                            from,
                            to,
                            seed,
                            "mps::uniform_stochastic");
}

Series normal_stochastic(pluss::msharp::RangeView::Int sizes,
                         ScalarType dtype,
                         double mean,
                         double std,
                         std::uint64_t seed) {
  PLUSS_CHECK(std >= 0.0, "mps::normal_stochastic: expected std >= 0");
  return random_stochastic_(sizes,
                            dtype,
                            MPSGraphRandomDistributionNormal,
                            mean,
                            std,
                            seed,
                            "mps::normal_stochastic");
}

} // namespace pluss::codem::native::mps
