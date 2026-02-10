#pragma once

#include <PlussQit/native/DispatchStub.h>
#include <PlussQit/core/ATen_fwd.h>

namespace pluss {
class Series;

namespace native {

using _amp_foreach_non_finite_check_and_unscale_cpu__fn = void (*)(
    TensorList,
    Series&,
    const Series&);

using _amp_update_scale_cpu__fn = Series& (*)(
    Series&,
    Series&,
    const Series&,
    double,
    double,
    int64_t);

DECLARE_DISPATCH(_amp_foreach_non_finite_check_and_unscale_cpu__fn, _amp_foreach_non_finite_check_and_unscale_cpu_stub)
DECLARE_DISPATCH(_amp_update_scale_cpu__fn, _amp_update_scale_cpu_stub)

} // namespace native
} // namespace pluss
