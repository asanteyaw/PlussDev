#include <pluss/pluss.h>
#include <pluss/nn.h>
#include <pluss/subsp/math/diff.h>

#include <iostream>
#include <vector>

int main() {
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::subsp::math::diff::Trace;

  const Config cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);

  Series w = pluss::make_series(std::vector<double>{0.25, 0.25}, {2}, cfg);
  Series b = pluss::make_series(std::vector<double>{-1.0}, {1}, cfg);
  Series w_tape = pluss::make_series(std::vector<double>{0.25, 0.25}, {2}, cfg);
  Series b_tape = pluss::make_series(std::vector<double>{-1.0}, {1}, cfg);

  const Series w_target = pluss::make_series(std::vector<double>{2.0, 1.0}, {2}, cfg);
  const Series b_target = pluss::make_series(std::vector<double>{0.5}, {1}, cfg);

  pluss::nn::optim::MinimaxOptions w_opt{};
  w_opt.primal_method = pluss::nn::optim::MinimaxPrimalMethod::AdamW;
  w_opt.learning_rate = 0.03;
  w_opt.weight_decay = 1e-3;
  w_opt.dual_schedule = pluss::nn::optim::MinimaxSchedule::ViolationAdaptive;
  w_opt.penalty_schedule = pluss::nn::optim::MinimaxSchedule::Exponential;
  w_opt.penalty_exponential_growth = 1.002;
  w_opt.dual_learning_rate = 0.08;
  w_opt.equality_penalty = 8.0;
  w_opt.inequality_penalty = 8.0;

  pluss::nn::optim::MinimaxOptions b_opt = w_opt;
  b_opt.primal_method = pluss::nn::optim::MinimaxPrimalMethod::SGD;
  b_opt.learning_rate = 0.01;
  b_opt.momentum = 0.9;

  pluss::nn::optim::MinimaxConstraints constraints{};
  constraints.lower_bounds = {
      pluss::make_series(std::vector<double>{0.0, 0.0}, {2}, cfg),
      Series::undefined(),
  };
  constraints.inequalities = [](const std::vector<Series>& p) {
    // Nonlinear inequality: ||w||^2 - 5 <= 0.
    return std::vector<Series>{p[0].square().sum().sub(5.0)};
  };
  constraints.inequalities_trace = [](const std::vector<Trace>& p) {
    return std::vector<Trace>{pluss::subsp::math::diff::sum(pluss::subsp::math::diff::square(p[0])) - 5.0};
  };
  constraints.equalities = [](const std::vector<Series>& p) {
    // Equality: sum(w) + b - 2 = 0.
    return std::vector<Series>{p[0].sum().add(p[1]).sub(2.0)};
  };
  constraints.equalities_trace = [](const std::vector<Trace>& p) {
    return std::vector<Trace>{pluss::subsp::math::diff::sum(p[0]) + p[1] - 2.0};
  };

  pluss::nn::optim::Minimax minimax(
      std::vector<pluss::nn::optim::MinimaxGroup>{
          pluss::nn::optim::MinimaxGroup{std::vector<Series*>{&w}, w_opt},
          pluss::nn::optim::MinimaxGroup{std::vector<Series*>{&b}, b_opt},
      },
      constraints);

  pluss::nn::optim::Minimax minimax_tape(
      std::vector<pluss::nn::optim::MinimaxGroup>{
          pluss::nn::optim::MinimaxGroup{std::vector<Series*>{&w_tape}, w_opt},
          pluss::nn::optim::MinimaxGroup{std::vector<Series*>{&b_tape}, b_opt},
      },
      constraints);

  std::cout << "[minimax-multiblock] explicit gradients\n";
  for (int iter = 1; iter <= 200; ++iter) {
    const Series gw = w.sub(w_target);
    const Series gb = b.sub(b_target);
    minimax.step(std::vector<Series>{gw, gb});
    if (iter % 50 == 0) {
      const double eq = w.sum().add(b).sub(2.0).item<double>();
      const double ineq = w.square().sum().sub(5.0).item<double>();
      std::cout << "iter=" << iter
                << " obj=" << w.sub(w_target).square().sum().add(b.sub(b_target).square().sum()).mul(0.5).item<double>()
                << " eq=" << eq
                << " ineq=" << ineq
                << '\n';
    }
  }
  std::cout << "final explicit w=[" << w.select(0, 0).item<double>() << ", " << w.select(0, 1).item<double>()
            << "] b=" << b.item<double>() << '\n';

  std::cout << "[minimax-multiblock] tape gradients via optimizer.step()\n";
  for (int iter = 1; iter <= 200; ++iter) {
    auto tw = pluss::subsp::math::diff::trace(w_tape, true);
    auto tb = pluss::subsp::math::diff::trace(b_tape, true);
    const Series loss = tw.value().sub(w_target).square().sum().add(tb.value().sub(b_target).square().sum()).mul(0.5);
    tw.clear_sensitivity();
    tb.clear_sensitivity();
    loss.reverse();
    minimax_tape.step();
    if (iter % 50 == 0) {
      const double eq = w_tape.sum().add(b_tape).sub(2.0).item<double>();
      const double ineq = w_tape.square().sum().sub(5.0).item<double>();
      std::cout << "iter=" << iter
                << " obj=" << loss.item<double>()
                << " eq=" << eq
                << " ineq=" << ineq
                << '\n';
    }
  }
  std::cout << "final tape w=[" << w_tape.select(0, 0).item<double>() << ", " << w_tape.select(0, 1).item<double>()
            << "] b=" << b_tape.item<double>() << '\n';
  return 0;
}
