#pragma once

#include "pluss/subsp/eng/lti/state.h"
#include "pluss/subsp/math/qp.h"

#include <cstdint>
#include <utility>

namespace pluss::subsp::eng::control::mpc {

struct Weights {
  Series state;
  Series input;
  Series terminal{};
};

struct Bounds {
  Series input_lower;
  Series input_upper;
  Series state_lower{};
  Series state_upper{};
};

struct Options {
  std::int64_t horizon{10};
  pluss::subsp::math::qp::Options qp{};
};

struct Result {
  Series control;
  Series controls;
  Series states;
  pluss::subsp::math::qp::Result optimization;
};

class Plan final {
public:
  std::int64_t horizon() const noexcept { return horizon_; }
  std::int64_t states() const noexcept { return states_; }
  std::int64_t inputs() const noexcept { return inputs_; }
  ScalarType dtype() const noexcept { return solver_.dtype(); }
  Platform platform() const noexcept { return solver_.platform(); }

private:
  friend Plan build(const lti::StateSpace &, const Weights &, const Bounds &,
                    Options);
  friend Result solve(const Plan &, const Series &, const Series &,
                      pluss::subsp::math::qp::WarmStart);

  Plan(std::int64_t horizon, std::int64_t states, std::int64_t inputs,
       Series sx, Series su, Series qbar, Series input_lower,
       Series input_upper, Series state_lower, Series state_upper,
       bool has_state_bounds, pluss::subsp::math::qp::Solver solver)
      : horizon_(horizon), states_(states), inputs_(inputs), sx_(std::move(sx)),
        su_(std::move(su)), qbar_(std::move(qbar)),
        input_lower_(std::move(input_lower)),
        input_upper_(std::move(input_upper)),
        state_lower_(std::move(state_lower)),
        state_upper_(std::move(state_upper)),
        has_state_bounds_(has_state_bounds), solver_(std::move(solver)) {}

  std::int64_t horizon_{};
  std::int64_t states_{};
  std::int64_t inputs_{};
  Series sx_;
  Series su_;
  Series qbar_;
  Series input_lower_;
  Series input_upper_;
  Series state_lower_;
  Series state_upper_;
  bool has_state_bounds_{};
  pluss::subsp::math::qp::Solver solver_;
};

// Compiles invariant horizon matrices and factorizes the QP system once.
Plan build(const lti::StateSpace &system, const Weights &weights,
           const Bounds &bounds, Options options = {});

Result solve(const Plan &plan, const Series &initial_state,
             const Series &reference,
             pluss::subsp::math::qp::WarmStart warm = {});

// Condensed finite-horizon regulation for x[k+1] = A*x[k] + B*u[k].
// `reference` is either [states] or [..., horizon, states]. Bounds follow
// the corresponding input/state convention.
Result solve(const lti::StateSpace &system, const Series &initial_state,
             const Series &reference, const Weights &weights,
             const Bounds &bounds, Options options = {},
             pluss::subsp::math::qp::WarmStart warm = {});

} // namespace pluss::subsp::eng::control::mpc
