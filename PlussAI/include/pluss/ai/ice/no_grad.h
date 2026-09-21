#pragma once

#include "pluss/nn.h"

namespace pluss::ai::ice {

// Ice keeps a local name for downstream source compatibility, but the
// implementation now delegates to the lower-level NodeK/Series autodiff
// gate so every op in the scope sees the same thread-local no-grad state.
class NoGradGuard {
public:
  NoGradGuard() noexcept = default;

  template <class Range>
  explicit NoGradGuard(const Range&) noexcept {}

  ~NoGradGuard() = default;

  NoGradGuard(const NoGradGuard&) = delete;
  NoGradGuard& operator=(const NoGradGuard&) = delete;
  NoGradGuard(NoGradGuard&&) = delete;
  NoGradGuard& operator=(NoGradGuard&&) = delete;

private:
  pluss::nn::NoGradGuard guard_{};
};

} // namespace pluss::ai::ice
