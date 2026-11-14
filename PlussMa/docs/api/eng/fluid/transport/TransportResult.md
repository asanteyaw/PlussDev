<!-- generated from TransportResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `fluid::TransportResult`

The quality at every hydraulic event, with the mass balance.

## Signature

```cpp
fluid::TransportResult
```

## What It Does

The result of `transport`: the hydraulic `RunResult` that drove it and the quality sampled at the same event times.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `hydraulic` | The `RunResult` used. |  |
| `node` | Mixed quality, `[sample, ..., node]`. |  |
| `link` | Volume mean quality of a pipe, upstream quality of a pump or valve, `[sample, ..., link]`. |  |
| `tank` | Quality of each fully mixed tank, `[sample, ..., tank]`. |  |
| `mass` | Stored quality-volume in the pipes and tanks, `[sample, ...]`. |  |
| `mass_error` | The inventory change minus the cumulative reactions and boundary fluxes, `[sample, ...]`. |  |

## Returns

Returned by `transport`.

## Errors

Reading a field does not run again.

## Interpretation and Use

`mass_error` should stay at rounding level in any run: a value that grows means the quality step is too coarse or a boundary is not accounted for. A transient is sampled at the hydraulic events only, so use a short `Schedule::step` to see a front arrive.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Two sources mixing, with the mass balance.

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
### Example 2

Decay and the mass error.

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
  const std::int64_t cells = 20;
  fluid::Schedule schedule;
  schedule.duration = 4.0 * 3600.0;
  schedule.step = kTransit / cells;
  schedule.pattern_step = 24.0 * 3600.0;
  fluid::RouteOptions route;
  route.step = kTransit / cells; // Courant number one: the front is carried exactly
  route.courant = 1.0;
  route.cells = cells;
  out.writeln("pipe volume ", kVolume, " m3, transit time ", kTransit, " s");
  for (double k : {0.0, 1e-4, 3e-4}) {
    fluid::Quality quality;
    quality.initial = vec({1.0, 0.0});
    quality.pipe_rate = vec({-k}); // a negative rate is a decay
    const auto r = fluid::transport(network, schedule, quality, route);
    out.writeln("k = ", k, ": junction ", flat(r.node).back(), ", exp(-k T) = ", std::exp(-k * kTransit), ", mass error ", flat(r.mass_error).back());
  }
  // the same decay with Courant 0.9 and 4 cells blurs the front
  fluid::RouteOptions coarse;
  coarse.step = 20.0;
  coarse.courant = 0.9;
  coarse.cells = 4;
  fluid::Quality quality;
  quality.initial = vec({1.0, 0.0});
  quality.pipe_rate = vec({-1e-4});
  fluid::Schedule longer = schedule;
  longer.step = 60.0;
  out.writeln("4 cells, Courant 0.9: junction ", flat(fluid::transport(network, longer, quality, coarse).node).back(), ", exact ", std::exp(-1e-4 * kTransit));
}
```

Output

```text
pipe volume 17.671459 m3, transit time 1767.145868 s
k = 0.000000: junction 1.000000, exp(-k T) = 1.000000, mass error -0.000000
k = 0.000100: junction 0.838020, exp(-k T) = 0.838019, mass error -0.000000
k = 0.000300: junction 0.588525, exp(-k T) = 0.588520, mass error -0.000000
4 cells, Courant 0.9: junction 0.841060, exact 0.838019
```
### Example 3

Age at the nodes.

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

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="RouteOptions.html">
    <span class="label">Previous</span>
    <span class="title">RouteOptions</span>
  </a>
  <a class="next-link" href="transport.html">
    <span class="label">Next</span>
    <span class="title">transport</span>
  </a>
</div>
<!-- apiref:nav:end -->
