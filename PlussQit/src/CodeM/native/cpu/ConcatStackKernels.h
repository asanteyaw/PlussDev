#pragma once
#include "CodeM/core/SeriesSlim.h"
#include "MSharp/RangeView.h"

namespace pluss::codem::native::cpu {
  Series cat(pluss::msharp::RangeView::Of<Series> tensors, std::int64_t dim);
  Series stack(pluss::msharp::RangeView::Of<Series> tensors, std::int64_t dim);
}
