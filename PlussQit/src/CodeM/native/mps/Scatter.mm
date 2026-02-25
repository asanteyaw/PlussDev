#include "CodeM/native/mps/Scatter.h"
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
constant ulong kBadOffset = ~ulong(0);

struct ScatterMeta {
  uint dim;
  uint ndim;
  ulong out_sizes[kMaxRank];
  ulong out_strides[kMaxRank];
  ulong src_sizes[kMaxRank];
};

inline ulong out_offset_from_src_gid(ulong gid, constant ScatterMeta& m, const device long* index) {
  ulong rem = gid;
  ulong out_off = 0;
  for (int d = static_cast<int>(m.ndim) - 1; d >= 0; --d) {
    const ulong sz = m.src_sizes[d];
    ulong coord = (sz == 0) ? 0 : (rem % sz);
    rem = (sz == 0) ? 0 : (rem / sz);
    if (static_cast<uint>(d) == m.dim) {
      long pick = index[gid];
      const long dim_sz = static_cast<long>(m.out_sizes[m.dim]);
      if (pick < 0) pick += dim_sz;
      if (pick < 0 || pick >= dim_sz) return kBadOffset;
      coord = static_cast<ulong>(pick);
    }
    out_off += coord * m.out_strides[d];
  }
  return out_off;
}

inline void atomic_add_f32(device atomic_uint* addr, float v) {
  uint old_bits = atomic_load_explicit(addr, memory_order_relaxed);
  while (true) {
    const float cur = as_type<float>(old_bits);
    const uint next_bits = as_type<uint>(cur + v);
    if (atomic_compare_exchange_weak_explicit(addr, &old_bits, next_bits,
                                              memory_order_relaxed, memory_order_relaxed)) {
      break;
    }
  }
}

inline void atomic_amax_f32(device atomic_uint* addr, float v) {
  uint old_bits = atomic_load_explicit(addr, memory_order_relaxed);
  while (true) {
    const float cur = as_type<float>(old_bits);
    if (!(v > cur)) break;
    const uint next_bits = as_type<uint>(v);
    if (atomic_compare_exchange_weak_explicit(addr, &old_bits, next_bits,
                                              memory_order_relaxed, memory_order_relaxed)) {
      break;
    }
  }
}

inline void atomic_amin_f32(device atomic_uint* addr, float v) {
  uint old_bits = atomic_load_explicit(addr, memory_order_relaxed);
  while (true) {
    const float cur = as_type<float>(old_bits);
    if (!(v < cur)) break;
    const uint next_bits = as_type<uint>(v);
    if (atomic_compare_exchange_weak_explicit(addr, &old_bits, next_bits,
                                              memory_order_relaxed, memory_order_relaxed)) {
      break;
    }
  }
}

kernel void scatter_set_f32_i64(
    const device float* src [[buffer(0)]],
    const device long* index [[buffer(1)]],
    device float* out [[buffer(2)]],
    constant ScatterMeta& meta [[buffer(3)]],
    uint gid [[thread_position_in_grid]]) {
  const ulong off = out_offset_from_src_gid(static_cast<ulong>(gid), meta, index);
  if (off == kBadOffset) return;
  out[off] = src[gid];
}

kernel void scatter_add_f32_i64(
    const device float* src [[buffer(0)]],
    const device long* index [[buffer(1)]],
    device float* out [[buffer(2)]],
    constant ScatterMeta& meta [[buffer(3)]],
    uint gid [[thread_position_in_grid]]) {
  const ulong off = out_offset_from_src_gid(static_cast<ulong>(gid), meta, index);
  if (off == kBadOffset) return;
  device atomic_uint* out_bits = reinterpret_cast<device atomic_uint*>(out);
  atomic_add_f32(out_bits + off, src[gid]);
}

kernel void scatter_amax_f32_i64(
    const device float* src [[buffer(0)]],
    const device long* index [[buffer(1)]],
    device float* out [[buffer(2)]],
    constant ScatterMeta& meta [[buffer(3)]],
    uint gid [[thread_position_in_grid]]) {
  const ulong off = out_offset_from_src_gid(static_cast<ulong>(gid), meta, index);
  if (off == kBadOffset) return;
  device atomic_uint* out_bits = reinterpret_cast<device atomic_uint*>(out);
  atomic_amax_f32(out_bits + off, src[gid]);
}

kernel void scatter_amin_f32_i64(
    const device float* src [[buffer(0)]],
    const device long* index [[buffer(1)]],
    device float* out [[buffer(2)]],
    constant ScatterMeta& meta [[buffer(3)]],
    uint gid [[thread_position_in_grid]]) {
  const ulong off = out_offset_from_src_gid(static_cast<ulong>(gid), meta, index);
  if (off == kBadOffset) return;
  device atomic_uint* out_bits = reinterpret_cast<device atomic_uint*>(out);
  atomic_amin_f32(out_bits + off, src[gid]);
}
)metal";

struct ScatterMetaHost {
  std::uint32_t dim{0};
  std::uint32_t ndim{0};
  std::array<std::uint64_t, kMaxRank> out_sizes{};
  std::array<std::uint64_t, kMaxRank> out_strides{};
  std::array<std::uint64_t, kMaxRank> src_sizes{};
};

struct Context {
  bool ready{false};
  id<MTLDevice> device{nil};
  id<MTLCommandQueue> queue{nil};
  id<MTLLibrary> library{nil};
  id<MTLComputePipelineState> scatter_set_f32_i64{nil};
  id<MTLComputePipelineState> scatter_add_f32_i64{nil};
  id<MTLComputePipelineState> scatter_amax_f32_i64{nil};
  id<MTLComputePipelineState> scatter_amin_f32_i64{nil};
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
        ctx.error = "MPS scatter init failed: ";
        ctx.error += runtime_error_cstr();
        return;
      }
      ctx.queue = shared_queue_mm();
      if (ctx.queue == nil) {
        ctx.error = "MPS scatter init failed: no shared Metal command queue";
        return;
      }

      NSError* err = nil;
      NSString* src = [NSString stringWithUTF8String:kShaderSrc];
      ctx.library = [ctx.device newLibraryWithSource:src options:nil error:&err];
      if (ctx.library == nil) {
        ctx.error = "MPS scatter init failed: shader compile error";
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
          ctx.error = "MPS scatter init failed: missing kernel ";
          ctx.error += fn_name;
          return false;
        }
        *out = [ctx.device newComputePipelineStateWithFunction:fn error:&p_err];
        if (*out == nil) {
          ctx.error = "MPS scatter init failed: pipeline creation error for ";
          ctx.error += fn_name;
          if (p_err != nil && p_err.localizedDescription != nil) {
            ctx.error += ": ";
            ctx.error += std::string([p_err.localizedDescription UTF8String]);
          }
          return false;
        }
        return true;
      };

      if (!make_pipeline("scatter_set_f32_i64", &ctx.scatter_set_f32_i64)) return;
      if (!make_pipeline("scatter_add_f32_i64", &ctx.scatter_add_f32_i64)) return;
      if (!make_pipeline("scatter_amax_f32_i64", &ctx.scatter_amax_f32_i64)) return;
      if (!make_pipeline("scatter_amin_f32_i64", &ctx.scatter_amin_f32_i64)) return;
      ctx.ready = true;
    }
  });
  return ctx;
}

void validate_inputs_(const Series& out,
                      std::int64_t dim,
                      const Series& index,
                      const Series& src,
                      ScatterMetaHost& meta,
                      const char* op) {
  if (!out.defined() || !index.defined() || !src.defined()) {
    fail_(std::string(op) + ": undefined Series");
  }
  if (out.platform() != Platform::MPS || index.platform() != Platform::MPS || src.platform() != Platform::MPS) {
    fail_(std::string(op) + ": expected MPS Series");
  }
  if (!out.is_contiguous() || !index.is_contiguous() || !src.is_contiguous()) {
    fail_(std::string(op) + ": requires contiguous Series");
  }
  if (out.scalar_type() != ScalarType::Float32 || src.scalar_type() != ScalarType::Float32) {
    fail_(std::string(op) + ": only float32 is supported on current MPS path");
  }
  if (index.scalar_type() != ScalarType::Int64) {
    fail_(std::string(op) + ": index must be int64");
  }

  const auto out_sizes = out.sizes();
  const auto out_strides = out.strides();
  const auto idx_sizes = index.sizes();
  const auto src_sizes = src.sizes();
  if (out_sizes.size() != idx_sizes.size() || idx_sizes.size() != src_sizes.size()) {
    fail_(std::string(op) + ": rank mismatch");
  }
  if (out_sizes.size() > kMaxRank) {
    fail_(std::string(op) + ": rank > 8 unsupported on current MPS path");
  }
  if (out_sizes.size() != out_strides.size()) {
    fail_(std::string(op) + ": invalid output strides");
  }

  const auto nd = static_cast<std::int64_t>(out_sizes.size());
  const auto d = normalize_dim_(dim, nd, op);
  for (std::int64_t i = 0; i < nd; ++i) {
    if (idx_sizes[static_cast<std::size_t>(i)] != src_sizes[static_cast<std::size_t>(i)]) {
      fail_(std::string(op) + ": index/src shape mismatch");
    }
    if (i != d && idx_sizes[static_cast<std::size_t>(i)] != out_sizes[static_cast<std::size_t>(i)]) {
      fail_(std::string(op) + ": index shape mismatch (non-scatter dims must match output)");
    }
  }

  sync_host_if_needed(index);
  const auto* idx_ptr = reinterpret_cast<const std::int64_t*>(index.raw_data());
  const auto dim_size = out_sizes[static_cast<std::size_t>(d)];
  for (std::int64_t i = 0; i < index.numel(); ++i) {
    const auto v = idx_ptr[i];
    if (v < -dim_size || v >= dim_size) {
      fail_(std::string(op) + ": index out of range");
    }
  }

  meta.dim = static_cast<std::uint32_t>(d);
  meta.ndim = static_cast<std::uint32_t>(out_sizes.size());
  meta.out_sizes.fill(1);
  meta.out_strides.fill(0);
  meta.src_sizes.fill(1);
  for (std::size_t i = 0; i < out_sizes.size(); ++i) {
    if (out_sizes[i] < 0 || out_strides[i] < 0 || src_sizes[i] < 0) {
      fail_(std::string(op) + ": negative size/stride unsupported");
    }
    meta.out_sizes[i] = static_cast<std::uint64_t>(out_sizes[i]);
    meta.out_strides[i] = static_cast<std::uint64_t>(out_strides[i]);
    meta.src_sizes[i] = static_cast<std::uint64_t>(src_sizes[i]);
  }
}

void encode_scatter_(id<MTLComputePipelineState> pso,
                     const Series& out,
                     const Series& index,
                     const Series& src,
                     const ScatterMetaHost& meta,
                     const char* op_name) {
  auto& c = ctx_();
  @autoreleasepool {
    const std::size_t out_bytes = static_cast<std::size_t>(out.numel()) * sizeof(float);
    const std::size_t idx_bytes = static_cast<std::size_t>(index.numel()) * sizeof(std::int64_t);
    const std::size_t src_bytes = static_cast<std::size_t>(src.numel()) * sizeof(float);
    id<MTLBuffer> out_buf = ensure_device_buffer_for_read_mm(out, c.device, out_bytes);
    id<MTLBuffer> idx_buf = ensure_device_buffer_for_read_mm(index, c.device, idx_bytes);
    id<MTLBuffer> src_buf = ensure_device_buffer_for_read_mm(src, c.device, src_bytes);
    if (out_buf == nil || idx_buf == nil || src_buf == nil) {
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
    [enc setBuffer:src_buf offset:0 atIndex:0];
    [enc setBuffer:idx_buf offset:0 atIndex:1];
    [enc setBuffer:out_buf offset:0 atIndex:2];
    [enc setBytes:&meta length:sizeof(meta) atIndex:3];

    const NSUInteger grid_n = static_cast<NSUInteger>(src.numel());
    NSUInteger tg = pso.maxTotalThreadsPerThreadgroup;
    if (tg == 0) tg = 64;
    tg = std::min<NSUInteger>(tg, 256);
    [enc dispatchThreads:MTLSizeMake(grid_n, 1, 1) threadsPerThreadgroup:MTLSizeMake(tg, 1, 1)];
    [enc endEncoding];
    submit_command_buffer_mm(cb, src.numel());
    mark_device_data_dirty_mm(out, cb);
  }
}

} // namespace

bool supports_scatter(ScalarType out_dt, ScalarType index_dt, ScalarType src_dt) noexcept {
  return out_dt == ScalarType::Float32 && src_dt == ScalarType::Float32 && index_dt == ScalarType::Int64;
}

bool supports_scatter_reduce(ScalarType out_dt, ScalarType index_dt, ScalarType src_dt) noexcept {
  return out_dt == ScalarType::Float32 && src_dt == ScalarType::Float32 && index_dt == ScalarType::Int64;
}

void scatter_set_(Series& out, std::int64_t dim, const Series& index, const Series& src) {
  auto& c = ctx_();
  if (!c.ready) {
    fail_("mps::scatter_set_: backend unavailable (" + c.error + ")");
  }
  if (src.numel() == 0) return;

  ScatterMetaHost meta;
  validate_inputs_(out, dim, index, src, meta, "mps::scatter_set_");
  encode_scatter_(c.scatter_set_f32_i64, out, index, src, meta, "mps::scatter_set_");
}

void scatter_reduce_(Series& out, std::int64_t dim, const Series& index, const Series& src, ScatterReduceOp op) {
  auto& c = ctx_();
  if (!c.ready) {
    fail_("mps::scatter_reduce_: backend unavailable (" + c.error + ")");
  }
  if (src.numel() == 0) return;

  ScatterMetaHost meta;
  validate_inputs_(out, dim, index, src, meta, "mps::scatter_reduce_");
  id<MTLComputePipelineState> pso = nil;
  const char* name = "mps::scatter_reduce_";
  switch (op) {
    case ScatterReduceOp::Sum: pso = c.scatter_add_f32_i64; name = "mps::scatter_reduce_sum_"; break;
    case ScatterReduceOp::AMax: pso = c.scatter_amax_f32_i64; name = "mps::scatter_reduce_amax_"; break;
    case ScatterReduceOp::AMin: pso = c.scatter_amin_f32_i64; name = "mps::scatter_reduce_amin_"; break;
    default: fail_("mps::scatter_reduce_: unsupported reduce op");
  }
  encode_scatter_(pso, out, index, src, meta, name);
}

} // namespace pluss::codem::native::mps
