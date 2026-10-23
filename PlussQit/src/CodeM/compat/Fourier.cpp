#include "CodeM/compat/Fourier.h"

#include "CodeM/core/Errors.h"
#include "CodeM/core/OpsInternal.h"
#include "CodeM/core/Series.h"
#include "CodeM/core/SeriesTape.h"
#include "CodeM/native/cpu/FourierKernels.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace pluss::codem::_compat::fourier {
namespace {

enum class Norm { Backward, Forward, Ortho };

Norm parse_norm(std::string_view norm, std::string_view operation) {
  if (norm == "backward")
    return Norm::Backward;
  if (norm == "forward")
    return Norm::Forward;
  if (norm == "ortho")
    return Norm::Ortho;
  throw_error(std::string(operation) +
              ": norm must be 'backward', 'forward', or 'ortho'");
}

std::int64_t normalize_dim(std::int64_t dim, std::int64_t rank,
                           std::string_view operation) {
  if (dim < 0)
    dim += rank;
  PLUSS_CHECK(dim >= 0 && dim < rank,
              std::string(operation) + ": dim out of range");
  return dim;
}

void check_dtype(ScalarType dtype, std::string_view operation) {
  PLUSS_CHECK(
      dtype != ScalarType::Float16 && dtype != ScalarType::BFloat16,
      std::string(operation) +
          ": half-precision input is not supported; convert to Float32 first");
}

ScalarType complex_dtype(ScalarType dtype) {
  return dtype == ScalarType::Float64 || dtype == ScalarType::Complex128
             ? ScalarType::Complex128
             : ScalarType::Complex64;
}

Series resize_axis(const Series &input, std::int64_t n, std::int64_t dim,
                   std::string_view operation) {
  PLUSS_CHECK(n > 0, std::string(operation) + ": n must be greater than zero");
  const std::int64_t current = input.size(dim);
  if (current == n)
    return input;
  if (current > n)
    return input.slice(dim, 0, n);

  std::vector<std::int64_t> pad_shape(input.sizes().begin(),
                                      input.sizes().end());
  pad_shape[static_cast<std::size_t>(dim)] = n - current;
  const Series padding =
      Series::zeros(pad_shape, input.scalar_type()).to(input.platform());
  return input.unite({padding}, dim, true);
}

Series raw_transform(const Series &input, std::int64_t dim, bool forward,
                     bool hermitian_output = false, bool one_sided = false) {
  const Platform platform = input.platform();
  const Series cpu = input.to(Platform::CPU).contiguous();
  Series output = native::cpu::fourier_transform(cpu, dim, forward,
                                                 hermitian_output, one_sided);

  if (series_tape::recording_enabled() && series_tape::tracks_slope(cpu)) {
    auto link = std::make_shared<series_tape::SourceLink>();
    link->op_name = forward ? "fft" : "ifft";
    link->parents = {cpu};
    link->parent_data_guarded = {false};
    link->higher_order_support = series_tape::HigherOrderSupport::Traceable;
    link->propagate = [cpu, dim, forward,
                       one_sided](const Series &output_sensitivity) {
      const double length = static_cast<double>(cpu.size(dim));
      // A one-sided spectrum is the first half of the full one: the discarded
      // bins carry no sensitivity.
      Series full_sensitivity = output_sensitivity;
      if (one_sided)
        full_sensitivity =
            resize_axis(output_sensitivity, cpu.size(dim), dim, "rfft");
      Series input_sensitivity = raw_transform(full_sensitivity, dim, !forward);
      // Native forward is unscaled and native inverse carries 1/N.
      input_sensitivity = forward ? input_sensitivity.mul(length)
                                  : input_sensitivity.div(length);
      series_tape::accumulate_sensitivity(cpu, input_sensitivity);
    };
    series_tape::bind_source_link(output, std::move(link));
  }

  return platform == Platform::CPU ? output : output.to(platform);
}

double normalization_scale(Norm norm, bool forward, std::int64_t length) {
  const double n = static_cast<double>(length);
  if (norm == Norm::Ortho)
    return forward ? 1.0 / std::sqrt(n) : std::sqrt(n);
  if (norm == Norm::Forward)
    return forward ? 1.0 / n : n;
  return 1.0;
}

Series complex_transform(const Series &input, std::int64_t n, std::int64_t dim,
                         std::string_view norm_text, bool forward,
                         std::string_view operation, bool one_sided = false) {
  PLUSS_CHECK(input.defined(),
              std::string(operation) + ": input must be defined");
  check_dtype(input.scalar_type(), operation);
  PLUSS_CHECK(input.dim() > 0, std::string(operation) +
                                   ": input must have at least one dimension");
  dim = normalize_dim(dim, input.dim(), operation);
  const std::int64_t length = n == -1 ? input.size(dim) : n;
  PLUSS_CHECK(length > 0, std::string(operation) +
                              ": transform length must be greater than zero");
  const Norm norm = parse_norm(norm_text, operation);

  const bool real_signal = !dtype_of(input.scalar_type()).is_complex();
  Series prepared = input;
  if (n != -1)
    prepared = resize_axis(prepared, length, dim, operation);
  prepared = prepared.to(complex_dtype(prepared.scalar_type()));
  Series output =
      raw_transform(prepared, dim, forward, real_signal && forward, one_sided);
  const double scale = normalization_scale(norm, forward, length);
  if (scale != 1.0)
    output = output.mul(scale);
  return output;
}

} // namespace

Series fft(const Series &input, std::int64_t n, std::int64_t dim,
           std::string_view norm) {
  return complex_transform(input, n, dim, norm, true, "fft");
}

Series ifft(const Series &input, std::int64_t n, std::int64_t dim,
            std::string_view norm) {
  return complex_transform(input, n, dim, norm, false, "ifft");
}

Series rfft(const Series &input, std::int64_t n, std::int64_t dim,
            std::string_view norm) {
  PLUSS_CHECK(!dtype_of(input.scalar_type()).is_complex(),
              "rfft: input must be real-valued");
  PLUSS_CHECK(input.dim() > 0, "rfft: input must have at least one dimension");
  dim = normalize_dim(dim, input.dim(), "rfft");
  return complex_transform(input, n, dim, norm, true, "rfft", true);
}

Series irfft(const Series &input, std::int64_t n, std::int64_t dim,
             std::string_view norm) {
  PLUSS_CHECK(input.defined(), "irfft: input must be defined");
  PLUSS_CHECK(input.dim() > 0, "irfft: input must have at least one dimension");
  dim = normalize_dim(dim, input.dim(), "irfft");
  check_dtype(input.scalar_type(), "irfft");
  const std::int64_t length = n == -1 ? 2 * (input.size(dim) - 1) : n;
  PLUSS_CHECK(length > 0, "irfft: transform length must be greater than zero");
  const std::int64_t one_sided = length / 2 + 1;

  Series half = input.to(complex_dtype(input.scalar_type()));
  half = resize_axis(half, one_sided, dim, "irfft");
  const std::int64_t reflected_end =
      length % 2 == 0 ? one_sided - 1 : one_sided;
  Series full = half;
  if (reflected_end > 1) {
    Series reflected =
        ops_internal::conj(half.slice(dim, 1, reflected_end).flip(dim));
    full = half.unite({reflected}, dim, true);
  }
  return ops_internal::real(
      complex_transform(full, length, dim, norm, false, "irfft"));
}

} // namespace pluss::codem::_compat::fourier
