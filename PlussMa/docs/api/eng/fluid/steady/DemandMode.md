<!-- generated from DemandMode.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `fluid::DemandMode`

Whether a demand is fixed or falls with the pressure.

## Signature

```cpp
fluid::DemandMode
```

## What It Does

Chooses how the demand at a junction behaves. `Fixed` delivers the whole demand whatever the pressure. `Pressure` delivers it by the Wagner law: nothing at or below `min_pressure`, the whole demand at or above `full_pressure`, and a power law between.

## Fields

| Field | Meaning | Default |
|---|---|---|
| `Fixed` | The delivered demand is the requested demand. Pressures can fall below zero; they are reported without complaint. |  |
| `Pressure` | `d(p) = D ((p - p_min) / (p_full - p_min))^γ` between the pressures, with the exponent `γ = Options::demand_exponent`. A source (negative demand) is never throttled. |  |

## Returns

An enumeration; `Options::demand_mode`.

## Errors

`steady` rejects pressure bounds that are not ordered and exponents that are not positive.

## Interpretation and Use

Use `Fixed` while the pressures are decent. Use `Pressure` to see what a stressed network delivers. Check `SteadyResult::pressure` after a `Fixed` solve of a stressed network.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

Fixed against Pressure on a starved junction.

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
### Example 2

Thresholds and exponents.

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
  for (double minimum : {0.0, 10.0}) {
    for (double exponent : {0.5, 1.0}) {
      fluid::Options options;
      options.demand_mode = fluid::DemandMode::Pressure;
      options.min_pressure = minimum;
      options.full_pressure = minimum + 15.0;
      options.demand_exponent = exponent;
      const auto r = fluid::steady(fluid::network(one_pipe(0.6)), options);
      out.writeln("min ", minimum, " m, full ", minimum + 15.0, " m, exponent ", exponent, ": pressure head ", flat(r.head)[1] - 5.0, ", delivered ", flat(r.demand)[1]);
    }
  }
  fluid::Options options;
  options.demand_mode = fluid::DemandMode::Pressure;
  const auto source = fluid::steady(fluid::network(one_pipe(-0.05)), options);
  out.writeln("a source of 0.05 m3/s is delivered whole: ", flat(source.demand)[1]);
}
```

Output

```text
min 0.000000 m, full 15.000000 m, exponent 0.500000: pressure head 3.209615, delivered 0.277544
min 0.000000 m, full 15.000000 m, exponent 1.000000: pressure head 6.642288, delivered 0.265692
min 10.000000 m, full 25.000000 m, exponent 0.500000: pressure head 12.485326, delivered 0.244229
min 10.000000 m, full 25.000000 m, exponent 1.000000: pressure head 15.781659, delivered 0.231266
a source of 0.05 m3/s is delivered whole: -0.050000
```
### Example 3

Bounds that are refused.

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
  attempt("Hazen-Williams with C = 0", [] {
    auto net = one_pipe();
    net.pipes.roughness = vec({0.0});
    fluid::Options options;
    options.loss = fluid::LossModel::HazenWilliams;
    (void)fluid::steady(fluid::network(net), options);
  });
  attempt("full pressure below the minimum", [] {
    fluid::Options options;
    options.demand_mode = fluid::DemandMode::Pressure;
    options.min_pressure = 20.0;
    options.full_pressure = 10.0;
    (void)fluid::steady(fluid::network(one_pipe()), options);
  });
  attempt("a demand exponent of zero", [] {
    fluid::Options options;
    options.demand_exponent = 0.0;
    (void)fluid::steady(fluid::network(one_pipe()), options);
  });
  attempt("a head tolerance of zero", [] {
    fluid::Options options;
    options.head_tolerance = 0.0;
    (void)fluid::steady(fluid::network(one_pipe()), options);
  });
  attempt("an iteration limit of zero", [] {
    fluid::Options options;
    options.max_iterations = 0;
    (void)fluid::steady(fluid::network(one_pipe()), options);
  });
}
```

Output

```text
Hazen-Williams with C = 0: eng::fluid::steady: Hazen-Williams coefficients must be positive
full pressure below the minimum: eng::fluid::steady: pressure bounds, exponents, tolerances, gradient floor, and iteration limit are invalid
a demand exponent of zero: eng::fluid::steady: pressure bounds, exponents, tolerances, gradient floor, and iteration limit are invalid
a head tolerance of zero: eng::fluid::steady: pressure bounds, exponents, tolerances, gradient floor, and iteration limit are invalid
an iteration limit of zero: eng::fluid::steady: pressure bounds, exponents, tolerances, gradient floor, and iteration limit are invalid
```

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="LossModel.html">
    <span class="label">Previous</span>
    <span class="title">LossModel</span>
  </a>
  <a class="next-link" href="Options.html">
    <span class="label">Next</span>
    <span class="title">Options</span>
  </a>
</div>
<!-- apiref:nav:end -->
