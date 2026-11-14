<!-- generated from economic_dispatch.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Economic dispatch

[Back to Power 3: Dispatch and Optimal Power Flow](../power_03_dispatch_and_opf.md)

A network has several generators with different costs, a demand to meet, and lines that can carry only so much. Economic dispatch chooses the output of every generator that meets the demand at the least total cost without overloading a line. `power::dispatch` solves that problem for the DC network model as a quadratic program. This page explains the cost model, finds the optimum of a network by hand and checks the solver against it, binds a line rating and generator limits, handles several demand scenarios in one call, and shows how accurate the default solver is and how to tighten it.

## The mental model

Every generator has a cost curve, a quadratic function of its active output `g`, `a g^2 + b g`, with the quadratic coefficient `a` and the linear coefficient `b` in cost per per-unit squared and cost per per-unit. The dispatch minimises the total cost over all generators subject to three kinds of constraint:

- the **power balance**: the generation equals the demand (the DC model has no losses);
- the **generator limits**: each output lies between its lower and upper bound;
- the **line ratings**: the DC flow on every line, a linear function of the generation through the power transfer factors, stays within the rating in both directions.

Without line limits the optimum has a classical form: every generator that is not at a limit runs at the same **incremental cost** `lambda`, where `2 a g + b = lambda`, and `lambda` is the price of one more per-unit of demand. A line rating that binds separates the generators' prices.

`dispatch(grid, cost, line_limit, options)` takes the compiled grid, a `GenCost`, a `Series` of ratings with one entry per line (per unit, the same in both directions), and `DispatchOptions`. A `GenCost` holds, per generator, `lower` and `upper` bounds, and `linear` and `quadratic` coefficients; `constant` and the reactive coefficients are used only by the [optimal power flow](optimal_power_flow.md). It returns a `DispatchResult`:

| Member | Shape | Meaning |
|---|---|---|
| `generation` | `[..., generator]` | the optimal active output of each generator, per unit |
| `angle`, `injection` | `[..., bus]` | the resulting DC angles and net injections |
| `flow` | `[..., line]` | the DC flow on every line |
| `objective` | `[...]` | the total cost, from the quadratic and linear terms |
| `post_flow`, `outage_valid` | `[..., outage, line]`, `[..., outage]` | with security on, the flows after each outage; undefined otherwise |
| `converged`, `iterations`, `primal_residual`, `dual_residual` | | the diagnostics of the quadratic-programming solver |

The load is the fixed bus injection of the network; the generators listed in `Gens` are the decision variables, and an entry of `Gens::p` is only a starting value. A generator with `active` false is fixed at zero.

## The optimum by hand

The ring of three buses has generators at buses `0` and `1` and a load of `1.5` per unit at bus `2`. Generator `0` costs `5 g^2 + 10 g` and generator `1` costs `8 g^2 + 12 g`. With no active limit the incremental costs are equal, `10 g0 + 10 = 16 g1 + 12 = lambda`, and `g0 + g1 = 1.5` gives `16 (lambda - 10) + 10 (lambda - 12) = 240`, so `lambda = 20`, `g0 = 1.0` and `g1 = 0.5`. The cost is `5 + 10 + 2 + 6 = 23`. The ratings are loose, `10` per unit on every line. The program solves it with the default solver options and with a tight tolerance.

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

// a tight tolerance for the quadratic-programming solver
static power::DispatchOptions tight() {
  power::DispatchOptions options;
  options.qp.abs_tol = 1e-10;
  options.qp.rel_tol = 1e-10;
  options.qp.max_iters = 50000;
  return options;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8).with_sep(""));
  const power::Grid grid = power::make(ring());
  const auto ratings = vec({10.0, 10.0, 10.0});

  const auto rough = power::dispatch(grid, costs(), ratings);
  out.writeln("default solver: generation ", flat(rough.generation), ", cost ", flat(rough.objective), ", converged ", rough.converged, " in ", rough.iterations, " iterations");
  const auto exact = power::dispatch(grid, costs(), ratings, tight());
  out.writeln("tight solver:   generation ", flat(exact.generation), ", cost ", flat(exact.objective), ", converged ", exact.converged, " in ", exact.iterations, " iterations");
  out.writeln("by hand:        generation [1, 0.5], cost 23, incremental cost 20");
  const auto g = flat(exact.generation);
  out.writeln("incremental costs of the two generators (tight): ", 10.0 * g[0] + 10.0, " ", 16.0 * g[1] + 12.0);
  out.writeln("DC flows (p.u.): ", flat(exact.flow), "   largest balance error: ", std::abs(g[0] + g[1] - 1.5));
}
```

Output

```text
default solver: generation [0.99999744, 0.49999840], cost [22.99991664], converged true in 60 iterations
tight solver:   generation [1.00000000, 0.50000000], cost [23.00000000], converged true in 100 iterations
by hand:        generation [1, 0.5], cost 23, incremental cost 20
incremental costs of the two generators (tight): 20.00000000 20.00000000
DC flows (p.u.): [0.33333333, 0.83333333, 0.66666667]   largest balance error: 0.00000000
```

The tight solver reproduces the hand result, `1.0` and `0.5`, to eight digits, with both incremental costs equal at `20`. The default solver is accurate to about five digits, `0.999997` and `0.499998`, a tolerance of `1e-5` that is generous for planning but visible when the result is compared with an exact value.

## A line rating that binds

Lower the rating of line `2`, the direct line from bus `0` to the load, to `0.6` per unit. The flow on that line is `0.75 - g1 / 6` (the power transfer factors of the ring, from the [previous chapter](../power_02_sensitivities_and_contingencies/sensitivity_factors.md)), so `0.75 - g1 / 6 <= 0.6` forces `g1 >= 0.9` and the cheaper generator must back off: `g0 = 0.6`, `g1 = 0.9`. The cost is `1.8 + 6 + 6.48 + 10.8 = 25.08`, `2.08` more than the unconstrained optimum, which is the price of the rating. The program solves it.

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

// a tight tolerance for the quadratic-programming solver
static power::DispatchOptions tight() {
  power::DispatchOptions options;
  options.qp.abs_tol = 1e-10;
  options.qp.rel_tol = 1e-10;
  options.qp.max_iters = 50000;
  return options;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(7).with_sep(""));
  const power::Grid grid = power::make(ring());
  const auto result = power::dispatch(grid, costs(), vec({10.0, 10.0, 0.6}), tight());
  out.writeln("generation: ", flat(result.generation), "   by hand 0.6, 0.9");
  out.writeln("flow on line 2: ", flat(result.flow)[2], "   rating 0.6");
  out.writeln("cost: ", flat(result.objective), "   by hand 25.08, against 23 without the rating");
  const auto g = flat(result.generation);
  out.writeln("incremental costs: ", 10.0 * g[0] + 10.0, " ", 16.0 * g[1] + 12.0, "   (no longer equal: the rating separates them)");
}
```

Output

```text
generation: [0.6000000, 0.9000000]   by hand 0.6, 0.9
flow on line 2: 0.6000000   rating 0.6
cost: [25.0800000]   by hand 25.08, against 23 without the rating
incremental costs: 16.0000000 26.4000000   (no longer equal: the rating separates them)
```

The flow on line `2` sits exactly at its rating, and the incremental costs differ, `16` for generator `0` against `26.4` for generator `1`: the cheaper generator is held back by the line, so it runs at a lower marginal cost than the dearer one, which must make up the difference.

## Generator limits, out-of-service units and unbounded generators

Three more inputs shape the problem. An upper bound that binds fixes a generator at its limit while the others share the rest. A generator marked `active` false in the network is fixed at zero whatever its bounds. And a bound may be infinite for a generator with no practical limit. The program solves the unconstrained ring with generator `0` capped at `0.8`, with generator `1` out of service, and with both upper bounds infinite. With the cap, `g0 = 0.8` and the other generator supplies `0.7`; with generator `1` out, generator `0` supplies the whole `1.5`.

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

// a tight tolerance for the quadratic-programming solver
static power::DispatchOptions tight() {
  power::DispatchOptions options;
  options.qp.abs_tol = 1e-10;
  options.qp.rel_tol = 1e-10;
  options.qp.max_iters = 50000;
  return options;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const auto ratings = vec({10.0, 10.0, 10.0});

  power::GenCost capped = costs();
  capped.upper = vec({0.8, 2.0});
  out.writeln("g0 limited to 0.8:        ", flat(power::dispatch(power::make(ring()), capped, ratings, tight()).generation));

  power::Net one_off = ring();
  one_off.gens.active = {true, false};
  out.writeln("generator 1 out of service: ", flat(power::dispatch(power::make(one_off), costs(), ratings, tight()).generation));

  power::GenCost unbounded = costs();
  unbounded.upper = vec({std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity()});
  out.writeln("no upper bounds:          ", flat(power::dispatch(power::make(ring()), unbounded, ratings, tight()).generation));

  // a constant cost does not change the decision; the dispatch objective holds the quadratic and linear terms only
  power::GenCost with_constant = costs();
  with_constant.constant = vec({100.0, 50.0});
  const auto k = power::dispatch(power::make(ring()), with_constant, ratings, tight());
  out.writeln("with fixed costs 100 and 50: generation ", flat(k.generation), ", objective ", flat(k.objective));
}
```

Output

```text
g0 limited to 0.8:        [0.800000, 0.700000]
generator 1 out of service: [1.500000, 0.000000]
no upper bounds:          [1.000000, 0.500000]
with fixed costs 100 and 50: generation [1.000000, 0.500000], objective [23.000000]
```

The capped generator runs at its limit, `0.8`, and the other makes up the difference, `0.7`. With generator `1` out of service generator `0` supplies the whole demand, `1.5`, and infinite upper bounds leave the unconstrained optimum, `1.0` and `0.5`. A fixed cost, whether or not the unit is committed, does not change how output is shared among running units, and the objective of the dispatch holds only the quadratic and linear terms: the same `23` is returned with fixed costs of `100` and `50`.

## Many demand scenarios in one call

The loads belong to the network, so a leading axis on the bus injections is a set of demand scenarios, and every scenario is optimised in one call. The program solves demands of `1.0` and `2.0` per unit. For the demand of `1.0` the incremental costs equal `lambda`, `16 (lambda - 10) + 10 (lambda - 12) = 160`, `lambda = 16.923`, `g0 = 0.6923`, `g1 = 0.3077`.

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

// a tight tolerance for the quadratic-programming solver
static power::DispatchOptions tight() {
  power::DispatchOptions options;
  options.qp.abs_tol = 1e-10;
  options.qp.rel_tol = 1e-10;
  options.qp.max_iters = 50000;
  return options;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  power::Net batch = ring();
  batch.buses.p = pluss::series::make(std::vector<double>{0, 0, -1.0, 0, 0, -2.0}).reshape({2, 3});
  batch.buses.q = pluss::series::make(std::vector<double>{0, 0, -0.2, 0, 0, -0.4}).reshape({2, 3});
  const auto result = power::dispatch(power::make(batch), costs(), vec({10.0, 10.0, 10.0}), tight());
  const auto g = flat(result.generation);
  out.writeln("generation shape ", result.generation.sizes(), ": demand 1.0 -> ", g[0], " ", g[1], ";  demand 2.0 -> ", g[2], " ", g[3]);
  out.writeln("costs ", flat(result.objective));
  const double lambda = (440.0) / 26.0;
  out.writeln("by hand for demand 1.0: lambda ", lambda, ", generation ", (lambda - 10.0) / 10.0, " ", (lambda - 12.0) / 16.0);
}
```

Output

```text
generation shape [2, 2]: demand 1.0 -> 0.692308 0.307692;  demand 2.0 -> 1.307692 0.692308
costs [13.769231, 33.769231]
by hand for demand 1.0: lambda 16.923077, generation 0.692308 0.307692
```

Each scenario has its own optimum, `[2, 2]` results in all, and the demand of `1.0` agrees with the hand formula. The cost curves and ratings may also carry the scenario axis, one set per scenario.

## Decisions

- **Accuracy:** the default solver tolerance, `1e-5`, is good for planning; tighten `options.qp.abs_tol` and `rel_tol`, and raise `max_iters`, when you compare with an exact value or want clean prices.
- **Line losses and voltages matter:** dispatch is a DC model, lossless and with flat voltage; for losses and voltage limits use the [optimal power flow](optimal_power_flow.md).
- **Outages must be survived:** `DispatchOptions::secure`, described on the [next page](secure_dispatch.md).

## Pitfalls

Each of the mistakes below is one a first dispatch tends to contain. The program makes four: comparing the default-accuracy result with an exact value for equality, asking for more demand than the generators can supply, giving a concave cost curve, and giving a line rating for the wrong number of lines.

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

// a tight tolerance for the quadratic-programming solver
static power::DispatchOptions tight() {
  power::DispatchOptions options;
  options.qp.abs_tol = 1e-10;
  options.qp.rel_tol = 1e-10;
  options.qp.max_iters = 50000;
  return options;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2).with_sep(""));
  const power::Grid grid = power::make(ring());
  const auto ratings = vec({10.0, 10.0, 10.0});

  // 1. the default accuracy is about five digits
  const auto rough = power::dispatch(grid, costs(), ratings);
  out.writeln("default solver: |g0 - 1| = ", std::abs(flat(rough.generation)[0] - 1.0), ", equal to 1 exactly: ", flat(rough.generation)[0] == 1.0);

  // 2. more demand than the generators can supply: the load is 1.5 and each unit is capped at 0.5
  power::GenCost small = costs();
  small.upper = vec({0.5, 0.5});
  const auto infeasible = power::dispatch(grid, small, ratings);
  out.writeln("demand 1.5, capacity 1.0: converged ", infeasible.converged, ", primal residual ", infeasible.primal_residual);

  // 3. a concave cost curve
  power::GenCost concave = costs();
  concave.quadratic = vec({-1.0, 8.0});
  try {
    (void)power::dispatch(grid, concave, ratings);
  } catch (const std::exception &e) {
    std::cout << "negative quadratic coefficient: " << e.what() << "\n";
  }

  // 4. ratings for the wrong number of lines
  try {
    (void)power::dispatch(grid, costs(), vec({10.0, 10.0}));
  } catch (const std::exception &e) {
    std::cout << "two ratings for three lines: " << e.what() << "\n";
  }
}
```

Output

```text
default solver: |g0 - 1| = 2.56e-06, equal to 1 exactly: false
demand 1.5, capacity 1.0: converged false, primal residual 1.67e-01
negative quadratic coefficient: eng::power::dispatch: quadratic costs must be non-negative
two ratings for three lines: eng::power::dispatch: line limits must be real floating [..., elements]
```

A solution is never exactly `1.0`: compare with a tolerance of at least the solver's, or tighten it. An infeasible dispatch does not throw: it returns with `converged` false and a primal residual that says by how much the constraints are violated, `0.167` per unit here, so check `converged` before using the generation. A concave cost curve would make the problem nonconvex and is refused, and ratings must be given once per line.

Reference: [`dispatch`](../../../../api/eng/power/dispatch/dispatch.md), [`GenCost`](../../../../api/eng/power/dispatch/GenCost.md), [`DispatchOptions`](../../../../api/eng/power/dispatch/DispatchOptions.md), [`DispatchResult`](../../../../api/eng/power/dispatch/DispatchResult.md), [`ptdf`](../../../../api/eng/power/security/ptdf.md).

Where next: [Secure dispatch](secure_dispatch.md) adds the loss of any one line to the constraints.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="secure_dispatch.html">
    <span class="label">Next</span>
    <span class="title">Secure dispatch</span>
  </a>
</div>
<!-- apiref:nav:end -->
