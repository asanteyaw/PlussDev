#pragma once

#include <PlussQit/native/DispatchStub.h>
#include <PlussQit/native/ReductionType.h>
#include <MSharp/core/Scalar.h>
#include <optional>

namespace pluss {
class Series;

namespace native {

using segment_reduce_lengths_fn = Series (*)(
    ReductionType,
    const Series&,
    const Series&,
    int64_t,
    const std::optional<Scalar>&);
DECLARE_DISPATCH(segment_reduce_lengths_fn, _segment_reduce_lengths_stub)

using segment_reduce_offsets_fn = Series (*)(
    ReductionType,
    const Series&,
    const Series&,
    int64_t,
    const std::optional<Scalar>&);
DECLARE_DISPATCH(segment_reduce_offsets_fn, _segment_reduce_offsets_stub)

using segment_reduce_lengths_backward_fn = Series (*)(
    const Series&,
    const Series&,
    const Series&,
    ReductionType,
    const Series&,
    int64_t,
    const std::optional<Scalar>&);
DECLARE_DISPATCH(segment_reduce_lengths_backward_fn, _segment_reduce_lengths_backward_stub)

using segment_reduce_offsets_backward_fn = Series (*)(
    const Series&,
    const Series&,
    const Series&,
    ReductionType,
    const Series&,
    int64_t,
    const std::optional<Scalar>&);
DECLARE_DISPATCH(segment_reduce_offsets_backward_fn, _segment_reduce_offsets_backward_stub)

} // namespace native
} // namespace pluss
