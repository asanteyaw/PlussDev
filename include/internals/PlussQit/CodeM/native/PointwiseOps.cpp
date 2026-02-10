// Ternary and higher-order pointwise operations
#define TORCH_ASSERT_ONLY_METHOD_OPERATORS
#include <PlussQit/native/PointwiseOps.h>

#include <PlussQit/core/Series.h>
#include <PlussQit/TensorMeta.h>

#ifndef AT_PER_OPERATOR_HEADERS
#include <PlussQit/NativeFunctions.h>
#else
#include <PlussQit/ops/addcdiv_native.h>
#include <PlussQit/ops/addcmul_native.h>
#endif

namespace pluss::meta {

TORCH_META_FUNC(addcmul)
(const Series& self,
 const Series& tensor1,
 const Series& tensor2,
 const Scalar& value) {
  build(TensorIteratorConfig()
      .allow_cpu_scalars(true)
      .promote_inputs_to_common_dtype(true)
      .cast_common_dtype_to_outputs(true)
      .enforce_safe_casting_to_output(true)
      .add_owned_output(maybe_get_output())
      .add_owned_const_input(self)
      .add_owned_const_input(tensor1)
      .add_owned_const_input(tensor2));
}

TORCH_META_FUNC(addcdiv)
(const Series& self,
 const Series& tensor1,
 const Series& tensor2,
 const Scalar& value) {
  if (isIntegralType(tensor1.scalar_type(), /*includeBool=*/true) &&
      isIntegralType(tensor2.scalar_type(), /*includeBool=*/true)) {
    TORCH_CHECK(
        false,
        "Integer division with addcdiv is no longer supported, and in a future  ",
        "release addcdiv will perform a true division of tensor1 and tensor2. ",
        "The historic addcdiv behavior can be implemented as ",
        "(input + value * torch.trunc(tensor1 / tensor2)).to(input.dtype) ",
        "for integer inputs and as ",
        "(input + value * tensor1 / tensor2) for float inputs. ",
        "The future addcdiv behavior is just the latter implementation: ",
        "(input + value * tensor1 / tensor2), for all dtypes.");
  }
  build_ternary_op(maybe_get_output(), self, tensor1, tensor2);
}

} // namespace pluss::meta
namespace pluss::native {

TORCH_IMPL_FUNC(addcmul_out)
(const Series& self,
 const Series& tensor1,
 const Series& tensor2,
 const Scalar& value,
 const Series& result) {
  addcmul_stub(device_type(), *this, value);
}

TORCH_IMPL_FUNC(addcdiv_out)
(const Series& self,
 const Series& tensor1,
 const Series& tensor2,
 const Scalar& value,
 const Series& result) {
  addcdiv_stub(device_type(), *this, value);
}

DEFINE_DISPATCH(addcmul_stub);
DEFINE_DISPATCH(addcdiv_stub);

} // namespace pluss::native
