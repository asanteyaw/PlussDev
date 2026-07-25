#pragma once

#include "pluss/ai/ice/options.h"
#include "pluss/nn.h"

#include <memory>
#include <vector>

namespace pluss::ai::ice {

// Constructs whichever optimizer `options.optimizer` selects, with
// `options.grad_clip` and non-finite diagnostics (`options.fail_on_non_finite`)
// already wired in — the same construction `ice::Model` uses internally for
// its generic learn() loop, exposed standalone for training loops that
// don't fit Model's generic forward/loss shape (e.g. a per-step negative
// sampling loop, where there's no fixed (x, y) batch) but still want ice's
// default safety net instead of hand-rolling it.
[[nodiscard]] std::unique_ptr<pluss::nn::optim::Optimizer> make_optimizer(
    const std::vector<Series*>& parameters, const TrainOptions& options);

} // namespace pluss::ai::ice
