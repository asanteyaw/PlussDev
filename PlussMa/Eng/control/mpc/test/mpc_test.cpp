#include "pluss/codem/series_factory.h"
#include "pluss/ma/eng.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace control = pluss::subsp::eng::control;
namespace lti = pluss::subsp::eng::lti;

namespace {
void expect(bool value, const std::string &message) {
  if (!value)
    throw std::runtime_error(message);
}
} // namespace

int main() {
  const auto scalar_matrix = [](double value) {
    return pluss::series::make(std::vector<double>{value}, {1, 1});
  };
  const auto system = lti::state(scalar_matrix(1.0), scalar_matrix(1.0),
                                 scalar_matrix(1.0), scalar_matrix(0.0), 1.0);
  control::mpc::Weights weights{scalar_matrix(1.0), scalar_matrix(0.1),
                                scalar_matrix(2.0)};
  control::mpc::Bounds bounds{
      pluss::series::make({-0.4}), pluss::series::make({0.4}),
      pluss::series::make({-2.0}), pluss::series::make({1.05})};
  control::mpc::Options options;
  options.horizon = 4;
  options.qp.abs_tol = 1e-7;
  options.qp.rel_tol = 1e-7;
  options.qp.check_every = 5;

  auto result =
      control::mpc::solve(system, pluss::series::make({0.0}),
                          pluss::series::make({1.0}), weights, bounds, options);
  expect(result.optimization.converged, "MPC QP did not converge");
  expect(result.controls.sizes() == std::vector<std::int64_t>{4, 1},
         "MPC control trajectory shape is wrong");
  expect(result.states.sizes() == std::vector<std::int64_t>{4, 1},
         "MPC state trajectory shape is wrong");
  expect(result.control.select(-1, 0).item<double>() <= 0.40001 &&
             result.control.select(-1, 0).item<double>() >= -0.40001,
         "MPC first control violates bounds");
  expect(result.controls.abs().amax().item<double>() <= 0.40001,
         "MPC trajectory violates input bounds");
  expect(result.states.amax().item<double>() <= 1.05001,
         "MPC trajectory violates state bounds");
  expect(result.states.select(0, 3).select(-1, 0).item<double>() > 0.8,
         "MPC did not approach its reference");

  const auto plan = control::mpc::build(system, weights, bounds, options);
  auto warm = control::mpc::solve(plan, pluss::series::make({0.1}),
                                  pluss::series::make({1.0}),
                                  result.optimization.warm());
  expect(warm.optimization.converged, "warm MPC QP did not converge");
  expect(warm.states.isfinite().all().item<bool>(),
         "warm MPC produced non-finite states");

  auto batched = control::mpc::solve(
      plan, pluss::series::make(std::vector<double>{0.0, 0.25}, {2, 1}),
      pluss::series::make({1.0}));
  expect(batched.optimization.converged, "batched MPC QP did not converge");
  expect(batched.controls.sizes() == std::vector<std::int64_t>{2, 4, 1},
         "batched MPC control shape is wrong");
  expect(batched.states.sizes() == std::vector<std::int64_t>{2, 4, 1},
         "batched MPC state shape is wrong");
  expect(batched.controls.abs().amax().item<double>() <= 0.40001 &&
             batched.states.amax().item<double>() <= 1.05001,
         "batched MPC violates a bound");

  std::cout << "Condensed linear MPC checks passed\n";
}
