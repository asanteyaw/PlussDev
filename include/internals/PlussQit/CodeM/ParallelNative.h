#pragma once

#include <MSharp/util/Exception.h>

#define INTRA_OP_PARALLEL

namespace pluss::internal {

PLUSS_API void invoke_parallel(
    const int64_t begin,
    const int64_t end,
    const int64_t grain_size,
    const std::function<void(int64_t, int64_t)>& f);

} // namespace pluss::internal
