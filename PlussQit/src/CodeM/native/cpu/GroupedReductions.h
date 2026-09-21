#pragma once

#include <cstdint>
#include <utility>

#include "CodeM/core/SeriesSlim.h"

namespace pluss::codem::native::cpu {

Series grouped_median(const Series& values,
                      const Series& group_ids,
                      std::int64_t group_count);

// The two order-statistics grouped_median averages together for an
// even-sized group (equal to each other, and to grouped_median's own result,
// for an odd-sized group) -- exposed separately, read-only, purely so
// Series::grouped_median's reverse-mode node can route gradient to the
// specific elements each bound came from (grouped_median itself doesn't
// carry that once the two are averaged into one value). Same nth_element
// selection as grouped_median, recomputed independently rather than
// threaded out of it, so it never mutates or risks the forward path.
std::pair<Series, Series> grouped_median_bounds(const Series& values,
                                                const Series& group_ids,
                                                std::int64_t group_count);

Series grouped_mode(const Series& values,
                    const Series& group_ids,
                    std::int64_t group_count);

} // namespace pluss::codem::native::cpu
