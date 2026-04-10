#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"
#include "MSharp/RangeView.h"

namespace pluss::codem::index_put_ops {
  Series index_put(const Series& a, pluss::msharp::RangeView::Of<Series> indices, const Series& values, bool accumulate);
}

namespace pluss::codem::ops {
  inline Series index_put(const Series& a, pluss::msharp::RangeView::Of<Series> indices, const Series& values, bool accumulate) {
    return pluss::codem::index_put_ops::index_put(a, indices, values, accumulate);
  }
}
