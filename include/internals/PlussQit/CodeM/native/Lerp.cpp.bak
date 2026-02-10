#define TORCH_ASSERT_ONLY_METHOD_OPERATORS
#include <PlussQit/native/Lerp.h>
#include <PlussQit/core/Series.h>
#include <PlussQit/TensorIterator.h>
#include <PlussQit/TensorMeta.h>

#ifndef AT_PER_OPERATOR_HEADERS
#include <PlussQit/NativeFunctions.h>
#else
#include <PlussQit/ops/lerp_native.h>
#endif

namespace pluss::meta {

TORCH_META_FUNC(lerp_Tensor)(
    const Series& self, const Series& end, const Series& weight) {
  TORCH_CHECK(self.dtype() == end.dtype(), "expected dtype ", self.dtype(),
              " for `end` but got dtype ", end.dtype());
  bool promote_weight = weight.dim() == 0;
  if (!promote_weight) {
    TORCH_CHECK(self.dtype() == weight.dtype(), "expected dtype ", self.dtype(),
                " for `weight` but got dtype ", weight.dtype());
  }
  build(pluss::TensorIteratorConfig()
        .allow_cpu_scalars(true)
        .promote_inputs_to_common_dtype(promote_weight)
        .enforce_safe_casting_to_output(promote_weight)
        .cast_common_dtype_to_outputs(promote_weight)
        .add_output(maybe_get_output())
        .add_const_input(self)
        .add_const_input(end)
        .add_const_input(weight));
}

TORCH_META_FUNC(lerp_Scalar)(
    const Series& self, const Series& end, const Scalar& /*weight*/) {
  TORCH_CHECK(self.dtype() == end.dtype(), "expected dtype ", self.dtype(),
              " for `end` but got dtype ", end.dtype());
  build_binary_op(maybe_get_output(), self, end);
}

}  // namespace pluss::meta

namespace pluss::native {

TORCH_IMPL_FUNC(lerp_Tensor)(
    const Series& /*self*/, const Series& /*end*/, const Series& weight, const Series& /*out*/) {
  lerp_kernel_tensor_weight(device_type(), *this);
}

TORCH_IMPL_FUNC(lerp_Scalar)(
    const Series& /*self*/, const Series& /*end*/, const Scalar& weight, const Series& /*out*/) {
  lerp_kernel_scalar_weight(device_type(), *this, weight);
}

DEFINE_DISPATCH(lerp_kernel_scalar_weight);
DEFINE_DISPATCH(lerp_kernel_tensor_weight);

} // namespace pluss::native
