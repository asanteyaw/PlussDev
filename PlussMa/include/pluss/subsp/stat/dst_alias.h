#pragma once

#include "pluss/subsp/stat/distr_fwd.h"

// Opt-in PlussMa-scoped shorthand for distribution kind enum aliases.
// Include this header when you explicitly want `ma::dst::Normal` style usage.
namespace pluss::ma {
namespace dst = ::pluss::subsp::stat::distr::dst;
} // namespace pluss::ma
