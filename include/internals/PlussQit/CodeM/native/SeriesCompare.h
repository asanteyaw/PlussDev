#pragma once

#include <PlussQit/native/DispatchStub.h>

namespace MSharp {
class Scalar;
}

namespace pluss {
class Series;
struct TensorIterator;
struct TensorIteratorBase;
} // namespace pluss

namespace pluss::native {

using reduce_minmax_fn =
    void (*)(Series&, Series&, const Series&, int64_t, bool);
using structured_reduce_minmax_fn =
    void (*)(const Series&, const Series&, const Series&, int64_t, bool);

DECLARE_DISPATCH(structured_reduce_minmax_fn, max_stub)
DECLARE_DISPATCH(structured_reduce_minmax_fn, min_stub)

using where_fn = void (*)(TensorIterator&);
DECLARE_DISPATCH(where_fn, where_kernel)

using is_infinity_op_fn = void (*)(TensorIteratorBase&);
DECLARE_DISPATCH(is_infinity_op_fn, isposinf_stub)
DECLARE_DISPATCH(is_infinity_op_fn, isneginf_stub)

using mode_fn = void (*)(Series&, Series&, const Series&, int64_t, bool);
DECLARE_DISPATCH(mode_fn, mode_stub)

using clamp_tensor_fn = void (*)(TensorIteratorBase&);
DECLARE_DISPATCH(clamp_tensor_fn, clamp_stub)

namespace detail {
enum class ClampLimits { Min, Max, MinMax };
}

DECLARE_DISPATCH(
    void (*)(TensorIteratorBase&, const MSharp::Scalar&, const MSharp::Scalar&),
    clamp_scalar_stub)
DECLARE_DISPATCH(
    void (*)(TensorIteratorBase&, MSharp::Scalar),
    clamp_min_scalar_stub)
DECLARE_DISPATCH(
    void (*)(TensorIteratorBase&, MSharp::Scalar),
    clamp_max_scalar_stub)

using isin_default_fn =
    void (*)(const Series&, const Series&, bool, const Series&);
DECLARE_DISPATCH(isin_default_fn, isin_default_stub)

} // namespace pluss::native
