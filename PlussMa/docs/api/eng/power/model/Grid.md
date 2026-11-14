<!-- generated from Grid.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `power::Grid`

An immutable compiled network.

## Signature

```cpp
std::int64_t power::Grid::buses() const noexcept;
std::int64_t power::Grid::gens() const noexcept;
std::int64_t power::Grid::lines() const noexcept;
const std::vector<std::int64_t>& power::Grid::batch_shape() const noexcept;
double power::Grid::base_power() const noexcept;
const power::Base& power::Grid::base() const noexcept;
const pluss::Series& power::Grid::ybus() const noexcept;
```

## What It Does

A `Grid` is what `power::make` returns: the validated network with its admittance matrices and incidence maps, held immutably so that one grid can be analysed repeatedly. Every analysis takes a `Grid`.

## Methods

`buses(), gens(), lines()`
: The numbers of buses, generators and lines.

`batch_shape()`
: The leading scenario axes shared by all values.

`base_power(), base()`
: The per-unit base.

`dtype(), platform()`
: The element type and device of the stored data.

`ybus(), yfrom(), yto()`
: The complex bus admittance matrix, and the admittances from each line's ends to the buses.

## Returns

Accessors return counts, the base and the admittance matrices.

## Errors

Accessors do not fail. Invalid networks are rejected by `make` before a `Grid` exists.

## Changelog

- **0.1** — Initial public contract.

## Interpretation and Use

A `Grid` cannot be default-constructed. The admittance matrix `Y` relates the complex bus current injections to the voltages, and `V conj(Y V)` is the complex power injected at every bus.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Counts and the admittance matrix of a two-bus grid.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <complex>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ};
  net.buses.p = vec({0.0, -0.5});       // the load bus draws 0.5 per unit of active power
  net.buses.q = vec({0.0, -0.2});       // and 0.2 of reactive power
  net.buses.vm = vec({1.0, 1.0});       // set point at the slack, starting guess at the load bus
  net.buses.va = vec({0.0, 0.0});
  net.lines.from = {0};
  net.lines.to = {1};
  net.lines.r = vec({0.01});
  net.lines.x = vec({0.1});

  const power::Grid grid = power::make(net);
  out.writeln("buses ", grid.buses(), ", generators ", grid.gens(), ", lines ", grid.lines(), ", base power (W) ", grid.base_power());
  const auto re = flat(grid.ybus().real()), im = flat(grid.ybus().imag());
  out.writeln("Y[0][0] from the grid: ", re[0], " ", im[0], "j   Y[0][1]: ", re[1], " ", im[1], "j");
  const std::complex<double> y = 1.0 / std::complex<double>(0.01, 0.1);
  out.writeln("1 / (r + jx) by hand:  ", y.real(), " ", y.imag(), "j");
}
```

Output

```text
buses 2, generators 0, lines 1, base power (W) 100000000.000000
Y[0][0] from the grid: 0.990099 -9.900990j   Y[0][1]: -0.990099 9.900990j
1 / (r + jx) by hand:  0.990099 -9.900990j
```
### Example 2

The admittance matrix used to check an AC solution.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <complex>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a slack at bus 0, a generator bus at bus 1 (1.02 p.u., 0.6 p.u. generation, 0.2 p.u. load) and a load bus at bus 2
static power::Net three_bus(power::BusKind second = power::BusKind::PV) {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, second, power::BusKind::PQ};
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

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const power::Grid grid = power::make(three_bus());
  const auto r = power::ac(grid);
  out.writeln("voltage magnitudes: ", flat(r.magnitude), ", angles (rad): ", flat(r.angle));
  out.writeln("converged ", r.converged.item<bool>(), " in ", r.iterations, " iterations, residual ", r.residual.item<double>());

  // the defining equation, rebuilt from the voltages
  const auto vr = flat(r.voltage.real()), vi = flat(r.voltage.imag()), yr = flat(grid.ybus().real()), yi = flat(grid.ybus().imag());
  const auto pr = flat(r.injection.real()), pi = flat(r.injection.imag());
  double worst = 0.0;
  for (int i = 0; i < 3; ++i) {
    std::complex<double> current = 0.0;
    for (int j = 0; j < 3; ++j) current += std::complex<double>(yr[3 * i + j], yi[3 * i + j]) * std::complex<double>(vr[j], vi[j]);
    const std::complex<double> power = std::complex<double>(vr[i], vi[i]) * std::conj(current);
    worst = std::max(worst, std::abs(power - std::complex<double>(pr[i], pi[i])));
    out.writeln("bus ", i, ": reported ", pr[i], " + ", pi[i], "j    V conj(Y V) = ", power.real(), " + ", power.imag(), "j");
  }
  out.writeln("largest difference: ", worst);

  // losses: the sum of the two ends of every line
  const auto from = flat(r.from_power.real()), to = flat(r.to_power.real());
  double loss = 0.0, injected = 0.0;
  for (std::size_t line = 0; line < from.size(); ++line) loss += from[line] + to[line];
  for (double v : pr) injected += v;
  out.writeln("active losses from the line ends: ", loss, "   sum of the injections: ", injected);
}
```

Output

```text
voltage magnitudes: [1.04000, 1.02000, 1.00575], angles (rad): [0.00000, 0.01136, -0.04632]
converged true in 3 iterations, residual 0.00000
bus 0: reported 0.10314 + 0.32097j    V conj(Y V) = 0.10314 + 0.32097j
bus 1: reported 0.40000 + -0.19852j    V conj(Y V) = 0.40000 + -0.19852j
bus 2: reported -0.50000 + -0.20000j    V conj(Y V) = -0.50000 + -0.20000j
largest difference: 0.00000
active losses from the line ends: 0.00314   sum of the injections: 0.00314
```
### Example 3

The batch shape of a scenario sweep.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

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

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));

  const auto base = power::ac(power::make(three_bus()));
  power::Net outage = three_bus();
  outage.lines.active = {true, true, false};   // line 0 - 2 out of service
  const auto cut = power::ac(power::make(outage));
  out.writeln("all lines in:         load-bus voltage ", flat(base.magnitude)[2], ", slack output (p.u.) ", flat(base.injection.real())[0]);
  out.writeln("line 0-2 out:         load-bus voltage ", flat(cut.magnitude)[2], ", slack output (p.u.) ", flat(cut.injection.real())[0]);

  // three load levels in one call: a leading axis of three on the injections
  power::Net sweep;
  sweep.base = {100e6, 230e3};
  sweep.buses.kind = {power::BusKind::Slack, power::BusKind::PQ};
  sweep.buses.p = pluss::series::make(std::vector<double>{0, -0.2, 0, -0.5, 0, -1.0}).reshape({3, 2});
  sweep.buses.q = pluss::series::make(std::vector<double>{0, -0.08, 0, -0.2, 0, -0.4}).reshape({3, 2});
  sweep.buses.vm = vec({1.0, 1.0});
  sweep.buses.va = vec({0.0, 0.0});
  sweep.lines.from = {0};
  sweep.lines.to = {1};
  sweep.lines.r = vec({0.01});
  sweep.lines.x = vec({0.1});
  const power::Grid grid = power::make(sweep);
  const auto r = power::ac(grid);
  out.writeln("batch shape of the grid: ", grid.batch_shape(), "; magnitude shape ", r.magnitude.sizes());
  out.writeln("load-bus voltage at 0.2, 0.5 and 1.0 per unit of load: ", flat(r.magnitude.select(-1, 1)));
}
```

Output

```text
all lines in:         load-bus voltage 1.00575, slack output (p.u.) 0.10314
line 0-2 out:         load-bus voltage 0.96833, slack output (p.u.) 0.10646
batch shape of the grid: [3]; magnitude shape [3, 2]
load-bus voltage at 0.2, 0.5 and 1.0 per unit of load: [0.98971, 0.97309, 0.94169]
```

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Net.html">
    <span class="label">Previous</span>
    <span class="title">Net</span>
  </a>
  <a class="next-link" href="make.html">
    <span class="label">Next</span>
    <span class="title">make</span>
  </a>
</div>
<!-- apiref:nav:end -->
