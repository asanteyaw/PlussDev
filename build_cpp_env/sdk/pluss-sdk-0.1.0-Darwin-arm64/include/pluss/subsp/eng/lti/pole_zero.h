#pragma once

#include "pluss/codem/codem.h"

#include <utility>

namespace pluss::subsp::eng::lti {

class PoleZero final {
public:
  const Series &zeros() const noexcept { return zeros_; }
  const Series &poles() const noexcept { return poles_; }
  const Series &gain() const noexcept { return gain_; }

  double sample_time() const noexcept { return sample_time_; }
  bool is_cont() const noexcept { return sample_time_ == 0.0; }
  bool is_disc() const noexcept { return sample_time_ > 0.0; }
  bool is_stable() const;

private:
  friend PoleZero zpk(const Series &, const Series &, const Series &, double);

  PoleZero(Series zeros, Series poles, Series gain, double sample_time)
      : zeros_(std::move(zeros)), poles_(std::move(poles)),
        gain_(std::move(gain)), sample_time_(sample_time) {}

  Series zeros_;
  Series poles_;
  Series gain_;
  double sample_time_{};
};

PoleZero zpk(const Series &zeros, const Series &poles, const Series &gain,
             double sample_time = 0.0);

// Produces [b0, b1, b2, a0, a1, a2] rows for a discrete system.
Series to_sos(const PoleZero &system);

} // namespace pluss::subsp::eng::lti
