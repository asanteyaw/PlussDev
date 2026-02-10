#pragma once
#include <PlussQit/native/DispatchStub.h>

namespace pluss {
class TensorBase;
}

namespace pluss::native {

using pixel_shuffle_fn = void(*)(TensorBase&, const TensorBase&, int64_t);
DECLARE_DISPATCH(pixel_shuffle_fn, pixel_shuffle_kernel)
DECLARE_DISPATCH(pixel_shuffle_fn, pixel_unshuffle_kernel)

} // pluss::native
