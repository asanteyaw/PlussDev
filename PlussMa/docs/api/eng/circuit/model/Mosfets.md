<!-- generated from Mosfets.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `circuit::Mosfets`

Enhancement MOSFETs of a circuit.

## Signature

```cpp
circuit::Mosfets
```

## What It Does

Describes MOSFETs by their three terminals (the body is tied to the source unless a `bulk` node is given) and the level-1 square-law model. In the effective frame (a PMOS is the NMOS equations on negated voltages) with overdrive `vov = vgs - Vth`: no current for `vov <= 0`; in the triode region (`vds < vov`) `id = kp (vov vds - vds^2 / 2) (1 + lambda vds)`; in saturation `id = kp vov^2 / 2 (1 + lambda vds)`. A negative drain-source voltage swaps the roles of drain and source. Circuits with MOSFETs are solved by Newton iteration in `op`, `ac` (around an operating point) and `tran`. The optional gate capacitances are constant linear capacitors.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `drain` | The drain node of each MOSFET. | empty |
| `gate` | The gate node of each MOSFET. | empty |
| `source` | The source node of each MOSFET; it must differ from the drain. | empty |
| `polarity` | `1` for an NMOS and `-1` for a PMOS, one per MOSFET. | required |
| `transconductance` | `kp = mu Cox W / L` in A/V^2, `[..., mosfets]`; positive and finite. | required |
| `threshold` | The magnitude of the threshold voltage `Vth` in volts, `[..., mosfets]`; positive and finite, the same convention for NMOS and PMOS. | required |
| `channel_modulation` | The channel-length modulation `lambda` in 1/V, `[..., mosfets]`; non-negative and finite. | undefined: zero |
| `bulk` | The bulk node of each MOSFET, one per MOSFET, or empty to tie the body to the source. | empty |
| `body_effect` | The body-effect coefficient `gamma` in sqrt(V), `[..., mosfets]`; non-negative. With a `bulk`, `Vth = Vth0 + gamma (sqrt(phi + vsb) - sqrt(phi))`. | undefined: zero |
| `surface_potential` | The surface potential `phi` in volts, `[..., mosfets]`; positive. | `0.6` |
| `gate_source_capacitance` | A constant gate-source capacitance in farads, `[..., mosfets]`; non-negative. | undefined: none |
| `gate_drain_capacitance` | A constant gate-drain capacitance in farads, `[..., mosfets]`; non-negative. | undefined: none |

## Returns

A value; validated by `make`.

## Errors

`make` throws `pluss::Error` for a terminal outside the circuit, a drain tied to the source, lists of different lengths, a polarity other than `1` or `-1`, a missing, non-positive or non-finite `transconductance` or `threshold`, a negative `channel_modulation` or capacitance, a capacitance across a node and itself, values of a different dtype or platform than the other groups, or incompatible batch axes.

## Interpretation and Use

The gate draws no DC current, so a gate needs a DC path to a defined voltage like any other node. `Result::drain_current` holds the current into each drain terminal; the capacitances are appended to the net's capacitors (in the order diodes, transistors, MOSFETs) for `Start`. A sweep of gate voltages is a batch on the gate source.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A common-source bias point.

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
### Example 2

The triode region.

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
### Example 3

A swept gate voltage.

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
### Example 4

A PMOS mirrors an NMOS.

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
### Example 5

Channel-length modulation.

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
### Example 6

Small-signal gain.

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
### Example 7

Gate capacitance in a transient.

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
### Example 8

The body effect lowers a source follower.

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
### Example 9

A batch of body-effect coefficients and surface potentials.

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
### Example 10

Rejected body parameters.

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
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const pluss::Error &e) {
      out.writeln(e.message());
    }
  };
  auto lengths = follower(0.5, true);
  lengths.mosfets.bulk = {0, 0};
  report([&] { (void)ckt::make(lengths); });
  auto negative = follower(0.5, true);
  negative.mosfets.body_effect = vec({-0.1});
  report([&] { (void)ckt::make(negative); });
  auto potential = follower(0.5, true);
  potential.mosfets.surface_potential = vec({0.0});
  report([&] { (void)ckt::make(potential); });
  auto outside = follower(0.5, true);
  outside.mosfets.bulk = {9};
  report([&] { (void)ckt::make(outside); });
}
```

Output

```text
eng::circuit::make: MOSFET bulk must be empty (body tied to the source) or have one node per MOSFET
eng::circuit::make: MOSFET body-effect coefficients values must be non-negative
eng::circuit::make: MOSFET surface potentials values must be positive
eng::circuit::make: MOSFET source/bulk contains a node outside [0, nodes)
```
### Example 11

Rejected MOSFETs.

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
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Transistors.html">
    <span class="label">Previous</span>
    <span class="title">Transistors</span>
  </a>
  <a class="next-link" href="Net.html">
    <span class="label">Next</span>
    <span class="title">Net</span>
  </a>
</div>
<!-- apiref:nav:end -->
