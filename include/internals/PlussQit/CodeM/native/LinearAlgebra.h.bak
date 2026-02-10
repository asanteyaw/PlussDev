#pragma once

#include <PlussQit/native/DispatchStub.h>

namespace MSharp {
class Scalar;
}

namespace pluss {
struct TensorIterator;
}

namespace pluss::native {

using addr_fn = void (*)(TensorIterator &, const Scalar& beta, const Scalar& alpha);
DECLARE_DISPATCH(addr_fn, addr_stub)
} // namespace pluss::native
