#include "CodeM/native/mps/Cast.h"
#include "CodeM/native/mps/BufferCacheMM.h"

#include "CodeM/core/Errors.h"

#include <Foundation/Foundation.h>
#include <Metal/Metal.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>

namespace pluss::codem::native::mps {
namespace {

constexpr const char* kShaderSrc = R"metal(
#include <metal_stdlib>
using namespace metal;

kernel void cast_f32_to_u8(
    const device float* in [[buffer(0)]],
    device uchar* out [[buffer(1)]],
    uint gid [[thread_position_in_grid]]) {
  out[gid] = (in[gid] != 0.0f) ? uchar(1) : uchar(0);
}

kernel void cast_u8_to_f32(
    const device uchar* in [[buffer(0)]],
    device float* out [[buffer(1)]],
    uint gid [[thread_position_in_grid]]) {
  out[gid] = (in[gid] == uchar(0)) ? 0.0f : 1.0f;
}

kernel void cast_f32_to_i64(
    const device float* in [[buffer(0)]],
    device long* out [[buffer(1)]],
    uint gid [[thread_position_in_grid]]) {
  out[gid] = long(in[gid]);
}

kernel void cast_i64_to_f32(
    const device long* in [[buffer(0)]],
    device float* out [[buffer(1)]],
    uint gid [[thread_position_in_grid]]) {
  out[gid] = float(in[gid]);
}

kernel void cast_u8_to_i64(
    const device uchar* in [[buffer(0)]],
    device long* out [[buffer(1)]],
    uint gid [[thread_position_in_grid]]) {
  out[gid] = long(in[gid] == uchar(0) ? 0 : 1);
}

kernel void cast_i64_to_u8(
    const device long* in [[buffer(0)]],
    device uchar* out [[buffer(1)]],
    uint gid [[thread_position_in_grid]]) {
  out[gid] = (in[gid] == 0) ? uchar(0) : uchar(1);
}
)metal";

struct Context {
  bool ready{false};
  id<MTLDevice> device{nil};
  id<MTLCommandQueue> queue{nil};
  id<MTLLibrary> library{nil};
  id<MTLComputePipelineState> cast_f32_to_u8{nil};
  id<MTLComputePipelineState> cast_u8_to_f32{nil};
  id<MTLComputePipelineState> cast_f32_to_i64{nil};
  id<MTLComputePipelineState> cast_i64_to_f32{nil};
  id<MTLComputePipelineState> cast_u8_to_i64{nil};
  id<MTLComputePipelineState> cast_i64_to_u8{nil};
  std::string error{};
};

[[noreturn]] void fail_(const std::string& msg) {
  throw_error(msg);
}

Context& ctx_() {
  static Context ctx;
  static std::once_flag once;
  std::call_once(once, [&]() {
    @autoreleasepool {
      ctx.device = shared_device_mm();
      if (ctx.device == nil) {
        ctx.error = "MPS cast init failed: ";
        ctx.error += runtime_error_cstr();
        return;
      }
      ctx.queue = shared_queue_mm();
      if (ctx.queue == nil) {
        ctx.error = "MPS cast init failed: no shared Metal command queue";
        return;
      }

      NSError* err = nil;
      NSString* src = [NSString stringWithUTF8String:kShaderSrc];
      ctx.library = [ctx.device newLibraryWithSource:src options:nil error:&err];
      if (ctx.library == nil) {
        ctx.error = "MPS cast init failed: shader compile error";
        if (err != nil && err.localizedDescription != nil) {
          ctx.error += ": ";
          ctx.error += std::string([err.localizedDescription UTF8String]);
        }
        return;
      }

      auto make_pipeline = [&](const char* fn_name, __strong id<MTLComputePipelineState>* out) {
        NSError* p_err = nil;
        NSString* n = [NSString stringWithUTF8String:fn_name];
        id<MTLFunction> fn = [ctx.library newFunctionWithName:n];
        if (fn == nil) {
          ctx.error = "MPS cast init failed: missing kernel ";
          ctx.error += fn_name;
          return false;
        }
        *out = [ctx.device newComputePipelineStateWithFunction:fn error:&p_err];
        if (*out == nil) {
          ctx.error = "MPS cast init failed: pipeline creation error for ";
          ctx.error += fn_name;
          if (p_err != nil && p_err.localizedDescription != nil) {
            ctx.error += ": ";
            ctx.error += std::string([p_err.localizedDescription UTF8String]);
          }
          return false;
        }
        return true;
      };

      if (!make_pipeline("cast_f32_to_u8", &ctx.cast_f32_to_u8)) return;
      if (!make_pipeline("cast_u8_to_f32", &ctx.cast_u8_to_f32)) return;
      if (!make_pipeline("cast_f32_to_i64", &ctx.cast_f32_to_i64)) return;
      if (!make_pipeline("cast_i64_to_f32", &ctx.cast_i64_to_f32)) return;
      if (!make_pipeline("cast_u8_to_i64", &ctx.cast_u8_to_i64)) return;
      if (!make_pipeline("cast_i64_to_u8", &ctx.cast_i64_to_u8)) return;
      ctx.ready = true;
    }
  });
  return ctx;
}

void validate_(const Series& in, const Series& out) {
  if (!in.defined() || !out.defined()) {
    fail_("mps::cast: undefined Series");
  }
  if (in.platform() != Platform::MPS || out.platform() != Platform::MPS) {
    fail_("mps::cast: expected MPS Series");
  }
  if (!in.is_contiguous() || !out.is_contiguous()) {
    fail_("mps::cast: requires contiguous Series");
  }
  const auto in_sizes = in.sizes();
  const auto out_sizes = out.sizes();
  if (in_sizes.size() != out_sizes.size() ||
      !std::equal(in_sizes.begin(), in_sizes.end(), out_sizes.begin())) {
    fail_("mps::cast: shape mismatch");
  }
}

id<MTLComputePipelineState> choose_pso_(Context& c, ScalarType src, ScalarType dst) {
  if (src == ScalarType::Float32 && dst == ScalarType::Bool) {
    return c.cast_f32_to_u8;
  }
  if (src == ScalarType::Bool && dst == ScalarType::Float32) {
    return c.cast_u8_to_f32;
  }
  if (src == ScalarType::Float32 && dst == ScalarType::Int64) {
    return c.cast_f32_to_i64;
  }
  if (src == ScalarType::Int64 && dst == ScalarType::Float32) {
    return c.cast_i64_to_f32;
  }
  if (src == ScalarType::Bool && dst == ScalarType::Int64) {
    return c.cast_u8_to_i64;
  }
  if (src == ScalarType::Int64 && dst == ScalarType::Bool) {
    return c.cast_i64_to_u8;
  }
  return nil;
}

std::size_t dtype_bytes_(ScalarType dt) {
  switch (dt) {
    case ScalarType::Float32: return sizeof(float);
    case ScalarType::Bool: return sizeof(std::uint8_t);
    case ScalarType::Int64: return sizeof(std::int64_t);
    default: break;
  }
  return 0;
}

} // namespace

bool supports_cast(ScalarType src, ScalarType dst) noexcept {
  return (src == ScalarType::Float32 && dst == ScalarType::Bool) ||
         (src == ScalarType::Bool && dst == ScalarType::Float32) ||
         (src == ScalarType::Float32 && dst == ScalarType::Int64) ||
         (src == ScalarType::Int64 && dst == ScalarType::Float32) ||
         (src == ScalarType::Bool && dst == ScalarType::Int64) ||
         (src == ScalarType::Int64 && dst == ScalarType::Bool);
}

void cast(const Series& in, Series& out) {
  validate_(in, out);
  if (!supports_cast(in.scalar_type(), out.scalar_type())) {
    fail_("mps::cast: unsupported dtype conversion on current MPS path");
  }

  auto& c = ctx_();
  if (!c.ready) {
    fail_(std::string("mps::cast: MPS backend unavailable (") + c.error + ")");
  }

  @autoreleasepool {
    const std::size_t in_bytes = static_cast<std::size_t>(in.numel()) * dtype_bytes_(in.scalar_type());
    const std::size_t out_bytes = static_cast<std::size_t>(out.numel()) * dtype_bytes_(out.scalar_type());
    id<MTLBuffer> in_buf = ensure_device_buffer_for_read_mm(in, c.device, in_bytes);
    id<MTLBuffer> out_buf = ensure_device_buffer_for_write_mm(out, c.device, out_bytes);
    if (in_buf == nil || out_buf == nil) {
      fail_("mps::cast: failed to allocate Metal buffers");
    }

    id<MTLComputePipelineState> pso = choose_pso_(c, in.scalar_type(), out.scalar_type());
    if (pso == nil) {
      fail_("mps::cast: missing pipeline for dtype conversion");
    }

    id<MTLCommandBuffer> cb = create_command_buffer_mm(c.queue);
    if (cb == nil) {
      fail_("mps::cast: failed to create command buffer");
    }
    id<MTLComputeCommandEncoder> enc = create_compute_encoder_mm(cb);
    if (enc == nil) {
      fail_("mps::cast: failed to create compute encoder");
    }
    [enc setComputePipelineState:pso];
    [enc setBuffer:in_buf offset:0 atIndex:0];
    [enc setBuffer:out_buf offset:0 atIndex:1];

    const NSUInteger grid_n = static_cast<NSUInteger>(out.numel());
    NSUInteger tg = pso.maxTotalThreadsPerThreadgroup;
    if (tg == 0) tg = 64;
    tg = std::min<NSUInteger>(tg, 256);
    [enc dispatchThreads:MTLSizeMake(grid_n, 1, 1) threadsPerThreadgroup:MTLSizeMake(tg, 1, 1)];
    [enc endEncoding];

    submit_command_buffer_mm(cb, out.numel());
    mark_device_data_dirty_mm(out, cb);
  }
}

} // namespace pluss::codem::native::mps
