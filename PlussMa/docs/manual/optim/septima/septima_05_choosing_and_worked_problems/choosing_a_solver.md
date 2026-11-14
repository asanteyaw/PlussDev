<!-- generated from choosing_a_solver.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Choosing a solver on worked problems

[Back to Septima 5: Choosing a Solver, and Worked Problems](../septima_05_choosing_and_worked_problems.md)

Which solver should you use? This page answers with three problems, each chosen because it favours a different answer. A **model fitted to data** (logistic regression with a small penalty) is convex and smooth, so every optimiser must find the same answer; the comparison is cost. **Constrained least squares** (non-negative weights that sum to one, matching a target) is a quadratic program, which Optima solves exactly as the reference; here the question is how close Septima's bound projection and augmented Lagrangian get. And the **chained Rosenbrock function** with no constraints is solved by Optima and all three first-order methods, to read off what each is best at.

The conclusion of the third table is the practical rule: fewest evaluations, Optima; simplest and cheapest per iteration, L-BFGS; saddle-prone objectives, L-SR1; Adam when nothing else works or when the gradient is noisy.

## Program 1: A model fitted to data

Two hundred points in the plane are labelled by which side of a line they fall on, with one label in ten flipped. The model is logistic regression with a small L2 penalty. The data are a `Series` held fixed inside the objective; the parameters are a weight column and a bias; the loss is a few lines of `Series` arithmetic.

```cpp
#include <pluss/ma.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>

namespace optim = pluss::subsp::math::optim;
using pluss::Series;
using Pack = optim::ParameterPack<pluss::Series>;
using Block = optim::ParameterPack<pluss::Series>::Block;

const auto f64 = pluss::ScalarType::Float64;

int main() {
  const int n = 200;
  std::uint64_t state = 2024;
  const auto uniform = [&] { state = state * 6364136223846793005ULL + 1442695040888963407ULL; return ((state >> 11) + 0.5) / 9007199254740992.0; };
  std::vector<double> features, labels;
  for (int i = 0; i < n; ++i) {
    const double x0 = 4.0 * uniform() - 2.0, x1 = 4.0 * uniform() - 2.0;
    double label = 1.5 * x0 - x1 + 0.3 > 0.0 ? 1.0 : -1.0;
    if (uniform() < 0.10) label = -label;
    features.push_back(x0);
    features.push_back(x1);
    labels.push_back(label);
  }
  const auto X = pluss::make_series(features, {n, 2}).to(f64);
  const auto y = pluss::make_series(labels, {n, 1}).to(f64);

  const auto loss_of = [&](const pluss::Series& w, const pluss::Series& b) -> pluss::Series {
    const pluss::Series z = X.matmul(w) + b;  // [n, 1]
    const pluss::Series margin = y * z;
    return (1.0 + (margin * -1.0).exp()).log().sum() * (1.0 / n) + 0.05 * w.square().sum();
  };
  const auto zeros_w = pluss::Series::zeros({2, 1}, pluss::ScalarType::Float64);
  const auto zeros_b = pluss::Series::zeros({1}, pluss::ScalarType::Float64);

  std::vector<std::pair<std::string, std::vector<double>>> fits;
  std::cout << std::left << std::setw(10) << "optimizer" << std::setw(14) << "exit" << std::setw(8) << "iters" << std::setw(8) << "evals" << std::setw(12) << "loss"
            << "weights, bias\n";
  const auto report = [&](const std::string& name, const char* exit, std::int64_t iterations, std::int64_t evaluations, double loss,
                          const pluss::Series& w, const pluss::Series& b) {
    const auto wv = w.reshape({2}).host<double>();
    const double bias = as_f64(b);
    fits.push_back({name, {wv[0], wv[1], bias}});
    std::cout << std::left << std::setw(10) << name << std::setw(14) << exit << std::setw(8) << iterations << std::setw(8) << evaluations << std::fixed
              << std::setprecision(6) << std::setw(12) << loss << std::setprecision(4) << wv[0] << ", " << wv[1] << ", " << bias << "\n";
  };

  std::function<pluss::Series(const Pack&)> named = [&](const Pack& p) { return loss_of(p.at("w"), p.at("b")); };
  const optim::ParameterPack<pluss::Series> start(std::vector<Block>{{"w", zeros_w}, {"b", zeros_b}});
  {
    optim::SeptimaSolveOptions options;
    options.max_iterations = 3000;
    options.step.adam.learning_rate = 0.1;
    options.gradient_tolerance = 1e-5;
    options.step_tolerance = 1e-5;
    const auto r = optim::septima(named, start, options);
    report("Adam", r.converged ? "Converged" : "MaxIterations", r.iteration, r.function_evaluations, r.objective_value, r.params[0], r.params[1]);
  }
  {
    optim::SeptimaSolveOptions options;
    options.max_iterations = 500;
    options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
    const auto r = optim::septima(named, start, options);
    report("L-BFGS", r.converged ? "Converged" : "NoProgress", r.iteration, r.function_evaluations, r.objective_value, r.params[0], r.params[1]);
  }
  {
    const auto [params, r] = optim::l_sr1_trust_region(
        [&](const std::vector<pluss::Series>& blocks) -> pluss::Series { return loss_of(blocks[0], blocks[1]); }, std::vector<pluss::Series>{zeros_w, zeros_b});
    report("L-SR1", r.converged ? "Converged" : "not converged", r.iteration, r.function_evaluations, r.objective_value, params[0], params[1]);
  }

  double spread = 0.0;
  for (std::size_t k = 0; k < 3; ++k)
    for (const auto& fit : fits) spread = std::max(spread, std::abs(fit.second[k] - fits.back().second[k]));
  const auto& best = fits.back().second;
  int correct = 0;
  for (int i = 0; i < n; ++i) {
    const double z = features[2 * i] * best[0] + features[2 * i + 1] * best[1] + best[2];
    if ((z > 0.0 ? 1.0 : -1.0) == labels[static_cast<std::size_t>(i)]) ++correct;
  }
  std::cout << std::scientific << std::setprecision(1) << "\nlargest difference between the three answers: " << spread << "\n"
            << std::fixed << std::setprecision(3) << "fraction classified correctly: " << static_cast<double>(correct) / n << "\n";
  return 0;
}
```

Output

```text
optimizer exit          iters   evals   loss        weights, bias
Adam      Converged     190     190     0.557757    0.7150, -0.4648, -0.0354
L-BFGS    Converged     11      11      0.557757    0.7150, -0.4648, -0.0354
L-SR1     Converged     8       9       0.557757    0.7150, -0.4648, -0.0354

largest difference between the three answers: 2.3e-05
fraction classified correctly: 0.825
```
The problem is convex, so every optimizer must find the same answer, and they do: the three fits agree to 2.3e-05, with a loss of 0.557757, weights `(0.7150, -0.4648)` and a bias of -0.0354. The weights are in the ratio 1.54 to -1, close to the 1.5 to -1 of the line that generated the labels, and the model classifies 82.5% of the points correctly, which is about as good as it can do when one label in ten is wrong.

The cost is where they differ. L-SR1 converges in 8 iterations and 9 evaluations, L-BFGS in 11 iterations and 11 evaluations, Adam in 190. For a smooth model fitted to a modest amount of data, with all the data in the objective at once, a curvature method is the right choice. Adam's strength is elsewhere: it needs only a gradient and tolerates a gradient that is noisy, which a curvature method does not.

## Program 2: Constrained least squares, checked against Optima

Choose non-negative weights that sum to 1 so that a weighted mix of five series matches a target. The data are built from the weights `(0.5, 0, 0.3, 0, 0.2)` plus a little noise, so the right answer has two weights at zero, which are active bounds. Optima solves it as a quadratic program. Septima uses its bound projection for the non-negativity and an equality residual for the sum.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::subsp::math::optim;
using pluss::Series;
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
  const auto objective = [&](const pluss::Series& w) -> pluss::Series {
    const pluss::Series mix = A.matmul(w.reshape({columns, 1}));
    return (mix - target.reshape({rows, 1})).square().sum();
  };
  const auto start = pluss::make_series(std::vector<double>(columns, 0.2), {columns}).to(f64);

  std::cout << std::left << std::setw(18) << "solver" << std::setw(8) << "evals" << std::setw(12) << "objective" << std::setw(10) << "sum(w)" << "weights\n";
  const auto row = [&](const char* label, std::int64_t evals, double value, const Series& w) {
    const auto v = w.host<double>();
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

## Program 3: One function, every solver

The ten-variable chained Rosenbrock function, with no constraints, solved by Optima and by the three first-order methods.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::subsp::math::optim;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  const int n = 10;
  std::vector<double> first, second, start;
  for (int i = 0; i + 1 < n; ++i) { first.push_back(i); second.push_back(i + 1); }
  for (int i = 0; i < n; ++i) start.push_back(i % 2 == 0 ? -1.2 : 1.0);
  const auto head = pluss::make_series(first, {n - 1}).to(pluss::ScalarType::Int64);
  const auto tail = pluss::make_series(second, {n - 1}).to(pluss::ScalarType::Int64);
  auto chain = [&](const pluss::Series& x) -> pluss::Series {
    const pluss::Series a = x.index_select(0, head), b = x.index_select(0, tail);
    return (100.0 * (b - a.square()).square() + (1.0 - a).square()).sum();
  };

  std::cout << std::left << std::setw(16) << "solver" << std::setw(16) << "exit" << std::setw(8) << "iters" << std::setw(8) << "evals" << std::setw(12) << "objective"
            << "gradient\n";
  const auto row = [](const char* label, const char* exit, std::int64_t iterations, std::int64_t evaluations, double value, double gradient) {
    std::cout << std::left << std::setw(16) << label << std::setw(16) << exit << std::setw(8) << iterations << std::setw(8) << evaluations << std::scientific
              << std::setprecision(1) << std::setw(12) << value << gradient << "\n";
  };

  {
    const auto r = optim::optima(chain, vec(start));
    row("Optima", r.exit_flag == optim::OptimaExitFlag::Converged ? "converged" : "other", r.iteration, r.function_evaluations, r.evaluation.value.item<double>(),
        r.output.first_order_optimality);
  }
  {
    optim::SeptimaSolveOptions options;
    options.max_iterations = 5000;
    options.step.adam.learning_rate = 0.05;
    const auto r = optim::septima(chain, vec(start), options);
    row("Septima, Adam", r.converged ? "converged" : "MaxIterations", r.iteration, r.function_evaluations, r.objective_value, r.gradient_inf_norm);
  }
  {
    optim::SeptimaSolveOptions options;
    options.max_iterations = 2000;
    options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
    const auto r = optim::septima(chain, vec(start), options);
    row("Septima, L-BFGS", r.converged ? "converged" : "NoProgress", r.iteration, r.function_evaluations, r.objective_value, r.gradient_inf_norm);
  }
  {
    const auto r = optim::l_sr1_trust_region(chain, vec(start));
    row("L-SR1", r.converged ? "converged" : "not converged", r.iteration, r.function_evaluations, r.objective_value, r.gradient_norm);
  }
  return 0;
}
```

Output

```text
solver          exit            iters   evals   objective   gradient
Optima          converged       74      83      1.2e-10     3.8e-07
Septima, Adam   converged       4388    4388    2.1e-17     3.6e-09
Septima, L-BFGS converged       108     108     5.3e-20     6.1e-09
L-SR1           converged       115     262     2.0e-20     6.8e-09
```
Optima converges in 74 iterations and 83 evaluations to an objective of 1.2e-10. L-SR1 converges in 115 iterations and 262 evaluations to 2.0e-20 with a gradient of 6.8e-09. L-BFGS converges in 108 iterations and 108 evaluations to 5.3e-20 and 6.1e-09, the cheapest of the accurate runs. Adam converges after 4388 iterations to 2.1e-17.

Read the table by what you need. Fewest evaluations: Optima. Most accurate per evaluation: L-BFGS. Saddle-prone objective: L-SR1. Adam: when nothing else works or when the gradient is noisy.

## Decisions and pitfalls

- **Use a curvature method for a smooth model on all the data at once.** L-SR1 needed 8 iterations and L-BFGS 11 where Adam needed 190 on the logistic regression.
- **Adam's strength is noisy gradients and huge parameter sets.** When the loss is computed from mini-batches or the problem is badly scaled, it is the right tool; for a smooth deterministic fit it is not.
- **Check a constrained fit against a reference.** On a quadratic program, Optima gives the exact answer in 15 evaluations; Septima agrees (weights to four digits) in 319 evaluations with a violation of 7e-8.
- **Septima is at its best when the objective is convex, the equality is linear and the rest is bounds.** For a few nonlinear constraints that must hold to high precision, use Optima.
- **Compare solutions, not only flags.** Several solvers can report convergence at different accuracy; the objective and the gradient at the answer are the real comparison.

Where next: That completes the Septima chapters. The linear-programming chapters begin with [First programs](../../linprog.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
