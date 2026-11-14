<!-- generated from step_response.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Step response and starting state

[Back to Circuits 2: Transient Response](../circuit_02_transient.md)

You close a switch, or a source changes, and you want to know what the capacitor voltages and inductor currents do next: how fast a node charges, how long a current takes to build, how much a charged capacitor still holds after a while. `circuit::tran` computes the **response in time**: every node voltage and branch current at a list of equally spaced instants, starting from a state you choose. This page works through an RC and an RL circuit against their closed forms, starts a circuit from a charged capacitor and from the DC operating point, and sweeps a component.

## The mental model

A circuit with capacitors and inductors obeys one linear equation, `E x' + F x = u`, where `x` collects the node voltages and the branch currents, `F` is the same matrix that `op` solves, `E` holds the capacitances and inductances, and `u` holds the sources. `tran` marches that equation forward by a fixed step `h`, the spacing of the `time` vector you give it. Between two samples every voltage and current is assumed to move smoothly; across the whole run the error shrinks with the square of `h` (see [Schemes and step size](schemes_and_step_size.md)).

Three rules make the result predictable.

- **The time vector is the clock.** It must be a real vector of at least two strictly increasing, evenly spaced values; the step is `time[1] - time[0]`. The first value does not have to be zero.
- **The first sample is the starting state.** It is the instant the run begins. The sources are at their full value from that instant (a step), and each capacitor and inductor holds the value in `Start`. An undefined member of `Start` means *at rest*: capacitor voltages and inductor currents of zero. The first sample is made consistent with those values: a resistive node that a source drives reads its final voltage immediately, while a capacitor cannot jump.
- **Every array gains a time axis.** The axes of each result are `[..., time, elements]`: the time axis sits just before the last axis, after any batch axes, exactly as the frequency axis does in `ac`. For one circuit, `voltage` has shape `[time samples, nodes]`.

## An RC step response

A `5 V` source drives a `1 kohm` resistor in series with a `1 microfarad` capacitor to ground, and the output is the capacitor voltage. The time constant is `tau = R C = 1 ms`, and the closed form for a capacitor that starts empty is `v(t) = 5 (1 - exp(-t / tau))`. The program runs `5 ms` in steps of `0.1 ms`, prints the output at every millisecond beside the closed form, and reports the first sample.

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
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};

  std::vector<double> time;
  for (int k = 0; k <= 50; ++k) time.push_back(k * 1e-4);
  const auto run = ckt::tran(ckt::make(net), vec(time));

  out.writeln("voltage shape", run.voltage.sizes(), " (time sample, node)");
  out.writeln("first sample (ground, source node, capacitor)", flat(run.voltage.select(0, 0)));
  const std::vector<double> output = flat(run.voltage.select(1, 2));
  out.writeln("  ms    tran   closed form");
  for (int ms = 0; ms <= 5; ++ms) {
    const int k = ms * 10;
    out.writeln(ms, output[k], 5.0 * (1.0 - std::exp(-time[k] / 1e-3)));
  }
}
```

Output

```text
voltage shape [51, 3]  (time sample, node)
first sample (ground, source node, capacitor) [0.00000, 5.00000, 0.00000]
  ms    tran   closed form
0 0.00000 0.00000
1 3.16214 3.16060
2 4.32445 4.32332
3 4.75169 4.75106
4 4.90873 4.90842
5 4.96645 4.96631
```

The capacitor voltage climbs from `0` towards `5 V`, `3.16 V` after one time constant (`63.2 %` of the way) and within two thousandths of a volt of the closed form at every millisecond. The first sample is the consistent starting state: the source node already reads `5 V` and the capacitor still `0 V`.

## An RL step response

A current in an inductor cannot jump, so it builds up gradually. A `5 V` source drives a `10 ohm` resistor in series with a `10 mH` inductor to ground. The time constant is `tau = L / R = 1 ms` and the inductor current follows `i(t) = (5 / 10) (1 - exp(-t / tau))`. The source current of a delivering source is negative (see [Describing a circuit](../circuit_01_linear_networks/describing_a_circuit.md)), so it mirrors the inductor current with the opposite sign.

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
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.resistors = {{1}, {2}, vec({10.0})};
  net.inductors = {{2}, {0}, vec({0.01})};

  std::vector<double> time;
  for (int k = 0; k <= 50; ++k) time.push_back(k * 1e-4);
  const auto run = ckt::tran(ckt::make(net), vec(time));

  const std::vector<double> current = flat(run.inductor_current);
  const std::vector<double> source = flat(run.source_current);
  const std::vector<double> middle = flat(run.voltage.select(1, 2));   // the node between the resistor and the inductor
  out.writeln("  ms    inductor   closed form   source current   node 2");
  for (int ms = 0; ms <= 5; ++ms) {
    const int k = ms * 10;
    out.writeln(ms, current[k], 0.5 * (1.0 - std::exp(-time[k] / 1e-3)), source[k], middle[k]);
  }
}
```

Output

```text
  ms    inductor   closed form   source current   node 2
0 0.00000 0.00000 0.00000 5.00000
1 0.31621 0.31606 -0.31621 1.83786
2 0.43245 0.43233 -0.43245 0.67555
3 0.47517 0.47511 -0.47517 0.24831
4 0.49087 0.49084 -0.49087 0.09127
5 0.49665 0.49663 -0.49665 0.03355
```

The current rises as `0.5 (1 - exp(-t / tau))`, reaching `0.316 A` after one time constant, and the source current is its negative at every instant, because the resistor and the inductor carry the same current. The node between them starts at the full `5 V` (an inductor refuses a sudden change in current, so at first the resistor carries none and drops nothing) and falls to `0.03 V` by `5 ms` as the inductor current settles and its voltage vanishes.

## Starting from a charged capacitor

`Start` names the energy stored at the first sample: `capacitor_voltage` holds one value per capacitor, in the order the capacitors were given to `Net`, and `inductor_current` one per inductor. With the source off (`0 V`) and the capacitor charged to `4 V`, the capacitor discharges through the resistor: `v(t) = 4 exp(-t / tau)`, which is `4 / e = 1.4715 V` after one time constant.

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
  net.voltage_sources = {{1}, {0}, vec({0.0})};   // the source is off
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};

  ckt::Start start;
  start.capacitor_voltage = vec({4.0});
  std::vector<double> time;
  for (int k = 0; k <= 30; ++k) time.push_back(k * 1e-4);
  const auto run = ckt::tran(ckt::make(net), vec(time), {}, start);

  const std::vector<double> output = flat(run.voltage.select(1, 2));
  out.writeln("capacitor voltage at t = 0     ", output[0]);
  out.writeln("capacitor voltage at t = tau   ", output[10], " closed form", 4.0 * std::exp(-1.0));
  out.writeln("capacitor voltage at t = 3 tau ", output[30], " closed form", 4.0 * std::exp(-3.0));
  out.writeln("source current at t = 0        ", flat(run.source_current)[0], " (the capacitor pushes 4 mA out through the resistor)");
}
```

Output

```text
capacitor voltage at t = 0      4.00000
capacitor voltage at t = tau    1.47029  closed form 1.47152
capacitor voltage at t = 3 tau  0.19865  closed form 0.19915
source current at t = 0         0.00400  (the capacitor pushes 4 mA out through the resistor)
```

The starting voltage is honoured exactly at the first sample and decays by `1 / e` per time constant. At `t = 0` the capacitor, held at `4 V`, drives `4 mA` through the resistor into the source branch, so the source current reads `+0.004`: a source that absorbs current has a positive current in this convention.

An inductor works the same way. `start.inductor_current = vec({0.25})` makes an inductor begin with `0.25 A` flowing from its positive node to its negative node, and the run then shows that current relaxing to the value the sources impose.

## Starting from the operating point

A circuit that has been on for a long time sits at its DC operating point. To simulate a disturbance of such a circuit, start every capacitor at its operating-point voltage and every inductor at its operating-point current; the circuit then stays exactly there until something changes. The program computes `op` for a divider that has a capacitor on its output, passes the capacitor's voltage as `Start`, and runs. A second run from rest shows what the default does instead.

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
  net.nodes = 3;
  net.voltage_sources = {{2}, {0}, vec({10.0})};
  net.resistors = {{2, 1}, {1, 0}, vec({1000.0, 1000.0})};
  net.capacitors = {{1}, {0}, vec({1e-6})};
  const auto circuit = ckt::make(net);

  const auto operating = ckt::op(circuit);
  ckt::Start settled;
  settled.capacitor_voltage = operating.voltage.select(0, 1).reshape({1});   // node 1 carries the capacitor

  std::vector<double> time;
  for (int k = 0; k <= 20; ++k) time.push_back(k * 1e-4);
  const auto kept = ckt::tran(circuit, vec(time), {}, settled);
  const auto from_rest = ckt::tran(circuit, vec(time));

  out.writeln("operating point, node 1       ", flat(operating.voltage)[1]);
  out.writeln("from the operating point      ", flat(kept.voltage.select(1, 1)).front(), flat(kept.voltage.select(1, 1)).back());
  out.writeln("from rest                     ", flat(from_rest.voltage.select(1, 1)).front(), flat(from_rest.voltage.select(1, 1)).back());
}
```

Output

```text
operating point, node 1        5.00000
from the operating point       5.00000 5.00000
from rest                      0.00000 4.90964
```

Started from the operating point, node 1 stays at `5 V` for the whole run: the circuit is already at rest in the sense that matters. Started from the default, the same node begins at `0 V` and charges towards `5 V`. Both are correct answers to different questions: the default asks what happens when the sources are switched on at the first sample, the operating-point start asks what happens to a circuit that was already running.

## A sweep of component values

Batched component values give a whole family of responses in one call, exactly as in `op` and `ac`. The program gives the resistor three values, `0.5`, `1` and `2 kohm`, so the time constants are `0.5`, `1` and `2 ms`. The result has shape `[3, 51, 3]`: component value, time sample, node. The output voltage is printed at `1 ms` for each lane.

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
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.resistors = {{1}, {2}, vec({500.0, 1000.0, 2000.0}).reshape({3, 1})};
  net.capacitors = {{2}, {0}, vec({1e-6})};

  std::vector<double> time;
  for (int k = 0; k <= 50; ++k) time.push_back(k * 1e-4);
  const auto run = ckt::tran(ckt::make(net), vec(time));

  out.writeln("voltage shape", run.voltage.sizes(), " (resistor value, time sample, node)");
  const pluss::Series at_1ms = run.voltage.select(1, 10).select(1, 2);   // shape [3]
  const double taus[3] = {0.5e-3, 1e-3, 2e-3};
  for (int lane = 0; lane < 3; ++lane)
    out.writeln("tau", taus[lane], " output at 1 ms", flat(at_1ms)[lane], " closed form", 5.0 * (1.0 - std::exp(-1e-3 / taus[lane])));
}
```

Output

```text
voltage shape [3, 51, 3]  (resistor value, time sample, node)
tau 0.00050  output at 1 ms 4.32785  closed form 4.32332
tau 0.00100  output at 1 ms 3.16214  closed form 3.16060
tau 0.00200  output at 1 ms 1.96766  closed form 1.96735
```

Only the resistor carries a batch axis; the source and the capacitor are shared. Each lane is an independent run with its own time constant, and the lane with the smallest `tau` has charged furthest by `1 ms`.

## Errors

The time vector is checked before anything is solved, and so are the start values. The last call places a capacitor directly across an ideal source: from rest the capacitor would have to start at `0 V` while the source holds the node at `5 V`, which no circuit can do, and the error says so.

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

  report([&] { (void)ckt::tran(circuit, vec({0.0})); });
  report([&] { (void)ckt::tran(circuit, vec({0.0, 1e-3, 3e-3})); });
  report([&] { (void)ckt::tran(circuit, vec({1e-3, 0.0})); });
  ckt::Start two;
  two.capacitor_voltage = vec({1.0, 2.0});
  report([&] { (void)ckt::tran(circuit, vec({0.0, 1e-3}), {}, two); });

  // a capacitor directly across the source cannot start at a voltage the source does not impose
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
eng::circuit::tran: start capacitor_voltage needs one value per element (1), got 2
eng::circuit::tran: the starting state cannot be reached; a capacitor connected directly across ideal voltage sources must start at the voltage they impose, an inductor in series with only current sources must start at the current they impose, and every node needs a path to ground (a loop of ideal voltage sources, or two sources fixing the same voltage, also has no solution)
```

Each message names the call's problem: too few samples, an uneven grid, a decreasing grid, a start vector with two values for one capacitor, and the impossible starting state of the capacitor across the source.

## Decisions

- **Switching on a circuit, or a source step:** the default `Start` (at rest) with the source at its final value.
- **A disturbance of a circuit that was already running:** `op` first, then pass the capacitor voltages and inductor currents it implies as `Start`.
- **A circuit with stored energy and no source:** a `Start` with the stored voltages and currents; the sources may be zero.
- **Several component values:** a batch on the components, one call.
- **How long to run and how fine to step:** a few time constants of the slowest part of the circuit, and a step of a tenth of the fastest time constant you need to see, or smaller; see [Schemes and step size](schemes_and_step_size.md).

## Pitfalls

The program makes three mistakes on the RC circuit with a `5 V` source: expecting the circuit to start at its DC value when no `Start` is given, giving capacitor voltages in a different order from the capacitors, and reading the time axis as the last axis instead of the one before it.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  // two capacitors in an RC ladder: node 2 through 1 kohm from the source node, node 3 through 1 kohm from node 2
  ckt::Net net;
  net.nodes = 4;
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.resistors = {{1, 2}, {2, 3}, vec({1000.0, 1000.0})};
  net.capacitors = {{3, 2}, {0, 0}, vec({1e-6, 1e-6})};   // capacitor 0 is at node 3, capacitor 1 at node 2
  const auto circuit = ckt::make(net);
  std::vector<double> time;
  for (int k = 0; k <= 10; ++k) time.push_back(k * 1e-4);

  // 1. no Start: the run begins at rest, not at the DC value
  const auto rest = ckt::tran(circuit, vec(time));
  out.writeln("default start, node 3 at t = 0 and the DC value", flat(rest.voltage.select(1, 3)).front(), flat(ckt::op(circuit).voltage)[3]);

  // 2. capacitor voltages follow the order of the capacitors in the Net, not the order of the nodes
  ckt::Start in_capacitor_order;   // node 2 at 5 V and node 3 at 0 V
  in_capacitor_order.capacitor_voltage = vec({0.0, 5.0});   // capacitor 0 (node 3), capacitor 1 (node 2)
  ckt::Start in_node_order;
  in_node_order.capacitor_voltage = vec({5.0, 0.0});        // the same values written in node order
  const auto first = [&](const ckt::Start &start) { return flat(ckt::tran(circuit, vec(time), {}, start).voltage.select(0, 0)); };
  out.writeln("node 2 and node 3 at t = 0, in capacitor order", first(in_capacitor_order)[2], first(in_capacitor_order)[3]);
  out.writeln("node 2 and node 3 at t = 0, in node order     ", first(in_node_order)[2], first(in_node_order)[3]);

  // 3. the time axis precedes the node axis
  out.writeln("voltage shape", rest.voltage.sizes(), "(time sample, node); select(0, k) is the instant k, select(1, n) is the node n");
}
```

Output

```text
default start, node 3 at t = 0 and the DC value 0.0000 5.0000
node 2 and node 3 at t = 0, in capacitor order 5.0000 0.0000
node 2 and node 3 at t = 0, in node order      0.0000 5.0000
voltage shape [11, 4] (time sample, node); select(0, k) is the instant k, select(1, n) is the node n
```

Without a `Start`, node 3 begins at `0 V` even though its DC value is `5 V`; the run is a switch-on, not a continuation. The two `Start` calls place the same numbers on different capacitors: written in capacitor order (capacitor 0 is at node 3) the start puts `5 V` on node 2 and `0 V` on node 3, and written in node order it puts them on the wrong capacitors, with node 2 at `0 V` and node 3 at `5 V`. The shape line shows that the time axis comes first.

## Where next

[Time-varying sources](time_varying_sources.md) drives the same circuits with sinusoids, pulses and ramps through `Drive`. [Schemes and step size](schemes_and_step_size.md) explains the accuracy of the two integrators and when each is the right one. Reference pages: [`tran`](../../../../api/eng/circuit/analysis/tran.md), [`Start`](../../../../api/eng/circuit/analysis/Start.md), [`Result`](../../../../api/eng/circuit/analysis/Result.md), [`op`](../../../../api/eng/circuit/analysis/op.md), [`make`](../../../../api/eng/circuit/model/make.md), [`Net`](../../../../api/eng/circuit/model/Net.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="time_varying_sources.html">
    <span class="label">Next</span>
    <span class="title">Time-varying sources</span>
  </a>
</div>
<!-- apiref:nav:end -->
