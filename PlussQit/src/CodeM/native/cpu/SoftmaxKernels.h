#pragma once
#include "CodeM/core/Series.h"

namespace pluss::codem::native::cpu {
  // softmax/log_softmax over dim for floating dtypes (float/half/bf16/double). Complex unsupported.
  Series softmax(const Series& a, std::int64_t dim);
  Series log_softmax(const Series& a, std::int64_t dim);
  Series logsumexp(const Series& a, std::int64_t dim, bool keepdim=false);
}
