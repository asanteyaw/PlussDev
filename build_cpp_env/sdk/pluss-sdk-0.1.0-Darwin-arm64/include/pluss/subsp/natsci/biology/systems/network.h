#pragma once

// The mass-action reaction network lives in common::reaction (shared with
// chemistry); this header keeps the biology::systems::network spelling.

#include "pluss/subsp/natsci/common/reaction.h"

namespace pluss::subsp::natsci::biology::systems {
namespace network = ::pluss::subsp::natsci::common::reaction;
}
