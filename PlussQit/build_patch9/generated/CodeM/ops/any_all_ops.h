#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"

namespace pluss::codem::any_all_ops {
  Series any_all(const Series& a);
}

namespace pluss::codem::ops {
  inline Series any_all(const Series& a) {
    return pluss::codem::any_all_ops::any_all(a);
  }
}
