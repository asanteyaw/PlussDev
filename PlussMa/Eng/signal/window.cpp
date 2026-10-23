#include "pluss/subsp/eng/signal/window.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"

#include <cmath>
#include <numbers>
#include <string>
#include <string_view>

namespace pluss::subsp::eng::signal {
namespace {

void validate(std::int64_t size, const WindowOptions &options,
              std::string_view operation) {
  PLUSS_CHECK(size >= 0,
              std::string(operation) + ": size must be non-negative");
  PLUSS_CHECK(options.dtype == ScalarType::Float32 ||
                  options.dtype == ScalarType::Float64,
              std::string(operation) + ": dtype must be Float32 or Float64");
}

Series lanes(std::int64_t size, const WindowOptions &options) {
  return series::arange(0, size, options.dtype).to(options.platform);
}

Series angle(std::int64_t size, const WindowOptions &options) {
  const double period = options.mode == WindowMode::Periodic
                            ? static_cast<double>(size)
                            : static_cast<double>(size - 1);
  return lanes(size, options).mul(2.0 * std::numbers::pi / period);
}

Series short_window(std::int64_t size, const WindowOptions &options,
                    std::string_view operation) {
  validate(size, options, operation);
  return series::ones({size}, options.dtype).to(options.platform);
}

} // namespace

Series boxcar(std::int64_t size, const WindowOptions &options) {
  return short_window(size, options, "eng::signal::boxcar");
}

Series bartlett(std::int64_t size, const WindowOptions &options) {
  if (size <= 1)
    return short_window(size, options, "eng::signal::bartlett");
  validate(size, options, "eng::signal::bartlett");
  const double extent = options.mode == WindowMode::Periodic
                            ? static_cast<double>(size)
                            : static_cast<double>(size - 1);
  const double center = extent / 2.0;
  return lanes(size, options).sub(center).abs().div(center).neg().add(1.0);
}

Series blackman(std::int64_t size, const WindowOptions &options) {
  if (size <= 1)
    return short_window(size, options, "eng::signal::blackman");
  validate(size, options, "eng::signal::blackman");
  const Series phase = angle(size, options);
  return phase.cos().mul(-0.5).add(phase.mul(2.0).cos().mul(0.08)).add(0.42);
}

Series hamming(std::int64_t size, const WindowOptions &options) {
  if (size <= 1)
    return short_window(size, options, "eng::signal::hamming");
  validate(size, options, "eng::signal::hamming");
  return angle(size, options).cos().mul(-0.46).add(0.54);
}

Series hann(std::int64_t size, const WindowOptions &options) {
  if (size <= 1)
    return short_window(size, options, "eng::signal::hann");
  validate(size, options, "eng::signal::hann");
  return angle(size, options).cos().mul(-0.5).add(0.5);
}

} // namespace pluss::subsp::eng::signal
