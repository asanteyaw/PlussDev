#pragma once
#include "CodeM/core/Series.h"
#include <vector>

namespace pluss::codem::native::cpu {
  Series cat(const std::vector<Series>& tensors, std::int64_t dim);
  Series stack(const std::vector<Series>& tensors, std::int64_t dim);
}
