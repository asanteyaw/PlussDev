<!-- generated from transport.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `fluid::transport`

Follows a constituent or the age of the water through a run.

## Signature

```cpp
TransportResult transport(const Network &network, const Schedule &schedule, const Quality &quality, const RouteOptions &route_options = {}, const Options &hydraulic_options = {});
```

## What It Does

Calls `run` for the hydraulics and advances the quality through each hydraulic interval with the flows held fixed: conservative first-order upwind transport in equal-volume pipe cells, complete mixing at junctions, fully mixed tanks, zero-volume pumps and valves, and exact first-order reactions.

## Parameters

`network`
: The compiled `Network`.

`schedule`
: The `Schedule` of the hydraulics.

`quality`
: The `Quality`.

`route_options`
: The `RouteOptions`.

`hydraulic_options`
: The `Options` of the hydraulic solves.

## Returns

A `TransportResult`.

## Errors

Throws `std::invalid_argument` for invalid quality or route options. Throws `std::runtime_error` for the failures of `run`.

## Interpretation and Use

The quality can only be as right as the flows that carry it. Choose cells and step for the sharpness of the front you need, and read `mass_error` after every run.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Two sources mixing at a junction.

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

A decay checked against its exact value.

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

Quality that is refused.

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
  <a class="prev-link" href="TransportResult.html">
    <span class="label">Previous</span>
    <span class="title">TransportResult</span>
  </a>
  <a class="next-link" href="Heat.html">
    <span class="label">Next</span>
    <span class="title">Heat</span>
  </a>
</div>
<!-- apiref:nav:end -->
