#include <PlussQit/core/Series.h>
#include <PlussQit/native/DispatchStub.h>

namespace pluss::native {

using fused_adagrad_fn = void (*)(
    const pluss::Series& param,
    const pluss::Series& grad,
    const pluss::Series& state_sum,
    const pluss::Series& state_step,
    const double lr,
    const double lr_decay,
    const double weight_decay,
    const double eps,
    const bool maximize,
    const float* grad_scale_ptr);

DECLARE_DISPATCH(fused_adagrad_fn, fused_adagrad_stub)

} // namespace pluss::native
