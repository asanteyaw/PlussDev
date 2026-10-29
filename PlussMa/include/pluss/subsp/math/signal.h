#pragma once

// pluss::subsp::math::signal -- eager transforms of sampled signals built on the Series FFT.
//
//   analytic_signal(x, dim)  x + i H[x]: the FFT with the negative frequencies removed and the positive ones doubled
//   hilbert(x, dim)          H[x] = (1/pi) PV int x(t) / (s - t) dt on the periodic sample, the imaginary part of the analytic signal
//
// The Hilbert transform is a principal-value integral, so it is an array computation rather than an operator (an
// operator returns an analytic Function; a principal value has a removable singularity the lazy integral cannot
// evaluate). On a periodic sample of whole periods H[cos] = sin and H[sin] = -cos; the Nyquist and zero frequencies
// are kept unchanged. Built from differentiable Series operations, so reverse mode flows through it.

#include "pluss/codem/series_factory.h"

#include <cstdint>
#include <vector>

namespace pluss::subsp::math::signal {

using pluss::Series;

[[nodiscard]] inline Series analytic_signal(const Series& x, std::int64_t dim = -1) {
  const std::int64_t axis = dim < 0 ? dim + x.dim() : dim;
  const std::int64_t n = x.size(axis);
  std::vector<double> gain(static_cast<std::size_t>(n), 0.0);
  gain[0] = 1.0;
  for (std::int64_t k = 1; k < (n + 1) / 2; ++k) gain[static_cast<std::size_t>(k)] = 2.0;
  if (n % 2 == 0) gain[static_cast<std::size_t>(n / 2)] = 1.0;
  std::vector<std::int64_t> shape(static_cast<std::size_t>(x.dim()), 1);
  shape[static_cast<std::size_t>(axis)] = n;
  const Series h = pluss::make_series(gain, shape).to(x.platform());
  return x.fft(-1, axis).mul(h).ifft(-1, axis);
}

[[nodiscard]] inline Series hilbert(const Series& x, std::int64_t dim = -1) { return analytic_signal(x, dim).imag(); }

}  // namespace pluss::subsp::math::signal
