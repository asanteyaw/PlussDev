<!-- generated from Drive.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `circuit::Drive`

Time-varying values of the independent sources.

## Signature

```cpp
circuit::Drive
```

## What It Does

Supplies the value of each independent source at each instant of a `tran` run. A source that has no entry in the drive keeps the value stored in the circuit for the whole run, so a step needs no drive at all.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `voltage` | The voltage of each voltage source at each instant, `[..., time, voltage sources]`, in the order the sources were given to the net. | Undefined: every voltage source keeps its stored value. |
| `current` | The current of each current source at each instant, `[..., time, current sources]`. | Undefined: every current source keeps its stored value. |

## Returns

A value; validated by `tran`.

## Errors

A member must be a real floating, finite array on the circuit's platform with one row per time sample and one column per source. Leading axes must either be absent, so that every lane shares the drive, or equal the circuit's parameter batch. A member for a kind of source the circuit does not have is rejected.

## Interpretation and Use

The source is known only at the samples; between them the integrator treats it as the straight line joining the neighbouring values. A waveform that is piecewise linear between samples is therefore exact, a sinusoid needs enough samples per period (twenty or more), and a jump that falls on a sample is best given the average of the levels on either side. A drive with leading axes needs a circuit whose parameters have the same leading axes; give a source a batched stored value to create them, since the stored value is replaced by the drive.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A voltage drive and a current drive.

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
### Example 2

One member driven, the other held, and no drive at all.

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
  // one voltage source and one current source; only the voltage is driven, the current keeps its stored value
  ckt::Net net;
  net.nodes = 2;
  net.voltage_sources = {{1}, {0}, vec({0.0})};
  net.current_sources = {{1}, {0}, vec({0.001})};   // 1 mA, held for the whole run
  net.resistors = {{1}, {0}, vec({1000.0})};
  const std::vector<double> time = grid(1.0, 4);
  ckt::Drive voltage_only;
  voltage_only.voltage = vec({1.0, 2.0, 3.0, 4.0}).reshape({4, 1});
  const auto run = ckt::tran(ckt::make(net), vec(time), voltage_only);
  out.writeln("source current, with the drive      ", flat(run.source_current));
  const auto held = ckt::tran(ckt::make(net), vec(time));   // no drive at all: the stored 0 V is held
  out.writeln("source current, no drive (0 V held) ", flat(held.source_current));
  out.writeln("node 1 voltage, with the drive      ", flat(run.voltage.select(1, 1)));
}
```

Output

```text
source current, with the drive       [-0.00200, -0.00300, -0.00400, -0.00500]
source current, no drive (0 V held)  [-0.00100, -0.00100, -0.00100, -0.00100]
node 1 voltage, with the drive       [1.00000, 2.00000, 3.00000, 4.00000]
```
### Example 3

A drive with a lane per amplitude, and one drive shared by every lane of a swept circuit.

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
  net.voltage_sources = {{1}, {0}, vec({1.0, 2.0, 4.0}).reshape({3, 1})};   // the stored values only give the batch shape
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  const std::vector<double> time = grid(1e-4, 101);
  std::vector<double> waves;
  for (double amplitude : {1.0, 2.0, 4.0})
    for (double t : time) waves.push_back(amplitude * std::sin(1000.0 * t));
  ckt::Drive drive;
  drive.voltage = vec(waves).reshape({3, 101, 1});          // one waveform per lane
  const auto run = ckt::tran(ckt::make(net), vec(time), drive);
  out.writeln("voltage shape", run.voltage.sizes());
  for (int lane = 0; lane < 3; ++lane) out.writeln("lane", lane, "output at 10 ms", flat(run.voltage.select(0, lane).select(1, 2))[100]);

  // one waveform shared by every lane of a swept circuit
  ckt::Net swept = net;
  swept.voltage_sources.value = vec({1.0});
  swept.resistors.value = vec({500.0, 1000.0, 2000.0}).reshape({3, 1});
  ckt::Drive shared;
  shared.voltage = vec(std::vector<double>(waves.begin(), waves.begin() + 101)).reshape({101, 1});   // no lane axis
  const auto swept_run = ckt::tran(ckt::make(swept), vec(time), shared);
  out.writeln("shared drive, voltage shape", swept_run.voltage.sizes());
}
```

Output

```text
voltage shape [3, 101, 3]
lane 0 output at 10 ms 0.14777
lane 1 output at 10 ms 0.29555
lane 2 output at 10 ms 0.59110
shared drive, voltage shape [3, 101, 3]
```
### Example 4

Rejected drives.

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
  const pluss::Series time = vec({0.0, 1e-3, 2e-3});
  ckt::Drive rows;
  rows.voltage = vec({1.0, 2.0}).reshape({2, 1});
  report([&] { (void)ckt::tran(circuit, time, rows); });
  ckt::Drive columns;
  columns.voltage = vec({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}).reshape({3, 2});
  report([&] { (void)ckt::tran(circuit, time, columns); });
  ckt::Drive lanes;
  lanes.voltage = vec({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}).reshape({2, 3, 1});
  report([&] { (void)ckt::tran(circuit, time, lanes); });
  ckt::Drive missing;
  missing.current = vec({1.0, 2.0, 3.0}).reshape({3, 1});
  report([&] { (void)ckt::tran(circuit, time, missing); });
  ckt::Drive not_finite;
  not_finite.voltage = vec({1.0, std::nan(""), 3.0}).reshape({3, 1});
  report([&] { (void)ckt::tran(circuit, time, not_finite); });
}
```

Output

```text
eng::circuit::tran: drive voltage needs one row per time sample (3), got 2
eng::circuit::tran: drive voltage needs one column per source (1), got 2
eng::circuit::tran: drive voltage leading axes must match the circuit's parameter batch
eng::circuit::tran: drive current has values but the circuit has no such sources
eng::circuit::tran: drive voltage must be finite
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tran.html">
    <span class="label">Previous</span>
    <span class="title">tran</span>
  </a>
  <a class="next-link" href="Start.html">
    <span class="label">Next</span>
    <span class="title">Start</span>
  </a>
</div>
<!-- apiref:nav:end -->
