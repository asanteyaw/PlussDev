#include <pluss/pluss.h>
#include <pluss/ma.h>
#include <pluss/subsp/math/diff.h>

#include <iostream>
#include <vector>

int main() {
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::subsp::math::diff::Trace;

  const Config cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  Series x = pluss::make_series(std::vector<double>{0.5, -0.5}, {2}, cfg);
  Series x_tape = pluss::make_series(std::vector<double>{0.5, -0.5}, {2}, cfg);
  const Series target = pluss::make_series(std::vector<double>{2.0, 1.0}, {2}, cfg);

  pluss::optim::MinimaxOptions options{};
  options.adam.learning_rate = 0.04;
  options.dual_learning_rate = 0.08;
  options.equality_penalty = 6.0;
  options.inequality_penalty = 6.0;
  options.project_bounds = true;
  options.enable_constraint_autodiff = true;

  pluss::optim::MinimaxConstraints constraints{};
  constraints.equalities = [](const std::vector<Series>& p) {
    return std::vector<Series>{p[0].sum() - 3.0};
  };
  constraints.equalities_trace = [](const std::vector<Trace>& p) {
    return std::vector<Trace>{pluss::subsp::math::diff::sum(p[0]) - 3.0};
  };
  constraints.inequalities = [](const std::vector<Series>& p) {
    return std::vector<Series>{p[0].square().sum() - 10.0};
  };
  constraints.inequalities_trace = [](const std::vector<Trace>& p) {
    return std::vector<Trace>{pluss::subsp::math::diff::sum(pluss::subsp::math::diff::square(p[0])) - 10.0};
  };
  constraints.lbs = {pluss::make_series(std::vector<double>{0.0, 0.0}, {2}, cfg)};

  std::vector<Series> params{x};
  std::vector<Series> params_tape{x_tape};
  pluss::optim::MinimaxState state = pluss::optim::minimax_state_like(params, constraints);
  pluss::optim::MinimaxState state_tape = pluss::optim::minimax_state_like(params_tape, constraints);

  std::cout << "[functional minimax] explicit gradients\n";
  for (int iter = 1; iter <= 120; ++iter) {
    const Series grad = params[0] - target;
    pluss::optim::minimax_step_inplace(params, std::vector<Series>{grad}, state, options, constraints);
    if (iter % 30 == 0) {
      const double obj = ((params[0] - target).square().sum() * 0.5).item<double>();
      const double eq = (params[0].sum() - 3.0).item<double>();
      const double ineq = (params[0].square().sum() - 10.0).item<double>();
      std::cout << "iter=" << iter << " obj=" << obj << " eq=" << eq << " ineq=" << ineq << '\n';
    }
  }
  std::cout << "final explicit: [" << params[0].select(0, 0).item<double>()
            << ", " << params[0].select(0, 1).item<double>() << "]\n";

  std::cout << "[functional minimax] tape gradients\n";
  for (int iter = 1; iter <= 120; ++iter) {
    auto tracked = pluss::subsp::math::diff::trace(params_tape[0], true);
    const Series loss = (tracked.value() - target).square().sum() * 0.5;
    tracked.clear_sensitivity();
    loss.reverse();
    params_tape[0] = tracked.value();
    pluss::optim::minimax_step_inplace(params_tape, state_tape, options, constraints);
    if (iter % 30 == 0) {
      const double eq = (params_tape[0].sum() - 3.0).item<double>();
      const double ineq = (params_tape[0].square().sum() - 10.0).item<double>();
      std::cout << "iter=" << iter << " obj=" << loss.item<double>() << " eq=" << eq << " ineq=" << ineq << '\n';
    }
  }
  std::cout << "final tape: [" << params_tape[0].select(0, 0).item<double>()
            << ", " << params_tape[0].select(0, 1).item<double>() << "]\n";

  return 0;
}
