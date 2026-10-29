#pragma once

// Gillespie direct-method SSA, vectorised across replicates.
//
// Every replicate is a row of `state` [B, S]; one loop iteration fires one
// event in every still-active replicate using whole-batch Series ops (two
// uniform draws, a cumulative-sum reaction pick, a one-hot stoichiometry
// matmul). The only host loop is the sequential event loop itself.
//
// Source: Wikipedia "Gillespie algorithm" (direct method):
//   a0 = sum_j a_j,  tau = ln(1/r1)/a0,  mu = smallest j with
//   sum_{j'<=j} a_j' > r2 a0,  x <- x + nu_mu.
// A replicate whose next event time exceeds t_end stops with its current state
// (exact sampling of the state at t_end; the exponential clock is memoryless).

#include "pluss/subsp/natsci/common/dynamics.h"
#include "pluss/tils/aux/rng_seed.h"

#include <algorithm>

namespace pluss::subsp::natsci::common::ssa {

// propensity: state [B, S] -> a [B, R], non-negative.
using PropensityFn = std::function<Series(const Series &)>;

struct SsaOptions final {
  std::int64_t max_events{1'000'000}; // per replicate
  std::uint64_t seed{0};              // 0 = draw from the global stream
};

struct SsaResult final {
  Series state = pluss::series::undefined(); // [B, S] at t_end
  Series events = pluss::series::undefined(); // [B] events fired
  bool completed{false}; // every replicate reached t_end (or went extinct)
};

// stoichiometry nu: [R, S] net change per reaction.
inline SsaResult gillespie(const PropensityFn &propensity, const Series &nu,
                           const Series &x0, double t_end,
                           const SsaOptions &options = {}) {
  dynamics::require(x0.dim() == 2, "gillespie: state must be [replicates, species]");
  dynamics::require(nu.dim() == 2 && nu.size(-1) == x0.size(-1),
                    "gillespie: stoichiometry must be [reactions, species]");
  dynamics::require_nonnegative(x0, "gillespie: initial state");
  dynamics::require(std::isfinite(t_end) && t_end >= 0.0,
                    "gillespie: t_end must be finite and non-negative");
  const std::int64_t batch = x0.size(0);
  const std::int64_t reactions = nu.size(0);
  const Config cfg =
      Config{}.with_dtype(x0.scalar_type()).with_platform(x0.platform());
  const Series nu_t = nu.to(x0.scalar_type()).to(x0.platform());
  const Series reaction_index =
      pluss::series::arange(0, reactions, 1, x0.scalar_type()).to(x0.platform());

  Series x = x0;
  Series t = pluss::series::zeros({batch}, cfg);
  Series count = pluss::series::zeros({batch}, cfg);
  // A replicate is finished for good once its next event time passes t_end
  // (the state at t_end is then exact) or its total propensity is zero.
  Series done = pluss::series::zeros({batch}, cfg).gt(0.0);
  const std::uint64_t base = pluss::tils::seed_or_next(options.seed);
  bool completed = false;

  for (std::int64_t event = 0; event < options.max_events; ++event) {
    const Series a = propensity(x);
    const Series a0 = a.sum(-1, false);
    const Series r1 = pluss::series::rand({batch}, cfg, base + 2 * event + 1);
    const Series r2 = pluss::series::rand({batch}, cfg, base + 2 * event + 2);
    // r1 in [0,1): use 1 - r1 in (0,1] so ln is finite.
    const Series safe_a0 = a0.gt(0.0).where(a0, a0.mul(0.0).add(1.0));
    const Series tau = r1.neg().add(1.0).log().neg().div(safe_a0);
    const Series t_new = t.add(tau);
    const Series stops = a0.le(0.0).logical_or(t_new.gt(t_end));
    done = done.logical_or(stops);
    const Series fire = done.logical_not();
    // chosen reaction = #{cumsum_j <= r2 a0}, clamped to R-1 for round-off.
    const Series target = r2.mul(a0).unsqueeze(-1);
    const Series picked = a.cumsum(-1, false).le(target).to(x0.scalar_type())
                              .sum(-1, true)
                              .clamp(0.0, static_cast<double>(reactions - 1));
    const Series onehot = picked.eq(reaction_index).to(x0.scalar_type());
    const Series fired = fire.to(x0.scalar_type());
    x = x.add(onehot.matmul(nu_t).mul(fired.unsqueeze(-1)));
    t = fire.where(t_new, t);
    count = count.add(fired);
    if ((event & 7) == 7 || event + 1 == options.max_events) {
      if (done.all().item<bool>()) {
        completed = true;
        break;
      }
    }
  }
  return {std::move(x), std::move(count), completed};
}

} // namespace pluss::subsp::natsci::common::ssa
