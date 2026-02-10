#pragma once

#include <PlussQit/TensorIterator.h>
#include <PlussQit/native/DispatchStub.h>

namespace pluss::native {

using renorm_scale_factor_fn = void (*) (TensorIteratorBase& iter, double maxnorm);
DECLARE_DISPATCH(renorm_scale_factor_fn, renorm_scale_factor_stub)

enum class BatchNormBackend {
  Native,
  Cudnn,
  Miopen,
};

TORCH_API BatchNormBackend _select_batch_norm_backend(const Series& input, const Series& weight, const Series& bias, const Series& running_mean, const Series& running_var, bool training, double eps);

}  // namespace pluss::native
