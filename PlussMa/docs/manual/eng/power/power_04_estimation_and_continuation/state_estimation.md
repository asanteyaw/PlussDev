<!-- generated from state_estimation.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# State estimation

[Back to Power 4: Estimation and Continuation](../power_04_estimation_and_continuation.md)

The power flow turns known injections into voltages. A control room has the opposite problem: a set of noisy measurements, some of them redundant, some possibly wrong, and it needs the best estimate of the voltages on every bus. **Weighted least-squares state estimation** finds the voltages that fit all the measurements best, weighting each by its accuracy. `power::estimate` does it for the AC network. This page explains the problem, estimates the state of a network from ten measurements of every family, checks it against an independent least-squares solver, reads the residuals and the covariance, finds a gross error by its normalised residual, and shows the mistakes the estimator is built to reject.

## The mental model

The **state** is the voltage magnitude at every bus and the angle at every bus except the reference, `2 n - 1` unknowns for a network of `n` buses. A **measurement** is a number with a known accuracy `sigma`: a voltage magnitude or angle, an active or reactive power injected at a bus, or the active or reactive power entering a line at either end. Each measurement is a known function `h(x)` of the state through the network equations. The estimate minimises the weighted sum of squares

```{math}
J(x) = \sum_i \left( \frac{z_i - h_i(x)}{\sigma_i} \right)^2
```

by Gauss-Newton iteration. A `Measurements` has `kind` (one of eight `MeasureKind` families), `index` (the bus or line the measurement belongs to), `value` and `stddev`, listed in any order and with the measurement axis last; bus-power measurements are **net** bus injections, not generator outputs. `estimate(grid, measurements, options)` returns an `EstimateResult`:

| Member | Shape | Meaning |
|---|---|---|
| `voltage`, `magnitude`, `angle` | `[..., bus]` | the estimated bus voltages |
| `injection`, `from_power`, `to_power` | `[..., bus]`, `[..., line]` | the powers implied by the estimate |
| `predicted` | `[..., measurement]` | `h(x)` at the estimate, in the order given |
| `residual` | `[..., measurement]` | `z - h(x)` |
| `normalized_residual` | `[..., measurement]` | the residual divided by its own standard deviation |
| `objective` | `[...]` | the minimum `J` |
| `covariance` | `[..., state, state]` | the covariance of the estimated state, angles first and then magnitudes |
| `converged`, `iterations` | | the diagnostics of the iteration |

`EstimateOptions` holds `tolerance` (`1e-8`), `max_iterations` (`20`) and `observability_tolerance` (`1e-10`). The network must be **observable**: the measurements must determine all `2 n - 1` unknowns, which needs at least that many of them, suitably placed, and the estimator reports an unobservable measurement set as an error.

## Estimating a state

The measurements are taken from the exact solution of the three-bus network, each disturbed by an error of up to `0.004` per unit (the estimator knows only that `sigma = 0.01`). The five unknowns are the angles at buses `1` and `2` and the three magnitudes, and there are ten measurements: five degrees of freedom of redundancy. The estimate is checked against an independent weighted least-squares solver (a general-purpose nonlinear least-squares solver given the same network equations and the same data), which gives magnitudes `1.042122, 1.020020, 1.006194`, angles `0, 0.011622, -0.046207` and a minimum of `J` of `0.39826`. The program also prints the true state.

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

The estimate agrees with the independent solver to six digits and sits closer to the true state than the raw measurements do: the estimated angle of bus `2`, `-0.046207`, is within `0.0001` of the true `-0.046317`, and the magnitude at bus `1`, `1.020019`, within `2e-5` of the true value. The minimum `J` is `0.3983`: with five degrees of freedom the expected value is about five, and a value this small says the data fit the model better than the stated accuracy.

## Residuals, covariance and a gross error

Two diagnostics say how far to trust the result. The **normalised residuals** are the measurement errors in units of their own standard deviation: with good data they are near or below one, and a measurement that is much larger is suspect. The **covariance** of the state gives the accuracy of the estimate itself, and is smaller than the accuracy of any single measurement, because the measurements share information. The program prints both for the clean data, then adds a gross error of `0.3` per unit to the measurement of the active power entering line `0`, and shows how the objective and the normalised residuals react.

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

The clean data have normalised residuals of at most `0.4`, and the angles of the estimated state have standard deviations of `0.0013` and `0.0014`, far better than the `0.01` of any single measurement, and the magnitudes `0.007` to `0.01`. A gross error of thirty standard deviations raises `J` from `0.4` to `530`, far past the `11.07` that a chi-square test at 95 percent allows for five degrees of freedom, and the normalised residuals single out the measurement at fault: measurement `5`, the one with the error, at `23.0`, together with the measurements it contradicts, at `21.2` and `7.5`. The estimator reports this evidence and does not delete the measurement or choose a threshold: that is the operator's decision, which is why the objective and the residuals are returned.

## Options

`tolerance` is the size of the state update at which the iteration stops, `max_iterations` caps it, and `observability_tolerance` is the smallest eigenvalue of the gain matrix that is accepted as observable. The program estimates with a loose tolerance of `1e-3`, with an iteration cap of `2`, and with an observability tolerance of `1`, which is larger than the gain's smallest eigenvalue and so declares the measurement set unobservable.

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

The loose tolerance stops sooner and gives the magnitude at bus `2` to about four digits; a budget of `2` iterations ends before the iteration converges and says so; and an observability tolerance above the smallest eigenvalue of the gain refuses the problem as unobservable. Leave the observability tolerance alone unless you want to reject poorly conditioned measurement sets.

## Decisions

- **Redundancy:** more measurements than the `2 n - 1` unknowns, spread over the network; redundancy is what lets the covariance shrink and the residuals expose bad data.
- **Weights:** `stddev` the real accuracy of each instrument; the estimate trusts a measurement in inverse proportion to it.
- **Bad data:** read the normalised residuals and the objective after every estimate, remove the largest suspect measurement, and estimate again.

## Pitfalls

The estimator is built to reject unusable inputs, and to trust the data it is given. The program makes four mistakes: too few measurements, a standard deviation of zero, a measurement of a bus that does not exist, and a generator's output given where a bus's net injection was meant. The last is accepted, and what it does to the residuals is the point.

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

The first three are refused with an error that names the problem. The fourth is a valid measurement of the wrong quantity: bus `1` injects a net `0.4`, the `0.6` generated less the `0.2` consumed there. The estimator accepts it, pulls the whole state toward the contradiction, and the objective climbs to about `167`, with large normalised residuals on the measurements that disagree. A check of the objective after every run catches this class of mistake, which no input validation can.

Reference: [`estimate`](../../../../api/eng/power/estimation/estimate.md), [`Measurements`](../../../../api/eng/power/estimation/Measurements.md), [`MeasureKind`](../../../../api/eng/power/estimation/MeasureKind.md), [`EstimateOptions`](../../../../api/eng/power/estimation/EstimateOptions.md), [`EstimateResult`](../../../../api/eng/power/estimation/EstimateResult.md).

Where next: [Continuation power flow](continuation_power_flow.md) follows the voltages of a network as load increases, through the point where the ordinary power flow fails.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="continuation_power_flow.html">
    <span class="label">Next</span>
    <span class="title">Continuation power flow</span>
  </a>
</div>
<!-- apiref:nav:end -->
