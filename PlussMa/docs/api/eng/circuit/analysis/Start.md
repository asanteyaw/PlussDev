<!-- generated from Start.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `circuit::Start`

The energy stored in the circuit at the first instant of a transient run.

## Signature

```cpp
circuit::Start
```

## What It Does

Names the capacitor voltages and inductor currents with which `tran` begins. The first sample of the run is made consistent with them: each capacitor is held at its voltage and each inductor at its current while the rest of the circuit, resistive nodes and source currents included, is solved around them.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `capacitor_voltage` | The voltage across each capacitor, positive node minus negative node, `[..., capacitors]`, in the order the capacitors were given to the net. | Undefined: every capacitor starts at zero volts. |
| `inductor_current` | The current through each inductor from its positive node to its negative node, `[..., inductors]`. | Undefined: every inductor starts at zero amperes. |

## Returns

A value; validated by `tran`.

## Errors

A member must be a real floating, finite array on the circuit's platform with one value per element, or none for a circuit without such elements. Leading axes must either be absent, so that every lane starts alike, or equal the circuit's parameter batch. A start that contradicts what the ideal sources impose, such as a capacitor directly across a voltage source at another voltage, has no solution and is reported as an unreachable starting state.

## Interpretation and Use

A circuit that was already running starts from its operating point: pass the voltage across each capacitor and the current through each inductor from `op`. The default describes switching on from rest. A capacitor voltage or inductor current is a state, not a source: it cannot jump, so a node that a capacitor holds keeps its start voltage at the first sample while a resistive node that a source drives reads its full value at once.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A capacitor and an inductor with stored energy.

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
### Example 2

Starting from the operating point, and from rest.

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
  // a divider with a capacitor on its output node
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{2}, {0}, vec({10.0})};
  net.resistors = {{2, 1}, {1, 0}, vec({1000.0, 1000.0})};
  net.capacitors = {{1}, {0}, vec({1e-6})};
  const auto circuit = ckt::make(net);
  const auto operating = ckt::op(circuit);
  ckt::Start settled;
  settled.capacitor_voltage = operating.voltage.select(0, 1).reshape({1});
  out.writeln("from the operating point, node 1 at the first and last sample", flat(ckt::tran(circuit, vec(grid(1e-4, 21)), {}, settled).voltage.select(1, 1)).front(),
              flat(ckt::tran(circuit, vec(grid(1e-4, 21)), {}, settled).voltage.select(1, 1)).back());
  out.writeln("from rest, node 1 at the first and last sample              ", flat(ckt::tran(circuit, vec(grid(1e-4, 21))).voltage.select(1, 1)).front(),
              flat(ckt::tran(circuit, vec(grid(1e-4, 21))).voltage.select(1, 1)).back());
}
```

Output

```text
from the operating point, node 1 at the first and last sample 5.00000 5.00000
from rest, node 1 at the first and last sample               0.00000 4.90964
```
### Example 3

One start per lane, and one start shared by every lane.

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
  net.voltage_sources = {{1}, {0}, vec({0.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  const std::vector<double> time = grid(1e-4, 21);
  // one start voltage per lane, three lanes of a swept circuit
  ckt::Net swept = net;
  swept.resistors.value = vec({1000.0, 1000.0, 1000.0}).reshape({3, 1});
  ckt::Start lanes;
  lanes.capacitor_voltage = vec({1.0, 2.0, 4.0}).reshape({3, 1});
  const auto run = ckt::tran(ckt::make(swept), vec(time), {}, lanes);
  out.writeln("voltage shape", run.voltage.sizes());
  for (int lane = 0; lane < 3; ++lane)
    out.writeln("lane", lane, "start", flat(run.voltage.select(0, lane).select(1, 2))[0], "at 2 ms", flat(run.voltage.select(0, lane).select(1, 2))[20], " closed form", (lane == 0 ? 1.0 : lane == 1 ? 2.0 : 4.0) * std::exp(-2.0));

  // one start voltage shared by every lane
  ckt::Start shared;
  shared.capacitor_voltage = vec({3.0});
  out.writeln("shared start, lane 2 at the first sample", flat(ckt::tran(ckt::make(swept), vec(time), {}, shared).voltage.select(0, 2).select(1, 2))[0]);
}
```

Output

```text
voltage shape [3, 21, 3]
lane 0 start 1.00000 at 2 ms 0.13511  closed form 0.13534
lane 1 start 2.00000 at 2 ms 0.27022  closed form 0.27067
lane 2 start 4.00000 at 2 ms 0.54044  closed form 0.54134
shared start, lane 2 at the first sample 3.00000
```
### Example 4

Rejected starts.

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
  const pluss::Series time = vec({0.0, 1e-3});
  ckt::Start two;
  two.capacitor_voltage = vec({1.0, 2.0});
  report([&] { (void)ckt::tran(circuit, time, {}, two); });
  ckt::Start no_inductor;
  no_inductor.inductor_current = vec({0.1});
  report([&] { (void)ckt::tran(circuit, time, {}, no_inductor); });
  ckt::Start not_finite;
  not_finite.capacitor_voltage = vec({std::nan("")});
  report([&] { (void)ckt::tran(circuit, time, {}, not_finite); });
  ckt::Net across;
  across.nodes = 2;
  across.voltage_sources = {{1}, {0}, vec({5.0})};
  across.resistors = {{1}, {0}, vec({1000.0})};
  across.capacitors = {{1}, {0}, vec({1e-6})};
  ckt::Start wrong;
  wrong.capacitor_voltage = vec({2.0});
  report([&] { (void)ckt::tran(ckt::make(across), time, {}, wrong); });
}
```

Output

```text
eng::circuit::tran: start capacitor_voltage needs one value per element (1), got 2
eng::circuit::tran: start inductor_current has values but the circuit has no such elements
eng::circuit::tran: start capacitor_voltage must be finite
eng::circuit::tran: the starting state cannot be reached; a capacitor connected directly across ideal voltage sources must start at the voltage they impose, an inductor in series with only current sources must start at the current they impose, and every node needs a path to ground (a loop of ideal voltage sources, or two sources fixing the same voltage, also has no solution)
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Drive.html">
    <span class="label">Previous</span>
    <span class="title">Drive</span>
  </a>
  <a class="next-link" href="TranOptions.html">
    <span class="label">Next</span>
    <span class="title">TranOptions</span>
  </a>
</div>
<!-- apiref:nav:end -->
