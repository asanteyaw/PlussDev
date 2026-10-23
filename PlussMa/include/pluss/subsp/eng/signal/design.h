#pragma once

#include "pluss/subsp/eng/lti/pole_zero.h"

#include <cstdint>
#include <optional>
#include <utility>

namespace pluss::subsp::eng::signal {

enum class Band {
  Low,
  High,
  Pass,
  Stop,
};

struct IirOptions {
  Band band{Band::Low};
  // A value designs a digital filter in the same frequency units. Omitting
  // the rate designs an analog filter with angular-frequency cutoffs.
  std::optional<double> sample_rate{2.0};
  ScalarType dtype{ScalarType::Float64};
  Platform platform{Platform::CPU};
};

lti::PoleZero butter(std::int64_t order, double cutoff,
                     const IirOptions &options = {});
lti::PoleZero butter(std::int64_t order, std::pair<double, double> cutoff,
                     const IirOptions &options);

lti::PoleZero cheby1(std::int64_t order, double ripple, double cutoff,
                     const IirOptions &options = {});
lti::PoleZero cheby1(std::int64_t order, double ripple,
                     std::pair<double, double> cutoff,
                     const IirOptions &options);

lti::PoleZero cheby2(std::int64_t order, double attenuation, double cutoff,
                     const IirOptions &options = {});
lti::PoleZero cheby2(std::int64_t order, double attenuation,
                     std::pair<double, double> cutoff,
                     const IirOptions &options);

} // namespace pluss::subsp::eng::signal
