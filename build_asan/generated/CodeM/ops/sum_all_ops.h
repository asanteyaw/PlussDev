#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"

namespace pluss::codem::sum_all_ops {
  Series sum_all(const Series& a);
}

namespace pluss::codem::ops {
  inline Series sum_all(const Series& a) {
    return pluss::codem::sum_all_ops::sum_all(a);
  }
}
