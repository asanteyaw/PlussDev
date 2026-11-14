<!-- generated from time_varying_sources.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Time-varying sources

[Back to Circuits 2: Transient Response](../circuit_02_transient.md)

Real inputs are rarely constant: a mains sinusoid, a clock pulse, a ramp, a measured waveform. `Drive` gives `tran` the value of each source at every instant of the run. This page drives an RC low-pass with a sinusoid and checks the result against the closed form and against the AC response, passes a pulse through it, turns a square wave into a triangle wave with an op-amp integrator, drives a circuit with a ramp of current, and sweeps the amplitude of a source in one call.

## The mental model

`Drive` has two members, `voltage` and `current`, one value per source per instant. `drive.voltage` has shape `[time, voltage sources]` and `drive.current` has shape `[time, current sources]`, where the time axis has the same length as the `time` vector and the sources are in the order they were given to `Net`. An undefined member holds the value stored in the circuit constant over the whole run, which is why a step needs no `Drive` at all.

The source is known only at the sample instants. Between two samples the integrator treats it as the **straight line** joining them, so a waveform is represented exactly when it is piecewise linear between samples, and approximately otherwise. Two consequences follow, and the programs below show both: a sinusoid needs enough samples per period for a straight-line approximation to be accurate, and an edge that falls between two samples becomes a ramp one step long.

Batches work as in the other analyses. A drive with leading axes needs a circuit whose parameters carry the same leading axes; a drive with no leading axes is shared by every lane.

## A sinusoid through an RC low-pass

A `1 V` sinusoid of angular frequency `w = 1000 rad/s` drives the `1 kohm`, `1 microfarad` low-pass, whose corner is at `w = 1 / tau = 1000 rad/s`. Switched on at `t = 0` with the capacitor empty, the output is the steady sinusoid plus a transient that decays as `exp(-t / tau)`:

```{math}
v(t) = \frac{\sin(\omega t - \arctan \omega\tau)}{\sqrt{1 + (\omega\tau)^2}} + \frac{\omega\tau}{1 + (\omega\tau)^2}\, e^{-t/\tau}.
```

The program runs `40 ms` (more than six periods) in steps of `0.1 ms`, prints the output beside this formula at a few instants, and compares the amplitude over the last period with the gain `circuit::ac` reports at the same frequency.

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
  net.voltage_sources = {{1}, {0}, vec({0.0})};   // the stored value is replaced by the drive
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  const auto circuit = ckt::make(net);

  const double w = 1000.0, tau = 1e-3;
  std::vector<double> time, source;
  for (int k = 0; k <= 400; ++k) {
    time.push_back(k * 1e-4);
    source.push_back(std::sin(w * time.back()));
  }
  ckt::Drive drive;
  drive.voltage = vec(source).reshape({401, 1});
  const auto run = ckt::tran(circuit, vec(time), drive);

  const std::vector<double> output = flat(run.voltage.select(1, 2));
  const double x = w * tau;
  out.writeln("  ms    tran     closed form");
  for (int ms : {1, 5, 10, 20, 40}) {
    const double t = ms * 1e-3;
    const double exact = std::sin(w * t - std::atan(x)) / std::sqrt(1.0 + x * x) + x / (1.0 + x * x) * std::exp(-t / tau);
    out.writeln(ms, output[ms * 10], exact);
  }
  const double last_period_peak = *std::max_element(output.end() - 63, output.end());
  net.voltage_sources.value = vec({1.0});                    // `ac` uses the stored value as the phasor: a unit source gives the gain
  const auto response = ckt::ac(ckt::make(net), vec({w}));
  out.writeln("peak over the last period", last_period_peak, " AC gain", flat(response.voltage.select(1, 2).abs())[0]);
}
```

Output

```text
  ms    tran     closed form
1 0.33402 0.33452
5 -0.61754 -0.61792
10 0.14777 0.14755
20 0.25205 0.25243
40 0.70571 0.70603
peak over the last period 0.70655  AC gain 0.70711
```

The simulated output tracks the closed form to better than `0.001 V`, including the early instants where the decaying transient is still visible, and by the end of the run the peak of the output has settled to within `0.001` of the gain `0.70711` that the phasor analysis gives for a unit source. (`ac` takes the stored value of each source as its phasor, so the program gives the stored value `1` before calling it; the stored `0` of the run above is replaced by the drive in `tran`.) The two analyses answer different questions about the same circuit: `ac` gives only the settled sinusoid, `tran` gives how the circuit arrives there.

## A pulse

A pulse is a source that jumps, and a jump that falls on a sample instant needs a decision, because the straight line between samples cannot be vertical. Here a `5 V` pulse starts at `1 ms` and ends at `4 ms`; the output charges for three time constants, reaching `5 (1 - exp(-3)) = 4.7510 V`, and then discharges. The program drives the circuit twice, sampled every `0.1 ms`. In the first drive the sample at the edge carries the new level (`5 V` at `1 ms`, `0 V` at `4 ms`); in the second it carries the **average of the two levels** (`2.5 V` at both edges), which centres the one-step ramp on the edge.

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
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  const auto circuit = ckt::make(net);

  std::vector<double> time, edge_new_level, edge_averaged;
  for (int k = 0; k <= 100; ++k) {
    time.push_back(k * 1e-4);
    edge_new_level.push_back(k >= 10 && k < 40 ? 5.0 : 0.0);
    edge_averaged.push_back(k == 10 || k == 40 ? 2.5 : edge_new_level.back());
  }
  const auto response = [&](const std::vector<double> &source) {
    ckt::Drive drive;
    drive.voltage = vec(source).reshape({101, 1});
    return flat(ckt::tran(circuit, vec(time), drive).voltage.select(1, 2));
  };
  const std::vector<double> a = response(edge_new_level), b = response(edge_averaged);

  const double top = 5.0 * (1.0 - std::exp(-3.0));
  out.writeln("  ms    new level   averaged   closed form");
  out.writeln(1, a[10], b[10], 0.0);
  out.writeln(2, a[20], b[20], 5.0 * (1.0 - std::exp(-1.0)));
  out.writeln(4, a[40], b[40], top);
  out.writeln(7, a[70], b[70], top * std::exp(-3.0));
  out.writeln(10, a[100], b[100], top * std::exp(-6.0));
}
```

Output

```text
  ms    new level   averaged   closed form
1 0.23810 0.11905 0.00000
2 3.24965 3.15753 3.16060
4 4.52542 4.63202 4.75106
7 0.22474 0.23657 0.23654
10 0.01116 0.01175 0.01178
```

Both runs follow the exponential charge and discharge curves, and they differ in where the pulse sits. With the new level on the edge sample, each edge is a one-step ramp that ends at the edge sample, centred `0.05 ms` early, so the whole pulse is `0.05 ms` ahead of the ideal one and the output at `7 ms` reads `0.22474` against the closed form `0.23654`, about `5 %` low. With the averaged edge sample the ramp is centred on the edge, and from `7 ms` onwards the output agrees with the closed form to `3e-5` (`0.23657` against `0.23654`). The rows at `1 ms` and `4 ms` fall on the edges themselves, where the averaged source is exactly half way through its ramp, so the two columns bracket the ideal step there rather than match it.

## An integrator turns a square wave into a triangle

An inverting op amp with a capacitor in the feedback path integrates its input: `vout = -(1 / R C) * integral of vin`. With `R = 1 kohm` and `C = 1 microfarad` the output of a unit input changes at `-1000 V/s`, so a square wave of `+1 V` for `1 ms` and `-1 V` for `1 ms` produces a triangle that goes down to `-1 V` and back to `0 V`. The program samples the square wave every `0.05 ms`, gives each edge sample the average of the two levels (`0 V`, as in the pulse above), and prints the output at the middle of every half millisecond, `0.25, 0.75, ...` ms.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  ckt::Net net;
  net.nodes = 4;
  net.voltage_sources = {{1}, {0}, vec({0.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.capacitors = {{2}, {3}, vec({1e-6})};
  net.amps = {{3}, {0}, {0}, {2}};

  std::vector<double> time, source;
  for (int k = 0; k <= 80; ++k) {                          // 4 ms in steps of 0.05 ms
    time.push_back(k * 5e-5);
    const bool edge = k > 0 && k % 20 == 0;                          // an edge falls on every millisecond
    source.push_back(edge ? 0.0 : (k / 20) % 2 == 0 ? 1.0 : -1.0);   // +1 V, -1 V, +1 V, -1 V; one millisecond each
  }
  ckt::Drive drive;
  drive.voltage = vec(source).reshape({81, 1});
  const auto run = ckt::tran(ckt::make(net), vec(time), drive);

  const std::vector<double> output = flat(run.voltage.select(1, 3));
  out.writeln("  ms    output");
  for (int quarter = 1; quarter <= 15; quarter += 2) out.writeln(quarter * 0.25, output[quarter * 5]);
  out.writeln("at the edge instants 1 ms and 2 ms", output[20], output[40]);
}
```

Output

```text
  ms    output
0.25000 -0.25000
0.75000 -0.75000
1.25000 -0.75000
1.75000 -0.25000
2.25000 -0.25000
2.75000 -0.75000
3.25000 -0.75000
3.75000 -0.25000
at the edge instants 1 ms and 2 ms -0.97500 -0.02500
```

The output is the triangle the formula predicts, and exactly so between the edges: `-0.25 V` a quarter of a millisecond into the first `+1 V` half, `-0.75 V` at three quarters, back to `-0.75 V` and `-0.25 V` on the way up, repeating every two milliseconds. A resistive, capacitive and ideal-amplifier network is simulated here as a single linear system, with the amplifier's ideal constraint enforced at every step. The last line samples the output at the edge instants themselves, `1 ms` and `2 ms`, where the averaged edge sample sits in the middle of its one-step ramp: the output reads `0.025 V` short of the ideal corner (`-0.975` instead of `-1`, `-0.025` instead of `0`), half a step of integration (`0.025 ms` at `-1000 V/s`).

## A current source with a ramp

`drive.current` works the same way. A current ramp `i(t) = a t`, with `a = 1 A/s` (so `1 mA` after a millisecond), is injected into the node of a `1 kohm`, `1 microfarad` parallel RC. The voltage of such a node follows the ramp with a lag: `v(t) = R a (t - tau + tau exp(-t / tau))`, which is `4.0067 V` at `5 ms`. The current source injects into its negative node, so the source is declared from the ground to node 1.

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
  net.nodes = 2;
  net.current_sources = {{0}, {1}, vec({0.0})};   // injects into node 1
  net.resistors = {{1}, {0}, vec({1000.0})};
  net.capacitors = {{1}, {0}, vec({1e-6})};

  std::vector<double> time, ramp;
  for (int k = 0; k <= 50; ++k) {
    time.push_back(k * 1e-4);
    ramp.push_back(1.0 * time.back());           // 1 A/s
  }
  ckt::Drive drive;
  drive.current = vec(ramp).reshape({51, 1});
  const auto run = ckt::tran(ckt::make(net), vec(time), drive);

  const std::vector<double> output = flat(run.voltage.select(1, 1));
  const double tau = 1e-3;
  out.writeln("  ms    voltage   closed form");
  for (int ms : {1, 2, 3, 4, 5}) {
    const double t = ms * 1e-3;
    out.writeln(ms, output[ms * 10], 1000.0 * (t - tau + tau * std::exp(-t / tau)));
  }
}
```

Output

```text
  ms    voltage   closed form
1 0.36757 0.36788
2 1.13511 1.13534
3 2.04966 2.04979
4 3.01825 3.01832
5 4.00671 4.00674
```

A ramp is piecewise linear, so the straight-line treatment between samples represents it exactly and the only error left is the integrator's own, which is why the agreement is closer than for the pulse.

## A sweep of source amplitudes

To run a waveform at several amplitudes in one call, give the circuit a parameter batch and the drive the same leading axis. The program gives the source a batched stored value of three lanes (the values are replaced by the drive, but they tell `make` the batch shape), and the drive three sinusoids of amplitude `1`, `2` and `4`, one per lane. The result has shape `[3, time, nodes]`, and the steady peak of each lane is proportional to its amplitude.

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
  net.voltage_sources = {{1}, {0}, vec({1.0, 2.0, 4.0}).reshape({3, 1})};   // the lanes
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};

  std::vector<double> time, waves;
  for (int k = 0; k <= 400; ++k) time.push_back(k * 1e-4);
  for (double amplitude : {1.0, 2.0, 4.0})
    for (double t : time) waves.push_back(amplitude * std::sin(1000.0 * t));
  ckt::Drive drive;
  drive.voltage = vec(waves).reshape({3, 401, 1});
  const auto run = ckt::tran(ckt::make(net), vec(time), drive);

  out.writeln("voltage shape", run.voltage.sizes(), " (amplitude, time sample, node)");
  for (int lane = 0; lane < 3; ++lane) {
    const std::vector<double> output = flat(run.voltage.select(0, lane).select(1, 2));
    out.writeln("peak over the last period, lane", lane, *std::max_element(output.end() - 63, output.end()));
  }
}
```

Output

```text
voltage shape [3, 401, 3]  (amplitude, time sample, node)
peak over the last period, lane 0 0.70655
peak over the last period, lane 1 1.41309
peak over the last period, lane 2 2.82618
```

The three peaks, `0.70655`, `1.41309` and `2.82618`, are in the ratio `1 : 2 : 4` and each is within `0.001` of its amplitude times `0.70711`: the circuit is linear, so doubling the source doubles every response. A drive with leading axes that do not match the circuit's batch is rejected, as the errors below show.

## Errors

`Drive` is checked against the circuit and the time vector before anything is solved.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

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
  const pluss::Series time = vec({0.0, 1e-3, 2e-3});

  ckt::Drive short_drive;
  short_drive.voltage = vec({1.0, 2.0}).reshape({2, 1});          // two rows for three samples
  report([&] { (void)ckt::tran(circuit, time, short_drive); });
  ckt::Drive wide_drive;
  wide_drive.voltage = vec({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}).reshape({3, 2});   // two columns for one source
  report([&] { (void)ckt::tran(circuit, time, wide_drive); });
  ckt::Drive extra_lanes;
  extra_lanes.voltage = vec({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}).reshape({2, 3, 1});   // two lanes for an unbatched circuit
  report([&] { (void)ckt::tran(circuit, time, extra_lanes); });
  ckt::Drive no_such_source;
  no_such_source.current = vec({1.0, 2.0, 3.0}).reshape({3, 1});   // the circuit has no current source
  report([&] { (void)ckt::tran(circuit, time, no_such_source); });
}
```

Output

```text
eng::circuit::tran: drive voltage needs one row per time sample (3), got 2
eng::circuit::tran: drive voltage needs one column per source (1), got 2
eng::circuit::tran: drive voltage leading axes must match the circuit's parameter batch
eng::circuit::tran: drive current has values but the circuit has no such sources
```

Each message says which part of the drive is wrong and what was expected: the number of rows (one per time sample, three here), the number of columns (one per source), the leading axes (the circuit's batch), or a drive for a source the circuit does not have.

## Decisions

- **A sinusoid:** at least twenty samples per period, and enough periods that the start-up transient has decayed (several time constants of the slowest part of the circuit).
- **A pulse or a square wave:** a step small enough that a ramp one step long is negligible against the features you care about; give each edge sample the average of the two levels, so the one-step ramp is centred on the edge.
- **A measured waveform:** sample the circuit's `time` vector at the instants of the measurement, if they are evenly spaced; otherwise resample the measurement onto an even grid first.
- **Only the settled sinusoid:** use `circuit::ac`; it needs no run at all and gives amplitude and phase directly.
- **Several amplitudes or component values:** a parameter batch on the circuit and the same leading axis on the drive.

## Pitfalls

The program drives the RC low-pass at its corner frequency with a sinusoid sampled at `40`, `10` and `4` points per period, and shows the settled peak against the `0.70711` of the AC analysis.

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
  net.voltage_sources = {{1}, {0}, vec({0.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  const auto circuit = ckt::make(net);
  const double w = 1000.0, period = 2.0 * M_PI / w;

  for (int per : {40, 10, 4}) {
    const double step = period / per;
    const int samples = 12 * per + 1;                    // twelve periods
    std::vector<double> time, source;
    for (int k = 0; k < samples; ++k) {
      time.push_back(k * step);
      source.push_back(std::sin(w * time.back()));
    }
    ckt::Drive drive;
    drive.voltage = vec(source).reshape({samples, 1});
    const auto run = ckt::tran(circuit, vec(time), drive);
    const std::vector<double> output = flat(run.voltage.select(1, 2));
    out.writeln("samples per period", per, " settled peak", *std::max_element(output.end() - per, output.end()), " AC gain", 1.0 / std::sqrt(2.0));
  }
}
```

Output

```text
samples per period 40  settled peak 0.70638  AC gain 0.70711
samples per period 10  settled peak 0.68828  AC gain 0.70711
samples per period 4  settled peak 0.48576  AC gain 0.70711
```

At `40` samples per period the settled peak is within `0.001` of the AC gain; at `10` it is visibly off, and at `4` the straight line through the samples has replaced the sinusoid by something much closer to a triangle wave, which the circuit then filters faithfully. The circuit analysis is exact for the waveform it is given, and the waveform it was given is the straight line through the samples.

## Where next

[Schemes and step size](schemes_and_step_size.md) explains the accuracy of the integrators and how to pick the step. Reference pages: [`tran`](../../../../api/eng/circuit/analysis/tran.md), [`Drive`](../../../../api/eng/circuit/analysis/Drive.md), [`Start`](../../../../api/eng/circuit/analysis/Start.md), [`ac`](../../../../api/eng/circuit/analysis/ac.md), [`Net`](../../../../api/eng/circuit/model/Net.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="step_response.html">
    <span class="label">Previous</span>
    <span class="title">Step response and starting state</span>
  </a>
  <a class="next-link" href="schemes_and_step_size.html">
    <span class="label">Next</span>
    <span class="title">Schemes and step size</span>
  </a>
</div>
<!-- apiref:nav:end -->
