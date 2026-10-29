#pragma once

#include "CodeM/core/SeriesSlim.h"

namespace pluss::codem::native::cpu {

// Conventional complex DFT: forward is unscaled; inverse carries 1/N.
// Input must be a contiguous CPU Complex64 or Complex128 Series. With
// `hermitian_output` the caller guarantees that the input is the complex form
// of a real signal along `dim` (imaginary parts are never read), and the output
// is exactly conjugate-symmetric. With `one_sided` (which requires
// `hermitian_output` and `forward`) only the first N / 2 + 1 entries along
// `dim` are produced.
Series fourier_transform(const Series &input, std::int64_t dim, bool forward,
                         bool hermitian_output = false, bool one_sided = false);

} // namespace pluss::codem::native::cpu
