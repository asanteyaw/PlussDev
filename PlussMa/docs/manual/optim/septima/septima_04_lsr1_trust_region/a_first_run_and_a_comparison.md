<!-- generated from a_first_run_and_a_comparison.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# A first run, and a comparison

[Back to The L-SR1 Trust-Region Optimizer](../septima_04_lsr1_trust_region.md)


The first part prints every field of the result for a quadratic. The second part solves the ten-variable chained Rosenbrock function with all three optimizers.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::optim;
using optim::Trace;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  {
    auto quadratic = [](const Trace& x) -> Trace { return optim::sum(optim::square(x - 3.0)); };
    const auto r = optim::l_sr1_trust_region(quadratic, vec({0.0, 0.0}));
    std::cout << std::boolalpha << "converged            " << r.converged << "\n"
              << "exit_flag            " << static_cast<int>(r.exit_flag) << "  (1 = Converged)\n"
              << "message              " << r.message << "\n"
              << "params               " << r.params.select(0, 0).item<double>() << ", " << r.params.select(0, 1).item<double>() << "\n"
              << "objective_value      " << r.objective_value << "\n"
              << std::scientific << std::setprecision(1) << "gradient_norm        " << r.gradient_norm << "  (the 2-norm of the gradient)\n"
              << "step_norm            " << r.step_norm << "\n"
              << "iteration            " << r.iteration << "\n"
              << "function_evaluations " << r.function_evaluations << "  (the trust-region step may be tried more than once)\n\n";
  }

  const int n = 10;
  std::vector<double> first, second, start;
  for (int i = 0; i + 1 < n; ++i) { first.push_back(i); second.push_back(i + 1); }
  for (int i = 0; i < n; ++i) start.push_back(i % 2 == 0 ? -1.2 : 1.0);
  const auto head = pluss::make_series(first, {n - 1}).to(pluss::ScalarType::Int64);
  const auto tail = pluss::make_series(second, {n - 1}).to(pluss::ScalarType::Int64);
  auto chain = [&](const Trace& x) -> Trace {
    const Trace a = pluss::subsp::math::diff::index_select(x, 0, head), b = pluss::subsp::math::diff::index_select(x, 0, tail);
    return optim::sum(100.0 * optim::square(b - optim::square(a)) + optim::square(1.0 - a));
  };

  std::cout << std::left << std::setw(12) << "optimizer" << std::setw(14) << "exit" << std::setw(8) << "iters" << std::setw(8) << "evals" << std::setw(12) << "objective"
            << "gradient\n";
  {
    const auto r = optim::l_sr1_trust_region(chain, vec(start));
    std::cout << std::left << std::setw(12) << "L-SR1" << std::setw(14) << (r.converged ? "Converged" : "not converged") << std::setw(8) << r.iteration << std::setw(8)
              << r.function_evaluations << std::scientific << std::setprecision(1) << std::setw(12) << r.objective_value << r.gradient_norm << "\n";
  }
  {
    optim::SeptimaSolveOptions options;
    options.max_iterations = 2000;
    options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
    const auto r = optim::septima(chain, vec(start), options);
    std::cout << std::left << std::setw(12) << "L-BFGS" << std::setw(14) << (r.converged ? "Converged" : "NoProgress") << std::setw(8) << r.iteration << std::setw(8)
              << r.function_evaluations << std::scientific << std::setprecision(1) << std::setw(12) << r.objective_value << r.gradient_inf_norm << "\n";
  }
  {
    optim::SeptimaSolveOptions options;
    options.max_iterations = 5000;
    options.step.adam.learning_rate = 0.05;
    const auto r = optim::septima(chain, vec(start), options);
    std::cout << std::left << std::setw(12) << "Adam" << std::setw(14) << (r.converged ? "Converged" : "MaxIterations") << std::setw(8) << r.iteration << std::setw(8)
              << r.function_evaluations << std::scientific << std::setprecision(1) << std::setw(12) << r.objective_value << r.gradient_inf_norm << "\n";
  }
  return 0;
}
```

Output

```text
converged            true
exit_flag            1  (1 = Converged)
message              l_sr1_trust_region: converged (||g||_2 <= gradient_tolerance)
params               3, 3
objective_value      0
gradient_norm        0.0e+00  (the 2-norm of the gradient)
step_norm            0.0e+00
iteration            3
function_evaluations 4  (the trust-region step may be tried more than once)

optimizer   exit          iters   evals   objective   gradient
L-SR1       Converged     119     286     9.2e-20     1.1e-08
L-BFGS      NoProgress    124     124     3.3e-10     2.5e-04
Adam        Converged     4388    4388    2.1e-17     3.6e-09
```
On the quadratic the method converges in 3 iterations and 4 function evaluations. The evaluations exceed the iterations because a trust-region step may be tried more than once: a step the function does not reward is rejected, the radius shrinks, and the step is tried again. `gradient_norm` is the **2-norm** of the gradient, where Septima's `gradient_inf_norm` is the largest component; the default tolerance is 1e-6 on it.

On the chain, the three optimizers differ in what they achieve, not only in how long they take. L-SR1 converges in 119 iterations and 286 evaluations to an objective of 9.2e-20 with a gradient of 1.1e-08. L-BFGS spends 124 iterations and 124 evaluations and stops with `NoProgress` at 3.3e-10 and a gradient of 2.5e-04: it uses one evaluation per iteration where L-SR1 uses about two and a half, but it cannot get any closer. Adam, at a learning rate of 0.05, converges after 4388 iterations. When you need a very accurate answer to a smooth unconstrained problem, L-SR1 is the optimizer to try.
