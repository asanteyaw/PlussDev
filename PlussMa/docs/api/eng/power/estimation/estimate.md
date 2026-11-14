<!-- generated from estimate.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `power::estimate`

Weighted least-squares state estimation from redundant measurements.

## Signature

```cpp
power::EstimateResult power::estimate(const power::Grid& grid, const power::Measurements& measurements, const power::EstimateOptions& options = {});
```

## What It Does

Finds the bus voltages that minimise `J(x) = sum(((z_i - h_i(x)) / sigma_i)^2)` over the measurements by Gauss-Newton iteration, with a fraction-to-boundary limiter on the step. The unknowns are the angles of every bus but the reference and every magnitude.

## Parameters

`grid`
: The compiled network.

`measurements`
: The measurement set.

`options`
: Tolerance, budget and observability threshold.

## Returns

An `EstimateResult`.

## Errors

Throws `std::invalid_argument` for fewer measurements than the `2n - 1` unknowns, non-positive standard deviations, an index outside its family, or an unobservable measurement set.

## Interpretation and Use

Use redundancy: more measurements than unknowns lets the covariance shrink and the residuals expose bad data. Read `objective` and `normalized_residual` after every estimate.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Ten measurements of a three-bus network.

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

Detecting a gross error.

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
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const power::Grid grid = power::make(three_bus());
  const auto truth = power::ac(grid);

  const auto clean = power::estimate(grid, measurements(truth));
  out.writeln("normalised residuals, clean data: ", flat(clean.normalized_residual));
  const auto covariance = flat(clean.covariance);   // [state][state], angles of buses 1 and 2, then three magnitudes
  std::vector<double> deviation;
  for (int i = 0; i < 5; ++i) deviation.push_back(std::sqrt(covariance[5 * i + i]));
  out.writeln("standard deviation of the estimated state: ", deviation, "   (every measurement had sigma = 0.01)");

  const auto bad = power::estimate(grid, measurements(truth, 0.3));
  out.writeln("with a 0.3 error on measurement 5 (active power entering line 0): J = ", flat(bad.objective), " against ", flat(clean.objective));
  out.writeln("normalised residuals: ", flat(bad.normalized_residual));
  out.writeln("a chi-square test with 5 degrees of freedom at 95 percent rejects J above 11.07: clean data ", flat(clean.objective)[0] > 11.07 ? "rejected" : "accepted", ", bad data ", flat(bad.objective)[0] > 11.07 ? "rejected" : "accepted");
}
```

Output

```text
normalised residuals, clean data: [-0.1112, -0.1454, 0.1505, 0.3908, -0.3012, 0.3667, -0.2717, 0.4038, 0.1357, 0.0899]
standard deviation of the estimated state: [0.0013, 0.0014, 0.0099, 0.0071, 0.0071]   (every measurement had sigma = 0.01)
with a 0.3 error on measurement 5 (active power entering line 0): J = [529.8492] against [0.3982]
normalised residuals: [-0.5538, -0.3124, 0.0865, 21.2091, 7.5120, 23.0128, -5.8337, 0.2815, -2.5070, 1.2983]
a chi-square test with 5 degrees of freedom at 95 percent rejects J above 11.07: clean data accepted, bad data rejected
```
### Example 3

Options.

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
  out.print_defaults(PrintOptions{}.fixed(7).with_sep(""));
  const power::Grid grid = power::make(three_bus());
  const auto data = measurements(power::ac(grid));

  const auto show = [&](const char *label, const power::EstimateOptions &options) {
    try {
      const auto r = power::estimate(grid, data, options);
      out.writeln(label, ": converged ", r.converged.item<bool>(), ", iterations ", r.iterations, ", magnitude at bus 2 ", flat(r.magnitude)[2]);
    } catch (const std::exception &e) {
      std::cout << label << ": " << e.what() << "\n";
    }
  };
  show("defaults                     ", power::EstimateOptions{});
  power::EstimateOptions loose;
  loose.tolerance = 1e-3;
  show("tolerance 1e-3               ", loose);
  power::EstimateOptions capped;
  capped.max_iterations = 2;
  show("max_iterations 2             ", capped);
  power::EstimateOptions strict;
  strict.observability_tolerance = 1.0;
  show("observability_tolerance 1.0  ", strict);
}
```

Output

```text
defaults                     : converged true, iterations 5, magnitude at bus 2 1.0061935
tolerance 1e-3               : converged true, iterations 2, magnitude at bus 2 1.0061963
max_iterations 2             : converged false, iterations 2, magnitude at bus 2 1.0061963
observability_tolerance 1.0  : eng::power::estimate: measurement Jacobian is not observable
```
### Example 4

Refused and misleading inputs.

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
  <a class="prev-link" href="EstimateResult.html">
    <span class="label">Previous</span>
    <span class="title">EstimateResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
