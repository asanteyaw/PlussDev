#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"

namespace pluss::codem::put_ops {
  Series put(const Series& a, const Series& index, const Series& src, bool accumulate);
}

namespace pluss::codem::ops {
  inline Series put(const Series& a, const Series& index, const Series& src, bool accumulate) {
    return pluss::codem::put_ops::put(a, index, src, accumulate);
  }
}
