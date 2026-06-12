#include "CodeM/native/mps/Gather.h"
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

struct GatherMeta {
  uint dim;
  uint ndim;
  ulong out_sizes[kMaxRank];
  ulong in_sizes[kMaxRank];
  ulong in_strides[kMaxRank];
  ulong idx_strides[kMaxRank];
};

struct Complex64 {
  float re;
  float im;
};

kernel void gather_f32_i64(
    const device float* in [[buffer(0)]],
    const device long* index [[buffer(1)]],
    device float* out [[buffer(2)]],
    constant GatherMeta& meta [[buffer(3)]],
    uint gid [[thread_position_in_grid]]) {
  ulong rem = static_cast<ulong>(gid);
  ulong in_off = 0;
  ulong idx_off = 0;
  for (int d = static_cast<int>(meta.ndim) - 1; d >= 0; --d) {
    const ulong sz = meta.out_sizes[d];
    const ulong i = (sz == 0) ? 0 : (rem % sz);
    rem = (sz == 0) ? 0 : (rem / sz);
    idx_off += i * meta.idx_strides[d];
    if (static_cast<uint>(d) != meta.dim) {
      in_off += i * meta.in_strides[d];
    }
  }

  long pick = index[idx_off];
  const long dim_sz = static_cast<long>(meta.in_sizes[meta.dim]);
  if (pick < 0) {
    pick += dim_sz;
  }
  if (pick < 0 || pick >= dim_sz) {
    out[gid] = 0.0f;
    return;
  }
  in_off += static_cast<ulong>(pick) * meta.in_strides[meta.dim];
  out[gid] = in[in_off];
}

kernel void gather_c64_i64(
    const device Complex64* in [[buffer(0)]],
    const device long* index [[buffer(1)]],
    device Complex64* out [[buffer(2)]],
    constant GatherMeta& meta [[buffer(3)]],
    uint gid [[thread_position_in_grid]]) {
  ulong rem = static_cast<ulong>(gid);
  ulong in_off = 0;
  ulong idx_off = 0;
  for (int d = static_cast<int>(meta.ndim) - 1; d >= 0; --d) {
    const ulong sz = meta.out_sizes[d];
    const ulong i = (sz == 0) ? 0 : (rem % sz);
    rem = (sz == 0) ? 0 : (rem / sz);
    idx_off += i * meta.idx_strides[d];
    if (static_cast<uint>(d) != meta.dim) {
      in_off += i * meta.in_strides[d];
    }
  }

  long pick = index[idx_off];
  const long dim_sz = static_cast<long>(meta.in_sizes[meta.dim]);
  if (pick < 0) {
    pick += dim_sz;
  }
  if (pick < 0 || pick >= dim_sz) {
    out[gid] = Complex64{0.0f, 0.0f};
    return;
  }
  in_off += static_cast<ulong>(pick) * meta.in_strides[meta.dim];
  out[gid] = in[in_off];
}

kernel void gather_i64_i64(
    const device long* in [[buffer(0)]],
    const device long* index [[buffer(1)]],
    device long* out [[buffer(2)]],
    constant GatherMeta& meta [[buffer(3)]],
    uint gid [[thread_position_in_grid]]) {
  ulong rem = static_cast<ulong>(gid);
  ulong in_off = 0;
  ulong idx_off = 0;
  for (int d = static_cast<int>(meta.ndim) - 1; d >= 0; --d) {
    const ulong sz = meta.out_sizes[d];
    const ulong i = (sz == 0) ? 0 : (rem % sz);
    rem = (sz == 0) ? 0 : (rem / sz);
    idx_off += i * meta.idx_strides[d];
    if (static_cast<uint>(d) != meta.dim) {
      in_off += i * meta.in_strides[d];
    }
  }

  long pick = index[idx_off];
  const long dim_sz = static_cast<long>(meta.in_sizes[meta.dim]);
  if (pick < 0) {
    pick += dim_sz;
  }
  if (pick < 0 || pick >= dim_sz) {
    out[gid] = static_cast<long>(0);
    return;
  }
  in_off += static_cast<ulong>(pick) * meta.in_strides[meta.dim];
  out[gid] = in[in_off];
}

kernel void gather_i32_i64(
    const device int* in [[buffer(0)]],
    const device long* index [[buffer(1)]],
    device int* out [[buffer(2)]],
    constant GatherMeta& meta [[buffer(3)]],
    uint gid [[thread_position_in_grid]]) {
  ulong rem = static_cast<ulong>(gid);
  ulong in_off = 0;
  ulong idx_off = 0;
  for (int d = static_cast<int>(meta.ndim) - 1; d >= 0; --d) {
    const ulong sz = meta.out_sizes[d];
    const ulong i = (sz == 0) ? 0 : (rem % sz);
    rem = (sz == 0) ? 0 : (rem / sz);
    idx_off += i * meta.idx_strides[d];
    if (static_cast<uint>(d) != meta.dim) {
      in_off += i * meta.in_strides[d];
    }
  }

  long pick = index[idx_off];
  const long dim_sz = static_cast<long>(meta.in_sizes[meta.dim]);
  if (pick < 0) {
    pick += dim_sz;
  }
  if (pick < 0 || pick >= dim_sz) {
    out[gid] = static_cast<int>(0);
    return;
  }
  in_off += static_cast<ulong>(pick) * meta.in_strides[meta.dim];
  out[gid] = in[in_off];
}
)metal";

struct GatherMetaHost {
  std::uint32_t dim{0};
  std::uint32_t ndim{0};
  std::array<std::uint64_t, kMaxRank> out_sizes{};
  std::array<std::uint64_t, kMaxRank> in_sizes{};
  std::array<std::uint64_t, kMaxRank> in_strides{};
  std::array<std::uint64_t, kMaxRank> idx_strides{};
};

struct Context {
  bool ready{false};
  id<MTLDevice> device{nil};
  id<MTLCommandQueue> queue{nil};
  id<MTLLibrary> library{nil};
  id<MTLComputePipelineState> gather_f32_i64{nil};
  id<MTLComputePipelineState> gather_c64_i64{nil};
  id<MTLComputePipelineState> gather_i64_i64{nil};
  id<MTLComputePipelineState> gather_i32_i64{nil};
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
        ctx.error = "MPS gather init failed: ";
        ctx.error += runtime_error_cstr();
        return;
      }
      ctx.queue = shared_queue_mm();
      if (ctx.queue == nil) {
        ctx.error = "MPS gather init failed: no shared Metal command queue";
        return;
      }

      NSError* err = nil;
      NSString* src = [NSString stringWithUTF8String:kShaderSrc];
      ctx.library = [ctx.device newLibraryWithSource:src options:nil error:&err];
      if (ctx.library == nil) {
        ctx.error = "MPS gather init failed: shader compile error";
        if (err != nil && err.localizedDescription != nil) {
          ctx.error += ": ";
          ctx.error += std::string([err.localizedDescription UTF8String]);
        }
        return;
      }

      NSError* p_err = nil;
      id<MTLFunction> fn = [ctx.library newFunctionWithName:@"gather_f32_i64"];
      if (fn == nil) {
        ctx.error = "MPS gather init failed: missing kernel gather_f32_i64";
        return;
      }
      ctx.gather_f32_i64 = [ctx.device newComputePipelineStateWithFunction:fn error:&p_err];
      if (ctx.gather_f32_i64 == nil) {
        ctx.error = "MPS gather init failed: pipeline creation error";
        if (p_err != nil && p_err.localizedDescription != nil) {
          ctx.error += ": ";
          ctx.error += std::string([p_err.localizedDescription UTF8String]);
        }
        return;
      }
      NSError* p_err_c64 = nil;
      id<MTLFunction> fn_c64 = [ctx.library newFunctionWithName:@"gather_c64_i64"];
      if (fn_c64 == nil) {
        ctx.error = "MPS gather init failed: missing kernel gather_c64_i64";
        return;
      }
      ctx.gather_c64_i64 = [ctx.device newComputePipelineStateWithFunction:fn_c64 error:&p_err_c64];
      if (ctx.gather_c64_i64 == nil) {
        ctx.error = "MPS gather init failed: pipeline creation error";
        if (p_err_c64 != nil && p_err_c64.localizedDescription != nil) {
          ctx.error += ": ";
          ctx.error += std::string([p_err_c64.localizedDescription UTF8String]);
        }
        return;
      }
      NSError* p_err_i64 = nil;
      id<MTLFunction> fn_i64 = [ctx.library newFunctionWithName:@"gather_i64_i64"];
      if (fn_i64 == nil) {
        ctx.error = "MPS gather init failed: missing kernel gather_i64_i64";
        return;
      }
      ctx.gather_i64_i64 = [ctx.device newComputePipelineStateWithFunction:fn_i64 error:&p_err_i64];
      if (ctx.gather_i64_i64 == nil) {
        ctx.error = "MPS gather init failed: pipeline creation error";
        if (p_err_i64 != nil && p_err_i64.localizedDescription != nil) {
          ctx.error += ": ";
          ctx.error += std::string([p_err_i64.localizedDescription UTF8String]);
        }
        return;
      }
      NSError* p_err_i32 = nil;
      id<MTLFunction> fn_i32 = [ctx.library newFunctionWithName:@"gather_i32_i64"];
      if (fn_i32 == nil) {
        ctx.error = "MPS gather init failed: missing kernel gather_i32_i64";
        return;
      }
      ctx.gather_i32_i64 = [ctx.device newComputePipelineStateWithFunction:fn_i32 error:&p_err_i32];
      if (ctx.gather_i32_i64 == nil) {
        ctx.error = "MPS gather init failed: pipeline creation error";
        if (p_err_i32 != nil && p_err_i32.localizedDescription != nil) {
          ctx.error += ": ";
          ctx.error += std::string([p_err_i32.localizedDescription UTF8String]);
        }
        return;
      }
      ctx.ready = true;
    }
  });
  return ctx;
}

void validate_inputs_(const Series& input, const Series& index, const Series& out, std::int64_t dim, GatherMetaHost& meta) {
  if (!input.defined() || !index.defined() || !out.defined()) {
    fail_("mps::gather: undefined Series");
  }
  if (input.platform() != Platform::MPS || index.platform() != Platform::MPS || out.platform() != Platform::MPS) {
    fail_("mps::gather: expected MPS Series");
  }
  if (!input.is_contiguous() || !index.is_contiguous() || !out.is_contiguous()) {
    fail_("mps::gather: requires contiguous Series");
  }
  if (input.scalar_type() != out.scalar_type()) {
    fail_("mps::gather: input/output dtype mismatch");
  }
  if (input.scalar_type() != ScalarType::Float32 &&
      input.scalar_type() != ScalarType::Complex64 &&
      input.scalar_type() != ScalarType::Int64 &&
      input.scalar_type() != ScalarType::Int32) {
    fail_("mps::gather: only float32/complex64/int32/int64 input/output supported on current MPS path");
  }
  if (index.scalar_type() != ScalarType::Int64) {
    fail_("mps::gather: index must be int64");
  }

  const auto in_sizes = input.sizes();
  const auto idx_sizes = index.sizes();
  const auto out_sizes = out.sizes();
  const auto in_strides = input.strides();
  const auto idx_strides = index.strides();
  if (in_sizes.size() != idx_sizes.size() || idx_sizes.size() != out_sizes.size()) {
    fail_("mps::gather: rank mismatch");
  }
  if (in_sizes.size() > kMaxRank) {
    fail_("mps::gather: rank > 8 unsupported on current MPS path");
  }
  if (in_sizes.size() != in_strides.size() || idx_sizes.size() != idx_strides.size()) {
    fail_("mps::gather: invalid sizes/strides");
  }

  const auto nd = static_cast<std::int64_t>(in_sizes.size());
  const auto d = normalize_dim_(dim, nd, "mps::gather");
  for (std::int64_t i = 0; i < nd; ++i) {
    if (i != d && idx_sizes[static_cast<std::size_t>(i)] != in_sizes[static_cast<std::size_t>(i)]) {
      fail_("mps::gather: index shape must match input except at dim");
    }
    if (out_sizes[static_cast<std::size_t>(i)] != idx_sizes[static_cast<std::size_t>(i)]) {
      fail_("mps::gather: output shape mismatch");
    }
  }

  sync_host_if_needed(index);
  const auto* idx_ptr = reinterpret_cast<const std::int64_t*>(index.raw_data());
  const auto dim_size = in_sizes[static_cast<std::size_t>(d)];
  for (std::int64_t i = 0; i < index.numel(); ++i) {
    const auto v = idx_ptr[i];
    if (v < -dim_size || v >= dim_size) {
      fail_("mps::gather: index out of range");
    }
  }

  meta.dim = static_cast<std::uint32_t>(d);
  meta.ndim = static_cast<std::uint32_t>(in_sizes.size());
  meta.out_sizes.fill(1);
  meta.in_sizes.fill(1);
  meta.in_strides.fill(0);
  meta.idx_strides.fill(0);
  for (std::size_t i = 0; i < in_sizes.size(); ++i) {
    if (in_sizes[i] < 0 || out_sizes[i] < 0 || in_strides[i] < 0 || idx_strides[i] < 0) {
      fail_("mps::gather: negative size/stride unsupported");
    }
    meta.out_sizes[i] = static_cast<std::uint64_t>(out_sizes[i]);
    meta.in_sizes[i] = static_cast<std::uint64_t>(in_sizes[i]);
    meta.in_strides[i] = static_cast<std::uint64_t>(in_strides[i]);
    meta.idx_strides[i] = static_cast<std::uint64_t>(idx_strides[i]);
  }
}

} // namespace

bool supports_gather(ScalarType input_dt, ScalarType index_dt) noexcept {
  return (input_dt == ScalarType::Float32 || input_dt == ScalarType::Complex64 ||
          input_dt == ScalarType::Int64 || input_dt == ScalarType::Int32) &&
      index_dt == ScalarType::Int64;
}

void gather(const Series& input, std::int64_t dim, const Series& index, Series& out) {
  auto& c = ctx_();
  if (!c.ready) {
    fail_("mps::gather: backend unavailable (" + c.error + ")");
  }
  GatherMetaHost meta;
  validate_inputs_(input, index, out, dim, meta);

  if (out.numel() == 0) {
    return;
  }

  @autoreleasepool {
    const ScalarType input_dt = input.scalar_type();
    std::size_t elem_bytes = sizeof(std::int64_t);
    if (input_dt == ScalarType::Float32) {
      elem_bytes = sizeof(float);
    } else if (input_dt == ScalarType::Complex64) {
      elem_bytes = sizeof(float) * 2;
    } else if (input_dt == ScalarType::Int32) {
      elem_bytes = sizeof(std::int32_t);
    }
    const std::size_t in_bytes = static_cast<std::size_t>(input.numel()) * elem_bytes;
    const std::size_t idx_bytes = static_cast<std::size_t>(index.numel()) * sizeof(std::int64_t);
    const std::size_t out_bytes = static_cast<std::size_t>(out.numel()) * elem_bytes;
    id<MTLBuffer> in_buf = ensure_device_buffer_for_read_mm(input, c.device, in_bytes);
    id<MTLBuffer> idx_buf = ensure_device_buffer_for_read_mm(index, c.device, idx_bytes);
    id<MTLBuffer> out_buf = ensure_device_buffer_for_write_mm(out, c.device, out_bytes);
    if (in_buf == nil || idx_buf == nil || out_buf == nil) {
      fail_("mps::gather: failed to allocate Metal buffers");
    }

    id<MTLCommandBuffer> cb = create_command_buffer_mm(c.queue);
    if (cb == nil) {
      fail_("mps::gather: failed to create command buffer");
    }
    id<MTLComputeCommandEncoder> enc = create_compute_encoder_mm(cb);
    if (enc == nil) {
      fail_("mps::gather: failed to create compute encoder");
    }
    id<MTLComputePipelineState> pipeline = c.gather_i64_i64;
    if (input_dt == ScalarType::Float32) {
      pipeline = c.gather_f32_i64;
    } else if (input_dt == ScalarType::Complex64) {
      pipeline = c.gather_c64_i64;
    } else if (input_dt == ScalarType::Int32) {
      pipeline = c.gather_i32_i64;
    }
    [enc setComputePipelineState:pipeline];
    [enc setBuffer:in_buf offset:0 atIndex:0];
    [enc setBuffer:idx_buf offset:0 atIndex:1];
    [enc setBuffer:out_buf offset:0 atIndex:2];
    [enc setBytes:&meta length:sizeof(meta) atIndex:3];

    const NSUInteger grid_n = static_cast<NSUInteger>(out.numel());
    NSUInteger tg = pipeline.maxTotalThreadsPerThreadgroup;
    if (tg == 0) tg = 64;
    tg = std::min<NSUInteger>(tg, 256);
    [enc dispatchThreads:MTLSizeMake(grid_n, 1, 1) threadsPerThreadgroup:MTLSizeMake(tg, 1, 1)];
    [enc endEncoding];
    submit_command_buffer_mm(cb, out.numel());
    mark_device_data_dirty_mm(out, cb);
  }
}

} // namespace pluss::codem::native::mps
