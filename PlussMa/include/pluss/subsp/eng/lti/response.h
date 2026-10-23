#pragma once

#include "pluss/subsp/eng/lti/pole_zero.h"
#include "pluss/subsp/eng/lti/state.h"

namespace pluss::subsp::eng::lti {

// omega is angular frequency in radians per time unit. For a discrete model,
// sample_time maps it to the unit circle through z = exp(j * omega * T).
Series freqresp(const StateSpace &system, const Series &omega);
Series freqresp(const PoleZero &system, const Series &omega);

} // namespace pluss::subsp::eng::lti
