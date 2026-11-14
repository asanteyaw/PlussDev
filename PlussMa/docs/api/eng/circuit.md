<!-- generated from circuit.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Circuits

Linear electrical networks of resistors, capacitors, inductors, independent and controlled sources and ideal op amps. A circuit is described once, as a `Net`, compiled by `make` into a `Circuit`, and solved for its DC operating point and its AC response, for a whole batch of component values at once.

## Choosing A Surface

| Task | Branch |
|---|---|
| Describe and compile a linear circuit | [Circuit model](circuit/model.md) |
| DC operating point and AC response | [Circuit analysis](circuit/analysis.md) |

## Practical Examples

The examples solve a divider, an RC low-pass and a series resonance.

### Example 1

Solve the DC operating point of a divider.

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
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({10.0})};
  net.resistors = {{1, 2}, {2, 0}, vec({1000.0, 3000.0})};
  const auto result = ckt::op(ckt::make(net));

  out.writeln("node voltages ", flat(result.voltage));
  out.writeln("source current", flat(result.source_current), " (negative: the source delivers 2.5 mA)");
  out.writeln("residual      ", result.residual.item<double>());
  out.writeln("axes of the empty groups: inductors", result.inductor_current.sizes(), " VCCS", result.vccs_current.sizes(), " VCVS", result.vcvs_current.sizes(), " amps", result.amp_current.sizes());
}
```

Output

```text
node voltages  [0.000000, 10.000000, 7.500000]
source current [-0.002500]  (negative: the source delivers 2.5 mA)
residual       0.000000
axes of the empty groups: inductors [0]  VCCS [0]  VCVS [0]  amps [0]
```
### Example 2

Find the gain and phase of an RC low-pass.

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
  net.voltage_sources = {{1}, {0}, vec({1.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  const pluss::Series omega = vec({0.0, 1.0, 1000.0, 100000.0});

  const auto result = ckt::ac(ckt::make(net), omega);
  const pluss::Series output = result.voltage.select(1, 2);   // the capacitor voltage, node 2
  out.writeln("voltage shape", result.voltage.sizes(), " element type", result.voltage.dtype().name);
  out.writeln("gain         ", flat(output.abs()));
  out.writeln("phase (deg)  ", flat(output.angle() * (180.0 / M_PI)));
  out.writeln("analytic gain", 1.0, 1.0 / std::sqrt(1.0 + 1e-6), 1.0 / std::sqrt(2.0), 1.0 / std::sqrt(1.0 + 1e4));
  out.writeln("source current magnitude", flat(result.source_current.abs()));
}
```

Output

```text
voltage shape [4, 3]  element type complex128
gain          [1.000000, 1.000000, 0.707107, 0.010000]
phase (deg)   [0.000000, -0.057296, -45.000000, -89.427061]
analytic gain 1.000000 1.000000 0.707107 0.010000
source current magnitude [0.000000, 0.000001, 0.000707, 0.001000]
```
### Example 3

Find the resonance of a series RLC circuit.

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
  net.nodes = 4;
  net.voltage_sources = {{1}, {0}, vec({1.0})};
  net.resistors = {{1}, {2}, vec({10.0})};
  net.inductors = {{2}, {3}, vec({1e-3})};
  net.capacitors = {{3}, {0}, vec({1e-6})};
  const double w0 = 1.0 / std::sqrt(1e-3 * 1e-6);
  const double quality = std::sqrt(1e-3 / 1e-6) / 10.0;

  const auto result = ckt::ac(ckt::make(net), vec({0.1 * w0, w0, 10.0 * w0}));
  out.writeln("resonant frequency", w0, " quality factor", quality);
  out.writeln("source current magnitude  ", flat(result.source_current.abs()), " (0.1 A at resonance)");
  out.writeln("capacitor voltage magnitude", flat(result.voltage.select(1, 3).abs()), " (Q at resonance)");
  out.writeln("inductor current magnitude ", flat(result.inductor_current.abs()));
}
```

Output

```text
resonant frequency 31622.776602  quality factor 3.162278
source current magnitude   [0.003193, 0.100000, 0.003193]  (0.1 A at resonance)
capacitor voltage magnitude [1.009586, 3.162278, 0.010096]  (Q at resonance)
inductor current magnitude  [0.003193, 0.100000, 0.003193]
```
<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 3
:hidden:

circuit/model
circuit/analysis
```
<!-- apiref:toctree:end -->
