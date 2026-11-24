# Fitting a model to data

[Back to Methods 2: Fitting and Identifiability](../meth_02_fitting.md)

You have measurements and a model with a few unknown parameters, and you want the parameter values that fit best, with an honest statement of how well the data determine them. This page fits the Michaelis-Menten law to measured initial rates of an enzyme, reports the residuals and the confidence intervals of the parameters, and fits several starting points and data sets in one call.

## How it works

For a model {math}`\hat y(\beta)` with parameter vector {math}`\beta` and data {math}`y`, nonlinear least squares minimises the sum of squares {math}`S(\beta) = \sum_m (y_m - \hat y_m(\beta))^2`. The fit uses the Levenberg-Marquardt method: with the Jacobian {math}`J = \partial\hat y/\partial\beta` and the residuals {math}`r = y - \hat y`, each step solves

```{math}
(J^{\top}J + \lambda\,\operatorname{diag}(J^{\top}J))\,\Delta = J^{\top}r
```

and the damping {math}`\lambda` is divided by ten after a step that lowers {math}`S` and multiplied by ten after one that does not. The fit stops when the step or the change of {math}`S` is below a tolerance. At the solution, the covariance of the estimates is {math}`s^2 (J^{\top}J)^{-1}` with {math}`s^2 = S/(n - p)`, and the confidence interval of parameter {math}`j` is {math}`\hat\beta_j \pm t_{1-\alpha/2,\,n-p}\,s\sqrt{[(J^{\top}J)^{-1}]_{jj}}`.

The code is in `pluss::ma::natsci::common::estimation` (`estimation::` below). `estimation::levenberg_marquardt(model, y, beta0, options)` takes a model that maps parameters `[lanes, P]` to predictions `[lanes, M]`, data `y` of shape `[M]` or `[lanes, M]`, and starting values `beta0` of shape `[lanes, P]`. Each row of `beta0` is an independent fit, all advancing in the same operations. It returns a `Fit` with `beta`, `sse`, `residuals`, `jacobian`, `iterations` and `converged`. `estimation::confidence_intervals(fit, alpha)` returns `lower`, `upper` and `standard_error`, and `estimation::student_t_quantile(q, nu)` is the critical value used. The model must treat the leading axis as lanes and read each parameter as `beta.select(-1, i).unsqueeze(-1)` so it broadcasts against the observations. The programs fit the rates {math}`v = V_{\max} S/(K_m + S)` measured at six substrate concentrations.

## One fit

The program fits the six rates, starting from {math}`(V_{\max}, K_m) = (1, 1)`, and prints the estimates, the sum of squares, the six residuals and the iteration count.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace estimation = common::estimation;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// initial rates of an enzyme measured at six substrate concentrations (mM -> umol/min)
static pluss::Series substrate() { return vec({0.5, 1.0, 2.0, 4.0, 8.0, 16.0}); }
static pluss::Series rate() { return vec({0.3201, 0.4880, 0.8052, 1.1356, 1.4274, 1.6291}); }
// the model sees a batch of parameter vectors beta [lanes, 2] = (Vmax, Km) and returns the predicted rates [lanes, 6]
static pluss::Series michaelis_menten(const pluss::Series &beta) {
  const auto vmax = beta.select(-1, 0).unsqueeze(-1), km = beta.select(-1, 1).unsqueeze(-1);
  return vmax.mul(substrate()).div(km.add(substrate()));
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto fit = estimation::levenberg_marquardt(michaelis_menten, rate(), mat({{1.0, 1.0}}));
  out.writeln("(Vmax, Km)        = ", flat(fit.beta));
  out.writeln("sum of squares    = ", flat(fit.sse));
  out.writeln("residuals         = ", flat(fit.residuals));
  out.writeln("iterations        = ", flat(fit.iterations), ", converged = ", flat(fit.converged)[0] > 0.5 ? "yes" : "no");
}
```

Output

```text
(Vmax, Km)        = [1.9125, 2.7497]
sum of squares    = [0.0012]
residuals         = [0.0258, -0.0220, -0.0001, 0.0022, 0.0041, -0.0029]
iterations        = [4.0000], converged = yes
```

The estimates are {math}`V_{\max} = 1.9125` and {math}`K_m = 2.7497`, with a sum of squares of `0.0012`. The residuals are all below `0.03` and change sign (`0.0258, -0.0220, -0.0001, 0.0022, 0.0041, -0.0029`), with the largest at the lowest concentration. The fit took `4` iterations and `converged = yes`.

## Confidence intervals

The program turns the fit into 95 % and 99 % intervals, and prints the {math}`t` quantile used for four degrees of freedom (six readings, two parameters).

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace estimation = common::estimation;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// initial rates of an enzyme measured at six substrate concentrations (mM -> umol/min)
static pluss::Series substrate() { return vec({0.5, 1.0, 2.0, 4.0, 8.0, 16.0}); }
static pluss::Series rate() { return vec({0.3201, 0.4880, 0.8052, 1.1356, 1.4274, 1.6291}); }
// the model sees a batch of parameter vectors beta [lanes, 2] = (Vmax, Km) and returns the predicted rates [lanes, 6]
static pluss::Series michaelis_menten(const pluss::Series &beta) {
  const auto vmax = beta.select(-1, 0).unsqueeze(-1), km = beta.select(-1, 1).unsqueeze(-1);
  return vmax.mul(substrate()).div(km.add(substrate()));
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto fit = estimation::levenberg_marquardt(michaelis_menten, rate(), mat({{1.0, 1.0}}));
  const auto ci = estimation::confidence_intervals(fit);   // 95 % by default
  out.writeln("(Vmax, Km)  = ", flat(fit.beta));
  out.writeln("std. errors = ", flat(ci.standard_error));
  out.writeln("lower 95 %  = ", flat(ci.lower));
  out.writeln("upper 95 %  = ", flat(ci.upper));
  const auto wide = estimation::confidence_intervals(fit, 0.01);
  out.writeln("lower 99 %  = ", flat(wide.lower));
  out.writeln("upper 99 %  = ", flat(wide.upper));
  out.writeln("t quantile (0.975, 4 dof) = ", estimation::student_t_quantile(0.975, 4.0));
}
```

Output

```text
(Vmax, Km)  = [1.9125, 2.7497]
std. errors = [0.0249, 0.1041]
lower 95 %  = [1.8434, 2.4606]
upper 95 %  = [1.9815, 3.0388]
lower 99 %  = [1.7979, 2.2704]
upper 99 %  = [2.0270, 3.2290]
t quantile (0.975, 4 dof) = 2.7764
```

The standard errors are `0.0249` for {math}`V_{\max}` and `0.1041` for {math}`K_m`, so the 95 % intervals are [`1.8434`, `1.9815`] and [`2.4606`, `3.0388`]. The 99 % intervals are wider ([`1.7979`, `2.0270`] and [`2.2704`, `3.2290`]) because the critical value grows, from `2.7764` at the 95 % level.

## Several fits in one call

The program starts three fits from very different points, one row each of `beta0`, and then fits two data sets in one call by giving `y` a leading axis.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace estimation = common::estimation;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// initial rates of an enzyme measured at six substrate concentrations (mM -> umol/min)
static pluss::Series substrate() { return vec({0.5, 1.0, 2.0, 4.0, 8.0, 16.0}); }
static pluss::Series rate() { return vec({0.3201, 0.4880, 0.8052, 1.1356, 1.4274, 1.6291}); }
// the model sees a batch of parameter vectors beta [lanes, 2] = (Vmax, Km) and returns the predicted rates [lanes, 6]
static pluss::Series michaelis_menten(const pluss::Series &beta) {
  const auto vmax = beta.select(-1, 0).unsqueeze(-1), km = beta.select(-1, 1).unsqueeze(-1);
  return vmax.mul(substrate()).div(km.add(substrate()));
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // three starting points fitted in ONE call: each row of beta0 is a lane
  const auto fit = estimation::levenberg_marquardt(michaelis_menten, rate(), mat({{1.0, 1.0}, {10.0, 0.1}, {0.5, 20.0}}));
  out.writeln("beta (Vmax, Km per lane) = ", flat(fit.beta));
  out.writeln("sum of squares per lane  = ", flat(fit.sse));
  out.writeln("iterations per lane      = ", flat(fit.iterations));
  out.writeln("converged per lane       = ", flat(fit.converged));
  // two data sets against one start: y may be [lanes, observations]
  const auto second = vec({0.30, 0.50, 0.80, 1.15, 1.40, 1.65});
  const auto both = pluss::series::make(std::vector<double>{0.3201, 0.4880, 0.8052, 1.1356, 1.4274, 1.6291, 0.30, 0.50, 0.80, 1.15, 1.40, 1.65}, {2, 6}).to(pluss::ScalarType::Float64);
  const auto pair = estimation::levenberg_marquardt(michaelis_menten, both, mat({{1.0, 1.0}, {1.0, 1.0}}));
  out.writeln("two data sets, beta      = ", flat(pair.beta));
  (void)second;
}
```

Output

```text
beta (Vmax, Km per lane) = [1.9125, 2.7497, 1.9125, 2.7497, 1.9125, 2.7497]
sum of squares per lane  = [0.0012, 0.0012, 0.0012]
iterations per lane      = [4.0000, 8.0000, 10.0000]
converged per lane       = [1.0000, 1.0000, 1.0000]
two data sets, beta      = [1.9125, 2.7497, 1.9242, 2.7966]
```

All three starting points reach the same minimum, {math}`(1.9125, 2.7497)`, with the same sum of squares `0.0012`, after `4`, `8` and `10` iterations, and all three report `converged = 1`. The second data set (the same readings rounded to two decimals) gives its own estimates, `(1.9242, 2.7966)`, in the same call: the lanes do not interact.

## Decisions

- Make the model batched: parameters `[lanes, P]` in, predictions `[lanes, M]` out. The fit then evaluates every lane and every finite-difference perturbation in a single call to your model.
- Start from a value you can defend physically, and try a second start in another lane. Several lanes that agree on the minimum are the cheapest check against a poor local minimum.
- Read `converged` per lane. The iteration cap and a stalled step are reported as `0`; use `Options` to change `max_iterations`, `tol_x`, `tol_fun` and the finite-difference step.
- Ask for `confidence_intervals` only for fits with more observations than parameters; with `alpha` other than the default `0.05` you choose the level.

## Pitfalls

A fit that ran out of iterations is returned like any other. The program starts far from the answer with a cap of `3` iterations, then with the default cap.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace estimation = common::estimation;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// initial rates of an enzyme measured at six substrate concentrations (mM -> umol/min)
static pluss::Series substrate() { return vec({0.5, 1.0, 2.0, 4.0, 8.0, 16.0}); }
static pluss::Series rate() { return vec({0.3201, 0.4880, 0.8052, 1.1356, 1.4274, 1.6291}); }
// the model sees a batch of parameter vectors beta [lanes, 2] = (Vmax, Km) and returns the predicted rates [lanes, 6]
static pluss::Series michaelis_menten(const pluss::Series &beta) {
  const auto vmax = beta.select(-1, 0).unsqueeze(-1), km = beta.select(-1, 1).unsqueeze(-1);
  return vmax.mul(substrate()).div(km.add(substrate()));
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // starting far from the answer with a tight iteration budget: the fit stops on the cap, not on a tolerance
  estimation::Options opt;
  opt.max_iterations = 3;
  const auto fit = estimation::levenberg_marquardt(michaelis_menten, rate(), mat({{50.0, 100.0}}), opt);
  out.writeln("beta after 3 iterations = ", flat(fit.beta), "  converged = ", flat(fit.converged)[0] > 0.5 ? "yes" : "no");
  const auto full = estimation::levenberg_marquardt(michaelis_menten, rate(), mat({{50.0, 100.0}}));
  out.writeln("beta with the default cap = ", flat(full.beta), "  converged = ", flat(full.converged)[0] > 0.5 ? "yes" : "no", "  iterations = ", flat(full.iterations));
}
```

Output

```text
beta after 3 iterations = [7.3726, 38.5738]  converged = no
beta with the default cap = [1.9125, 2.7497]  converged = yes  iterations = [14.0000]
```

After `3` iterations the estimates are `(7.3726, 38.5738)` and `converged = no`; with the default cap the same start reaches `(1.9125, 2.7497)` in `14` iterations and `converged = yes`. The numbers from the first run look plausible and are wrong. Test `converged` before using `beta`.

Two parameters that appear only as a product cannot be separated by any data. The program fits {math}`v = (ab)\,S/(3 + S)` and asks for the intervals.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace estimation = common::estimation;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// initial rates of an enzyme measured at six substrate concentrations (mM -> umol/min)
static pluss::Series substrate() { return vec({0.5, 1.0, 2.0, 4.0, 8.0, 16.0}); }
static pluss::Series rate() { return vec({0.3201, 0.4880, 0.8052, 1.1356, 1.4274, 1.6291}); }
// the model sees a batch of parameter vectors beta [lanes, 2] = (Vmax, Km) and returns the predicted rates [lanes, 6]
static pluss::Series michaelis_menten(const pluss::Series &beta) {
  const auto vmax = beta.select(-1, 0).unsqueeze(-1), km = beta.select(-1, 1).unsqueeze(-1);
  return vmax.mul(substrate()).div(km.add(substrate()));
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // two parameters that only ever appear as a product cannot be told apart: v = (a b) S / (3 + S)
  const auto product = [](const pluss::Series &beta) {
    const auto ab = beta.select(-1, 0).mul(beta.select(-1, 1)).unsqueeze(-1);
    return ab.mul(substrate()).div(substrate().add(3.0));
  };
  const auto fit = estimation::levenberg_marquardt(product, rate(), mat({{1.0, 1.0}}));
  out.writeln("(a, b) = ", flat(fit.beta), ", a b = ", flat(fit.beta)[0] * flat(fit.beta)[1]);
  try {
    const auto ci = estimation::confidence_intervals(fit);
    out.writeln("standard errors = ", flat(ci.standard_error));
  } catch (const pluss::Error &e) {
    out.writeln("intervals refused: ", e.message());
  }
}
```

Output

```text
(a, b) = [1.4012, 1.4012], a b = 1.9633
intervals refused: natsci: estimation: J^T J is singular at the solution (some parameter combination is not identifiable); inspect the profile likelihood
```

The fit succeeds and returns {math}`a = b = 1.4012`, with the product {math}`ab = 1.9633` determined; only the product is. The intervals are refused because {math}`J^{\top}J` is singular, with a message that names the cause. A parameter pair that is not identifiable is a property of the model; [the profile page](profile_likelihood.md) and [the sensitivity page](sensitivities.md) show how to see it before fitting.

Data and starting values that are not what the fit expects are rejected. The program passes a NaN reading, a starting vector without a lane axis and a two-point fit of two parameters.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace estimation = common::estimation;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// initial rates of an enzyme measured at six substrate concentrations (mM -> umol/min)
static pluss::Series substrate() { return vec({0.5, 1.0, 2.0, 4.0, 8.0, 16.0}); }
static pluss::Series rate() { return vec({0.3201, 0.4880, 0.8052, 1.1356, 1.4274, 1.6291}); }
// the model sees a batch of parameter vectors beta [lanes, 2] = (Vmax, Km) and returns the predicted rates [lanes, 6]
static pluss::Series michaelis_menten(const pluss::Series &beta) {
  const auto vmax = beta.select(-1, 0).unsqueeze(-1), km = beta.select(-1, 1).unsqueeze(-1);
  return vmax.mul(substrate()).div(km.add(substrate()));
}

int main() {
  OutputEngine out;
  try {
    (void)estimation::levenberg_marquardt(michaelis_menten, vec({0.3, 0.5, std::nan(""), 1.1, 1.4, 1.6}), mat({{1.0, 1.0}}));
  } catch (const pluss::Error &e) {
    out.writeln("NaN in the data:        ", e.message());
  }
  try {
    (void)estimation::levenberg_marquardt(michaelis_menten, rate(), vec({1.0, 1.0}));
  } catch (const pluss::Error &e) {
    out.writeln("beta0 as a plain vector: ", e.message());
  }
  const auto two = estimation::levenberg_marquardt([](const pluss::Series &b) { return b.mul(vec({1.0, 2.0})); }, vec({1.0, 2.0}), mat({{0.5, 0.5}}));
  try {
    (void)estimation::confidence_intervals(two);
  } catch (const pluss::Error &e) {
    out.writeln("two points, two parameters: ", e.message());
  }
}
```

Output

```text
NaN in the data:         natsci: estimation: data must be finite
beta0 as a plain vector:  natsci: estimation: beta0 must be [lanes, parameters]
two points, two parameters:  natsci: estimation: need more observations than parameters
```

A NaN reading is refused (`data must be finite`) instead of being fitted, a starting vector must be `[lanes, parameters]`, and intervals need more observations than parameters. The two-point example fits exactly and has no degrees of freedom left, so it has no residual to estimate an error from.

## Where next

[Profile likelihood](profile_likelihood.md) gives intervals that do not assume the model is nearly linear near the minimum, and [Sensitivities and identifiability](sensitivities.md) tells which parameters a set of measurement times can determine at all. The rate laws fitted here come from [Biology 5: Enzymes](../../biology/bio_05_enzymes.md); [Biology 27: Enzyme Fitting](../../biology/bio_27_enzyme_fitting.md) fits whole progress curves.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
