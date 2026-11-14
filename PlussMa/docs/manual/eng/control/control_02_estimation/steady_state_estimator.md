<!-- generated from steady_state_estimator.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The steady-state estimator

[Back to Control 2: Estimation](../control_02_estimation.md)

You can measure some outputs of a system, but not its states, and the measurements are noisy. An **observer** reconstructs the states from the outputs, and the best linear observer for a given noise model is the Kalman filter. `control::lqe` computes the fixed gain that filter settles to when the system and noise statistics do not change: the steady-state estimator. It is the same mathematics as the LQR of the previous chapter, run on the dual system, and this page shows the identity and what the result means.

## The mental model

The discrete system is

```{math}
x_{k+1} = A x_k + w_k, \qquad y_k = C x_k + v_k,
```

with process noise `w` of covariance `W` and measurement noise `v` of covariance `V`. A **predictor-form observer** keeps an estimate `x^` and corrects it with the innovation, the difference between the measurement and the prediction:

```{math}
\hat x_{k+1} = A \hat x_k + L\,(y_k - C \hat x_k).
```

The estimation error then obeys `e_{k+1} = (A - L C) e_k + (noise)`, so `L` must make `A - L C` stable, and the best `L` minimises the error covariance. If `P` is that covariance (the covariance of the one-step *prediction* error), then

```{math}
P = A P A^T - A P C^T (C P C^T + V)^{-1} C P A^T + W, \qquad L = A P C^T (C P C^T + V)^{-1}.
```

This is the discrete Riccati equation of the LQR with the roles `(A, B, Q, R) -> (A^T, C^T, W, V)`: **estimation is control of the dual system**. `lqe` solves it, with `system` giving `A` and `C`, and returns an `LqeResult` of three members: `covariance`, the matrix `P`; `gain`, the matrix `L`; and `closed_loop`, `A - L C`. For a continuous system the same function solves the continuous equation.

## One state

The system `x[k+1] = 0.9 x[k] + w`, `y = x + v` with `W = 0.1` and `V = 1` has a scalar equation. The program solves it with `lqe`, checks the Riccati equation by hand, and checks the gain formula.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;

static pluss::Series scalar(double value) { return pluss::series::make(std::vector<double>{value}).reshape({1, 1}); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto plant = lti::state(scalar(0.9), scalar(1.0), scalar(1.0), scalar(0.0), 1.0);
  const auto estimator = ctl::lqe(plant, scalar(0.1), scalar(1.0));

  const double p = estimator.covariance.squeeze().item<double>();
  out.writeln("P (prediction error covariance)", p);
  out.writeln("L (gain)                       ", estimator.gain.squeeze().item<double>(), " hand: a P / (P + V) =", 0.9 * p / (p + 1.0));
  out.writeln("A - L C                        ", estimator.closed_loop.squeeze().item<double>());
  out.writeln("Riccati residual", std::abs(0.81 * p - 0.81 * p * p / (p + 1.0) + 0.1 - p));
}
```

Output

```text
P (prediction error covariance) 0.274414
L (gain)                        0.193793  hand: a P / (P + V) = 0.193793
A - L C                         0.706207
Riccati residual 0.000000
```

The prediction error has a steady variance of `0.274`, below the measurement variance of `1`: the filter knows the state better than any single measurement does. The error dynamics `A - L C = 0.706` are faster than the plant's `0.9`, which is the point of the correction. The Riccati residual is zero to the printed digits.

## Two states: a constant-velocity target

A target moves with roughly constant velocity and its position is measured with unit noise. The states are position and velocity, `A = [[1, 1], [0, 1]]`, `C = [1, 0]`. The velocity wanders slightly, with variance `0.01` per step, and the position has no process noise of its own: `W = diag(0, 0.01)`, `V = 1`. The program prints `P`, `L` and the closed-loop matrix, and verifies the dual identity by calling `dare` on the transposed system.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;

static pluss::Series matrix(std::vector<double> values, long rows, long columns) {
  return pluss::series::make(values).reshape({rows, columns});
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const pluss::Series a = matrix({1, 1, 0, 1}, 2, 2);
  const pluss::Series c = matrix({1, 0}, 1, 2);
  const auto plant = lti::state(a, matrix({0.5, 1}, 2, 1), c, matrix({0}, 1, 1), 1.0);
  const pluss::Series w = matrix({0, 0, 0, 0.01}, 2, 2);
  const pluss::Series v = matrix({1}, 1, 1);

  const auto estimator = ctl::lqe(plant, w, v);
  out.writeln("P", estimator.covariance.reshape({4}).items<double>());
  out.writeln("L", estimator.gain.reshape({2}).items<double>());
  out.writeln("A - L C", estimator.closed_loop.reshape({4}).items<double>());

  const auto dual = ctl::dare(a.transpose(0, 1), c.transpose(0, 1), w, v);
  out.writeln("dare on the dual system: largest difference in P", (dual.value - estimator.covariance).abs().max().item<double>(),
              " in L", (dual.gain.transpose(0, 1) - estimator.gain).abs().max().item<double>());
}
```

Output

```text
P [0.566832, 0.125173, 0.125173, 0.055284]
L [0.441659, 0.079889]
A - L C [0.558341, 1.000000, -0.079889, 1.000000]
dare on the dual system: largest difference in P 0.000000  in L 0.000000
```

The position gain `0.4417` says the estimate moves 44 percent of the way towards the measurement's surprise in the next position, and the velocity gain `0.0799` corrects the velocity by 8 percent of the position surprise. The matrix `A - L C` has the stable eigenvalues that the correction produces. `dare` on the dual system gives the same `P` and `L`.

## Correlated noise

If the process and measurement noise are correlated, with cross covariance `N = E[w v^T]` of shape `[states, outputs]`, it is passed as the fourth argument. The program compares the gain without and with a cross term for the scalar system, and checks that `lqe` with the cross term equals the dual `dare` with the same cross term.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;

static pluss::Series scalar(double value) { return pluss::series::make(std::vector<double>{value}).reshape({1, 1}); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto plant = lti::state(scalar(0.9), scalar(1.0), scalar(1.0), scalar(0.0), 1.0);
  const auto independent = ctl::lqe(plant, scalar(0.1), scalar(1.0));
  const auto correlated = ctl::lqe(plant, scalar(0.1), scalar(1.0), scalar(0.05));
  out.writeln("gain, independent noise ", independent.gain.squeeze().item<double>());
  out.writeln("gain, cross covariance 0.05", correlated.gain.squeeze().item<double>());

  const auto dual = ctl::dare(scalar(0.9), scalar(1.0), scalar(0.1), scalar(1.0), scalar(0.05));
  out.writeln("equals the dual dare with the same cross term:", (dual.gain - correlated.gain).abs().max().item<double>());
}
```

Output

```text
gain, independent noise  0.193793
gain, cross covariance 0.05 0.211719
equals the dual dare with the same cross term: 0.000000
```

## A continuous system

For a continuous plant the same call solves the continuous equation, and `L` is the gain of an observer `x^' = A x^ + L (y - C x^)`. The covariances are then spectral densities of the continuous noise. The program uses `x' = -x + w`, `y = x + v` with `W = V = 1`: the equation `-2P - P^2 + 1 = 0` gives `P = sqrt(2) - 1 = 0.4142`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;

static pluss::Series scalar(double value) { return pluss::series::make(std::vector<double>{value}).reshape({1, 1}); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto plant = lti::state(scalar(-1.0), scalar(1.0), scalar(1.0), scalar(0.0));
  const auto estimator = ctl::lqe(plant, scalar(1.0), scalar(1.0));
  out.writeln("P", estimator.covariance.squeeze().item<double>(), " hand: sqrt(2) - 1 =", std::sqrt(2.0) - 1.0);
  out.writeln("L", estimator.gain.squeeze().item<double>(), " A - L C", estimator.closed_loop.squeeze().item<double>());
}
```

Output

```text
P 0.414214  hand: sqrt(2) - 1 = 0.414214
L 0.414214  A - L C -1.414214
```

The error dynamics `A - L C = -1.414` are faster than the plant's `-1`, again because the correction is working.

## Errors

The covariances are checked before solving, and the messages name the argument. The program shows a negative process covariance, a zero noise covariance, a process covariance of the wrong shape, and a continuous or unobservable system.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;

static pluss::Series scalar(double value) { return pluss::series::make(std::vector<double>{value}).reshape({1, 1}); }
static pluss::Series matrix(std::vector<double> values, long rows, long columns) {
  return pluss::series::make(values).reshape({rows, columns});
}

int main() {
  OutputEngine out;
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const std::exception &e) {
      std::string text(e.what());
      if (const auto cut = text.find("Triggered"); cut != std::string::npos) text.erase(cut);
      while (!text.empty() && text.back() == '\n') text.pop_back();
      out.writeln(text);
    }
  };
  const auto plant = lti::state(scalar(0.9), scalar(1.0), scalar(1.0), scalar(0.0), 1.0);
  report([&] { (void)ctl::lqe(plant, scalar(-0.1), scalar(1.0)); });
  report([&] { (void)ctl::lqe(plant, scalar(0.1), scalar(0.0)); });
  report([&] { (void)ctl::lqe(plant, matrix({0.1, 0, 0, 0.1}, 2, 2), scalar(1.0)); });
  const auto blind = lti::state(matrix({1.2, 0, 0, 0.5}, 2, 2), matrix({1, 1}, 2, 1), matrix({0, 1}, 1, 2), scalar(0.0), 1.0);
  report([&] { (void)ctl::lqe(blind, matrix({0.1, 0, 0, 0.1}, 2, 2), scalar(1.0)); });
}
```

Output

```text
eng::control::lqe: process_cov must be positive semidefinite
eng::control::lqe: noise_cov must be positive definite
eng::control::lqe: process_cov must have shape [..., states, states]
eng::control::lqe: no stabilising solution; (A, C) must be detectable (every unstable mode must be visible in the outputs). Solver message: subsp::linearalg::dare: doubling iteration produced non-finite values
```

The last system has an unstable state (`1.2`) that the output cannot see, so no observer can track it, and the message states the condition: the pair must be detectable.

## Decisions

- **A fixed gain for a stationary problem, implemented as a simple recursion:** `lqe`, then run `x^ = A x^ + B u + L (y - C x^)` yourself, or simulate the closed-loop `A - L C`.
- **Estimates with uncertainty at every step, from a start with a poor initial guess, with missing data:** the time-varying filter of the [next page](kalman_filter.md); it converges to this estimator.
- **Setting `W` and `V`:** from the known noise levels of your sensors and the plausible size of unmodelled disturbances. Their ratio sets the gain: noisy measurements (large `V`) give a low gain.

## Pitfalls

The program makes three mistakes with a steady-state estimator: taking its covariance as the error after the measurement, giving a standard deviation where a variance is expected, and trusting the gain to cope with outliers.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;

static pluss::Series scalar(double value) { return pluss::series::make(std::vector<double>{value}).reshape({1, 1}); }
static double rms(const pluss::Series &e) { return std::sqrt(e.square().mean().item<double>()); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const int steps = 400;

  // a sensor with a standard deviation of 0.5: sin has a mean square of 1/2, so 0.7071 sin has a variance of 0.25
  std::vector<double> truth, measured;
  double x = 3.0;
  for (int t = 0; t < steps; ++t) {
    truth.push_back(x);
    measured.push_back(x + 0.7071 * std::sin(1.7 * t));
    x = 0.97 * x + 0.3 * std::cos(2.3 * t);
  }
  const auto plant = lti::state(scalar(0.97), scalar(1.0), scalar(1.0), scalar(0.0), 1.0);
  const auto start = pluss::series::make(std::vector<double>{0.0});
  const auto state = pluss::make_series(truth, {steps}, f64);
  const auto run = [&](const std::vector<double> &data, double process, double noise) {
    return ctl::kalman(plant, pluss::make_series(data, {steps, 1}, f64), scalar(process), scalar(noise), start, scalar(10.0));
  };
  const auto last = [&](const pluss::Series &s) { return s.select(0, steps - 1).squeeze().item<double>(); };

  // 1. the covariance of lqe is the covariance before the measurement; the one after is smaller
  const auto estimator = ctl::lqe(plant, scalar(0.045), scalar(0.25));
  const double p = estimator.covariance.squeeze().item<double>();
  const auto filter = run(measured, 0.045, 0.25);
  out.writeln("lqe covariance P = ", p, "   the filter's prediction covariance at the last step: ", last(filter.predicted_cov));
  out.writeln("after the measurement: P - P^2 / (P + V) = ", p - p * p / (p + 0.25), "   the filter's filtered covariance: ", last(filter.filtered_cov));

  // 2. W and V are variances: a standard deviation of 0.5 is V = 0.25
  const auto right = ctl::lqe(plant, scalar(0.045), scalar(0.25));
  const auto wrong = ctl::lqe(plant, scalar(0.045), scalar(0.5));
  out.writeln("gain with V = 0.25: ", right.gain.squeeze().item<double>(), "   with the standard deviation 0.5 given as V: ", wrong.gain.squeeze().item<double>());
  const auto consistency = [&](double noise) {
    const auto f = run(measured, 0.045, noise);
    return (f.innovation.square().mean() / f.innovation_cov.mean()).item<double>();
  };
  out.writeln("mean innovation^2 / mean innovation_cov, which should be 1: ", consistency(0.25), " with V = 0.25 and ", consistency(0.5), " with V = 0.5");

  // 3. the gain is optimal for Gaussian noise: a few wild measurements are trusted like any other
  auto spiked = measured;
  for (int t = 20; t < steps; t += 40) spiked[t] += 6.0;
  out.writeln("error of the estimate without outliers: ", rms(run(measured, 0.045, 0.25).filtered_mean.reshape({steps}) - state), "   with an outlier of +6 every 40 steps: ", rms(run(spiked, 0.045, 0.25).filtered_mean.reshape({steps}) - state));
  out.writeln("the estimator pole stays at ", estimator.closed_loop.squeeze().item<double>(), "   (stable, and no more robust to outliers)");
}
```

Output

```text
lqe covariance P = 0.1222   the filter's prediction covariance at the last step: 0.1222
after the measurement: P - P^2 / (P + V) = 0.0821   the filter's filtered covariance: 0.0821
gain with V = 0.25: 0.3186   with the standard deviation 0.5 given as V: 0.2328
mean innovation^2 / mean innovation_cov, which should be 1: 0.9757 with V = 0.25 and 0.5235 with V = 0.5
error of the estimate without outliers: 0.1606   with an outlier of +6 every 40 steps: 0.4416
the estimator pole stays at 0.6514   (stable, and no more robust to outliers)
```

`lqe` returns the covariance before the measurement, `0.1222`, which is exactly what the filter has at its last step; the covariance after the measurement is smaller, `P - P^2 / (P + V) = 0.0821`, which the filter reports as its filtered covariance. A sensor with a standard deviation of `0.5` has `V = 0.25`: giving `0.5` makes the estimator trust the sensor less, a gain of `0.2328` against `0.3186`, and the innovation check shows it, `0.5235` where `1` is consistent, against `0.9757` for the right variance. And the gain is optimal for Gaussian noise: an outlier of `+6` every `40` steps raises the error from `0.1606` to `0.4416`, while the estimator pole stays at `0.6514`, stable, since stability was never at risk.

Reference: [`state`](../../../../api/eng/lti/state/state.md), [`lqe`](../../../../api/eng/control/estimation/lqe.md), [`dare`](../../../../api/eng/control/optimal/dare.md).

Where next: [The Kalman filter](kalman_filter.md) runs the time-varying filter on data and shows that it converges to the gain computed here.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="kalman_filter.html">
    <span class="label">Next</span>
    <span class="title">The Kalman filter</span>
  </a>
</div>
<!-- apiref:nav:end -->
