#include "CodeM/native/mps/Matmul.h"
#include "CodeM/native/mps/BufferCacheMM.h"

#include "CodeM/core/Errors.h"

#include <Metal/Metal.h>
#include <MetalPerformanceShaders/MetalPerformanceShaders.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>

namespace pluss::codem::native::mps {
namespace {

struct Context {
  bool ready{false};
  id<MTLDevice> device{nil};
  id<MTLCommandQueue> queue{nil};
  std::string error{};
};

Context& ctx_() {
  static Context ctx;
  static std::once_flag once;
  std::call_once(once, [&]() {
    @autoreleasepool {
      ctx.device = shared_device_mm();
      if (ctx.device == nil) {
        ctx.error = "MPS matmul init failed: ";
        ctx.error += runtime_error_cstr();
        return;
      }
      ctx.queue = shared_queue_mm();
      if (ctx.queue == nil) {
        ctx.error = "MPS matmul init failed: no shared Metal command queue";
        return;
      }
      ctx.ready = true;
    }
  });
  return ctx;
}

[[noreturn]] void fail_(const std::string& msg) {
  throw_error(msg);
}

void check_series_(const Series& s, const char* op) {
  if (!s.defined()) fail_(std::string(op) + ": undefined Series");
  if (s.platform() != Platform::MPS) fail_(std::string(op) + ": expected MPS Series");
  if (!s.is_contiguous()) fail_(std::string(op) + ": currently requires contiguous Series");
  if (s.scalar_type() != ScalarType::Float32) fail_(std::string(op) + ": currently supports float32 only");
}

} // namespace

bool supports_matmul(const Series& a, const Series& b) noexcept {
  if (!ctx_().ready) return false;
  if (!a.defined() || !b.defined()) return false;
  if (a.platform() != Platform::MPS || b.platform() != Platform::MPS) return false;
  if (!a.is_contiguous() || !b.is_contiguous()) return false;
  if (a.scalar_type() != ScalarType::Float32 || b.scalar_type() != ScalarType::Float32) return false;
  if (a.dim() != 2 || b.dim() != 2) return false;
  return a.sizes()[1] == b.sizes()[0];
}

Series matmul(const Series& a, const Series& b) {
  auto& c = ctx_();
  if (!c.ready) {
    fail_("mps::matmul: backend unavailable (" + c.error + ")");
  }
  check_series_(a, "mps::matmul");
  check_series_(b, "mps::matmul");
  if (a.dim() != 2 || b.dim() != 2) {
    fail_("mps::matmul: currently supports 2D only");
  }

  const auto m64 = a.sizes()[0];
  const auto k64 = a.sizes()[1];
  const auto k2 = b.sizes()[0];
  const auto n64 = b.sizes()[1];
  if (k64 != k2) {
    fail_("mps::matmul: shape mismatch");
  }
  if (m64 < 0 || n64 < 0 || k64 < 0) {
    fail_("mps::matmul: invalid sizes");
  }
  if (m64 == 0 || n64 == 0 || k64 == 0) {
    return Series::empty({m64, n64}, ScalarType::Float32, Platform::MPS);
  }
  const NSUInteger m = static_cast<NSUInteger>(m64);
  const NSUInteger n = static_cast<NSUInteger>(n64);
  const NSUInteger k = static_cast<NSUInteger>(k64);

  Series out = Series::empty({m64, n64}, ScalarType::Float32, Platform::MPS);
  @autoreleasepool {
    const std::size_t a_bytes = static_cast<std::size_t>(a.numel()) * sizeof(float);
    const std::size_t b_bytes = static_cast<std::size_t>(b.numel()) * sizeof(float);
    const std::size_t out_bytes = static_cast<std::size_t>(out.numel()) * sizeof(float);

    id<MTLBuffer> a_buf = ensure_device_buffer_for_read_mm(a, c.device, a_bytes);
    id<MTLBuffer> b_buf = ensure_device_buffer_for_read_mm(b, c.device, b_bytes);
    id<MTLBuffer> out_buf = ensure_device_buffer_for_write_mm(out, c.device, out_bytes);
    if (a_buf == nil || b_buf == nil || out_buf == nil) {
      fail_("mps::matmul: failed to allocate Metal buffers");
    }

    MPSMatrixDescriptor* a_desc =
        [MPSMatrixDescriptor matrixDescriptorWithRows:m
                                              columns:k
                                             rowBytes:k * sizeof(float)
                                             dataType:MPSDataTypeFloat32];
    MPSMatrixDescriptor* b_desc =
        [MPSMatrixDescriptor matrixDescriptorWithRows:k
                                              columns:n
                                             rowBytes:n * sizeof(float)
                                             dataType:MPSDataTypeFloat32];
    MPSMatrixDescriptor* out_desc =
        [MPSMatrixDescriptor matrixDescriptorWithRows:m
                                              columns:n
                                             rowBytes:n * sizeof(float)
                                             dataType:MPSDataTypeFloat32];

    MPSMatrix* a_mat = [[MPSMatrix alloc] initWithBuffer:a_buf descriptor:a_desc];
    MPSMatrix* b_mat = [[MPSMatrix alloc] initWithBuffer:b_buf descriptor:b_desc];
    MPSMatrix* out_mat = [[MPSMatrix alloc] initWithBuffer:out_buf descriptor:out_desc];

    MPSMatrixMultiplication* mm = [[MPSMatrixMultiplication alloc] initWithDevice:c.device
                                                                     transposeLeft:false
                                                                    transposeRight:false
                                                                       resultRows:m
                                                                    resultColumns:n
                                                                   interiorColumns:k
                                                                              alpha:1.0
                                                                               beta:0.0];

    id<MTLCommandBuffer> cb = [c.queue commandBuffer];
    if (cb == nil) {
      fail_("mps::matmul: failed to create command buffer");
    }
    [mm encodeToCommandBuffer:cb leftMatrix:a_mat rightMatrix:b_mat resultMatrix:out_mat];
    [cb commit];
    mark_device_data_dirty_mm(out, cb);
  }
  return out;
}

} // namespace pluss::codem::native::mps
