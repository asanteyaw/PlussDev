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

  Series w = pluss::make_series(std::vector<double>{0.25, 0.25}, {2}, cfg);
  Series b = pluss::make_series(std::vector<double>{-1.0}, {1}, cfg);
  Series w_tape = pluss::make_series(std::vector<double>{0.25, 0.25}, {2}, cfg);
  Series b_tape = pluss::make_series(std::vector<double>{-1.0}, {1}, cfg);

  const Series w_target = pluss::make_series(std::vector<double>{2.0, 1.0}, {2}, cfg);
  const Series b_target = pluss::make_series(std::vector<double>{0.5}, {1}, cfg);

  pluss::optim::MinimaxOptions options{};
  options.adam.learning_rate = 0.03;
  options.adam.weight_decay = 1e-3;
  options.dual_learning_rate = 0.08;
  options.equality_penalty = 8.0;
  options.inequality_penalty = 8.0;
  options.project_bounds = true;
  options.enable_constraint_autodiff = true;

  pluss::optim::MinimaxConstraints constraints{};
  constraints.equalities = [](const std::vector<Series>& p) {
    return std::vector<Series>{p[0].sum() + p[1] - 2.0};
  };
  constraints.equalities_trace = [](const std::vector<Trace>& p) {
    return std::vector<Trace>{pluss::subsp::math::diff::sum(p[0]) + p[1] - 2.0};
  };
  constraints.inequalities = [](const std::vector<Series>& p) {
    return std::vector<Series>{p[0].square().sum() - 5.0};
  };
  constraints.inequalities_trace = [](const std::vector<Trace>& p) {
    return std::vector<Trace>{pluss::subsp::math::diff::sum(pluss::subsp::math::diff::square(p[0])) - 5.0};
  };
  constraints.lbs = {
      pluss::make_series(std::vector<double>{0.0, 0.0}, {2}, cfg),
      Series::undefined(),
  };

  std::vector<Series> params{w, b};
  std::vector<Series> params_tape{w_tape, b_tape};
  pluss::optim::MinimaxState state = pluss::optim::minimax_state_like(params, constraints);
  pluss::optim::MinimaxState state_tape = pluss::optim::minimax_state_like(params_tape, constraints);

  std::cout << "[functional minimax multiblock] explicit gradients\n";
  for (int iter = 1; iter <= 200; ++iter) {
    const Series grad_w = params[0] - w_target;
    const Series grad_b = params[1] - b_target;
    pluss::optim::minimax_step_inplace(params, std::vector<Series>{grad_w, grad_b}, state, options, constraints);
    if (iter % 50 == 0) {
      const double obj = (((params[0] - w_target).square().sum() + (params[1] - b_target).square().sum()) * 0.5)
                             .item<double>();
      const double eq = (params[0].sum() + params[1] - 2.0).item<double>();
      const double ineq = (params[0].square().sum() - 5.0).item<double>();
      std::cout << "iter=" << iter << " obj=" << obj << " eq=" << eq << " ineq=" << ineq << '\n';
    }
  }
  std::cout << "final explicit w=[" << params[0].select(0, 0).item<double>() << ", "
            << params[0].select(0, 1).item<double>() << "] b=" << params[1].item<double>() << '\n';

  std::cout << "[functional minimax multiblock] tape gradients\n";
  for (int iter = 1; iter <= 200; ++iter) {
    auto tracked_w = pluss::subsp::math::diff::trace(params_tape[0], true);
    auto tracked_b = pluss::subsp::math::diff::trace(params_tape[1], true);
    const Series loss = ((tracked_w.value() - w_target).square().sum() +
                         (tracked_b.value() - b_target).square().sum()) *
                        0.5;
    tracked_w.clear_sensitivity();
    tracked_b.clear_sensitivity();
    loss.reverse();
    params_tape[0] = tracked_w.value();
    params_tape[1] = tracked_b.value();
    pluss::optim::minimax_step_inplace(params_tape, state_tape, options, constraints);
    if (iter % 50 == 0) {
      const double eq = (params_tape[0].sum() + params_tape[1] - 2.0).item<double>();
      const double ineq = (params_tape[0].square().sum() - 5.0).item<double>();
      std::cout << "iter=" << iter << " obj=" << loss.item<double>() << " eq=" << eq << " ineq=" << ineq << '\n';
    }
  }
  std::cout << "final tape w=[" << params_tape[0].select(0, 0).item<double>() << ", "
            << params_tape[0].select(0, 1).item<double>() << "] b=" << params_tape[1].item<double>() << '\n';

  return 0;
}
