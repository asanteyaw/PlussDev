#include "pluss/subsp/eng/signal/design.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"

#include <cmath>
#include <complex>
#include <cstdint>
#include <limits>
#include <numbers>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pluss::subsp::eng::signal {
namespace {

using Complex = std::complex<double>;

struct Roots {
  std::vector<Complex> zeros;
  std::vector<Complex> poles;
  double gain{1.0};
};

Complex product(const std::vector<Complex> &values, Complex shift = {}) {
  Complex result{1.0, 0.0};
  for (const Complex value : values)
    result *= shift - value;
  return result;
}

void validate_common(std::int64_t order, const IirOptions &options,
                     std::string_view operation) {
  PLUSS_CHECK(order > 0, std::string(operation) + ": order must be positive");
  PLUSS_CHECK(order <= 500,
              std::string(operation) + ": order must not exceed 500");
  PLUSS_CHECK(options.dtype == ScalarType::Float32 ||
                  options.dtype == ScalarType::Float64,
              std::string(operation) + ": dtype must be Float32 or Float64");
  if (options.sample_rate.has_value()) {
    PLUSS_CHECK(
        std::isfinite(*options.sample_rate) && *options.sample_rate > 0.0,
        std::string(operation) + ": sample_rate must be finite and positive");
  }
}

void validate_scalar(double cutoff, const IirOptions &options,
                     std::string_view operation) {
  PLUSS_CHECK(options.band == Band::Low || options.band == Band::High,
              std::string(operation) +
                  ": a scalar cutoff requires Band::Low or Band::High");
  PLUSS_CHECK(std::isfinite(cutoff) && cutoff > 0.0,
              std::string(operation) + ": cutoff must be finite and positive");
  if (options.sample_rate.has_value())
    PLUSS_CHECK(cutoff < *options.sample_rate / 2.0,
                std::string(operation) +
                    ": a digital cutoff must be below the Nyquist frequency");
}

void validate_band(std::pair<double, double> cutoff, const IirOptions &options,
                   std::string_view operation) {
  PLUSS_CHECK(options.band == Band::Pass || options.band == Band::Stop,
              std::string(operation) +
                  ": two cutoffs require Band::Pass or Band::Stop");
  PLUSS_CHECK(std::isfinite(cutoff.first) && std::isfinite(cutoff.second) &&
                  cutoff.first > 0.0 && cutoff.first < cutoff.second,
              std::string(operation) +
                  ": cutoffs must be finite, positive, and increasing");
  if (options.sample_rate.has_value())
    PLUSS_CHECK(cutoff.second < *options.sample_rate / 2.0,
                std::string(operation) +
                    ": digital cutoffs must be below the Nyquist frequency");
}

Roots butter_proto(std::int64_t order) {
  Roots roots;
  roots.poles.reserve(static_cast<std::size_t>(order));
  for (std::int64_t lane = 0; lane < order; ++lane) {
    const double m = static_cast<double>(-order + 1 + 2 * lane);
    roots.poles.push_back(
        -std::exp(Complex{0.0, std::numbers::pi * m / (2.0 * order)}));
  }
  return roots;
}

Roots cheby1_proto(std::int64_t order, double ripple) {
  PLUSS_CHECK(std::isfinite(ripple) && ripple > 0.0,
              "eng::signal::cheby1: ripple must be finite and positive");
  const double epsilon = std::sqrt(std::pow(10.0, 0.1 * ripple) - 1.0);
  const double mu = std::asinh(1.0 / epsilon) / order;
  Roots roots;
  roots.poles.reserve(static_cast<std::size_t>(order));
  for (std::int64_t lane = 0; lane < order; ++lane) {
    const double m = static_cast<double>(-order + 1 + 2 * lane);
    const double theta = std::numbers::pi * m / (2.0 * order);
    roots.poles.push_back(-std::sinh(Complex{mu, theta}));
  }
  roots.gain = product(roots.poles).real();
  if (order % 2 == 0)
    roots.gain /= std::sqrt(1.0 + epsilon * epsilon);
  return roots;
}

Roots cheby2_proto(std::int64_t order, double attenuation) {
  PLUSS_CHECK(std::isfinite(attenuation) && attenuation > 0.0,
              "eng::signal::cheby2: attenuation must be finite and positive");
  const double delta = 1.0 / std::sqrt(std::pow(10.0, 0.1 * attenuation) - 1.0);
  const double mu = std::asinh(1.0 / delta) / order;
  Roots roots;
  roots.zeros.reserve(static_cast<std::size_t>(order));
  roots.poles.reserve(static_cast<std::size_t>(order));
  for (std::int64_t lane = 0; lane < order; ++lane) {
    const std::int64_t m = -order + 1 + 2 * lane;
    if (m != 0) {
      const double angle =
          std::numbers::pi * static_cast<double>(m) / (2.0 * order);
      roots.zeros.emplace_back(0.0, 1.0 / std::sin(angle));
    }
    const double theta =
        std::numbers::pi * static_cast<double>(m) / (2.0 * order);
    roots.poles.push_back(-1.0 / std::sinh(Complex{mu, theta}));
  }
  roots.gain = (product(roots.poles) / product(roots.zeros)).real();
  return roots;
}

Roots lowpass(Roots roots, double cutoff) {
  const std::int64_t degree =
      static_cast<std::int64_t>(roots.poles.size() - roots.zeros.size());
  for (Complex &zero : roots.zeros)
    zero *= cutoff;
  for (Complex &pole : roots.poles)
    pole *= cutoff;
  roots.gain *= std::pow(cutoff, degree);
  return roots;
}

Roots highpass(Roots roots, double cutoff) {
  const std::int64_t degree =
      static_cast<std::int64_t>(roots.poles.size() - roots.zeros.size());
  const Complex zero_product = product(roots.zeros);
  const Complex pole_product = product(roots.poles);
  for (Complex &zero : roots.zeros)
    zero = cutoff / zero;
  for (Complex &pole : roots.poles)
    pole = cutoff / pole;
  roots.zeros.insert(roots.zeros.end(), static_cast<std::size_t>(degree),
                     Complex{});
  roots.gain *= (zero_product / pole_product).real();
  return roots;
}

std::vector<Complex> split_band(const std::vector<Complex> &roots,
                                double center, double width, bool invert) {
  std::vector<Complex> result;
  result.reserve(roots.size() * 2);
  for (const Complex root : roots) {
    const Complex mapped = invert ? (width / 2.0) / root : root * (width / 2.0);
    const Complex branch = std::sqrt(mapped * mapped - center * center);
    result.push_back(mapped + branch);
    result.push_back(mapped - branch);
  }
  return result;
}

Roots bandpass(Roots roots, double low, double high) {
  const std::int64_t degree =
      static_cast<std::int64_t>(roots.poles.size() - roots.zeros.size());
  const double width = high - low;
  const double center = std::sqrt(low * high);
  roots.zeros = split_band(roots.zeros, center, width, false);
  roots.poles = split_band(roots.poles, center, width, false);
  roots.zeros.insert(roots.zeros.end(), static_cast<std::size_t>(degree),
                     Complex{});
  roots.gain *= std::pow(width, degree);
  return roots;
}

Roots bandstop(Roots roots, double low, double high) {
  const std::int64_t degree =
      static_cast<std::int64_t>(roots.poles.size() - roots.zeros.size());
  const double width = high - low;
  const double center = std::sqrt(low * high);
  const Complex zero_product = product(roots.zeros);
  const Complex pole_product = product(roots.poles);
  roots.zeros = split_band(roots.zeros, center, width, true);
  roots.poles = split_band(roots.poles, center, width, true);
  roots.zeros.insert(roots.zeros.end(), static_cast<std::size_t>(degree),
                     Complex{0.0, center});
  roots.zeros.insert(roots.zeros.end(), static_cast<std::size_t>(degree),
                     Complex{0.0, -center});
  roots.gain *= (zero_product / pole_product).real();
  return roots;
}

Roots tustin(Roots roots, double sample_rate) {
  const std::int64_t degree =
      static_cast<std::int64_t>(roots.poles.size() - roots.zeros.size());
  const double twice_rate = 2.0 * sample_rate;
  const Complex zero_gain = product(roots.zeros, twice_rate);
  const Complex pole_gain = product(roots.poles, twice_rate);
  for (Complex &zero : roots.zeros)
    zero = (twice_rate + zero) / (twice_rate - zero);
  for (Complex &pole : roots.poles)
    pole = (twice_rate + pole) / (twice_rate - pole);
  roots.zeros.insert(roots.zeros.end(), static_cast<std::size_t>(degree),
                     Complex{-1.0, 0.0});
  roots.gain *= (zero_gain / pole_gain).real();
  return roots;
}

Roots transform(Roots roots, double first, double second,
                const IirOptions &options) {
  if (options.sample_rate.has_value()) {
    const double rate = *options.sample_rate;
    first = 4.0 * std::tan(std::numbers::pi * first / rate);
    if (options.band == Band::Pass || options.band == Band::Stop)
      second = 4.0 * std::tan(std::numbers::pi * second / rate);
  }

  switch (options.band) {
  case Band::Low:
    roots = lowpass(std::move(roots), first);
    break;
  case Band::High:
    roots = highpass(std::move(roots), first);
    break;
  case Band::Pass:
    roots = bandpass(std::move(roots), first, second);
    break;
  case Band::Stop:
    roots = bandstop(std::move(roots), first, second);
    break;
  }
  if (options.sample_rate.has_value())
    roots = tustin(std::move(roots), 2.0);
  return roots;
}

lti::PoleZero materialize(Roots roots, const IirOptions &options) {
  const ScalarType complex = options.dtype == ScalarType::Float64
                                 ? ScalarType::Complex128
                                 : ScalarType::Complex64;
  const Config config =
      Config{}.with_dtype(complex).with_platform(options.platform);
  const Series zeros = series::make(roots.zeros, config);
  const Series poles = series::make(roots.poles, config);
  const Series gain =
      series::make(std::vector<Complex>{{roots.gain, 0.0}}, config).reshape({});
  const double sample_time =
      options.sample_rate.has_value() ? 1.0 / *options.sample_rate : 0.0;
  return lti::zpk(zeros, poles, gain, sample_time);
}

template <class Prototype>
lti::PoleZero design_scalar(std::int64_t order, double cutoff,
                            const IirOptions &options,
                            std::string_view operation, Prototype &&prototype) {
  validate_common(order, options, operation);
  validate_scalar(cutoff, options, operation);
  return materialize(transform(prototype(), cutoff, 0.0, options), options);
}

template <class Prototype>
lti::PoleZero design_band(std::int64_t order, std::pair<double, double> cutoff,
                          const IirOptions &options, std::string_view operation,
                          Prototype &&prototype) {
  validate_common(order, options, operation);
  validate_band(cutoff, options, operation);
  return materialize(
      transform(prototype(), cutoff.first, cutoff.second, options), options);
}

} // namespace

lti::PoleZero butter(std::int64_t order, double cutoff,
                     const IirOptions &options) {
  return design_scalar(order, cutoff, options, "eng::signal::butter",
                       [&] { return butter_proto(order); });
}

lti::PoleZero butter(std::int64_t order, std::pair<double, double> cutoff,
                     const IirOptions &options) {
  return design_band(order, cutoff, options, "eng::signal::butter",
                     [&] { return butter_proto(order); });
}

lti::PoleZero cheby1(std::int64_t order, double ripple, double cutoff,
                     const IirOptions &options) {
  return design_scalar(order, cutoff, options, "eng::signal::cheby1",
                       [&] { return cheby1_proto(order, ripple); });
}

lti::PoleZero cheby1(std::int64_t order, double ripple,
                     std::pair<double, double> cutoff,
                     const IirOptions &options) {
  return design_band(order, cutoff, options, "eng::signal::cheby1",
                     [&] { return cheby1_proto(order, ripple); });
}

lti::PoleZero cheby2(std::int64_t order, double attenuation, double cutoff,
                     const IirOptions &options) {
  return design_scalar(order, cutoff, options, "eng::signal::cheby2",
                       [&] { return cheby2_proto(order, attenuation); });
}

lti::PoleZero cheby2(std::int64_t order, double attenuation,
                     std::pair<double, double> cutoff,
                     const IirOptions &options) {
  return design_band(order, cutoff, options, "eng::signal::cheby2",
                     [&] { return cheby2_proto(order, attenuation); });
}

} // namespace pluss::subsp::eng::signal
