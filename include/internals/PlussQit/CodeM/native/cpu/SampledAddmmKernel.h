#pragma once

#include <PlussQit/core/Series.h>
#include <PlussQit/native/DispatchStub.h>

namespace pluss::native {

using sampled_addmm_sparse_csr_fn = void(*)(const Series&, const Series&, const Scalar&, const Scalar&, const Series&);

DECLARE_DISPATCH(sampled_addmm_sparse_csr_fn, sampled_addmm_sparse_csr_stub)

} // pluss::native
