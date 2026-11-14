<!-- generated from opf.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `power::opf`

Exact AC optimal power flow by sequential quadratic programming.

## Signature

```cpp
power::OpfResult power::opf(const power::Grid& grid, const power::GenCost& cost, const power::Limits& limits, const power::OpfOptions& options = {});
```

## What It Does

Minimises the generation cost over the bus voltages and the generator outputs, subject to the complex power balance at every bus, the generator bounds, the voltage bounds and, if given, the line apparent-power ratings. The derivatives are analytic. The slack bus holds its angle at zero; its magnitude is a decision variable bounded by `vm_lower` and `vm_upper`.

## Parameters

`grid`
: The compiled network.

`cost`
: The bounds and cost curves of the generators, including the optional reactive terms.

`limits`
: The reactive, voltage and optional line limits.

`options`
: The iteration budget and tolerances.

## Returns

An `OpfResult`.

## Errors

Throws `std::invalid_argument` for a lower bound above its upper bound or for arrays of the wrong length. An infeasible or unconverged problem does not throw; it returns `converged` false with a message.

## Interpretation and Use

Use it where losses, reactive power or voltages are part of the question, and as the reference that the lossless `dispatch` is checked against. Keep batches modest, since the optimiser is dense.

## Practical Examples

The following programs isolate this symbol's normal contract, a materially different mode or configuration, and relevant validation or interpretation behavior. Every program and displayed output is generated from the same compiled source.

### Example 1

A ring checked against an independent solver.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <limits>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a ring of three buses: generators at buses 0 and 1, a load of 1.5 per unit at bus 2.
// Lines: 0 joins 0-1 (x = 0.1), 1 joins 1-2 (x = 0.2), 2 joins 0-2 (x = 0.3).
static power::Net ring() {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PV, power::BusKind::PQ};
  net.buses.p = vec({0.0, 0.0, -1.5});
  net.buses.q = vec({0.0, 0.0, -0.3});
  net.buses.vm = vec({1.0, 1.0, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0});
  net.gens.bus = {0, 1};
  net.gens.p = vec({0.5, 1.0});    // starting values: the dispatch chooses them
  net.gens.q = vec({0.0, 0.0});
  net.lines.from = {0, 1, 0};
  net.lines.to = {1, 2, 2};
  net.lines.r = vec({0.01, 0.02, 0.03});
  net.lines.x = vec({0.1, 0.2, 0.3});
  return net;
}

// generator 0: cost 5 g^2 + 10 g, generator 1: cost 8 g^2 + 12 g, each between 0 and 2 per unit
static power::GenCost costs() {
  power::GenCost c;
  c.lower = vec({0.0, 0.0});
  c.upper = vec({2.0, 2.0});
  c.linear = vec({10.0, 12.0});
  c.quadratic = vec({5.0, 8.0});
  return c;
}


// generator reactive limits of -1 and 1 per unit, bus voltages between 0.95 and 1.05, and no flow rating
static power::Limits limits() {
  power::Limits l;
  l.q_lower = vec({-1.0, -1.0});
  l.q_upper = vec({1.0, 1.0});
  l.vm_lower = vec({0.95, 0.95, 0.95});
  l.vm_upper = vec({1.05, 1.05, 1.05});
  return l;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const power::Grid grid = power::make(ring());
  const auto r = power::opf(grid, costs(), limits());
  out.writeln("converged ", r.converged, " in ", r.iterations, " iterations: ", r.message);
  const auto p = flat(r.generation_p), q = flat(r.generation_q);
  out.writeln("active output ", p, "   reactive output ", q, "   cost ", flat(r.objective));
  out.writeln("independent solver: active output [1.013894, 0.516653], reactive [0.214211, 0.391257], cost 23.614121");
  out.writeln("voltage magnitudes ", flat(r.magnitude), "   angles ", flat(r.angle));
  out.writeln("price of active power at the buses ", flat(r.p_price), "   price of reactive power ", flat(r.q_price));
  out.writeln("incremental cost of generator 0: ", 10.0 * p[0] + 10.0, "   of generator 1 at bus 1: ", 16.0 * p[1] + 12.0, "   against the prices at buses 0 and 1");
  out.writeln("losses (generation - demand): ", p[0] + p[1] - 1.5, " per unit");
  const auto dc = power::dispatch(grid, costs(), vec({10.0, 10.0, 10.0}));
  out.writeln("lossless DC dispatch: ", flat(dc.generation), ", cost ", flat(dc.objective));
}
```

Output

```text
converged true in 7 iterations: optimality and feasibility tolerances satisfied
active output [1.013894, 0.516653]   reactive output [0.214211, 0.391257]   cost [23.614121]
independent solver: active output [1.013894, 0.516653], reactive [0.214211, 0.391257], cost 23.614121
voltage magnitudes [1.050000, 1.050000, 0.979157]   angles [0.000000, -0.030999, -0.191049]
price of active power at the buses [20.138939, 20.266446, 21.019468]   price of reactive power [-0.000000, -0.000000, 0.312656]
incremental cost of generator 0: 20.138940   of generator 1 at bus 1: 20.266446   against the prices at buses 0 and 1
losses (generation - demand): 0.030547 per unit
lossless DC dispatch: [0.999997, 0.499998], cost [22.999917]
```
### Example 2

Reactive, voltage and rating limits.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <limits>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a ring of three buses: generators at buses 0 and 1, a load of 1.5 per unit at bus 2.
// Lines: 0 joins 0-1 (x = 0.1), 1 joins 1-2 (x = 0.2), 2 joins 0-2 (x = 0.3).
static power::Net ring() {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PV, power::BusKind::PQ};
  net.buses.p = vec({0.0, 0.0, -1.5});
  net.buses.q = vec({0.0, 0.0, -0.3});
  net.buses.vm = vec({1.0, 1.0, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0});
  net.gens.bus = {0, 1};
  net.gens.p = vec({0.5, 1.0});    // starting values: the dispatch chooses them
  net.gens.q = vec({0.0, 0.0});
  net.lines.from = {0, 1, 0};
  net.lines.to = {1, 2, 2};
  net.lines.r = vec({0.01, 0.02, 0.03});
  net.lines.x = vec({0.1, 0.2, 0.3});
  return net;
}

// generator 0: cost 5 g^2 + 10 g, generator 1: cost 8 g^2 + 12 g, each between 0 and 2 per unit
static power::GenCost costs() {
  power::GenCost c;
  c.lower = vec({0.0, 0.0});
  c.upper = vec({2.0, 2.0});
  c.linear = vec({10.0, 12.0});
  c.quadratic = vec({5.0, 8.0});
  return c;
}


// generator reactive limits of -1 and 1 per unit, bus voltages between 0.95 and 1.05, and no flow rating
static power::Limits limits() {
  power::Limits l;
  l.q_lower = vec({-1.0, -1.0});
  l.q_upper = vec({1.0, 1.0});
  l.vm_lower = vec({0.95, 0.95, 0.95});
  l.vm_upper = vec({1.05, 1.05, 1.05});
  return l;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const power::Grid grid = power::make(ring());
  const auto show = [&](const char *label, const power::Limits &l) {
    const auto r = power::opf(grid, costs(), l);
    out.writeln(label, ": converged ", r.converged, ", active ", flat(r.generation_p), ", reactive ", flat(r.generation_q), ", cost ", flat(r.objective));
  };
  show("no limit binds          ", limits());
  for (double cap : {0.4, 0.35, 0.3}) {
    power::Limits l = limits();
    l.q_upper = vec({cap, cap});
    out.write("reactive output <= ", cap, "   ");
    show("", l);
  }
  for (double top : {1.04, 1.03}) {
    power::Limits l = limits();
    l.vm_upper = vec({top, top, top});
    out.write("voltages <= ", top, "         ");
    show("", l);
  }
  power::Limits rated = limits();
  rated.flow = vec({10.0, 10.0, 0.7});
  const auto r = power::opf(grid, costs(), rated);
  out.writeln("line 2 rated 0.7: converged ", r.converged, ", active ", flat(r.generation_p), ", cost ", flat(r.objective), ", apparent power at the sending ends ", flat(r.from_power.abs()), ", multipliers ", flat(r.from_shadow));
  out.writeln("independent solver: caps 0.35 -> [1.013759, 0.516903] cost 23.616473; 0.3 -> [0.855355, 0.674645] cost 23.948618; vm 1.04 -> cost 23.628361; 1.03 -> 23.643160; rating -> cost 23.635596");
}
```

Output

```text
no limit binds          : converged true, active [1.013894, 0.516653], reactive [0.214211, 0.391257], cost [23.614121]
reactive output <= 0.400000   : converged true, active [1.013894, 0.516653], reactive [0.214212, 0.391257], cost [23.614121]
reactive output <= 0.350000   : converged true, active [1.013759, 0.516903], reactive [0.256623, 0.349997], cost [23.616473]
reactive output <= 0.300000   : converged true, active [0.855355, 0.674645], reactive [0.300000, 0.300000], cost [23.948618]
voltages <= 1.040000         : converged true, active [1.014226, 0.517026], reactive [0.217266, 0.395249], cost [23.628361]
voltages <= 1.030000         : converged true, active [1.014572, 0.517412], reactive [0.220439, 0.399398], cost [23.643160]
line 2 rated 0.7: converged true, active [1.003282, 0.528177], cost [23.635598], apparent power at the sending ends [0.390835, 0.946695, 0.699999], multipliers [0.000000, 0.000000, 1.297702]
independent solver: caps 0.35 -> [1.013759, 0.516903] cost 23.616473; 0.3 -> [0.855355, 0.674645] cost 23.948618; vm 1.04 -> cost 23.628361; 1.03 -> 23.643160; rating -> cost 23.635596
```
### Example 3

Tolerances and iteration budget.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <limits>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a ring of three buses: generators at buses 0 and 1, a load of 1.5 per unit at bus 2.
// Lines: 0 joins 0-1 (x = 0.1), 1 joins 1-2 (x = 0.2), 2 joins 0-2 (x = 0.3).
static power::Net ring() {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PV, power::BusKind::PQ};
  net.buses.p = vec({0.0, 0.0, -1.5});
  net.buses.q = vec({0.0, 0.0, -0.3});
  net.buses.vm = vec({1.0, 1.0, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0});
  net.gens.bus = {0, 1};
  net.gens.p = vec({0.5, 1.0});    // starting values: the dispatch chooses them
  net.gens.q = vec({0.0, 0.0});
  net.lines.from = {0, 1, 0};
  net.lines.to = {1, 2, 2};
  net.lines.r = vec({0.01, 0.02, 0.03});
  net.lines.x = vec({0.1, 0.2, 0.3});
  return net;
}

// generator 0: cost 5 g^2 + 10 g, generator 1: cost 8 g^2 + 12 g, each between 0 and 2 per unit
static power::GenCost costs() {
  power::GenCost c;
  c.lower = vec({0.0, 0.0});
  c.upper = vec({2.0, 2.0});
  c.linear = vec({10.0, 12.0});
  c.quadratic = vec({5.0, 8.0});
  return c;
}


// generator reactive limits of -1 and 1 per unit, bus voltages between 0.95 and 1.05, and no flow rating
static power::Limits limits() {
  power::Limits l;
  l.q_lower = vec({-1.0, -1.0});
  l.q_upper = vec({1.0, 1.0});
  l.vm_lower = vec({0.95, 0.95, 0.95});
  l.vm_upper = vec({1.05, 1.05, 1.05});
  return l;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2).with_sep(""));
  const power::Grid grid = power::make(ring());
  struct Run { const char *name; power::OpfOptions options; };
  power::OpfOptions loose;
  loose.constraint_tolerance = 1e-3;
  loose.optimality_tolerance = 1e-3;
  power::OpfOptions short_budget;
  short_budget.max_iterations = 3;
  for (const Run &run : {Run{"defaults        ", power::OpfOptions{}}, Run{"tolerances 1e-3 ", loose}, Run{"max_iterations 3", short_budget}}) {
    const auto r = power::opf(grid, costs(), limits(), run.options);
    out.writeln(run.name, ": converged ", r.converged, ", iterations ", r.iterations, ", violation ", r.constraint_violation, ", optimality ", r.first_order_optimality, ", first generator ", flat(r.generation_p)[0]);
    if (!r.converged) std::cout << "                  message: " << r.message << "\n";
  }
}
```

Output

```text
defaults        : converged true, iterations 7, violation 5.22e-12, optimality 3.19e-07, first generator 1.01e+00
tolerances 1e-3 : converged true, iterations 5, violation 7.23e-08, optimality 1.01e-02, first generator 1.01e+00
max_iterations 3: converged false, iterations 3, violation 1.95e-04, optimality 2.98e-02, first generator 1.01e+00
                  message: maximum iterations reached diagnostic=iteration-limit measure=2.985e-02 reference=1.000e+00.
```
### Example 4

Infeasible limits and refused inputs.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <limits>

using namespace pluss::tils;
namespace power = pluss::subsp::eng::power;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// a ring of three buses: generators at buses 0 and 1, a load of 1.5 per unit at bus 2.
// Lines: 0 joins 0-1 (x = 0.1), 1 joins 1-2 (x = 0.2), 2 joins 0-2 (x = 0.3).
static power::Net ring() {
  power::Net net;
  net.base = {100e6, 230e3};
  net.buses.kind = {power::BusKind::Slack, power::BusKind::PV, power::BusKind::PQ};
  net.buses.p = vec({0.0, 0.0, -1.5});
  net.buses.q = vec({0.0, 0.0, -0.3});
  net.buses.vm = vec({1.0, 1.0, 1.0});
  net.buses.va = vec({0.0, 0.0, 0.0});
  net.gens.bus = {0, 1};
  net.gens.p = vec({0.5, 1.0});    // starting values: the dispatch chooses them
  net.gens.q = vec({0.0, 0.0});
  net.lines.from = {0, 1, 0};
  net.lines.to = {1, 2, 2};
  net.lines.r = vec({0.01, 0.02, 0.03});
  net.lines.x = vec({0.1, 0.2, 0.3});
  return net;
}

// generator 0: cost 5 g^2 + 10 g, generator 1: cost 8 g^2 + 12 g, each between 0 and 2 per unit
static power::GenCost costs() {
  power::GenCost c;
  c.lower = vec({0.0, 0.0});
  c.upper = vec({2.0, 2.0});
  c.linear = vec({10.0, 12.0});
  c.quadratic = vec({5.0, 8.0});
  return c;
}


// generator reactive limits of -1 and 1 per unit, bus voltages between 0.95 and 1.05, and no flow rating
static power::Limits limits() {
  power::Limits l;
  l.q_lower = vec({-1.0, -1.0});
  l.q_upper = vec({1.0, 1.0});
  l.vm_lower = vec({0.95, 0.95, 0.95});
  l.vm_upper = vec({1.05, 1.05, 1.05});
  return l;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const power::Grid grid = power::make(ring());

  // 1. voltage limits the network cannot meet: no point satisfies them
  power::Limits tight_voltage = limits();
  tight_voltage.vm_lower = vec({0.98, 0.98, 0.98});
  const auto a = power::opf(grid, costs(), tight_voltage);
  out.writeln("voltages >= 0.98: converged ", a.converged, ", violation ", a.constraint_violation, ", cost reported ", flat(a.objective), " (not a result)");
  std::cout << "  message: " << a.message << "\n";

  // 2. generator capacity of 1.0 for a demand of 1.5
  power::GenCost small = costs();
  small.upper = vec({0.5, 0.5});
  const auto b = power::opf(grid, small, limits());
  out.writeln("capacity 1.0 for demand 1.5: converged ", b.converged, ", violation ", b.constraint_violation);

  // 3. a lower voltage bound above the upper bound
  power::Limits crossed = limits();
  crossed.vm_lower = vec({1.1, 1.1, 1.1});
  try {
    (void)power::opf(grid, costs(), crossed);
  } catch (const std::exception &e) {
    std::cout << "lower bound above the upper bound: " << e.what() << "\n";
  }
}
```

Output

```text
voltages >= 0.98: converged false, violation 0.0054, cost reported [25.7340] (not a result)
  message: maximum iterations reached diagnostic=iteration-limit measure=7.602e+01 reference=3.052e-05.
capacity 1.0 for demand 1.5: converged false, violation 0.5294
lower bound above the upper bound: eng::power::opf: every lower bound must not exceed its upper bound
```

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="OpfResult.html">
    <span class="label">Previous</span>
    <span class="title">OpfResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
