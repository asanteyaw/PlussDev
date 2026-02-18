#pragma once

#include "aux/let.h"
#include "aux/queue.h"

namespace pluss {
using aux::queue;
using aux::queue_exec;
using aux::queue_into;
using aux::queue_options;
} // namespace pluss

// Convenience unqualified aliases for end users who prefer `let(...)`.
// Queue stays under `pluss::queue(...)`.
using pluss::aux::let;
