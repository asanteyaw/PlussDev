#pragma once

#include "CodeM/core/SeriesSlim.h"

namespace pluss::codem::native::cpu {

enum class TrigTransformKind { Dct, Idct, Dst, Idst };

Series cdist_kernel(const Series& a, const Series& b, double p);
Series trig_transform(const Series& self, std::int64_t dim, TrigTransformKind kind);
Series hilbert_transform(const Series& self, std::int64_t dim);

} // namespace pluss::codem::native::cpu

