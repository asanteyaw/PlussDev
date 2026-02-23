#include "CodeM/native/mps/Elementwise.h"
#include "CodeM/native/mps/BufferCacheMM.h"

#include "CodeM/core/Errors.h"

#include <Metal/Metal.h>
#include <Foundation/Foundation.h>

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

struct BinaryMeta {
  uint op;
  uint a_scalar;
  uint b_scalar;
};

struct TernaryMeta {
  uint cond_scalar;
  uint a_scalar;
  uint b_scalar;
};

kernel void binary_f32(
    const device float* a [[buffer(0)]],
    const device float* b [[buffer(1)]],
    device float* out [[buffer(2)]],
    constant BinaryMeta& meta [[buffer(3)]],
    uint gid [[thread_position_in_grid]]) {
  float av = a[meta.a_scalar != 0 ? 0 : gid];
  float bv = b[meta.b_scalar != 0 ? 0 : gid];
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
  float av = a[meta.a_scalar != 0 ? 0 : gid];
  float bv = b[meta.b_scalar != 0 ? 0 : gid];
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

kernel void where_f32(
    const device uchar* cond [[buffer(0)]],
    const device float* a [[buffer(1)]],
    const device float* b [[buffer(2)]],
    device float* out [[buffer(3)]],
    constant TernaryMeta& meta [[buffer(4)]],
    uint gid [[thread_position_in_grid]]) {
  bool m = cond[meta.cond_scalar != 0 ? 0 : gid] != 0;
  float av = a[meta.a_scalar != 0 ? 0 : gid];
  float bv = b[meta.b_scalar != 0 ? 0 : gid];
  out[gid] = m ? av : bv;
}

struct UnaryMeta {
  uint op;
};

kernel void unary_f32(
    const device float* in [[buffer(0)]],
    device float* out [[buffer(1)]],
    constant UnaryMeta& meta [[buffer(2)]],
    uint gid [[thread_position_in_grid]]) {
  float x = in[gid];
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
    case 24: v = max(0.0f, x); break;
    case 25: v = 1.0f / (1.0f + exp(-x)); break;
    case 23: v = trunc(x); break;
    default: v = 0.0f; break;
  }
  out[gid] = v;
}

kernel void unary_pred_f32(
    const device float* in [[buffer(0)]],
    device uchar* out [[buffer(1)]],
    constant UnaryMeta& meta [[buffer(2)]],
    uint gid [[thread_position_in_grid]]) {
  float x = in[gid];
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
  std::uint32_t op;
  std::uint32_t a_scalar;
  std::uint32_t b_scalar;
};

struct TernaryMetaHost {
  std::uint32_t cond_scalar;
  std::uint32_t a_scalar;
  std::uint32_t b_scalar;
};

struct UnaryMetaHost {
  std::uint32_t op;
};

struct Context {
  bool ready{false};
  id<MTLDevice> device{nil};
  id<MTLCommandQueue> queue{nil};
  id<MTLLibrary> library{nil};
  id<MTLComputePipelineState> binary_f32{nil};
  id<MTLComputePipelineState> binary_cmp_f32{nil};
  id<MTLComputePipelineState> where_f32{nil};
  id<MTLComputePipelineState> unary_f32{nil};
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
      if (!make_pipeline("where_f32", &ctx.where_f32)) return;
      if (!make_pipeline("unary_f32", &ctx.unary_f32)) return;
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

void require_scalar_or_dense_numel_(const Series& in, const Series& out, const char* op, const char* which) {
  const auto in_n = in.numel();
  const auto out_n = out.numel();
  if (!(in_n == out_n || in_n == 1)) {
    fail_(std::string(op) + ": MPS path currently supports only identical-shape or scalar broadcast for " + which);
  }
}

id<MTLCommandBuffer> encode_and_run_(id<MTLComputePipelineState> pso,
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
    id<MTLCommandBuffer> cb = [c.queue commandBuffer];
    if (cb == nil) {
      fail_(std::string(op_name) + ": failed to create command buffer");
    }
    id<MTLComputeCommandEncoder> enc = [cb computeCommandEncoder];
    if (enc == nil) {
      fail_(std::string(op_name) + ": failed to create compute encoder");
    }
    [enc setComputePipelineState:pso];
    if (b0 != nil) [enc setBuffer:b0 offset:0 atIndex:0];
    if (b1 != nil) [enc setBuffer:b1 offset:0 atIndex:1];
    if (b2 != nil) [enc setBuffer:b2 offset:0 atIndex:2];
    if (b3 != nil) [enc setBuffer:b3 offset:0 atIndex:3];
    [enc setBytes:meta_ptr length:meta_size atIndex:meta_index];

    const NSUInteger grid_n = static_cast<NSUInteger>(n);
    NSUInteger tg = pso.maxTotalThreadsPerThreadgroup;
    if (tg == 0) tg = 64;
    tg = std::min<NSUInteger>(tg, 256);
    const MTLSize grid = MTLSizeMake(grid_n, 1, 1);
    const MTLSize group = MTLSizeMake(tg, 1, 1);
    [enc dispatchThreads:grid threadsPerThreadgroup:group];
    [enc endEncoding];
    [cb commit];
    return cb;
  }
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

void validate_mps_tensor_(const Series& s, const char* op) {
  if (!s.defined()) {
    fail_(std::string(op) + ": undefined Series");
  }
  if (s.platform() != Platform::MPS) {
    fail_(std::string(op) + ": expected MPS Series");
  }
  if (s.scalar_type() != ScalarType::Float32 && s.scalar_type() != ScalarType::Bool) {
    fail_(std::string(op) + ": unsupported dtype on current MPS path");
  }
  require_contiguous_(s, op);
}

} // namespace

bool is_available() noexcept {
  return ctx_().ready;
}

bool supports_binary(cpu::BinaryOpId op, ScalarType dt) noexcept {
  if (dt != ScalarType::Float32) {
    return false;
  }
  using B = cpu::BinaryOpId;
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

void binary(cpu::BinaryOpId op, const Series& a, const Series& b, Series& out) {
  if (!is_available()) {
    fail_("binary: MPS backend unavailable");
  }
  validate_mps_tensor_(a, "binary");
  validate_mps_tensor_(b, "binary");
  validate_mps_tensor_(out, "binary");
  if (a.scalar_type() != ScalarType::Float32 || b.scalar_type() != ScalarType::Float32) {
    fail_("binary: only float32 inputs are supported on current MPS path");
  }

  const bool is_cmp = (op == cpu::BinaryOpId::Eq || op == cpu::BinaryOpId::Ne || op == cpu::BinaryOpId::Lt ||
                       op == cpu::BinaryOpId::Le || op == cpu::BinaryOpId::Gt || op == cpu::BinaryOpId::Ge);
  if (!supports_binary(op, ScalarType::Float32)) {
    fail_("binary: unsupported op on current MPS path");
  }

  require_scalar_or_dense_numel_(a, out, "binary", "lhs");
  require_scalar_or_dense_numel_(b, out, "binary", "rhs");
  if (out.numel() < 0) {
    fail_("binary: invalid output numel");
  }

  auto& c = ctx_();
  @autoreleasepool {
    const std::size_t a_bytes = static_cast<std::size_t>(a.numel()) * sizeof(float);
    const std::size_t b_bytes = static_cast<std::size_t>(b.numel()) * sizeof(float);
    const std::size_t out_bytes = static_cast<std::size_t>(out.numel()) * (is_cmp ? sizeof(std::uint8_t) : sizeof(float));
    id<MTLBuffer> a_buf = ensure_device_buffer_for_read_mm(a, c.device, a_bytes);
    id<MTLBuffer> b_buf = ensure_device_buffer_for_read_mm(b, c.device, b_bytes);
    id<MTLBuffer> out_buf = ensure_device_buffer_for_write_mm(out, c.device, out_bytes);
    if (a_buf == nil || b_buf == nil || out_buf == nil) {
      fail_("binary: failed to allocate Metal buffers");
    }
    BinaryMetaHost meta{
        binary_op_code_(op, is_cmp),
        static_cast<std::uint32_t>(a.numel() == 1 ? 1 : 0),
        static_cast<std::uint32_t>(b.numel() == 1 ? 1 : 0),
    };
    if (meta.op == 999) {
      fail_("binary: unsupported op code on current MPS path");
    }
    id<MTLCommandBuffer> cb = encode_and_run_(
        is_cmp ? c.binary_cmp_f32 : c.binary_f32,
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
      mark_device_data_dirty_mm(out, cb);
    }
  }
}

bool supports_where(ScalarType cond_dt, ScalarType data_dt) noexcept {
  return cond_dt == ScalarType::Bool && data_dt == ScalarType::Float32;
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
  require_scalar_or_dense_numel_(cond, out, "where", "condition");
  require_scalar_or_dense_numel_(a, out, "where", "lhs");
  require_scalar_or_dense_numel_(b, out, "where", "rhs");

  auto& c = ctx_();
  @autoreleasepool {
    const std::size_t cond_bytes = static_cast<std::size_t>(cond.numel()) * sizeof(std::uint8_t);
    const std::size_t a_bytes = static_cast<std::size_t>(a.numel()) * sizeof(float);
    const std::size_t b_bytes = static_cast<std::size_t>(b.numel()) * sizeof(float);
    const std::size_t out_bytes = static_cast<std::size_t>(out.numel()) * sizeof(float);
    id<MTLBuffer> cond_buf = ensure_device_buffer_for_read_mm(cond, c.device, cond_bytes);
    id<MTLBuffer> a_buf = ensure_device_buffer_for_read_mm(a, c.device, a_bytes);
    id<MTLBuffer> b_buf = ensure_device_buffer_for_read_mm(b, c.device, b_bytes);
    id<MTLBuffer> out_buf = ensure_device_buffer_for_write_mm(out, c.device, out_bytes);
    if (cond_buf == nil || a_buf == nil || b_buf == nil || out_buf == nil) {
      fail_("where: failed to allocate Metal buffers");
    }
    TernaryMetaHost meta{
        static_cast<std::uint32_t>(cond.numel() == 1 ? 1 : 0),
        static_cast<std::uint32_t>(a.numel() == 1 ? 1 : 0),
        static_cast<std::uint32_t>(b.numel() == 1 ? 1 : 0),
    };
    id<MTLCommandBuffer> cb = encode_and_run_(
        c.where_f32,
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
    id<MTLCommandBuffer> cb = encode_and_run_(c.unary_f32, in_buf, out_buf, nil, nil, &meta, sizeof(meta), 2, out.numel(), "unary");
    if (cb != nil) {
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
    id<MTLCommandBuffer> cb = encode_and_run_(c.unary_f32, in_buf, out_buf, nil, nil, &meta, sizeof(meta), 2, out.numel(), "unary_math");
    if (cb != nil) {
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
    id<MTLCommandBuffer> cb = encode_and_run_(c.unary_pred_f32, in_buf, out_buf, nil, nil, &meta, sizeof(meta), 2, out.numel(), "unary_pred");
    if (cb != nil) {
      mark_device_data_dirty_mm(out, cb);
    }
  }
}

} // namespace pluss::codem::native::mps
