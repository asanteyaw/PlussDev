#include "CodeM/native/mps/Elementwise.h"
#include "CodeM/native/mps/BufferCacheMM.h"
#include "CodeM/native/mps/MPSAllocatorMM.h"
#include "CodeM/native/mps/MPSGraphCache.h"

#include "CodeM/core/Errors.h"

#include <Foundation/Foundation.h>
#include <Metal/Metal.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <span>
#include <string>
#include <vector>

namespace pluss::codem::native::mps {
namespace {

constexpr std::size_t kMaxRank = 8;

constexpr const char* kShaderSrc = R"metal(
#include <metal_stdlib>
using namespace metal;

constant uint kMaxRank = 8;

struct BinaryMeta {
  uint op;
  uint mode;
  uint ndim;
  ulong out_sizes[kMaxRank];
  ulong a_strides[kMaxRank];
  ulong b_strides[kMaxRank];
};

struct TernaryMeta {
  uint mode;
  uint ndim;
  ulong out_sizes[kMaxRank];
  ulong cond_strides[kMaxRank];
  ulong a_strides[kMaxRank];
  ulong b_strides[kMaxRank];
};

inline ulong offset_from_linear(uint gid,
                                uint ndim,
                                constant ulong sizes[kMaxRank],
                                constant ulong strides[kMaxRank]) {
  ulong rem = static_cast<ulong>(gid);
  ulong off = 0;
  for (int d = static_cast<int>(ndim) - 1; d >= 0; --d) {
    const ulong sz = sizes[d];
    const ulong idx = (sz == 0) ? 0 : (rem % sz);
    rem = (sz == 0) ? 0 : (rem / sz);
    off += idx * strides[d];
  }
  return off;
}

kernel void binary_f32(
    const device float* a [[buffer(0)]],
    const device float* b [[buffer(1)]],
    device float* out [[buffer(2)]],
    constant BinaryMeta& meta [[buffer(3)]],
    uint gid [[thread_position_in_grid]]) {
  ulong ao = 0;
  ulong bo = 0;
  if (meta.mode == 0) {
    ao = static_cast<ulong>(gid);
    bo = static_cast<ulong>(gid);
  } else if (meta.mode == 1) {
    ao = 0;
    bo = static_cast<ulong>(gid);
  } else if (meta.mode == 2) {
    ao = static_cast<ulong>(gid);
    bo = 0;
  } else {
    ao = offset_from_linear(gid, meta.ndim, meta.out_sizes, meta.a_strides);
    bo = offset_from_linear(gid, meta.ndim, meta.out_sizes, meta.b_strides);
  }
  const float av = a[ao];
  const float bv = b[bo];
  float v = 0.0f;
  switch (meta.op) {
    case 0: v = av + bv; break;
    case 1: v = av - bv; break;
    case 2: v = av * bv; break;
    case 3: v = av / bv; break;
    case 4: v = pow(av, bv); break;
    default: v = 0.0f; break;
  }
  out[gid] = v;
}

kernel void binary_cmp_f32(
    const device float* a [[buffer(0)]],
    const device float* b [[buffer(1)]],
    device uchar* out [[buffer(2)]],
    constant BinaryMeta& meta [[buffer(3)]],
    uint gid [[thread_position_in_grid]]) {
  ulong ao = 0;
  ulong bo = 0;
  if (meta.mode == 0) {
    ao = static_cast<ulong>(gid);
    bo = static_cast<ulong>(gid);
  } else if (meta.mode == 1) {
    ao = 0;
    bo = static_cast<ulong>(gid);
  } else if (meta.mode == 2) {
    ao = static_cast<ulong>(gid);
    bo = 0;
  } else {
    ao = offset_from_linear(gid, meta.ndim, meta.out_sizes, meta.a_strides);
    bo = offset_from_linear(gid, meta.ndim, meta.out_sizes, meta.b_strides);
  }
  const float av = a[ao];
  const float bv = b[bo];
  bool v = false;
  switch (meta.op) {
    case 0: v = (av == bv); break;
    case 1: v = (av != bv); break;
    case 2: v = (av <  bv); break;
    case 3: v = (av <= bv); break;
    case 4: v = (av >  bv); break;
    case 5: v = (av >= bv); break;
    default: v = false; break;
  }
  out[gid] = v ? uchar(1) : uchar(0);
}

kernel void binary_c64(
    const device float2* a [[buffer(0)]],
    const device float2* b [[buffer(1)]],
    device float2* out [[buffer(2)]],
    constant BinaryMeta& meta [[buffer(3)]],
    uint gid [[thread_position_in_grid]]) {
  ulong ao = 0;
  ulong bo = 0;
  if (meta.mode == 0) {
    ao = static_cast<ulong>(gid);
    bo = static_cast<ulong>(gid);
  } else if (meta.mode == 1) {
    ao = 0;
    bo = static_cast<ulong>(gid);
  } else if (meta.mode == 2) {
    ao = static_cast<ulong>(gid);
    bo = 0;
  } else {
    ao = offset_from_linear(gid, meta.ndim, meta.out_sizes, meta.a_strides);
    bo = offset_from_linear(gid, meta.ndim, meta.out_sizes, meta.b_strides);
  }
  const float2 av = a[ao];
  const float2 bv = b[bo];
  float2 v = float2(0.0f, 0.0f);
  switch (meta.op) {
    case 0: v = av + bv; break;
    case 1: v = av - bv; break;
    case 2: v = float2(av.x * bv.x - av.y * bv.y, av.x * bv.y + av.y * bv.x); break;
    case 3: {
      const float denom = bv.x * bv.x + bv.y * bv.y;
      v = float2((av.x * bv.x + av.y * bv.y) / denom, (av.y * bv.x - av.x * bv.y) / denom);
      break;
    }
    default: v = float2(0.0f, 0.0f); break;
  }
  out[gid] = v;
}

kernel void binary_cmp_c64(
    const device float2* a [[buffer(0)]],
    const device float2* b [[buffer(1)]],
    device uchar* out [[buffer(2)]],
    constant BinaryMeta& meta [[buffer(3)]],
    uint gid [[thread_position_in_grid]]) {
  ulong ao = 0;
  ulong bo = 0;
  if (meta.mode == 0) {
    ao = static_cast<ulong>(gid);
    bo = static_cast<ulong>(gid);
  } else if (meta.mode == 1) {
    ao = 0;
    bo = static_cast<ulong>(gid);
  } else if (meta.mode == 2) {
    ao = static_cast<ulong>(gid);
    bo = 0;
  } else {
    ao = offset_from_linear(gid, meta.ndim, meta.out_sizes, meta.a_strides);
    bo = offset_from_linear(gid, meta.ndim, meta.out_sizes, meta.b_strides);
  }
  const float2 av = a[ao];
  const float2 bv = b[bo];
  const bool eq = (av.x == bv.x) && (av.y == bv.y);
  bool v = false;
  switch (meta.op) {
    case 0: v = eq; break;
    case 1: v = !eq; break;
    default: v = false; break;
  }
  out[gid] = v ? uchar(1) : uchar(0);
}

kernel void maximum_f32(
    const device float* a [[buffer(0)]],
    const device float* b [[buffer(1)]],
    device float* out [[buffer(2)]],
    constant BinaryMeta& meta [[buffer(3)]],
    uint gid [[thread_position_in_grid]]) {
  ulong ao = 0;
  ulong bo = 0;
  if (meta.mode == 0) {
    ao = static_cast<ulong>(gid);
    bo = static_cast<ulong>(gid);
  } else if (meta.mode == 1) {
    ao = 0;
    bo = static_cast<ulong>(gid);
  } else if (meta.mode == 2) {
    ao = static_cast<ulong>(gid);
    bo = 0;
  } else {
    ao = offset_from_linear(gid, meta.ndim, meta.out_sizes, meta.a_strides);
    bo = offset_from_linear(gid, meta.ndim, meta.out_sizes, meta.b_strides);
  }
  out[gid] = fmax(a[ao], b[bo]);
}

kernel void minimum_f32(
    const device float* a [[buffer(0)]],
    const device float* b [[buffer(1)]],
    device float* out [[buffer(2)]],
    constant BinaryMeta& meta [[buffer(3)]],
    uint gid [[thread_position_in_grid]]) {
  ulong ao = 0;
  ulong bo = 0;
  if (meta.mode == 0) {
    ao = static_cast<ulong>(gid);
    bo = static_cast<ulong>(gid);
  } else if (meta.mode == 1) {
    ao = 0;
    bo = static_cast<ulong>(gid);
  } else if (meta.mode == 2) {
    ao = static_cast<ulong>(gid);
    bo = 0;
  } else {
    ao = offset_from_linear(gid, meta.ndim, meta.out_sizes, meta.a_strides);
    bo = offset_from_linear(gid, meta.ndim, meta.out_sizes, meta.b_strides);
  }
  out[gid] = fmin(a[ao], b[bo]);
}

kernel void where_f32(
    const device uchar* cond [[buffer(0)]],
    const device float* a [[buffer(1)]],
    const device float* b [[buffer(2)]],
    device float* out [[buffer(3)]],
    constant TernaryMeta& meta [[buffer(4)]],
    uint gid [[thread_position_in_grid]]) {
  ulong co = 0;
  ulong ao = 0;
  ulong bo = 0;
  if (meta.mode == 0) {
    co = static_cast<ulong>(gid);
    ao = static_cast<ulong>(gid);
    bo = static_cast<ulong>(gid);
  } else {
    co = offset_from_linear(gid, meta.ndim, meta.out_sizes, meta.cond_strides);
    ao = offset_from_linear(gid, meta.ndim, meta.out_sizes, meta.a_strides);
    bo = offset_from_linear(gid, meta.ndim, meta.out_sizes, meta.b_strides);
  }
  out[gid] = (cond[co] != 0) ? a[ao] : b[bo];
}

kernel void where_c64(
    const device uchar* cond [[buffer(0)]],
    const device float2* a [[buffer(1)]],
    const device float2* b [[buffer(2)]],
    device float2* out [[buffer(3)]],
    constant TernaryMeta& meta [[buffer(4)]],
    uint gid [[thread_position_in_grid]]) {
  ulong co = 0;
  ulong ao = 0;
  ulong bo = 0;
  if (meta.mode == 0) {
    co = static_cast<ulong>(gid);
    ao = static_cast<ulong>(gid);
    bo = static_cast<ulong>(gid);
  } else {
    co = offset_from_linear(gid, meta.ndim, meta.out_sizes, meta.cond_strides);
    ao = offset_from_linear(gid, meta.ndim, meta.out_sizes, meta.a_strides);
    bo = offset_from_linear(gid, meta.ndim, meta.out_sizes, meta.b_strides);
  }
  out[gid] = (cond[co] != 0) ? a[ao] : b[bo];
}

struct UnaryMeta {
  uint op;
};

kernel void unary_f32(
    const device float* in [[buffer(0)]],
    device float* out [[buffer(1)]],
    constant UnaryMeta& meta [[buffer(2)]],
    uint gid [[thread_position_in_grid]]) {
  const float x = in[gid];
  float v = 0.0f;
  switch (meta.op) {
    case 0: v = -x; break;
    case 1: v = fabs(x); break;
    case 2: v = log(1.0f + x); break;
    case 3: v = exp(x) - 1.0f; break;
    case 4: v = exp(x); break;
    case 5: v = log(x); break;
    case 6: v = sqrt(x); break;
    case 7: v = sin(x); break;
    case 8: v = cos(x); break;
    case 9: v = tanh(x); break;
    case 10: v = tan(x); break;
    case 11: v = asin(x); break;
    case 12: v = acos(x); break;
    case 13: v = atan(x); break;
    case 14: v = sinh(x); break;
    case 15: v = cosh(x); break;
    case 16: v = 1.0f / x; break;
    case 17: v = floor(x); break;
    case 18: v = ceil(x); break;
    case 19: v = round(x); break;
    case 23: v = trunc(x); break;
    case 24: v = max(0.0f, x); break;
    case 25: v = 1.0f / (1.0f + exp(-x)); break;
    default: v = 0.0f; break;
  }
  out[gid] = v;
}

kernel void complex_conj_c64(
    const device float2* in [[buffer(0)]],
    device float2* out [[buffer(1)]],
    constant UnaryMeta& meta [[buffer(2)]],
    uint gid [[thread_position_in_grid]]) {
  (void)meta;
  const float2 x = in[gid];
  out[gid] = float2(x.x, -x.y);
}

kernel void complex_component_c64(
    const device float2* in [[buffer(0)]],
    device float* out [[buffer(1)]],
    constant UnaryMeta& meta [[buffer(2)]],
    uint gid [[thread_position_in_grid]]) {
  const float2 x = in[gid];
  float v = 0.0f;
  switch (meta.op) {
    case 0: v = x.x; break;
    case 1: v = x.y; break;
    case 2: v = length(x); break;
    case 3: v = atan2(x.y, x.x); break;
    default: v = 0.0f; break;
  }
  out[gid] = v;
}

kernel void complex_from_parts_c64(
    const device float* real [[buffer(0)]],
    const device float* imag [[buffer(1)]],
    device float2* out [[buffer(2)]],
    constant UnaryMeta& meta [[buffer(3)]],
    uint gid [[thread_position_in_grid]]) {
  (void)meta;
  out[gid] = float2(real[gid], imag[gid]);
}

kernel void unary_pred_f32(
    const device float* in [[buffer(0)]],
    device uchar* out [[buffer(1)]],
    constant UnaryMeta& meta [[buffer(2)]],
    uint gid [[thread_position_in_grid]]) {
  const float x = in[gid];
  bool v = false;
  switch (meta.op) {
    case 20: v = isnan(x); break;
    case 21: v = isinf(x); break;
    case 22: v = isfinite(x); break;
    default: v = false; break;
  }
  out[gid] = v ? uchar(1) : uchar(0);
}
)metal";

struct BinaryMetaHost {
  std::uint32_t op{0};
  std::uint32_t mode{3};
  std::uint32_t ndim{0};
  std::array<std::uint64_t, kMaxRank> out_sizes{};
  std::array<std::uint64_t, kMaxRank> a_strides{};
  std::array<std::uint64_t, kMaxRank> b_strides{};
};

struct TernaryMetaHost {
  std::uint32_t mode{1};
  std::uint32_t ndim{0};
  std::array<std::uint64_t, kMaxRank> out_sizes{};
  std::array<std::uint64_t, kMaxRank> cond_strides{};
  std::array<std::uint64_t, kMaxRank> a_strides{};
  std::array<std::uint64_t, kMaxRank> b_strides{};
};

struct UnaryMetaHost {
  std::uint32_t op{0};
};

struct Context {
  bool ready{false};
  id<MTLDevice> device{nil};
  id<MTLCommandQueue> queue{nil};
  id<MTLLibrary> library{nil};
  id<MTLComputePipelineState> binary_f32{nil};
  id<MTLComputePipelineState> binary_cmp_f32{nil};
  id<MTLComputePipelineState> binary_c64{nil};
  id<MTLComputePipelineState> binary_cmp_c64{nil};
  id<MTLComputePipelineState> maximum_f32{nil};
  id<MTLComputePipelineState> minimum_f32{nil};
  id<MTLComputePipelineState> where_f32{nil};
  id<MTLComputePipelineState> where_c64{nil};
  id<MTLComputePipelineState> unary_f32{nil};
  id<MTLComputePipelineState> complex_conj_c64{nil};
  id<MTLComputePipelineState> complex_component_c64{nil};
  id<MTLComputePipelineState> complex_from_parts_c64{nil};
  id<MTLComputePipelineState> unary_pred_f32{nil};
  std::string error{};
};

Context& ctx_() {
  static Context ctx;
  static std::once_flag init_once;
  std::call_once(init_once, [&]() {
    @autoreleasepool {
      ctx.device = shared_device_mm();
      if (ctx.device == nil) {
        ctx.error = "MPS init failed: ";
        ctx.error += runtime_error_cstr();
        return;
      }
      ctx.queue = shared_queue_mm();
      if (ctx.queue == nil) {
        ctx.error = "MPS init failed: no shared Metal command queue";
        return;
      }

      NSError* err = nil;
      NSString* src = [NSString stringWithUTF8String:kShaderSrc];
      ctx.library = [ctx.device newLibraryWithSource:src options:nil error:&err];
      if (ctx.library == nil) {
        ctx.error = "MPS init failed: shader compile error";
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
          ctx.error = "MPS init failed: missing kernel function ";
          ctx.error += fn_name;
          return false;
        }
        *out = [ctx.device newComputePipelineStateWithFunction:fn error:&p_err];
        if (*out == nil) {
          ctx.error = "MPS init failed: pipeline creation error for ";
          ctx.error += fn_name;
          if (p_err != nil && p_err.localizedDescription != nil) {
            ctx.error += ": ";
            ctx.error += std::string([p_err.localizedDescription UTF8String]);
          }
          return false;
        }
        return true;
      };

      if (!make_pipeline("binary_f32", &ctx.binary_f32)) return;
      if (!make_pipeline("binary_cmp_f32", &ctx.binary_cmp_f32)) return;
      if (!make_pipeline("binary_c64", &ctx.binary_c64)) return;
      if (!make_pipeline("binary_cmp_c64", &ctx.binary_cmp_c64)) return;
      if (!make_pipeline("maximum_f32", &ctx.maximum_f32)) return;
      if (!make_pipeline("minimum_f32", &ctx.minimum_f32)) return;
      if (!make_pipeline("where_f32", &ctx.where_f32)) return;
      if (!make_pipeline("where_c64", &ctx.where_c64)) return;
      if (!make_pipeline("unary_f32", &ctx.unary_f32)) return;
      if (!make_pipeline("complex_conj_c64", &ctx.complex_conj_c64)) return;
      if (!make_pipeline("complex_component_c64", &ctx.complex_component_c64)) return;
      if (!make_pipeline("complex_from_parts_c64", &ctx.complex_from_parts_c64)) return;
      if (!make_pipeline("unary_pred_f32", &ctx.unary_pred_f32)) return;
      ctx.ready = true;
    }
  });
  return ctx;
}

[[noreturn]] void fail_(const std::string& msg) {
  throw_error(msg);
}

void require_contiguous_(const Series& s, const char* op) {
  if (!s.is_contiguous()) {
    fail_(std::string(op) + ": MPS path currently requires contiguous Series");
  }
}

void validate_mps_tensor_(const Series& s, const char* op) {
  if (!s.defined()) {
    fail_(std::string(op) + ": undefined Series");
  }
  if (s.platform() != Platform::MPS) {
    fail_(std::string(op) + ": expected MPS Series");
  }
  if (s.scalar_type() != ScalarType::Float32 && s.scalar_type() != ScalarType::Bool &&
      s.scalar_type() != ScalarType::Complex64) {
    fail_(std::string(op) + ": unsupported dtype on current MPS path");
  }
  require_contiguous_(s, op);
}

id<MTLCommandBuffer> encode_op_(id<MTLComputePipelineState> pso,
                                id<MTLBuffer> b0,
                                id<MTLBuffer> b1,
                                id<MTLBuffer> b2,
                                id<MTLBuffer> b3,
                                const void* meta_ptr,
                                std::size_t meta_size,
                                std::uint32_t meta_index,
                                std::int64_t n,
                                const char* op_name) {
  auto& c = ctx_();
  if (!c.ready) {
    fail_(std::string(op_name) + ": MPS backend unavailable (" + c.error + ")");
  }
  if (n < 0) {
    fail_(std::string(op_name) + ": invalid numel");
  }
  if (n == 0) {
    return nil;
  }
  @autoreleasepool {
    id<MTLCommandBuffer> cb = create_command_buffer_mm(c.queue);
    if (cb == nil) {
      fail_(std::string(op_name) + ": failed to create command buffer");
    }
    id<MTLComputeCommandEncoder> enc = create_compute_encoder_mm(cb);
    if (enc == nil) {
      fail_(std::string(op_name) + ": failed to create compute encoder");
    }
    [enc setComputePipelineState:pso];
    if (b0 != nil) [enc setBuffer:b0 offset:0 atIndex:0];
    if (b1 != nil) [enc setBuffer:b1 offset:0 atIndex:1];
    if (b2 != nil) [enc setBuffer:b2 offset:0 atIndex:2];
    if (b3 != nil) [enc setBuffer:b3 offset:0 atIndex:3];
    id<MTLBuffer> meta_buf = acquire_pooled_buffer_mm(c.device, meta_size, op_name);
    if (meta_buf == nil) {
      fail_(std::string(op_name) + ": failed to allocate metadata buffer");
    }
    std::memcpy([meta_buf contents], meta_ptr, meta_size);
    [enc setBuffer:meta_buf offset:0 atIndex:meta_index];

    const NSUInteger grid_n = static_cast<NSUInteger>(n);
    NSUInteger tg = pso.maxTotalThreadsPerThreadgroup;
    if (tg == 0) tg = 64;
    tg = std::min<NSUInteger>(tg, 256);
    const MTLSize grid = MTLSizeMake(grid_n, 1, 1);
    const MTLSize group = MTLSizeMake(tg, 1, 1);
    [enc dispatchThreads:grid threadsPerThreadgroup:group];
    [enc endEncoding];
    defer_release_pooled_buffer_mm(meta_buf, cb);
    return cb;
  }
}

void fill_out_sizes_(std::span<const std::int64_t> out_sizes,
                     std::array<std::uint64_t, kMaxRank>& dst,
                     const char* op) {
  if (out_sizes.size() > kMaxRank) {
    fail_(std::string(op) + ": rank > 8 is unsupported on current MPS path");
  }
  dst.fill(1);
  for (std::size_t i = 0; i < out_sizes.size(); ++i) {
    if (out_sizes[i] < 0) {
      fail_(std::string(op) + ": negative output size");
    }
    dst[i] = static_cast<std::uint64_t>(out_sizes[i]);
  }
}

void fill_broadcast_strides_(const Series& in,
                             std::span<const std::int64_t> out_sizes,
                             std::array<std::uint64_t, kMaxRank>& dst,
                             const char* op,
                             const char* which) {
  const auto in_sizes = in.sizes();
  const auto in_strides = in.strides();
  if (in_sizes.size() != in_strides.size()) {
    fail_(std::string(op) + ": invalid sizes/strides for " + which);
  }
  if (out_sizes.size() > kMaxRank) {
    fail_(std::string(op) + ": rank > 8 is unsupported");
  }
  if (in_sizes.size() > out_sizes.size()) {
    fail_(std::string(op) + ": input rank exceeds output rank for " + which);
  }

  dst.fill(0);
  std::size_t in_d = in_sizes.size();
  for (std::size_t out_d = out_sizes.size(); out_d > 0; --out_d) {
    const std::size_t oi = out_d - 1;
    const auto out_sz = out_sizes[oi];
    if (out_sz < 0) {
      fail_(std::string(op) + ": negative output size for " + which);
    }

    std::int64_t in_sz = 1;
    std::int64_t in_st = 0;
    if (in_d > 0) {
      --in_d;
      in_sz = in_sizes[in_d];
      in_st = in_strides[in_d];
    }

    if (in_sz == out_sz) {
      if (in_st < 0) {
        fail_(std::string(op) + ": negative stride unsupported for " + which);
      }
      dst[oi] = static_cast<std::uint64_t>(in_st);
    } else if (in_sz == 1) {
      dst[oi] = 0;
    } else {
      fail_(std::string(op) + ": incompatible broadcast shape for " + which);
    }
  }
}

bool same_shape_(const Series& x, const Series& y) {
  const auto xs = x.sizes();
  const auto ys = y.sizes();
  return xs.size() == ys.size() && std::equal(xs.begin(), xs.end(), ys.begin());
}

std::uint32_t binary_mode_(const Series& a, const Series& b, const Series& out) {
  if (a.numel() == out.numel() && b.numel() == out.numel() && same_shape_(a, out) && same_shape_(b, out)) {
    return 0; // linear
  }
  if (a.numel() == 1 && b.numel() == out.numel() && same_shape_(b, out)) {
    return 1; // scalar lhs
  }
  if (b.numel() == 1 && a.numel() == out.numel() && same_shape_(a, out)) {
    return 2; // scalar rhs
  }
  return 3; // general broadcast
}

std::uint32_t ternary_mode_(const Series& cond, const Series& a, const Series& b, const Series& out) {
  if (cond.numel() == out.numel() && a.numel() == out.numel() && b.numel() == out.numel() &&
      same_shape_(cond, out) && same_shape_(a, out) && same_shape_(b, out)) {
    return 0; // linear
  }
  return 1; // general broadcast
}

std::uint32_t binary_op_code_(cpu::BinaryOpId op, bool cmp) {
  using B = cpu::BinaryOpId;
  if (!cmp) {
    switch (op) {
      case B::Add: return 0;
      case B::Sub: return 1;
      case B::Mul: return 2;
      case B::Div: return 3;
      case B::Pow: return 4;
      default: break;
    }
  } else {
    switch (op) {
      case B::Eq: return 0;
      case B::Ne: return 1;
      case B::Lt: return 2;
      case B::Le: return 3;
      case B::Gt: return 4;
      case B::Ge: return 5;
      default: break;
    }
  }
  return 999;
}

std::uint32_t unary_op_code_(cpu::UnaryOpId op) {
  using U = cpu::UnaryOpId;
  switch (op) {
    case U::Neg: return 0;
    case U::Abs: return 1;
    case U::Log1p: return 2;
    case U::Expm1: return 3;
    default: return 999;
  }
}

std::uint32_t unary_math_code_(UnaryMathOp op) {
  switch (op) {
    case UnaryMathOp::Exp: return 4;
    case UnaryMathOp::Log: return 5;
    case UnaryMathOp::Sqrt: return 6;
    case UnaryMathOp::Sin: return 7;
    case UnaryMathOp::Cos: return 8;
    case UnaryMathOp::Tanh: return 9;
    case UnaryMathOp::Tan: return 10;
    case UnaryMathOp::Asin: return 11;
    case UnaryMathOp::Acos: return 12;
    case UnaryMathOp::Atan: return 13;
    case UnaryMathOp::Sinh: return 14;
    case UnaryMathOp::Cosh: return 15;
    case UnaryMathOp::Reciprocal: return 16;
    case UnaryMathOp::Floor: return 17;
    case UnaryMathOp::Ceil: return 18;
    case UnaryMathOp::Round: return 19;
    case UnaryMathOp::Trunc: return 23;
    case UnaryMathOp::Relu: return 24;
    case UnaryMathOp::Sigmoid: return 25;
    default: return 999;
  }
}

std::uint32_t unary_pred_code_(UnaryPredOp op) {
  switch (op) {
    case UnaryPredOp::IsNan: return 20;
    case UnaryPredOp::IsInf: return 21;
    case UnaryPredOp::IsFinite: return 22;
    default: return 999;
  }
}

void append_dims_(std::vector<std::int64_t>& dst, std::span<const std::int64_t> dims) {
  dst.push_back(static_cast<std::int64_t>(dims.size()));
  dst.insert(dst.end(), dims.begin(), dims.end());
}

std::vector<std::int64_t> make_binary_cache_aux_(std::uint32_t op_code, std::uint32_t mode,
                                                 const Series& a, const Series& b, const Series& out) {
  std::vector<std::int64_t> aux;
  aux.reserve(4 + a.dim() + b.dim() + out.dim());
  aux.push_back(static_cast<std::int64_t>(op_code));
  aux.push_back(static_cast<std::int64_t>(mode));
  append_dims_(aux, a.sizes());
  append_dims_(aux, b.sizes());
  append_dims_(aux, out.sizes());
  return aux;
}

std::vector<std::int64_t> make_where_cache_aux_(std::uint32_t mode,
                                                const Series& cond,
                                                const Series& a,
                                                const Series& b,
                                                const Series& out) {
  std::vector<std::int64_t> aux;
  aux.reserve(3 + cond.dim() + a.dim() + b.dim() + out.dim());
  aux.push_back(static_cast<std::int64_t>(mode));
  append_dims_(aux, cond.sizes());
  append_dims_(aux, a.sizes());
  append_dims_(aux, b.sizes());
  append_dims_(aux, out.sizes());
  return aux;
}

} // namespace

bool is_available() noexcept {
  return ctx_().ready;
}

bool supports_binary(cpu::BinaryOpId op, ScalarType dt) noexcept {
  using B = cpu::BinaryOpId;
  if (dt == ScalarType::Complex64) {
    switch (op) {
      case B::Add:
      case B::Sub:
      case B::Mul:
      case B::Div:
      case B::Eq:
      case B::Ne:
        return true;
      default:
        return false;
    }
  }
  if (dt != ScalarType::Float32) {
    return false;
  }
  switch (op) {
    case B::Add:
    case B::Sub:
    case B::Mul:
    case B::Div:
    case B::Pow:
    case B::Eq:
    case B::Ne:
    case B::Lt:
    case B::Le:
    case B::Gt:
    case B::Ge:
      return true;
    default:
      return false;
  }
}

bool supports_maximum_minimum(ScalarType dt) noexcept {
  return dt == ScalarType::Float32;
}

void binary(cpu::BinaryOpId op, const Series& a, const Series& b, Series& out) {
  if (!is_available()) {
    fail_("binary: MPS backend unavailable (" + ctx_().error + ")");
  }
  Series lhs = a;
  Series rhs = b;
  // Typed scalar invocations (e.g., Series::mul(double)) may supply a host
  // 1-element tensor. Promote those scalars to MPS to keep the op native.
  if (lhs.platform() != Platform::MPS && lhs.defined() && lhs.numel() == 1) {
    lhs = lhs.to(Platform::MPS, out.scalar_type() == ScalarType::Complex64 ? ScalarType::Complex64 : ScalarType::Float32);
  }
  if (rhs.platform() != Platform::MPS && rhs.defined() && rhs.numel() == 1) {
    rhs = rhs.to(Platform::MPS, out.scalar_type() == ScalarType::Complex64 ? ScalarType::Complex64 : ScalarType::Float32);
  }
  validate_mps_tensor_(lhs, "binary");
  validate_mps_tensor_(rhs, "binary");
  validate_mps_tensor_(out, "binary");
  const ScalarType compute_dt = lhs.scalar_type();
  if (compute_dt != rhs.scalar_type()) {
    fail_("binary: dtype mismatch on current MPS path");
  }
  if (compute_dt != ScalarType::Float32 && compute_dt != ScalarType::Complex64) {
    fail_("binary: only float32 and complex64 inputs are supported on current MPS path");
  }

  const bool is_cmp = (op == cpu::BinaryOpId::Eq || op == cpu::BinaryOpId::Ne || op == cpu::BinaryOpId::Lt ||
                       op == cpu::BinaryOpId::Le || op == cpu::BinaryOpId::Gt || op == cpu::BinaryOpId::Ge);
  if (!supports_binary(op, compute_dt)) {
    fail_("binary: unsupported op on current MPS path");
  }
  if (is_cmp) {
    if (out.scalar_type() != ScalarType::Bool) {
      fail_("binary: comparison output must be bool");
    }
  } else if (out.scalar_type() != compute_dt) {
    fail_("binary: arithmetic output dtype mismatch");
  }
  if (out.numel() < 0) {
    fail_("binary: invalid output numel");
  }

  auto& c = ctx_();
  @autoreleasepool {
    const std::size_t elem_bytes = compute_dt == ScalarType::Complex64 ? sizeof(float) * 2 : sizeof(float);
    const std::size_t a_bytes = static_cast<std::size_t>(lhs.numel()) * elem_bytes;
    const std::size_t b_bytes = static_cast<std::size_t>(rhs.numel()) * elem_bytes;
    const std::size_t out_bytes = static_cast<std::size_t>(out.numel()) * (is_cmp ? sizeof(std::uint8_t) : elem_bytes);
    id<MTLBuffer> a_buf = ensure_device_buffer_for_read_mm(lhs, c.device, a_bytes);
    id<MTLBuffer> b_buf = ensure_device_buffer_for_read_mm(rhs, c.device, b_bytes);
    id<MTLBuffer> out_buf = ensure_device_buffer_for_write_mm(out, c.device, out_bytes);
    if (a_buf == nil || b_buf == nil || out_buf == nil) {
      fail_("binary: failed to allocate Metal buffers");
    }
    BinaryMetaHost meta{};
    const std::uint32_t op_code = binary_op_code_(op, is_cmp);
    if (op_code == 999) {
      fail_("binary: unsupported op code on current MPS path");
    }
    const std::uint32_t mode = binary_mode_(lhs, rhs, out);
    const auto aux = make_binary_cache_aux_(op_code, mode, lhs, rhs, out);
    const GraphCacheKeyView cache_key{
        is_cmp ? (compute_dt == ScalarType::Complex64 ? "elementwise.binary_cmp_c64" : "elementwise.binary_cmp")
               : (compute_dt == ScalarType::Complex64 ? "elementwise.binary_c64" : "elementwise.binary"),
        compute_dt,
        out.sizes(),
        aux};
    if (!graph_cache_lookup(cache_key, &meta, sizeof(meta))) {
      meta.op = op_code;
      meta.mode = mode;
      meta.ndim = static_cast<std::uint32_t>(out.sizes().size());
      fill_out_sizes_(out.sizes(), meta.out_sizes, "binary");
      if (meta.mode == 3) {
        fill_broadcast_strides_(lhs, out.sizes(), meta.a_strides, "binary", "lhs");
        fill_broadcast_strides_(rhs, out.sizes(), meta.b_strides, "binary", "rhs");
      }
      graph_cache_store(cache_key, &meta, sizeof(meta));
    }

    id<MTLCommandBuffer> cb = encode_op_(
        is_cmp ? (compute_dt == ScalarType::Complex64 ? c.binary_cmp_c64 : c.binary_cmp_f32)
               : (compute_dt == ScalarType::Complex64 ? c.binary_c64 : c.binary_f32),
        a_buf,
        b_buf,
        out_buf,
        nil,
        &meta,
        sizeof(meta),
        3,
        out.numel(),
        "binary");
    if (cb != nil) {
      submit_command_buffer_mm(cb, out.numel());
      mark_device_data_dirty_mm(out, cb);
    }
  }
}

bool supports_complex_conj(ScalarType dt) noexcept {
  return dt == ScalarType::Complex64;
}

void complex_conj(const Series& a, Series& out) {
  if (!is_available()) {
    fail_("complex_conj: MPS backend unavailable (" + ctx_().error + ")");
  }
  validate_mps_tensor_(a, "complex_conj");
  validate_mps_tensor_(out, "complex_conj");
  if (!supports_complex_conj(a.scalar_type()) || out.scalar_type() != a.scalar_type()) {
    fail_("complex_conj: expected complex64 input/output");
  }
  if (a.numel() != out.numel()) {
    fail_("complex_conj: shape mismatch");
  }

  auto& c = ctx_();
  @autoreleasepool {
    const std::size_t bytes = static_cast<std::size_t>(a.numel()) * sizeof(float) * 2;
    id<MTLBuffer> in_buf = ensure_device_buffer_for_read_mm(a, c.device, bytes);
    id<MTLBuffer> out_buf = ensure_device_buffer_for_write_mm(out, c.device, bytes);
    if (in_buf == nil || out_buf == nil) {
      fail_("complex_conj: failed to allocate Metal buffers");
    }
    UnaryMetaHost meta{0};
    id<MTLCommandBuffer> cb =
        encode_op_(c.complex_conj_c64, in_buf, out_buf, nil, nil, &meta, sizeof(meta), 2, out.numel(), "complex_conj");
    if (cb != nil) {
      submit_command_buffer_mm(cb, out.numel());
      mark_device_data_dirty_mm(out, cb);
    }
  }
}

std::uint32_t complex_component_code_(ComplexComponentOp op) {
  switch (op) {
    case ComplexComponentOp::Real: return 0;
    case ComplexComponentOp::Imag: return 1;
    case ComplexComponentOp::Abs: return 2;
    case ComplexComponentOp::Angle: return 3;
    default: return 999;
  }
}

bool supports_complex_component(ScalarType in_dt, ScalarType out_dt) noexcept {
  return in_dt == ScalarType::Complex64 && out_dt == ScalarType::Float32;
}

void complex_component(ComplexComponentOp op, const Series& a, Series& out) {
  if (!is_available()) {
    fail_("complex_component: MPS backend unavailable (" + ctx_().error + ")");
  }
  validate_mps_tensor_(a, "complex_component");
  validate_mps_tensor_(out, "complex_component");
  if (!supports_complex_component(a.scalar_type(), out.scalar_type())) {
    fail_("complex_component: expected complex64 input and float32 output");
  }
  if (a.numel() != out.numel()) {
    fail_("complex_component: shape mismatch");
  }

  auto& c = ctx_();
  @autoreleasepool {
    const std::size_t in_bytes = static_cast<std::size_t>(a.numel()) * sizeof(float) * 2;
    const std::size_t out_bytes = static_cast<std::size_t>(out.numel()) * sizeof(float);
    id<MTLBuffer> in_buf = ensure_device_buffer_for_read_mm(a, c.device, in_bytes);
    id<MTLBuffer> out_buf = ensure_device_buffer_for_write_mm(out, c.device, out_bytes);
    if (in_buf == nil || out_buf == nil) {
      fail_("complex_component: failed to allocate Metal buffers");
    }
    UnaryMetaHost meta{complex_component_code_(op)};
    if (meta.op == 999) {
      fail_("complex_component: unsupported component op");
    }
    id<MTLCommandBuffer> cb = encode_op_(
        c.complex_component_c64,
        in_buf,
        out_buf,
        nil,
        nil,
        &meta,
        sizeof(meta),
        2,
        out.numel(),
        "complex_component");
    if (cb != nil) {
      submit_command_buffer_mm(cb, out.numel());
      mark_device_data_dirty_mm(out, cb);
    }
  }
}

bool supports_complex_from_parts(ScalarType real_dt, ScalarType imag_dt, ScalarType out_dt) noexcept {
  return real_dt == ScalarType::Float32 && imag_dt == ScalarType::Float32 && out_dt == ScalarType::Complex64;
}

void complex_from_parts(const Series& real, const Series& imag, Series& out) {
  if (!is_available()) {
    fail_("complex_from_parts: MPS backend unavailable (" + ctx_().error + ")");
  }
  validate_mps_tensor_(real, "complex_from_parts");
  validate_mps_tensor_(imag, "complex_from_parts");
  validate_mps_tensor_(out, "complex_from_parts");
  if (!supports_complex_from_parts(real.scalar_type(), imag.scalar_type(), out.scalar_type())) {
    fail_("complex_from_parts: expected float32 real/imag inputs and complex64 output");
  }
  if (real.numel() != imag.numel() || real.numel() != out.numel()) {
    fail_("complex_from_parts: shape mismatch");
  }

  auto& c = ctx_();
  @autoreleasepool {
    const std::size_t in_bytes = static_cast<std::size_t>(real.numel()) * sizeof(float);
    const std::size_t out_bytes = static_cast<std::size_t>(out.numel()) * sizeof(float) * 2u;
    id<MTLBuffer> real_buf = ensure_device_buffer_for_read_mm(real, c.device, in_bytes);
    id<MTLBuffer> imag_buf = ensure_device_buffer_for_read_mm(imag, c.device, in_bytes);
    id<MTLBuffer> out_buf = ensure_device_buffer_for_write_mm(out, c.device, out_bytes);
    if (real_buf == nil || imag_buf == nil || out_buf == nil) {
      fail_("complex_from_parts: failed to allocate Metal buffers");
    }
    UnaryMetaHost meta{0};
    id<MTLCommandBuffer> cb = encode_op_(
        c.complex_from_parts_c64,
        real_buf,
        imag_buf,
        out_buf,
        nil,
        &meta,
        sizeof(meta),
        3,
        out.numel(),
        "complex_from_parts");
    if (cb != nil) {
      submit_command_buffer_mm(cb, out.numel());
      mark_device_data_dirty_mm(out, cb);
    }
  }
}

namespace {

void maxmin_impl_(const char* op_name,
                  id<MTLComputePipelineState> pso,
                  const Series& a,
                  const Series& b,
                  Series& out) {
  if (!is_available()) {
    fail_(std::string(op_name) + ": MPS backend unavailable");
  }
  validate_mps_tensor_(a, op_name);
  validate_mps_tensor_(b, op_name);
  validate_mps_tensor_(out, op_name);
  if (!supports_maximum_minimum(a.scalar_type()) ||
      a.scalar_type() != b.scalar_type() ||
      out.scalar_type() != a.scalar_type()) {
    fail_(std::string(op_name) + ": unsupported dtype combination on current MPS path");
  }
  if (out.numel() < 0) {
    fail_(std::string(op_name) + ": invalid output numel");
  }

  auto& c = ctx_();
  @autoreleasepool {
    const std::size_t a_bytes = static_cast<std::size_t>(a.numel()) * sizeof(float);
    const std::size_t b_bytes = static_cast<std::size_t>(b.numel()) * sizeof(float);
    const std::size_t out_bytes = static_cast<std::size_t>(out.numel()) * sizeof(float);
    id<MTLBuffer> a_buf = ensure_device_buffer_for_read_mm(a, c.device, a_bytes);
    id<MTLBuffer> b_buf = ensure_device_buffer_for_read_mm(b, c.device, b_bytes);
    id<MTLBuffer> out_buf = ensure_device_buffer_for_write_mm(out, c.device, out_bytes);
    if (a_buf == nil || b_buf == nil || out_buf == nil) {
      fail_(std::string(op_name) + ": failed to allocate Metal buffers");
    }
    BinaryMetaHost meta{};
    const std::uint32_t mode = binary_mode_(a, b, out);
    const auto aux = make_binary_cache_aux_(0u, mode, a, b, out);
    const GraphCacheKeyView cache_key{op_name, ScalarType::Float32, out.sizes(), aux};
    if (!graph_cache_lookup(cache_key, &meta, sizeof(meta))) {
      meta.op = 0;
      meta.mode = mode;
      meta.ndim = static_cast<std::uint32_t>(out.sizes().size());
      fill_out_sizes_(out.sizes(), meta.out_sizes, op_name);
      if (meta.mode == 3) {
        fill_broadcast_strides_(a, out.sizes(), meta.a_strides, op_name, "lhs");
        fill_broadcast_strides_(b, out.sizes(), meta.b_strides, op_name, "rhs");
      }
      graph_cache_store(cache_key, &meta, sizeof(meta));
    }

    id<MTLCommandBuffer> cb =
        encode_op_(pso, a_buf, b_buf, out_buf, nil, &meta, sizeof(meta), 3, out.numel(), op_name);
    if (cb != nil) {
      submit_command_buffer_mm(cb, out.numel());
      mark_device_data_dirty_mm(out, cb);
    }
  }
}

} // namespace

void maximum(const Series& a, const Series& b, Series& out) {
  auto& c = ctx_();
  maxmin_impl_("maximum", c.maximum_f32, a, b, out);
}

void minimum(const Series& a, const Series& b, Series& out) {
  auto& c = ctx_();
  maxmin_impl_("minimum", c.minimum_f32, a, b, out);
}

bool supports_where(ScalarType cond_dt, ScalarType data_dt) noexcept {
  return cond_dt == ScalarType::Bool && (data_dt == ScalarType::Float32 || data_dt == ScalarType::Complex64);
}

void where(const Series& cond, const Series& a, const Series& b, Series& out) {
  if (!is_available()) {
    fail_("where: MPS backend unavailable");
  }
  validate_mps_tensor_(cond, "where");
  validate_mps_tensor_(a, "where");
  validate_mps_tensor_(b, "where");
  validate_mps_tensor_(out, "where");
  if (!supports_where(cond.scalar_type(), a.scalar_type()) || a.scalar_type() != b.scalar_type() ||
      out.scalar_type() != a.scalar_type()) {
    fail_("where: unsupported dtype combination on current MPS path");
  }

  auto& c = ctx_();
  @autoreleasepool {
    const bool complex_data = a.scalar_type() == ScalarType::Complex64;
    const std::size_t elem_bytes = complex_data ? sizeof(float) * 2u : sizeof(float);
    const std::size_t cond_bytes = static_cast<std::size_t>(cond.numel()) * sizeof(std::uint8_t);
    const std::size_t a_bytes = static_cast<std::size_t>(a.numel()) * elem_bytes;
    const std::size_t b_bytes = static_cast<std::size_t>(b.numel()) * elem_bytes;
    const std::size_t out_bytes = static_cast<std::size_t>(out.numel()) * elem_bytes;
    id<MTLBuffer> cond_buf = ensure_device_buffer_for_read_mm(cond, c.device, cond_bytes);
    id<MTLBuffer> a_buf = ensure_device_buffer_for_read_mm(a, c.device, a_bytes);
    id<MTLBuffer> b_buf = ensure_device_buffer_for_read_mm(b, c.device, b_bytes);
    id<MTLBuffer> out_buf = ensure_device_buffer_for_write_mm(out, c.device, out_bytes);
    if (cond_buf == nil || a_buf == nil || b_buf == nil || out_buf == nil) {
      fail_("where: failed to allocate Metal buffers");
    }
    TernaryMetaHost meta{};
    const std::uint32_t mode = ternary_mode_(cond, a, b, out);
    const auto aux = make_where_cache_aux_(mode, cond, a, b, out);
    const GraphCacheKeyView cache_key{complex_data ? "elementwise.where_c64" : "elementwise.where", a.scalar_type(), out.sizes(), aux};
    if (!graph_cache_lookup(cache_key, &meta, sizeof(meta))) {
      meta.mode = mode;
      meta.ndim = static_cast<std::uint32_t>(out.sizes().size());
      fill_out_sizes_(out.sizes(), meta.out_sizes, "where");
      if (meta.mode != 0) {
        fill_broadcast_strides_(cond, out.sizes(), meta.cond_strides, "where", "condition");
        fill_broadcast_strides_(a, out.sizes(), meta.a_strides, "where", "lhs");
        fill_broadcast_strides_(b, out.sizes(), meta.b_strides, "where", "rhs");
      }
      graph_cache_store(cache_key, &meta, sizeof(meta));
    }

    id<MTLCommandBuffer> cb = encode_op_(
        complex_data ? c.where_c64 : c.where_f32,
        cond_buf,
        a_buf,
        b_buf,
        out_buf,
        &meta,
        sizeof(meta),
        4,
        out.numel(),
        "where");
    if (cb != nil) {
      submit_command_buffer_mm(cb, out.numel());
      mark_device_data_dirty_mm(out, cb);
    }
  }
}

bool supports_unary(cpu::UnaryOpId op, ScalarType dt) noexcept {
  if (dt != ScalarType::Float32) {
    return false;
  }
  using U = cpu::UnaryOpId;
  switch (op) {
    case U::Neg:
    case U::Abs:
    case U::Log1p:
    case U::Expm1:
      return true;
    default:
      return false;
  }
}

void unary(cpu::UnaryOpId op, const Series& a, Series& out) {
  if (!is_available()) {
    fail_("unary: MPS backend unavailable");
  }
  validate_mps_tensor_(a, "unary");
  validate_mps_tensor_(out, "unary");
  if (a.scalar_type() != ScalarType::Float32 || out.scalar_type() != ScalarType::Float32) {
    fail_("unary: only float32 is supported on current MPS path");
  }
  if (!supports_unary(op, a.scalar_type())) {
    fail_("unary: unsupported op on current MPS path");
  }
  if (a.numel() != out.numel()) {
    fail_("unary: shape mismatch on current MPS path");
  }

  auto& c = ctx_();
  @autoreleasepool {
    const std::size_t in_bytes = static_cast<std::size_t>(a.numel()) * sizeof(float);
    const std::size_t out_bytes = static_cast<std::size_t>(out.numel()) * sizeof(float);
    id<MTLBuffer> in_buf = ensure_device_buffer_for_read_mm(a, c.device, in_bytes);
    id<MTLBuffer> out_buf = ensure_device_buffer_for_write_mm(out, c.device, out_bytes);
    if (in_buf == nil || out_buf == nil) {
      fail_("unary: failed to allocate Metal buffers");
    }
    UnaryMetaHost meta{unary_op_code_(op)};
    if (meta.op == 999) {
      fail_("unary: unsupported op code on current MPS path");
    }
    id<MTLCommandBuffer> cb = encode_op_(c.unary_f32, in_buf, out_buf, nil, nil, &meta, sizeof(meta), 2, out.numel(), "unary");
    if (cb != nil) {
      submit_command_buffer_mm(cb, out.numel());
      mark_device_data_dirty_mm(out, cb);
    }
  }
}

bool supports_unary_math(UnaryMathOp op, ScalarType dt) noexcept {
  if (dt != ScalarType::Float32) {
    return false;
  }
  switch (op) {
    case UnaryMathOp::Exp:
    case UnaryMathOp::Log:
    case UnaryMathOp::Sqrt:
    case UnaryMathOp::Sin:
    case UnaryMathOp::Cos:
    case UnaryMathOp::Tanh:
    case UnaryMathOp::Tan:
    case UnaryMathOp::Asin:
    case UnaryMathOp::Acos:
    case UnaryMathOp::Atan:
    case UnaryMathOp::Sinh:
    case UnaryMathOp::Cosh:
    case UnaryMathOp::Reciprocal:
    case UnaryMathOp::Floor:
    case UnaryMathOp::Ceil:
    case UnaryMathOp::Round:
    case UnaryMathOp::Trunc:
    case UnaryMathOp::Relu:
    case UnaryMathOp::Sigmoid:
      return true;
    default:
      return false;
  }
}

void unary_math(UnaryMathOp op, const Series& a, Series& out) {
  if (!is_available()) {
    fail_("unary_math: MPS backend unavailable");
  }
  validate_mps_tensor_(a, "unary_math");
  validate_mps_tensor_(out, "unary_math");
  if (a.scalar_type() != ScalarType::Float32 || out.scalar_type() != ScalarType::Float32) {
    fail_("unary_math: only float32 is supported on current MPS path");
  }
  if (!supports_unary_math(op, a.scalar_type())) {
    fail_("unary_math: unsupported op on current MPS path");
  }
  if (a.numel() != out.numel()) {
    fail_("unary_math: shape mismatch on current MPS path");
  }

  auto& c = ctx_();
  @autoreleasepool {
    const std::size_t in_bytes = static_cast<std::size_t>(a.numel()) * sizeof(float);
    const std::size_t out_bytes = static_cast<std::size_t>(out.numel()) * sizeof(float);
    id<MTLBuffer> in_buf = ensure_device_buffer_for_read_mm(a, c.device, in_bytes);
    id<MTLBuffer> out_buf = ensure_device_buffer_for_write_mm(out, c.device, out_bytes);
    if (in_buf == nil || out_buf == nil) {
      fail_("unary_math: failed to allocate Metal buffers");
    }
    UnaryMetaHost meta{unary_math_code_(op)};
    if (meta.op == 999) {
      fail_("unary_math: unsupported op code on current MPS path");
    }
    id<MTLCommandBuffer> cb = encode_op_(c.unary_f32, in_buf, out_buf, nil, nil, &meta, sizeof(meta), 2, out.numel(), "unary_math");
    if (cb != nil) {
      submit_command_buffer_mm(cb, out.numel());
      mark_device_data_dirty_mm(out, cb);
    }
  }
}

bool supports_unary_pred(UnaryPredOp op, ScalarType dt) noexcept {
  if (dt != ScalarType::Float32) {
    return false;
  }
  switch (op) {
    case UnaryPredOp::IsNan:
    case UnaryPredOp::IsInf:
    case UnaryPredOp::IsFinite:
      return true;
    default:
      return false;
  }
}

void unary_pred(UnaryPredOp op, const Series& a, Series& out) {
  if (!is_available()) {
    fail_("unary_pred: MPS backend unavailable");
  }
  validate_mps_tensor_(a, "unary_pred");
  validate_mps_tensor_(out, "unary_pred");
  if (a.scalar_type() != ScalarType::Float32) {
    fail_("unary_pred: only float32 input is supported on current MPS path");
  }
  if (out.scalar_type() != ScalarType::Bool) {
    fail_("unary_pred: output must be bool on current MPS path");
  }
  if (!supports_unary_pred(op, a.scalar_type())) {
    fail_("unary_pred: unsupported op on current MPS path");
  }
  if (a.numel() != out.numel()) {
    fail_("unary_pred: shape mismatch on current MPS path");
  }

  auto& c = ctx_();
  @autoreleasepool {
    const std::size_t in_bytes = static_cast<std::size_t>(a.numel()) * sizeof(float);
    const std::size_t out_bytes = static_cast<std::size_t>(out.numel()) * sizeof(std::uint8_t);
    id<MTLBuffer> in_buf = ensure_device_buffer_for_read_mm(a, c.device, in_bytes);
    id<MTLBuffer> out_buf = ensure_device_buffer_for_write_mm(out, c.device, out_bytes);
    if (in_buf == nil || out_buf == nil) {
      fail_("unary_pred: failed to allocate Metal buffers");
    }
    UnaryMetaHost meta{unary_pred_code_(op)};
    if (meta.op == 999) {
      fail_("unary_pred: unsupported op code on current MPS path");
    }
    id<MTLCommandBuffer> cb = encode_op_(c.unary_pred_f32, in_buf, out_buf, nil, nil, &meta, sizeof(meta), 2, out.numel(), "unary_pred");
    if (cb != nil) {
      submit_command_buffer_mm(cb, out.numel());
      mark_device_data_dirty_mm(out, cb);
    }
  }
}

} // namespace pluss::codem::native::mps
