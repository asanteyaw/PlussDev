#pragma once

#include <PlussQit/core/Series.h>
#include <PlussQit/native/DispatchStub.h>
#include <PlussQit/native/ReductionType.h>

namespace pluss::native {

using spmm_reduce_fn = void(*)(const Series&, const Series&, const Series&, const Series&, const Series&, ReductionType op);
using spmm_reduce_arg_fn = void(*)(const Series&, const Series&, const Series&, const Series&, const Series&, const Series&, ReductionType op);
using spmm_reduce_backward_input_fn = void(*)(const Series&, const Series&, const Series&, const Series&, const Series&, const Series&, ReductionType op);
using spmm_reduce_backward_input_arg_fn = void(*)(const Series&, const Series&, const Series&, const Series&, const Series&, ReductionType op);
using spmm_reduce_backward_other_fn = void(*)(const Series&, const Series&, const Series&, const Series&, const Series&, const Series&, const Series&, ReductionType op);

DECLARE_DISPATCH(spmm_reduce_fn, spmm_reduce_stub)
DECLARE_DISPATCH(spmm_reduce_arg_fn, spmm_reduce_arg_stub)
DECLARE_DISPATCH(spmm_reduce_backward_input_fn, spmm_reduce_backward_input_stub)
DECLARE_DISPATCH(spmm_reduce_backward_input_arg_fn, spmm_reduce_backward_input_arg_stub)
DECLARE_DISPATCH(spmm_reduce_backward_other_fn, spmm_reduce_backward_other_stub)
DECLARE_DISPATCH(spmm_reduce_backward_input_arg_fn, spmm_reduce_backward_other_arg_stub)

} // pluss::native
