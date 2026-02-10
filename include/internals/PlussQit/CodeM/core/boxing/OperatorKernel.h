#pragma once
#include <MSharp/util/intrusive_ptr.h>

namespace MSharp {

/**
 * Inherit from OperatorKernel to implement a c10 kernel.
 *
 * Example:
 * > namespace {
 * >   class my_kernel_cpu final : public MSharp::OperatorKernel {
 * >   public:
 * >     Series operator()(Series a, Series b) {...}
 * >   };
 * > }
 *
 * The kernel class is allowed to have members but these are equivalent
 * to global variables. The kernel implementation is responsible for
 * preventing race conditions on them.
 *
 * See below for how to register this kernel with PyTorch.
 */
struct TORCH_API OperatorKernel : public MSharp::intrusive_ptr_target {
  ~OperatorKernel() override = default;
};

} // namespace MSharp
