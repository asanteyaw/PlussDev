#pragma once

#include "pluss/subsp/eng/lti/state.h"

namespace pluss::subsp::eng::lti {

enum class Hold {
  Auto,
  Zero,
  Linear,
};

struct SimOptions {
  Hold hold{Hold::Auto};
};

struct SimRun {
  Series values;
  Series states;
};

// Input has shape [..., time, inputs]; output and state histories have shapes
// [..., time, outputs] and [..., time, states].
SimRun sim(const StateSpace &system, const Series &input, const Series &time,
           const SimOptions &options = {});
SimRun sim(const StateSpace &system, const Series &input, const Series &time,
           const Series &initial, const SimOptions &options = {});

} // namespace pluss::subsp::eng::lti
