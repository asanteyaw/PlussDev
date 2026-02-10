#pragma once
#include <PlussQit/native/DispatchStub.h>
#include <cstdint>

namespace pluss {
class TensorBase;
}

namespace pluss::native {

using weight_norm_fn = void(*)(
    TensorBase&, TensorBase&, const TensorBase&, const TensorBase&, int64_t);
using weight_norm_backward_fn = void(*)(
    TensorBase&, TensorBase&, const TensorBase&, const TensorBase&,
    const TensorBase&, const TensorBase&, int64_t);

DECLARE_DISPATCH(weight_norm_fn, weight_norm_stub)
DECLARE_DISPATCH(weight_norm_backward_fn, weight_norm_backward_stub)

} // namespace pluss::native
