# Sensitivities and identifiability

[Back to Methods 2: Fitting and Identifiability](../meth_02_fitting.md)

Before collecting data you can ask which parameters the experiment will be able to determine. This page integrates the sensitivity of a model's output to each of its parameters along the trajectory, scales it into relative sensitivities, and turns it into the Fisher information of a set of measurement times, from which you read the best accuracy any estimate of the parameters can reach, and whether some parameter combination cannot be recovered at all.

## How it works

For a model {math}`\mathrm{d}y/\mathrm{d}t = f(y, p)` with parameters {math}`p`, the sensitivity {math}`S = \partial y/\partial p` obeys the linear equation

```{math}
\dot S = f_y\,S + f_p,\qquad S(0) = 0
```

and is integrated together with the state as one augmented system. The two Jacobians {math}`f_y` and {math}`f_p` are central differences of the rate you pass. If you observe the state at times {math}`t_k` with independent unit noise, the Fisher information of the parameters is {math}`F = \sum_k S_k^{\top}S_k`, and the Cramér-Rao bound says no unbiased estimate has a covariance below {math}`F^{-1}`. An eigenvalue of {math}`F` near zero is a parameter combination the measurements cannot see.

The code is in `pluss::ma::natsci::common::sensitivity` (`sensitivity::` below). `sensitivity::trajectory(f, y0, p, t0, t1, samples)` returns `times`, `states` `[samples+1, n]` and `dydp` `[samples+1, n, P]`; the rate `f(y, p)` must read its parameters as `p.select(-1, i).unsqueeze(-1)` so that they broadcast. `sensitivity::normalized(dydp, y, p)` returns the dimensionless {math}`\partial\ln y/\partial\ln p`; `sensitivity::fisher_information(dydp)` sums over a stack `[T, n, P]` of the samples you observe; `sensitivity::fisher_spectrum(F)`, `sensitivity::numerical_rank(spectrum)` and `sensitivity::cramer_rao_std(F)` read the result. The programs use logistic growth, {math}`y' = r\,y\,(1 - y/K)`, with {math}`p = (r, K)`, {math}`y(0) = 1`, {math}`r = 0.8` and {math}`K = 10`.

## The sensitivity trajectory

The program integrates the logistic model with its sensitivities to {math}`r` and {math}`K` over three time units, and compares the last value with the closed-form derivatives.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace sensitivity = common::sensitivity;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// logistic growth  y' = r y (1 - y / K)  with p = (r, K): read each parameter as p.select(-1, i).unsqueeze(-1) so it broadcasts against y
static pluss::Series logistic(const pluss::Series &y, const pluss::Series &p) {
  const auto r = p.select(-1, 0).unsqueeze(-1), k = p.select(-1, 1).unsqueeze(-1);
  return r.mul(y).mul(y.div(k).neg().add(1.0));
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // y(0) = 1, r = 0.8, K = 10, observed at t = 0, 1, 2, 3
  const auto s = sensitivity::trajectory(logistic, vec({1.0}), vec({0.8, 10.0}), 0.0, 3.0, 3);
  out.writeln("times        ", flat(s.times));
  out.writeln("y(t)         ", flat(s.states));
  out.writeln("dy/dr  (t)   ", flat(s.dydp.select(-1, 0)));
  out.writeln("dy/dK  (t)   ", flat(s.dydp.select(-1, 1)));
  // the closed form at t = 3: with E = exp(r t), D = K + y0 (E - 1)   dy/dr = K y0 t E (K - y0) / D^2,  dy/dK = y0^2 E (E - 1) / D^2
  const double r = 0.8, k = 10.0, t = 3.0, e = std::exp(r * t), d = k + (e - 1.0);
  out.writeln("closed form at t = 3: dy/dr = ", k * t * e * (k - 1.0) / (d * d), ", dy/dK = ", e * (e - 1.0) / (d * d));
}
```

Output

```text
times        [0.0000, 1.0000, 2.0000, 3.0000]
y(t)         [1.0000, 1.9826, 3.5498, 5.5052]
dy/dr  (t)   [0.0000, 1.5895, 4.5794, 7.4234]
dy/dK  (t)   [0.0000, 0.0216, 0.1006, 0.2756]
closed form at t = 3: dy/dr = 7.4234, dy/dK = 0.2756
```

The state grows from `1.0000` to `5.5052`, and the sensitivity to {math}`r` grows with it (`1.5895`, `4.5794`, `7.4234`), much faster than the sensitivity to {math}`K` (`0.0216`, `0.1006`, `0.2756`): early on, the growth rate matters far more than the carrying capacity. At {math}`t = 3` the integrated values `7.4234` and `0.2756` equal the closed-form `7.4234` and `0.2756`.

## Relative sensitivities

A percent change is easier to compare than a derivative with units. The program prints {math}`\partial\ln y/\partial\ln r` and {math}`\partial\ln y/\partial\ln K` at each of the first eight times.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace sensitivity = common::sensitivity;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// logistic growth  y' = r y (1 - y / K)  with p = (r, K): read each parameter as p.select(-1, i).unsqueeze(-1) so it broadcasts against y
static pluss::Series logistic(const pluss::Series &y, const pluss::Series &p) {
  const auto r = p.select(-1, 0).unsqueeze(-1), k = p.select(-1, 1).unsqueeze(-1);
  return r.mul(y).mul(y.div(k).neg().add(1.0));
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto p = vec({0.8, 10.0});
  const auto s = sensitivity::trajectory(logistic, vec({1.0}), p, 0.0, 8.0, 8);
  // the state and sensitivities at each sample, then d ln y / d ln p: the percent change of y per percent change of the parameter
  for (std::int64_t k = 1; k <= 8; k += 1) {
    const auto yk = s.states.select(0, k), sk = s.dydp.select(0, k);
    const auto rel = sensitivity::normalized(sk, yk, p);
    out.writeln("t = ", k, "   y = ", flat(yk), "   d ln y/d ln r, d ln y/d ln K = ", flat(rel));
  }
}
```

Output

```text
t = 1   y = [1.9826]   d ln y/d ln r, d ln y/d ln K = [0.6414, 0.1092]
t = 2   y = [3.5498]   d ln y/d ln r, d ln y/d ln K = [1.0320, 0.2833]
t = 3   y = [5.5052]   d ln y/d ln r, d ln y/d ln K = [1.0787, 0.5006]
t = 4   y = [7.3160]   d ln y/d ln r, d ln y/d ln K = [0.8589, 0.7018]
t = 5   y = [8.5849]   d ln y/d ln r, d ln y/d ln K = [0.5661, 0.8428]
t = 6   y = [9.3104]   d ln y/d ln r, d ln y/d ln K = [0.3310, 0.9234]
t = 7   y = [9.6779]   d ln y/d ln r, d ln y/d ln K = [0.1804, 0.9642]
t = 8   y = [9.8527]   d ln y/d ln r, d ln y/d ln K = [0.0943, 0.9836]
```

At {math}`t = 3` a one percent change of {math}`r` moves {math}`y` by `1.0787` percent and of {math}`K` by `0.5006` percent. The influence of {math}`r` peaks near {math}`t = 3` (`1.0787`) and then falls to `0.0943` at {math}`t = 8`, while that of {math}`K` rises to `0.9836`: late in the run, the population has reached {math}`K` and says almost nothing about the rate that carried it there.

## What six readings can determine

The program observes the state at {math}`t = 1, \dots, 6` with unit noise and prints the Fisher information, its eigenvalues, the numerical rank and the Cramér-Rao bounds on {math}`r` and {math}`K`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace sensitivity = common::sensitivity;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// logistic growth  y' = r y (1 - y / K)  with p = (r, K): read each parameter as p.select(-1, i).unsqueeze(-1) so it broadcasts against y
static pluss::Series logistic(const pluss::Series &y, const pluss::Series &p) {
  const auto r = p.select(-1, 0).unsqueeze(-1), k = p.select(-1, 1).unsqueeze(-1);
  return r.mul(y).mul(y.div(k).neg().add(1.0));
}

// observe y at the given times with unit noise and report what the data can say about (r, K)
static void report(OutputEngine &out, const char *label, const std::vector<double> &times) {
  const auto p = vec({0.8, 10.0});
  const double horizon = times.back();
  const auto s = sensitivity::trajectory(logistic, vec({1.0}), p, 0.0, horizon, static_cast<std::int64_t>(horizon * 4));
  std::vector<pluss::Series> rows;
  for (const double t : times)
    rows.push_back(s.dydp.select(0, static_cast<std::int64_t>(t * 4)).unsqueeze(0));
  const auto stacked = rows.front().unite(std::vector<pluss::Series>(rows.begin() + 1, rows.end()), 0);   // [T, n, P]
  const auto fim = sensitivity::fisher_information(stacked);
  const auto spectrum = sensitivity::fisher_spectrum(fim);
  out.writeln(label);
  out.writeln("  Fisher information = ", flat(fim));
  out.writeln("  eigenvalues        = ", flat(spectrum), ", numerical rank = ", flat(sensitivity::numerical_rank(spectrum)));
  out.writeln("  Cramer-Rao std (r, K) = ", flat(sensitivity::cramer_rao_std(fim)));
}
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  report(out, "six readings through the growth phase (t = 1 .. 6)", {1, 2, 3, 4, 5, 6});
}
```

Output

```text
six readings through the growth phase (t = 1 .. 6)
  Fisher information = [192.0339, 14.2800, 14.2800, 1.6127]
  eigenvalues        = [193.0988, 0.5478], numerical rank = [2.0000]
  Cramer-Rao std (r, K) = [0.1235, 1.3474]
```

The information matrix is `[192.0339, 14.2800; 14.2800, 1.6127]`, with eigenvalues `193.0988` and `0.5478` and numerical rank `2`: both parameters are identifiable. The Cramér-Rao standard deviations are `0.1235` for {math}`r` and `1.3474` for {math}`K` for unit measurement noise; both scale in proportion to the real noise level. The small second eigenvalue says that one combination of the two is much worse determined than the other.

## Decisions

- Use sensitivities to design an experiment, before the data exist: the Fisher information of a candidate set of times needs only the model and a rough guess of the parameters.
- Report the spectrum, not just the Cramér-Rao bounds. The bound is a lower limit, and it is only as reliable as the linearisation at the guessed parameters.
- For readings with different noise levels, divide each row of the sensitivity by its standard deviation before summing; for several observed outputs, stack their sensitivities on the second axis.

## Pitfalls

The same number of readings can determine very different amounts. The program compares readings at {math}`t = 1\dots6`, during growth, with readings at {math}`t = 15\dots20`, after the population has saturated.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace sensitivity = common::sensitivity;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// logistic growth  y' = r y (1 - y / K)  with p = (r, K): read each parameter as p.select(-1, i).unsqueeze(-1) so it broadcasts against y
static pluss::Series logistic(const pluss::Series &y, const pluss::Series &p) {
  const auto r = p.select(-1, 0).unsqueeze(-1), k = p.select(-1, 1).unsqueeze(-1);
  return r.mul(y).mul(y.div(k).neg().add(1.0));
}

static void report(OutputEngine &out, const char *label, const std::vector<double> &times) {
  const auto p = vec({0.8, 10.0});
  const auto s = sensitivity::trajectory(logistic, vec({1.0}), p, 0.0, 20.0, 80);
  std::vector<pluss::Series> rows;
  for (const double t : times)
    rows.push_back(s.dydp.select(0, static_cast<std::int64_t>(t * 4)).unsqueeze(0));
  const auto fim = sensitivity::fisher_information(rows.front().unite(std::vector<pluss::Series>(rows.begin() + 1, rows.end()), 0));
  const auto spectrum = sensitivity::fisher_spectrum(fim);
  out.writeln(label, ":  eigenvalues = ", flat(spectrum), "  Cramer-Rao std (r, K) = ", flat(sensitivity::cramer_rao_std(fim)));
}
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the same number of readings, but all of them after the population has saturated: r has left almost no trace
  report(out, "t = 1 .. 6  ", {1, 2, 3, 4, 5, 6});
  report(out, "t = 15 .. 20", {15, 16, 17, 18, 19, 20});
}
```

Output

```text
t = 1 .. 6  :  eigenvalues = [193.0988, 0.5478]  Cramer-Rao std (r, K) = [0.1235, 1.3474]
t = 15 .. 20:  eigenvalues = [5.9996, 0.0000]  Cramer-Rao std (r, K) = [144.1135, 0.5558]
```

Early readings give eigenvalues `193.0988` and `0.5478` and standard deviations `0.1235` and `1.3474`. Late readings give `5.9996` and `0.0000` and the standard deviation of {math}`r` explodes to `144.1135`: after saturation the readings only say that the population is at {math}`K`, and the rate that led there is invisible.

A parameter combination that never appears separately cannot be determined. The program uses {math}`y' = -(a+b)\,y`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace sensitivity = common::sensitivity;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// logistic growth  y' = r y (1 - y / K)  with p = (r, K): read each parameter as p.select(-1, i).unsqueeze(-1) so it broadcasts against y
static pluss::Series logistic(const pluss::Series &y, const pluss::Series &p) {
  const auto r = p.select(-1, 0).unsqueeze(-1), k = p.select(-1, 1).unsqueeze(-1);
  return r.mul(y).mul(y.div(k).neg().add(1.0));
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // y' = -(a + b) y: only the SUM a + b ever appears, so the two parameters cannot be separated by any data
  const auto decay = [](const pluss::Series &y, const pluss::Series &p) { return y.mul(p.select(-1, 0).add(p.select(-1, 1)).unsqueeze(-1).neg()); };
  const auto s = sensitivity::trajectory(decay, vec({1.0}), vec({0.3, 0.2}), 0.0, 4.0, 4);
  const auto fim = sensitivity::fisher_information(s.dydp);
  const auto spectrum = sensitivity::fisher_spectrum(fim);
  out.writeln("eigenvalues = ", flat(spectrum), ", numerical rank = ", flat(sensitivity::numerical_rank(spectrum)));
  try {
    out.writeln("Cramer-Rao std = ", flat(sensitivity::cramer_rao_std(fim)));
  } catch (const pluss::Error &e) {
    out.writeln("Cramer-Rao bound refused: ", e.message());
  }
}
```

Output

```text
eigenvalues = [3.3007, 0.0000], numerical rank = [1.0000]
Cramer-Rao bound refused: natsci: cramer_rao_std: the Fisher information is singular (a parameter combination is not identifiable); inspect fisher_spectrum
```

The Fisher information has the eigenvalues `3.3007` and `0.0000` and numerical rank `1`: one combination, {math}`a+b`, is determined and the difference is not. The Cramér-Rao bound is refused with a message that names the singular information. A singular information matrix has no finite bound, and the call says so instead of returning a number.

A NaN parameter, a count of zero samples and a parameter that is not a vector are rejected.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace sensitivity = common::sensitivity;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// logistic growth  y' = r y (1 - y / K)  with p = (r, K): read each parameter as p.select(-1, i).unsqueeze(-1) so it broadcasts against y
static pluss::Series logistic(const pluss::Series &y, const pluss::Series &p) {
  const auto r = p.select(-1, 0).unsqueeze(-1), k = p.select(-1, 1).unsqueeze(-1);
  return r.mul(y).mul(y.div(k).neg().add(1.0));
}

int main() {
  OutputEngine out;
  try {
    (void)sensitivity::trajectory(logistic, vec({1.0}), vec({0.8, std::nan("")}), 0.0, 3.0, 3);
  } catch (const pluss::Error &e) {
    out.writeln("NaN parameter: ", e.message());
  }
  try {
    (void)sensitivity::trajectory(logistic, vec({1.0}), vec({0.8, 10.0}), 0.0, 3.0, 0);
  } catch (const pluss::Error &e) {
    out.writeln("zero samples:  ", e.message());
  }
  try {
    (void)sensitivity::trajectory(logistic, vec({1.0}), pluss::series::make(0.8).to(pluss::ScalarType::Float64), 0.0, 3.0, 3);
  } catch (const pluss::Error &e) {
    out.writeln("scalar p:      ", e.message());
  }
}
```

Output

```text
NaN parameter:  natsci: sensitivity: parameters must be finite
zero samples:   natsci: sensitivity: samples and steps_per_sample must be positive
scalar p:       natsci: sensitivity: y0 and p need a species / parameter axis
```

Each error names the argument. A NaN parameter would otherwise propagate through the integration and leave NaN sensitivities that could be summed into a Fisher information without complaint.

## Where next

[Profile likelihood](profile_likelihood.md) tests the same identifiability on real data rather than by linearisation. The logistic law is introduced in [Biology 1: Growth](../../biology/bio_01_growth.md), and [Biology 3: Demography](../../biology/bio_03_demography.md) asks which vital rates matter most for a population's growth.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
