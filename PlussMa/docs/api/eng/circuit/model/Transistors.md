<!-- generated from Transistors.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `circuit::Transistors`

Bipolar transistors of a circuit.

## Signature

```cpp
circuit::Transistors
```

## What It Does

Describes bipolar transistors by their three terminals and their model values. The model is the Ebers-Moll transport model: a base-emitter and a base-collector junction, each a diode of saturation current `Is`, with the forward current `Is (exp(vbe / Vt) - 1)` reaching the collector and `1 / beta_f` of it the base, and the reverse junction current `beta_r` times smaller at the base. It is deliberately limited: there is no terminal resistance and the stored charge is two constant capacitances, and the Early effect is optional (`early_voltage`). A PNP is the NPN model with every voltage and current reversed. Circuits with transistors are solved by Newton iteration in `op`, `ac` (around an operating point) and `tran`.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `collector` | The collector node of each transistor. | empty |
| `base` | The base node of each transistor. | empty |
| `emitter` | The emitter node of each transistor. The three nodes of a transistor must differ. | empty |
| `polarity` | `1` for an NPN and `-1` for a PNP, one per transistor. | required |
| `saturation` | The saturation current `Is`, `[..., transistors]`; positive and finite. | required |
| `forward_gain` | The forward current gain `beta_f`, `[..., transistors]`; positive and finite. | required |
| `reverse_gain` | The reverse current gain `beta_r`, `[..., transistors]`; positive and finite. | `1` |
| `base_emitter_capacitance` | A constant base-emitter capacitance in farads, `[..., transistors]`; non-negative. It is a linear capacitor appended to the net's capacitors. | undefined: none |
| `base_collector_capacitance` | A constant base-collector capacitance in farads, `[..., transistors]`; non-negative. | undefined: none |
| `early_voltage` | The Early voltage `VA` in volts, `[..., transistors]`; positive and finite. The transport current is scaled by `1 + vce / VA`. | undefined: no Early effect |

## Returns

A value; validated by `make`.

## Errors

`make` throws `pluss::Error` for a terminal outside the circuit, two terminals of one transistor on the same node, lists of different lengths, a polarity other than `1` or `-1`, a missing, non-positive or non-finite `saturation`, `forward_gain`, `reverse_gain` or `early_voltage`, values of a different dtype or platform than the other groups, or values with an incompatible batch axis.

## Interpretation and Use

While the transistor is active the collector current is `forward_gain` times the base current and does not depend on the collector voltage unless an Early voltage is given, in which case it rises with it; when the collector resistor can drop no more of the supply the transistor saturates and the ratio falls. `Result::collector_current` and `Result::base_current` are the currents flowing into those terminals; the emitter current is `-(Ic + Ib)`. A PNP is powered from supplies of the opposite sign.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

An NPN common-emitter bias point.

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
  // NPN common emitter: 10 V supply, 1 kohm collector resistor, 5 V base drive through 100 kohm
  ckt::Net net;
  net.nodes = 5;
  net.voltage_sources = {{1, 3}, {0, 0}, vec({10.0, 5.0})};
  net.resistors = {{1, 3}, {2, 4}, vec({1000.0, 100000.0})};
  net.transistors.collector = {2};
  net.transistors.base = {4};
  net.transistors.emitter = {0};
  net.transistors.polarity = {1};
  net.transistors.saturation = vec({1e-14});
  net.transistors.forward_gain = vec({100.0});

  const auto result = ckt::op(ckt::make(net));
  const std::vector<double> v = flat(result.voltage);
  const double ic = flat(result.collector_current)[0], ib = flat(result.base_current)[0];
  out.writeln("base voltage      ", v[4]);
  out.writeln("collector voltage ", v[2]);
  out.writeln("base current      ", ib);
  out.writeln("collector current ", ic);
  out.writeln("current gain Ic/Ib", ic / ib);
  out.writeln("emitter current   ", -(ic + ib));
}
```

Output

```text
base voltage       0.692490
collector voltage  5.692490
base current       0.000043
collector current  0.004308
current gain Ic/Ib 100.000000
emitter current    -0.004351
```
### Example 2

A PNP is the mirror image of an NPN.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static ckt::Net stage(std::int64_t polarity, double sign) {
  ckt::Net net;
  net.nodes = 5;
  net.voltage_sources = {{1, 3}, {0, 0}, vec({10.0 * sign, 5.0 * sign})};
  net.resistors = {{1, 3}, {2, 4}, vec({1000.0, 100000.0})};
  net.transistors.collector = {2};
  net.transistors.base = {4};
  net.transistors.emitter = {0};
  net.transistors.polarity = {polarity};
  net.transistors.saturation = vec({1e-14});
  net.transistors.forward_gain = vec({100.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  // a PNP on negated supplies is the mirror image of the NPN
  const auto npn = ckt::op(ckt::make(stage(1, 1.0)));
  const auto pnp = ckt::op(ckt::make(stage(-1, -1.0)));
  out.writeln("NPN collector voltage and current", flat(npn.voltage)[2], flat(npn.collector_current)[0]);
  out.writeln("PNP collector voltage and current", flat(pnp.voltage)[2], flat(pnp.collector_current)[0]);
}
```

Output

```text
NPN collector voltage and current 5.692490 0.004308
PNP collector voltage and current -5.692490 -0.004308
```
### Example 3

The Early effect.

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
  // the Early voltage makes the collector current grow with the collector voltage: the supply moves the current
  for (double supply : {5.0, 10.0, 20.0}) {
    ckt::Net net;
    net.nodes = 5;
    net.voltage_sources = {{1, 3}, {0, 0}, vec({supply, 5.0})};
    net.resistors = {{1, 3}, {2, 4}, vec({100.0, 100000.0})};
    net.transistors.collector = {2};
    net.transistors.base = {4};
    net.transistors.emitter = {0};
    net.transistors.polarity = {1};
    net.transistors.saturation = vec({1e-14});
    net.transistors.forward_gain = vec({100.0});
    const auto flat_model = ckt::op(ckt::make(net));
    net.transistors.early_voltage = vec({50.0});
    const auto early = ckt::op(ckt::make(net));
    out.writeln("supply", supply, " collector current without and with the Early effect", flat(flat_model.collector_current)[0], flat(early.collector_current)[0]);
  }
}
```

Output

```text
supply 5.000000  collector current without and with the Early effect 0.004308 0.004698
supply 10.000000  collector current without and with the Early effect 0.004308 0.005125
supply 20.000000  collector current without and with the Early effect 0.004308 0.005979
```
### Example 4

Rejected transistors.

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
    net.nodes = 5;
    net.voltage_sources = {{1, 3}, {0, 0}, vec({10.0, 5.0})};
    net.resistors = {{1, 3}, {2, 4}, vec({1000.0, 100000.0})};
    net.transistors.collector = {2};
    net.transistors.base = {4};
    net.transistors.emitter = {0};
    net.transistors.polarity = {1};
    net.transistors.saturation = vec({1e-14});
    net.transistors.forward_gain = vec({100.0});
    return net;
  };
  auto polarity = base();
  polarity.transistors.polarity = {2};
  report([&] { (void)ckt::make(polarity); });
  auto tied = base();
  tied.transistors.emitter = {2};
  report([&] { (void)ckt::make(tied); });
  auto lengths = base();
  lengths.transistors.polarity = {1, 1};
  report([&] { (void)ckt::make(lengths); });
  auto no_gain = base();
  no_gain.transistors.forward_gain = ::pluss::Series{};
  report([&] { (void)ckt::make(no_gain); });
  auto negative_gain = base();
  negative_gain.transistors.reverse_gain = vec({-1.0});
  report([&] { (void)ckt::make(negative_gain); });
}
```

Output

```text
eng::circuit::make: transistor polarity must be +1 (NPN) or -1 (PNP)
eng::circuit::make: transistor collector/emitter cannot connect a node to itself
eng::circuit::make: transistor collector, base, emitter and polarity arrays must have equal length
eng::circuit::make: transistor forward gains values must be real floating [..., elements]
eng::circuit::make: transistor reverse gains values must be positive
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Diodes.html">
    <span class="label">Previous</span>
    <span class="title">Diodes</span>
  </a>
  <a class="next-link" href="Mosfets.html">
    <span class="label">Next</span>
    <span class="title">Mosfets</span>
  </a>
</div>
<!-- apiref:nav:end -->
