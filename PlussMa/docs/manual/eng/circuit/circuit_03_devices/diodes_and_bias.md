<!-- generated from diodes_and_bias.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Diodes and bias points

[Back to Circuits 3: Diodes and Transistors](../circuit_03_devices.md)

A resistor obeys a straight line; a diode does not. Its current grows exponentially with the voltage across it, so the equations of a circuit with a diode are **nonlinear** and cannot be solved in one linear step. `circuit::op` solves them by Newton iteration. This page describes a diode, finds the bias point of a diode behind a resistor, sweeps a source to draw the diode's characteristic, shows how the saturation current and the emission coefficient move it, and then shows the controls of the iteration.

## The mental model

A diode between an anode (`positive`) and a cathode (`negative`) carries the current `i = Is (exp(v / (n Vt)) - 1)` from anode to cathode, where `v` is the anode voltage minus the cathode voltage, `Is` is the **saturation current**, `n` is the **ideality** (emission coefficient) and `Vt` is the **thermal voltage**, `25.85 mV` at room temperature. Three facts follow.

- **Forward** (`v` positive and a few `Vt`), the current rises by a factor of `e` for every `n Vt` of extra voltage, so the voltage across a conducting diode barely moves while its current changes by orders of magnitude. For `Is = 1e-14 A` the diode sits near `0.6` to `0.7 V`.
- **Reverse** (`v` negative), the current is `-Is`, almost nothing.
- **A tiny conductance `gmin`** (default `1e-18 S`) is placed across every junction, far below the leakage of any real junction. It is a numerical aid and a knob, not part of the model: set it higher to model a leaky junction.
- **Convergence aids and limits:** a solve that does not converge is retried automatically with a large `gmin` stepped down, and with the sources ramped up from a tenth; and a device current above `max_current` (default `1e6 A`) is an error, because nothing in the circuit limits it.

`op` linearises every diode around the previous iterate, solves the resulting linear circuit, limits the change in each junction voltage so the exponential cannot overflow, and repeats until the update is below `tolerance`. The result is the same `Result` a linear circuit gives, plus the diode currents and the number of iterations.

## A diode behind a resistor

A `5 V` source drives a `1 kohm` resistor in series with a diode of `Is = 1e-14 A`. Kirchhoff's current law gives one equation in the diode voltage `v`: `(5 - v) / 1000 = Is (exp(v / Vt) - 1)`, which has no closed form but a unique root. The program solves the circuit, then checks the current through the resistor against the diode current, and reports how many Newton iterations it took.

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
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.diodes.positive = {2};
  net.diodes.negative = {0};
  net.diodes.saturation = vec({1e-14});

  const auto circuit = ckt::make(net);
  const auto result = ckt::op(circuit);
  const double vd = flat(result.voltage)[2];
  const double id = flat(result.diode_current)[0];
  out.writeln("diodes in the circuit", circuit.diodes());
  out.writeln("diode voltage        ", vd);
  out.writeln("diode current        ", id);
  out.writeln("resistor current     ", (5.0 - vd) / 1000.0);
  out.writeln("source current       ", flat(result.source_current)[0]);
  out.writeln("Newton iterations    ", result.iterations.items<std::int64_t>()[0]);
  out.writeln("residual below 1e-9   ", result.residual.item<double>() < 1e-9);
}
```

Output

```text
diodes in the circuit 1
diode voltage         0.692490
diode current         0.004308
resistor current      0.004308
source current        -0.004308
Newton iterations     12
residual below 1e-9    true
```

The diode settles at `0.6925 V` and carries `4.308 mA`, which is exactly what the resistor carries: `(5 - 0.6925) / 1000`. The source current is `-4.308 mA`, negative because the source delivers current (see [Describing a circuit](../circuit_01_linear_networks/describing_a_circuit.md)). Newton needed `12` iterations from a start with every voltage at zero, and `residual` confirms the nonlinear equations are satisfied: it is the largest error of the circuit equations, evaluated with the exact diode current and not its linearisation. `circuit.diodes()` reports how many diodes the compiled circuit holds.

## Drawing the characteristic

A swept source is a batch, exactly as in the linear chapters: give the source a leading axis and every value is its own solve. The program sweeps the source from `-1 V` to `5 V` across a `100 ohm` resistor and prints the diode voltage and current at each value. Currents are printed in microamps.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const double drive[] = {-1.0, 0.0, 0.5, 1.0, 2.0, 5.0};
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({-1.0, 0.0, 0.5, 1.0, 2.0, 5.0}).reshape({6, 1})};
  net.resistors = {{1}, {2}, vec({100.0})};
  net.diodes.positive = {2};
  net.diodes.negative = {0};
  net.diodes.saturation = vec({1e-12});

  const auto result = ckt::op(ckt::make(net));
  out.writeln("diode_current shape", result.diode_current.sizes(), " (source value, diode)");
  const std::vector<double> volts = flat(result.voltage.select(1, 2));
  const std::vector<double> amps = flat(result.diode_current);
  out.writeln("  source   diode V   diode I (uA)");
  for (int k = 0; k < 6; ++k) out.writeln(drive[k], volts[k], amps[k] * 1e6);
}
```

Output

```text
diode_current shape [6, 1]  (source value, diode)
  source   diode V   diode I (uA)
-1.000000 -1.000000 -0.000001
0.000000 0.000000 0.000000
0.500000 0.485600 143.997775
1.000000 0.573208 4267.919583
2.000000 0.603845 13961.550383
5.000000 0.633321 43666.787832
```

`diode_current` has shape `[6, 1]`: one row per source value, one column per diode. At `-1 V` the diode is reverse biased and carries about `-2e-12 A`, the saturation current plus the leakage across the junction; at `0.5 V` it carries `144 uA`; and from `1 V` to `5 V` its current grows from `4.27 mA` to `43.7 mA`, ten times larger, while the voltage across it moves only from `0.573 V` to `0.633 V`. That near-constant forward voltage is the diode's defining behaviour in a circuit.

## Saturation current and ideality

`Is` and `n` are device values like any other: they take a batch axis. The program gives the same circuit three saturation currents, a decade apart, and then three ideality factors, and prints the diode voltage at the same `5 V` and `1 kohm`.

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
  const auto build = [](pluss::Series saturation, pluss::Series ideality) {
    ckt::Net net;
    net.nodes = 3;
    net.voltage_sources = {{1}, {0}, vec({5.0})};
    net.resistors = {{1}, {2}, vec({1000.0})};
    net.diodes.positive = {2};
    net.diodes.negative = {0};
    net.diodes.saturation = saturation;
    net.diodes.ideality = ideality;
    return net;
  };
  const auto saturation_sweep = ckt::op(ckt::make(build(vec({1e-15, 1e-14, 1e-13}).reshape({3, 1}), vec({1.0}))));
  out.writeln("diode voltage for Is = 1e-15, 1e-14, 1e-13", flat(saturation_sweep.voltage.select(1, 2)));
  const auto ideality_sweep = ckt::op(ckt::make(build(vec({1e-14}), vec({1.0, 1.5, 2.0}).reshape({3, 1}))));
  out.writeln("diode voltage for n = 1, 1.5, 2           ", flat(ideality_sweep.voltage.select(1, 2)));
  out.writeln("change in voltage per decade of Is, and n Vt ln 10 for n = 1", 0.02585 * std::log(10.0));
}
```

Output

```text
diode voltage for Is = 1e-15, 1e-14, 1e-13 [0.75165, 0.69249, 0.63332]
diode voltage for n = 1, 1.5, 2            [0.69249, 1.03552, 1.37605]
change in voltage per decade of Is, and n Vt ln 10 for n = 1 0.05952
```

A decade more saturation current lowers the forward voltage by about `59 mV`, which is `n Vt ln 10 = 59.5 mV` for `n = 1`: the diode needs that much less voltage to carry the same current. The ideality factor stretches the whole curve, so at the same `5 V` and `1 kohm` a diode with `n = 2` sits at `1.376 V` instead of `0.692 V`. Both come from the same call: the batch axis sits on the device values, exactly as it does on resistors.

## Controlling the iteration

`SolveOptions` holds the controls of the Newton iteration. The program solves the same circuit with three tolerances and prints how many iterations each took, then shows what `gmin` does to a reverse-biased diode: the saturation current is `1e-14 A`, and `gmin` adds its own leakage `gmin x v` across the junction. The default is small enough to leave the saturation current, and larger values model a leaky junction.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static ckt::Net bias(double volts) {
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({volts})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.diodes.positive = {2};
  net.diodes.negative = {0};
  net.diodes.saturation = vec({1e-14});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const auto forward = ckt::make(bias(5.0));
  for (double tolerance : {1e-3, 1e-6, 1e-10}) {
    ckt::SolveOptions options;
    options.tolerance = tolerance;
    const auto result = ckt::op(forward, options);
    out.writeln("tolerance 1e", static_cast<int>(std::lround(std::log10(tolerance))), " iterations", result.iterations.items<std::int64_t>()[0], " diode voltage", flat(result.voltage)[2]);
  }
  out.print_defaults(PrintOptions{});
  const auto reverse = ckt::make(bias(-5.0));
  for (double gmin : {0.0, 1e-12, 1e-9}) {
    ckt::SolveOptions options;
    options.gmin = gmin;
    out.writeln("gmin", gmin, " reverse diode current", flat(ckt::op(reverse, options).diode_current)[0]);
  }
}
```

Output

```text
tolerance 1e -3  iterations 9  diode voltage 0.69249039
tolerance 1e -6  iterations 11  diode voltage 0.69249038
tolerance 1e -10  iterations 12  diode voltage 0.69249038
gmin 0  reverse diode current -1e-14
gmin 1e-12  reverse diode current -5.00999999499e-12
gmin 1e-09  reverse diode current -5.0000049999950006e-09
```

A looser tolerance stops sooner (`9`, `11` and `12` iterations) and the answer differs only in the last printed digits, which shows how fast Newton converges once it is close: each iteration roughly doubles the number of correct digits. For the reverse-biased diode, with `gmin` zero the current is exactly the saturation current, `-1e-14 A`; at `1e-12 S` the leakage dominates and the current is `-5.01e-12 A`, five hundred times larger; at `1e-9 S` it is `-5e-9 A`. The default, `1e-18 S`, leaves the current at the saturation current to four digits.

## Errors

An unreachable bias, a description the compiler rejects and an out-of-range control are all reported with a message. The program allows too few iterations, asks for a non-positive thermal voltage, and then gives a diode a zero saturation current and a diode with its two terminals on the same node.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }

static ckt::Net bias() {
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.diodes.positive = {2};
  net.diodes.negative = {0};
  net.diodes.saturation = vec({1e-14});
  return net;
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
  const auto circuit = ckt::make(bias());
  ckt::SolveOptions few;
  few.max_iterations = 3;
  report([&] { (void)ckt::op(circuit, few); });
  ckt::SolveOptions cold;
  cold.thermal_voltage = 0.0;
  report([&] { (void)ckt::op(circuit, cold); });
  auto no_saturation = bias();
  no_saturation.diodes.saturation = vec({0.0});
  report([&] { (void)ckt::make(no_saturation); });
  auto same_node = bias();
  same_node.diodes.negative = {2};
  report([&] { (void)ckt::make(same_node); });
}
```

Output

```text
eng::circuit::op: the Newton iteration did not converge in 3 iterations (last update 4.59); raise max_iterations, loosen tolerance, or check that the bias is reachable (gmin stepping and source stepping were also tried)
eng::circuit::op: solver thermal_voltage must be positive
eng::circuit::make: diode saturation currents values must be positive
eng::circuit::make: diodes cannot connect a node to itself
```

Each message names the cause: the iteration limit, the invalid control, the value that must be positive, and the diode that connects a node to itself.

## Decisions

- **A bias point or an operating point of a circuit with diodes:** `op`, which solves the nonlinear equations; nothing else changes about how the result is read.
- **A characteristic or a family of curves:** a batch on the source, on `Is` or on `n`, one call.
- **A reverse leakage that matters:** raise `gmin` to the leakage conductance of your device; the default `1e-18 S` leaves the saturation current.
- **Convergence trouble:** raise `max_iterations` before loosening `tolerance`, and check that the bias is reachable (a diode driven past the supply voltage has no solution).

## Pitfalls

The program makes two mistakes on the diode circuit: connecting the diode backwards, and putting an ideal source straight across a diode.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.diodes.saturation = vec({1e-14});

  // 1. anode and cathode swapped: the diode blocks and the node follows the source
  net.diodes.positive = {0};
  net.diodes.negative = {2};
  const auto backwards = ckt::op(ckt::make(net));
  out.writeln("diode node voltage with the diode backwards", flat(backwards.voltage)[2]);

  // 2. an ideal source straight across a diode: nothing limits the current, so the solver refuses the answer
  ckt::Net across;
  across.nodes = 2;
  across.voltage_sources = {{1}, {0}, vec({5.0})};
  across.diodes.positive = {1};
  across.diodes.negative = {0};
  across.diodes.saturation = vec({1e-14});
  try {
    (void)ckt::op(ckt::make(across));
  } catch (const pluss::Error &e) {
    out.writeln(e.message());
  }
}
```

Output

```text
diode node voltage with the diode backwards 4.999999999989995
eng::circuit::op: a device carries 1.01e+70 A, more than max_current (1e+06 A); a junction driven directly by an ideal source has nothing to limit its current, so add a series resistance (or raise max_current)
```

With the diode backwards the node follows the source to within `5e-9 V`: the diode blocks and no current flows through the resistor. With the `5 V` source directly across the diode nothing limits the current (the model would carry `1e70 A`), so `op` refuses it with a message that names the cause and the remedy: put a resistance in series, as a real circuit always has one, or raise `max_current` if the huge current is what you meant.

## Where next

[The transistor amplifier](transistor_amplifier.md) adds a three-terminal device and the small-signal response around a bias point. [The rectifier in time](rectifier_transient.md) puts a diode into `tran`. Reference pages: [`op`](../../../../api/eng/circuit/analysis/op.md), [`SolveOptions`](../../../../api/eng/circuit/analysis/SolveOptions.md), [`Diodes`](../../../../api/eng/circuit/model/Diodes.md), [`Result`](../../../../api/eng/circuit/analysis/Result.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="transistor_amplifier.html">
    <span class="label">Next</span>
    <span class="title">The transistor amplifier</span>
  </a>
</div>
<!-- apiref:nav:end -->
