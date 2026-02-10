#pragma once

#include <PlussQit/native/DispatchStub.h>

namespace pluss {

class Series;
struct TensorIterator;
class TensorBase;

namespace native {

using copy_fn = void (*)(TensorIterator&, bool non_blocking);

DECLARE_DISPATCH(copy_fn, copy_stub)

TORCH_API void copy_ignoring_overlaps(const TensorBase &dst, const TensorBase &src);

} // namespace native
} // namespace pluss
