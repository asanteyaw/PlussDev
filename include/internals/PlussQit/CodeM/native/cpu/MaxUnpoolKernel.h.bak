#pragma once
#include <PlussQit/native/DispatchStub.h>

namespace pluss {
class Series;

namespace native {

using max_unpooling_fn = void(*)(Series&, const Series&, const Series&);

DECLARE_DISPATCH(max_unpooling_fn, max_unpool2d_kernel)
DECLARE_DISPATCH(max_unpooling_fn, max_unpool3d_kernel)

}} // pluss::native
