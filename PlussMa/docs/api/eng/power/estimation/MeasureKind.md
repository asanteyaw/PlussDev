<!-- generated from MeasureKind.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `power::MeasureKind`

The eight kinds of measurement the estimator understands.

## Signature

```cpp
power::MeasureKind
```

## What It Does

Names the physical quantity of each measurement: `FromP` and `ToP` (active power entering a line at the sending or receiving end), `FromQ` and `ToQ` (the reactive counterparts), `BusP` and `BusQ` (the net active and reactive injection at a bus), `Angle` and `Magnitude` (the voltage angle and magnitude at a bus).

## Fields

| Field | Meaning | Default |
|---|---|---|
| `FromP, ToP` | Active power entering a line at its sending or receiving end; `index` is the line. |  |
| `FromQ, ToQ` | Reactive power entering a line at its sending or receiving end; `index` is the line. |  |
| `BusP, BusQ` | Net active or reactive injection at a bus; `index` is the bus. |  |
| `Angle, Magnitude` | Voltage angle (radians) or magnitude at a bus; `index` is the bus. |  |

## Returns

An enumeration used in `Measurements::kind`.

## Errors

A measurement whose `index` lies outside its element family is rejected by `estimate`.

## Interpretation and Use

Bus-power measurements are net injections: generation minus consumption at the bus, not the output of a generator.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Each family appearing in a measurement set.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// the three-bus network of chapter 1: a slack, a generator bus held at 1.02, and a load bus
static power::Net three_bus() {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PV, power::BusKind::PQ};
  net.buses.p = vec({0.0, -0.2, -0.5});
  net.buses.q = vec({0.0, 0.0, -0.2});
  net.buses.vm = vec({1.04, 1.02, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0});
  net.gens.bus = {1};
  net.gens.p = vec({0.6});
  net.gens.q = vec({0.0});
  net.lines.from = {0, 1, 0};
  net.lines.to = {1, 2, 2};
  net.lines.r = vec({0.01, 0.02, 0.015});
  net.lines.x = vec({0.1, 0.2, 0.25});
  net.lines.charge = vec({0.02, 0.05, 0.04});
  return net;
}

// ten measurements taken from the exact solution of the network, each disturbed by a fixed error of the stated size:
// voltage magnitudes at every bus, net active power at buses 1 and 2, active power entering lines 0 and 2 at their sending ends,
// active power entering line 1 at its receiving end, reactive power entering line 1 at its sending end, and the angle at bus 2
static power::Measurements measurements(const power::AcResult &truth, double gross_error = 0.0) {
  const auto vm = flat(truth.magnitude), va = flat(truth.angle), injected = flat(truth.injection.real());
  const auto from_p = flat(truth.from_power.real()), from_q = flat(truth.from_power.imag()), to_p = flat(truth.to_power.real());
  power::Measurements m;
  m.kind = {power::MeasureKind::Magnitude, power::MeasureKind::Magnitude, power::MeasureKind::Magnitude, power::MeasureKind::BusP, power::MeasureKind::BusP,
            power::MeasureKind::FromP, power::MeasureKind::FromP, power::MeasureKind::ToP, power::MeasureKind::FromQ, power::MeasureKind::Angle};
  m.index = {0, 1, 2, 1, 2, 0, 2, 1, 1, 2};
  std::vector<double> value = {vm[0], vm[1], vm[2], injected[1], injected[2], from_p[0], from_p[2], to_p[1], from_q[1], va[2]};
  const std::vector<double> error = {0.002, -0.001, 0.0015, 0.004, -0.003, 0.002, -0.002, 0.003, -0.002, 0.001};
  for (std::size_t i = 0; i < value.size(); ++i) value[i] += error[i];
  value[5] += gross_error;
  m.value = vec(value);
  m.stddev = vec(std::vector<double>(value.size(), 0.01));
  return m;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const power::Grid grid = power::make(three_bus());
  const auto truth = power::ac(grid);
  const auto result = power::estimate(grid, measurements(truth));
  out.writeln("converged ", result.converged.item<bool>(), " in ", result.iterations, " iterations");
  out.writeln("estimated magnitudes ", flat(result.magnitude), "   true ", flat(truth.magnitude));
  out.writeln("estimated angles     ", flat(result.angle), "   true ", flat(truth.angle));
  out.writeln("independent least squares: magnitudes [1.042122, 1.020020, 1.006194], angles [0, 0.011622, -0.046207], objective 0.39826");
  out.writeln("objective J = ", flat(result.objective));
}
```

Output

```text
converged true in 5 iterations
estimated magnitudes [1.042122, 1.020019, 1.006194]   true [1.040000, 1.020000, 1.005749]
estimated angles     [0.000000, 0.011622, -0.046207]   true [0.000000, 0.011362, -0.046317]
independent least squares: magnitudes [1.042122, 1.020020, 1.006194], angles [0, 0.011622, -0.046207], objective 0.39826
objective J = [0.398250]
```
### Example 2

A measurement of a bus that does not exist.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// the three-bus network of chapter 1: a slack, a generator bus held at 1.02, and a load bus
static power::Net three_bus() {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PV, power::BusKind::PQ};
  net.buses.p = vec({0.0, -0.2, -0.5});
  net.buses.q = vec({0.0, 0.0, -0.2});
  net.buses.vm = vec({1.04, 1.02, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0});
  net.gens.bus = {1};
  net.gens.p = vec({0.6});
  net.gens.q = vec({0.0});
  net.lines.from = {0, 1, 0};
  net.lines.to = {1, 2, 2};
  net.lines.r = vec({0.01, 0.02, 0.015});
  net.lines.x = vec({0.1, 0.2, 0.25});
  net.lines.charge = vec({0.02, 0.05, 0.04});
  return net;
}

// ten measurements taken from the exact solution of the network, each disturbed by a fixed error of the stated size:
// voltage magnitudes at every bus, net active power at buses 1 and 2, active power entering lines 0 and 2 at their sending ends,
// active power entering line 1 at its receiving end, reactive power entering line 1 at its sending end, and the angle at bus 2
static power::Measurements measurements(const power::AcResult &truth, double gross_error = 0.0) {
  const auto vm = flat(truth.magnitude), va = flat(truth.angle), injected = flat(truth.injection.real());
  const auto from_p = flat(truth.from_power.real()), from_q = flat(truth.from_power.imag()), to_p = flat(truth.to_power.real());
  power::Measurements m;
  m.kind = {power::MeasureKind::Magnitude, power::MeasureKind::Magnitude, power::MeasureKind::Magnitude, power::MeasureKind::BusP, power::MeasureKind::BusP,
            power::MeasureKind::FromP, power::MeasureKind::FromP, power::MeasureKind::ToP, power::MeasureKind::FromQ, power::MeasureKind::Angle};
  m.index = {0, 1, 2, 1, 2, 0, 2, 1, 1, 2};
  std::vector<double> value = {vm[0], vm[1], vm[2], injected[1], injected[2], from_p[0], from_p[2], to_p[1], from_q[1], va[2]};
  const std::vector<double> error = {0.002, -0.001, 0.0015, 0.004, -0.003, 0.002, -0.002, 0.003, -0.002, 0.001};
  for (std::size_t i = 0; i < value.size(); ++i) value[i] += error[i];
  value[5] += gross_error;
  m.value = vec(value);
  m.stddev = vec(std::vector<double>(value.size(), 0.01));
  return m;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  const power::Grid grid = power::make(three_bus());
  const auto truth = power::ac(grid);
  const auto attempt = [&](const char *label, const power::Measurements &m) {
    try {
      const auto r = power::estimate(grid, m);
      out.writeln(label, ": accepted, J = ", flat(r.objective), ", normalised residuals ", flat(r.normalized_residual));
    } catch (const std::exception &e) {
      std::cout << label << ": " << e.what() << "\n";
    }
  };

  power::Measurements few = measurements(truth);
  few.kind.resize(3);
  few.index.resize(3);
  few.value = vec({1.04, 1.02, 1.0});
  few.stddev = vec({0.01, 0.01, 0.01});
  attempt("three measurements for five unknowns", few);

  power::Measurements zero = measurements(truth);
  auto sigma = flat(zero.stddev);
  sigma[0] = 0.0;
  zero.stddev = vec(sigma);
  attempt("a standard deviation of zero         ", zero);

  power::Measurements missing = measurements(truth);
  missing.index[0] = 9;
  attempt("a measurement of bus 9               ", missing);

  // bus 1 injects 0.4 net (0.6 generated, 0.2 consumed): the generator output 0.6 is not the quantity measured
  power::Measurements generator = measurements(truth);
  auto value = flat(generator.value);
  value[3] = 0.6 + 0.004;
  generator.value = vec(value);
  attempt("generator output 0.6 for the net 0.4 ", generator);
}
```

Output

```text
three measurements for five unknowns: eng::power::estimate: measurement count is smaller than the state dimension
a standard deviation of zero         : eng::power::estimate: standard deviations must be positive
a measurement of bus 9               : eng::power::estimate: measurement index lies outside its element family
generator output 0.6 for the net 0.4 : accepted, J = [166.715], normalised residuals [-2.324, 0.033, 0.372, 12.902, 4.141, 11.872, 0.405, 4.120, -0.033, -0.087]
```
### Example 3

A generator output given as a net injection.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// the three-bus network of chapter 1: a slack, a generator bus held at 1.02, and a load bus
static power::Net three_bus() {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PV, power::BusKind::PQ};
  net.buses.p = vec({0.0, -0.2, -0.5});
  net.buses.q = vec({0.0, 0.0, -0.2});
  net.buses.vm = vec({1.04, 1.02, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0});
  net.gens.bus = {1};
  net.gens.p = vec({0.6});
  net.gens.q = vec({0.0});
  net.lines.from = {0, 1, 0};
  net.lines.to = {1, 2, 2};
  net.lines.r = vec({0.01, 0.02, 0.015});
  net.lines.x = vec({0.1, 0.2, 0.25});
  net.lines.charge = vec({0.02, 0.05, 0.04});
  return net;
}

// ten measurements taken from the exact solution of the network, each disturbed by a fixed error of the stated size:
// voltage magnitudes at every bus, net active power at buses 1 and 2, active power entering lines 0 and 2 at their sending ends,
// active power entering line 1 at its receiving end, reactive power entering line 1 at its sending end, and the angle at bus 2
static power::Measurements measurements(const power::AcResult &truth, double gross_error = 0.0) {
  const auto vm = flat(truth.magnitude), va = flat(truth.angle), injected = flat(truth.injection.real());
  const auto from_p = flat(truth.from_power.real()), from_q = flat(truth.from_power.imag()), to_p = flat(truth.to_power.real());
  power::Measurements m;
  m.kind = {power::MeasureKind::Magnitude, power::MeasureKind::Magnitude, power::MeasureKind::Magnitude, power::MeasureKind::BusP, power::MeasureKind::BusP,
            power::MeasureKind::FromP, power::MeasureKind::FromP, power::MeasureKind::ToP, power::MeasureKind::FromQ, power::MeasureKind::Angle};
  m.index = {0, 1, 2, 1, 2, 0, 2, 1, 1, 2};
  std::vector<double> value = {vm[0], vm[1], vm[2], injected[1], injected[2], from_p[0], from_p[2], to_p[1], from_q[1], va[2]};
  const std::vector<double> error = {0.002, -0.001, 0.0015, 0.004, -0.003, 0.002, -0.002, 0.003, -0.002, 0.001};
  for (std::size_t i = 0; i < value.size(); ++i) value[i] += error[i];
  value[5] += gross_error;
  m.value = vec(value);
  m.stddev = vec(std::vector<double>(value.size(), 0.01));
  return m;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  const power::Grid grid = power::make(three_bus());
  const auto truth = power::ac(grid);
  const auto attempt = [&](const char *label, const power::Measurements &m) {
    try {
      const auto r = power::estimate(grid, m);
      out.writeln(label, ": accepted, J = ", flat(r.objective), ", normalised residuals ", flat(r.normalized_residual));
    } catch (const std::exception &e) {
      std::cout << label << ": " << e.what() << "\n";
    }
  };

  power::Measurements few = measurements(truth);
  few.kind.resize(3);
  few.index.resize(3);
  few.value = vec({1.04, 1.02, 1.0});
  few.stddev = vec({0.01, 0.01, 0.01});
  attempt("three measurements for five unknowns", few);

  power::Measurements zero = measurements(truth);
  auto sigma = flat(zero.stddev);
  sigma[0] = 0.0;
  zero.stddev = vec(sigma);
  attempt("a standard deviation of zero         ", zero);

  power::Measurements missing = measurements(truth);
  missing.index[0] = 9;
  attempt("a measurement of bus 9               ", missing);

  // bus 1 injects 0.4 net (0.6 generated, 0.2 consumed): the generator output 0.6 is not the quantity measured
  power::Measurements generator = measurements(truth);
  auto value = flat(generator.value);
  value[3] = 0.6 + 0.004;
  generator.value = vec(value);
  attempt("generator output 0.6 for the net 0.4 ", generator);
}
```

Output

```text
three measurements for five unknowns: eng::power::estimate: measurement count is smaller than the state dimension
a standard deviation of zero         : eng::power::estimate: standard deviations must be positive
a measurement of bus 9               : eng::power::estimate: measurement index lies outside its element family
generator output 0.6 for the net 0.4 : accepted, J = [166.715], normalised residuals [-2.324, 0.033, 0.372, 12.902, 4.141, 11.872, 0.405, 4.120, -0.033, -0.087]
```

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="Measurements.html">
    <span class="label">Next</span>
    <span class="title">Measurements</span>
  </a>
</div>
<!-- apiref:nav:end -->
