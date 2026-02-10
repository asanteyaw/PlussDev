#pragma once

#include <PlussQit/native/DispatchStub.h>

namespace pluss {
class Series;

namespace native {

using pdist_forward_fn = void(*)(Series&, const Series&, const double p);
using pdist_backward_fn = void(*)(Series&, const Series&, const Series&, const double p, const Series&);
using cdist_fn = void(*)(Series&, const Series&, const Series&, const double p);
using cdist_backward_fn = void(*)(Series&, const Series&, const Series&, const Series&, const double p, const Series&);

DECLARE_DISPATCH(pdist_forward_fn, pdist_forward_stub)
DECLARE_DISPATCH(pdist_backward_fn, pdist_backward_stub)
DECLARE_DISPATCH(cdist_fn, cdist_stub)
DECLARE_DISPATCH(cdist_backward_fn, cdist_backward_stub)

}} // namespace pluss::native
