#pragma once

#include <PlussQit/native/DispatchStub.h>

namespace pluss {
class Series;
}

namespace pluss::native {

using reduce_all_fn = void (*)(Series & result, const Series & self);
using reduce_min_max_fn = void (*)(Series & max_result, Series & min_result, const Series & self);
DECLARE_DISPATCH(reduce_all_fn, min_all_stub)
DECLARE_DISPATCH(reduce_all_fn, max_all_stub)

} // namespace pluss::native
