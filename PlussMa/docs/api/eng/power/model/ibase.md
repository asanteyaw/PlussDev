<!-- generated from ibase.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `power::ibase`

Current base of a per-unit system.

## Signature

```cpp
double power::ibase(const power::Base& base);
```

## What It Does

Returns `power / (sqrt(3) voltage)`, the line current in amperes that equals `1` per unit.

## Parameters

`base`
: The power and voltage bases.

## Returns

The current base in amperes.

## Errors

Throws `std::invalid_argument` for a base that is not positive and finite.

## Interpretation and Use

Divide a line current in amperes by it to get per unit.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The three derived bases.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace power = pluss::ma::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const power::Base base{100e6, 230e3};   // 100 MVA, 230 kV line to line
  out.writeln("impedance base (ohm): ", power::zbase(base));
  out.writeln("admittance base (S):  ", power::ybase(base));
  out.writeln("current base (A):     ", power::ibase(base));
  out.writeln("50 MW in per unit: ", flat(power::pu(vec({50e6}), base.power)), "   0.5 per unit in watts: ", flat(power::si(vec({0.5}), base.power)));
  out.writeln("52.9 ohm in per unit: ", flat(power::pu(vec({52.9}), power::zbase(base))));
}
```

Output

```text
impedance base (ohm): 529.00000
admittance base (S):  0.00189
current base (A):     251.02186
50 MW in per unit: [0.50000]   0.5 per unit in watts: [50000000.00000]
52.9 ohm in per unit: [0.10000]
```
### Example 2

A base in a network.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <complex>

using namespace pluss::tils;
namespace power = pluss::ma::eng::power;

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
### Example 3

A rejected base.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <functional>

using namespace pluss::tils;
namespace power = pluss::ma::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static power::Net two_bus() {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ};
  net.buses.p = vec({0.0, -0.5});
  net.buses.q = vec({0.0, -0.2});
  net.buses.vm = vec({1.0, 1.0});
  net.buses.va = vec({0.0, 0.0});
  net.lines.from = {0};
  net.lines.to = {1};
  net.lines.r = vec({0.01});
  net.lines.x = vec({0.1});
  return net;
}

static void report(const char *what, const std::function<void()> &build) {
  try {
    build();
    std::cout << what << ": accepted\n";
  } catch (const std::exception &e) {
    std::cout << what << ": " << e.what() << "\n";
  }
}

int main() {
  report("no slack bus          ", [] { auto n = two_bus(); n.buses.kind = {power::BusKind::PQ, power::BusKind::PQ}; (void)power::make(n); });
  report("two slack buses       ", [] { auto n = two_bus(); n.buses.kind = {power::BusKind::Slack, power::BusKind::Slack}; (void)power::make(n); });
  report("an isolated bus       ", [] { auto n = two_bus(); n.lines.active = {false}; (void)power::make(n); });
  report("zero line impedance   ", [] { auto n = two_bus(); n.lines.r = vec({0.0}); n.lines.x = vec({0.0}); (void)power::make(n); });
  report("generator off the grid", [] { auto n = two_bus(); n.gens.bus = {7}; n.gens.p = vec({0.1}); n.gens.q = vec({0.0}); (void)power::make(n); });

  // a 50 MW load written as 50 where per unit was meant: accepted, and the answer is nonsense
  power::Net watts = two_bus();
  watts.buses.p = vec({0.0, -50.0});
  watts.buses.q = vec({0.0, -20.0});
  const auto r = power::ac(power::make(watts));
  std::cout << "a load of 50 per unit (meant 0.5): accepted, converged " << r.converged.item<bool>() << ", load-bus voltage " << flat(r.magnitude)[1] << "\n";
}
```

Output

```text
no slack bus          : eng::power::make: exactly one slack bus is required
two slack buses       : eng::power::make: exactly one slack bus is required
an isolated bus       : eng::power::make: every bus must connect to the slack island
zero line impedance   : eng::power::make: line series impedance cannot be zero
generator off the grid: eng::power::make: generator bus lies outside the network
a load of 50 per unit (meant 0.5): accepted, converged 0, load-bus voltage 4.48327
```

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ybase.html">
    <span class="label">Previous</span>
    <span class="title">ybase</span>
  </a>
  <a class="next-link" href="pu.html">
    <span class="label">Next</span>
    <span class="title">pu</span>
  </a>
</div>
<!-- apiref:nav:end -->
