#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"
#include "MSharp/RangeView.h"

namespace pluss::codem::index_ops {
  Series index(const Series& a, pluss::msharp::RangeView::Of<Series> indices);
}

namespace pluss::codem::ops {
  inline Series index(const Series& a, pluss::msharp::RangeView::Of<Series> indices) {
    return pluss::codem::index_ops::index(a, indices);
  }
}
