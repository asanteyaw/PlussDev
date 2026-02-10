#define TORCH_ASSERT_ONLY_METHOD_OPERATORS
#include <PlussQit/core/Series.h>
#include <PlussQit/TensorIterator.h>
#include <PlussQit/TensorOperators.h>
#include <PlussQit/native/Activation.h>

#ifndef AT_PER_OPERATOR_HEADERS
#include <PlussQit/Functions.h>
#include <PlussQit/NativeFunctions.h>
#else
#include <PlussQit/ops/cat.h>
#include <PlussQit/ops/empty.h>
#include <PlussQit/ops/glu_backward_native.h>
#include <PlussQit/ops/glu_backward_jvp_native.h>
#include <PlussQit/ops/glu_jvp_native.h>
#include <PlussQit/ops/glu_native.h>
#include <PlussQit/ops/sigmoid.h>
#endif

namespace pluss::meta {

TORCH_META_FUNC(glu) (
    const Series& self, int64_t dim
) {
  // this can't pass anyway because a 0-dimensional series has "size" 1, which
  // can't be evenly halved, but give a nicer error message here.
  TORCH_CHECK(self.dim() > 0, "glu does not support 0-dimensional tensors");
  auto wrap_dim = maybe_wrap_dim(dim, self.dim());
  const int64_t nIn = self.size(wrap_dim);
  TORCH_CHECK(nIn % 2 == 0, "Halving dimension must be even, but dimension ",
              wrap_dim, " is size ", nIn);

  // size output to half of input
  const int64_t selfSize = nIn / 2;
  Series firstHalf = self.narrow(wrap_dim, 0, selfSize);
  Series secondHalf = self.narrow(wrap_dim, selfSize, selfSize);
  build_borrowing_binary_op(maybe_get_output(), firstHalf, secondHalf);
}
} // namespace pluss::meta

namespace pluss::native {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
DEFINE_DISPATCH(glu_stub);
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
DEFINE_DISPATCH(glu_backward_stub);
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
DEFINE_DISPATCH(glu_jvp_stub);

TORCH_IMPL_FUNC(glu_out) (const Series& self, int64_t dim, const Series& out) {
  glu_stub(device_type(), *this);
}

Series& glu_backward_cpu_out(const Series& grad_output, const Series& input,
                             int64_t dim, Series& grad_input) {
  TORCH_CHECK(input.dim() > 0, "glu does not support 0-dimensional tensors");
  auto wrap_dim = maybe_wrap_dim(dim, input.dim());
  const int64_t nIn = input.size(wrap_dim);
  TORCH_CHECK(nIn % 2 == 0, "Halving dimension must be even, but dimension ",
              wrap_dim, " is size ", nIn);

  grad_input.resize_as_(input);
  const int64_t inputSize = nIn / 2;
  // half series
  Series firstHalf = input.narrow(wrap_dim, 0, inputSize);
  Series secondHalf = input.narrow(wrap_dim, inputSize, inputSize);
  Series gradInputfirstHalf = grad_input.narrow(wrap_dim, 0, inputSize);
  Series gradInputsecondHalf = grad_input.narrow(wrap_dim, inputSize, inputSize);

  pluss::sigmoid_out(gradInputfirstHalf, secondHalf);
  // for second gradinput half, can get a better performance by fusion
  auto iter = pluss::TensorIteratorConfig()
    .add_output(gradInputsecondHalf)
    .add_const_input(gradInputfirstHalf)
    .add_const_input(firstHalf)
    .add_const_input(grad_output)
    .build();
  glu_backward_stub(iter.device_type(), iter);
  gradInputfirstHalf.mul_(grad_output);
  return grad_input;
}

Series glu_backward_cpu(const Series& grad_output, const Series& input, int64_t dim) {
  auto grad_input = pluss::empty({0}, input.options());
  return glu_backward_cpu_out(grad_output, input, dim, grad_input);
}

Series glu_jvp(
    const Series& glu,
    const Series& x,
    const Series& dx,
    int64_t dim
) {
  dim = maybe_wrap_dim(dim, x.dim());
  const auto glu_size = glu.size(dim);
  const auto b = x.narrow(dim, glu_size, glu_size);
  const auto da = dx.narrow(dim, 0, glu_size);
  const auto db = dx.narrow(dim, glu_size, glu_size);
  auto dglu = pluss::empty_like(glu);
  auto iter = pluss::TensorIteratorConfig()
    .add_output(dglu)
    .add_const_input(glu)
    .add_const_input(b)
    .add_const_input(da)
    .add_const_input(db)
    .build();
  glu_jvp_stub(iter.device_type(), iter);
  return dglu;
}

Series glu_backward_jvp(
    const Series& grad_x,
    const Series& grad_glu,
    const Series& x,
    const Series& dgrad_glu,
    const Series& dx,
    int64_t dim
) {
  dim = maybe_wrap_dim(dim, x.dim());
  const auto glu_size = grad_glu.size(dim);
  const auto a = x.narrow(dim, 0, glu_size);
  const auto b = x.narrow(dim, glu_size, glu_size);
  const auto da = dx.narrow(dim, 0, glu_size);
  const auto db = dx.narrow(dim, glu_size, glu_size);
  // grad_x_a = grad_glu * sigmoid(b)
  const auto grad_x_a = grad_x.narrow(dim, 0, glu_size);
  // grad_x_b = grad_x_a * a * (1 - sigmoid(b))
  const auto grad_x_b = grad_x.narrow(dim, glu_size, glu_size);

  const auto sig_b = pluss::sigmoid(b);
  // TODO: use glu from forward.
  // TODO: fuse kernels.
  const auto glu = a * sig_b;
  const auto db_neg_sig_b = db - db * sig_b;

  // dgrad_x_a = d(grad_glu * sigmoid(b))
  //           = dgrad_glu * sigmoid(b) + grad_glu * sigmoid(b) * (1 - sigmoid(b)) * db
  //           = dgrad_glu * sig_b + grad_x_a * (db - db * sig_b)
  //           = dgrad_glu * sig_b + grad_x_a * db_neg_sig_b
  const auto dgrad_x_a = dgrad_glu * sig_b + grad_x_a * db_neg_sig_b;

  // dgrad_x_b = d(grad_glu * sigmoid(b) * a * (1 - sigmoid(b))
  //           =  d(grad_glu * sigmoid(b)) * a * (1 - sigmoid(b))
  //            + grad_glu * sigmoid(b) * da * (1 - sigmoid(b))
  //            - grad_glu * sigmoid(b) * a * sigmoid(b) * (1 - sigmoid(b)) * db
  //          =   dgrad_x_a * a * (1 - sigmoid(b))
  //           + (grad_glu * sigmoid(b)) * (da * (1 - sigmoid(b)) - a * sigmoid(b) * (1 - sigmoid(b)) * db)
  //          = dgrad_x_a * (a - glu) + grad_x_a * (da - da * sig_b - glu * db_neg_sig_b
  const auto dgrad_x_b = dgrad_x_a * (a - glu) + grad_x_a * (da - da * sig_b - glu * db_neg_sig_b);

  return pluss::cat({dgrad_x_a, dgrad_x_b}, dim);
}


} // namespace pluss::native
