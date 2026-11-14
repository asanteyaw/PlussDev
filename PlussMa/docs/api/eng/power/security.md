<!-- generated from security.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Sensitivity and outage factors

This branch answers what-if questions with the linear DC model. `ptdf` gives the change of every line flow per unit of injection at every bus, `lodf` the redistribution of flow when a line is lost, and `outages` applies the factors to give the flows after every single-line outage in one call.

## Choosing A Surface

Use `ptdf` for changes of injection, and `outages` to screen every N-1 contingency of a network at once. Confirm the critical cases with `ac`.

## Practical Examples

The examples exercise the principal surface of this branch.

### Example 1

Power transfer factors of a ring against hand values.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a ring of three buses: the slack at bus 0, a load of 0.5 per unit at bus 2, and bus 1 between them.
// Lines: 0 joins buses 0-1 (x = 0.1), line 1 joins 1-2 (x = 0.2), line 2 joins 0-2 (x = 0.3).
static power::Net ring() {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ, power::BusKind::PQ};
  net.buses.p = vec({0.0, 0.0, -0.5});
  net.buses.q = vec({0.0, 0.0, -0.1});
  net.buses.vm = vec({1.0, 1.0, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0});
  net.lines.from = {0, 1, 0};
  net.lines.to = {1, 2, 2};
  net.lines.r = vec({0.01, 0.02, 0.03});
  net.lines.x = vec({0.1, 0.2, 0.3});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const power::Grid grid = power::make(ring());
  const auto factors = power::ptdf(grid);
  const auto f = flat(factors);
  out.writeln("shape of the PTDF matrix: ", factors.sizes(), " (line, bus)");
  for (int line = 0; line < 3; ++line) out.writeln("line ", line, ": ", f[3 * line], " ", f[3 * line + 1], " ", f[3 * line + 2]);
  out.writeln("by hand: bus 1 column -5/6, +1/6, -1/6 (the reactance ratio 0.5 : 0.1 of the two paths from bus 1 to the slack); bus 2 column -1/2, -1/2, -1/2");
}
```

Output

```text
shape of the PTDF matrix: [3, 3] (line, bus)
line 0: 0.00000 -0.83333 -0.50000
line 1: 0.00000 0.16667 -0.50000
line 2: 0.00000 -0.16667 -0.50000
by hand: bus 1 column -5/6, +1/6, -1/6 (the reactance ratio 0.5 : 0.1 of the two paths from bus 1 to the slack); bus 2 column -1/2, -1/2, -1/2
```
### Example 2

N-1 screening of a five-bus network.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }


// five buses: a slack generator at bus 0, a second generator of 4 per unit at bus 3, and loads of 3, 3 and 4 per unit at buses 1, 2 and 4.
// Lines: 0: 0-1, 1: 0-3, 2: 0-4, 3: 1-2, 4: 2-3, 5: 3-4.
static power::Net five_bus() {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ, power::BusKind::PQ, power::BusKind::PV, power::BusKind::PQ};
  net.buses.p = vec({0.0, -3.0, -3.0, 0.0, -4.0});
  net.buses.q = vec({0.0, -0.6, -0.6, 0.0, -0.8});
  net.buses.vm = vec({1.0, 1.0, 1.0, 1.0, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0, 0.0, 0.0});
  net.gens.bus = {3};
  net.gens.p = vec({4.0});
  net.gens.q = vec({0.0});
  net.lines.from = {0, 0, 0, 1, 2, 3};
  net.lines.to = {1, 3, 4, 2, 3, 4};
  net.lines.x = vec({0.028, 0.030, 0.006, 0.011, 0.030, 0.030});
  net.lines.r = vec({0.0028, 0.003, 0.0006, 0.0011, 0.003, 0.003});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  const power::Grid grid = power::make(five_bus());
  const auto result = power::outages(grid);
  const std::vector<double> rating = {4.0, 3.0, 4.5, 4.0, 3.5, 4.1};
  const auto base = flat(result.base_flow);
  const auto after = flat(result.flow);

  out.writeln("base flows:  ", base);
  out.writeln("ratings:     ", rating);
  int violations = 0;
  for (int k = 0; k < 6; ++k)
    for (int m = 0; m < 6; ++m)
      if (m != k && std::abs(after[6 * k + m]) > rating[m]) {
        ++violations;
        out.writeln("losing line ", k, " overloads line ", m, ": flow ", after[6 * k + m], " against a rating of ", rating[m]);
      }
  out.writeln(violations, " overloads in all");
}
```

Output

```text
base flows:  [3.006, -0.185, 3.179, 0.006, -2.994, 0.821]
ratings:     [4.000, 3.000, 4.500, 4.000, 3.500, 4.100]
losing line 0 overloads line 2: flow 4.545 against a rating of 4.500
losing line 0 overloads line 4: flow -6.000 against a rating of 3.500
losing line 4 overloads line 0: flow 6.000 against a rating of 4.000
3 overloads in all
```
### Example 3

Outages that split a network.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a ring of three buses: the slack at bus 0, a load of 0.5 per unit at bus 2, and bus 1 between them.
// Lines: 0 joins buses 0-1 (x = 0.1), line 1 joins 1-2 (x = 0.2), line 2 joins 0-2 (x = 0.3).
static power::Net ring() {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PQ, power::BusKind::PQ};
  net.buses.p = vec({0.0, 0.0, -0.5});
  net.buses.q = vec({0.0, 0.0, -0.1});
  net.buses.vm = vec({1.0, 1.0, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0});
  net.lines.from = {0, 1, 0};
  net.lines.to = {1, 2, 2};
  net.lines.r = vec({0.01, 0.02, 0.03});
  net.lines.x = vec({0.1, 0.2, 0.3});
  return net;
}


int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  power::Net spur = ring();
  spur.buses.kind.push_back(power::BusKind::PQ);   // a fourth bus on a spur from bus 2
  spur.buses.p = vec({0.0, 0.0, -0.5, -0.1});
  spur.buses.q = vec({0.0, 0.0, -0.1, -0.02});
  spur.buses.vm = vec({1.0, 1.0, 1.0, 1.0});
  spur.buses.va = vec({0.0, 0.0, 0.0, 0.0});
  spur.lines.from.push_back(2);
  spur.lines.to.push_back(3);
  spur.lines.r = vec({0.01, 0.02, 0.03, 0.01});
  spur.lines.x = vec({0.1, 0.2, 0.3, 0.1});
  const power::Grid grid = power::make(spur);

  const auto result = power::outages(grid);
  out.writeln("valid outages (one entry per line): ", flat(result.valid));
  const auto after = flat(result.flow);
  for (int k = 0; k < 4; ++k) out.writeln("flows after losing line ", k, ": ", after[4 * k], " ", after[4 * k + 1], " ", after[4 * k + 2], " ", after[4 * k + 3]);
  out.writeln("with tolerance 0.9, valid outages: ", flat(power::lodf(grid, 0.9).valid));
}
```

Output

```text
valid outages (one entry per line): [1.000, 1.000, 1.000, 0.000]
flows after losing line 0: 0.000 0.000 0.600 0.100
flows after losing line 1: 0.000 0.000 0.600 0.100
flows after losing line 2: 0.600 0.600 0.000 0.100
flows after losing line 3: nan nan nan nan
with tolerance 0.9, valid outages: [0.000, 0.000, 0.000, 0.000]
```

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

security/LodfResult
security/OutageResult
security/ptdf
security/lodf
security/outages
```
<!-- apiref:toctree:end -->
