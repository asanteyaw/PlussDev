#include <PlussQit/core/Series.h>
#include <PlussQit/native/DispatchStub.h>

namespace pluss::native {

enum class ADAM_MODE : uint8_t { ORIGINAL = 0, ADAMW = 1 };

using fused_adam_fn = void (*)(
    const pluss::Series& param,
    const pluss::Series& grad,
    const pluss::Series& exp_avg,
    const pluss::Series& exp_avg_sq,
    const pluss::Series& max_exp_avg_sq,
    const pluss::Series& state_step,
    const double lr,
    const double beta1,
    const double beta2,
    const double weight_decay,
    const double eps,
    const bool amsgrad,
    const bool maximize,
    const float* grad_scale_ptr,
    const ADAM_MODE);

DECLARE_DISPATCH(fused_adam_fn, fused_adam_stub)

} // namespace pluss::native
