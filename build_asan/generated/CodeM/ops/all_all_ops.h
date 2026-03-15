#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"

namespace pluss::codem::all_all_ops {
  Series all_all(const Series& a);
}

namespace pluss::codem::ops {
  inline Series all_all(const Series& a) {
    return pluss::codem::all_all_ops::all_all(a);
  }
}
