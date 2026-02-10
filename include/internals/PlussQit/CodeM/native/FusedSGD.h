#include <PlussQit/core/Series.h>
#include <PlussQit/native/DispatchStub.h>

namespace pluss::native {

using fused_sgd_fn = void (*)(
    const pluss::Series& param,
    const pluss::Series& grad,
    const pluss::Series& momentum_buffer,
    const double weight_decay,
    const double momentum,
    const double lr,
    const double dampening,
    const bool nesterov,
    const bool maximize,
    const bool is_first_step,
    const float* grad_scale_ptr);

DECLARE_DISPATCH(fused_sgd_fn, fused_sgd_stub)

} // namespace pluss::native
