#pragma once

#include "pluss/nn.h"

#include <vector>

namespace pluss::ai::ice {

// RAII scope guard modeled on torch.no_grad() (confirmed: no engine-wide
// autograd-enabled flag exists anywhere in this codebase, so this is not a
// literal port of one). Temporarily disables gradient tracking on
// `parameters` for the guard's lifetime, restoring each parameter's prior
// tracked state on destruction — sufficient for `ice::Model::infer()`/
// `assess()`, whose forward() is always invoked with exactly `parameters`
// as its tracked leaves, so no downstream computation graph gets built for
// the duration of the guard.
class NoGradGuard {
public:
  explicit NoGradGuard(const std::vector<Series*>& parameters) : parameters_(parameters) {
    previous_.reserve(parameters_.size());
    for (auto* param : parameters_) {
      previous_.push_back(param->tracks_slope());
      param->tracks_slope_(false);
    }
  }

  ~NoGradGuard() {
    for (std::size_t i = 0; i < parameters_.size(); ++i) {
      parameters_[i]->tracks_slope_(previous_[i]);
    }
  }

  NoGradGuard(const NoGradGuard&) = delete;
  NoGradGuard& operator=(const NoGradGuard&) = delete;
  NoGradGuard(NoGradGuard&&) = delete;
  NoGradGuard& operator=(NoGradGuard&&) = delete;

private:
  std::vector<Series*> parameters_;
  std::vector<bool> previous_;
};

} // namespace pluss::ai::ice
