#include "CodeM/native/mps/Nonzero.h"
#include "CodeM/native/mps/BufferCacheMM.h"
#include "CodeM/native/mps/MPSAllocatorMM.h"

#include "CodeM/core/Errors.h"

#include <Foundation/Foundation.h>
#include <Metal/Metal.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <span>
#include <string>

namespace pluss::codem::native::mps {
namespace {

constexpr std::size_t kMaxRank = 8;

constexpr const char* kShaderSrc = R"metal(
#include <metal_stdlib>
using namespace metal;

constant uint kMaxRank = 8;

struct NonzeroMeta {
  uint ndim;
  ulong numel;
  ulong sizes[kMaxRank];
  ulong strides[kMaxRank];
};

kernel void nonzero_u8_i64(
    const device uchar* mask [[buffer(0)]],
    device long* out [[buffer(1)]],
    device uint* counter [[buffer(2)]],
    constant NonzeroMeta& meta [[buffer(3)]],
    uint gid [[thread_position_in_grid]]) {
  if (gid != 0u) {
    return;
  }
  uint row = 0u;
  for (ulong lin = 0; lin < meta.numel; ++lin) {
    if (mask[lin] == 0) {
      continue;
    }
    ulong rem = lin;
    for (uint d = 0; d < meta.ndim; ++d) {
      const ulong stride = meta.strides[d];
      const ulong idx = (stride == 0) ? 0 : (rem / stride);
      rem = (stride == 0) ? 0 : (rem - idx * stride);
      out[static_cast<ulong>(row) * meta.ndim + d] = static_cast<long>(idx);
    }
    row += 1u;
  }
  counter[0] = row;
}
)metal";

struct NonzeroMetaHost {
  std::uint32_t ndim{0};
  std::uint64_t numel{0};
  std::array<std::uint64_t, kMaxRank> sizes{};
  std::array<std::uint64_t, kMaxRank> strides{};
};

struct Context {
  bool ready{false};
  id<MTLDevice> device{nil};
  id<MTLCommandQueue> queue{nil};
  id<MTLLibrary> library{nil};
  id<MTLComputePipelineState> nonzero_u8_i64{nil};
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
        ctx.error = "MPS nonzero init failed: ";
        ctx.error += runtime_error_cstr();
        return;
      }
      ctx.queue = shared_queue_mm();
      if (ctx.queue == nil) {
        ctx.error = "MPS nonzero init failed: no shared Metal command queue";
        return;
      }

      NSError* err = nil;
      NSString* src = [NSString stringWithUTF8String:kShaderSrc];
      ctx.library = [ctx.device newLibraryWithSource:src options:nil error:&err];
      if (ctx.library == nil) {
        ctx.error = "MPS nonzero init failed: shader compile error";
        if (err != nil && err.localizedDescription != nil) {
          ctx.error += ": ";
          ctx.error += std::string([err.localizedDescription UTF8String]);
        }
        return;
      }

      NSError* p_err = nil;
      id<MTLFunction> fn = [ctx.library newFunctionWithName:@"nonzero_u8_i64"];
      if (fn == nil) {
        ctx.error = "MPS nonzero init failed: missing kernel nonzero_u8_i64";
        return;
      }
      ctx.nonzero_u8_i64 = [ctx.device newComputePipelineStateWithFunction:fn error:&p_err];
      if (ctx.nonzero_u8_i64 == nil) {
        ctx.error = "MPS nonzero init failed: pipeline creation error";
        if (p_err != nil && p_err.localizedDescription != nil) {
          ctx.error += ": ";
          ctx.error += std::string([p_err.localizedDescription UTF8String]);
        }
        return;
      }
      ctx.ready = true;
    }
  });
  return ctx;
}

void validate_mask_(const Series& mask, NonzeroMetaHost& meta) {
  if (!mask.defined()) {
    fail_("mps::nonzero: undefined mask");
  }
  if (mask.platform() != Platform::MPS) {
    fail_("mps::nonzero: expected MPS mask");
  }
  if (!mask.is_contiguous()) {
    fail_("mps::nonzero: requires contiguous mask");
  }
  if (mask.scalar_type() != ScalarType::Bool) {
    fail_("mps::nonzero: mask must be bool");
  }
  const auto sizes = mask.sizes();
  if (sizes.size() > kMaxRank) {
    fail_("mps::nonzero: rank > 8 unsupported on current MPS path");
  }

  meta.ndim = static_cast<std::uint32_t>(sizes.size());
  meta.numel = static_cast<std::uint64_t>(mask.numel());
  meta.sizes.fill(1);
  meta.strides.fill(0);
  if (sizes.empty()) {
    return;
  }
  std::array<std::uint64_t, kMaxRank> strides{};
  strides.fill(0);
  const std::size_t nd = sizes.size();
  strides[nd - 1] = 1;
  for (std::size_t i = nd - 1; i > 0; --i) {
    strides[i - 1] = strides[i] * static_cast<std::uint64_t>(sizes[i]);
  }
  for (std::size_t i = 0; i < nd; ++i) {
    if (sizes[i] < 0) {
      fail_("mps::nonzero: negative size unsupported");
    }
    meta.sizes[i] = static_cast<std::uint64_t>(sizes[i]);
    meta.strides[i] = strides[i];
  }
}

} // namespace

bool supports_nonzero(ScalarType mask_dt) noexcept {
  return ctx_().ready && mask_dt == ScalarType::Bool;
}

Series nonzero(const Series& mask) {
  auto& c = ctx_();
  if (!c.ready) {
    fail_("mps::nonzero: backend unavailable (" + c.error + ")");
  }

  if (mask.dim() == 0) {
    sync_host_if_needed(mask);
    const bool* mp = reinterpret_cast<const bool*>(mask.raw_data());
    const std::int64_t count = (mp != nullptr && mp[0]) ? 1 : 0;
    const std::array<std::int64_t, 2> out_shape{{count, 0}};
    return Series::empty(std::span<const std::int64_t>(out_shape.data(), out_shape.size()),
                         ScalarType::Int64,
                         Platform::MPS);
  }
  if (mask.numel() == 0) {
    const std::array<std::int64_t, 2> out_shape{{0, mask.dim()}};
    return Series::empty(std::span<const std::int64_t>(out_shape.data(), out_shape.size()),
                         ScalarType::Int64,
                         Platform::MPS);
  }

  NonzeroMetaHost meta;
  validate_mask_(mask, meta);
  Series out_full = Series::zeros({mask.numel(), mask.dim()}, ScalarType::Int64).to(Platform::MPS).contiguous();

  @autoreleasepool {
    const std::size_t mask_bytes = static_cast<std::size_t>(mask.numel()) * sizeof(std::uint8_t);
    const std::size_t out_bytes = static_cast<std::size_t>(out_full.numel()) * sizeof(std::int64_t);
    id<MTLBuffer> mask_buf = ensure_device_buffer_for_read_mm(mask, c.device, mask_bytes);
    id<MTLBuffer> out_buf = ensure_device_buffer_for_write_mm(out_full, c.device, out_bytes);
    id<MTLBuffer> counter_buf = acquire_pooled_buffer_mm(c.device, sizeof(std::uint32_t), "mps_nonzero_counter");
    id<MTLBuffer> meta_buf = acquire_pooled_buffer_mm(c.device, sizeof(NonzeroMetaHost), "mps_nonzero_meta");
    if (mask_buf == nil || out_buf == nil || counter_buf == nil || meta_buf == nil) {
      if (counter_buf != nil) release_pooled_buffer_mm(counter_buf);
      if (meta_buf != nil) release_pooled_buffer_mm(meta_buf);
      fail_("mps::nonzero: failed to allocate Metal buffers");
    }

    const std::uint32_t zero = 0;
    std::memcpy([counter_buf contents], &zero, sizeof(zero));
    std::memcpy([meta_buf contents], &meta, sizeof(meta));

    id<MTLCommandBuffer> cb = create_command_buffer_mm(c.queue);
    if (cb == nil) {
      release_pooled_buffer_mm(counter_buf);
      release_pooled_buffer_mm(meta_buf);
      fail_("mps::nonzero: failed to create command buffer");
    }
    id<MTLComputeCommandEncoder> enc = create_compute_encoder_mm(cb);
    if (enc == nil) {
      release_pooled_buffer_mm(counter_buf);
      release_pooled_buffer_mm(meta_buf);
      fail_("mps::nonzero: failed to create compute encoder");
    }

    [enc setComputePipelineState:c.nonzero_u8_i64];
    [enc setBuffer:mask_buf offset:0 atIndex:0];
    [enc setBuffer:out_buf offset:0 atIndex:1];
    [enc setBuffer:counter_buf offset:0 atIndex:2];
    [enc setBuffer:meta_buf offset:0 atIndex:3];
    const NSUInteger grid_n = 1;
    NSUInteger tg = c.nonzero_u8_i64.maxTotalThreadsPerThreadgroup;
    if (tg == 0) tg = 64;
    tg = std::min<NSUInteger>(tg, 256);
    [enc dispatchThreads:MTLSizeMake(grid_n, 1, 1) threadsPerThreadgroup:MTLSizeMake(tg, 1, 1)];
    [enc endEncoding];

    submit_command_buffer_mm(cb, mask.numel());
    mark_device_data_dirty_mm(out_full, cb);
    wait_command_buffer_mm(cb);

    const auto* cp = reinterpret_cast<const std::uint32_t*>([counter_buf contents]);
    const std::uint32_t count_u32 = (cp == nullptr) ? 0u : *cp;
    release_pooled_buffer_mm(counter_buf);
    release_pooled_buffer_mm(meta_buf);

    const std::int64_t count = std::min<std::int64_t>(
        static_cast<std::int64_t>(count_u32),
        mask.numel());
    if (count <= 0) {
      const std::array<std::int64_t, 2> out_shape{{0, mask.dim()}};
      return Series::empty(std::span<const std::int64_t>(out_shape.data(), out_shape.size()),
                           ScalarType::Int64,
                           Platform::MPS);
    }
    if (count >= mask.numel()) {
      return out_full;
    }
    const std::array<std::int64_t, 2> trim_shape{{count, mask.dim()}};
    Series out_trimmed =
        Series::empty(std::span<const std::int64_t>(trim_shape.data(), trim_shape.size()),
                      ScalarType::Int64,
                      Platform::MPS)
            .contiguous();
    const std::size_t trim_bytes = static_cast<std::size_t>(out_trimmed.numel()) * sizeof(std::int64_t);
    id<MTLBuffer> src_buf = ensure_device_buffer_for_read_mm(out_full, c.device, out_bytes);
    id<MTLBuffer> dst_buf = ensure_device_buffer_for_write_mm(out_trimmed, c.device, trim_bytes);
    if (src_buf == nil || dst_buf == nil) {
      fail_("mps::nonzero: failed to allocate trim buffers");
    }

    id<MTLCommandBuffer> trim_cb = create_command_buffer_mm(c.queue);
    if (trim_cb == nil) {
      fail_("mps::nonzero: failed to create trim command buffer");
    }
    id<MTLBlitCommandEncoder> blit = [trim_cb blitCommandEncoder];
    if (blit == nil) {
      fail_("mps::nonzero: failed to create trim blit encoder");
    }
    [blit copyFromBuffer:src_buf sourceOffset:0 toBuffer:dst_buf destinationOffset:0 size:trim_bytes];
    [blit endEncoding];
    submit_command_buffer_mm(trim_cb, out_trimmed.numel());
    mark_device_data_dirty_mm(out_trimmed, trim_cb);
    return out_trimmed;
  }
}

} // namespace pluss::codem::native::mps
