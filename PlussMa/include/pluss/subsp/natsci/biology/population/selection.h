#pragma once

// Replicator dynamics on the simplex: dx_i = x_i ((A x)_i - x.A x).
// The flow preserves sum(x) = 1 exactly in the continuous system
// (sum of rates = x.Ax - x.Ax = 0). Source: Wikipedia "Replicator equation".

#include "pluss/subsp/natsci/biology/dynamics.h"

namespace pluss::subsp::natsci::biology::population::selection {

inline Series replicator_rate(const Series &x, const Series &payoff) {
  dynamics::require_square(payoff, "replicator: payoff");
  dynamics::require_species(x, payoff.size(-1), "replicator: state");
  const Series fitness = dynamics::matvec(payoff, x);
  const Series mean = x.mul(fitness).sum(-1, true);
  return x.mul(fitness.sub(mean));
}

// Mean fitness x.A x (non-decreasing along the flow when A is symmetric).
inline Series mean_fitness(const Series &x, const Series &payoff) {
  return x.mul(dynamics::matvec(payoff, x)).sum(-1, false);
}

// Project onto the simplex by renormalising (guards integrator drift).
inline Series normalize(const Series &x) {
  dynamics::require_nonnegative(x, "replicator: state");
  return x.div(x.sum(-1, true));
}

} // namespace pluss::subsp::natsci::biology::population::selection
