#include "CodeM/native/mps/Matmul.h"
#include "CodeM/native/mps/BufferCacheMM.h"

#include "CodeM/core/Errors.h"

#include <Metal/Metal.h>
#include <MetalPerformanceShaders/MetalPerformanceShaders.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <string>

namespace pluss::codem::native::mps {
namespace {

constexpr const char* kStrictShaderSrc = R"metal(
#include <metal_stdlib>
using namespace metal;

struct MatmulMeta {
  uint m;
  uint n;
  uint k;
  uint pad;
};

kernel void strict_matmul_f32(
    const device float* a [[buffer(0)]],
    const device float* b [[buffer(1)]],
    device float* out [[buffer(2)]],
    constant MatmulMeta& meta [[buffer(3)]],
    uint2 gid [[thread_position_in_grid]]) {
  const uint col = gid.x;
  const uint row = gid.y;
  if (row >= meta.m || col >= meta.n) {
    return;
  }

  float acc = 0.0f;
  for (uint t = 0; t < meta.k; ++t) {
    acc = fma(a[row * meta.k + t], b[t * meta.n + col], acc);
  }
  out[row * meta.n + col] = acc;
}

kernel void strict_matmul_i32(
    const device int* a [[buffer(0)]],
    const device int* b [[buffer(1)]],
    device int* out [[buffer(2)]],
    constant MatmulMeta& meta [[buffer(3)]],
    uint2 gid [[thread_position_in_grid]]) {
  const uint col = gid.x;
  const uint row = gid.y;
  if (row >= meta.m || col >= meta.n) {
    return;
  }

  int acc = 0;
  for (uint t = 0; t < meta.k; ++t) {
    acc += a[row * meta.k + t] * b[t * meta.n + col];
  }
  out[row * meta.n + col] = acc;
}

struct c64 {
  float re;
  float im;
};

kernel void strict_matmul_c64(
    const device c64* a [[buffer(0)]],
    const device c64* b [[buffer(1)]],
    device c64* out [[buffer(2)]],
    constant MatmulMeta& meta [[buffer(3)]],
    uint2 gid [[thread_position_in_grid]]) {
  const uint col = gid.x;
  const uint row = gid.y;
  if (row >= meta.m || col >= meta.n) {
    return;
  }

  float acc_re = 0.0f;
  float acc_im = 0.0f;
  for (uint t = 0; t < meta.k; ++t) {
    const c64 av = a[row * meta.k + t];
    const c64 bv = b[t * meta.n + col];
    acc_re = fma(av.re, bv.re, fma(-av.im, bv.im, acc_re));
    acc_im = fma(av.re, bv.im, fma(av.im, bv.re, acc_im));
  }
  out[row * meta.n + col] = c64{acc_re, acc_im};
}
)metal";

enum class MatmulAlgo : std::uint8_t {
  Fast,
  Strict,
};

struct Context {
  bool ready{false};
  id<MTLDevice> device{nil};
  id<MTLCommandQueue> queue{nil};
  std::string error{};
};

struct StrictContext {
  bool ready{false};
  id<MTLLibrary> library{nil};
  id<MTLComputePipelineState> strict_f32{nil};
  id<MTLComputePipelineState> strict_i32{nil};
  id<MTLComputePipelineState> strict_c64{nil};
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

bool ieq_(const char* a, const char* b) noexcept {
  if (a == nullptr || b == nullptr) return false;
  while (*a != '\0' && *b != '\0') {
    if (std::tolower(static_cast<unsigned char>(*a)) != std::tolower(static_cast<unsigned char>(*b))) {
      return false;
    }
    ++a;
    ++b;
  }
  return *a == '\0' && *b == '\0';
}

bool parse_bool_env_(const char* key, bool def) noexcept {
  const char* raw = std::getenv(key);
  if (raw == nullptr || *raw == '\0') return def;
  if (ieq_(raw, "1") || ieq_(raw, "true") || ieq_(raw, "yes") || ieq_(raw, "on")) return true;
  if (ieq_(raw, "0") || ieq_(raw, "false") || ieq_(raw, "no") || ieq_(raw, "off")) return false;
  return def;
}

bool allow_reduced_precision_() noexcept {
  static const bool v = parse_bool_env_("PLUSS_MPS_MATMUL_ALLOW_REDUCED_PRECISION", true);
  return v;
}

MatmulAlgo parse_matmul_algo_() noexcept {
  const char* raw = std::getenv("PLUSS_MPS_MATMUL_ALGO");
  if (raw == nullptr || *raw == '\0') return MatmulAlgo::Fast;
  if (ieq_(raw, "strict") || ieq_(raw, "deterministic")) return MatmulAlgo::Strict;
  return MatmulAlgo::Fast;
}

MatmulAlgo matmul_algo_() noexcept {
  static const MatmulAlgo algo = parse_matmul_algo_();
  return algo;
}

StrictContext& strict_ctx_(id<MTLDevice> device) {
  static StrictContext ctx;
  static std::once_flag once;
  std::call_once(once, [&]() {
    @autoreleasepool {
      if (device == nil) {
        ctx.error = "strict matmul init failed: null device";
        return;
      }
      NSError* err = nil;
      NSString* src = [NSString stringWithUTF8String:kStrictShaderSrc];
      ctx.library = [device newLibraryWithSource:src options:nil error:&err];
      if (ctx.library == nil) {
        ctx.error = "strict matmul init failed: shader compile error";
        if (err != nil && err.localizedDescription != nil) {
          ctx.error += ": ";
          ctx.error += std::string([err.localizedDescription UTF8String]);
        }
        return;
      }

      NSError* p_err = nil;
      id<MTLFunction> fn = [ctx.library newFunctionWithName:@"strict_matmul_f32"];
      if (fn == nil) {
        ctx.error = "strict matmul init failed: missing kernel strict_matmul_f32";
        return;
      }
      ctx.strict_f32 = [device newComputePipelineStateWithFunction:fn error:&p_err];
      if (ctx.strict_f32 == nil) {
        ctx.error = "strict matmul init failed: pipeline creation error";
        if (p_err != nil && p_err.localizedDescription != nil) {
          ctx.error += ": ";
          ctx.error += std::string([p_err.localizedDescription UTF8String]);
        }
        return;
      }

      NSError* p_err_i32 = nil;
      id<MTLFunction> fn_i32 = [ctx.library newFunctionWithName:@"strict_matmul_i32"];
      if (fn_i32 == nil) {
        ctx.error = "strict matmul init failed: missing kernel strict_matmul_i32";
        return;
      }
      ctx.strict_i32 = [device newComputePipelineStateWithFunction:fn_i32 error:&p_err_i32];
      if (ctx.strict_i32 == nil) {
        ctx.error = "strict matmul init failed: pipeline creation error";
        if (p_err_i32 != nil && p_err_i32.localizedDescription != nil) {
          ctx.error += ": ";
          ctx.error += std::string([p_err_i32.localizedDescription UTF8String]);
        }
        return;
      }

      NSError* p_err_c64 = nil;
      id<MTLFunction> fn_c64 = [ctx.library newFunctionWithName:@"strict_matmul_c64"];
      if (fn_c64 == nil) {
        ctx.error = "strict matmul init failed: missing kernel strict_matmul_c64";
        return;
      }
      ctx.strict_c64 = [device newComputePipelineStateWithFunction:fn_c64 error:&p_err_c64];
      if (ctx.strict_c64 == nil) {
        ctx.error = "strict matmul init failed: pipeline creation error";
        if (p_err_c64 != nil && p_err_c64.localizedDescription != nil) {
          ctx.error += ": ";
          ctx.error += std::string([p_err_c64.localizedDescription UTF8String]);
        }
        return;
      }
      ctx.ready = true;
    }
  });
  return ctx;
}

bool matmul_dtype_supported_(ScalarType dt) noexcept {
  return dt == ScalarType::Float32 || dt == ScalarType::Int32 || dt == ScalarType::Complex64;
}

[[noreturn]] void fail_(const std::string& msg) {
  throw_error(msg);
}

void check_series_(const Series& s, const char* op) {
  if (!s.defined()) fail_(std::string(op) + ": undefined Series");
  if (s.platform() != Platform::MPS) fail_(std::string(op) + ": expected MPS Series");
  if (!s.is_contiguous()) fail_(std::string(op) + ": currently requires contiguous Series");
  if (!matmul_dtype_supported_(s.scalar_type())) {
    fail_(std::string(op) + ": currently supports float32/int32 only");
  }
}

} // namespace

bool supports_matmul(const Series& a, const Series& b) noexcept {
  if (!ctx_().ready) return false;
  if (!a.defined() || !b.defined()) return false;
  if (a.platform() != Platform::MPS || b.platform() != Platform::MPS) return false;
  if (!a.is_contiguous() || !b.is_contiguous()) return false;
  if (!matmul_dtype_supported_(a.scalar_type()) || !matmul_dtype_supported_(b.scalar_type())) return false;
  if (a.scalar_type() != b.scalar_type()) return false;
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
  if (a.scalar_type() != b.scalar_type()) {
    fail_("mps::matmul: dtype mismatch");
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
  const ScalarType dt = a.scalar_type();
  if (m64 == 0 || n64 == 0 || k64 == 0) {
    return Series::empty({m64, n64}, dt, Platform::MPS);
  }
  const NSUInteger m = static_cast<NSUInteger>(m64);
  const NSUInteger n = static_cast<NSUInteger>(n64);
  const NSUInteger k = static_cast<NSUInteger>(k64);

  Series out = Series::empty({m64, n64}, dt, Platform::MPS);
  @autoreleasepool {
    const std::size_t elem_bytes =
        (dt == ScalarType::Int32) ? sizeof(std::int32_t) : (dt == ScalarType::Complex64 ? sizeof(float) * 2u : sizeof(float));
    const std::size_t a_bytes = static_cast<std::size_t>(a.numel()) * elem_bytes;
    const std::size_t b_bytes = static_cast<std::size_t>(b.numel()) * elem_bytes;
    const std::size_t out_bytes = static_cast<std::size_t>(out.numel()) * elem_bytes;

    id<MTLBuffer> a_buf = ensure_device_buffer_for_read_mm(a, c.device, a_bytes);
    id<MTLBuffer> b_buf = ensure_device_buffer_for_read_mm(b, c.device, b_bytes);
    id<MTLBuffer> out_buf = ensure_device_buffer_for_write_mm(out, c.device, out_bytes);
    if (a_buf == nil || b_buf == nil || out_buf == nil) {
      fail_("mps::matmul: failed to allocate Metal buffers");
    }

    const NSUInteger a_row_bytes_f32 = k * sizeof(float);
    const NSUInteger b_row_bytes_f32 = n * sizeof(float);
    const NSUInteger out_row_bytes_f32 = n * sizeof(float);
    const bool fast_rowbytes_aligned =
        (a_row_bytes_f32 % 16u == 0u) &&
        (b_row_bytes_f32 % 16u == 0u) &&
        (out_row_bytes_f32 % 16u == 0u);
    if (matmul_algo_() == MatmulAlgo::Fast && dt == ScalarType::Float32 && fast_rowbytes_aligned) {
      MPSMatrixDescriptor* a_desc =
          [MPSMatrixDescriptor matrixDescriptorWithRows:m
                                                columns:k
                                               rowBytes:a_row_bytes_f32
                                               dataType:MPSDataTypeFloat32];
      MPSMatrixDescriptor* b_desc =
          [MPSMatrixDescriptor matrixDescriptorWithRows:k
                                                columns:n
                                               rowBytes:b_row_bytes_f32
                                               dataType:MPSDataTypeFloat32];
      MPSMatrixDescriptor* out_desc =
          [MPSMatrixDescriptor matrixDescriptorWithRows:m
                                                columns:n
                                               rowBytes:out_row_bytes_f32
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
      mm.options = allow_reduced_precision_() ? MPSKernelOptionsAllowReducedPrecision : MPSKernelOptionsNone;

      id<MTLCommandBuffer> cb = create_command_buffer_mm(c.queue);
      if (cb == nil) {
        fail_("mps::matmul: failed to create command buffer");
      }
      [mm encodeToCommandBuffer:cb leftMatrix:a_mat rightMatrix:b_mat resultMatrix:out_mat];
      submit_command_buffer_mm(cb, out.numel());
      mark_device_data_dirty_mm(out, cb);
      return out;
    }

    auto& sctx = strict_ctx_(c.device);
    if (!sctx.ready || sctx.strict_f32 == nil || sctx.strict_i32 == nil || sctx.strict_c64 == nil) {
      fail_("mps::matmul(strict): backend unavailable (" + sctx.error + ")");
    }

    id<MTLCommandBuffer> cb = create_command_buffer_mm(c.queue);
    if (cb == nil) {
      fail_("mps::matmul(strict): failed to create command buffer");
    }
    id<MTLComputeCommandEncoder> enc = create_compute_encoder_mm(cb);
    if (enc == nil) {
      fail_("mps::matmul(strict): failed to create compute encoder");
    }
    id<MTLComputePipelineState> pso =
        (dt == ScalarType::Int32) ? sctx.strict_i32 : (dt == ScalarType::Complex64 ? sctx.strict_c64 : sctx.strict_f32);
    [enc setComputePipelineState:pso];
    [enc setBuffer:a_buf offset:0 atIndex:0];
    [enc setBuffer:b_buf offset:0 atIndex:1];
    [enc setBuffer:out_buf offset:0 atIndex:2];
    struct Meta {
      std::uint32_t m;
      std::uint32_t n;
      std::uint32_t k;
      std::uint32_t pad;
    } meta{
        static_cast<std::uint32_t>(m),
        static_cast<std::uint32_t>(n),
        static_cast<std::uint32_t>(k),
        0u,
    };
    [enc setBytes:&meta length:sizeof(meta) atIndex:3];

    const NSUInteger grid_x = static_cast<NSUInteger>(n);
    const NSUInteger grid_y = static_cast<NSUInteger>(m);
    const NSUInteger max_tg = pso.maxTotalThreadsPerThreadgroup > 0
                                  ? pso.maxTotalThreadsPerThreadgroup
                                  : 256;
    NSUInteger tx = pso.threadExecutionWidth > 0 ? pso.threadExecutionWidth : 16;
    tx = std::min<NSUInteger>(tx, 16);
    if (tx == 0) tx = 8;
    NSUInteger ty = std::max<NSUInteger>(1, std::min<NSUInteger>(16, max_tg / tx));
    const MTLSize grid = MTLSizeMake(grid_x, grid_y, 1);
    const MTLSize group = MTLSizeMake(tx, ty, 1);
    [enc dispatchThreads:grid threadsPerThreadgroup:group];
    [enc endEncoding];
    submit_command_buffer_mm(cb, out.numel());
    mark_device_data_dirty_mm(out, cb);
  }
  return out;
}

} // namespace pluss::codem::native::mps
