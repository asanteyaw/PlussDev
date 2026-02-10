#define TORCH_ASSERT_ONLY_METHOD_OPERATORS
#include <PlussQit/native/UnfoldBackward.h>

#ifndef AT_PER_OPERATOR_HEADERS
#include <PlussQit/Functions.h>
#include <PlussQit/NativeFunctions.h>
#else
#include <PlussQit/ops/empty.h>
#include <PlussQit/ops/unfold_backward_native.h>
#include <PlussQit/ops/zeros.h>
#endif

namespace pluss::native {

DEFINE_DISPATCH(unfold_backward_stub);

Series unfold_backward(
  const Series& grad,
  IntArrayRef input_sizes,
  int64_t dim,
  int64_t size,
  int64_t step
) {
  auto grad_input = pluss::zeros(input_sizes, grad.options());
  if (step >= size) {
    auto gI_unfolded = grad_input.unfold(dim, size, step);
    gI_unfolded.copy_(grad);
    return grad_input;
  }

  unfold_backward_stub(
    grad.device().type(),
    grad_input,
    grad,
    dim, size, step
  );

  return grad_input;
}

} // namespace pluss::native
