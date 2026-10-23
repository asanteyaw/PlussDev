<!-- generated from the_settings_that_matter.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The settings that matter

[Back to The L-SR1 Trust-Region Optimizer](../septima_04_lsr1_trust_region.md)


The memory of the curvature model, and the starting size of the trust region, on the chain; then the first twelve iterations.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::optim;
using optim::Trace;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

const char* name_of(const optim::Lsr1Result& r) {
  switch (r.exit_flag) {
    case optim::Lsr1ExitFlag::Converged: return "Converged";
    case optim::Lsr1ExitFlag::MaxIterations: return "MaxIterations";
    case optim::Lsr1ExitFlag::NoProgress: return "NoProgress";
    default: return "other";
  }
}

int main() {
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

  std::cout << std::left << std::setw(22) << "setting" << std::setw(15) << "exit" << std::setw(8) << "iters" << std::setw(8) << "evals" << "objective\n";
  const auto row = [&](const std::string& label, const optim::Lsr1SolveOptions& options) {
    const auto r = optim::l_sr1_trust_region(chain, vec(start), options);
    std::cout << std::left << std::setw(22) << label << std::setw(15) << name_of(r) << std::setw(8) << r.iteration << std::setw(8) << r.function_evaluations
              << std::scientific << std::setprecision(1) << r.objective_value << "\n";
  };
  for (const std::int64_t memory : {1, 3, 10, 30}) {
    optim::Lsr1SolveOptions options;
    options.max_iterations = 1000;
    options.step.memory_size = memory;
    row("memory " + std::to_string(memory), options);
  }
  for (const double radius : {0.01, 1.0, 100.0}) {
    optim::Lsr1SolveOptions options;
    options.max_iterations = 1000;
    options.step.initial_radius = radius;
    std::ostringstream label;
    label << "initial radius " << radius;
    row(label.str(), options);
  }

  std::cout << "\nthe first twelve iterations\n";
  optim::Lsr1SolveOptions options;
  options.max_iterations = 12;
  options.progress_hook = [](const optim::Lsr1ProgressSnapshot& s) {
    std::cout << std::right << std::setw(4) << s.iteration << std::scientific << std::setprecision(3) << "  objective " << s.objective << "  gradient " << s.gradient_norm
              << std::fixed << std::setprecision(3) << "  radius " << s.trust_radius << "  step " << s.step_norm << "\n";
    return true;
  };
  optim::l_sr1_trust_region(chain, vec(start), options);
  return 0;
}
```

Output

```text
setting               exit           iters   evals   objective
memory 1              MaxIterations  1000    1256    1.9e+00
memory 3              MaxIterations  1000    1529    1.4e-01
memory 10             Converged      119     286     9.2e-20
memory 30             Converged      119     286     9.2e-20
initial radius 0.01   NoProgress     385     877     6.3e-12
initial radius 1      Converged      119     286     9.2e-20
initial radius 100    Converged      175     551     5.6e-18

the first twelve iterations
   1  objective 7.074e+02  gradient 8.151e+02  radius 1.000  step 1.000
   2  objective 3.064e+02  gradient 3.890e+02  radius 1.000  step 0.694
   3  objective 1.240e+02  gradient 1.864e+02  radius 1.000  step 0.691
   4  objective 1.046e+02  gradient 2.747e+02  radius 1.000  step 1.000
   5  objective 5.346e+01  gradient 1.432e+02  radius 1.000  step 0.280
   6  objective 5.290e+01  gradient 9.880e+01  radius 0.250  step 0.500
   7  objective 3.624e+01  gradient 7.876e+01  radius 0.500  step 0.250
   8  objective 1.508e+01  gradient 5.381e+01  radius 0.500  step 0.500
   9  objective 1.274e+01  gradient 4.060e+01  radius 0.500  step 0.500
  10  objective 1.034e+01  gradient 2.599e+01  radius 0.500  step 0.323
  11  objective 9.103e+00  gradient 1.452e+01  radius 0.500  step 0.125
  12  objective 8.484e+00  gradient 5.051e+00  radius 0.500  step 0.080
```
The memory matters more than for L-BFGS. With a memory of 1 or 3 the model is too poor to follow the valley: neither converges in 1000 iterations, ending at 1.9 and 0.14. With 10 it converges in 119 iterations. A memory larger than the number of variables is capped at the number of variables (more than n step vectors in n dimensions are linearly dependent and add nothing), so 30 gives exactly the same run as 10 on this ten-variable problem; the default of 10 is a sensible value, and more never helps once it reaches n. The initial radius is adapted by the method and matters little: 1 (the default) converges in 119 iterations, 100 in 175, and 0.01, which starts the method taking tiny steps, ends with `NoProgress` after 385 iterations at 6.3e-12.

The trace shows the trust region at work. The radius sits at 1 while the model predicts well; at iteration 6 the decrease in the objective is much smaller than predicted, the radius drops to 0.25 and then recovers to 0.5, and the steps follow it: lengths of 0.5, 0.25, 0.5. The objective falls from 707 to 8.5 in twelve iterations. A radius that keeps shrinking would mean the model is poor, which usually means the memory is too small.
