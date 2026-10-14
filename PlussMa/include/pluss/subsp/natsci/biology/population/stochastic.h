#pragma once

// Gillespie SSA lives in common::ssa (shared with chemistry); this header
// keeps the biology::population::stochastic spelling.

#include "pluss/subsp/natsci/common/ssa.h"

namespace pluss::subsp::natsci::biology::population {
namespace stochastic = ::pluss::subsp::natsci::common::ssa;
}
