<!-- generated from mosfets_and_charge_storage.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# MOSFETs and charge storage

[Back to Circuits 3: Diodes and Transistors](../circuit_03_devices.md)

A MOSFET is controlled by a voltage, not a current: the gate voltage sets the channel and the drain current follows. This page finds the bias point of a common-source stage in each of the three regions, sweeps the gate voltage, mirrors the stage with a PMOS, computes the small-signal gain, and then charges the gate through a resistor: devices store charge, and the stored charge is what makes them slow.

## The mental model

`circuit::Mosfets` describes enhancement MOSFETs by `drain`, `gate` and `source` (the body is tied to the source), `polarity` (`1` NMOS, `-1` PMOS), the transconductance `kp = mu Cox W / L` in A/V^2, the threshold voltage magnitude and an optional channel-length modulation `lambda`. The model is the level-1 square law. With the overdrive `vov = vgs - Vth`:

- **Cutoff** (`vov <= 0`): no current.
- **Triode** (`vds < vov`): `id = kp (vov vds - vds^2 / 2) (1 + lambda vds)`.
- **Saturation** (`vds >= vov`): `id = kp vov^2 / 2 (1 + lambda vds)`.

A negative `vds` swaps the roles of drain and source, and a PMOS is the NMOS equations on negated voltages, with `Result::drain_current` the current into the drain terminal. Everything else is as for diodes and transistors: `op` solves by Newton iteration with the automatic convergence aids, leading axes sweep any value, and `ac` linearises around an operating point.

**Charge storage** is a set of constant capacitances on the devices: `junction_capacitance` on diodes, `base_emitter_capacitance` and `base_collector_capacitance` on transistors, `gate_source_capacitance` and `gate_drain_capacitance` on MOSFETs. They are ordinary capacitors appended to the net's own in the order diodes, transistors, MOSFETs, so `tran` and `Start` treat them exactly like any other capacitor.

## A common-source stage

A `10 V` supply feeds the drain through `500 ohm`; the gate is at `3 V`; `kp = 2 mA/V^2` and `Vth = 1 V`. The overdrive is `2 V` and the saturation current `kp vov^2 / 2 = 4 mA`, so the drain sits at `10 - 4 mA x 500 = 8 V`, above the overdrive, which confirms saturation.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static ckt::Net common_source(double vdd, double vgs, double rd, double lambda = 0.0, std::int64_t polarity = 1) {
  // nodes: 1 supply, 2 drain, 3 gate drive; source grounded
  ckt::Net net;
  net.nodes = 4;
  net.voltage_sources = {{1, 3}, {0, 0}, vec({vdd, vgs})};
  net.resistors = {{1}, {2}, vec({rd})};
  net.mosfets.drain = {2};
  net.mosfets.gate = {3};
  net.mosfets.source = {0};
  net.mosfets.polarity = {polarity};
  net.mosfets.transconductance = vec({2e-3});
  net.mosfets.threshold = vec({1.0});
  if (lambda > 0.0) net.mosfets.channel_modulation = vec({lambda});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  // kp = 2 mA/V^2, Vth = 1 V, Rd = 500 ohm: at vgs = 3 V the overdrive is 2 V and the saturation current kp vov^2 / 2 = 4 mA
  const auto circuit = ckt::make(common_source(10.0, 3.0, 500.0));
  const auto result = ckt::op(circuit);
  out.writeln("MOSFETs in the circuit", circuit.mosfets());
  out.writeln("drain voltage", flat(result.voltage)[2], " drain current", flat(result.drain_current)[0]);
  out.writeln("saturated: the drain is above the overdrive", flat(result.voltage)[2] > 2.0);
  out.writeln("residual below 1e-9", result.residual.item<double>() < 1e-9);
}
```

Output

```text
MOSFETs in the circuit 1
drain voltage 8.000000  drain current 0.004000
saturated: the drain is above the overdrive true
residual below 1e-9 true
```

The drain is at exactly `8 V` with `4 mA` flowing, and the residual confirms the nonlinear equations are solved.

## The triode region

With a `5 kohm` drain resistor and a `5 V` gate the saturation current would be `16 mA`, far more than the resistor allows, so the transistor falls into the triode region: `(5 - vds) / 5000 = kp (4 vds - vds^2 / 2)`, which is `5 vds^2 - 41 vds + 5 = 0`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static ckt::Net common_source(double vdd, double vgs, double rd, double lambda = 0.0, std::int64_t polarity = 1) {
  // nodes: 1 supply, 2 drain, 3 gate drive; source grounded
  ckt::Net net;
  net.nodes = 4;
  net.voltage_sources = {{1, 3}, {0, 0}, vec({vdd, vgs})};
  net.resistors = {{1}, {2}, vec({rd})};
  net.mosfets.drain = {2};
  net.mosfets.gate = {3};
  net.mosfets.source = {0};
  net.mosfets.polarity = {polarity};
  net.mosfets.transconductance = vec({2e-3});
  net.mosfets.threshold = vec({1.0});
  if (lambda > 0.0) net.mosfets.channel_modulation = vec({lambda});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  // a large drain resistor pulls the transistor into the triode region: 5 vds^2 - 41 vds + 5 = 0
  const auto result = ckt::op(ckt::make(common_source(5.0, 5.0, 5000.0)));
  const double vds = flat(result.voltage)[2];
  out.writeln("drain voltage", vds, " closed form", (41.0 - std::sqrt(1681.0 - 100.0)) / 10.0);
  out.writeln("drain current", flat(result.drain_current)[0], " from the triode law", 2e-3 * (4.0 * vds - 0.5 * vds * vds));
  out.writeln("triode: the drain is below the overdrive of 4 V", vds < 4.0);
}
```

Output

```text
drain voltage 0.123821  closed form 0.123821
drain current 0.000975  from the triode law 0.000975
triode: the drain is below the overdrive of 4 V true
```

The drain settles at `0.1238 V`, matching the closed form, with a current of `0.975 mA` from the triode law.

## Sweeping the gate

A swept gate is a batch on its source. The program uses a `2 kohm` drain resistor and gate voltages from `0.5` to `5 V`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static ckt::Net common_source(double vdd, double vgs, double rd, double lambda = 0.0, std::int64_t polarity = 1) {
  // nodes: 1 supply, 2 drain, 3 gate drive; source grounded
  ckt::Net net;
  net.nodes = 4;
  net.voltage_sources = {{1, 3}, {0, 0}, vec({vdd, vgs})};
  net.resistors = {{1}, {2}, vec({rd})};
  net.mosfets.drain = {2};
  net.mosfets.gate = {3};
  net.mosfets.source = {0};
  net.mosfets.polarity = {polarity};
  net.mosfets.transconductance = vec({2e-3});
  net.mosfets.threshold = vec({1.0});
  if (lambda > 0.0) net.mosfets.channel_modulation = vec({lambda});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // a swept gate voltage is a batch: cutoff, saturation and triode in one call
  const double gates[] = {0.5, 1.5, 2.0, 3.0, 4.0, 5.0};
  ckt::Net net = common_source(10.0, 0.0, 2000.0);
  net.voltage_sources.value = mat({10, 0.5, 10, 1.5, 10, 2.0, 10, 3.0, 10, 4.0, 10, 5.0}, {6, 2});
  const auto result = ckt::op(ckt::make(net));
  out.writeln("drain_current shape", result.drain_current.sizes(), " (gate value, MOSFET)");
  const std::vector<double> vd = flat(result.voltage.select(1, 2)), id = flat(result.drain_current);
  out.writeln("  Vgs    Vd      Id (mA)");
  for (int k = 0; k < 6; ++k) out.writeln(gates[k], vd[k], id[k] * 1e3);
}
```

Output

```text
drain_current shape [6, 1]  (gate value, MOSFET)
  Vgs    Vd      Id (mA)
0.50000 10.00000 0.00000
1.50000 9.50000 0.25000
2.00000 8.00000 1.00000
3.00000 2.00000 4.00000
4.00000 0.89150 4.55425
5.00000 0.63579 4.68210
```

Below the `1 V` threshold nothing flows and the drain sits at the supply; at `1.5 V` and `2 V` the current rises as the square of the overdrive (`0.25` and `1 mA`) with the drain at `9.5` and `8 V`; at `3 V` the stage is at the edge between the regions (`4 mA`, drain at `2 V` which equals the overdrive); and above it the drain is below the overdrive, in the triode region, where more gate voltage buys little more current (`4.55` and `4.68 mA`).

## A PMOS mirrors an NMOS

On negated supplies a PMOS carries exactly the NMOS current in the opposite direction.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static ckt::Net common_source(double vdd, double vgs, double rd, double lambda = 0.0, std::int64_t polarity = 1) {
  // nodes: 1 supply, 2 drain, 3 gate drive; source grounded
  ckt::Net net;
  net.nodes = 4;
  net.voltage_sources = {{1, 3}, {0, 0}, vec({vdd, vgs})};
  net.resistors = {{1}, {2}, vec({rd})};
  net.mosfets.drain = {2};
  net.mosfets.gate = {3};
  net.mosfets.source = {0};
  net.mosfets.polarity = {polarity};
  net.mosfets.transconductance = vec({2e-3});
  net.mosfets.threshold = vec({1.0});
  if (lambda > 0.0) net.mosfets.channel_modulation = vec({lambda});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  // a PMOS on negated supplies is the mirror image of the NMOS: every voltage and the drain current change sign
  const auto n = ckt::op(ckt::make(common_source(10.0, 3.0, 500.0)));
  const auto p = ckt::op(ckt::make(common_source(-10.0, -3.0, 500.0, 0.0, -1)));
  out.writeln("NMOS drain voltage and current", flat(n.voltage)[2], flat(n.drain_current)[0]);
  out.writeln("PMOS drain voltage and current", flat(p.voltage)[2], flat(p.drain_current)[0]);
}
```

Output

```text
NMOS drain voltage and current 8.000000 0.004000
PMOS drain voltage and current -8.000000 -0.004000
```

The PMOS drain is at `-8 V` with `-4 mA`.

## Channel-length modulation

A non-zero `lambda` makes the saturation current grow with the drain voltage. With `lambda = 0.05` the bias equation `10 - vd = 2 (1 + 0.05 vd)` gives `vd = 8 / 1.1`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static ckt::Net common_source(double vdd, double vgs, double rd, double lambda = 0.0, std::int64_t polarity = 1) {
  // nodes: 1 supply, 2 drain, 3 gate drive; source grounded
  ckt::Net net;
  net.nodes = 4;
  net.voltage_sources = {{1, 3}, {0, 0}, vec({vdd, vgs})};
  net.resistors = {{1}, {2}, vec({rd})};
  net.mosfets.drain = {2};
  net.mosfets.gate = {3};
  net.mosfets.source = {0};
  net.mosfets.polarity = {polarity};
  net.mosfets.transconductance = vec({2e-3});
  net.mosfets.threshold = vec({1.0});
  if (lambda > 0.0) net.mosfets.channel_modulation = vec({lambda});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  // channel-length modulation lambda makes the saturation current grow with the drain voltage: 10 - vd = 2 (1 + lambda vd)
  const auto flat_model = ckt::op(ckt::make(common_source(10.0, 3.0, 500.0)));
  const auto modulated = ckt::op(ckt::make(common_source(10.0, 3.0, 500.0, 0.05)));
  out.writeln("lambda 0   drain voltage", flat(flat_model.voltage)[2], " current", flat(flat_model.drain_current)[0]);
  out.writeln("lambda 0.05 drain voltage", flat(modulated.voltage)[2], " current", flat(modulated.drain_current)[0], " closed form", 8.0 / 1.1);
}
```

Output

```text
lambda 0   drain voltage 8.000000  current 0.004000
lambda 0.05 drain voltage 7.272727  current 0.005455  closed form 7.272727
```

The drain moves from `8 V` to `7.273 V`, equal to the closed form, and the current rises from `4 mA` to `5.45 mA`.

## Small-signal gain

Around the bias point the transistor is a transconductance `gm = kp vov = 4 mS` in parallel with the output resistance `ro`. The gain of the common-source stage is `-gm Rd` without modulation and `-gm (Rd || ro)` with it. As for the transistor, the excitation circuit is built with the supply at `0 V` and `ac` takes the bias result.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static ckt::Net common_source(double vdd, double vgs, double rd, double lambda = 0.0, std::int64_t polarity = 1) {
  // nodes: 1 supply, 2 drain, 3 gate drive; source grounded
  ckt::Net net;
  net.nodes = 4;
  net.voltage_sources = {{1, 3}, {0, 0}, vec({vdd, vgs})};
  net.resistors = {{1}, {2}, vec({rd})};
  net.mosfets.drain = {2};
  net.mosfets.gate = {3};
  net.mosfets.source = {0};
  net.mosfets.polarity = {polarity};
  net.mosfets.transconductance = vec({2e-3});
  net.mosfets.threshold = vec({1.0});
  if (lambda > 0.0) net.mosfets.channel_modulation = vec({lambda});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // small-signal gain of the common-source stage: -gm Rd with gm = kp vov = 4 mS
  const auto bias = ckt::op(ckt::make(common_source(10.0, 3.0, 500.0)));
  const auto response = ckt::ac(ckt::make(common_source(0.0, 1.0, 500.0)), vec({0.0, 1e3}), bias);
  const pluss::Series gain = response.voltage.select(-1, 2);
  out.writeln("gain at 0 rad/s", flat(gain.real())[0], " formula", -2e-3 * 2.0 * 500.0);
  // with channel-length modulation the output resistance ro = 1 / (lambda id) shunts the load
  const auto lam_bias = ckt::op(ckt::make(common_source(10.0, 3.0, 500.0, 0.05)));
  const auto lam = ckt::ac(ckt::make(common_source(0.0, 1.0, 500.0, 0.05)), vec({0.0}), lam_bias);
  out.writeln("gain with lambda 0.05", flat(lam.voltage.select(-1, 2).real())[0]);
}
```

Output

```text
gain at 0 rad/s -2.00000  formula -2.00000
gain with lambda 0.05 -2.47934
```

The gain is `-2.00000`, equal to `-gm Rd`, and `-2.479` with `lambda = 0.05`, where the larger current raises `gm` and the output resistance lowers the load.

## Charging the gate

The gate capacitance is what a driver has to charge before the channel opens. The program drives a MOSFET gate through `1 kohm` from a `5 V` step; the gate has `1 uF` to the source. The charging is the RC response with `tau = 1 ms`.

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
  // the gate capacitance is a capacitor: a 5 V step through 1 kohm charges 1 uF with tau = 1 ms
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({5.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.mosfets.drain = {1};
  net.mosfets.gate = {2};
  net.mosfets.source = {0};
  net.mosfets.polarity = {1};
  net.mosfets.transconductance = vec({2e-3});
  net.mosfets.threshold = vec({1.0});
  net.mosfets.gate_source_capacitance = vec({1e-6});
  std::vector<double> time;
  for (int k = 0; k <= 30; ++k) time.push_back(k * 1e-4);
  const auto run = ckt::tran(ckt::make(net), vec(time));
  const std::vector<double> gate = flat(run.voltage.select(1, 2));
  out.writeln("  ms   gate     closed form");
  for (int ms = 0; ms <= 3; ++ms) out.writeln(ms, gate[ms * 10], 5.0 * (1.0 - std::exp(-1.0 * ms)));
  out.writeln("drain_current shape", run.drain_current.sizes(), " (time sample, MOSFET)");
}
```

Output

```text
  ms   gate     closed form
0 0.00000 0.00000
1 3.16214 3.16060
2 4.32445 4.32332
3 4.75169 4.75106
drain_current shape [31, 1]  (time sample, MOSFET)
```

The gate rises as `5 (1 - exp(-t / tau))`: `3.162 V` against `3.161 V` at `1 ms`, with the same results as any capacitor because the device capacitance is one.

The same mechanism gives a diode its junction capacitance: a reverse-biased diode blocks, so a `1 uF` junction capacitance behind `1 kohm` charges the node with the same time constant.

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

## Starting a circuit with device capacitances

Device capacitors are capacitors, so `Start::capacitor_voltage` has one value for each, the net's own first and then those of the diodes, transistors and MOSFETs. Capacitors in parallel must start at the same voltage, and `tran` says so if they do not.

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

Node 2 starts at `2 V` (the net's capacitor holds `2 V` and the diode's, from ground to the node, `-2 V`) and relaxes towards its operating point; start values that disagree for the same pair of nodes are reported.

## The body effect

A MOSFET's channel is controlled relative to its body. With the body tied to the source (the default) nothing changes, but when the source rises above the bulk, as in a source follower, the threshold rises with it: `Vth = Vth0 + gamma (sqrt(phi + vsb) - sqrt(phi))`, with `gamma` in `sqrt(V)` and `phi` the surface potential (default `0.6 V`). Give the transistor a `bulk` node and a `body_effect`. The program is a source follower: `10 V` on the drain, `5 V` on the gate, a `1 kohm` source resistor, and the bulk at ground.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static ckt::Net follower(double gamma, bool bulk_grounded) {
  // nodes: 1 drain supply, 2 gate drive, 3 source; the source resistor goes to ground
  ckt::Net net;
  net.nodes = 4;
  net.voltage_sources = {{1, 2}, {0, 0}, vec({10.0, 5.0})};
  net.resistors = {{3}, {0}, vec({1000.0})};
  net.mosfets.drain = {1};
  net.mosfets.gate = {2};
  net.mosfets.source = {3};
  net.mosfets.polarity = {1};
  if (bulk_grounded) net.mosfets.bulk = {0};
  net.mosfets.transconductance = vec({2e-3});
  net.mosfets.threshold = vec({1.0});
  if (gamma > 0.0) net.mosfets.body_effect = vec({gamma});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  // a source follower: the source rises, the bulk stays at ground, and the raised source-bulk voltage lifts the threshold
  out.writeln("source voltage, body tied to the source        ", flat(ckt::op(ckt::make(follower(0.5, false))).voltage)[3]);
  out.writeln("source voltage, bulk grounded, gamma = 0        ", flat(ckt::op(ckt::make(follower(0.0, true))).voltage)[3]);
  out.writeln("source voltage, bulk grounded, gamma = 0.5      ", flat(ckt::op(ckt::make(follower(0.5, true))).voltage)[3]);
  out.writeln("source voltage, bulk grounded, gamma = 1.0      ", flat(ckt::op(ckt::make(follower(1.0, true))).voltage)[3]);
}
```

Output

```text
source voltage, body tied to the source         2.43845
source voltage, bulk grounded, gamma = 0         2.43845
source voltage, bulk grounded, gamma = 0.5       2.11108
source voltage, bulk grounded, gamma = 1.0       1.84953
```

With the body tied to the source, or with `gamma = 0`, the source sits at `2.438 V`. A `gamma` of `0.5` lowers it to `2.111 V` and `1.0` to `1.850 V`: the rising threshold leaves less overdrive, so less current and less output.

`body_effect` and `surface_potential` are device values like any other, so a batch sweeps them.

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
  // gamma and the surface potential are device values: a batch of body-effect coefficients in one call
  ckt::Net net;
  net.nodes = 4;
  net.voltage_sources = {{1, 2}, {0, 0}, vec({10.0, 5.0})};
  net.resistors = {{3}, {0}, vec({1000.0})};
  net.mosfets.drain = {1};
  net.mosfets.gate = {2};
  net.mosfets.source = {3};
  net.mosfets.polarity = {1};
  net.mosfets.bulk = {0};
  net.mosfets.transconductance = vec({2e-3});
  net.mosfets.threshold = vec({1.0});
  net.mosfets.body_effect = vec({0.0, 0.3, 0.6, 0.9}).reshape({4, 1});
  const auto result = ckt::op(ckt::make(net));
  out.writeln("source voltage for gamma 0, 0.3, 0.6, 0.9", flat(result.voltage.select(1, 3)));
  out.writeln("drain current in mA                      ", flat(result.drain_current.mul(1e3)));
  net.mosfets.body_effect = vec({0.6});
  net.mosfets.surface_potential = vec({0.4, 0.6, 0.9}).reshape({3, 1});
  out.writeln("source voltage for phi 0.4, 0.6, 0.9      ", flat(ckt::op(ckt::make(net)).voltage.select(1, 3)));
}
```

Output

```text
source voltage for gamma 0, 0.3, 0.6, 0.9 [2.43845, 2.23308, 2.05407, 1.89739]
drain current in mA                       [2.43845, 2.23308, 2.05407, 1.89739]
source voltage for phi 0.4, 0.6, 0.9       [2.02312, 2.05407, 2.08739]
```

The source voltage falls from `2.438 V` through `2.233`, `2.054` to `1.897 V` as `gamma` rises from `0` to `0.9`, and the surface potential has the opposite and weaker effect (`2.023`, `2.054`, `2.087 V` for `0.4`, `0.6`, `0.9 V`): a larger `phi` flattens the rise of the threshold.

## Voltage-dependent junction capacitance

The depletion capacitance of a junction falls as the reverse bias grows: `C(v) = Cj0 / (1 - v / Vj)^m`, with the zero-bias capacitance `Cj0` (`junction_capacitance`), the junction potential `Vj` (`junction_potential`) and the grading coefficient `m` (`grading`, default `0.5`), continued linearly above `Vj / 2` so it stays finite in forward bias. Giving a `junction_potential` makes the capacitance voltage dependent: the device stores a charge `Q(v)` and its time derivative is a current, in the transient and in the small-signal response alike. The program biases a `1 uF` junction behind `1 kohm` at three reverse voltages and measures the gain of a small AC signal at the RC corner of each.

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

The capacitance falls from `0.642 uF` at `-1 V` to `0.435 uF` at `-3 V` and `0.256 uF` at `-10 V`, and the gain is `1 / sqrt 2` at each corner `1 / (R C)` (`1558`, `2299` and `3910 rad/s`), so the AC response follows the bias.

The transient does too. A small step on a junction biased at `-3 V` charges it with the time constant `R C(bias) = 0.435 ms`.

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

The node follows `1 - exp(-t / tau)`: `0.613` against `0.632` at one time constant (the rest of the difference is the integrator's step of a tenth of `tau`), and `0.981` against `0.982` at four.

## Diffusion charge

A forward-biased junction also stores diffusion charge, proportional to the current: `Q = tau i`, with the transit time `tau` (`transit_time`). For small signals this is a capacitance `tau g`, so the diode is the admittance `g (1 + j w tau)`. The program compares the AC response of a diode behind `1 kohm` with the formula.

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

The conductance at this bias is `166.6 mS`; the gain agrees with the formula at `1e3`, `1e5` and `1e6 rad/s` (`0.00597`, `0.00594`, `0.00423`), the last falling because the diffusion capacitance shorts the node at high frequency.

## Switching a junction

Stored charge is what makes a diode slow. The program switches a `3 V` step through `1 kohm` onto a diode with and without a depletion capacitance (`1 nF`, `0.7 V`) and a transit time (`100 ns`).

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

Without charge the node jumps to the forward drop, `0.6765 V`, as soon as the source switches; with it the node rises over about a microsecond (`0.029`, `0.139`, `0.279`, `0.459`, `0.651 V` at `0.40`, `0.44`, `0.50`, `0.60`, `0.80 us`) because the source must first supply the junction's charge.

## Errors

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static pluss::Series mat(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::series::make(v, shape); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static ckt::Net common_source(double vdd, double vgs, double rd, double lambda = 0.0, std::int64_t polarity = 1) {
  // nodes: 1 supply, 2 drain, 3 gate drive; source grounded
  ckt::Net net;
  net.nodes = 4;
  net.voltage_sources = {{1, 3}, {0, 0}, vec({vdd, vgs})};
  net.resistors = {{1}, {2}, vec({rd})};
  net.mosfets.drain = {2};
  net.mosfets.gate = {3};
  net.mosfets.source = {0};
  net.mosfets.polarity = {polarity};
  net.mosfets.transconductance = vec({2e-3});
  net.mosfets.threshold = vec({1.0});
  if (lambda > 0.0) net.mosfets.channel_modulation = vec({lambda});
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
  auto polarity = common_source(10.0, 3.0, 500.0);
  polarity.mosfets.polarity = {2};
  report([&] { (void)ckt::make(polarity); });
  auto no_gain = common_source(10.0, 3.0, 500.0);
  no_gain.mosfets.transconductance = vec({0.0});
  report([&] { (void)ckt::make(no_gain); });
  auto no_threshold = common_source(10.0, 3.0, 500.0);
  no_threshold.mosfets.threshold = pluss::Series{};
  report([&] { (void)ckt::make(no_threshold); });
  auto tied = common_source(10.0, 3.0, 500.0);
  tied.mosfets.source = {2};
  report([&] { (void)ckt::make(tied); });
  auto negative_cap = common_source(10.0, 3.0, 500.0);
  negative_cap.mosfets.gate_source_capacitance = vec({-1e-9});
  report([&] { (void)ckt::make(negative_cap); });
  auto lengths = common_source(10.0, 3.0, 500.0);
  lengths.mosfets.gate = {3, 3};
  report([&] { (void)ckt::make(lengths); });
}
```

Output

```text
eng::circuit::make: MOSFET polarity must be +1 (NMOS) or -1 (PMOS)
eng::circuit::make: MOSFET transconductances values must be positive
eng::circuit::make: MOSFET threshold voltages values must be real floating [..., elements]
eng::circuit::make: MOSFET drain/source cannot connect a node to itself
eng::circuit::make: MOSFET gate-source capacitances values must be non-negative
eng::circuit::make: MOSFET drain, gate, source and polarity arrays must have equal length
```

## Decisions

- **A voltage-controlled device:** `Mosfets`; give `kp` and `Vth` from the datasheet or from `mu Cox W / L`.
- **Output resistance:** `channel_modulation`; leave it undefined for an ideal current source in saturation.
- **Switching speed:** the gate, junction and base capacitances; give them and run `tran`. Add a `junction_potential` for a depletion capacitance that follows the bias and a `transit_time` for diffusion charge.
- **A source follower or a stacked device:** give the MOSFET a `bulk` node and a `body_effect`.
- **The gate has no DC current:** it needs a DC path to a defined voltage like any node.

## Pitfalls

The program makes two mistakes: leaving the gate undriven, and entering a PMOS threshold as a negative number. Device charge needs no extra mistake to avoid, but note that a `junction_capacitance` without a `junction_potential` is a constant capacitor.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static ckt::Net stage(bool driven_gate, double threshold, std::int64_t polarity, double supply) {
  ckt::Net net;
  net.nodes = 4;
  net.voltage_sources = {{1}, {0}, vec({supply})};
  if (driven_gate) {
    net.voltage_sources = {{1, 3}, {0, 0}, vec({supply, 3.0 * (supply < 0 ? -1.0 : 1.0)})};
  }
  net.resistors = {{1}, {2}, vec({500.0})};
  net.mosfets.drain = {2};
  net.mosfets.gate = {3};
  net.mosfets.source = {0};
  net.mosfets.polarity = {polarity};
  net.mosfets.transconductance = vec({2e-3});
  net.mosfets.threshold = vec({threshold});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  // 1. a gate with nothing driving it has no DC path: the node equation is singular
  report([&] { (void)ckt::op(ckt::make(stage(false, 1.0, 1, 10.0))); });
  // 2. the PMOS threshold is a magnitude: entering the usual negative value is refused
  report([&] { (void)ckt::make(stage(true, -1.0, -1, -10.0)); });
  // 3. the same PMOS with the threshold as a magnitude conducts
  out.writeln("PMOS drain voltage with a threshold of 1", flat(ckt::op(ckt::make(stage(true, 1.0, -1, -10.0))).voltage)[2]);
}
```

Output

```text
eng::circuit::op: the operating-point equations are singular; check for a node with no path to ground, and for a loop of ideal voltage sources and inductors (gmin stepping and source stepping were also tried)
eng::circuit::make: MOSFET threshold voltages values must be positive
PMOS drain voltage with a threshold of 1 -8.00000
```

An undriven gate has no DC path, so the node equation is singular and `op` says so after trying its convergence aids. A PMOS threshold entered as the usual negative number is refused (`threshold voltages values must be positive`): the threshold is a magnitude for both polarities. With the threshold as a magnitude, the PMOS on negated supplies conducts and its drain sits at `-8 V`.

## Where next

Reference pages: [`Mosfets`](../../../../api/eng/circuit/model/Mosfets.md), [`Diodes`](../../../../api/eng/circuit/model/Diodes.md), [`Transistors`](../../../../api/eng/circuit/model/Transistors.md), [`Result`](../../../../api/eng/circuit/analysis/Result.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rectifier_transient.html">
    <span class="label">Previous</span>
    <span class="title">The rectifier in time</span>
  </a>
</div>
<!-- apiref:nav:end -->
