<!-- generated from the_other_adam_settings.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The other Adam settings

[Back to Tuning and Robustness](../septima_03_tuning_and_robustness.md)


Each setting changed on its own.

```cpp
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
```

Output

```text
weight decay of 0.1 (the exact minimizer without decay is 3)
  no decay         Converged      iterations 245   x = 3.0000
  coupled decay    NoProgress     iterations 544   x = 2.8571
  decoupled decay  MaxIterations  iterations 3000  x = 2.9594

Rosenbrock, learning rate 0.05, 2000 iterations: objective reached
  defaults (beta1 0.9, beta2 0.999)   8.69e-09
  amsgrad                             2.28e-05
  beta1 = 0.5                         4.28e-03
  beta2 = 0.9                         3.44e-03

maximize: x = 1.0000, objective = 1.0000
```
**Weight decay changes the answer.** Without decay the quadratic's minimum, 3, is found in 245 iterations. Coupled decay of 0.1 adds `0.1 x` to the gradient, so the solver is now minimizing a different function, whose minimum is `6 / 2.1 = 2.8571`, and it finds exactly that, ending with `NoProgress` because that is where it settles. Decoupled decay shrinks the parameter directly instead and, with the same value, is still moving after 3000 iterations at 2.9594. Weight decay is a regularizer: use it when you want the pull toward zero, and never to speed a run up.

**The momentum settings matter less than the learning rate and can hurt.** On the Rosenbrock valley at a learning rate of 0.05 the defaults reach 8.7e-09 after 2000 iterations. `amsgrad` reaches 2.3e-05, a shorter memory of the gradient (`beta1 = 0.5`) 4.3e-03, and a shorter memory of the squared gradient (`beta2 = 0.9`) 3.4e-03. Change them only with a reason, and one at a time.

**`maximize` flips the direction.** The last line maximizes `-x^2 + 2x` and finds `x = 1`, objective 1. The reported `objective_value` is the function you wrote, not its negative.
