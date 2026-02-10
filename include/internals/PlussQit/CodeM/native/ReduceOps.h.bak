#pragma once

#include <PlussQit/native/DispatchStub.h>
#include <MSharp/util/ArrayRef.h>
#include <optional>

namespace MSharp {
class Scalar;
}

namespace pluss {
struct TensorIterator;
class Series;
}

namespace pluss::native {

using reduce_fn = void(*)(TensorIterator &);

DECLARE_DISPATCH(reduce_fn, sum_stub)
DECLARE_DISPATCH(reduce_fn, nansum_stub)
DECLARE_DISPATCH(reduce_fn, prod_stub)
DECLARE_DISPATCH(reduce_fn, mean_stub)
DECLARE_DISPATCH(reduce_fn, and_stub)
DECLARE_DISPATCH(reduce_fn, or_stub)
DECLARE_DISPATCH(reduce_fn, min_values_stub)
DECLARE_DISPATCH(reduce_fn, max_values_stub)
DECLARE_DISPATCH(reduce_fn, argmax_stub)
DECLARE_DISPATCH(reduce_fn, argmin_stub)

using reduce_std_var_function =
    void (*)(TensorIterator&, double correction, bool take_sqrt);
DECLARE_DISPATCH(reduce_std_var_function, std_var_stub)

using reduce_norm_fn =
    void (*)(Series&, const Series&, const MSharp::Scalar&, std::optional<int64_t>);
DECLARE_DISPATCH(reduce_norm_fn, norm_kernel)

using reduce_fn_flag = void(*)(TensorIterator &, const MSharp::Scalar&);
DECLARE_DISPATCH(reduce_fn_flag, norm_stub)

using structured_cum_fn = void (*)(const Series&, const Series&, int64_t);
using cum_fn = void (*)(Series&, const Series&, int64_t);
DECLARE_DISPATCH(structured_cum_fn, cumsum_stub)
DECLARE_DISPATCH(structured_cum_fn, cumprod_stub)
DECLARE_DISPATCH(cum_fn, logcumsumexp_stub)

DECLARE_DISPATCH(void (*)(const Series&, int64_t, bool, Series&, Series&), aminmax_stub)
DECLARE_DISPATCH(void (*)(const Series&, Series&, Series&), aminmax_allreduce_stub)

// Used in cuda/Normalization.cu
TORCH_API std::tuple<Series&,Series&> var_mean_out(
    Series &result1, Series &result2, const Series &self, IntArrayRef dim,
    int64_t correction, bool keepdim);

} // namespace pluss::native
