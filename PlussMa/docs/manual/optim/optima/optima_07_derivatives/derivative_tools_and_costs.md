<!-- generated from derivative_tools_and_costs.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Derivative tools, call counts and long loops

[Back to Optima 7: Derivatives](../optima_07_derivatives.md)

The machinery that the algorithms use is available directly: `grad` and `value_and_grad` for a scalar function, `jacrev` and `jacfwd` for the Jacobian of a vector function, `jvp` for a derivative along a direction, and `hvp` for the exact Hessian times a direction without forming the Hessian. They work on the same parameter packs as the solver, so they are useful for sensitivity analysis and for checking your own work.

The page then asks what each way of supplying derivatives *costs*. Counting the calls into your own functions shows what an exact Hessian adds on top of a BFGS run, and a long recurrence (a state-space filter, a volatility model) shows when the tape becomes the wrong tool: it records every operation and grows with the loop, where `fwd_objective` carries a fixed number of partial derivatives and allocates nothing.

## Program 4: Gradients, Jacobians and products on their own

The machinery that the algorithms use is available directly on the same parameter packs: `grad` and `value_and_grad` for a scalar function, `jacrev` and `jacfwd` for the Jacobian of a vector function, `jvp` for a derivative along a direction, and `hvp` for the exact Hessian times a direction without ever forming the Hessian. Every function is written once, on plain `Series`. The program evaluates each at a point and compares it with the formula worked out by hand.

```cpp
#include <pluss/ma.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>

namespace optim = pluss::subsp::math::optim;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> pair(double x, double y) { return optim::pack_scalars({{"x", scalar(x)}, {"y", scalar(y)}}); }
double at(const optim::ParameterPack<pluss::Series>& p, const char* name) { return as_f64(p.at(name)); }

// The two functions, written once on plain Series. Every derivative tool below accepts them as they are.
pluss::Series f_of(const optim::ParameterPack<pluss::Series>& p) {
  return p.at("x").square() * p.at("y") + p.at("y").sin();
}
pluss::Series big_f_of(const optim::ParameterPack<pluss::Series>& p) {
  return (p.at("x") * p.at("y")).reshape({1}).unite({(p.at("x") + p.at("y").square()).reshape({1})}, 0);
}

int main() {
  const double x = 1.5, y = 0.7;
  const auto point = pair(x, y);
  std::cout << std::fixed << std::setprecision(6);
  double worst = 0.0;
  const auto check = [&](const char* label, std::initializer_list<std::pair<double, double>> pairs) {
    double err = 0.0;
    for (const auto& [got, want] : pairs) err = std::max(err, std::abs(got - want));
    worst = std::max(worst, err);
    std::cout << std::left << std::setw(28) << label << "max error " << std::scientific << std::setprecision(1) << err << std::fixed << std::setprecision(6) << "\n";
  };

  const auto [value, g] = optim::value_and_grad(f_of, point);
  std::cout << "f = " << value.item<double>() << ", df/dx = " << at(g, "x") << ", df/dy = " << at(g, "y") << "\n";
  check("value_and_grad", {{value.item<double>(), x * x * y + std::sin(y)}, {at(g, "x"), 2 * x * y}, {at(g, "y"), x * x + std::cos(y)}});

  const auto [fv, jr] = optim::jacrev(big_f_of, point);
  check("jacrev", {{as_f64(jr.at("x").select(0, 0)), y}, {as_f64(jr.at("x").select(0, 1)), 1.0}, {as_f64(jr.at("y").select(0, 0)), x}, {as_f64(jr.at("y").select(0, 1)), 2 * y}});
  const auto [fv2, jf] = optim::jacfwd(big_f_of, point);
  check("jacfwd", {{as_f64(jf.at("x").select(0, 0)), y}, {as_f64(jf.at("x").select(0, 1)), 1.0}, {as_f64(jf.at("y").select(0, 0)), x}, {as_f64(jf.at("y").select(0, 1)), 2 * y}});

  // Directional derivative of f along (1, -2): df/dx - 2 df/dy.
  const auto [fj, directional] = optim::jvp(f_of, point, pair(1.0, -2.0));
  check("jvp along (1, -2)", {{as_f64(directional), 2 * x * y - 2 * (x * x + std::cos(y))}});

  // Hessian of f times the direction (1, -2): [f_xx f_xy; f_xy f_yy] (1, -2) with f_xx = 2y, f_xy = 2x, f_yy = -sin y.
  const auto [fh, hv] = optim::hvp(f_of, point, pair(1.0, -2.0));
  check("hvp along (1, -2)", {{at(hv, "x"), 2 * y * 1.0 + 2 * x * -2.0}, {at(hv, "y"), 2 * x * 1.0 - std::sin(y) * -2.0}});

  std::cout << "\nall five agree with the hand-worked formulas to better than 1e-12: " << std::boolalpha << (worst < 1e-12) << "\n";
  return 0;
}
```

Output

```text
f = 2.219218, df/dx = 2.100000, df/dy = 3.014842
value_and_grad              max error 0.0e+00
jacrev                      max error 0.0e+00
jacfwd                      max error 0.0e+00
jvp along (1, -2)           max error 0.0e+00
hvp along (1, -2)           max error 0.0e+00

all five agree with the hand-worked formulas to better than 1e-12: true
```
All five agree with the formulas to better than 1e-12. `jacrev` and `jacfwd` return the same Jacobian: it is built with one pass per output, which suits functions with few outputs and many inputs, such as a loss. `hvp` differentiates the reverse-mode gradient a second time, so it is exact and needs no second form of the function: one product gives one column of the Hessian, so the Hessian of a function of `n` variables costs `n` products. The Jacobian of a vector function comes back as one block per parameter, each of shape `[outputs, block size]`, which is the shape the analytic constraints of Program 3 return.

## Program 5: Counting the calls

Each way of supplying derivatives costs a different number of calls into your own functions. The four-variable benchmark is solved by sequential QP with the BFGS model and a traced gradient, with the BFGS model and a hand-written gradient, and with the exact Hessian. Every call into the objective callbacks is counted.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::subsp::math::optim;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> four(double a, double b, double c, double d) {
  return optim::pack_scalars({{"a", scalar(a)}, {"b", scalar(b)}, {"c", scalar(c)}, {"d", scalar(d)}});
}

pluss::Series objective_of(const optim::ParameterPack<pluss::Series>& p) {
  return p.at("a") * p.at("d") * (p.at("a") + p.at("b") + p.at("c")) + p.at("c");
}
pluss::Series inequality_of(const optim::ParameterPack<pluss::Series>& p) {
  return 25.0 - p.at("a") * p.at("b") * p.at("c") * p.at("d");
}
pluss::Series equality_of(const optim::ParameterPack<pluss::Series>& p) {
  return p.at("a").square() + p.at("b").square() + p.at("c").square() + p.at("d").square() - 40.0;
}

struct Counts { int traced = 0, analytic = 0; };

optim::ConstrainedProblem problem(Counts& counts, bool analytic_gradient) {
  optim::ConstrainedProblem prob;
  if (analytic_gradient) {
    prob.objective.analytic = [&counts](const optim::ParameterPack<pluss::Series>& p) {
      ++counts.analytic;
      const double a = as_f64(p.at("a")), b = as_f64(p.at("b")), c = as_f64(p.at("c")), d = as_f64(p.at("d"));
      return std::pair{scalar(a * d * (a + b + c) + c), four(d * (2 * a + b + c), a * d, a * d + 1.0, a * (a + b + c))};
    };
  } else {
    prob.objective.autodiff = [&counts](const optim::ParameterPack<pluss::Series>& p) { ++counts.traced; return objective_of(p); };
  }
  prob.nonlinear.inequality = [](const optim::ParameterPack<pluss::Series>& p) { return inequality_of(p); };
  prob.nonlinear.equality = [](const optim::ParameterPack<pluss::Series>& p) { return equality_of(p); };
  prob.bounds.lower = four(1, 1, 1, 1);
  prob.bounds.upper = four(5, 5, 5, 5);
  return prob;
}

int main() {
  struct Row { const char* label; bool analytic; optim::OptimaHessianApproximation model; };
  const Row rows[] = {{"traced gradient, BFGS", false, optim::OptimaHessianApproximation::Bfgs},
                      {"analytic gradient, BFGS", true, optim::OptimaHessianApproximation::Bfgs},
                      {"traced gradient, exact Hessian", false, optim::OptimaHessianApproximation::Exact}};
  std::cout << std::left << std::setw(34) << "setup" << std::setw(12) << "iterations" << std::setw(10) << "traced" << std::setw(10) << "analytic" << "objective\n";
  for (const auto& row : rows) {
    Counts counts;
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::SequentialQP;
    o.sequential_qp.hessian_approximation = row.model;
    const auto r = optim::optima(problem(counts, row.analytic), four(1, 5, 5, 1), o);
    std::cout << std::left << std::setw(34) << row.label << std::setw(12) << r.iteration << std::setw(10) << counts.traced
              << std::setw(10) << counts.analytic << std::fixed << std::setprecision(7) << r.evaluation.value.item<double>() << "\n";
  }
  return 0;
}
```

Output

```text
setup                             iterations  traced    analytic  objective
traced gradient, BFGS             4           6         0         17.0140172
analytic gradient, BFGS           4           0         6         17.0140172
traced gradient, exact Hessian    5           31        0         17.0140173
```
With BFGS the run takes 4 iterations and 6 calls to the objective, whether the gradient is traced or written by hand; the hand-written one appears in the `analytic` column instead, with the same count and the same answer. The exact model reaches the same optimum in 5 iterations and 31 traced calls: 7 for the gradients plus 24 for the Hessian-vector products. That is 6 Hessians of 4 variables each: a Hessian is assembled from one Hessian-vector product per variable, and each one runs your function again. The extra calls grow with the problem as `n` per iteration, which is the cost of exactness (chapter 6); a function whose evaluation is cheap, or whose curvature is learned slowly by the quasi-Newton models, can repay it. A function whose evaluation is expensive is better served by BFGS or the limited-memory model.

## Program 6: Long loops without a tape

Some objectives are a loop of a few scalar operations repeated thousands of times: a state-space filter, a volatility model, a recursion over a time series. Reverse mode records every operation of the loop on a tape, so the tape grows with the length of the loop. `Fwd<N>` carries the value and the `N` partial derivatives in plain doubles instead: no tape and no allocation. The program fits a GARCH(1,1) model, three parameters, to 2000 simulated returns, with the log-likelihood written once as a template and used with both number types.

```cpp
#include <pluss/ma.h>

#include <array>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

namespace optim = pluss::subsp::math::optim;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> params3(double omega, double alpha, double beta) {
  return optim::pack_scalars({{"omega", scalar(omega)}, {"alpha", scalar(alpha)}, {"beta", scalar(beta)}});
}

// Simulated returns from a GARCH(1,1) with omega = 0.05, alpha = 0.1, beta = 0.85, from a fixed generator.
std::vector<double> simulate(int n) {
  std::uint64_t state = 12345;
  const auto uniform = [&] { state = state * 6364136223846793005ULL + 1442695040888963407ULL; return ((state >> 11) + 0.5) / 9007199254740992.0; };
  std::vector<double> r(static_cast<std::size_t>(n));
  double variance = 0.05 / (1.0 - 0.1 - 0.85);
  for (int t = 0; t < n; ++t) {
    const double z = std::sqrt(-2.0 * std::log(uniform())) * std::cos(2.0 * 3.14159265358979323846 * uniform());
    r[static_cast<std::size_t>(t)] = std::sqrt(variance) * z;
    variance = 0.05 + 0.1 * r[static_cast<std::size_t>(t)] * r[static_cast<std::size_t>(t)] + 0.85 * variance;
  }
  return r;
}

// The same text runs on a Series and on a forward-mode number; a Series takes its logarithm as a method.
inline pluss::Series log(const pluss::Series& x) { return x.log(); }

template <class Number>
Number negative_log_likelihood(const std::vector<double>& r, const Number& omega, const Number& alpha, const Number& beta) {
  Number variance = omega / (1.0 - alpha - beta);
  Number total = variance * 0.0;
  for (double x : r) {
    total = total + 0.5 * (log(variance) + (x * x) / variance);
    variance = omega + alpha * (x * x) + beta * variance;
  }
  return total / static_cast<double>(r.size());
}

int main() {
  const auto returns = simulate(2000);
  const auto lower = params3(1e-4, 1e-3, 1e-3), upper = params3(1.0, 0.5, 0.995);
  const auto start = params3(0.1, 0.05, 0.8);

  optim::ConstrainedProblem traced;
  traced.objective.autodiff = [&](const optim::ParameterPack<pluss::Series>& p) -> pluss::Series {
    return negative_log_likelihood<pluss::Series>(returns, p.at("omega"), p.at("alpha"), p.at("beta"));
  };
  traced.nonlinear.inequality = [](const optim::ParameterPack<pluss::Series>& p) -> pluss::Series { return p.at("alpha") + p.at("beta") - 0.999; };
  traced.bounds.lower = lower;
  traced.bounds.upper = upper;

  using F = optim::Fwd<3>;
  optim::ConstrainedProblem forward = traced;
  forward.objective.autodiff = nullptr;
  forward.objective.analytic = optim::fwd_objective<3>([&](const std::array<F, 3>& p) -> F {
    return negative_log_likelihood<F>(returns, p[0], p[1], p[2]);
  });

  optim::OptimaState results[2];
  const optim::ConstrainedProblem* problems[2] = {&traced, &forward};
  for (int i = 0; i < 2; ++i) {
    results[i] = optim::optima(*problems[i], start);
  }

  const char* labels[2] = {"reverse mode (pluss::Series)", "forward mode (Fwd<3>)"};
  std::cout << std::left << std::setw(24) << "mode" << std::setw(8) << "exit" << std::setw(12) << "iterations" << std::setw(12) << "objective"
            << "omega, alpha, beta\n";
  for (int i = 0; i < 2; ++i) {
    const auto& p = results[i].params;
    std::cout << std::left << std::setw(24) << labels[i] << std::setw(8) << static_cast<int>(results[i].exit_flag) << std::setw(12)
              << results[i].iteration << std::fixed << std::setprecision(6) << std::setw(12) << results[i].evaluation.value.item<double>()
              << std::setprecision(4) << as_f64(p.at("omega")) << ", " << as_f64(p.at("alpha")) << ", " << as_f64(p.at("beta")) << "\n";
  }
  return 0;
}
```

Output

```text
mode                    exit    iterations  objective   omega, alpha, beta
reverse mode (pluss::Series)1       11          0.433348    0.0340, 0.0807, 0.8835
forward mode (Fwd<3>)   1       11          0.433348    0.0340, 0.0807, 0.8835
```
Both modes give the same fit: 11 iterations, an objective of 0.433348, parameters (0.0340, 0.0807, 0.8835). The choice is therefore about how the derivative is computed, not about the answer. Forward mode does `N` derivative updates for every operation of the plain function, so it suits a small `N` (up to a few dozen); with hundreds of parameters, use `autodiff`, whose work does not grow with `N` in that way. Which is quicker for your function depends on its length, on `N` and on your machine, so measure both before committing.

## Decisions and pitfalls

- **One function, every derivative.** `hvp`, `jvp` and the exact curvature models differentiate the same `Series` function; there is no second form to write.
- **An exact Hessian costs extra calls.** It reaches the same optimum in a similar number of iterations but adds calls to your function (24 here for 6 Hessians of 4 variables); on a small problem BFGS is cheaper.
- **`fwd_objective` for long loops with few parameters.** It does `N` derivative updates per operation, so it suits `N` up to a few dozen; with hundreds of parameters use reverse mode.
- **The answer does not depend on the mode.** Both modes gave the same fit in the loop example; the choice is about memory and time.

Where next: [Chapter 8](../optima_08_worked_problems.md) puts everything together on five worked problems.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="automatic_analytic_and_audited_derivatives.html">
    <span class="label">Previous</span>
    <span class="title">Automatic, analytic and audited derivatives</span>
  </a>
</div>
<!-- apiref:nav:end -->
