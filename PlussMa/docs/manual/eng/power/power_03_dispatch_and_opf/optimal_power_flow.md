<!-- generated from optimal_power_flow.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Optimal power flow

[Back to Power 3: Dispatch and Optimal Power Flow](../power_03_dispatch_and_opf.md)

Economic dispatch works on the lossless DC model. Real networks have losses, reactive power, and voltage limits that shape the dispatch: a generator far from the load wastes power in the lines, and a load bus that sags needs reactive support. The **AC optimal power flow** chooses the active and reactive output of every generator and every bus voltage at least cost, subject to the exact AC network equations and to limits on generator reactive power, bus voltages and line loading. `power::opf` solves it. This page explains the problem, solves a network and checks the answer against an independent solver and against the incremental cost, shows each kind of limit binding, reads the prices, and controls the optimiser.

## The mental model

The unknowns are the voltage angle and magnitude of every bus and the active and reactive output of every generator. The constraints are the complex power balance at every bus, as in the [AC power flow](../power_01_networks_and_power_flow/ac_power_flow.md), plus the limits: generator active output between `lower` and `upper` (the same `GenCost` as the DC dispatch), generator reactive output between `q_lower` and `q_upper`, bus voltage magnitude between `vm_lower` and `vm_upper`, and, if given, the apparent power at each end of every line below `flow`. The objective is the total generation cost, `a g^2 + b g + c` for the active output, and optionally a polynomial in the reactive output.

`opf(grid, cost, limits, options)` solves it by a sequential quadratic-programming method with analytic derivatives, and returns an `OpfResult`:

| Member | Shape | Meaning |
|---|---|---|
| `generation_p`, `generation_q` | `[..., generator]` | the optimal active and reactive output, per unit |
| `voltage`, `magnitude`, `angle` | `[..., bus]` | the optimal bus voltages |
| `injection` | `[..., bus]` | the complex net injection at every bus |
| `from_power`, `to_power` | `[..., line]` | the complex power at the two ends of each line |
| `objective` | `[...]` | the optimal cost, one per scenario |
| `p_price`, `q_price` | `[..., bus]` | the multipliers of the active and reactive power balances: the cost of one more per-unit of demand at the bus |
| `from_shadow`, `to_shadow` | `[..., line]` | the multipliers of the line-rating constraints, nonzero only for a binding rating |
| `converged`, `iterations`, `constraint_violation`, `first_order_optimality`, `message` | | the diagnostics of the optimiser |

`Limits::flow` may be left undefined to switch the rating off; the other limits are required. The network's slack bus holds its voltage angle at zero, and its `Buses::vm` set point is only a starting guess: the voltage magnitudes are decision variables.

## A network, checked two ways

The ring of three buses has generators at buses `0` and `1` (cost `5 g^2 + 10 g` and `8 g^2 + 12 g`, between `0` and `2` per unit) and a load of `1.5 + 0.3j` at bus `2`, with generator reactive output between `-1` and `1`, and voltages between `0.95` and `1.05`. The solution is checked two ways. The first is an independent solver: the same optimisation, written out with the network equations from the admittance matrix and solved by a separate general-purpose constrained optimiser, gives `g0 = 1.013894`, `g1 = 0.516653` and a cost of `23.614121`. The second is the optimality condition: at an unconstrained generator the incremental cost `10 g + 10` equals the price of active power at its bus, `p_price`. The program prints both, and the lossless DC dispatch of the previous page for comparison.

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

The generation, voltages and cost agree with the independent solver to the printed digits, and the incremental costs of the two generators, `20.1389` and `20.2664`, are the prices at their buses: at an optimum every unconstrained generator runs where its marginal cost equals the price of power where it sits. The price rises toward the load, `21.0195` at bus `2`, because delivering power there also costs losses. The AC solution generates `0.0305` per unit more than the demand to cover the losses, so the cost is `23.61` against `23.0` for the lossless dispatch.

## The limits

Three limits can bind, and each has its own effect. A **reactive limit** caps what a generator can supply: when the cheap solution needs more reactive power than a generator can give, the dispatch moves active power onto the other, and the voltage profile changes. A **voltage limit** bounds how far a bus may sag or rise. And a **line rating** bounds the apparent power at either end of a line. The program tightens each in turn on the same network and prints what changes, with the independent solver's results beside it: generator reactive output capped at `0.4`, `0.35` and `0.3`, bus voltage upper limit `1.04` and `1.03`, and an apparent-power rating of `0.7` on line `2`.

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

A reactive cap of `0.4` does not bind, since the unconstrained solution needs at most `0.391`, and the answer is unchanged. At `0.35` the second generator reaches its cap and the dispatch shifts a little active power toward it, `0.5169` instead of `0.5167`, at a cost of `23.6165`. At `0.3` both generators are at their caps, `0.3` each, and the active output moves a lot, `0.855` and `0.675`, raising the cost to `23.9486`. Lowering the voltage ceiling costs a little more at each step. The line rating of `0.7` holds the apparent power on line `2` at `0.699999` and prices it: its multiplier is `1.2977`, the cost of tightening that rating by one per unit. Every cost agrees with the independent solver to better than `1e-5`.

## Controlling the optimiser

`OpfOptions` holds `max_iterations` (default `160`), the `constraint_tolerance` on the violation of the network equations and limits (`1e-8`), and the `optimality_tolerance` on the first-order optimality (`1e-8`). The program solves the network with the defaults, with loose tolerances of `1e-3`, and with an iteration budget of `3`, and prints the iterations, the diagnostics and the generation.

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

Loose tolerances stop after `5` iterations with the first generator still at `1.01` to three digits, against `7` iterations and six digits for the defaults. A budget of three stops before the iteration has converged, and the result says so, with a message naming the measure that was still too large.

## Decisions

- **DC dispatch or AC OPF:** the dispatch for many scenarios, security constraints and market studies, where losses and voltages are not the question; the OPF when losses, reactive power or voltages are, and for the base case that the dispatch is checked against.
- **Reading prices:** `p_price` at a bus is the cost of one more per-unit of load there, and the differences between buses are losses and congestion.
- **Many scenarios:** leading axes on the injections or the costs, solved together in one call; the optimiser is dense, so keep the batches modest.

## Pitfalls

An OPF can have no feasible point, and then no answer exists. The program makes three mistakes: voltage limits no generator can meet from this network (a lower bound of `0.98` at every bus, with the upper bound `1.05`), generator capacity below the demand, and a lower bound above its upper bound.

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

An infeasible problem does not throw: the optimiser stops at the point that violates the constraints least, reports `converged` false and the size of the violation, and the objective it returns is the cost of that point, not an optimum. A bound pair that contradicts itself is refused up front.

Reference: [`opf`](../../../../api/eng/power/opf/opf.md), [`Limits`](../../../../api/eng/power/opf/Limits.md), [`OpfOptions`](../../../../api/eng/power/opf/OpfOptions.md), [`OpfResult`](../../../../api/eng/power/opf/OpfResult.md), [`GenCost`](../../../../api/eng/power/dispatch/GenCost.md).

Where next: the next chapter turns the AC model around: estimating the voltages from noisy measurements, and following a network through increasing load to the point of voltage collapse.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="secure_dispatch.html">
    <span class="label">Previous</span>
    <span class="title">Secure dispatch</span>
  </a>
</div>
<!-- apiref:nav:end -->
