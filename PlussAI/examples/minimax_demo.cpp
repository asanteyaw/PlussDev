#include <pluss/pluss.h>
#include <pluss/nn.h>
#include <pluss/subsp/math/diff.h>

#include <iostream>
#include <vector>

int main() {
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  const Config cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);

  Series params = pluss::make_series(std::vector<float>{2.0f, -1.0f}, {2}, cfg);
  Series params_tape = pluss::make_series(std::vector<float>{2.0f, -1.0f}, {2}, cfg);
  const Series target = pluss::make_series(std::vector<float>{3.0f, 3.0f}, {2}, cfg);

  pluss::nn::optim::MinimaxOptions options{};
  // Switchable primal optimizer inside Minimax: Adam / AdamW / SGD / RMSprop.
  options.primal_method = pluss::nn::optim::MinimaxPrimalMethod::AdamW;
  options.learning_rate = 0.05;
  options.dual_learning_rate = 0.1;
  options.inequality_penalty = 10.0;

  pluss::nn::optim::MinimaxConstraints constraints{};
  constraints.lower_bounds = {pluss::make_series(std::vector<float>{0.0f, 0.0f}, {2}, cfg)};
  constraints.inequalities = [](const std::vector<Series>& p) {
    return std::vector<Series>{p[0].sum().sub(4.0)};
  };
  constraints.inequalities_trace = [](const std::vector<pluss::subsp::math::diff::Trace>& p) {
    return std::vector<pluss::subsp::math::diff::Trace>{pluss::subsp::math::diff::sum(p[0]) - 4.0};
  };

  pluss::nn::optim::Minimax minimax({&params}, options, constraints);
  pluss::nn::optim::Minimax minimax_tape({&params_tape}, options, constraints);

  std::cout << "[minimax] explicit gradients\n";
  for (int iter = 1; iter <= 120; ++iter) {
    minimax.step(std::vector<Series>{params.sub(target)});
    if (iter % 30 == 0) {
      const double loss = params.sub(target).square().sum().mul(0.5).item<double>();
      const double ineq = params.sum().sub(4.0).item<double>();
      std::cout << "iter=" << iter << " loss=" << loss << " ineq(sum(x)-4)=" << ineq << '\n';
    }
  }
  std::cout << "final explicit: ["
            << params.select(0, 0).item<double>() << ", "
            << params.select(0, 1).item<double>() << "]\n";

  std::cout << "[minimax] tape gradients via optimizer.step()\n";
  for (int iter = 1; iter <= 120; ++iter) {
    auto tx = pluss::subsp::math::diff::trace(params_tape, true);
    const Series loss = tx.value().sub(target).square().sum().mul(0.5);
    tx.clear_sensitivity();
    loss.reverse();
    minimax_tape.step();
    if (iter % 30 == 0) {
      const double ineq = params_tape.sum().sub(4.0).item<double>();
      std::cout << "iter=" << iter << " loss=" << loss.item<double>() << " ineq(sum(x)-4)=" << ineq << '\n';
    }
  }
  std::cout << "final tape: ["
            << params_tape.select(0, 0).item<double>() << ", "
            << params_tape.select(0, 1).item<double>() << "]\n";
  return 0;
}
