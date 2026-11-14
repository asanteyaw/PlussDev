<!-- generated from steady.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Steady hydraulics

This branch solves a network at one instant. `steady` finds the junction heads and the link flows as one coupled nonlinear system, with a choice of head-loss law and of fixed or pressure-driven demand.

## Choosing A Surface

Use `steady` for a single operating point, a family of scenarios, or a sensitivity. Check `converged` and, under fixed demands, the pressures.

## Practical Examples

The examples exercise the principal surface of this branch.

### Example 1

Every field of a looped network.

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

// a reservoir (node 0) and four junctions joined by six pipes that form two loops
static fluid::Net looped() {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction, fluid::NodeKind::Junction,
                    fluid::NodeKind::Junction, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 10.0, 12.0, 8.0, 15.0});
  net.nodes.demand = vec({0.0, 0.010, 0.015, 0.012, 0.008});
  net.nodes.head = vec({60.0, 55.0, 55.0, 55.0, 55.0});
  net.pipes.from = {0, 1, 1, 2, 3, 2};
  net.pipes.to = {1, 2, 3, 4, 4, 3};
  net.pipes.length = vec({800.0, 500.0, 600.0, 450.0, 550.0, 300.0});
  net.pipes.diameter = vec({0.30, 0.20, 0.20, 0.15, 0.15, 0.10});
  net.pipes.roughness = vec({1e-4, 1e-4, 1e-4, 1e-4, 1e-4, 1e-4});
  net.pipes.minor = vec({0.0, 0.0, 0.0, 0.0, 0.0, 0.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const auto r = fluid::steady(fluid::network(looped()));
  out.writeln("head              ", flat(r.head));
  out.writeln("pressure (kPa)    ", (r.pressure / 1000.0).reshape({r.pressure.numel()}).items<double>());
  out.writeln("flow              ", flat(r.flow));
  out.writeln("velocity          ", flat(r.velocity));
  out.writeln("head loss         ", flat(r.head_loss));
  out.writeln("friction factor   ", flat(r.friction_factor));
  out.writeln("balance           ", flat(r.balance));
  out.writeln("demand delivered  ", flat(r.demand));
  out.writeln("emitter outflow   ", flat(r.emitter));
  out.writeln("converged ", flat(r.converged)[0], " in ", r.iterations, " iterations");
  out.writeln("largest energy residual ", r.energy_residual.abs().max().item<double>(), " m");
}
```

Output

```text
head              [60.000000, 59.002957, 58.118057, 58.146721, 57.917692]
pressure (kPa)    [587.339882, 479.689849, 451.449572, 490.886153, 420.121206]
flow              [0.045000, 0.018502, 0.016498, 0.004074, 0.003926, -0.000572]
velocity          [0.636620, 0.588934, 0.525151, 0.230515, 0.222192, -0.072783]
head loss         [0.997043, 0.884900, 0.856236, 0.200365, 0.229029, -0.028664]
friction factor   [0.018094, 0.020016, 0.020298, 0.024652, 0.024815, 0.035375]
balance           [-0.045000, -0.000000, 0.000000, -0.000000, 0.000000]
demand delivered  [0.000000, 0.010000, 0.015000, 0.012000, 0.008000]
emitter outflow   [0.000000, 0.000000, 0.000000, 0.000000, 0.000000]
converged 1.000000 in 3 iterations
largest energy residual 0.000000 m
```
### Example 2

Tolerances and iteration limits.

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

// a reservoir (node 0) and four junctions joined by six pipes that form two loops
static fluid::Net looped() {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction, fluid::NodeKind::Junction,
                    fluid::NodeKind::Junction, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 10.0, 12.0, 8.0, 15.0});
  net.nodes.demand = vec({0.0, 0.010, 0.015, 0.012, 0.008});
  net.nodes.head = vec({60.0, 55.0, 55.0, 55.0, 55.0});
  net.pipes.from = {0, 1, 1, 2, 3, 2};
  net.pipes.to = {1, 2, 3, 4, 4, 3};
  net.pipes.length = vec({800.0, 500.0, 600.0, 450.0, 550.0, 300.0});
  net.pipes.diameter = vec({0.30, 0.20, 0.20, 0.15, 0.15, 0.10});
  net.pipes.roughness = vec({1e-4, 1e-4, 1e-4, 1e-4, 1e-4, 1e-4});
  net.pipes.minor = vec({0.0, 0.0, 0.0, 0.0, 0.0, 0.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const auto network = fluid::network(looped());
  for (std::int64_t limit : {1, 2, 3}) {
    fluid::Options options;
    options.max_iterations = limit;
    options.fail_on_nonconvergence = false;
    const auto s = fluid::steady(network, options);
    out.writeln("max_iterations ", limit, ": converged ", flat(s.converged)[0], ", iterations ", s.iterations, ", largest energy residual ", s.energy_residual.abs().max().item<double>());
  }
  fluid::Options strict;
  strict.max_iterations = 1;
  attempt("max_iterations 1, failing on nonconvergence", [&] { (void)fluid::steady(network, strict); });
  for (double tolerance : {1e-3, 1e-6, 1e-12}) {
    fluid::Options options;
    options.head_tolerance = tolerance;
    options.flow_tolerance = tolerance * 1e-2;
    out.writeln("head_tolerance 1e-", int(-std::log10(tolerance) + 0.5), ": iterations ", fluid::steady(network, options).iterations);
  }
}
```

Output

```text
max_iterations 1: converged 0.000000, iterations 1, largest energy residual 0.002334
max_iterations 2: converged 0.000000, iterations 2, largest energy residual 0.000004
max_iterations 3: converged 1.000000, iterations 3, largest energy residual 0.000000
max_iterations 1, failing on nonconvergence: eng::fluid::steady: coupled hydraulic Newton solve did not converge
head_tolerance 1e-3: iterations 2
head_tolerance 1e-6: iterations 3
head_tolerance 1e-12: iterations 4
```
### Example 3

Fixed and pressure-driven demand.

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

// a reservoir at 50 m feeding one junction through a 1 km pipe
static fluid::Net one_pipe(double demand = 0.02) {
  fluid::Net net;
  net.nodes.kind = {fluid::NodeKind::Reservoir, fluid::NodeKind::Junction};
  net.nodes.elevation = vec({0.0, 5.0});
  net.nodes.demand = vec({0.0, demand});
  net.nodes.head = vec({50.0, 49.0});
  net.pipes.from = {0};
  net.pipes.to = {1};
  net.pipes.length = vec({1000.0});
  net.pipes.diameter = vec({0.30});
  net.pipes.roughness = vec({1.0e-4});
  net.pipes.minor = vec({0.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  fluid::Options pressure_driven;
  pressure_driven.demand_mode = fluid::DemandMode::Pressure;
  pressure_driven.min_pressure = 0.0;
  pressure_driven.full_pressure = 15.0;
  pressure_driven.demand_exponent = 0.5;
  for (double demand : {0.2, 0.6, 1.0}) {
    const auto fixed = fluid::steady(fluid::network(one_pipe(demand)));
    const auto driven = fluid::steady(fluid::network(one_pipe(demand)), pressure_driven);
    out.writeln("requested ", demand, ": Fixed delivers ", flat(fixed.demand)[1], " at pressure head ", flat(fixed.head)[1] - 5.0, "; Pressure delivers ", flat(driven.demand)[1], " at pressure head ", flat(driven.head)[1] - 5.0);
  }
  // an emitter: a leak that grows with the square root of the pressure head
  for (double c : {0.0, 0.002, 0.01}) {
    auto net = one_pipe(0.02);
    net.nodes.emitter = vec({0.0, c});
    const auto r = fluid::steady(fluid::network(net));
    const double p = flat(r.head)[1] - 5.0;
    out.writeln("emitter ", c, ": outflow ", flat(r.emitter)[1], ", C sqrt(p) = ", c * std::sqrt(p), ", pipe flow ", flat(r.flow)[0]);
  }
}
```

Output

```text
requested 0.200000: Fixed delivers 0.200000 at pressure head 23.011199; Pressure delivers 0.200000 at pressure head 23.011199
requested 0.600000: Fixed delivers 0.600000 at pressure head -146.321537; Pressure delivers 0.277544 at pressure head 3.209615
requested 1.000000: Fixed delivers 1.000000 at pressure head -482.190058; Pressure delivers 0.284220 at pressure head 1.211716
emitter 0.000000: outflow 0.000000, C sqrt(p) = 0.000000, pipe flow 0.020000
emitter 0.002000: outflow 0.013310, C sqrt(p) = 0.013310, pipe flow 0.033310
emitter 0.010000: outflow 0.063962, C sqrt(p) = 0.063962, pipe flow 0.083962
```

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

steady/LossModel
steady/DemandMode
steady/Options
steady/SteadyResult
steady/steady
```
<!-- apiref:toctree:end -->
