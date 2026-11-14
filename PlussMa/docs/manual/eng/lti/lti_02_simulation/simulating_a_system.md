<!-- generated from simulating_a_system.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Simulating a system

[Back to Systems 2: Simulation](../lti_02_simulation.md)

You have a state-space model and an input signal, and you want the output and the states over time. `lti::sim` computes them. It does not step an approximate integrator: for a continuous model it uses the **exact** solution over each time interval for the input you describe, so the error is zero for inputs that follow the chosen holding rule, whatever the step size. This page shows the holds, discrete and continuous models, initial states, batches of inputs and systems, and the checks that protect you from a mismatched grid.

## The mental model

`sim(system, input, time)` takes an input of shape `[..., T, inputs]` sampled at the `T` instants in `time` (a strictly increasing vector) and returns a `SimRun` with two members: `values`, the outputs `[..., T, outputs]`, and `states`, the state history `[..., T, states]`. Row `k` of each is the value at `time[k]`; the first row is the initial condition.

Between samples the input has to be described somehow. That is the **hold**:

| `Hold` | The input between `t_k` and `t_{k+1}` | Exact for |
|---|---|---|
| `Zero` | constant at the value `u_k` | piecewise-constant inputs (steps, a digital-to-analogue converter) |
| `Linear` | a straight line from `u_k` to `u_{k+1}` | piecewise-linear inputs (ramps, sampled smooth signals) |
| `Auto` (default) | `Linear` for a continuous model, `Zero` for a discrete one | |

Over one interval of length `h` the state moves by `x_{k+1} = A_d x_k + (input terms)`, where `A_d = e^{A h}` and the input terms come from the same matrix exponential, so the update is the solution of the differential equation, not a numerical approximation. On a uniform time grid the exponential is computed once and reused; on a non-uniform grid it is computed for every interval, in one batched call.

A discrete model has no between-samples: the time vector must be uniform with increments equal to the model's sample time, and the state update is the difference equation.

## A step response

The first-order lag `x' = -x + u`, `y = x` responds to a unit step with `y(t) = 1 - e^{-t}`. The program simulates it at half-second intervals and prints the exact solution next to the simulated one. The step is piecewise-constant, so the result is exact to rounding, not to a step-size error.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;

static pluss::Series scalar(double value) { return pluss::series::make(std::vector<double>{value}).reshape({1, 1}); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const auto lag = lti::state(scalar(-1.0), scalar(1.0), scalar(1.0), scalar(0.0));
  const pluss::Series time = pluss::series::arange(0, 6, f64) * 0.5;
  const pluss::Series step = pluss::series::ones({6, 1}, f64);

  const auto run = lti::sim(lag, step, time);
  std::vector<double> exact;
  for (const double t : time.items<double>()) exact.push_back(1.0 - std::exp(-t));
  out.writeln("time     ", time.items<double>());
  out.writeln("simulated", run.values.reshape({6}).items<double>());
  out.writeln("1 - e^-t ", exact);
  out.writeln("shapes: values", run.values.sizes(), " states", run.states.sizes());
}
```

Output

```text
time      [0.000000, 0.500000, 1.000000, 1.500000, 2.000000, 2.500000]
simulated [0.000000, 0.393469, 0.632121, 0.776870, 0.864665, 0.917915]
1 - e^-t  [0.000000, 0.393469, 0.632121, 0.776870, 0.864665, 0.917915]
shapes: values [6, 1]  states [6, 1]
```

The two rows agree to every printed digit. Both histories have a row for every time instant; with one output and one state they have the same numbers, and `y` is read from the state because `C = 1`.

## Choosing the hold

The input `u(t) = t` is a ramp, and the exact response of the lag to it is `y(t) = t - 1 + e^{-t}`. With `Hold::Linear` (the default for continuous models) the samples of the ramp are joined by straight lines, which reproduces the ramp exactly, and so does the simulation. With `Hold::Zero` the same samples are held constant until the next one, which is a staircase that lags the ramp, and the response is different.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;

static pluss::Series scalar(double value) { return pluss::series::make(std::vector<double>{value}).reshape({1, 1}); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const auto lag = lti::state(scalar(-1.0), scalar(1.0), scalar(1.0), scalar(0.0));
  const pluss::Series time = pluss::series::arange(0, 6, f64) * 0.5;
  const pluss::Series ramp = time.unsqueeze(-1);

  std::vector<double> exact;
  for (const double t : time.items<double>()) exact.push_back(t - 1.0 + std::exp(-t));
  const auto linear = lti::sim(lag, ramp, time, lti::SimOptions{.hold = lti::Hold::Linear});
  const auto zero = lti::sim(lag, ramp, time, lti::SimOptions{.hold = lti::Hold::Zero});
  const auto automatic = lti::sim(lag, ramp, time);

  out.writeln("exact       ", exact);
  out.writeln("Hold::Linear", linear.values.reshape({6}).items<double>());
  out.writeln("Hold::Zero  ", zero.values.reshape({6}).items<double>());
  out.writeln("Auto against Linear, largest difference:", (automatic.values - linear.values).abs().max().item<double>());
}
```

Output

```text
exact        [0.000000, 0.106531, 0.367879, 0.723130, 1.135335, 1.582085]
Hold::Linear [0.000000, 0.106531, 0.367879, 0.723130, 1.135335, 1.582085]
Hold::Zero   [0.000000, 0.000000, 0.196735, 0.512795, 0.901230, 1.333562]
Auto against Linear, largest difference: 0.000000
```

`Linear` matches the exact response in every digit. `Zero` is low by an amount that grows with the ramp: the staircase input is, on average, half a step behind the ramp. Use `Zero` when the input really is piecewise constant, for example the output of a sample-and-hold; use `Linear` when the samples come from a smooth signal.

## Starting from an initial state

The default starting state is zero. Pass an `initial` vector of length `states` (or `[..., states]`) to start elsewhere. With zero input the response from `x0 = 2` of the lag is `2 e^{-t}`, the free response.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;

static pluss::Series scalar(double value) { return pluss::series::make(std::vector<double>{value}).reshape({1, 1}); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const auto lag = lti::state(scalar(-1.0), scalar(1.0), scalar(1.0), scalar(0.0));
  const pluss::Series time = pluss::series::arange(0, 6, f64) * 0.5;
  const pluss::Series silence = pluss::series::zeros({6, 1}, f64);

  const auto run = lti::sim(lag, silence, time, pluss::series::make(std::vector<double>{2.0}));
  std::vector<double> exact;
  for (const double t : time.items<double>()) exact.push_back(2.0 * std::exp(-t));
  out.writeln("free response", run.values.reshape({6}).items<double>());
  out.writeln("2 e^-t       ", exact);
}
```

Output

```text
free response [2.000000, 1.213061, 0.735759, 0.446260, 0.270671, 0.164170]
2 e^-t        [2.000000, 1.213061, 0.735759, 0.446260, 0.270671, 0.164170]
```

An oscillator `x'' = -4x` started at `x = 1`, `x' = 0` keeps swinging as `cos(2 t)`. The program shows an undamped system, which has no decay and so tests the integration directly: the exact solution is reproduced, with no numerical damping.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;

static pluss::Series matrix(std::vector<double> values, long rows, long columns) {
  return pluss::series::make(values).reshape({rows, columns});
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const auto oscillator = lti::state(matrix({0, 1, -4, 0}, 2, 2), matrix({0, 1}, 2, 1), matrix({1, 0}, 1, 2), matrix({0}, 1, 1));
  const pluss::Series time = pluss::series::arange(0, 5, f64) * 0.25;
  const auto run = lti::sim(oscillator, pluss::series::zeros({5, 1}, f64), time, pluss::series::make(std::vector<double>{1.0, 0.0}));
  std::vector<double> exact;
  for (const double t : time.items<double>()) exact.push_back(std::cos(2.0 * t));
  out.writeln("position", run.values.reshape({5}).items<double>());
  out.writeln("cos(2 t)", exact);
}
```

Output

```text
position [1.000000, 0.877583, 0.540302, 0.070737, -0.416147]
cos(2 t) [1.000000, 0.877583, 0.540302, 0.070737, -0.416147]
```

## The direct term

The output is `C x + D u`, so `D` acts instantly. For the lag with `D = 0.5` the output at time zero under a unit step is already `0.5`, before the state has moved.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;

static pluss::Series scalar(double value) { return pluss::series::make(std::vector<double>{value}).reshape({1, 1}); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series time = pluss::series::arange(0, 4, f64) * 0.5;
  const pluss::Series step = pluss::series::ones({4, 1}, f64);
  const auto plain = lti::state(scalar(-1.0), scalar(1.0), scalar(1.0), scalar(0.0));
  const auto direct = lti::state(scalar(-1.0), scalar(1.0), scalar(1.0), scalar(0.5));
  out.writeln("D = 0  ", lti::sim(plain, step, time).values.reshape({4}).items<double>());
  out.writeln("D = 0.5", lti::sim(direct, step, time).values.reshape({4}).items<double>());
}
```

Output

```text
D = 0   [0.000000, 0.393469, 0.632121, 0.776870]
D = 0.5 [0.500000, 0.893469, 1.132121, 1.276870]
```

The second response is the first plus `0.5` throughout: the direct term adds `D u` to the output without touching the states.

## A discrete model

For a discrete model `time` must be the sample instants, spaced exactly `sample_time` apart, and the update is the difference equation. The system `x[k+1] = 0.5 x[k] + u[k]`, `y = x` responds to a unit step with `y[k] = 2 (1 - 0.5^k)`, which tends to `2`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;

static pluss::Series scalar(double value) { return pluss::series::make(std::vector<double>{value}).reshape({1, 1}); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const auto system = lti::state(scalar(0.5), scalar(1.0), scalar(1.0), scalar(0.0), 1.0);
  const pluss::Series k = pluss::series::arange(0, 6, f64);
  const auto run = lti::sim(system, pluss::series::ones({6, 1}, f64), k);
  out.writeln("y[k]           ", run.values.reshape({6}).items<double>());
  out.writeln("2 (1 - 0.5^k)  ", std::vector<double>{0, 1, 1.5, 1.75, 1.875, 1.9375});
}
```

Output

```text
y[k]            [0.000000, 1.000000, 1.500000, 1.750000, 1.875000, 1.937500]
2 (1 - 0.5^k)   [0.000000, 1.000000, 1.500000, 1.750000, 1.875000, 1.937500]
```

The output at `k = 0` is zero because `y = x` and the state starts at zero: the input at step `k` affects the state at step `k + 1`. A model with a direct term `D` would respond at `k = 0`.

## Batches of inputs and systems

Extra leading axes of the input are batch axes. The program drives the lag with three step inputs of amplitude 1, 2 and 3: the outputs scale in proportion, because the system is linear. A batch of systems works the same way (see [State-space models](../lti_01_models_and_analysis/state_space_models.md)), and an `initial` state can carry the batch axes too.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;

static pluss::Series scalar(double value) { return pluss::series::make(std::vector<double>{value}).reshape({1, 1}); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const auto lag = lti::state(scalar(-1.0), scalar(1.0), scalar(1.0), scalar(0.0));
  const pluss::Series time = pluss::series::arange(0, 6, f64) * 0.5;
  const pluss::Series amplitudes = pluss::series::make(std::vector<double>{1, 2, 3}).reshape({3, 1, 1});
  const pluss::Series inputs = pluss::series::ones({3, 6, 1}, f64) * amplitudes;

  const auto run = lti::sim(lag, inputs, time);
  out.writeln("values shape", run.values.sizes(), " states shape", run.states.sizes());
  out.writeln("final outputs", run.values.select(1, 5).reshape({3}).items<double>());
}
```

Output

```text
values shape [3, 6, 1]  states shape [3, 6, 1]
final outputs [0.917915, 1.835830, 2.753745]
```

The final outputs are `0.917915`, `1.835830` and `2.753745`: the first is `1 - e^{-2.5}`, and the others are twice and three times it.

## Long runs

The step-by-step update runs as a tight loop, and on a uniform time grid the matrix exponential is computed once, so a simulation of ten thousand steps costs the same as one of a hundred plus the loop. The program simulates the spring for 10 000 steps and checks the final value against the DC gain: `1/2`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;

static pluss::Series matrix(std::vector<double> values, long rows, long columns) {
  return pluss::series::make(values).reshape({rows, columns});
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const auto spring = lti::state(matrix({0, 1, -2, -3}, 2, 2), matrix({0, 1}, 2, 1), matrix({1, 0}, 1, 2), matrix({0}, 1, 1));
  const pluss::Series time = pluss::series::arange(0, 10000, f64) * 0.01;
  const auto run = lti::sim(spring, pluss::series::ones({10000, 1}, f64), time);
  out.writeln("samples", run.values.size(0), " final position", run.values.select(0, 9999).squeeze().item<double>(), " DC gain 1/2");
}
```

Output

```text
samples 10000  final position 0.500000  DC gain 1/2
```

## Errors

The time vector and the grid are validated before anything runs. The program shows time values that are not increasing, an input whose length does not match the time vector, an input of the wrong width, a discrete model on a grid that does not match its sample time, an unsupported hold for a discrete model, an initial state of the wrong length, an empty time vector, and an input without a time axis.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;

static pluss::Series scalar(double value) { return pluss::series::make(std::vector<double>{value}).reshape({1, 1}); }

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
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const auto lag = lti::state(scalar(-1.0), scalar(1.0), scalar(1.0), scalar(0.0));
  const auto discrete = lti::state(scalar(0.5), scalar(1.0), scalar(1.0), scalar(0.0), 1.0);
  const pluss::Series time = pluss::series::arange(0, 6, f64) * 0.5;
  const pluss::Series step = pluss::series::ones({6, 1}, f64);
  const pluss::Series k = pluss::series::arange(0, 6, f64);

  report([&] { (void)lti::sim(lag, step, pluss::series::make(std::vector<double>{0, 0.5, 0.5, 1, 1.5, 2})); });
  report([&] { (void)lti::sim(lag, pluss::series::ones({5, 1}, f64), time); });
  report([&] { (void)lti::sim(lag, pluss::series::ones({6, 2}, f64), time); });
  report([&] { (void)lti::sim(discrete, step, time); });
  report([&] { (void)lti::sim(discrete, step, k, lti::SimOptions{.hold = lti::Hold::Linear}); });
  report([&] { (void)lti::sim(lag, step, time, pluss::series::make(std::vector<double>{1.0, 2.0})); });
  report([&] { (void)lti::sim(lag, step, pluss::series::zeros({0}, f64)); });
  report([&] { (void)lti::sim(lag, pluss::series::ones({6}, f64), time); });
}
```

Output

```text
eng::lti::sim: time values must be strictly increasing
eng::lti::sim: input and time lengths must match
eng::lti::sim: input width must equal the system input count
eng::lti::sim: discrete time increments must equal sample_time
eng::lti::sim: discrete simulation only supports zero-order sample holding
eng::lti::sim: initial state must have shape [..., states]
eng::lti::sim: time must be a non-empty vector
eng::lti::sim: input must have shape [..., time, inputs]
```

## Decisions

- **Sampled data from a smooth signal:** the default, `Auto` (`Linear`). **A held command, a digital controller's output:** `Hold::Zero`.
- **Comparing with an analytic solution:** build the input so the hold is exact (steps with `Zero`, ramps with `Linear`) and the match will be to rounding.
- **A discrete controller closed around a continuous plant:** simulate on a fine uniform grid with `Zero` hold, with the controller's sample times a multiple of the grid step.

## Pitfalls

The program makes four mistakes with a simulation: reading the first row as the response to the first input, giving the wrong number of input rows, sampling a discrete model on the wrong grid, and distrusting a grid that is not uniform.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace lti = pluss::subsp::eng::lti;

static pluss::Series matrix(std::vector<double> values, long rows, long columns) {
  return pluss::series::make(values).reshape({rows, columns});
}
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const auto report = [&](const char *label, const auto &call) {
    try {
      call();
    } catch (const std::exception &e) {
      std::string text(e.what());
      if (const auto cut = text.find("Triggered"); cut != std::string::npos) text.erase(cut);
      while (!text.empty() && text.back() == '\n') text.pop_back();
      out.writeln(std::string(label) + ": " + text);
    }
  };

  // a lag with a direct path: y = x + 0.5 u
  const auto lag = lti::state(matrix({-1.0}, 1, 1), matrix({1.0}, 1, 1), matrix({1.0}, 1, 1), matrix({0.5}, 1, 1));
  const auto time = pluss::series::arange(0, 4, f64) * 0.5;   // 0, 0.5, 1, 1.5
  const auto step = pluss::series::ones({4, 1}, f64);

  // 1. the first row is the initial condition, not the response to the first input
  const auto from_rest = lti::sim(lag, step, time);
  out.writeln("response to a unit step from rest: ", flat(from_rest.values), "   the first row is C x0 + D u0 = 0.5, with x0 = 0");
  const auto driven = lti::sim(lag, step, time, pluss::series::make(std::vector<double>{2.0}));
  out.writeln("from x0 = 2 with the same step:    ", flat(driven.values), "   the first row is 2 + 0.5");

  // 2. the input needs one row per time value
  report("five input rows for four times", [&] { (void)lti::sim(lag, pluss::series::ones({5, 1}, f64), time); });

  // 3. a discrete model needs the exact sample time as the step
  const auto discrete = lti::state(matrix({0.5}, 1, 1), matrix({1.0}, 1, 1), matrix({1.0}, 1, 1), matrix({0.0}, 1, 1), 1.0);
  report("a discrete model sampled at 1 on a grid of 0.5", [&] { (void)lti::sim(discrete, step, time); });
  const auto k = pluss::series::arange(0, 4, f64);
  out.writeln("on a grid of 1:                                  ", flat(lti::sim(discrete, step, k).values));

  // 4. a non-uniform grid is allowed for a continuous model
  const auto uneven = pluss::series::make(std::vector<double>{0.0, 0.1, 0.5, 2.0});
  out.writeln("uneven grid:                                     ", flat(lti::sim(lag, step, uneven).values), "   exact: ", std::vector<double>{0.5, 1.0 - std::exp(-0.1) + 0.5, 1.0 - std::exp(-0.5) + 0.5, 1.0 - std::exp(-2.0) + 0.5});
}
```

Output

```text
response to a unit step from rest: [0.5000, 0.8935, 1.1321, 1.2769]   the first row is C x0 + D u0 = 0.5, with x0 = 0
from x0 = 2 with the same step:    [2.5000, 2.1065, 1.8679, 1.7231]   the first row is 2 + 0.5
five input rows for four times: eng::lti::sim: input and time lengths must match
a discrete model sampled at 1 on a grid of 0.5: eng::lti::sim: discrete time increments must equal sample_time
on a grid of 1:                                  [0.0000, 1.0000, 1.5000, 1.7500]
uneven grid:                                     [0.5000, 0.5952, 0.8935, 1.3647]   exact: [0.5000, 0.5952, 0.8935, 1.3647]
```

The first row of the result is the output at the first time, `C x0 + D u0`: from rest it is the direct term `0.5`, and from `x0 = 2` it is `2.5`; it is not the response to the first input sample. The input needs one row per time value, and a mismatch is refused with the two lengths named. A discrete model needs its own sample time as the step: a grid of `0.5` for a model sampled at `1` is refused, and on a grid of `1` it steps as `x[k+1] = 0.5 x[k] + u[k]`. A grid that is not uniform is allowed for a continuous model, at the cost of a matrix exponential per distinct interval, and gives the exact response: `0.5952`, `0.8935` and `1.3647` match `1 - e^-t + 0.5` at `0.1`, `0.5` and `2`.

Reference: [`state`](../../../../api/eng/lti/state/state.md), [`sim`](../../../../api/eng/lti/simulation/sim.md), [`SimOptions`](../../../../api/eng/lti/simulation/SimOptions.md), [`Hold`](../../../../api/eng/lti/simulation/Hold.md).

Where next: the [linear systems reference](../../../../api/eng/lti.md) lists every call used in these chapters.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
