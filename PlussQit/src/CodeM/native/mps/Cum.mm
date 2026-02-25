#include "CodeM/native/mps/Cum.h"
#include "CodeM/native/mps/BufferCacheMM.h"

#include "CodeM/core/Errors.h"

#include <Foundation/Foundation.h>
#include <Metal/Metal.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>

namespace pluss::codem::native::mps {
namespace {

constexpr std::size_t kMaxRank = 8;

constexpr const char* kShaderSrc = R"metal(
#include <metal_stdlib>
using namespace metal;

constant uint kMaxRank = 8;

struct CumMeta {
  uint dim;
  uint ndim;
  ulong sizes[kMaxRank];
  ulong in_strides[kMaxRank];
};

kernel void cumsum_dim_f32(
    const device float* in [[buffer(0)]],
    device float* out [[buffer(1)]],
    constant CumMeta& meta [[buffer(2)]],
    uint gid [[thread_position_in_grid]]) {
  ulong rem = static_cast<ulong>(gid);
  ulong in_off = 0;
  ulong dim_i = 0;
  for (int d = static_cast<int>(meta.ndim) - 1; d >= 0; --d) {
    const ulong sz = meta.sizes[d];
    const ulong coord = (sz == 0) ? 0 : (rem % sz);
    rem = (sz == 0) ? 0 : (rem / sz);
    in_off += coord * meta.in_strides[d];
    if (static_cast<uint>(d) == meta.dim) {
      dim_i = coord;
    }
  }
  const ulong step = meta.in_strides[meta.dim];
  const ulong base = in_off - dim_i * step;
  float acc = 0.0f;
  for (ulong r = 0; r <= dim_i; ++r) {
    acc += in[base + r * step];
  }
  out[gid] = acc;
}

kernel void cumprod_dim_f32(
    const device float* in [[buffer(0)]],
    device float* out [[buffer(1)]],
    constant CumMeta& meta [[buffer(2)]],
    uint gid [[thread_position_in_grid]]) {
  ulong rem = static_cast<ulong>(gid);
  ulong in_off = 0;
  ulong dim_i = 0;
  for (int d = static_cast<int>(meta.ndim) - 1; d >= 0; --d) {
    const ulong sz = meta.sizes[d];
    const ulong coord = (sz == 0) ? 0 : (rem % sz);
    rem = (sz == 0) ? 0 : (rem / sz);
    in_off += coord * meta.in_strides[d];
    if (static_cast<uint>(d) == meta.dim) {
      dim_i = coord;
    }
  }
  const ulong step = meta.in_strides[meta.dim];
  const ulong base = in_off - dim_i * step;
  float acc = 1.0f;
  for (ulong r = 0; r <= dim_i; ++r) {
    acc *= in[base + r * step];
  }
  out[gid] = acc;
}
)metal";

struct CumMetaHost {
  std::uint32_t dim{0};
  std::uint32_t ndim{0};
  std::array<std::uint64_t, kMaxRank> sizes{};
  std::array<std::uint64_t, kMaxRank> in_strides{};
};

struct Context {
  bool ready{false};
  id<MTLDevice> device{nil};
  id<MTLCommandQueue> queue{nil};
  id<MTLLibrary> library{nil};
  id<MTLComputePipelineState> cumsum_dim_f32{nil};
  id<MTLComputePipelineState> cumprod_dim_f32{nil};
  std::string error{};
};

[[noreturn]] void fail_(const std::string& msg) {
  throw_error(msg);
}

std::int64_t normalize_dim_(std::int64_t dim, std::int64_t nd, const char* op) {
  if (dim < 0) dim += nd;
  if (dim < 0 || dim >= nd) {
    fail_(std::string(op) + ": dim out of range");
  }
  return dim;
}

Context& ctx_() {
  static Context ctx;
  static std::once_flag once;
  std::call_once(once, [&]() {
    @autoreleasepool {
      ctx.device = shared_device_mm();
      if (ctx.device == nil) {
        ctx.error = "MPS cum init failed: ";
        ctx.error += runtime_error_cstr();
        return;
      }
      ctx.queue = shared_queue_mm();
      if (ctx.queue == nil) {
        ctx.error = "MPS cum init failed: no shared Metal command queue";
        return;
      }

      NSError* err = nil;
      NSString* src = [NSString stringWithUTF8String:kShaderSrc];
      ctx.library = [ctx.device newLibraryWithSource:src options:nil error:&err];
      if (ctx.library == nil) {
        ctx.error = "MPS cum init failed: shader compile error";
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
          ctx.error = "MPS cum init failed: missing kernel ";
          ctx.error += fn_name;
          return false;
        }
        *out = [ctx.device newComputePipelineStateWithFunction:fn error:&p_err];
        if (*out == nil) {
          ctx.error = "MPS cum init failed: pipeline creation error for ";
          ctx.error += fn_name;
          if (p_err != nil && p_err.localizedDescription != nil) {
            ctx.error += ": ";
            ctx.error += std::string([p_err.localizedDescription UTF8String]);
          }
          return false;
        }
        return true;
      };

      if (!make_pipeline("cumsum_dim_f32", &ctx.cumsum_dim_f32)) return;
      if (!make_pipeline("cumprod_dim_f32", &ctx.cumprod_dim_f32)) return;
      ctx.ready = true;
    }
  });
  return ctx;
}

void validate_inputs_(const Series& in, const Series& out, std::int64_t dim, CumMetaHost& meta, const char* op) {
  if (!in.defined() || !out.defined()) {
    fail_(std::string(op) + ": undefined Series");
  }
  if (in.platform() != Platform::MPS || out.platform() != Platform::MPS) {
    fail_(std::string(op) + ": expected MPS Series");
  }
  if (!in.is_contiguous() || !out.is_contiguous()) {
    fail_(std::string(op) + ": requires contiguous Series");
  }
  if (in.scalar_type() != ScalarType::Float32 || out.scalar_type() != ScalarType::Float32) {
    fail_(std::string(op) + ": only float32 is supported");
  }
  if (!std::equal(in.sizes().begin(), in.sizes().end(), out.sizes().begin(), out.sizes().end())) {
    fail_(std::string(op) + ": input/output shape mismatch");
  }
  if (in.sizes().size() > kMaxRank) {
    fail_(std::string(op) + ": rank > 8 unsupported on current MPS path");
  }

  const auto nd = static_cast<std::int64_t>(in.sizes().size());
  const auto d = normalize_dim_(dim, nd, op);
  meta.dim = static_cast<std::uint32_t>(d);
  meta.ndim = static_cast<std::uint32_t>(in.sizes().size());
  meta.sizes.fill(1);
  meta.in_strides.fill(0);
  for (std::size_t i = 0; i < in.sizes().size(); ++i) {
    const auto sz = in.sizes()[i];
    const auto st = in.strides()[i];
    if (sz < 0 || st < 0) {
      fail_(std::string(op) + ": negative size/stride unsupported");
    }
    meta.sizes[i] = static_cast<std::uint64_t>(sz);
    meta.in_strides[i] = static_cast<std::uint64_t>(st);
  }
}

void encode_cum_(id<MTLComputePipelineState> pso,
                 const Series& in,
                 Series& out,
                 const CumMetaHost& meta,
                 const char* op_name) {
  auto& c = ctx_();
  @autoreleasepool {
    const std::size_t in_bytes = static_cast<std::size_t>(in.numel()) * sizeof(float);
    const std::size_t out_bytes = static_cast<std::size_t>(out.numel()) * sizeof(float);
    id<MTLBuffer> in_buf = ensure_device_buffer_for_read_mm(in, c.device, in_bytes);
    id<MTLBuffer> out_buf = ensure_device_buffer_for_write_mm(out, c.device, out_bytes);
    if (in_buf == nil || out_buf == nil) {
      fail_(std::string(op_name) + ": failed to allocate Metal buffers");
    }

    id<MTLCommandBuffer> cb = create_command_buffer_mm(c.queue);
    if (cb == nil) {
      fail_(std::string(op_name) + ": failed to create command buffer");
    }
    id<MTLComputeCommandEncoder> enc = create_compute_encoder_mm(cb);
    if (enc == nil) {
      fail_(std::string(op_name) + ": failed to create compute encoder");
    }
    [enc setComputePipelineState:pso];
    [enc setBuffer:in_buf offset:0 atIndex:0];
    [enc setBuffer:out_buf offset:0 atIndex:1];
    [enc setBytes:&meta length:sizeof(meta) atIndex:2];

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

} // namespace

bool supports_cumsum_dim(ScalarType dt) noexcept {
  return dt == ScalarType::Float32;
}

bool supports_cumprod_dim(ScalarType dt) noexcept {
  return dt == ScalarType::Float32;
}

void cumsum_dim(const Series& in, std::int64_t dim, Series& out) {
  auto& c = ctx_();
  if (!c.ready) {
    fail_("mps::cumsum_dim: backend unavailable (" + c.error + ")");
  }
  if (out.numel() == 0) return;
  CumMetaHost meta;
  validate_inputs_(in, out, dim, meta, "mps::cumsum_dim");
  encode_cum_(c.cumsum_dim_f32, in, out, meta, "mps::cumsum_dim");
}

void cumprod_dim(const Series& in, std::int64_t dim, Series& out) {
  auto& c = ctx_();
  if (!c.ready) {
    fail_("mps::cumprod_dim: backend unavailable (" + c.error + ")");
  }
  if (out.numel() == 0) return;
  CumMetaHost meta;
  validate_inputs_(in, out, dim, meta, "mps::cumprod_dim");
  encode_cum_(c.cumprod_dim_f32, in, out, meta, "mps::cumprod_dim");
}

} // namespace pluss::codem::native::mps
