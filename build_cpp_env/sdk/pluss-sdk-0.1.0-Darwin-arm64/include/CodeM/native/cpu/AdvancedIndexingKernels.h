#pragma once
#include "CodeM/core/SeriesSlim.h"
#include "MSharp/RangeView.h"

namespace pluss::codem::native::cpu {
  Series index(const Series& a, pluss::msharp::RangeView::Of<Series> indices);
  Series index_put(const Series& a, pluss::msharp::RangeView::Of<Series> indices, const Series& values, bool accumulate);
  // dst[dst_off] += src[src_off] for every arithmetic dtype (float16/bfloat16 accumulate in float).
  void add_inplace_at(ScalarType st, std::byte* dst_raw, const std::byte* src_raw, std::int64_t dst_off, std::int64_t src_off);
  void index_put_inplace(Series& a, pluss::msharp::RangeView::Of<Series> indices, const Series& values, bool accumulate);
} // namespace pluss::codem::native::cpu
