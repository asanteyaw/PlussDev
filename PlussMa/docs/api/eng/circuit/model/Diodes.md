<!-- generated from Diodes.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `circuit::Diodes`

Junction diodes of a circuit.

## Signature

```cpp
circuit::Diodes
```

## What It Does

Describes diodes by their two terminals and their model values. A diode carries the current `i = Is (exp(v / (n Vt)) - 1)` from `positive` (the anode) to `negative` (the cathode), where `v` is the anode voltage minus the cathode voltage, `Is` is the saturation current, `n` the ideality factor and `Vt` the thermal voltage of `SolveOptions`. A tiny conductance `gmin`, also from `SolveOptions`, is added across every junction. Circuits with diodes are solved by Newton iteration in `op`, `ac` (around an operating point) and `tran`.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `positive` | The anode node of each diode. | empty |
| `negative` | The cathode node of each diode; it must differ from the anode. | empty |
| `saturation` | The saturation current `Is` of each diode in amperes, `[..., diodes]`; positive and finite. | required |
| `ideality` | The emission coefficient `n` of each diode, `[..., diodes]`; positive and finite. | `1` |
| `junction_capacitance` | The zero-bias depletion capacitance `Cj0` in farads, `[..., diodes]`; non-negative and finite. Constant (a linear capacitor appended to the net's capacitors) unless `junction_potential` is given. | undefined: none |
| `junction_potential` | The junction potential `Vj` in volts, `[..., diodes]`; positive. Makes the depletion capacitance voltage dependent, `Cj0 / (1 - v / Vj)^m`, continued linearly above `Vj / 2`. | undefined: constant capacitance |
| `grading` | The grading coefficient `m`, `[..., diodes]`, in `[0, 1)`. | `0.5` |
| `transit_time` | The transit time `tau` in seconds, `[..., diodes]`; non-negative. Adds the diffusion charge `tau i`, a capacitance `tau g`. | undefined: none |

## Returns

A value; validated by `make`.

## Errors

`make` throws `pluss::Error` for an anode or cathode outside the circuit, a diode with both terminals on the same node, endpoint lists of different lengths, a missing, non-positive or non-finite `saturation` or `ideality`, values of a different dtype or platform than the other groups, or values with an incompatible batch axis.

## Interpretation and Use

Forward voltage rises by `n Vt ln 10`, about `60 mV` for `n = 1`, for each tenfold increase in current, so a conducting silicon diode sits near `0.6` to `0.7 V` whatever the current. Both `saturation` and `ideality` take leading batch axes like any other value, so a family of devices is one call. The diode current appears in `Result::diode_current`.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A diode behind a resistor and its bias point.

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
  // 5 V source, 1 kohm, silicon diode to ground
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.diodes.positive = {2};
  net.diodes.negative = {0};
  net.diodes.saturation = vec({1e-14});

  const auto result = ckt::op(ckt::make(net));
  const double vd = flat(result.voltage)[2];
  const double id = flat(result.diode_current)[0];
  out.writeln("diode voltage     ", vd);
  out.writeln("diode current     ", id);
  out.writeln("resistor current  ", (5.0 - vd) / 1000.0);
  out.writeln("Newton iterations ", result.iterations.items<std::int64_t>()[0]);
  out.writeln("residual below 1e-9", result.residual.item<double>() < 1e-9);
}
```

Output

```text
diode voltage      0.692490
diode current      0.004308
resistor current   0.004308
Newton iterations  12
residual below 1e-9 true
```
### Example 2

A swept source: the diode characteristic.

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
  // a swept source is a batch: each lane is its own solve
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
  const double drive[] = {-1.0, 0.0, 0.5, 1.0, 2.0, 5.0};
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
### Example 3

The ideality factor widens the knee.

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
  // the emission coefficient widens the knee: a larger n needs a larger voltage for the same current
  for (double n : {1.0, 1.5, 2.0}) {
    ckt::Net net;
    net.nodes = 3;
    net.voltage_sources = {{1}, {0}, vec({5.0})};
    net.resistors = {{1}, {2}, vec({1000.0})};
    net.diodes.positive = {2};
    net.diodes.negative = {0};
    net.diodes.saturation = vec({1e-14});
    net.diodes.ideality = vec({n});
    const auto result = ckt::op(ckt::make(net));
    out.writeln("ideality", n, " diode voltage", flat(result.voltage)[2], " current", flat(result.diode_current)[0]);
  }
}
```

Output

```text
ideality 1.00000  diode voltage 0.69249  current 0.00431
ideality 1.50000  diode voltage 1.03552  current 0.00396
ideality 2.00000  diode voltage 1.37605  current 0.00362
```
### Example 4

A junction capacitance in a transient.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // a reverse-biased diode blocks, so its junction capacitance is all the node sees: a 5 V step through 1 kohm charges it
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.diodes.positive = {0};
  net.diodes.negative = {2};
  net.diodes.saturation = vec({1e-14});
  net.diodes.junction_capacitance = vec({1e-6});
  std::vector<double> time;
  for (int k = 0; k <= 30; ++k) time.push_back(k * 1e-4);
  const auto run = ckt::tran(ckt::make(net), vec(time));
  const std::vector<double> node = flat(run.voltage.select(1, 2));
  out.writeln("  ms   node     closed form");
  for (int ms = 0; ms <= 3; ++ms) out.writeln(ms, node[ms * 10], 5.0 * (1.0 - std::exp(-1.0 * ms)));
}
```

Output

```text
  ms   node     closed form
0 0.00000 0.00000
1 3.16214 3.16060
2 4.32445 4.32332
3 4.75169 4.75106
```
### Example 5

Capacitors in parallel start at the same voltage.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // capacitors in parallel must start at the same voltage: the disagreement is reported
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({0.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  net.diodes.positive = {0};
  net.diodes.negative = {2};
  net.diodes.saturation = vec({1e-14});
  net.diodes.junction_capacitance = vec({2e-6});
  const auto circuit = ckt::make(net);
  std::vector<double> time;
  for (int k = 0; k <= 20; ++k) time.push_back(k * 1e-4);
  ckt::Start agree;
  agree.capacitor_voltage = vec({2.0, -2.0});   // node 2 starts 2 V above ground: the net capacitor (node 2 to ground) holds 2 V, the diode capacitor (ground to node 2) -2 V
  const auto run = ckt::tran(circuit, vec(time), {}, agree);
  out.writeln("node 2 starts at", flat(run.voltage.select(1, 2))[0], " and relaxes to", flat(run.voltage.select(1, 2)).back());
  ckt::Start disagree;
  disagree.capacitor_voltage = vec({1.0, 2.0});
  try {
    (void)ckt::tran(circuit, vec(time), {}, disagree);
  } catch (const pluss::Error &e) {
    out.writeln(e.message());
  }
}
```

Output

```text
node 2 starts at 2.00000  and relaxes to 1.02677
eng::circuit::tran: the start voltages of capacitors that share the same nodes disagree (parallel capacitors must start at the same voltage), or a capacitor directly across ideal sources starts at another voltage
```
### Example 6

The reverse bias sets the depletion capacitance and the AC corner.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static ckt::Net junction(double source, double cj0) {
  // a source behind 1 kohm drives a diode (anode at node 2, cathode at ground) with a voltage-dependent junction capacitance
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({source})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.diodes.positive = {2};
  net.diodes.negative = {0};
  net.diodes.saturation = vec({1e-14});
  net.diodes.junction_capacitance = vec({cj0});
  net.diodes.junction_potential = vec({0.7});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // C(v) = Cj0 (1 - v / Vj)^-m with m = 0.5: the reverse bias sets the capacitance, and with it the low-pass corner
  for (double bias : {-1.0, -3.0, -10.0}) {
    const auto at_bias = ckt::op(ckt::make(junction(bias, 1e-6)));
    const double c = 1e-6 / std::sqrt(1.0 - bias / 0.7);
    const double corner = 1.0 / (1000.0 * c);
    const auto response = ckt::ac(ckt::make(junction(1.0, 1e-6)), vec({corner}), at_bias);
    out.writeln("reverse bias", bias, " V  capacitance (uF)", c * 1e6, " corner (rad/s)", corner, " gain at the corner", flat(response.voltage.select(-1, 2).abs())[0], "(1 / sqrt 2 = 0.70711)");
  }
}
```

Output

```text
reverse bias -1.00000  V  capacitance (uF) 0.64169  corner (rad/s) 1558.38744  gain at the corner 0.70711 (1 / sqrt 2 = 0.70711)
reverse bias -3.00000  V  capacitance (uF) 0.43496  corner (rad/s) 2299.06813  gain at the corner 0.70711 (1 / sqrt 2 = 0.70711)
reverse bias -10.00000  V  capacitance (uF) 0.25577  corner (rad/s) 3909.69491  gain at the corner 0.70711 (1 / sqrt 2 = 0.70711)
```
### Example 7

The time constant of a voltage-dependent junction.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static ckt::Net junction(double source, double cj0) {
  // a source behind 1 kohm drives a diode (anode at node 2, cathode at ground) with a voltage-dependent junction capacitance
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({source})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.diodes.positive = {2};
  net.diodes.negative = {0};
  net.diodes.saturation = vec({1e-14});
  net.diodes.junction_capacitance = vec({cj0});
  net.diodes.junction_potential = vec({0.7});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  // a small step on a reverse-biased junction charges C(bias) through 1 kohm: tau = R Cj0 / sqrt(1 - v / Vj)
  const double c = 1e-6 / std::sqrt(1.0 + 3.0 / 0.7), tau = 1000.0 * c;
  std::vector<double> time, wave;
  for (int k = 0; k <= 40; ++k) {
    time.push_back(k * tau / 10.0);
    wave.push_back(k == 0 ? -3.0 : -2.999);
  }
  ckt::Drive step;
  step.voltage = vec(wave).reshape({41, 1});
  const auto run = ckt::tran(ckt::make(junction(-3.0, 1e-6)), vec(time), step);
  const std::vector<double> node = flat(run.voltage.select(1, 2));
  out.writeln("time constant (ms)", tau * 1e3);
  out.writeln("  t / tau   node - (-3 V) in mV   closed form");
  for (int k : {10, 20, 30, 40}) out.writeln(k / 10.0, (node[k] + 3.0) * 1e3, 1.0 * (1.0 - std::exp(-k / 10.0)));
}
```

Output

```text
time constant (ms) 0.434959
  t / tau   node - (-3 V) in mV   closed form
1.000000 0.613064 0.632121
2.000000 0.857758 0.864665
3.000000 0.947709 0.950213
4.000000 0.980777 0.981684
```
### Example 8

Diffusion charge.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // diffusion charge tau i makes a forward-biased diode an admittance g (1 + j w tau): its AC response against the formula
  const auto build = [](double volts) {
    ckt::Net net;
    net.nodes = 3;
    net.voltage_sources = {{1}, {0}, vec({volts})};
    net.resistors = {{1}, {2}, vec({1000.0})};
    net.diodes.positive = {2};
    net.diodes.negative = {0};
    net.diodes.saturation = vec({1e-14});
    net.diodes.transit_time = vec({1e-6});
    return ckt::make(net);
  };
  const auto bias = ckt::op(build(5.0));
  const double g = flat(bias.diode_current)[0] / 0.02585;
  out.writeln("diode conductance g (mS)", g * 1e3);
  for (double w : {1e3, 1e5, 1e6}) {
    const auto response = ckt::ac(build(1.0), vec({w}), bias).voltage.select(-1, 2);
    const std::complex<double> y(g, w * 1e-6 * g), expected = 1.0 / (1.0 + 1000.0 * y);
    out.writeln("omega", w, " |gain|", flat(response.abs())[0], " formula", std::abs(expected));
  }
}
```

Output

```text
diode conductance g (mS) 166.63480
omega 1000.00000  |gain| 0.00597  formula 0.00597
omega 100000.00000  |gain| 0.00594  formula 0.00594
omega 1000000.00000  |gain| 0.00423  formula 0.00423
```
### Example 9

Switching a junction with stored charge.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  // switching a forward junction: the stored charge (depletion plus diffusion) delays the rise of the node voltage
  const auto build = [](bool with_charge) {
    ckt::Net net;
    net.nodes = 3;
    net.voltage_sources = {{1}, {0}, vec({3.0})};
    net.resistors = {{1}, {2}, vec({1000.0})};
    net.diodes.positive = {2};
    net.diodes.negative = {0};
    net.diodes.saturation = vec({1e-14});
    if (with_charge) {
      net.diodes.junction_capacitance = vec({1e-9});
      net.diodes.junction_potential = vec({0.7});
      net.diodes.transit_time = vec({1e-7});
    }
    return ckt::make(net);
  };
  std::vector<double> time, wave;
  for (int k = 0; k <= 80; ++k) {
    time.push_back(k * 2e-8);
    wave.push_back(k < 20 ? 0.0 : 3.0);
  }
  ckt::Drive drive;
  drive.voltage = vec(wave).reshape({81, 1});
  const auto ideal = flat(ckt::tran(build(false), vec(time), drive).voltage.select(1, 2));
  const auto stored = flat(ckt::tran(build(true), vec(time), drive).voltage.select(1, 2));
  out.writeln("  t (us)   without charge   with charge");
  for (int k : {19, 20, 22, 25, 30, 40, 60, 80}) out.writeln(time[k] * 1e6, ideal[k], stored[k]);
}
```

Output

```text
  t (us)   without charge   with charge
0.3800 0.0000 0.0000
0.4000 0.6765 0.0294
0.4400 0.6765 0.1387
0.5000 0.6765 0.2785
0.6000 0.6765 0.4591
0.8000 0.6765 0.6512
1.2000 0.6765 0.6758
1.6000 0.6765 0.6765
```
### Example 10

Rejected charge parameters.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
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
  const auto base = [] {
    ckt::Net net;
    net.nodes = 3;
    net.voltage_sources = {{1}, {0}, vec({5.0})};
    net.resistors = {{1}, {2}, vec({1000.0})};
    net.diodes.positive = {2};
    net.diodes.negative = {0};
    net.diodes.saturation = vec({1e-14});
    net.diodes.junction_capacitance = vec({1e-9});
    return net;
  };
  auto potential = base();
  potential.diodes.junction_potential = vec({-0.7});
  report([&] { (void)ckt::make(potential); });
  auto grading = base();
  grading.diodes.junction_potential = vec({0.7});
  grading.diodes.grading = vec({1.0});
  report([&] { (void)ckt::make(grading); });
  auto transit = base();
  transit.diodes.transit_time = vec({-1e-9});
  report([&] { (void)ckt::make(transit); });
  auto capacitance = base();
  capacitance.diodes.junction_capacitance = vec({-1e-9});
  report([&] { (void)ckt::make(capacitance); });
}
```

Output

```text
eng::circuit::make: diode junction potentials values must be positive
eng::circuit::make: diode grading coefficients must be below one
eng::circuit::make: diode transit times values must be non-negative
eng::circuit::make: diode junction capacitances values must be non-negative
```
### Example 11

Rejected diodes.

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
  const auto base = [] {
    ckt::Net net;
    net.nodes = 3;
    net.voltage_sources = {{1}, {0}, vec({5.0})};
    net.resistors = {{1}, {2}, vec({1000.0})};
    net.diodes.positive = {2};
    net.diodes.negative = {0};
    net.diodes.saturation = vec({1e-14});
    return net;
  };
  auto zero = base();
  zero.diodes.saturation = vec({0.0});
  report([&] { (void)ckt::make(zero); });
  auto missing = base();
  missing.diodes.saturation = ::pluss::Series{};
  report([&] { (void)ckt::make(missing); });
  auto shorted = base();
  shorted.diodes.negative = {2};
  report([&] { (void)ckt::make(shorted); });
  auto outside = base();
  outside.diodes.positive = {9};
  report([&] { (void)ckt::make(outside); });
  auto bad_ideality = base();
  bad_ideality.diodes.ideality = vec({-1.0});
  report([&] { (void)ckt::make(bad_ideality); });
}
```

Output

```text
eng::circuit::make: diode saturation currents values must be positive
eng::circuit::make: diode saturation currents values must be real floating [..., elements]
eng::circuit::make: diodes cannot connect a node to itself
eng::circuit::make: diodes contains a node outside [0, nodes)
eng::circuit::make: diode ideality factors values must be positive
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Amps.html">
    <span class="label">Previous</span>
    <span class="title">Amps</span>
  </a>
  <a class="next-link" href="Transistors.html">
    <span class="label">Next</span>
    <span class="title">Transistors</span>
  </a>
</div>
<!-- apiref:nav:end -->
