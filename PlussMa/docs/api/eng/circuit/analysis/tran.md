<!-- generated from tran.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `circuit::tran`

Computes the response of a circuit in time.

## Signature

```cpp
circuit::Result circuit::tran(const circuit::Circuit& circuit, const pluss::Series& time, const circuit::Drive& drive = {}, const circuit::Start& start = {}, const circuit::TranOptions& options = {});
```

## What It Does

Marches the circuit equations forward from the first instant of `time` to the last, at the fixed step of the grid, and returns every node voltage and branch current at every instant. The first sample is the starting state: a state that satisfies the circuit equations with each capacitor held at its `start` voltage and each inductor at its `start` current. The independent sources take the values of `drive` at each instant; a source with no drive value holds the value stored in the circuit. The two time-stepping rules and the controls of the Newton iteration are selected by `options`. A circuit with diodes or transistors solves the nonlinear step equation at every sample by Newton iteration, starting from the previous sample.

## Parameters

`circuit`
: The compiled circuit. Its sources hold the values used wherever `drive` does not supply one, and its parameter batch becomes the leading axes of every result.

`time`
: The instants in seconds: a real floating vector of at least two strictly increasing, evenly spaced values on the circuit's platform. The step is `time[1] - time[0]` and the first value need not be zero. Spacing may differ from the step by rounding of the values themselves, which is much coarser for single precision.

`drive`
: The values of the independent sources at each instant; see `Drive`. The default holds every source at its stored value.

`start`
: The energy stored at the first instant; see `Start`. The default is a circuit at rest, with every capacitor voltage and inductor current zero.

`options`
: The time-stepping rule and, for circuits with devices, the iteration controls; see `TranOptions`. The default is the trapezoid rule.

## Returns

A `Result` whose arrays all gain a time axis immediately before the final element axis: `voltage` is `[..., time, nodes]` with node `0` zero, `source_current` is `[..., time, voltage sources]`, and likewise for the inductor, VCCS, VCVS and op-amp currents. `residual` is `[..., time]`: the largest defect of the equations solved for each sample (the starting-state equations at the first sample and the implicit step equation after it). For a circuit with devices, `diode_current` is `[..., time, diodes]`, `collector_current` and `base_current` are `[..., time, transistors]`, `iterations` is `[time]` with the Newton iterations of each sample (the first is the starting-state solve) and `refinements` is `[time]` with the number of sub-steps a sample needed (zero where the step was accepted as it was), and `residual` is the defect of the nonlinear step equation. The element type is that of the circuit.

## Errors

Throws `pluss::Error` for a time vector that is too short, not strictly increasing, not evenly spaced, not floating, or on another platform; for a `drive` or `start` array of the wrong shape, platform or element type, with non-finite values, or for elements the circuit does not have; for a device circuit whose Newton iteration does not converge at some sample within `options.solver.max_iterations`, for an invalid control, and for equations without a solution: a node with no path to ground, two ideal sources fixing the same voltage, a loop of ideal voltage sources, or a start state that contradicts what the sources impose (a capacitor directly across ideal voltage sources at another voltage). Checks run before anything is solved.

## Interpretation and Use

The step sets the accuracy: both rules are second order, so halving the step divides the error of a smooth response by about four. Use at least twenty samples per time constant or per period for the part of the response you care about. A step much longer than the fastest time constant makes the trapezoid rule overshoot and ring; `Scheme::Gear2` damps that instead. Sources are known only at the samples and are straight lines between them, so a jump that falls on a sample is a ramp one step long; give that sample the average of the two levels to centre the ramp on the jump. The step is adaptive where devices switch: each interval is compared with the same interval taken as two half steps, and when they differ by more than `options.solver.step_tolerance` it is subdivided with trapezoid sub-steps (up to `max_refinement` halvings), so a long sample interval does not make a switching circuit ring. `Result::refinements` says where that happened. The sources are still straight lines between samples, so sample a waveform finely enough to represent it. The run takes memory proportional to the number of samples times the number of unknowns times the batch size.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A step response and its closed form, with the shapes of the result.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static std::vector<double> grid(double step, int count) {
  std::vector<double> t;
  for (int k = 0; k < count; ++k) t.push_back(k * step);
  return t;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  const auto run = ckt::tran(ckt::make(net), vec(grid(1e-4, 51)));   // 5 ms in steps of 0.1 ms

  out.writeln("voltage shape", run.voltage.sizes(), " source current shape", run.source_current.sizes(), " residual shape", run.residual.sizes());
  out.writeln("first sample", flat(run.voltage.select(0, 0)));
  const std::vector<double> v = flat(run.voltage.select(1, 2));
  for (int ms : {1, 2, 5}) out.writeln("t =", ms, "ms  v =", v[ms * 10], " closed form", 5.0 * (1.0 - std::exp(-ms * 1e-3 / 1e-3)));
  out.writeln("largest step residual", run.residual.amax().item<double>() < 1e-12 ? "below 1e-12" : "too large");
}
```

Output

```text
voltage shape [51, 3]  source current shape [51, 1]  residual shape [51]
first sample [0.00000, 5.00000, 0.00000]
t = 1 ms  v = 3.16214  closed form 3.16060
t = 2 ms  v = 4.32445  closed form 4.32332
t = 5 ms  v = 4.96645  closed form 4.96631
largest step residual below 1e-12
```
### Example 2

The starting state: a charged capacitor and a flowing inductor current.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static std::vector<double> grid(double step, int count) {
  std::vector<double> t;
  for (int k = 0; k < count; ++k) t.push_back(k * step);
  return t;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // a discharging capacitor: the source is off and the capacitor starts at 4 V
  ckt::Net rc;
  rc.nodes = 3;
  rc.voltage_sources = {{1}, {0}, vec({0.0})};
  rc.resistors = {{1}, {2}, vec({1000.0})};
  rc.capacitors = {{2}, {0}, vec({1e-6})};
  ckt::Start charged;
  charged.capacitor_voltage = vec({4.0});
  const auto discharge = ckt::tran(ckt::make(rc), vec(grid(1e-4, 31)), {}, charged);
  const std::vector<double> v = flat(discharge.voltage.select(1, 2));
  out.writeln("capacitor at t = 0, tau, 3 tau", v[0], v[10], v[30], " closed form", 4.0, 4.0 * std::exp(-1.0), 4.0 * std::exp(-3.0));

  // a decaying inductor current: the source is off and the inductor starts at 0.25 A
  ckt::Net rl;
  rl.nodes = 3;
  rl.voltage_sources = {{1}, {0}, vec({0.0})};
  rl.resistors = {{1}, {2}, vec({10.0})};
  rl.inductors = {{2}, {0}, vec({0.01})};
  ckt::Start flowing;
  flowing.inductor_current = vec({0.25});
  const auto decay = ckt::tran(ckt::make(rl), vec(grid(1e-4, 31)), {}, flowing);
  const std::vector<double> i = flat(decay.inductor_current);
  out.writeln("inductor at t = 0, tau, 3 tau", i[0], i[10], i[30], " closed form", 0.25, 0.25 * std::exp(-1.0), 0.25 * std::exp(-3.0));
}
```

Output

```text
capacitor at t = 0, tau, 3 tau 4.00000 1.47029 0.19865  closed form 4.00000 1.47152 0.19915
inductor at t = 0, tau, 3 tau 0.25000 0.09189 0.01242  closed form 0.25000 0.09197 0.01245
```
### Example 3

A sinusoidal voltage drive and a current ramp.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static std::vector<double> grid(double step, int count) {
  std::vector<double> t;
  for (int k = 0; k < count; ++k) t.push_back(k * step);
  return t;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // a sinusoidal voltage source through an RC low-pass at its corner frequency
  ckt::Net rc;
  rc.nodes = 3;
  rc.voltage_sources = {{1}, {0}, vec({0.0})};
  rc.resistors = {{1}, {2}, vec({1000.0})};
  rc.capacitors = {{2}, {0}, vec({1e-6})};
  const std::vector<double> time = grid(1e-4, 401);
  std::vector<double> sine;
  for (double t : time) sine.push_back(std::sin(1000.0 * t));
  ckt::Drive voltage_drive;
  voltage_drive.voltage = vec(sine).reshape({401, 1});
  const std::vector<double> v = flat(ckt::tran(ckt::make(rc), vec(time), voltage_drive).voltage.select(1, 2));
  out.writeln("sinusoid: output at 10, 20, 40 ms", v[100], v[200], v[400]);

  // a current ramp of 1 A/s into a parallel RC: v = R a (t - tau + tau exp(-t/tau))
  ckt::Net parallel;
  parallel.nodes = 2;
  parallel.current_sources = {{0}, {1}, vec({0.0})};
  parallel.resistors = {{1}, {0}, vec({1000.0})};
  parallel.capacitors = {{1}, {0}, vec({1e-6})};
  std::vector<double> ramp;
  for (int k = 0; k < 51; ++k) ramp.push_back(k * 1e-4);
  ckt::Drive current_drive;
  current_drive.current = vec(ramp).reshape({51, 1});
  const std::vector<double> u = flat(ckt::tran(ckt::make(parallel), vec(grid(1e-4, 51)), current_drive).voltage.select(1, 1));
  out.writeln("ramp: voltage at 5 ms", u[50], " closed form", 1000.0 * (5e-3 - 1e-3 + 1e-3 * std::exp(-5.0)));
}
```

Output

```text
sinusoid: output at 10, 20, 40 ms 0.14777 0.25205 0.70571
ramp: voltage at 5 ms 4.00671  closed form 4.00674
```
### Example 4

The two time-stepping rules on an oscillator that must keep its energy.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static std::vector<double> grid(double step, int count) {
  std::vector<double> t;
  for (int k = 0; k < count; ++k) t.push_back(k * step);
  return t;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  // a lossless LC oscillator: 1 mH, 1 uF, the capacitor starts at 1 V
  ckt::Net lc;
  lc.nodes = 2;
  lc.inductors = {{1}, {0}, vec({1e-3})};
  lc.capacitors = {{1}, {0}, vec({1e-6})};
  const auto circuit = ckt::make(lc);
  ckt::Start start;
  start.capacitor_voltage = vec({1.0});
  const double period = 2.0 * M_PI * std::sqrt(1e-3 * 1e-6);
  const std::vector<double> time = grid(period / 40.0, 161);   // four periods at 40 samples per period

  for (auto scheme : {ckt::Scheme::Trapezoid, ckt::Scheme::Gear2}) {
    ckt::TranOptions options;
    options.scheme = scheme;
    const auto run = ckt::tran(circuit, vec(time), {}, start, options);
    const double v = flat(run.voltage.select(1, 1)).back(), i = flat(run.inductor_current).back();
    out.writeln(scheme == ckt::Scheme::Trapezoid ? "Trapezoid" : "Gear2    ", "energy left after four periods", (0.5e-6 * v * v + 0.5e-3 * i * i) / 0.5e-6);
  }
}
```

Output

```text
Trapezoid energy left after four periods 1.000000
Gear2     energy left after four periods 0.921931
```
### Example 5

A parameter sweep and a single-precision circuit.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static std::vector<double> grid(double step, int count) {
  std::vector<double> t;
  for (int k = 0; k < count; ++k) t.push_back(k * step);
  return t;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // a sweep of the resistor: tau = 0.5, 1 and 2 ms
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.resistors = {{1}, {2}, vec({500.0, 1000.0, 2000.0}).reshape({3, 1})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  const auto run = ckt::tran(ckt::make(net), vec(grid(1e-4, 51)));
  out.writeln("voltage shape", run.voltage.sizes(), " residual shape", run.residual.sizes());
  const double taus[3] = {0.5e-3, 1e-3, 2e-3};
  for (int lane = 0; lane < 3; ++lane)
    out.writeln("lane", lane, "output at 2 ms", flat(run.voltage.select(0, lane).select(1, 2))[20], " closed form", 5.0 * (1.0 - std::exp(-2e-3 / taus[lane])));

  // single precision circuits and time vectors stay single precision
  ckt::Net single;
  single.nodes = 3;
  const auto f32 = pluss::ScalarType::Float32;
  single.voltage_sources = {{1}, {0}, vec({5.0}).to(f32)};
  single.resistors = {{1}, {2}, vec({1000.0}).to(f32)};
  single.capacitors = {{2}, {0}, vec({1e-6}).to(f32)};
  const auto fast = ckt::tran(ckt::make(single), vec(grid(1e-4, 51)).to(f32));
  out.writeln("single precision run has element type", fast.voltage.dtype().name, " output at 5 ms", flat(fast.voltage.select(1, 2))[50]);
}
```

Output

```text
voltage shape [3, 51, 3]  residual shape [3, 51]
lane 0 output at 2 ms 4.90964  closed form 4.90842
lane 1 output at 2 ms 4.32445  closed form 4.32332
lane 2 output at 2 ms 3.16099  closed form 3.16060
single precision run has element type float32  output at 5 ms 4.96645
```
### Example 6

The shape of every result array for a circuit with every kind of element.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static std::vector<double> grid(double step, int count) {
  std::vector<double> t;
  for (int k = 0; k < count; ++k) t.push_back(k * step);
  return t;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // node 1 source, node 2 an RC low-pass, node 3 a VCVS of gain 2 with an inductor, nodes 4 and 5 an op-amp inverter with a transconductance draw
  ckt::Net net;
  net.nodes = 6;
  net.voltage_sources = {{1}, {0}, vec({1.0})};
  net.resistors = {{1, 3, 4, 5}, {2, 4, 5, 0}, vec({1000.0, 1000.0, 1000.0, 2000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  net.inductors = {{3}, {0}, vec({0.5})};
  net.vcvs = {{3}, {0}, {2}, {0}, vec({2.0})};
  net.vccs = {{5}, {0}, {2}, {0}, vec({1e-3})};
  net.amps = {{5}, {0}, {0}, {4}};
  const auto run = ckt::tran(ckt::make(net), vec(grid(1e-4, 11)));
  out.writeln("voltage         ", run.voltage.sizes());
  out.writeln("source_current  ", run.source_current.sizes());
  out.writeln("inductor_current", run.inductor_current.sizes());
  out.writeln("vccs_current    ", run.vccs_current.sizes());
  out.writeln("vcvs_current    ", run.vcvs_current.sizes());
  out.writeln("amp_current     ", run.amp_current.sizes());
  out.writeln("residual        ", run.residual.sizes());
}
```

Output

```text
voltage          [11, 6]
source_current   [11, 1]
inductor_current [11, 1]
vccs_current     [11, 1]
vcvs_current     [11, 1]
amp_current      [11, 1]
residual         [11]
```
### Example 7

Rejected time vectors and an unreachable starting state.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static std::vector<double> grid(double step, int count) {
  std::vector<double> t;
  for (int k = 0; k < count; ++k) t.push_back(k * step);
  return t;
}

int main() {
  OutputEngine out;
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  const ckt::Circuit circuit = ckt::make(net);
  report([&] { (void)ckt::tran(circuit, vec({0.0})); });
  report([&] { (void)ckt::tran(circuit, vec({0.0, 1e-3, 3e-3})); });
  report([&] { (void)ckt::tran(circuit, vec({1e-3, 0.0})); });
  report([&] { (void)ckt::tran(circuit, pluss::series::zeros({2, 2}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64))); });

  // a capacitor directly across the ideal source cannot start at rest
  ckt::Net across;
  across.nodes = 2;
  across.voltage_sources = {{1}, {0}, vec({5.0})};
  across.resistors = {{1}, {0}, vec({1000.0})};
  across.capacitors = {{1}, {0}, vec({1e-6})};
  report([&] { (void)ckt::tran(ckt::make(across), vec({0.0, 1e-3})); });
}
```

Output

```text
eng::circuit::tran: time must be a real floating vector with at least two samples
eng::circuit::tran: time must be uniformly spaced (one step size for the whole run)
eng::circuit::tran: time must be finite and strictly increasing
eng::circuit::tran: time must be a real floating vector with at least two samples
eng::circuit::tran: the starting state cannot be reached; a capacitor connected directly across ideal voltage sources must start at the voltage they impose, an inductor in series with only current sources must start at the current they impose, and every node needs a path to ground (a loop of ideal voltage sources, or two sources fixing the same voltage, also has no solution)
```
### Example 8

A half-wave rectifier driven by a sine.

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
  out.print_defaults(PrintOptions{}.fixed(4));
  // half-wave rectifier: 5 V peak, 500 Hz source, diode, 1 kohm and 1 uF load
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({0.0})};
  net.diodes.positive = {1};
  net.diodes.negative = {2};
  net.diodes.saturation = vec({1e-12});
  net.resistors = {{2}, {0}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};

  const int samples = 401;
  const double step = 5e-5;
  std::vector<double> time, wave;
  for (int k = 0; k < samples; ++k) {
    time.push_back(k * step);
    wave.push_back(5.0 * std::sin(2.0 * M_PI * 500.0 * k * step));
  }
  ckt::Drive drive;
  drive.voltage = vec(wave).reshape({samples, 1});
  const auto run = ckt::tran(ckt::make(net), vec(time), drive);

  const std::vector<double> load = flat(run.voltage.select(1, 2));
  const std::vector<double> diode = flat(run.diode_current);
  out.writeln("   ms    source    load   diode current");
  for (int k = 0; k <= 80; k += 5) out.writeln(time[k] * 1e3, wave[k], load[k], diode[k]);
  out.writeln("load peak", *std::max_element(load.begin(), load.end()), " lowest", *std::min_element(load.begin(), load.end()));
  out.writeln("diode_current shape", run.diode_current.sizes(), " (time sample, diode)");
  out.writeln("largest step residual below 1e-8", run.residual.amax().item<double>() < 1e-8);
}
```

Output

```text
   ms    source    load   diode current
0.0000 0.0000 0.0000 0.0000
0.2500 3.5355 2.9312 0.0142
0.5000 5.0000 4.4182 0.0060
0.7500 3.5355 3.6491 -0.0000
1.0000 0.0000 2.8418 -0.0000
1.2500 -3.5355 2.2131 -0.0000
1.5000 -5.0000 1.7234 -0.0000
1.7500 -3.5355 1.3421 -0.0000
2.0000 -0.0000 1.0452 -0.0000
2.2500 3.5355 2.9311 0.0143
2.5000 5.0000 4.4181 0.0060
2.7500 3.5355 3.6491 -0.0000
3.0000 0.0000 2.8418 -0.0000
3.2500 -3.5355 2.2131 -0.0000
3.5000 -5.0000 1.7234 -0.0000
3.7500 -3.5355 1.3421 -0.0000
4.0000 -0.0000 1.0452 -0.0000
load peak 4.4182  lowest 0.0000
diode_current shape [401, 1]  (time sample, diode)
largest step residual below 1e-8 true
```
### Example 9

The Newton iterations of each sample.

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
  // iterations records how many Newton iterations each time sample needed
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({0.0})};
  net.diodes.positive = {1};
  net.diodes.negative = {2};
  net.diodes.saturation = vec({1e-12});
  net.resistors = {{2}, {0}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  const int samples = 201;
  std::vector<double> time, wave;
  for (int k = 0; k < samples; ++k) {
    time.push_back(k * 5e-5);
    wave.push_back(5.0 * std::sin(2.0 * M_PI * 500.0 * k * 5e-5));
  }
  ckt::Drive drive;
  drive.voltage = vec(wave).reshape({samples, 1});
  const auto run = ckt::tran(ckt::make(net), vec(time), drive);
  const std::vector<double> counts = flat(run.iterations.to(pluss::ScalarType::Float64));
  double most = 0.0, total = 0.0;
  for (double c : counts) {
    most = std::max(most, c);
    total += c;
  }
  out.writeln("iterations shape", run.iterations.sizes());
  out.writeln("most iterations in one sample", most, " total", total);
  out.writeln("first sample (the starting state) took", counts[0], " iterations");
}
```

Output

```text
iterations shape [201]
most iterations in one sample 241  total 3968
first sample (the starting state) took 1  iterations
```
### Example 10

A charged capacitor behind a blocking diode.

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
  out.print_defaults(PrintOptions{}.fixed(5));
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({0.0})};
  net.diodes.positive = {1};
  net.diodes.negative = {2};
  net.diodes.saturation = vec({1e-12});
  net.resistors = {{2}, {0}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  ckt::Start charged;
  charged.capacitor_voltage = vec({3.0});
  std::vector<double> time;
  for (int k = 0; k <= 30; ++k) time.push_back(k * 1e-4);
  const auto run = ckt::tran(ckt::make(net), vec(time), {}, charged);
  const std::vector<double> load = flat(run.voltage.select(1, 2));
  out.writeln("  ms    load    closed form 3 exp(-t / RC)");
  for (int ms = 0; ms <= 3; ++ms) out.writeln(ms, load[ms * 10], 3.0 * std::exp(-1.0 * ms));
  out.writeln("diode current below 1e-9 A at every sample", run.diode_current.abs().amax().item<double>() < 1e-9);
}
```

Output

```text
  ms    load    closed form 3 exp(-t / RC)
0 3.00000 3.00000
1 1.10272 1.10364
2 0.40533 0.40601
3 0.14899 0.14936
diode current below 1e-9 A at every sample true
```
### Example 11

A coarse step is subdivided where the diode switches.

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
  out.print_defaults(PrintOptions{}.fixed(4));
  // a half-wave rectifier sampled four times per cycle: the step is subdivided where the diode switches
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({0.0})};
  net.diodes.positive = {1};
  net.diodes.negative = {2};
  net.diodes.saturation = vec({1e-12});
  net.resistors = {{2}, {0}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-5})};
  const auto circuit = ckt::make(net);
  for (double step : {5e-5, 5e-4}) {
    const int samples = static_cast<int>(std::lround(8e-3 / step)) + 1;
    std::vector<double> time, wave;
    for (int k = 0; k < samples; ++k) {
      time.push_back(k * step);
      wave.push_back(5.0 * std::sin(2.0 * M_PI * 500.0 * k * step));
    }
    ckt::Drive drive;
    drive.voltage = vec(wave).reshape({samples, 1});
    const auto run = ckt::tran(circuit, vec(time), drive);
    std::int64_t refined = 0;
    for (std::int64_t count : run.refinements.items<std::int64_t>()) refined += count;
    out.writeln("step", step * 1e6, "us  highest load voltage", run.voltage.select(1, 2).amax().item<double>(), " refined sub-steps", refined);
  }
  // with refinement off a long step rings above the source peak
  ckt::TranOptions off;
  off.solver.max_refinement = 0;
  const double step = 5e-4;
  std::vector<double> time, wave;
  for (int k = 0; k <= 16; ++k) {
    time.push_back(k * step);
    wave.push_back(5.0 * std::sin(2.0 * M_PI * 500.0 * k * step));
  }
  ckt::Drive drive;
  drive.voltage = vec(wave).reshape({17, 1});
  out.writeln("500 us, max_refinement 0, highest load voltage", ckt::tran(circuit, vec(time), drive, {}, off).voltage.select(1, 2).amax().item<double>(), " (the source peak is 5)");
}
```

Output

```text
step 50.0000 us  highest load voltage 4.3993  refined sub-steps 84
step 500.0000 us  highest load voltage 4.3451  refined sub-steps 65
500 us, max_refinement 0, highest load voltage 8.4496  (the source peak is 5)
```
### Example 12

An iteration limit that is too small, and invalid controls.

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
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({0.0})};
  net.diodes.positive = {1};
  net.diodes.negative = {2};
  net.diodes.saturation = vec({1e-12});
  net.resistors = {{2}, {0}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  const auto circuit = ckt::make(net);
  const int samples = 41;
  std::vector<double> time, wave;
  for (int k = 0; k < samples; ++k) {
    time.push_back(k * 5e-5);
    wave.push_back(5.0 * std::sin(2.0 * M_PI * 500.0 * k * 5e-5));
  }
  ckt::Drive drive;
  drive.voltage = vec(wave).reshape({samples, 1});
  ckt::TranOptions few;
  few.solver.max_iterations = 1;
  report([&] { (void)ckt::tran(circuit, vec(time), drive, {}, few); });
  ckt::TranOptions negative;
  negative.solver.tolerance = -1.0;
  report([&] { (void)ckt::tran(circuit, vec(time), drive, {}, negative); });
  ckt::TranOptions leaky;
  leaky.solver.gmin = -1.0;
  report([&] { (void)ckt::tran(circuit, vec(time), drive, {}, leaky); });
}
```

Output

```text
eng::circuit::tran: the Newton iteration did not converge in 1 iterations (last update 0.782); raise max_iterations, loosen tolerance, or check that the bias is reachable (gmin stepping was also tried)
eng::circuit::tran: solver tolerance must be positive
eng::circuit::tran: solver gmin must be non-negative
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ac.html">
    <span class="label">Previous</span>
    <span class="title">ac</span>
  </a>
  <a class="next-link" href="Drive.html">
    <span class="label">Next</span>
    <span class="title">Drive</span>
  </a>
</div>
<!-- apiref:nav:end -->
