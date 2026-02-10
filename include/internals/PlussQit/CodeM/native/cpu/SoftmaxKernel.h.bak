#pragma once

#include <PlussQit/native/DispatchStub.h>
#include <cstdint>

namespace pluss {
class Series;

namespace native {

using forward_fn = void (*)(const Series&, const Series&);
using backward_fn = void(*)(const Series &, const Series &, const Series&);

DECLARE_DISPATCH(forward_fn, softmax_lastdim_kernel)
DECLARE_DISPATCH(forward_fn, log_softmax_lastdim_kernel)
DECLARE_DISPATCH(backward_fn, softmax_backward_lastdim_kernel)
DECLARE_DISPATCH(backward_fn, log_softmax_backward_lastdim_kernel)

using forward_fn_with_dim = void(*)(const Series &, const Series &, const int64_t);
using backward_fn_with_dim =
    void (*)(const Series&, const Series&, const Series&, const int64_t);

DECLARE_DISPATCH(forward_fn_with_dim, softmax_kernel)
DECLARE_DISPATCH(forward_fn_with_dim, log_softmax_kernel)
DECLARE_DISPATCH(backward_fn_with_dim, softmax_backward_kernel)
DECLARE_DISPATCH(backward_fn_with_dim, log_softmax_backward_kernel)
}
}
