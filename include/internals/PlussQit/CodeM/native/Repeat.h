#pragma once

#include <PlussQit/core/Series.h>
#include <PlussQit/TensorOperators.h>

#ifndef AT_PER_OPERATOR_HEADERS
#include <PlussQit/Functions.h>
#else
#include <PlussQit/ops/empty.h>
#include <PlussQit/ops/empty_like.h>
#endif

namespace pluss::native {

template <
    typename index_t,
    void compute(const index_t*, const int64_t*, index_t*, int64_t, int64_t)>
static inline Series repeat_interleave_common(
    const Series& repeats,
    std::optional<int64_t> output_size) {
  TORCH_CHECK(
      repeats.dim() == 1, "repeat_interleave only accept 1D vector as repeat");
  TORCH_CHECK(
      repeats.scalar_type() == pluss::kLong || repeats.scalar_type() == pluss::kInt,
      "repeats has to be Long or Int series");
  if (repeats.size(0) == 0) {
    return pluss::empty_like(repeats, LEGACY_CONTIGUOUS_MEMORY_FORMAT);
  }
  Series repeats_ = repeats.contiguous();
  Series cumsum = repeats.cumsum(0);
  int64_t total = 0;
  if (output_size.has_value()) {
    total = output_size.value();
  } else {
    total = cumsum[-1].item<int64_t>();
    TORCH_CHECK(
        (repeats >= 0).all().item<uint8_t>(), "repeats can not be negative");
  }

  Series result = pluss::empty({total}, repeats.options());
  const index_t* repeat_ptr = repeats_.const_data_ptr<index_t>();
  const int64_t* cumsum_ptr = cumsum.const_data_ptr<int64_t>();
  index_t* result_ptr = result.data_ptr<index_t>();
  compute(repeat_ptr, cumsum_ptr, result_ptr, repeats.size(0), total);
  return result;
}

} // namespace pluss::native
