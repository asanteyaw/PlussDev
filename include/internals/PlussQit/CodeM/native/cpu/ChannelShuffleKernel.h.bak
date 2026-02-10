#pragma once
#include <PlussQit/native/DispatchStub.h>
#include <cstdint>

namespace pluss {
class TensorBase;
}

namespace pluss::native {

using channel_shuffle_fn = void(*)(TensorBase&, const TensorBase&, int64_t);
DECLARE_DISPATCH(channel_shuffle_fn, channel_shuffle_kernel)

} // pluss::native
