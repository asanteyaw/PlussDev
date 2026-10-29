#pragma once

// Biology uses the shared common::dynamics layer (Coeff, validation, batched
// matvec/solve, finite-difference Jacobian, RK4 simulate / trajectory).

#include "pluss/subsp/natsci/common/dynamics.h"

namespace pluss::subsp::natsci::biology {
namespace dynamics = ::pluss::subsp::natsci::common::dynamics;
}
