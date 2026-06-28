#pragma once
#include "CodeM/core/SeriesSlim.h"
#include "MSharp/RangeView.h"

namespace pluss::codem::native::cpu {
  Series index(const Series& a, pluss::msharp::RangeView::Of<Series> indices);
  Series index_put(const Series& a, pluss::msharp::RangeView::Of<Series> indices, const Series& values, bool accumulate);
  void index_put_inplace(Series& a, pluss::msharp::RangeView::Of<Series> indices, const Series& values, bool accumulate);
} // namespace pluss::codem::native::cpu
