<!-- generated from QualityMode.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `fluid::QualityMode`

Whether the transported quantity is a concentration or an age.

## Signature

```cpp
fluid::QualityMode
```

## What It Does

Chooses what `transport` follows. `Constituent` is a non-negative concentration with an initial field, optional sources and first-order reactions. `Age` is the same conservation law with age zero at the reservoirs and a unit source of one second per second.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `Constituent` | A concentration-like scalar. |  |
| `Age` | The time since the water left a reservoir, s. No source or reaction field is accepted. |  |

## Returns

An enumeration; `Quality::mode`.

## Errors

Age with a source or reaction field is rejected by `transport`.

## Interpretation and Use

Use `Age` to find where the water has sat without knowing what is in it. In steady flow the age at the end of a pipe settles at its volume over the flow.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

The age of the water along a pipe.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <functional>
#include <iostream>

using namespace pluss::tils;
namespace fluid = pluss::subsp::eng::fluid;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static void attempt(const char *label, const std::function<void()> &call) {
  try {
    call();
    std::cout << label << ": accepted\n";
  } catch (const std::exception &e) {
    std::cout << label << ": " << e.what() << "\n";
  }
}

// a reservoir at 50 m feeding a junction through a 1 km pipe of 0.15 m; the pipe holds 17.67 m3 and the draw is 0.01 m3/s
static fluid::Net line(double demand = 0.01) {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 5.0});
  net.nodes.demand = vec({0.0, demand});
  net.nodes.head = vec({50.0, 49.0});
  net.pipes.from = {0};
  net.pipes.to = {1};
  net.pipes.length = vec({1000.0});
  net.pipes.diameter = vec({0.15});
  net.pipes.roughness = vec({1e-4});
  net.pipes.minor = vec({0.0});
  return net;
}

static const double kPi = 3.14159265358979323846;
static const double kVolume = kPi * 0.15 * 0.15 / 4.0 * 1000.0; // m3
static const double kTransit = kVolume / 0.01;                   // s

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2).with_sep(""));
  const auto network = fluid::network(line());
  fluid::Schedule schedule;
  schedule.duration = 3.0 * 3600.0;
  schedule.step = 60.0;
  schedule.pattern_step = 24.0 * 3600.0;
  fluid::Quality age;
  age.mode = fluid::QualityMode::Age;
  fluid::RouteOptions route;
  route.step = 20.0;
  route.courant = 1.0;
  route.cells = 20;
  const auto r = fluid::transport(network, schedule, age, route);
  const auto time = flat(r.hydraulic.time), node = flat(r.node);
  for (std::size_t i = 0; i < time.size(); i += 24)
    out.writeln("t ", time[i], " s: age at the reservoir ", node[2 * i], " s, at the junction ", node[2 * i + 1], " s");
  out.writeln("age at the end ", node.back(), " s; pipe volume / flow = ", kTransit, " s");
}
```

Output

```text
t 0.00 s: age at the reservoir 0.00 s, at the junction 0.00 s
t 1440.00 s: age at the reservoir 0.00 s, at the junction 1414.48 s
t 2880.00 s: age at the reservoir 0.00 s, at the junction 1766.64 s
t 4320.00 s: age at the reservoir 0.00 s, at the junction 1767.15 s
t 5760.00 s: age at the reservoir 0.00 s, at the junction 1767.15 s
t 7200.00 s: age at the reservoir 0.00 s, at the junction 1767.15 s
t 8640.00 s: age at the reservoir 0.00 s, at the junction 1767.15 s
t 10080.00 s: age at the reservoir 0.00 s, at the junction 1767.15 s
age at the end 1767.15 s; pipe volume / flow = 1767.15 s
```
### Example 2

A constituent that mixes.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <functional>
#include <iostream>

using namespace pluss::tils;
namespace fluid = pluss::subsp::eng::fluid;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static void attempt(const char *label, const std::function<void()> &call) {
  try {
    call();
    std::cout << label << ": accepted\n";
  } catch (const std::exception &e) {
    std::cout << label << ": " << e.what() << "\n";
  }
}

// two reservoirs feed junction 2 through pipes of different length
static fluid::Net two_sources() {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Reservoir, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 0.0, 5.0});
  net.nodes.demand = vec({0.0, 0.0, 0.02});
  net.nodes.head = vec({50.0, 48.0, 40.0});
  net.pipes.from = {0, 1};
  net.pipes.to = {2, 2};
  net.pipes.length = vec({600.0, 300.0});
  net.pipes.diameter = vec({0.15, 0.15});
  net.pipes.roughness = vec({1e-4, 1e-4});
  net.pipes.minor = vec({0.0, 0.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const auto network = fluid::network(two_sources());
  const auto flow = flat(fluid::steady(network).flow);
  fluid::Schedule schedule;
  schedule.duration = 1800.0;
  schedule.step = 120.0;
  schedule.pattern_step = 24.0 * 3600.0;
  fluid::Quality quality;
  quality.initial = vec({1.0, 0.0, 0.0});
  fluid::RouteOptions route;
  route.step = 10.0;
  route.courant = 0.9;
  route.cells = 20;
  const auto r = fluid::transport(network, schedule, quality, route);
  out.writeln("shapes: node ", r.node.sizes(), ", link ", r.link.sizes(), ", tank ", r.tank.sizes(), ", mass ", r.mass.sizes());
  const auto time = flat(r.hydraulic.time), node = flat(r.node), mass = flat(r.mass), err = flat(r.mass_error);
  for (std::size_t i = 0; i < time.size(); i += 3)
    out.writeln("t ", time[i], " s: junction ", node[3 * i + 2], ", stored mass ", mass[i], ", mass error ", err[i]);
  out.writeln("final ", node.back(), ", flow-weighted average ", flow[0] / (flow[0] + flow[1]));
}
```

Output

```text
shapes: node [16, 3], link [16, 2], tank [16, 0], mass [16]
t 0.00000 s: junction 0.00000, stored mass 0.00000, mass error 0.00000
t 360.00000 s: junction 0.00005, stored mass 4.72697, mass error -0.00000
t 720.00000 s: junction 0.20884, stored mass 9.11669, mass error -0.00000
t 1080.00000 s: junction 0.62419, stored mass 10.54425, mass error -0.00000
t 1440.00000 s: junction 0.65622, stored mass 10.60245, mass error -0.00000
t 1800.00000 s: junction 0.65652, stored mass 10.60287, mass error -0.00000
final 0.65652, flow-weighted average 0.65653
```
### Example 3

Fields that Age refuses.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <functional>
#include <iostream>

using namespace pluss::tils;
namespace fluid = pluss::subsp::eng::fluid;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static void attempt(const char *label, const std::function<void()> &call) {
  try {
    call();
    std::cout << label << ": accepted\n";
  } catch (const std::exception &e) {
    std::cout << label << ": " << e.what() << "\n";
  }
}

// a reservoir at 50 m feeding a junction through a 1 km pipe of 0.15 m; the pipe holds 17.67 m3 and the draw is 0.01 m3/s
static fluid::Net line(double demand = 0.01) {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 5.0});
  net.nodes.demand = vec({0.0, demand});
  net.nodes.head = vec({50.0, 49.0});
  net.pipes.from = {0};
  net.pipes.to = {1};
  net.pipes.length = vec({1000.0});
  net.pipes.diameter = vec({0.15});
  net.pipes.roughness = vec({1e-4});
  net.pipes.minor = vec({0.0});
  return net;
}

static const double kPi = 3.14159265358979323846;
static const double kVolume = kPi * 0.15 * 0.15 / 4.0 * 1000.0; // m3
static const double kTransit = kVolume / 0.01;                   // s

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const auto network = fluid::network(line());
  fluid::Schedule schedule;
  schedule.duration = 3600.0;
  schedule.step = 600.0;
  schedule.pattern_step = 24.0 * 3600.0;
  attempt("a constituent with no initial quality", [&] { fluid::Quality q; (void)fluid::transport(network, schedule, q); });
  attempt("age with a decay rate", [&] { fluid::Quality q; q.mode = fluid::QualityMode::Age; q.pipe_rate = vec({-1e-4}); (void)fluid::transport(network, schedule, q); });
  attempt("a negative concentration", [&] { fluid::Quality q; q.initial = vec({-1.0, 0.0}); (void)fluid::transport(network, schedule, q); });
  attempt("an initial field of the wrong width", [&] { fluid::Quality q; q.initial = vec({1.0}); (void)fluid::transport(network, schedule, q); });
  attempt("a Courant limit of 1.5", [&] { fluid::Quality q; q.initial = vec({1.0, 0.0}); fluid::RouteOptions r; r.courant = 1.5; (void)fluid::transport(network, schedule, q, r); });
  attempt("zero cells", [&] { fluid::Quality q; q.initial = vec({1.0, 0.0}); fluid::RouteOptions r; r.cells = 0; (void)fluid::transport(network, schedule, q, r); });
  attempt("a quality step of zero", [&] { fluid::Quality q; q.initial = vec({1.0, 0.0}); fluid::RouteOptions r; r.step = 0.0; (void)fluid::transport(network, schedule, q, r); });
}
```

Output

```text
a constituent with no initial quality: eng::fluid::transport: initial quality is required
age with a decay rate: eng::fluid::transport: Age supplies its zero boundary and unit growth law; source and reaction fields are not accepted
a negative concentration: eng::fluid::transport: initial quality must be nonnegative
an initial field of the wrong width: eng::fluid::transport: initial quality must be real floating [..., 2]
a Courant limit of 1.5: eng::fluid::transport: route step, Courant number, flow tolerance, and cell count are invalid
zero cells: eng::fluid::transport: route step, Courant number, flow tolerance, and cell count are invalid
a quality step of zero: eng::fluid::transport: route step, Courant number, flow tolerance, and cell count are invalid
```

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="Quality.html">
    <span class="label">Next</span>
    <span class="title">Quality</span>
  </a>
</div>
<!-- apiref:nav:end -->
