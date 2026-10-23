#include "pluss/subsp/eng/control/mpc.h"

#include "CodeM/core/Errors.h"
#include "CodeM/core/TypePromotion.h"
#include "pluss/codem/series_factory.h"

#include <cmath>
#include <string>
#include <vector>

namespace pluss::subsp::eng::control::mpc {
namespace {

constexpr const char *op = "eng::control::mpc::solve";

Series join(const std::vector<Series> &parts, std::int64_t axis) {
  PLUSS_CHECK(!parts.empty(), std::string(op) + ": internal empty assembly");
  return parts.front().unite(
      std::vector<Series>(parts.begin() + 1, parts.end()), axis);
}

Series block_diag(const Series &block, const Series &last, std::int64_t count) {
  std::vector<Series> rows;
  rows.reserve(static_cast<std::size_t>(count));
  for (std::int64_t row = 0; row < count; ++row) {
    std::vector<Series> columns;
    columns.reserve(static_cast<std::size_t>(count));
    for (std::int64_t column = 0; column < count; ++column) {
      columns.push_back(row == column ? (row + 1 == count ? last : block)
                                      : block.mul(0.0));
    }
    rows.push_back(join(columns, -1));
  }
  return join(rows, -2);
}

Series trajectory(const Series &value, std::int64_t horizon, std::int64_t width,
                  const char *name) {
  PLUSS_CHECK(value.defined(),
              std::string(op) + ": " + name + " must be defined");
  PLUSS_CHECK(value.dim() >= 1 && value.size(-1) == width,
              std::string(op) + ": " + name + " has the wrong trailing width");
  if (value.dim() == 1) {
    return value.unsqueeze(0).expand({horizon, width}, true);
  }
  PLUSS_CHECK(value.size(-2) == horizon,
              std::string(op) + ": " + name +
                  " must be a vector or end in [horizon, width]");
  return value;
}

Series flatten_trajectory(const Series &value) {
  std::vector<std::int64_t> shape(value.sizes().begin(), value.sizes().end());
  const std::int64_t width = shape.back();
  shape.pop_back();
  const std::int64_t horizon = shape.back();
  shape.pop_back();
  shape.push_back(horizon * width);
  return value.reshape(shape);
}

Series unflatten_trajectory(const Series &value, std::int64_t horizon,
                            std::int64_t width) {
  std::vector<std::int64_t> shape(value.sizes().begin(), value.sizes().end());
  shape.pop_back();
  shape.push_back(horizon);
  shape.push_back(width);
  return value.reshape(shape);
}

void validate_weight(const Series &value, std::int64_t width,
                     const char *name) {
  PLUSS_CHECK(value.defined() && value.dim() >= 2 && value.size(-2) == width &&
                  value.size(-1) == width,
              std::string(op) + ": " + name + " must end in a square " +
                  std::to_string(width) + " by " + std::to_string(width) +
                  " matrix");
  PLUSS_CHECK(value.dtype().is_floating(),
              std::string(op) + ": " + name + " must be real floating");
}

void validate_cost(const Series &value, const char *name, bool strict) {
  const double tolerance =
      value.scalar_type() == ScalarType::Float32 ? 2e-5 : 1e-10;
  PLUSS_CHECK(value.isfinite().all().item<bool>(),
              std::string(op) + ": " + name + " must be finite");
  PLUSS_CHECK(value.allclose(value.transpose(-2, -1), tolerance, tolerance),
              std::string(op) + ": " + name + " must be symmetric");
  const double floor = value.linearalg.eigvalsh().amin().item<double>();
  PLUSS_CHECK(std::isfinite(floor) &&
                  (strict ? floor > tolerance : floor >= -tolerance),
              std::string(op) + ": " + name +
                  (strict ? " must be positive definite"
                          : " must be positive semidefinite"));
}

} // namespace

Plan build(const lti::StateSpace &system, const Weights &weights,
           const Bounds &bounds, Options options) {
  PLUSS_CHECK(system.is_disc(), std::string(op) + ": system must be discrete");
  PLUSS_CHECK(options.horizon > 0,
              std::string(op) + ": horizon must be positive");
  const std::int64_t horizon = options.horizon;
  const std::int64_t states = system.states();
  const std::int64_t inputs = system.inputs();
  validate_weight(weights.state, states, "weights.state");
  validate_weight(weights.input, inputs, "weights.input");
  const Series terminal =
      weights.terminal.defined() ? weights.terminal : weights.state;
  validate_weight(terminal, states, "weights.terminal");
  PLUSS_CHECK(system.a().platform() == weights.state.platform() &&
                  system.a().platform() == weights.input.platform() &&
                  system.a().platform() == terminal.platform(),
              std::string(op) + ": system and weights must share one platform");

  ScalarType dtype = system.a().scalar_type();
  dtype = codem::promote_types(dtype, weights.state.scalar_type());
  dtype = codem::promote_types(dtype, weights.input.scalar_type());
  dtype = codem::promote_types(dtype, terminal.scalar_type());
  const Series a = system.a().to(dtype);
  const Series b = system.b().to(dtype);
  const Series q = weights.state.to(dtype);
  const Series r = weights.input.to(dtype);
  const Series qf = terminal.to(dtype);
  validate_cost(q, "weights.state", false);
  validate_cost(r, "weights.input", true);
  validate_cost(qf, "weights.terminal", false);

  std::vector<Series> powers;
  powers.reserve(static_cast<std::size_t>(horizon + 1));
  powers.push_back(pluss::series::eye(states, dtype).to(a.platform()));
  for (std::int64_t k = 1; k <= horizon; ++k)
    powers.push_back(a.matmul(powers.back()));

  std::vector<Series> sx_rows;
  std::vector<Series> su_rows;
  sx_rows.reserve(static_cast<std::size_t>(horizon));
  su_rows.reserve(static_cast<std::size_t>(horizon));
  for (std::int64_t row = 0; row < horizon; ++row) {
    sx_rows.push_back(powers[static_cast<std::size_t>(row + 1)]);
    std::vector<Series> blocks;
    blocks.reserve(static_cast<std::size_t>(horizon));
    for (std::int64_t column = 0; column < horizon; ++column) {
      blocks.push_back(
          column <= row
              ? powers[static_cast<std::size_t>(row - column)].matmul(b)
              : b.mul(0.0));
    }
    su_rows.push_back(join(blocks, -1));
  }
  const Series sx = join(sx_rows, -2);
  const Series su = join(su_rows, -2);
  const Series qbar = block_diag(q, qf, horizon);
  const Series rbar = block_diag(r, r, horizon);
  const Series sut = su.transpose(-2, -1);
  const Series p = sut.matmul(qbar).matmul(su).add(rbar);

  PLUSS_CHECK(bounds.input_lower.defined() && bounds.input_upper.defined(),
              std::string(op) + ": input bounds must be defined");
  PLUSS_CHECK(bounds.input_lower.platform() == a.platform() &&
                  bounds.input_upper.platform() == a.platform(),
              std::string(op) +
                  ": input bounds must share the system platform");
  const Series in_lo =
      flatten_trajectory(trajectory(bounds.input_lower.to(a.platform(), dtype),
                                    horizon, inputs, "bounds.input_lower"));
  const Series in_hi =
      flatten_trajectory(trajectory(bounds.input_upper.to(a.platform(), dtype),
                                    horizon, inputs, "bounds.input_upper"));
  const std::int64_t controls = horizon * inputs;
  Series constraint = pluss::series::eye(controls, dtype).to(a.platform());

  const bool has_state_bounds =
      bounds.state_lower.defined() || bounds.state_upper.defined();
  PLUSS_CHECK(bounds.state_lower.defined() == bounds.state_upper.defined(),
              std::string(op) + ": state bounds must be supplied together");
  if (has_state_bounds) {
    PLUSS_CHECK(bounds.state_lower.platform() == a.platform() &&
                    bounds.state_upper.platform() == a.platform(),
                std::string(op) +
                    ": state bounds must share the system platform");
    const Series state_lo = flatten_trajectory(
        trajectory(bounds.state_lower.to(a.platform(), dtype), horizon, states,
                   "bounds.state_lower"));
    const Series state_hi = flatten_trajectory(
        trajectory(bounds.state_upper.to(a.platform(), dtype), horizon, states,
                   "bounds.state_upper"));
    std::vector<std::int64_t> eye_shape(su.sizes().begin(), su.sizes().end());
    eye_shape[eye_shape.size() - 2] = controls;
    eye_shape[eye_shape.size() - 1] = controls;
    constraint = constraint.expand(eye_shape, true).unite({su}, -2);
    return Plan(horizon, states, inputs, sx, su, qbar, in_lo, in_hi, state_lo,
                state_hi, true,
                pluss::subsp::math::qp::prepare(p, constraint, options.qp));
  }

  return Plan(horizon, states, inputs, sx, su, qbar, in_lo, in_hi,
              pluss::series::undefined(), pluss::series::undefined(), false,
              pluss::subsp::math::qp::prepare(p, constraint, options.qp));
}

Result solve(const Plan &plan, const Series &initial_state,
             const Series &reference, pluss::subsp::math::qp::WarmStart warm) {
  PLUSS_CHECK(initial_state.defined() && initial_state.dim() >= 1 &&
                  initial_state.size(-1) == plan.states_,
              std::string(op) + ": initial_state must end in [states]");
  PLUSS_CHECK(initial_state.dtype().is_floating() &&
                  reference.dtype().is_floating(),
              std::string(op) + ": state and reference must be real floating");
  PLUSS_CHECK(initial_state.platform() == plan.platform() &&
                  reference.platform() == plan.platform(),
              std::string(op) +
                  ": state and reference must share the plan platform");
  const Series x0 = initial_state.to(plan.dtype());
  const Series ref = flatten_trajectory(trajectory(
      reference.to(plan.dtype()), plan.horizon_, plan.states_, "reference"));
  const Series nominal = plan.sx_.matmul(x0.unsqueeze(-1)).squeeze(-1);
  const Series error = nominal.sub(ref);
  const Series linear = plan.su_.transpose(-2, -1)
                            .matmul(plan.qbar_.matmul(error.unsqueeze(-1)))
                            .squeeze(-1);
  Series lower = plan.input_lower_;
  Series upper = plan.input_upper_;
  if (plan.has_state_bounds_) {
    const Series batch_zero = nominal.sum(-1, false).mul(0.0).unsqueeze(-1);
    lower = plan.input_lower_.add(batch_zero)
                .unite({plan.state_lower_.sub(nominal)}, -1);
    upper = plan.input_upper_.add(batch_zero)
                .unite({plan.state_upper_.sub(nominal)}, -1);
  }

  auto optimization = plan.solver_.solve(linear, lower, upper, std::move(warm));
  const Series controls_flat = optimization.x;
  const Series states_flat =
      nominal.add(plan.su_.matmul(controls_flat.unsqueeze(-1)).squeeze(-1));
  const Series controls_out =
      unflatten_trajectory(controls_flat, plan.horizon_, plan.inputs_);
  return {controls_out.select(-2, 0), controls_out,
          unflatten_trajectory(states_flat, plan.horizon_, plan.states_),
          std::move(optimization)};
}

Result solve(const lti::StateSpace &system, const Series &initial_state,
             const Series &reference, const Weights &weights,
             const Bounds &bounds, Options options,
             pluss::subsp::math::qp::WarmStart warm) {
  return solve(build(system, weights, bounds, options), initial_state,
               reference, std::move(warm));
}

} // namespace pluss::subsp::eng::control::mpc
