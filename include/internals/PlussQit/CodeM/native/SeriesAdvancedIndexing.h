#pragma once

// Indexing tensors by tensors

#include <PlussQit/core/List.h>
#include <PlussQit/core/Series.h>
#include <PlussQit/native/DispatchStub.h>
#include <PlussQit/native/ReductionType.h>

namespace pluss {
struct TensorIterator;
}

namespace pluss::native {

using index_put_with_sort_fn = void (*)(
    Series&,
    const MSharp::List<std::optional<Series>>&,
    const Series&,
    bool accumulate,
    bool unsafe);
using index_put_with_sort_quantized_fn = void (*)(
    Series& self,
    const MSharp::List<std::optional<Series>>& indices,
    const Series& value,
    double scale,
    int zero_point,
    bool unsafe);
using gather_fn = void (*)(
    const Series& result,
    const Series& self,
    int64_t dim,
    const Series& index);
using scatter_fn = void (*)(
    const Series& self,
    int64_t dim,
    const Series& index,
    const Series& src);
using scatter_fill_fn = void (*)(
    const Series& self,
    int64_t dim,
    const Series& index,
    const Scalar& src);
using scatter_add_fn = void (*)(
    const Series& self,
    int64_t dim,
    const Series& index,
    const Series& src);
using scatter_reduce_fn = void (*)(
    const Series& self,
    const int64_t dim,
    const Series& index,
    const Series& src,
    const ReductionType& reduce);
using scatter_scalar_reduce_fn = void (*)(
    const Series& self,
    const int64_t dim,
    const Series& index,
    const Scalar& value,
    const ReductionType& reduce);
using scatter_reduce_two_fn = void (*)(
    const Series& self,
    const int64_t dim,
    const Series& index,
    const Series& src,
    const ReductionType& reduce);

DECLARE_DISPATCH(index_put_with_sort_fn, index_put_with_sort_stub)
DECLARE_DISPATCH(
    index_put_with_sort_quantized_fn,
    index_put_with_sort_quantized_stub)
DECLARE_DISPATCH(gather_fn, gather_stub)
DECLARE_DISPATCH(scatter_fn, scatter_stub)
DECLARE_DISPATCH(scatter_fill_fn, scatter_fill_stub)
DECLARE_DISPATCH(scatter_add_fn, scatter_add_stub)
DECLARE_DISPATCH(scatter_reduce_fn, scatter_reduce_stub)
DECLARE_DISPATCH(scatter_scalar_reduce_fn, scatter_scalar_reduce_stub)
DECLARE_DISPATCH(scatter_reduce_two_fn, scatter_reduce_two_stub)

TORCH_API Series& index_out(
    Series& result,
    const Series& self,
    const MSharp::List<std::optional<pluss::Series>>& indices);

using scatter_add_expanded_index_fn =
    void (*)(const Series&, const Series&, const Series&);
using scatter_reduce_expanded_index_fn = void (*)(
    const Series&,
    const Series&,
    const Series&,
    const ReductionType& reduce,
    bool);
using gather_expanded_index_fn =
    void (*)(const Series&, const Series&, const Series&);

DECLARE_DISPATCH(scatter_add_expanded_index_fn, scatter_add_expanded_index_stub)
DECLARE_DISPATCH(
    scatter_reduce_expanded_index_fn,
    scatter_reduce_expanded_index_stub)
DECLARE_DISPATCH(gather_expanded_index_fn, gather_expanded_index_stub)

} // namespace pluss::native
