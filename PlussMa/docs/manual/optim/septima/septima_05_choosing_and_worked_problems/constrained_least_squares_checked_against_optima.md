<!-- generated from constrained_least_squares_checked_against_optima.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Constrained least squares, checked against Optima

[Back to Choosing a Solver, and Worked Problems](../septima_05_choosing_and_worked_problems.md)


Choose non-negative weights that sum to 1 so that a weighted mix of five series matches a target. The data are built from the weights `(0.5, 0, 0.3, 0, 0.2)` plus a little noise, so the right answer has two weights at zero, which are active bounds. Optima solves it as a quadratic program. Septima uses its bound projection for the non-negativity and an equality residual for the sum.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::optim;
using optim::Trace;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;

int main() {
  const int rows = 40, columns = 5;
  std::uint64_t state = 99;
  const auto uniform = [&] { state = state * 6364136223846793005ULL + 1442695040888963407ULL; return ((state >> 11) + 0.5) / 9007199254740992.0; };
  const std::vector<double> truth = {0.5, 0.0, 0.3, 0.0, 0.2};
  std::vector<double> a, t;
  for (int i = 0; i < rows; ++i) {
    double mix = 0.0;
    for (int j = 0; j < columns; ++j) {
      const double v = uniform() - 0.5;
      a.push_back(v);
      mix += truth[static_cast<std::size_t>(j)] * v;
    }
    t.push_back(mix + 0.01 * (uniform() - 0.5));
  }
  const auto A = pluss::make_series(a, {rows, columns}).to(f64);
  const auto target = pluss::make_series(t, {rows}).to(f64);
  const auto objective = [&](const Trace& w) -> Trace {
    const Trace mix = optim::matmul(optim::fixed(A), pluss::subsp::math::diff::reshape(w, {columns, 1}));
    return optim::sum(optim::square(mix - optim::fixed(target.reshape({rows, 1}))));
  };
  const auto start = pluss::make_series(std::vector<double>(columns, 0.2), {columns}).to(f64);

  std::cout << std::left << std::setw(18) << "solver" << std::setw(8) << "evals" << std::setw(12) << "objective" << std::setw(10) << "sum(w)" << "weights\n";
  const auto row = [&](const char* label, std::int64_t evals, double value, const Series& w) {
    const auto v = w.to_vec<double>();
    double sum = 0.0;
    for (double x : v) sum += x;
    std::cout << std::left << std::setw(18) << label << std::setw(8) << evals << std::fixed << std::setprecision(5) << std::setw(12) << value << std::setw(10) << sum;
    for (double x : v) std::cout << std::setprecision(4) << x << "  ";
    std::cout << "\n";
  };

  // Optima: bounds and one linear equality.
  const auto o = optim::optima(objective, start, {},
                               optim::constraints()
                                   .lbs(pluss::make_series(std::vector<double>(columns, 0.0), {columns}).to(f64))
                                   .ln_eq(pluss::make_series(std::vector<double>(columns, 1.0), {1, columns}).to(f64), pluss::make_series(std::vector<double>{1.0}, {1}).to(f64)));
  row("Optima", o.function_evaluations, o.evaluation.value.item<double>(), o.params.at("x"));

  // Septima: bounds by projection, the sum by an equality residual.
  optim::SeptimaConstraints constraints;
  constraints.lbs = {pluss::make_series(std::vector<double>(columns, 0.0), {columns}).to(f64)};
  constraints.equalities = [](const std::vector<Series>& x) { return std::vector<Series>{x[0].sum() - 1.0}; };
  optim::SeptimaSolveOptions options;
  options.max_iterations = 4000;
  options.gradient_tolerance = 1e-5;
  options.step_tolerance = 1e-6;
  options.step.adam.learning_rate = 0.01;  // the constraint schedule is the library default
  const auto s = optim::septima(objective, start, options, constraints);
  row("Septima, Adam", s.function_evaluations, s.objective_value, s.params[0]);
  std::cout << "\nSeptima's equality violation: " << std::scientific << std::setprecision(1) << s.constraint_violation << ", exit "
            << (s.converged ? "Converged" : "not converged") << " after " << s.iteration << " iterations\n";
  return 0;
}
```

Output

```text
solver            evals   objective   sum(w)    weights
Optima            15      0.00031     1.00000   0.5017  0.0011  0.2985  0.0000  0.1987  
Septima, Adam     319     0.00031     1.00000   0.5017  0.0011  0.2985  0.0000  0.1987  

Septima's equality violation: 6.8e-08, exit Converged after 319 iterations
```
The two solvers agree: the weights are `0.5017, 0.0011, 0.2985, 0.0000, 0.1987` in both, they sum to 1.00000, and the objective is 0.00031. Septima reports `Converged` after 319 iterations with an equality violation of 6.8e-08, using the default constraint schedule. Optima needs 15 evaluations to Septima's 319.

This is a problem where Septima's constraint handling is at its best: the objective is convex, the equality is linear, and the rest is bounds. Chapter 2, Program 4, is the harder case, with nonlinear equalities and inequalities and a non-convex problem; Septima reaches the same solution as Optima there too, in 242 evaluations with L-BFGS where Optima needs 6.
