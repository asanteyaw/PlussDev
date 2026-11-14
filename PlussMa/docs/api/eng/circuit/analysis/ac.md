<!-- generated from ac.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `circuit::ac`

Computes the small-signal phasor response of a circuit.

## Signature

```cpp
circuit::Result circuit::ac(const circuit::Circuit& circuit, const pluss::Series& omega);

circuit::Result circuit::ac(const circuit::Circuit& circuit, const pluss::Series& omega, const circuit::Result& operating_point, const circuit::SolveOptions& options = {});
```

## What It Does

Solves the phasor equations at every angular frequency in `omega`. The independent sources are phasors, a capacitor has admittance `j w C` and an inductor impedance `j w L`. The second overload analyses a circuit with diodes or transistors: every device is replaced by its small-signal conductances, evaluated at the junction voltages of `operating_point`, and the stored source values are the small-signal excitation. Build the circuit twice with the same topology, once with the bias values for `op` and once with the excitation (a supply that is an AC ground at zero volts), and pass the `op` result of the first to `ac` of the second.

## Parameters

`circuit`
: The compiled circuit.

`omega`
: Angular frequencies in radians per second: a non-empty, finite, non-negative real vector on the circuit's platform.

`operating_point`
: The `op` result of the biased circuit. Required for a circuit with diodes or transistors; its `voltage` must have the circuit's node count and parameter batch.

`options`
: The `thermal_voltage` and `gmin` that the operating point was solved with; see `SolveOptions`. Use the same values as for `op`.

## Returns

A `Result` of complex arrays with a frequency axis before the final axis.

## Errors

Throws `pluss::Error` for a malformed `omega` or for singular equations at some frequency; for a circuit with devices, for the first overload (which has no operating point) and for an `operating_point` of a different shape.

## Interpretation and Use

`omega` is angular: for a frequency `f` in hertz use `2 pi f`. At `omega = 0` the result is the DC solution; with devices it is the linear response of the circuit to the excitation around the bias, not a second bias point. The gain of an amplifier stage is the ratio of the output phasor to the input excitation.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

An RC low-pass at its corner frequency.

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
### Example 2

A series resonance and an integrator.

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
### Example 3

A sweep of component values.

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
  net.voltage_sources = {{1}, {0}, vec({1.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({0.1e-6, 1e-6, 10e-6}).reshape({3, 1})};
  const auto result = ckt::ac(ckt::make(net), vec({100.0, 1000.0, 10000.0}));

  out.writeln("voltage shape", result.voltage.sizes(), " (capacitor value, frequency, node)");
  const pluss::Series gain = result.voltage.select(2, 2).abs();   // node 2, shape [3, 3]
  out.writeln("gain: rows are 0.1, 1 and 10 microfarad; columns 100, 1000 and 10000 rad/s");
  for (long row = 0; row < 3; ++row) out.writeln(flat(gain.select(0, row)));
}
```

Output

```text
voltage shape [3, 3, 3]  (capacitor value, frequency, node)
gain: rows are 0.1, 1 and 10 microfarad; columns 100, 1000 and 10000 rad/s
[0.99995, 0.99504, 0.70711]
[0.99504, 0.70711, 0.09950]
[0.70711, 0.09950, 0.01000]
```
### Example 4

Rejected frequency vectors.

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
  net.voltage_sources = {{1}, {0}, vec({1.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.capacitors = {{2}, {0}, vec({1e-6})};
  const ckt::Circuit circuit = ckt::make(net);
  report([&] { (void)ckt::ac(circuit, vec({-1.0})); });
  report([&] { (void)ckt::ac(circuit, pluss::series::zeros({0}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64))); });
  report([&] { (void)ckt::ac(circuit, pluss::series::zeros({2, 2}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64))); });
}
```

Output

```text
eng::circuit::ac: omega must be a non-empty finite non-negative real vector
eng::circuit::ac: omega must be a non-empty finite non-negative real vector
eng::circuit::ac: omega must be a non-empty finite non-negative real vector
```
### Example 5

The small-signal response of a diode around its bias point.

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
  // bias the diode with 5 V behind 1 kohm, then excite the same circuit with a 1 V small signal
  const auto build = [](double volts) {
    ckt::Net net;
    net.nodes = 3;
    net.voltage_sources = {{1}, {0}, vec({volts})};
    net.resistors = {{1}, {2}, vec({1000.0})};
    net.capacitors = {{2}, {0}, vec({1e-7})};
    net.diodes.positive = {2};
    net.diodes.negative = {0};
    net.diodes.saturation = vec({1e-14});
    return ckt::make(net);
  };
  const auto bias = ckt::op(build(5.0));
  const double rd = 0.02585 / flat(bias.diode_current)[0];   // the diode's dynamic resistance, n Vt / Id
  const auto response = ckt::ac(build(1.0), vec({0.0, 1e4, 1e5}), bias);
  const auto gain = response.voltage.select(-1, 2).abs();
  out.writeln("dynamic resistance", rd);
  out.writeln("DC gain rd / (R + rd)", rd / (1000.0 + rd), " ac at omega = 0", flat(gain)[0]);
  out.writeln("gain at 1e4 and 1e5 rad/s", flat(gain)[1], flat(gain)[2]);
}
```

Output

```text
dynamic resistance 6.001147
DC gain rd / (R + rd) 0.005965  ac at omega = 0 0.005965
gain at 1e4 and 1e5 rad/s 0.005965 0.005955
```
### Example 6

The gain of a transistor stage against its formula.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace ckt = pluss::subsp::eng::circuit;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static ckt::Circuit stage(double supply, double drive) {
  ckt::Net net;
  net.nodes = 5;
  net.voltage_sources = {{1, 3}, {0, 0}, vec({supply, drive})};
  net.resistors = {{1, 3}, {2, 4}, vec({1000.0, 10000.0})};
  net.transistors.collector = {2};
  net.transistors.base = {4};
  net.transistors.emitter = {0};
  net.transistors.polarity = {1};
  net.transistors.saturation = vec({1e-14});
  net.transistors.forward_gain = vec({100.0});
  return ckt::make(net);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto bias = ckt::op(stage(10.0, 1.2));
  const double gm = flat(bias.collector_current)[0] / 0.02585;
  const double rpi = 100.0 / gm;
  const auto gain = flat(ckt::ac(stage(0.0, 1.0), vec({0.0}), bias).voltage.select(-1, 2).real())[0];
  out.writeln("computed gain", gain, " formula -gm Rc rpi / (Rb + rpi)", -gm * 1000.0 * rpi / (10000.0 + rpi));
}
```

Output

```text
computed gain -9.5116  formula -gm Rc rpi / (Rb + rpi) -9.5116
```
### Example 7

A device circuit without its operating point, and with one of another circuit.

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
  ckt::Net net;
  net.nodes = 3;
  net.voltage_sources = {{1}, {0}, vec({1.0})};
  net.resistors = {{1}, {2}, vec({1000.0})};
  net.diodes.positive = {2};
  net.diodes.negative = {0};
  net.diodes.saturation = vec({1e-14});
  const auto circuit = ckt::make(net);
  try {
    (void)ckt::ac(circuit, vec({1.0}));
  } catch (const pluss::Error &e) {
    out.writeln(e.message());
  }
  // an operating point of a different circuit is rejected as well
  ckt::Net other;
  other.nodes = 2;
  other.voltage_sources = {{1}, {0}, vec({1.0})};
  other.resistors = {{1}, {0}, vec({1.0})};
  try {
    (void)ckt::ac(circuit, vec({1.0}), ckt::op(ckt::make(other)));
  } catch (const pluss::Error &e) {
    out.writeln(e.message());
  }
}
```

Output

```text
eng::circuit::ac: a circuit with diodes or transistors needs its operating point; call ac(circuit, omega, operating_point) with the result of op on the biased circuit
eng::circuit::ac: operating_point must be the op result of a circuit with the same nodes and parameter batch
```
<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="op.html">
    <span class="label">Previous</span>
    <span class="title">op</span>
  </a>
  <a class="next-link" href="tran.html">
    <span class="label">Next</span>
    <span class="title">tran</span>
  </a>
</div>
<!-- apiref:nav:end -->
