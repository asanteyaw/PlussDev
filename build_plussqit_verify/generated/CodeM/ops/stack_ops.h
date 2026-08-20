#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"
#include "MSharp/RangeView.h"

namespace pluss::codem::stack_ops {
  Series stack(pluss::msharp::RangeView::Of<Series> tensors, std::int64_t dim);
}

namespace pluss::codem::ops {
  inline Series stack(pluss::msharp::RangeView::Of<Series> tensors, std::int64_t dim) {
    return pluss::codem::stack_ops::stack(tensors, dim);
  }
}
