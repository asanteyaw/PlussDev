<!-- generated from schemes_and_step_size.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Schemes and step size

[Back to Circuits 2: Transient Response](../circuit_02_transient.md)

A transient run is a series of straight steps along a curve, so its accuracy depends on how long the steps are and on the rule used to take them. `tran` offers two rules, selected by `TranOptions::scheme`, and this page shows what each one does to accuracy, to an oscillation that should not lose energy, and to a circuit whose step is long compared with its fastest time constant. It ends with a rule of thumb for choosing the step and the scheme.

## The mental model

Both schemes are **second order**: halving the step divides the error of a smooth response by about four. They differ in how they treat the part of the response that changes faster than the step can follow.

- **`Scheme::Trapezoid`** averages the equations at the two ends of the step. It is exact for a straight-line response, it conserves the energy of a lossless oscillation, and it never adds or removes damping. Its weakness is a step much longer than a fast time constant: the fast part of the response then flips sign from step to step and decays only slowly.
- **`Scheme::Gear2`** extends the equations backwards over the two previous steps, starting with one backward-Euler step. The price is **numerical damping**: a sustained oscillation loses amplitude, more of it the fewer samples there are per period.

Neither is the better scheme in general; the choice depends on whether the circuit has an oscillation you must keep or a stiff part you must not let ring.

## Accuracy against the step

The RC step response of [Step response and starting state](step_response.md) has the closed form `5 (1 - exp(-t / tau))`, so the largest error over the run can be measured exactly. The program runs `5 ms` with the step halved from `0.4 ms` down to `0.05 ms` (that is, from `tau / 2.5` to `tau / 20`), and prints the largest error of each scheme and the ratio between successive errors.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  const auto circuit = ckt::make(net);

  const auto worst_error = [&](ckt::Scheme scheme, double step) {
    std::vector<double> time;
    for (int k = 0; k * step <= 5e-3 * (1.0 + 1e-9); ++k) time.push_back(k * step);
    ckt::TranOptions options;
    options.scheme = scheme;
    const std::vector<double> v = flat(ckt::tran(circuit, vec(time), {}, {}, options).voltage.select(1, 2));
    double worst = 0.0;
    for (std::size_t k = 0; k < time.size(); ++k) worst = std::max(worst, std::abs(v[k] - 5.0 * (1.0 - std::exp(-time[k] / 1e-3))));
    return worst;
  };

  out.writeln("  step (ms)   trapezoid error   ratio     gear2 error   ratio");
  double previous_trap = 0.0, previous_gear = 0.0;
  for (double step : {4e-4, 2e-4, 1e-4, 5e-5}) {
    const double trap = worst_error(ckt::Scheme::Trapezoid, step), gear = worst_error(ckt::Scheme::Gear2, step);
    out.writeln(step * 1e3, trap, previous_trap > 0 ? previous_trap / trap : 0.0, gear, previous_gear > 0 ? previous_gear / gear : 0.0);
    previous_trap = trap;
    previous_gear = gear;
  }
}
```

Output

```text
  step (ms)   trapezoid error   ratio     gear2 error   ratio
0.400000 0.024490 0.000000 0.219828 0.000000
0.200000 0.006158 3.976843 0.079772 2.755697
0.100000 0.001534 4.013079 0.025664 3.108285
0.050000 0.000383 4.003255 0.007446 3.446878
```

The trapezoid error falls by `4.0` for every halving of the step (the ratio column), which is second order exactly. The Gear2 error falls by `2.8`, `3.1` and `3.4`, climbing towards `4` as the step shrinks: it starts with one first-order backward-Euler step, and the error of that step is still visible while the step is long. The trapezoid rule is also the more accurate of the two on this response, by a factor of about `20` at the finest step.

## An oscillation that must keep its energy

A `1 mH` inductor and a `1 microfarad` capacitor with no resistance oscillate forever at `w0 = 31 623 rad/s`, period `T = 198.7 microseconds`; started with the capacitor at `1 V` they hold the energy `C v^2 / 2 = 0.5 microjoule` for ever, exchanging it between the capacitor and the inductor. The energy stored at an instant is `C v^2 / 2 + L i^2 / 2`, which `tran` lets you compute from the capacitor voltage and the inductor current. The program runs four periods at `20`, `40` and `200` samples per period and prints the fraction of the initial energy left at the end. A perfect run would print `1`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const double inductance = 1e-3, capacitance = 1e-6;
  ckt::Net net;
  net.nodes = 2;
  net.inductors = {{1}, {0}, vec({inductance})};
  net.capacitors = {{1}, {0}, vec({capacitance})};
  const auto circuit = ckt::make(net);
  ckt::Start start;
  start.capacitor_voltage = vec({1.0});
  const double period = 2.0 * M_PI * std::sqrt(inductance * capacitance);
  const double initial_energy = 0.5 * capacitance;

  out.writeln("samples per period   trapezoid   gear2   (energy left after four periods)");
  for (int per : {20, 40, 200}) {
    std::vector<double> time;
    for (int k = 0; k <= 4 * per; ++k) time.push_back(k * period / per);
    double left[2];
    int i = 0;
    for (auto scheme : {ckt::Scheme::Trapezoid, ckt::Scheme::Gear2}) {
      ckt::TranOptions options;
      options.scheme = scheme;
      const auto run = ckt::tran(circuit, vec(time), {}, start, options);
      const double v = flat(run.voltage.select(1, 1)).back(), current = flat(run.inductor_current).back();
      left[i++] = (0.5 * capacitance * v * v + 0.5 * inductance * current * current) / initial_energy;
    }
    out.writeln(per, left[0], left[1]);
  }
}
```

Output

```text
samples per period   trapezoid   gear2   (energy left after four periods)
20 1.000000 0.644587
40 1.000000 0.921931
200 1.000000 0.998135
```

The trapezoid rule holds the energy to rounding error at every step size: what a coarse step costs it is *phase*, since each period comes out a little longer than the true one, never energy. Gear2 loses energy: more than a third of it over four periods at `20` samples per period (`0.645` left), about `8 %` at `40`, and `0.2 %` at `200`. A lossless circuit that decays in a Gear2 run has not lost energy to anything physical.

## A step longer than the time constant

The trapezoid rule has one more property, and it only shows when the step is long compared with the fastest time constant of the circuit. The RC low-pass cannot overshoot its source: a capacitor charged through a resistor from a `5 V` source never exceeds `5 V`. The program runs the step response with steps of `0.5`, `2`, `4` and `10` time constants and prints the largest voltage each scheme produces over `20` steps.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  const auto circuit = ckt::make(net);

  out.writeln("step / tau   trapezoid max   gear2 max   (the source is 5 V)");
  for (double ratio : {0.5, 2.0, 4.0, 10.0}) {
    std::vector<double> time;
    for (int k = 0; k <= 20; ++k) time.push_back(k * ratio * 1e-3);
    double largest[2];
    int i = 0;
    for (auto scheme : {ckt::Scheme::Trapezoid, ckt::Scheme::Gear2}) {
      ckt::TranOptions options;
      options.scheme = scheme;
      const std::vector<double> v = flat(ckt::tran(circuit, vec(time), {}, {}, options).voltage.select(1, 2));
      largest[i++] = *std::max_element(v.begin(), v.end());
    }
    out.writeln(ratio, largest[0], largest[1]);
  }

  // the first samples at ten time constants per step
  std::vector<double> time;
  for (int k = 0; k <= 8; ++k) time.push_back(k * 1e-2);
  ckt::TranOptions gear;
  gear.scheme = ckt::Scheme::Gear2;
  out.writeln("trapezoid, step = 10 tau", flat(ckt::tran(circuit, vec(time)).voltage.select(1, 2)));
  out.writeln("gear2,     step = 10 tau", flat(ckt::tran(circuit, vec(time), {}, {}, gear).voltage.select(1, 2)));
}
```

Output

```text
step / tau   trapezoid max   gear2 max   (the source is 5 V)
0.50000 4.99982 4.99996
2.00000 5.00000 5.10204
4.00000 6.66667 5.12397
10.00000 8.33333 5.13834
trapezoid, step = 10 tau [0.00000, 8.33333, 2.77778, 6.48148, 4.01235, 5.65844, 4.56104, 5.29264, 4.80491]
gear2,     step = 10 tau [0.00000, 4.54545, 5.13834, 5.04382, 5.00161, 4.99837, 4.99965, 5.00001, 5.00002]
```

With a step up to two time constants the trapezoid rule stays at or below the source voltage, as the circuit does. Beyond two it **overshoots**, by `67 %` at four time constants per step and `167 %` at ten, and the first samples at ten time constants per step show the ringing: the output swings above and below `5 V`, each swing `2/3` of the previous one with the opposite sign (`8.33`, `2.78`, `6.48`, `4.01`, ...), and settles only slowly. Gear2 overshoots too, but by at most `0.14 V` (under `3 %`), and the overshoot is gone within a few steps (`5.138`, `5.044`, `5.0016`, `4.998`, ...). A passive RC circuit has no way to overshoot, so overshoot here is always the integrator's, never the circuit's.

## Choosing the scheme and the step

- **The slowest part of the response sets the run length;** the fastest part you need to *see* sets the step. Twenty samples per time constant gave a trapezoid error of a few parts in ten thousand above.
- **An oscillator, a resonant filter, anything whose energy should persist:** `Trapezoid` with at least twenty samples per period. Check the stored energy of a lossless case against what it should be.
- **A circuit with widely separated time constants** (a fast stage feeding a slow one, a stiff switching network): `Gear2`, so the fast part dies within a few steps instead of ringing. Accuracy on the slow part is then set by the step relative to the slow time constant.
- **A source with edges:** either scheme works; see the notes on edge samples in [Time-varying sources](time_varying_sources.md).
- **Not sure:** run both. When they agree to the accuracy you need, either is right; when they differ, the one that rings or decays has told you the step is too long.

## Pitfalls

The program makes the two mistakes the previous sections explain, on one step that is too long for the circuit it is applied to. It reads an overshoot above the source as a property of the RC circuit, and reads the energy loss of an LC oscillator as dissipation. Each is compared with what the circuit must do.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));

  // 1. a passive RC circuit cannot exceed its 5 V source; the trapezoid rule at 4 tau per step does
  ckt::Net rc;
  rc.nodes = 3;
  rc.voltage_sources = {{1}, {0}, vec({5.0})};
  rc.resistors = {{1}, {2}, vec({1000.0})};
  rc.capacitors = {{2}, {0}, vec({1e-6})};
  std::vector<double> coarse, fine;
  for (int k = 0; k <= 20; ++k) coarse.push_back(k * 4e-3);
  for (int k = 0; k <= 400; ++k) fine.push_back(k * 2e-4);
  const std::vector<double> coarse_v = flat(ckt::tran(ckt::make(rc), vec(coarse)).voltage.select(1, 2));
  const std::vector<double> fine_v = flat(ckt::tran(ckt::make(rc), vec(fine)).voltage.select(1, 2));
  out.writeln("largest RC output, step = 4 tau", *std::max_element(coarse_v.begin(), coarse_v.end()), " step = tau / 5", *std::max_element(fine_v.begin(), fine_v.end()));

  // 2. a lossless LC cannot lose energy; Gear2 at 20 samples per period shows a loss
  ckt::Net lc;
  lc.nodes = 2;
  lc.inductors = {{1}, {0}, vec({1e-3})};
  lc.capacitors = {{1}, {0}, vec({1e-6})};
  ckt::Start start;
  start.capacitor_voltage = vec({1.0});
  const double period = 2.0 * M_PI * std::sqrt(1e-3 * 1e-6);
  std::vector<double> time;
  for (int k = 0; k <= 80; ++k) time.push_back(k * period / 20.0);
  ckt::TranOptions gear;
  gear.scheme = ckt::Scheme::Gear2;
  const auto energy_left = [&](const ckt::TranOptions &options) {
    const auto run = ckt::tran(ckt::make(lc), vec(time), {}, start, options);
    const double v = flat(run.voltage.select(1, 1)).back(), current = flat(run.inductor_current).back();
    return (0.5e-6 * v * v + 0.5e-3 * current * current) / 0.5e-6;
  };
  out.writeln("LC energy left after four periods, Gear2 at 20 samples per period    ", energy_left(gear));
  out.writeln("                                   trapezoid at 20 samples per period", energy_left(ckt::TranOptions{}));
}
```

Output

```text
largest RC output, step = 4 tau 6.66667  step = tau / 5 5.00000
LC energy left after four periods, Gear2 at 20 samples per period     0.64459
                                   trapezoid at 20 samples per period 1.00000
```

The first line shows `6.66667` on a circuit whose output cannot pass `5`, and `5.00000` once the step is a fifth of the time constant. The second shows the Gear2 run keeping only `0.64` of the energy a lossless circuit must keep, against `1.00000` for the trapezoid rule at the same step. In both cases the cure is the one in the section above: a shorter step, or the other scheme.

## Where next

The next chapters of the circuit guide add nonlinear devices to the same analyses. Reference pages: [`tran`](../../../../api/eng/circuit/analysis/tran.md), [`TranOptions`](../../../../api/eng/circuit/analysis/TranOptions.md), [`Scheme`](../../../../api/eng/circuit/analysis/Scheme.md), [`Drive`](../../../../api/eng/circuit/analysis/Drive.md), [`Start`](../../../../api/eng/circuit/analysis/Start.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="time_varying_sources.html">
    <span class="label">Previous</span>
    <span class="title">Time-varying sources</span>
  </a>
</div>
<!-- apiref:nav:end -->
