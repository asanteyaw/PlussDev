#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"

namespace pluss::codem::sin_ops {
  Series sin(const Series& a);
}

namespace pluss::codem::ops {
  inline Series sin(const Series& a) {
    return pluss::codem::sin_ops::sin(a);
  }
}
