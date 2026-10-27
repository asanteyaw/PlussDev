#pragma once

// Spectral inverse of the constant-coefficient periodic operators in field.h. The discrete Laplacians diagonalise exactly in the Fourier basis, so
//   compact: -(4/h^2) sin^2(pi j / n)      wide: -sin^2(2 pi j / n) / h^2       (per axis, j = integer frequency)
// and a Poisson solve is one forward transform, one divide and one inverse transform: direct, no iteration, accurate to rounding, O(N log N).
// The transforms are the canonical Series ones (rfft over the last spatial axis, fft over the others, mirrored on the way back).
// Null modes (the constant, plus the alternating mode of an even axis for the wide stencil) are detected from the integer frequencies and set to zero.

#include "pluss/subsp/natsci/common/field.h"

#include <cmath>
#include <numbers>
#include <vector>

namespace pluss::subsp::natsci::common::spatial::spectral {

// real field [..., n_0, ..., n_{d-1}] -> one-sided complex spectrum [..., n_0, ..., n_{d-1}/2 + 1]
[[nodiscard]] inline Series forward(const Series &field, const PeriodicGrid &grid) {
  const std::int64_t last = field.dim() - 1;
  Series spectrum = field.rfft(-1, last);
  for (std::int64_t axis = field.dim() - grid.dim(); axis < last; ++axis)
    spectrum = spectrum.fft(-1, axis);
  return spectrum;
}
[[nodiscard]] inline Series inverse(const Series &spectrum, const PeriodicGrid &grid) {
  const std::int64_t last = spectrum.dim() - 1;
  Series field = spectrum;
  for (std::int64_t axis = spectrum.dim() - grid.dim(); axis < last; ++axis)
    field = field.ifft(-1, axis);
  return field.irfft(grid.shape().back(), last);
}

struct Symbol final {
  Series value = pluss::series::undefined(); // eigenvalue of the operator, shape [n_0, ..., n_{d-1}/2 + 1]
  Series keep = pluss::series::undefined();  // 1 where the mode is invertible, 0 on the null space
};

// eigenvalues of laplacian (wide = false) or wide_laplacian (wide = true) on the one-sided spectrum layout
[[nodiscard]] inline Symbol symbol(const PeriodicGrid &grid, bool wide, pluss::ScalarType dtype) {
  const std::int64_t d = grid.dim();
  Series lambda = pluss::series::undefined(), null_modes = pluss::series::undefined();
  for (std::int64_t a = 0; a < d; ++a) {
    const std::int64_t n = grid.shape()[a], m = (a == d - 1) ? n / 2 + 1 : n;
    const double h = grid.spacing(a).si_value();
    std::vector<std::int64_t> shape(static_cast<std::size_t>(d), 1);
    shape[a] = m;
    const Series j = pluss::series::arange(0, m, 1, dtype).reshape(shape);
    const Series theta = j.mul(std::numbers::pi / static_cast<double>(n) * (wide ? 2.0 : 1.0));
    const Series term = theta.sin().square().mul(wide ? -1.0 / (h * h) : -4.0 / (h * h));
    const Series null_axis = (wide && n % 2 == 0) ? j.eq(0.0).logical_or(j.eq(static_cast<double>(n / 2))) : j.eq(0.0);
    lambda = a == 0 ? term : lambda.add(term);
    null_modes = a == 0 ? null_axis : null_modes.logical_and(null_axis);
  }
  const Series zero = lambda.mul(0.0);
  return {null_modes.where(zero.add(1.0), lambda), null_modes.where(zero, zero.add(1.0))};
}

// solution of laplacian(u) = rhs (or wide_laplacian) with the null-space component of u set to zero; rhs must already be compatible
[[nodiscard]] inline Series solve(const Series &rhs, const PeriodicGrid &grid, bool wide) {
  const Symbol s = symbol(grid, wide, rhs.scalar_type());
  return inverse(forward(rhs, grid).div(s.value).mul(s.keep), grid);
}

} // namespace pluss::subsp::natsci::common::spatial::spectral
