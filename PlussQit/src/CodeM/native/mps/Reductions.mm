#include "CodeM/native/mps/Reductions.h"
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

struct ReduceMeta {
  uint dim;
  uint in_ndim;
  uint out_ndim;
  ulong in_sizes[kMaxRank];
  ulong in_strides[kMaxRank];
  ulong out_sizes[kMaxRank];
};

inline ulong in_base_from_out_gid(uint gid, constant ReduceMeta& m) {
  ulong rem = static_cast<ulong>(gid);
  ulong base = 0;
  for (int od = static_cast<int>(m.out_ndim) - 1; od >= 0; --od) {
    const ulong sz = m.out_sizes[od];
    const ulong idx = (sz == 0) ? 0 : (rem % sz);
    rem = (sz == 0) ? 0 : (rem / sz);
    const uint in_d = (static_cast<uint>(od) < m.dim) ? static_cast<uint>(od) : static_cast<uint>(od + 1);
    base += idx * m.in_strides[in_d];
  }
  return base;
}

kernel void sum_dim_f32(
    const device float* in [[buffer(0)]],
    device float* out [[buffer(1)]],
    constant ReduceMeta& meta [[buffer(2)]],
    uint gid [[thread_position_in_grid]]) {
  const ulong base = in_base_from_out_gid(gid, meta);
  const ulong rsz = meta.in_sizes[meta.dim];
  const ulong rstep = meta.in_strides[meta.dim];
  float acc = 0.0f;
  for (ulong r = 0; r < rsz; ++r) {
    acc += in[base + r * rstep];
  }
  out[gid] = acc;
}

kernel void prod_dim_f32(
    const device float* in [[buffer(0)]],
    device float* out [[buffer(1)]],
    constant ReduceMeta& meta [[buffer(2)]],
    uint gid [[thread_position_in_grid]]) {
  const ulong base = in_base_from_out_gid(gid, meta);
  const ulong rsz = meta.in_sizes[meta.dim];
  const ulong rstep = meta.in_strides[meta.dim];
  float acc = 1.0f;
  for (ulong r = 0; r < rsz; ++r) {
    acc *= in[base + r * rstep];
  }
  out[gid] = acc;
}

kernel void amax_dim_f32(
    const device float* in [[buffer(0)]],
    device float* out [[buffer(1)]],
    constant ReduceMeta& meta [[buffer(2)]],
    uint gid [[thread_position_in_grid]]) {
  const ulong base = in_base_from_out_gid(gid, meta);
  const ulong rsz = meta.in_sizes[meta.dim];
  const ulong rstep = meta.in_strides[meta.dim];
  float acc = -INFINITY;
  for (ulong r = 0; r < rsz; ++r) {
    acc = fmax(acc, in[base + r * rstep]);
  }
  out[gid] = acc;
}

kernel void amin_dim_f32(
    const device float* in [[buffer(0)]],
    device float* out [[buffer(1)]],
    constant ReduceMeta& meta [[buffer(2)]],
    uint gid [[thread_position_in_grid]]) {
  const ulong base = in_base_from_out_gid(gid, meta);
  const ulong rsz = meta.in_sizes[meta.dim];
  const ulong rstep = meta.in_strides[meta.dim];
  float acc = INFINITY;
  for (ulong r = 0; r < rsz; ++r) {
    acc = fmin(acc, in[base + r * rstep]);
  }
  out[gid] = acc;
}

kernel void all_dim_u8(
    const device uchar* in [[buffer(0)]],
    device uchar* out [[buffer(1)]],
    constant ReduceMeta& meta [[buffer(2)]],
    uint gid [[thread_position_in_grid]]) {
  const ulong base = in_base_from_out_gid(gid, meta);
  const ulong rsz = meta.in_sizes[meta.dim];
  const ulong rstep = meta.in_strides[meta.dim];
  uchar acc = uchar(1);
  for (ulong r = 0; r < rsz; ++r) {
    if (in[base + r * rstep] == uchar(0)) {
      acc = uchar(0);
      break;
    }
  }
  out[gid] = acc;
}

kernel void any_dim_u8(
    const device uchar* in [[buffer(0)]],
    device uchar* out [[buffer(1)]],
    constant ReduceMeta& meta [[buffer(2)]],
    uint gid [[thread_position_in_grid]]) {
  const ulong base = in_base_from_out_gid(gid, meta);
  const ulong rsz = meta.in_sizes[meta.dim];
  const ulong rstep = meta.in_strides[meta.dim];
  uchar acc = uchar(0);
  for (ulong r = 0; r < rsz; ++r) {
    if (in[base + r * rstep] != uchar(0)) {
      acc = uchar(1);
      break;
    }
  }
  out[gid] = acc;
}

kernel void argmax_dim_f32(
    const device float* in [[buffer(0)]],
    device long* out [[buffer(1)]],
    constant ReduceMeta& meta [[buffer(2)]],
    uint gid [[thread_position_in_grid]]) {
  const ulong base = in_base_from_out_gid(gid, meta);
  const ulong rsz = meta.in_sizes[meta.dim];
  const ulong rstep = meta.in_strides[meta.dim];
  float best = -INFINITY;
  long arg = 0;
  for (ulong r = 0; r < rsz; ++r) {
    const float v = in[base + r * rstep];
    if (v > best) {
      best = v;
      arg = static_cast<long>(r);
    }
  }
  out[gid] = arg;
}

kernel void argmin_dim_f32(
    const device float* in [[buffer(0)]],
    device long* out [[buffer(1)]],
    constant ReduceMeta& meta [[buffer(2)]],
    uint gid [[thread_position_in_grid]]) {
  const ulong base = in_base_from_out_gid(gid, meta);
  const ulong rsz = meta.in_sizes[meta.dim];
  const ulong rstep = meta.in_strides[meta.dim];
  float best = INFINITY;
  long arg = 0;
  for (ulong r = 0; r < rsz; ++r) {
    const float v = in[base + r * rstep];
    if (v < best) {
      best = v;
      arg = static_cast<long>(r);
    }
  }
  out[gid] = arg;
}
)metal";

struct ReduceMetaHost {
  std::uint32_t dim{0};
  std::uint32_t in_ndim{0};
  std::uint32_t out_ndim{0};
  std::array<std::uint64_t, kMaxRank> in_sizes{};
  std::array<std::uint64_t, kMaxRank> in_strides{};
  std::array<std::uint64_t, kMaxRank> out_sizes{};
};

struct Context {
  bool ready{false};
  id<MTLDevice> device{nil};
  id<MTLCommandQueue> queue{nil};
  id<MTLLibrary> library{nil};
  id<MTLComputePipelineState> sum_dim_f32{nil};
  id<MTLComputePipelineState> prod_dim_f32{nil};
  id<MTLComputePipelineState> amax_dim_f32{nil};
  id<MTLComputePipelineState> amin_dim_f32{nil};
  id<MTLComputePipelineState> all_dim_u8{nil};
  id<MTLComputePipelineState> any_dim_u8{nil};
  id<MTLComputePipelineState> argmax_dim_f32{nil};
  id<MTLComputePipelineState> argmin_dim_f32{nil};
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
        ctx.error = "MPS reductions init failed: ";
        ctx.error += runtime_error_cstr();
        return;
      }
      ctx.queue = shared_queue_mm();
      if (ctx.queue == nil) {
        ctx.error = "MPS reductions init failed: no shared Metal command queue";
        return;
      }

      NSError* err = nil;
      NSString* src = [NSString stringWithUTF8String:kShaderSrc];
      ctx.library = [ctx.device newLibraryWithSource:src options:nil error:&err];
      if (ctx.library == nil) {
        ctx.error = "MPS reductions init failed: shader compile error";
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
          ctx.error = "MPS reductions init failed: missing kernel ";
          ctx.error += fn_name;
          return false;
        }
        *out = [ctx.device newComputePipelineStateWithFunction:fn error:&p_err];
        if (*out == nil) {
          ctx.error = "MPS reductions init failed: pipeline creation error for ";
          ctx.error += fn_name;
          if (p_err != nil && p_err.localizedDescription != nil) {
            ctx.error += ": ";
            ctx.error += std::string([p_err.localizedDescription UTF8String]);
          }
          return false;
        }
        return true;
      };

      if (!make_pipeline("sum_dim_f32", &ctx.sum_dim_f32)) return;
      if (!make_pipeline("prod_dim_f32", &ctx.prod_dim_f32)) return;
      if (!make_pipeline("amax_dim_f32", &ctx.amax_dim_f32)) return;
      if (!make_pipeline("amin_dim_f32", &ctx.amin_dim_f32)) return;
      if (!make_pipeline("all_dim_u8", &ctx.all_dim_u8)) return;
      if (!make_pipeline("any_dim_u8", &ctx.any_dim_u8)) return;
      if (!make_pipeline("argmax_dim_f32", &ctx.argmax_dim_f32)) return;
      if (!make_pipeline("argmin_dim_f32", &ctx.argmin_dim_f32)) return;
      ctx.ready = true;
    }
  });
  return ctx;
}

void validate_reduce_inputs_(const Series& a, const Series& out, const char* op) {
  if (!a.defined() || !out.defined()) {
    fail_(std::string(op) + ": undefined Series");
  }
  if (a.platform() != Platform::MPS || out.platform() != Platform::MPS) {
    fail_(std::string(op) + ": expected MPS Series");
  }
  if (!a.is_contiguous() || !out.is_contiguous()) {
    fail_(std::string(op) + ": requires contiguous Series");
  }
  if (a.dim() <= 0) {
    fail_(std::string(op) + ": expected rank >= 1");
  }
}

void fill_meta_(const Series& a, const Series& out, std::int64_t dim, ReduceMetaHost& meta, const char* op) {
  const auto in_sizes = a.sizes();
  const auto in_strides = a.strides();
  const auto out_sizes = out.sizes();
  if (in_sizes.size() > kMaxRank || out_sizes.size() > kMaxRank) {
    fail_(std::string(op) + ": rank > 8 unsupported on current MPS path");
  }
  if (in_sizes.size() != in_strides.size()) {
    fail_(std::string(op) + ": invalid sizes/strides");
  }
  if (out_sizes.size() + 1 != in_sizes.size()) {
    fail_(std::string(op) + ": output rank mismatch");
  }

  meta.in_sizes.fill(1);
  meta.in_strides.fill(0);
  meta.out_sizes.fill(1);
  meta.in_ndim = static_cast<std::uint32_t>(in_sizes.size());
  meta.out_ndim = static_cast<std::uint32_t>(out_sizes.size());
  meta.dim = static_cast<std::uint32_t>(normalize_dim_(dim, static_cast<std::int64_t>(in_sizes.size()), op));

  for (std::size_t i = 0; i < in_sizes.size(); ++i) {
    if (in_sizes[i] < 0 || in_strides[i] < 0) {
      fail_(std::string(op) + ": negative size/stride unsupported");
    }
    meta.in_sizes[i] = static_cast<std::uint64_t>(in_sizes[i]);
    meta.in_strides[i] = static_cast<std::uint64_t>(in_strides[i]);
  }
  for (std::size_t i = 0; i < out_sizes.size(); ++i) {
    if (out_sizes[i] < 0) {
      fail_(std::string(op) + ": negative output size unsupported");
    }
    meta.out_sizes[i] = static_cast<std::uint64_t>(out_sizes[i]);
  }
}

void encode_reduce_(id<MTLComputePipelineState> pso,
                    id<MTLBuffer> in_buf,
                    id<MTLBuffer> out_buf,
                    const ReduceMetaHost& meta,
                    std::int64_t out_numel,
                    const Series& out_series,
                    const char* op) {
  auto& c = ctx_();
  if (!c.ready) {
    fail_(std::string(op) + ": MPS backend unavailable (" + c.error + ")");
  }
  if (out_numel < 0) {
    fail_(std::string(op) + ": invalid numel");
  }
  if (out_numel == 0) {
    return;
  }

  @autoreleasepool {
    id<MTLCommandBuffer> cb = create_command_buffer_mm(c.queue);
    if (cb == nil) {
      fail_(std::string(op) + ": failed to create command buffer");
    }
    id<MTLComputeCommandEncoder> enc = create_compute_encoder_mm(cb);
    if (enc == nil) {
      fail_(std::string(op) + ": failed to create compute encoder");
    }
    [enc setComputePipelineState:pso];
    [enc setBuffer:in_buf offset:0 atIndex:0];
    [enc setBuffer:out_buf offset:0 atIndex:1];
    [enc setBytes:&meta length:sizeof(meta) atIndex:2];

    const NSUInteger grid_n = static_cast<NSUInteger>(out_numel);
    NSUInteger tg = pso.maxTotalThreadsPerThreadgroup;
    if (tg == 0) tg = 64;
    tg = std::min<NSUInteger>(tg, 256);
    [enc dispatchThreads:MTLSizeMake(grid_n, 1, 1) threadsPerThreadgroup:MTLSizeMake(tg, 1, 1)];
    [enc endEncoding];

    submit_command_buffer_mm(cb, out_numel);
    mark_device_data_dirty_mm(out_series, cb);
  }
}

} // namespace

bool supports_sum_dim(ScalarType dt) noexcept {
  return dt == ScalarType::Float32;
}

bool supports_prod_dim(ScalarType dt) noexcept {
  return dt == ScalarType::Float32;
}

bool supports_amax_amin_dim(ScalarType dt) noexcept {
  return dt == ScalarType::Float32;
}

bool supports_all_any_dim(ScalarType in_dt) noexcept {
  return in_dt == ScalarType::Bool;
}

bool supports_argmax_argmin_dim(ScalarType dt) noexcept {
  return dt == ScalarType::Float32;
}

void sum_dim(const Series& a, std::int64_t dim, Series& out) {
  validate_reduce_inputs_(a, out, "mps::sum_dim");
  if (a.scalar_type() != ScalarType::Float32 || out.scalar_type() != ScalarType::Float32) {
    fail_("mps::sum_dim: only float32 is supported");
  }
  auto& c = ctx_();
  @autoreleasepool {
    const std::size_t in_bytes = static_cast<std::size_t>(a.numel()) * sizeof(float);
    const std::size_t out_bytes = static_cast<std::size_t>(out.numel()) * sizeof(float);
    id<MTLBuffer> in_buf = ensure_device_buffer_for_read_mm(a, c.device, in_bytes);
    id<MTLBuffer> out_buf = ensure_device_buffer_for_write_mm(out, c.device, out_bytes);
    if (in_buf == nil || out_buf == nil) {
      fail_("mps::sum_dim: failed to allocate Metal buffers");
    }
    ReduceMetaHost meta;
    fill_meta_(a, out, dim, meta, "mps::sum_dim");
    encode_reduce_(c.sum_dim_f32, in_buf, out_buf, meta, out.numel(), out, "mps::sum_dim");
  }
}

void prod_dim(const Series& a, std::int64_t dim, Series& out) {
  validate_reduce_inputs_(a, out, "mps::prod_dim");
  if (a.scalar_type() != ScalarType::Float32 || out.scalar_type() != ScalarType::Float32) {
    fail_("mps::prod_dim: only float32 is supported");
  }
  auto& c = ctx_();
  @autoreleasepool {
    const std::size_t in_bytes = static_cast<std::size_t>(a.numel()) * sizeof(float);
    const std::size_t out_bytes = static_cast<std::size_t>(out.numel()) * sizeof(float);
    id<MTLBuffer> in_buf = ensure_device_buffer_for_read_mm(a, c.device, in_bytes);
    id<MTLBuffer> out_buf = ensure_device_buffer_for_write_mm(out, c.device, out_bytes);
    if (in_buf == nil || out_buf == nil) {
      fail_("mps::prod_dim: failed to allocate Metal buffers");
    }
    ReduceMetaHost meta;
    fill_meta_(a, out, dim, meta, "mps::prod_dim");
    encode_reduce_(c.prod_dim_f32, in_buf, out_buf, meta, out.numel(), out, "mps::prod_dim");
  }
}

void amax_dim(const Series& a, std::int64_t dim, Series& out) {
  validate_reduce_inputs_(a, out, "mps::amax_dim");
  if (a.scalar_type() != ScalarType::Float32 || out.scalar_type() != ScalarType::Float32) {
    fail_("mps::amax_dim: only float32 is supported");
  }
  auto& c = ctx_();
  @autoreleasepool {
    const std::size_t in_bytes = static_cast<std::size_t>(a.numel()) * sizeof(float);
    const std::size_t out_bytes = static_cast<std::size_t>(out.numel()) * sizeof(float);
    id<MTLBuffer> in_buf = ensure_device_buffer_for_read_mm(a, c.device, in_bytes);
    id<MTLBuffer> out_buf = ensure_device_buffer_for_write_mm(out, c.device, out_bytes);
    if (in_buf == nil || out_buf == nil) {
      fail_("mps::amax_dim: failed to allocate Metal buffers");
    }
    ReduceMetaHost meta;
    fill_meta_(a, out, dim, meta, "mps::amax_dim");
    encode_reduce_(c.amax_dim_f32, in_buf, out_buf, meta, out.numel(), out, "mps::amax_dim");
  }
}

void amin_dim(const Series& a, std::int64_t dim, Series& out) {
  validate_reduce_inputs_(a, out, "mps::amin_dim");
  if (a.scalar_type() != ScalarType::Float32 || out.scalar_type() != ScalarType::Float32) {
    fail_("mps::amin_dim: only float32 is supported");
  }
  auto& c = ctx_();
  @autoreleasepool {
    const std::size_t in_bytes = static_cast<std::size_t>(a.numel()) * sizeof(float);
    const std::size_t out_bytes = static_cast<std::size_t>(out.numel()) * sizeof(float);
    id<MTLBuffer> in_buf = ensure_device_buffer_for_read_mm(a, c.device, in_bytes);
    id<MTLBuffer> out_buf = ensure_device_buffer_for_write_mm(out, c.device, out_bytes);
    if (in_buf == nil || out_buf == nil) {
      fail_("mps::amin_dim: failed to allocate Metal buffers");
    }
    ReduceMetaHost meta;
    fill_meta_(a, out, dim, meta, "mps::amin_dim");
    encode_reduce_(c.amin_dim_f32, in_buf, out_buf, meta, out.numel(), out, "mps::amin_dim");
  }
}

void all_dim(const Series& a, std::int64_t dim, Series& out) {
  validate_reduce_inputs_(a, out, "mps::all_dim");
  if (a.scalar_type() != ScalarType::Bool || out.scalar_type() != ScalarType::Bool) {
    fail_("mps::all_dim: only bool is supported");
  }
  auto& c = ctx_();
  @autoreleasepool {
    const std::size_t in_bytes = static_cast<std::size_t>(a.numel()) * sizeof(std::uint8_t);
    const std::size_t out_bytes = static_cast<std::size_t>(out.numel()) * sizeof(std::uint8_t);
    id<MTLBuffer> in_buf = ensure_device_buffer_for_read_mm(a, c.device, in_bytes);
    id<MTLBuffer> out_buf = ensure_device_buffer_for_write_mm(out, c.device, out_bytes);
    if (in_buf == nil || out_buf == nil) {
      fail_("mps::all_dim: failed to allocate Metal buffers");
    }
    ReduceMetaHost meta;
    fill_meta_(a, out, dim, meta, "mps::all_dim");
    encode_reduce_(c.all_dim_u8, in_buf, out_buf, meta, out.numel(), out, "mps::all_dim");
  }
}

void any_dim(const Series& a, std::int64_t dim, Series& out) {
  validate_reduce_inputs_(a, out, "mps::any_dim");
  if (a.scalar_type() != ScalarType::Bool || out.scalar_type() != ScalarType::Bool) {
    fail_("mps::any_dim: only bool is supported");
  }
  auto& c = ctx_();
  @autoreleasepool {
    const std::size_t in_bytes = static_cast<std::size_t>(a.numel()) * sizeof(std::uint8_t);
    const std::size_t out_bytes = static_cast<std::size_t>(out.numel()) * sizeof(std::uint8_t);
    id<MTLBuffer> in_buf = ensure_device_buffer_for_read_mm(a, c.device, in_bytes);
    id<MTLBuffer> out_buf = ensure_device_buffer_for_write_mm(out, c.device, out_bytes);
    if (in_buf == nil || out_buf == nil) {
      fail_("mps::any_dim: failed to allocate Metal buffers");
    }
    ReduceMetaHost meta;
    fill_meta_(a, out, dim, meta, "mps::any_dim");
    encode_reduce_(c.any_dim_u8, in_buf, out_buf, meta, out.numel(), out, "mps::any_dim");
  }
}

void argmax_dim(const Series& a, std::int64_t dim, Series& out) {
  validate_reduce_inputs_(a, out, "mps::argmax_dim");
  if (a.scalar_type() != ScalarType::Float32 || out.scalar_type() != ScalarType::Int64) {
    fail_("mps::argmax_dim: expected float32 input and int64 output");
  }
  auto& c = ctx_();
  @autoreleasepool {
    const std::size_t in_bytes = static_cast<std::size_t>(a.numel()) * sizeof(float);
    const std::size_t out_bytes = static_cast<std::size_t>(out.numel()) * sizeof(std::int64_t);
    id<MTLBuffer> in_buf = ensure_device_buffer_for_read_mm(a, c.device, in_bytes);
    id<MTLBuffer> out_buf = ensure_device_buffer_for_write_mm(out, c.device, out_bytes);
    if (in_buf == nil || out_buf == nil) {
      fail_("mps::argmax_dim: failed to allocate Metal buffers");
    }
    ReduceMetaHost meta;
    fill_meta_(a, out, dim, meta, "mps::argmax_dim");
    encode_reduce_(c.argmax_dim_f32, in_buf, out_buf, meta, out.numel(), out, "mps::argmax_dim");
  }
}

void argmin_dim(const Series& a, std::int64_t dim, Series& out) {
  validate_reduce_inputs_(a, out, "mps::argmin_dim");
  if (a.scalar_type() != ScalarType::Float32 || out.scalar_type() != ScalarType::Int64) {
    fail_("mps::argmin_dim: expected float32 input and int64 output");
  }
  auto& c = ctx_();
  @autoreleasepool {
    const std::size_t in_bytes = static_cast<std::size_t>(a.numel()) * sizeof(float);
    const std::size_t out_bytes = static_cast<std::size_t>(out.numel()) * sizeof(std::int64_t);
    id<MTLBuffer> in_buf = ensure_device_buffer_for_read_mm(a, c.device, in_bytes);
    id<MTLBuffer> out_buf = ensure_device_buffer_for_write_mm(out, c.device, out_bytes);
    if (in_buf == nil || out_buf == nil) {
      fail_("mps::argmin_dim: failed to allocate Metal buffers");
    }
    ReduceMetaHost meta;
    fill_meta_(a, out, dim, meta, "mps::argmin_dim");
    encode_reduce_(c.argmin_dim_f32, in_buf, out_buf, meta, out.numel(), out, "mps::argmin_dim");
  }
}

} // namespace pluss::codem::native::mps
