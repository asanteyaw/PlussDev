// The other Adam settings, each changed on its own. (1) `weight_decay` adds a pull toward zero: the problem is to
// minimize sum((x - 3)^2), whose answer is 3, and a decay of 0.1 moves the answer, in two different ways. Coupled decay adds
// 0.1 x to the gradient, so the answer solves 2 (x - 3) + 0.1 x = 0, which is 6 / 2.1 = 2.857; decoupled decay shrinks the
// parameter directly after the step and is much slower to settle. (2) `amsgrad`, `beta1` and `beta2` on the Rosenbrock
// valley: the objective after 2000 iterations at a learning rate of 0.05. (3) `maximize` turns the minimizer into a
// maximizer: -x^2 + 2x has its maximum 1 at x = 1.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::optim;
using optim::Trace;
using Pack = optim::ParameterPack<Trace>;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  std::cout << std::fixed << std::setprecision(4);
  {
    auto quadratic = [](const Trace& x) -> Trace { return optim::sum(optim::square(x - 3.0)); };
    std::cout << "weight decay of 0.1 (the exact minimizer without decay is 3)\n";
    struct Row { const char* label; double decay; bool decoupled; };
    for (const Row row : {Row{"no decay", 0.0, false}, Row{"coupled decay", 0.1, false}, Row{"decoupled decay", 0.1, true}}) {
      optim::SeptimaSolveOptions options;
      options.max_iterations = 3000;
      options.gradient_tolerance = 1e-5;
      options.step_tolerance = 1e-5;
      options.step.adam.learning_rate = 0.05;
      options.step.adam.weight_decay = row.decay;
      options.step.adam.decoupled_weight_decay = row.decoupled;
      const auto r = optim::septima(quadratic, vec({0.0, 0.0}), options);
      const char* ended = r.converged ? "Converged" : r.exit_flag == optim::SeptimaExitFlag::NoProgress ? "NoProgress" : "MaxIterations";
      std::cout << "  " << std::left << std::setw(17) << row.label << std::setw(15) << ended << "iterations " << std::setw(6) << r.iteration
                << "x = " << r.params[0].select(0, 0).item<double>() << "\n";
    }
  }
  {
    std::function<Trace(const Pack&)> rosenbrock = [](const Pack& p) {
      return 100.0 * optim::square(p.at("y") - optim::square(p.at("x"))) + optim::square(1.0 - p.at("x"));
    };
    const auto start = optim::pack_scalars({{"x", scalar(-1.2)}, {"y", scalar(1.0)}});
    std::cout << "\nRosenbrock, learning rate 0.05, 2000 iterations: objective reached\n";
    struct Row { const char* label; bool amsgrad; double beta1; double beta2; };
    for (const Row row : {Row{"defaults (beta1 0.9, beta2 0.999)", false, 0.9, 0.999}, Row{"amsgrad", true, 0.9, 0.999},
                          Row{"beta1 = 0.5", false, 0.5, 0.999}, Row{"beta2 = 0.9", false, 0.9, 0.9}}) {
      optim::SeptimaSolveOptions options;
      options.max_iterations = 2000;
      options.step.adam.learning_rate = 0.05;
      options.step.adam.amsgrad = row.amsgrad;
      options.step.adam.beta1 = row.beta1;
      options.step.adam.beta2 = row.beta2;
      const auto r = optim::septima(rosenbrock, start, options);
      std::cout << "  " << std::left << std::setw(36) << row.label << std::scientific << std::setprecision(2) << r.objective_value << std::fixed
                << std::setprecision(4) << "\n";
    }
  }
  {
    auto concave = [](const Trace& x) -> Trace { return optim::sum(optim::square(x)) * -1.0 + optim::sum(x) * 2.0; };
    optim::SeptimaSolveOptions options;
    options.max_iterations = 500;
    options.step.adam.learning_rate = 0.1;
    options.step.adam.maximize = true;
    const auto r = optim::septima(concave, vec({0.0}), options);
    std::cout << "\nmaximize: x = " << r.params[0].item<double>() << ", objective = " << r.objective_value << "\n";
  }
  return 0;
}
