#pragma once

// PlussMa umbrella include for math layer algorithms.
#include "pluss/subsp/math/optim.h"
#include "../../Math/Numerical/optimization/dfo_solver.h"
#include "../../Math/Numerical/optimization/dfo_minimizers.h"
#include "../../Math/Numerical/optimization/mads_blackbox_minimizers.h"
#include "../../Math/Numerical/optimization/nelder_mead_minimizer.h"
#include "pluss/ma/stats.h"
#include "pluss/ma/linearalg.h"

namespace pluss {

// Short public alias for derivative-based constrained optimization.
namespace optim = ::pluss::subsp::math::optim;

} // namespace pluss
