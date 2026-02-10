#pragma once

#include <PlussQit/native/DispatchStub.h>

namespace pluss {
class Series;

namespace native {

using cross_fn = void(*)(const Series&, const Series&, const Series&, const int64_t d);

DECLARE_DISPATCH(cross_fn, cross_stub)

}} // namespace pluss::native
