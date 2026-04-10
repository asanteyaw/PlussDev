#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"
#include "MSharp/RangeView.h"

namespace pluss::codem::cat_ops {
  Series cat(pluss::msharp::RangeView::Of<Series> tensors, std::int64_t dim);
}

namespace pluss::codem::ops {
  inline Series cat(pluss::msharp::RangeView::Of<Series> tensors, std::int64_t dim) {
    return pluss::codem::cat_ops::cat(tensors, dim);
  }
}
