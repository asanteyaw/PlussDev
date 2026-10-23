#include "CodeM/native/cpu/FourierKernels.h"

#include "CodeM/core/Complex.h"
#include "CodeM/core/Errors.h"
#include "CodeM/native/cpu/FourierInternal.h"
#include "MSharp/Parallel.h"

#include <algorithm>
#include <complex>
#include <cstdint>
#include <vector>

namespace pluss::codem::native::cpu {
namespace {

std::int64_t normalize_dim(std::int64_t dim, std::int64_t rank) {
  if (dim < 0)
    dim += rank;
  PLUSS_CHECK(dim >= 0 && dim < rank, "fft: dim out of range");
  return dim;
}

// Rebuilds the redundant half of the spectrum of a real signal from the first
// half (rounding leaves tiny asymmetries, and a nonzero imaginary part in the
// zero-frequency and Nyquist bins).
template <class Complex>
void symmetrize(Complex *data, std::int64_t length, std::int64_t lanes) {
  for (std::int64_t lane = 0; lane < lanes; ++lane) {
    data[lane] = Complex(data[lane].real(), 0);
    for (std::int64_t k = 1; k <= (length - 1) / 2; ++k) {
      data[(length - k) * lanes + lane] = std::conj(data[k * lanes + lane]);
    }
    if (length % 2 == 0 && length > 1) {
      auto &nyquist = data[(length / 2) * lanes + lane];
      nyquist = Complex(nyquist.real(), 0);
    }
  }
}

template <class Complex>
Series transform_typed(const Series &input, std::int64_t dim, bool forward,
                       bool hermitian_output, bool one_sided) {
  using Real = typename Complex::value_type;
  auto sizes =
      std::vector<std::int64_t>(input.sizes().begin(), input.sizes().end());
  const std::int64_t length = sizes[static_cast<std::size_t>(dim)];
  std::int64_t outer = 1;
  std::int64_t inner = 1;
  for (std::int64_t axis = 0; axis < dim; ++axis)
    outer *= sizes[static_cast<std::size_t>(axis)];
  for (std::int64_t axis = dim + 1; axis < input.dim(); ++axis)
    inner *= sizes[static_cast<std::size_t>(axis)];

  // A real signal of even length is transformed as one complex signal of half
  // the length (even samples as the real part, odd samples as the imaginary
  // part) and the two spectra are separated afterwards: half the work.
  const bool real_path = hermitian_output && forward && inner == 1 &&
                         length >= 2 && length % 2 == 0;
  const std::int64_t out_length = one_sided ? length / 2 + 1 : length;
  auto out_sizes = sizes;
  out_sizes[static_cast<std::size_t>(dim)] = out_length;
  Series output = Series::empty(out_sizes, input.scalar_type());
  const auto *source = reinterpret_cast<const Complex *>(input.raw_data());
  auto *destination = reinterpret_cast<Complex *>(output.raw_data());

  // Neighbouring lanes (columns along the inner axes, or consecutive rows when
  // the axis is the last one) are transformed together, interleaved, so that
  // every pass reads and writes contiguous runs that vectorize and a transform
  // along a non-last axis is not a strided gather per lane.
  const std::int64_t unit = real_path ? length / 2 : length;
  const std::int64_t block = std::min<std::int64_t>(
      inner == 1 ? outer : inner, std::max<std::int64_t>(1, 32768 / unit));
  const std::int64_t lane_count = inner == 1 ? outer : inner;
  const std::int64_t blocks_per_row = (lane_count + block - 1) / block;
  const std::int64_t tasks =
      inner == 1 ? blocks_per_row : outer * blocks_per_row;
  const std::int64_t grain = std::max<std::int64_t>(
      1, 16384 / std::max<std::int64_t>(unit * block, 1));

  if (real_path) {
    const std::int64_t half = length / 2;
    const auto plan = fourier_detail::make_plan<Real>(half);
    const auto roots = fourier_detail::real_split_roots<Real>(length);
    pluss::msharp::parallel_for(
        0, tasks, grain, [&](std::int64_t begin, std::int64_t end) {
          fourier_detail::Workspace<Real> workspace;
          std::vector<Complex> packed(static_cast<std::size_t>(half * block));
          for (std::int64_t task = begin; task < end; ++task) {
            const std::int64_t first = task * block;
            const std::int64_t lanes = std::min(block, outer - first);
            for (std::int64_t b = 0; b < lanes; ++b) {
              const Complex *signal = source + (first + b) * length;
              for (std::int64_t j = 0; j < half; ++j) {
                packed[static_cast<std::size_t>(j * lanes + b)] =
                    Complex(signal[2 * j].real(), signal[2 * j + 1].real());
              }
            }
            const Complex *z = fourier_detail::transform_lanes(
                packed.data(), lanes, false, plan, workspace);
            for (std::int64_t b = 0; b < lanes; ++b) {
              Complex *spectrum = destination + (first + b) * out_length;
              for (std::int64_t k = 0; k <= half; ++k) {
                const Complex zk = z[(k == half ? 0 : k) * lanes + b];
                const Complex zc =
                    std::conj(z[(k == 0 ? 0 : half - k) * lanes + b]);
                const Complex even((zk.real() + zc.real()) * Real{0.5},
                                   (zk.imag() + zc.imag()) * Real{0.5});
                const Complex diff(zk.real() - zc.real(),
                                   zk.imag() - zc.imag());
                const Complex odd(diff.imag() * Real{0.5},
                                  -diff.real() * Real{0.5}); // diff / (2i)
                const Complex t = fourier_detail::cmul(
                    odd, (*roots)[static_cast<std::size_t>(k)]);
                spectrum[k] =
                    Complex(even.real() + t.real(), even.imag() + t.imag());
              }
              // The zero-frequency and Nyquist bins of a real signal are real;
              // their imaginary parts cancel exactly.
              spectrum[0] = Complex(spectrum[0].real(), 0);
              spectrum[half] = Complex(spectrum[half].real(), 0);
              if (!one_sided) {
                for (std::int64_t k = 1; k < half; ++k)
                  spectrum[length - k] = std::conj(spectrum[k]);
              }
            }
          }
        });
    return output;
  }

  const auto plan = fourier_detail::make_plan<Real>(length);
  const Real inverse_scale =
      forward ? Real{1} : Real{1} / static_cast<Real>(length);

  pluss::msharp::parallel_for(
      0, tasks, grain, [&](std::int64_t begin, std::int64_t end) {
        fourier_detail::Workspace<Real> workspace;
        std::vector<Complex> data(static_cast<std::size_t>(length * block));
        for (std::int64_t task = begin; task < end; ++task) {
          const std::int64_t outer_lane =
              inner == 1 ? 0 : task / blocks_per_row;
          const std::int64_t first =
              (inner == 1 ? task : task % blocks_per_row) * block;
          const std::int64_t lanes = std::min(block, lane_count - first);
          if (inner == 1) {
            for (std::int64_t b = 0; b < lanes; ++b) {
              const Complex *row = source + (first + b) * length;
              for (std::int64_t p = 0; p < length; ++p)
                data[static_cast<std::size_t>(p * lanes + b)] = row[p];
            }
          } else {
            for (std::int64_t p = 0; p < length; ++p) {
              const Complex *from =
                  source + (outer_lane * length + p) * inner + first;
              std::copy(from, from + lanes, data.begin() + p * lanes);
            }
          }
          Complex *result = fourier_detail::transform_lanes(
              data.data(), lanes, !forward, plan, workspace);
          if (!forward) {
            for (std::int64_t i = 0; i < length * lanes; ++i) {
              result[i] = Complex(result[i].real() * inverse_scale,
                                  result[i].imag() * inverse_scale);
            }
          }
          if (hermitian_output)
            symmetrize(result, length, lanes);
          if (inner == 1) {
            for (std::int64_t b = 0; b < lanes; ++b) {
              Complex *row = destination + (first + b) * out_length;
              for (std::int64_t p = 0; p < out_length; ++p)
                row[p] = result[p * lanes + b];
            }
          } else {
            for (std::int64_t p = 0; p < out_length; ++p) {
              Complex *to =
                  destination + (outer_lane * out_length + p) * inner + first;
              std::copy(result + p * lanes, result + (p + 1) * lanes, to);
            }
          }
        }
      });
  return output;
}

} // namespace

Series fourier_transform(const Series &input, std::int64_t dim, bool forward,
                         bool hermitian_output, bool one_sided) {
  PLUSS_CHECK(input.defined(), "fft: input must be defined");
  PLUSS_CHECK(input.platform() == Platform::CPU,
              "fft: native CPU kernel requires CPU input");
  PLUSS_CHECK(input.dim() > 0, "fft: input must have at least one dimension");
  PLUSS_CHECK(input.is_contiguous(),
              "fft: native CPU kernel requires contiguous input");
  PLUSS_CHECK(!one_sided || (hermitian_output && forward),
              "fft: a one-sided spectrum needs a real forward transform");
  dim = normalize_dim(dim, input.dim());
  PLUSS_CHECK(input.size(dim) > 0, "fft: transformed axis must be non-empty");

  if (input.scalar_type() == ScalarType::Complex64) {
    return transform_typed<Complex64>(input, dim, forward, hermitian_output,
                                      one_sided);
  }
  PLUSS_CHECK(input.scalar_type() == ScalarType::Complex128,
              "fft: native CPU kernel requires Complex64 or Complex128 input");
  return transform_typed<Complex128>(input, dim, forward, hermitian_output,
                                     one_sided);
}

} // namespace pluss::codem::native::cpu
