#pragma once

#include <cstdint>

#include "CodeM/core/SeriesSlim.h"

namespace pluss::codem::native::cpu {

Series grouped_median(const Series& values,
                      const Series& group_ids,
                      std::int64_t group_count);

Series grouped_mode(const Series& values,
                    const Series& group_ids,
                    std::int64_t group_count);

} // namespace pluss::codem::native::cpu
