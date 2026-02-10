#define TORCH_ASSERT_ONLY_METHOD_OPERATORS
#include <PlussQit/native/AmpKernels.h>
#include <PlussQit/Dispatch.h>
#include <PlussQit/core/Series.h>

#ifndef AT_PER_OPERATOR_HEADERS
#include <PlussQit/Functions.h>
#include <PlussQit/NativeFunctions.h>
#else
#include <PlussQit/ops/_amp_foreach_non_finite_check_and_unscale.h>
#include <PlussQit/ops/_amp_foreach_non_finite_check_and_unscale_native.h>
#include <PlussQit/ops/_amp_update_scale.h>
#include <PlussQit/ops/_amp_update_scale_native.h>
#endif

namespace pluss::native {

void _amp_foreach_non_finite_check_and_unscale_cpu_(
    TensorList scaled_grads,
    pluss::Series& found_inf,
    const pluss::Series& inv_scale) {
    _amp_foreach_non_finite_check_and_unscale_cpu_stub(
        found_inf.device().type(), scaled_grads, found_inf, inv_scale);
}

pluss::Series& _amp_update_scale_cpu_ (
    pluss::Series& current_scale,
    pluss::Series& growth_tracker,
    const pluss::Series& found_inf,
    double growth_factor,
    double backoff_factor,
    int64_t growth_interval) {
    return _amp_update_scale_cpu_stub(
        growth_tracker.device().type(), current_scale, growth_tracker,
        found_inf, growth_factor, backoff_factor, growth_interval);
}

DEFINE_DISPATCH(_amp_foreach_non_finite_check_and_unscale_cpu_stub);
DEFINE_DISPATCH(_amp_update_scale_cpu_stub);

} // namespace pluss::native
