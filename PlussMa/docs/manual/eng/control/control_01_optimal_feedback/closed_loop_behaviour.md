<!-- generated from closed_loop_behaviour.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Closed-loop behaviour

[Back to Control 1: Optimal Feedback](../control_01_optimal_feedback.md)

A gain from `lqr` is optimal for a cost you chose. This page closes the loop and watches it: the controlled plant is a state-space model with `A - B K` as its dynamics, and `lti::sim` runs it. You will see an unstable plant become stable, check that the cost the simulation accumulates equals `x0^T P x0`, see what the weight `R` does to the speed and the effort, and run a discrete loop.

## The mental model

With the law `u = -K x` the plant `x' = A x + B u` becomes `x' = (A - B K) x`, an autonomous system. Its model is `lti::state(A - B K, B, C, D)`, and the control signal is read out by adding the output `u = -K x`. So the closed loop is an ordinary state-space model, and every tool in the [linear systems](../../lti/lti_02_simulation/simulating_a_system.md) chapters applies to it.

The cost has an exact value along every trajectory. If `P` solves the Riccati equation, then for the LQR gain

```{math}
\int_0^\infty \left( x^T Q x + u^T R u \right) dt = x_0^T P\,x_0,
```

and for any other stabilising gain the integral is larger. The first two programs verify both statements by integrating the cost along the simulated response.

## An unstable plant becomes stable

The plant `x' = x + u` grows as `e^t` with no feedback. The program simulates it with and without the LQR gain `K = 2.414` from `x0 = 1`: the open loop is a model with `A = 1` and no input, and the closed loop has `A - B K = -1.414`. After 5 seconds the open-loop state is `e^5 = 148.4`, the closed-loop state `e^{-1.414 * 5} = 0.00085`.

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
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series time = pluss::series::arange(0, 6, f64);
  const pluss::Series silence = pluss::series::zeros({6, 1}, f64);
  const pluss::Series start = pluss::series::make(std::vector<double>{1.0});

  const auto design = ctl::care(scalar(1.0), scalar(1.0), scalar(1.0), scalar(1.0));
  const auto open_loop = lti::state(scalar(1.0), scalar(1.0), scalar(1.0), scalar(0.0));
  const auto closed_loop = lti::state(design.closed_loop, scalar(1.0), scalar(1.0), scalar(0.0));

  out.writeln("time        ", time.items<double>());
  out.writeln("open loop   ", lti::sim(open_loop, silence, time, start).values.reshape({6}).items<double>());
  out.writeln("closed loop ", lti::sim(closed_loop, silence, time, start).values.reshape({6}).items<double>());
  out.writeln("e^t at t = 5:", std::exp(5.0), " e^(-sqrt(2) t) at t = 5:", std::exp(-std::sqrt(2.0) * 5.0));
}
```

Output

```text
time         [0.00000, 1.00000, 2.00000, 3.00000, 4.00000, 5.00000]
open loop    [1.00000, 2.71828, 7.38906, 20.08554, 54.59815, 148.41316]
closed loop  [1.00000, 0.24312, 0.05911, 0.01437, 0.00349, 0.00085]
e^t at t = 5: 148.41316  e^(-sqrt(2) t) at t = 5: 0.00085
```

The open-loop row is `1, 2.718, 7.389, ...` and the closed-loop row `1, 0.243, 0.059, ...`; both match the exponentials, because the simulation is exact for a free response.

## The cost is `x0^T P x0`

To measure the cost along a trajectory, the closed loop is given two outputs: the state `x` and the control `u = -K x`. The program integrates `x^2 + u^2` with the trapezoid rule, for the LQR gain and for four other stabilising gains. For a scalar plant with weights 1 the cost of the gain `K` is `(1 + K^2) / (2 (K - 1))`, which has its minimum, `P = 2.4142`, at `K = 1 + sqrt(2)`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;
namespace integ = pluss::ma::math::integ;

static pluss::Series scalar(double value) { return pluss::series::make(std::vector<double>{value}).reshape({1, 1}); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series time = pluss::series::arange(0, 3001, f64) * 0.005;
  const pluss::Series silence = pluss::series::zeros({3001, 1}, f64);
  const pluss::Series start = pluss::series::make(std::vector<double>{1.0});
  const double lqr_gain = ctl::care(scalar(1.0), scalar(1.0), scalar(1.0), scalar(1.0)).gain.squeeze().item<double>();

  out.writeln("   K      cost by simulation   (1 + K^2) / (2 (K - 1))");
  for (const double k : {1.5, 2.0, lqr_gain, 3.0, 5.0}) {
    const pluss::Series outputs = pluss::series::make(std::vector<double>{1.0, -k}).reshape({2, 1});
    const auto loop = lti::state(scalar(1.0 - k), scalar(1.0), outputs, pluss::series::zeros({2, 1}, f64));
    const auto run = lti::sim(loop, silence, time, start);
    const pluss::Series x = run.values.select(1, 0);
    const pluss::Series u = run.values.select(1, 1);
    out.writeln(k, integ::trapezoid(x.square() + u.square(), time).item<double>(), (1.0 + k * k) / (2.0 * (k - 1.0)));
  }
}
```

Output

```text
   K      cost by simulation   (1 + K^2) / (2 (K - 1))
1.5000 3.2500 3.2500
2.0000 2.5000 2.5000
2.4142 2.4143 2.4142
3.0000 2.5001 2.5000
5.0000 3.2504 3.2500
```

The simulated costs agree with the formula to within `4e-4` (the trapezoid rule on a step of `0.005`). The row for the LQR gain, `2.4142`, is the smallest: gains both below and above it cost more, and the cost rises symmetrically in this case, `3.25` at `K = 1.5` and at `K = 5`. The minimum equals `P x0^2` with `x0 = 1`, which is the identity at the top of the page.

## The price of control

For the double integrator, `R` trades speed against effort. The program starts at position 1 with zero velocity and uses `Q = I` with `R = 0.01`, `1` and `100`. For each it reports the largest control magnitude, the time after which the position stays within 2 percent of zero, and the accumulated cost `integral (x^T x + R u^2) dt` next to `x0^T P x0`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;
namespace integ = pluss::ma::math::integ;

static pluss::Series matrix(std::vector<double> values, long rows, long columns) {
  return pluss::series::make(values).reshape({rows, columns});
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series a = matrix({0, 1, 0, 0}, 2, 2);
  const pluss::Series b = matrix({0, 1}, 2, 1);
  const pluss::Series q = pluss::series::eye(2, f64);
  const pluss::Series time = pluss::series::arange(0, 8001, f64) * 0.01;
  const pluss::Series silence = pluss::series::zeros({8001, 1}, f64);
  const pluss::Series start = pluss::series::make(std::vector<double>{1.0, 0.0});

  out.writeln("   R      gain K            peak |u|   settling time   cost     x0' P x0");
  for (const double weight : {0.01, 1.0, 100.0}) {
    const auto design = ctl::care(a, b, q, matrix({weight}, 1, 1));
    const pluss::Series outputs = pluss::series::eye(2, f64).unite({-design.gain}, 0);
    const auto loop = lti::state(design.closed_loop, b, outputs, pluss::series::zeros({3, 1}, f64));
    const auto run = lti::sim(loop, silence, time, start);

    const pluss::Series position = run.values.select(1, 0);
    const pluss::Series velocity = run.values.select(1, 1);
    const pluss::Series effort = run.values.select(1, 2);
    const std::vector<double> track = position.items<double>();
    std::size_t settled = 0;
    for (std::size_t i = track.size(); i-- > 0;) {
      if (std::abs(track[i]) > 0.02) {
        settled = i + 1;
        break;
      }
    }
    const double cost = integ::trapezoid(position.square() + velocity.square() + effort.square() * weight, time).item<double>();
    out.writeln(weight, design.gain.reshape({2}).items<double>(), effort.abs().max().item<double>(), time.select(0, static_cast<long>(settled)).item<double>(), cost,
                design.value.select(0, 0).select(0, 0).squeeze().item<double>());
  }
}
```

Output

```text
   R      gain K            peak |u|   settling time   cost     x0' P x0
0.0100 [10.0000, 10.9545] 10.0000 4.0000 1.0956 1.0954
1.0000 [1.0000, 1.7321] 1.0000 4.3500 1.7321 1.7321
100.0000 [0.1000, 0.4583] 0.1000 18.6700 4.5826 4.5826
```

Cheap control (`R = 0.01`) uses a large effort, a peak of `10`, and settles in `4` seconds. Expensive control (`R = 100`) holds the peak to `0.1` and takes much longer, `18.67` seconds, to settle. The cost column agrees with `x0^T P x0` in the last column to the accuracy of the integration (`P[0][0]` is the cost from the start `x0 = (1, 0)`).

## A discrete loop

For the discrete plant `x[k+1] = 1.2 x[k] + u[k]` the cost is a sum, and the minimum from `x0 = 1` is again `P`. The program simulates the closed loop for 60 steps and sums `x^2 + u^2`. The closed-loop pole is `0.406`, so the state decays by that factor per step and the tail beyond 60 steps is negligible.

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
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const auto design = ctl::dare(scalar(1.2), scalar(1.0), scalar(1.0), scalar(1.0));
  const double k = design.gain.squeeze().item<double>();

  const pluss::Series outputs = pluss::series::make(std::vector<double>{1.0, -k}).reshape({2, 1});
  const auto loop = lti::state(design.closed_loop, scalar(1.0), outputs, pluss::series::zeros({2, 1}, f64), 1.0);
  const pluss::Series steps = pluss::series::arange(0, 60, f64);
  const auto run = lti::sim(loop, pluss::series::zeros({60, 1}, f64), steps, pluss::series::make(std::vector<double>{1.0}));
  const pluss::Series x = run.values.select(1, 0);
  const pluss::Series u = run.values.select(1, 1);
  out.writeln("state, first five steps", x.slice(0, 0, 5).items<double>());
  out.writeln("sum of x^2 + u^2 over 60 steps:", (x.square() + u.square()).sum().item<double>(), " P =", design.value.squeeze().item<double>());
}
```

Output

```text
state, first five steps [1.000000, 0.406472, 0.165219, 0.067157, 0.027297]
sum of x^2 + u^2 over 60 steps: 1.952234  P = 1.952234
```

The state is `0.406^k`, and the sum of the stage costs equals `P = 1.952234` to the printed digits.

## Decisions

- **Judging a design:** simulate it. Look at the state, at the control (is its peak within what the actuator can deliver?) and at the cost.
- **Tuning:** move `R` to trade speed against effort; move entries of `Q` to make particular states stiffer. Re-run the simulation after each change.
- **Comparing with an alternative law:** compute the same cost along its trajectory; the LQR gain must come out smallest for the same `Q` and `R`.

## Pitfalls

The program makes four mistakes on the double integrator `x'' = u` started at position `1`: judging a design by its look rather than by its weights, ignoring the control at the first instant, measuring a cost over too short a horizon, and implementing a discrete design on a continuous plant.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;
namespace ctl = pluss::subsp::eng::control;
namespace integ = pluss::ma::math::integ;

static pluss::Series matrix(std::vector<double> values, long rows, long columns) {
  return pluss::series::make(values).reshape({rows, columns});
}
static pluss::Series scalar(double value) { return matrix({value}, 1, 1); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const auto start = pluss::series::make(std::vector<double>{1.0, 0.0});

  // the double integrator x'' = u
  const auto a = matrix({0, 1, 0, 0}, 2, 2), b = matrix({0, 1}, 2, 1);
  const auto q = pluss::series::eye(2, f64);
  // the closed loop with the states and the control u = -K x as its three outputs
  const auto loop = [&](const pluss::Series &gain) {
    const auto k = gain.reshape({2}).items<double>();
    const auto outputs = matrix({1, 0, 0, 1, -k[0], -k[1]}, 3, 2);
    return lti::state(a - b.matmul(gain), b, outputs, pluss::series::zeros({3, 1}, f64));
  };
  const auto run = [&](const pluss::Series &gain, double horizon, long steps) {
    const auto time = pluss::series::arange(0, steps + 1, f64) * (horizon / steps);
    const auto result = lti::sim(loop(gain), pluss::series::zeros({steps + 1, 1}, f64), time, start);
    return std::make_pair(time, result.values);
  };
  const auto cost = [&](const pluss::Series &gain, double r, double horizon, long steps) {
    const auto [time, y] = run(gain, horizon, steps);
    const auto x1 = y.select(1, 0), x2 = y.select(1, 1), u = y.select(1, 2);
    return integ::trapezoid(x1.square() + x2.square() + r * u.square(), time).item<double>();
  };

  // 1. the design is optimal for the weights given: with R = 100 the loop is slow, and for R = 100 it is still the cheapest
  const auto cheap = ctl::care(a, b, q, scalar(100.0));
  const auto fast = ctl::care(a, b, q, scalar(0.01));
  out.writeln("R = 100 design: K = ", cheap.gain.reshape({2}).items<double>(), ", cost with R = 100: ", cost(cheap.gain, 100.0, 120.0, 12000), "   the fast gain's cost with R = 100: ", cost(fast.gain, 100.0, 120.0, 12000));

  // 2. the peak control is at the start, and nothing limits it
  const auto [t_fast, y_fast] = run(fast.gain, 10.0, 2000);
  out.writeln("R = 0.01: K = ", fast.gain.reshape({2}).items<double>(), ", peak |u| = ", y_fast.select(1, 2).abs().max().item<double>(), " at the first sample, against an actuator limit of 5");

  // 3. a short horizon misses the tail of a slow loop
  const double exact = (cheap.value.matmul(start.reshape({2, 1})) * start.reshape({2, 1})).sum().item<double>();
  out.writeln("x0' P x0 = ", exact, ";  cost over 5 s: ", cost(cheap.gain, 100.0, 5.0, 1000), "   over 30 s: ", cost(cheap.gain, 100.0, 30.0, 6000), "   over 120 s: ", cost(cheap.gain, 100.0, 120.0, 12000));

  // 4. a gain designed for the discrete model is not the continuous gain
  const double h = 0.5;
  const auto ad = matrix({1, h, 0, 1}, 2, 2), bd = matrix({h * h / 2, h}, 2, 1);
  const auto discrete_gain = ctl::dare(ad, bd, q, scalar(1.0)).gain;
  const auto continuous_gain = ctl::care(a, b, q, scalar(1.0)).gain;
  out.writeln("continuous design K = ", continuous_gain.reshape({2}).items<double>(), "   discrete design K = ", discrete_gain.reshape({2}).items<double>());
  out.writeln("continuous plant, continuous gain: cost ", cost(continuous_gain, 1.0, 30.0, 6000), "   continuous plant, discrete gain: cost ", cost(discrete_gain, 1.0, 30.0, 6000));
}
```

Output

```text
R = 100 design: K = [0.1000, 0.4583], cost with R = 100: 4.5826   the fast gain's cost with R = 100: 459.3097
R = 0.01: K = [10.0000, 10.9545], peak |u| = 10.0000 at the first sample, against an actuator limit of 5
x0' P x0 = 4.5826;  cost over 5 s: 4.0617   over 30 s: 4.5826   over 120 s: 4.5826
continuous design K = [1.0000, 1.7321]   discrete design K = [0.6514, 1.3142]
continuous plant, continuous gain: cost 1.7321   continuous plant, discrete gain: cost 1.7985
```

A design with `R = 100` is slow, and it is optimal for that weight: its cost, `4.5826`, is `x0^T P x0`, while the fast design's gain costs `459.3` when the control is priced at `R = 100`. If the response is too slow, the weights are wrong, not the solver; change `R`. At the other end, `R = 0.01` gives a gain of `[10, 10.95]` and a first control of `10`, twice an actuator limit of `5`: an LQR design has no saturation, so the peak control is the first thing to check. The cost of the slow loop needs a long horizon: over `5` seconds the integral is `4.0617`, over `30` it has reached `4.5826`, the value of `x0^T P x0`, and `120` adds nothing. Finally, a gain from `dare` for a model sampled at `0.5` is `[0.6514, 1.3142]`, not the continuous `[1, 1.7321]`; applied to the continuous plant it costs `1.7985` against `1.7321`, and a design must be simulated in the loop it will run in.

Reference: [`care`](../../../../api/eng/control/optimal/care.md), [`state`](../../../../api/eng/lti/state/state.md), [`sim`](../../../../api/eng/lti/simulation/sim.md), [`dare`](../../../../api/eng/control/optimal/dare.md).

Where next: the [control reference](../../../../api/eng/control.md) lists every call used here.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="riccati_equations_and_lqr.html">
    <span class="label">Previous</span>
    <span class="title">Riccati equations and LQR</span>
  </a>
</div>
<!-- apiref:nav:end -->
