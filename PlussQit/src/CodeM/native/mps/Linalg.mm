#include "CodeM/native/mps/Linalg.h"
#include "CodeM/native/mps/BufferCacheMM.h"

#include "CodeM/core/Errors.h"

#include <Metal/Metal.h>
#include <MetalPerformanceShaders/MetalPerformanceShaders.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <mutex>
#include <string>
#include <vector>

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
        ctx.error = "MPS linalg init failed: ";
        ctx.error += runtime_error_cstr();
        return;
      }
      ctx.queue = shared_queue_mm();
      if (ctx.queue == nil) {
        ctx.error = "MPS linalg init failed: no shared Metal command queue";
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

bool supports_square_f32_(const Series& a) noexcept {
  if (!ctx_().ready) return false;
  if (!a.defined()) return false;
  if (a.platform() != Platform::MPS) return false;
  if (a.scalar_type() != ScalarType::Float32) return false;
  if (a.dim() != 2) return false;
  if (!a.is_contiguous()) return false;
  if (a.sizes()[0] <= 0) return false;
  return a.sizes()[0] == a.sizes()[1];
}

bool supports_rhs_f32_(const Series& b, std::int64_t n) noexcept {
  if (!b.defined()) return false;
  if (b.platform() != Platform::MPS) return false;
  if (b.scalar_type() != ScalarType::Float32) return false;
  if (!b.is_contiguous()) return false;
  if (b.dim() == 1) {
    return b.sizes()[0] == n;
  }
  if (b.dim() == 2) {
    return b.sizes()[0] == n && b.sizes()[1] > 0;
  }
  return false;
}

void check_square_f32_(const Series& a, const char* op) {
  if (!supports_square_f32_(a)) {
    fail_(std::string(op) + ": expected contiguous square float32 matrix on MPS");
  }
}

struct LuWorkspace {
  Series lu{pluss::series::undefined()};
  Series pivots{pluss::series::undefined()};
  Series status{pluss::series::undefined()};
};

LuWorkspace run_lu_factorization_(const Series& a, const char* op) {
  auto& c = ctx_();
  if (!c.ready) {
    fail_(std::string(op) + ": backend unavailable (" + c.error + ")");
  }
  check_square_f32_(a, op);

  const auto n64 = a.sizes()[0];
  const NSUInteger n = static_cast<NSUInteger>(n64);
  const std::size_t mat_bytes = static_cast<std::size_t>(a.numel()) * sizeof(float);
  const std::size_t piv_bytes = static_cast<std::size_t>(n64) * sizeof(std::uint32_t);
  const std::size_t status_bytes = sizeof(std::int32_t);

  LuWorkspace ws{};
  ws.lu = a.clone().contiguous();
  ws.pivots = Series::zeros({n64}, ScalarType::UInt32).to(Platform::MPS).contiguous();
  ws.status = Series::zeros({1}, ScalarType::Int32).to(Platform::MPS).contiguous();

  @autoreleasepool {
    id<MTLBuffer> lu_buf = ensure_device_buffer_for_read_mm(ws.lu, c.device, mat_bytes);
    id<MTLBuffer> piv_buf = ensure_device_buffer_for_write_mm(ws.pivots, c.device, piv_bytes);
    id<MTLBuffer> status_buf = ensure_device_buffer_for_write_mm(ws.status, c.device, status_bytes);
    if (lu_buf == nil || piv_buf == nil || status_buf == nil) {
      fail_(std::string(op) + ": failed to allocate Metal buffers");
    }

    id<MTLCommandBuffer> cb = create_command_buffer_mm(c.queue);
    if (cb == nil) {
      fail_(std::string(op) + ": failed to create command buffer");
    }

    MPSMatrixDecompositionLU* lu_op = [[MPSMatrixDecompositionLU alloc] initWithDevice:c.device
                                                                                   rows:n
                                                                                columns:n];
    MPSMatrixDescriptor* lu_desc = [MPSMatrixDescriptor matrixDescriptorWithRows:n
                                                                          columns:n
                                                                         rowBytes:n * sizeof(float)
                                                                         dataType:MPSDataTypeFloat32];
    MPSMatrixDescriptor* piv_desc = [MPSMatrixDescriptor matrixDescriptorWithRows:1
                                                                           columns:n
                                                                          rowBytes:n * sizeof(std::uint32_t)
                                                                          dataType:MPSDataTypeUInt32];

    MPSMatrix* lu_mat = [[MPSMatrix alloc] initWithBuffer:lu_buf
                                                   offset:static_cast<NSUInteger>(ws.lu.storage_offset())
                                               descriptor:lu_desc];
    MPSMatrix* piv_mat = [[MPSMatrix alloc] initWithBuffer:piv_buf
                                                    offset:static_cast<NSUInteger>(ws.pivots.storage_offset())
                                                descriptor:piv_desc];

    [lu_op encodeToCommandBuffer:cb
                    sourceMatrix:lu_mat
                    resultMatrix:lu_mat
                    pivotIndices:piv_mat
                          status:status_buf];

    submit_command_buffer_mm(cb, ws.lu.numel());
    mark_device_data_dirty_mm(ws.lu, cb);
    mark_device_data_dirty_mm(ws.pivots, cb);
    mark_device_data_dirty_mm(ws.status, cb);
  }
  return ws;
}

std::int32_t read_status_code_(const Series& status_mps) {
  const Series code_cpu = status_mps.to(Platform::CPU).to(ScalarType::Int32).contiguous();
  const auto v = code_cpu.host<std::int32_t>();
  return v.empty() ? 0 : v[0];
}

Series lu_solve_from_workspace_(const LuWorkspace& ws, const Series& b, const char* op) {
  const auto n64 = ws.lu.sizes()[0];
  if (!supports_rhs_f32_(b, n64)) {
    fail_(std::string(op) + ": expected rhs as contiguous float32 MPS tensor with shape [n] or [n, k]");
  }

  const std::int32_t status = read_status_code_(ws.status);
  if (status != 0) {
    fail_(std::string(op) + ": singular or ill-conditioned matrix");
  }

  const bool vector_rhs = (b.dim() == 1);
  const std::int64_t nrhs64 = vector_rhs ? 1 : b.sizes()[1];
  const NSUInteger n = static_cast<NSUInteger>(n64);
  const NSUInteger nrhs = static_cast<NSUInteger>(nrhs64);

  const Series rhs_mat = vector_rhs ? b.reshape({n64, 1}) : b;
  Series out_mat = Series::empty({n64, nrhs64}, ScalarType::Float32, Platform::MPS).contiguous();

  auto& c = ctx_();
  @autoreleasepool {
    const std::size_t lu_bytes = static_cast<std::size_t>(n64 * n64) * sizeof(float);
    const std::size_t piv_bytes = static_cast<std::size_t>(n64) * sizeof(std::uint32_t);
    const std::size_t rhs_bytes = static_cast<std::size_t>(rhs_mat.numel()) * sizeof(float);

    id<MTLBuffer> lu_buf = ensure_device_buffer_for_read_mm(ws.lu, c.device, lu_bytes);
    id<MTLBuffer> piv_buf = ensure_device_buffer_for_read_mm(ws.pivots, c.device, piv_bytes);
    id<MTLBuffer> rhs_buf = ensure_device_buffer_for_read_mm(rhs_mat, c.device, rhs_bytes);
    id<MTLBuffer> out_buf = ensure_device_buffer_for_write_mm(out_mat, c.device, rhs_bytes);
    if (lu_buf == nil || piv_buf == nil || rhs_buf == nil || out_buf == nil) {
      fail_(std::string(op) + ": failed to allocate Metal buffers");
    }

    id<MTLCommandBuffer> cb = create_command_buffer_mm(c.queue);
    if (cb == nil) {
      fail_(std::string(op) + ": failed to create command buffer");
    }

    MPSMatrixSolveLU* solver = [[MPSMatrixSolveLU alloc] initWithDevice:c.device
                                                               transpose:false
                                                                   order:n
                                                  numberOfRightHandSides:nrhs];
    MPSMatrixDescriptor* lu_desc = [MPSMatrixDescriptor matrixDescriptorWithRows:n
                                                                          columns:n
                                                                         rowBytes:n * sizeof(float)
                                                                         dataType:MPSDataTypeFloat32];
    MPSMatrixDescriptor* rhs_desc = [MPSMatrixDescriptor matrixDescriptorWithRows:n
                                                                           columns:nrhs
                                                                          rowBytes:nrhs * sizeof(float)
                                                                          dataType:MPSDataTypeFloat32];
    MPSMatrixDescriptor* piv_desc = [MPSMatrixDescriptor matrixDescriptorWithRows:1
                                                                           columns:n
                                                                          rowBytes:n * sizeof(std::uint32_t)
                                                                          dataType:MPSDataTypeUInt32];

    MPSMatrix* lu_mat = [[MPSMatrix alloc] initWithBuffer:lu_buf
                                                   offset:static_cast<NSUInteger>(ws.lu.storage_offset())
                                               descriptor:lu_desc];
    MPSMatrix* piv_mat = [[MPSMatrix alloc] initWithBuffer:piv_buf
                                                    offset:static_cast<NSUInteger>(ws.pivots.storage_offset())
                                                descriptor:piv_desc];
    MPSMatrix* rhs = [[MPSMatrix alloc] initWithBuffer:rhs_buf
                                                offset:static_cast<NSUInteger>(rhs_mat.storage_offset())
                                            descriptor:rhs_desc];
    MPSMatrix* out = [[MPSMatrix alloc] initWithBuffer:out_buf
                                                offset:static_cast<NSUInteger>(out_mat.storage_offset())
                                            descriptor:rhs_desc];

    [solver encodeToCommandBuffer:cb
                     sourceMatrix:lu_mat
              rightHandSideMatrix:rhs
                     pivotIndices:piv_mat
                   solutionMatrix:out];

    submit_command_buffer_mm(cb, out_mat.numel());
    mark_device_data_dirty_mm(out_mat, cb);
  }
  return vector_rhs ? out_mat.reshape({n64}) : out_mat;
}

} // namespace

bool supports_lu_solver(const Series& a) noexcept {
  return supports_square_f32_(a);
}

bool supports_lu_solve(const Series& a, const Series& b) noexcept {
  if (!supports_square_f32_(a)) return false;
  return supports_rhs_f32_(b, a.sizes()[0]);
}

Series lu_solve(const Series& a, const Series& b) {
  constexpr const char* op = "mps::linalg::lu_solve";
  auto ws = run_lu_factorization_(a, op);
  return lu_solve_from_workspace_(ws, b, op);
}

Series lu_inverse(const Series& a) {
  constexpr const char* op = "mps::linalg::inverse";
  auto ws = run_lu_factorization_(a, op);
  const auto n64 = a.sizes()[0];
  Series rhs = Series::eye(n64, n64, ScalarType::Float32).to(Platform::MPS).contiguous();
  return lu_solve_from_workspace_(ws, rhs, op);
}

LuDetStats lu_det_stats(const Series& a) {
  constexpr const char* op = "mps::linalg::det";
  auto ws = run_lu_factorization_(a, op);

  LuDetStats out{};
  const std::int32_t status = read_status_code_(ws.status);
  if (status != 0) {
    out.sign = 0.0f;
    out.logabs = -std::numeric_limits<float>::infinity();
    out.singular = true;
    return out;
  }

  const Series lu_cpu = ws.lu.to(Platform::CPU).to(ScalarType::Float32).contiguous();
  const Series piv_cpu = ws.pivots.to(Platform::CPU).to(ScalarType::UInt32).contiguous();
  const auto lu = lu_cpu.host<float>();
  const auto piv = piv_cpu.host<std::uint32_t>();

  const auto n64 = a.sizes()[0];
  int swap_count = 0;
  for (std::int64_t i = 0; i < n64 && i < static_cast<std::int64_t>(piv.size()); ++i) {
    const std::uint32_t p = piv[static_cast<std::size_t>(i)];
    const std::uint32_t expect = static_cast<std::uint32_t>(i + 1);
    if (p != 0 && p != expect) {
      ++swap_count;
    }
  }
  out.sign = (swap_count % 2 == 0) ? 1.0f : -1.0f;
  out.logabs = 0.0f;
  out.singular = false;

  for (std::int64_t i = 0; i < n64; ++i) {
    const float d = lu[static_cast<std::size_t>(i * n64 + i)];
    if (!std::isfinite(d) || std::fabs(d) <= 1e-20f) {
      out.sign = 0.0f;
      out.logabs = -std::numeric_limits<float>::infinity();
      out.singular = true;
      break;
    }
    if (d < 0.0f) {
      out.sign = -out.sign;
    }
    out.logabs += std::log(std::fabs(d));
  }
  return out;
}

bool supports_cholesky_factor(const Series& a) noexcept {
  return supports_square_f32_(a);
}

Series cholesky_factor(const Series& a, bool upper) {
  constexpr const char* op = "mps::linalg::cholesky";
  auto& c = ctx_();
  if (!c.ready) {
    fail_(std::string(op) + ": backend unavailable (" + c.error + ")");
  }
  check_square_f32_(a, op);

  const auto n64 = a.sizes()[0];
  const NSUInteger n = static_cast<NSUInteger>(n64);
  const std::size_t mat_bytes = static_cast<std::size_t>(a.numel()) * sizeof(float);
  const std::size_t status_bytes = sizeof(std::int32_t);

  Series out = a.clone().contiguous();
  Series status = Series::zeros({1}, ScalarType::Int32).to(Platform::MPS).contiguous();

  @autoreleasepool {
    id<MTLBuffer> out_buf = ensure_device_buffer_for_read_mm(out, c.device, mat_bytes);
    id<MTLBuffer> status_buf = ensure_device_buffer_for_write_mm(status, c.device, status_bytes);
    if (out_buf == nil || status_buf == nil) {
      fail_(std::string(op) + ": failed to allocate Metal buffers");
    }

    id<MTLCommandBuffer> cb = create_command_buffer_mm(c.queue);
    if (cb == nil) {
      fail_(std::string(op) + ": failed to create command buffer");
    }

    MPSMatrixDecompositionCholesky* chol =
        [[MPSMatrixDecompositionCholesky alloc] initWithDevice:c.device lower:!upper order:n];
    MPSMatrixDescriptor* mat_desc = [MPSMatrixDescriptor matrixDescriptorWithRows:n
                                                                           columns:n
                                                                          rowBytes:n * sizeof(float)
                                                                          dataType:MPSDataTypeFloat32];
    MPSMatrix* mat = [[MPSMatrix alloc] initWithBuffer:out_buf
                                                offset:static_cast<NSUInteger>(out.storage_offset())
                                            descriptor:mat_desc];

    [chol encodeToCommandBuffer:cb sourceMatrix:mat resultMatrix:mat status:status_buf];

    submit_command_buffer_mm(cb, out.numel());
    mark_device_data_dirty_mm(out, cb);
    mark_device_data_dirty_mm(status, cb);
  }

  if (read_status_code_(status) != 0) {
    fail_(std::string(op) + ": matrix is not positive-definite");
  }
  // MPS writes only one triangle; normalize the opposite half to explicit zeros
  // for deterministic parity with CPU paths.
  sync_host_if_needed(out);
  auto* outp = reinterpret_cast<float*>(out.raw_mutable_data());
  for (std::int64_t i = 0; i < n64; ++i) {
    for (std::int64_t j = 0; j < n64; ++j) {
      if ((!upper && j > i) || (upper && i > j)) {
        outp[i * n64 + j] = 0.0f;
      }
    }
  }
  mark_host_data_dirty(out);
  return out;
}

bool supports_cholesky_solve(const Series& factor, const Series& b) noexcept {
  if (!supports_square_f32_(factor)) return false;
  return supports_rhs_f32_(b, factor.sizes()[0]);
}

Series cholesky_solve(const Series& factor, const Series& b, bool upper) {
  constexpr const char* op = "mps::linalg::cholesky_solve";
  auto& c = ctx_();
  if (!c.ready) {
    fail_(std::string(op) + ": backend unavailable (" + c.error + ")");
  }
  if (!supports_cholesky_solve(factor, b)) {
    fail_(std::string(op) + ": expected contiguous float32 MPS factor/rhs with shape [n,n] and [n] or [n,k]");
  }

  const auto n64 = factor.sizes()[0];
  const bool vector_rhs = (b.dim() == 1);
  const std::int64_t nrhs64 = vector_rhs ? 1 : b.sizes()[1];
  const NSUInteger n = static_cast<NSUInteger>(n64);
  const NSUInteger nrhs = static_cast<NSUInteger>(nrhs64);

  const Series rhs_mat = vector_rhs ? b.reshape({n64, 1}) : b;
  Series out_mat = Series::empty({n64, nrhs64}, ScalarType::Float32, Platform::MPS).contiguous();

  @autoreleasepool {
    const std::size_t fac_bytes = static_cast<std::size_t>(factor.numel()) * sizeof(float);
    const std::size_t rhs_bytes = static_cast<std::size_t>(rhs_mat.numel()) * sizeof(float);

    id<MTLBuffer> fac_buf = ensure_device_buffer_for_read_mm(factor, c.device, fac_bytes);
    id<MTLBuffer> rhs_buf = ensure_device_buffer_for_read_mm(rhs_mat, c.device, rhs_bytes);
    id<MTLBuffer> out_buf = ensure_device_buffer_for_write_mm(out_mat, c.device, rhs_bytes);
    if (fac_buf == nil || rhs_buf == nil || out_buf == nil) {
      fail_(std::string(op) + ": failed to allocate Metal buffers");
    }

    id<MTLCommandBuffer> cb = create_command_buffer_mm(c.queue);
    if (cb == nil) {
      fail_(std::string(op) + ": failed to create command buffer");
    }

    MPSMatrixSolveCholesky* solver =
        [[MPSMatrixSolveCholesky alloc] initWithDevice:c.device
                                                 upper:upper
                                                 order:n
                                numberOfRightHandSides:nrhs];
    MPSMatrixDescriptor* fac_desc = [MPSMatrixDescriptor matrixDescriptorWithRows:n
                                                                           columns:n
                                                                          rowBytes:n * sizeof(float)
                                                                          dataType:MPSDataTypeFloat32];
    MPSMatrixDescriptor* rhs_desc = [MPSMatrixDescriptor matrixDescriptorWithRows:n
                                                                           columns:nrhs
                                                                          rowBytes:nrhs * sizeof(float)
                                                                          dataType:MPSDataTypeFloat32];

    MPSMatrix* fac = [[MPSMatrix alloc] initWithBuffer:fac_buf
                                                offset:static_cast<NSUInteger>(factor.storage_offset())
                                            descriptor:fac_desc];
    MPSMatrix* rhs = [[MPSMatrix alloc] initWithBuffer:rhs_buf
                                                offset:static_cast<NSUInteger>(rhs_mat.storage_offset())
                                            descriptor:rhs_desc];
    MPSMatrix* out = [[MPSMatrix alloc] initWithBuffer:out_buf
                                                offset:static_cast<NSUInteger>(out_mat.storage_offset())
                                            descriptor:rhs_desc];

    [solver encodeToCommandBuffer:cb sourceMatrix:fac rightHandSideMatrix:rhs solutionMatrix:out];

    submit_command_buffer_mm(cb, out_mat.numel());
    mark_device_data_dirty_mm(out_mat, cb);
  }
  return vector_rhs ? out_mat.reshape({n64}) : out_mat;
}

} // namespace pluss::codem::native::mps
