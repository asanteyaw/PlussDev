# Mass-action networks

[Back to Biology 25: Reaction Networks](../bio_25_networks.md)

You have a set of reactions with rate constants and you want to know how the concentrations change, where they settle, and how that depends on the constants. This page describes a reaction network as two matrices, evaluates its rates for one state or for many, finds its steady state under conservation laws, and follows the approach to it in time.

## How it works

A network has one row for each reaction and one column for each species. The stoichiometry matrix {math}`N` holds the net change of each species when the reaction happens once, and the order matrix {math}`O` holds the power of each concentration in the rate law. With rate constants {math}`k_r`, the flux of reaction {math}`r` and the rate of change of the concentrations are

```{math}
\begin{aligned}
&v_r = k_r\prod_s x_s^{O_{rs}}, \\
&\frac{\mathrm{d}x}{\mathrm{d}t} = N^{\top} v
\end{aligned}
```

`reaction::make(N, O)` builds the network, `reaction::flux(net, x, k)` and `reaction::rate(net, x, k)` evaluate {math}`v` and {math}`\mathrm{d}x/\mathrm{d}t`, and `reaction::jacobian(net, x, k)` the derivative of the rates. The state and the rate constants may have leading batch axes, so one call evaluates a whole ensemble. A steady state solves {math}`\mathrm{d}x/\mathrm{d}t = 0` together with the conservation laws that reaction networks always have (the total amount of enzyme, for example); `reaction::steady_state(net, k, x0, L, T)` takes the conservation matrix {math}`L` and the totals {math}`T` and finds {math}`x` with {math}`Lx = T`, starting from a positive guess {math}`x_0`. All programs below use one network, an enzyme {math}`E` that converts a substrate {math}`S` into a product {math}`P` through a complex {math}`C`, with substrate supplied at a constant rate and product removed in proportion to its amount.

## Building a network and evaluating its rates

The program builds the five reactions of the enzyme network, evaluates the fluxes and the rates of change at one state, and then evaluates the substrate rate at three substrate levels in a single call.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace reaction = natsci::common::reaction;
namespace dynamics = natsci::common::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// species (E, S, C, P). Reactions: 1 E + S -> C, 2 C -> E + S, 3 C -> E + P, 4 -> S (supply), 5 P -> (removal)
static reaction::Network enzyme_network() {
  const auto stoichiometry = mat({{-1, -1, 1, 0}, {1, 1, -1, 0}, {1, 0, -1, 1}, {0, 1, 0, 0}, {0, 0, 0, -1}});
  const auto orders = mat({{1, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 1}});
  return reaction::make(stoichiometry, orders);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto network = enzyme_network();
  const auto k = vec({10.0, 5.0, 5.0, 2.0, 1.0});       // rate constants of the five reactions
  const auto x = vec({1.0, 0.5, 0.0, 0.0});             // enzyme 1, substrate 0.5, no complex, no product
  out.writeln("species = ", network.species(), ", reactions = ", network.reactions());
  out.writeln("reaction fluxes v = ", flat(reaction::flux(network, x, k)));
  out.writeln("rates of change dx/dt = ", flat(reaction::rate(network, x, k)));
  // many states at once: the same network, three different substrate levels in one call
  const auto states = mat({{1.0, 0.1, 0.0, 0.0}, {1.0, 0.5, 0.0, 0.0}, {1.0, 2.0, 0.0, 0.0}});
  out.writeln("dS/dt for substrate 0.1, 0.5, 2 = ", flat(reaction::rate(network, states, k).select(-1, 1)));
}
```

Output

```text
species = 4, reactions = 5
reaction fluxes v = [5.0000, 0.0000, 0.0000, 2.0000, 0.0000]
rates of change dx/dt = [-5.0000, -3.0000, 5.0000, 0.0000]
dS/dt for substrate 0.1, 0.5, 2 = [1.0000, -3.0000, -18.0000]
```

There are `4` species and `5` reactions. With an empty complex and no product only two reactions run: binding, with flux `10 x 1 x 0.5 = 5.0000`, and the substrate supply, `2.0000`. The rates of change follow from the stoichiometry: the enzyme and the substrate lose `5` to binding, the substrate gains `2` from the supply (net `-3.0000`) and the complex gains `5.0000`. The three substrate levels give `1.0000`, `-3.0000` and `-18.0000`: a batch of states is one call.

## The steady state under a conservation law

The enzyme is neither made nor destroyed, so {math}`E + C = 1`. The program passes this law and finds the steady state from a positive guess, checks that the flux through the enzyme equals the supply, and then solves three different supply rates at once.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace reaction = natsci::common::reaction;
namespace dynamics = natsci::common::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// species (E, S, C, P). Reactions: 1 E + S -> C, 2 C -> E + S, 3 C -> E + P, 4 -> S (supply), 5 P -> (removal)
static reaction::Network enzyme_network() {
  const auto stoichiometry = mat({{-1, -1, 1, 0}, {1, 1, -1, 0}, {1, 0, -1, 1}, {0, 1, 0, 0}, {0, 0, 0, -1}});
  const auto orders = mat({{1, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 1}});
  return reaction::make(stoichiometry, orders);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto network = enzyme_network();
  const auto k = vec({10.0, 5.0, 5.0, 2.0, 1.0});
  // the enzyme is conserved: E + C = 1. The steady state solves rate(x) = 0 together with that law
  const auto conservation = mat({{1.0, 0.0, 1.0, 0.0}});
  const auto totals = vec({1.0});
  const auto steady = reaction::steady_state(network, k, vec({1.0, 1.0, 0.1, 1.0}), conservation, totals);
  out.writeln("steady state (E, S, C, P) = ", flat(steady.state));
  out.writeln("converged = ", steady.converged ? "yes" : "no", " after ", steady.iterations, " iterations, largest rate ", flat(steady.residual)[0]);
  const auto s = flat(steady.state);
  out.writeln("flux through the enzyme (kcat C) = ", 5.0 * s[2], ", product removal (k5 P) = ", 1.0 * s[3], ", substrate supply = 2");
  // several parameter sets at once: three supply rates, three steady states from one call (the enzyme can pass at most kcat * 1 = 5 per unit time)
  const auto supply = vec({0.5, 2.0, 4.0});
  const auto ks = vec({10.0, 5.0, 5.0, 0.5, 1.0, 10.0, 5.0, 5.0, 2.0, 1.0, 10.0, 5.0, 5.0, 4.0, 1.0}).reshape({3, 5});
  const auto starts = vec({1.0, 1.0, 0.1, 1.0, 1.0, 1.0, 0.1, 1.0, 1.0, 1.0, 0.1, 1.0}).reshape({3, 4});
  out.writeln("substrate supply                       ", flat(supply));
  out.writeln("steady substrate level, three supplies ", flat(reaction::steady_state(network, ks, starts, conservation, totals).state.select(-1, 1)));
}
```

Output

```text
steady state (E, S, C, P) = [0.6000, 0.6667, 0.4000, 2.0000]
converged = yes after 3 iterations, largest rate 0.0000
flux through the enzyme (kcat C) = 2.0000, product removal (k5 P) = 2.0000, substrate supply = 2
substrate supply                       [0.5000, 2.0000, 4.0000]
steady substrate level, three supplies [0.1111, 0.6667, 4.0000]
```

The network settles at {math}`E = 0.6`, {math}`S = 0.6667`, {math}`C = 0.4` and {math}`P = 2.0` in `3` iterations, with the largest remaining rate `0.0000`. At the steady state everything balances: the enzyme processes `5 x 0.4 = 2.0000` per unit time, the product is removed at `2.0000` and the supply is `2`. The three supplies give substrate levels `0.1111`, `0.6667` and `4.0000`, which follow the Michaelis-Menten law {math}`S = K_m v/(V_{max} - v)` with {math}`K_m = 1` and {math}`V_{max} = 5` and grow without bound as the supply approaches the capacity of the enzyme.

## The approach to the steady state

The steady state is where the system ends, not how it gets there. The program integrates the rates from an empty cell and prints the state at four times, and prints the Jacobian of the rates at the steady state.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace reaction = natsci::common::reaction;
namespace dynamics = natsci::common::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// species (E, S, C, P). Reactions: 1 E + S -> C, 2 C -> E + S, 3 C -> E + P, 4 -> S (supply), 5 P -> (removal)
static reaction::Network enzyme_network() {
  const auto stoichiometry = mat({{-1, -1, 1, 0}, {1, 1, -1, 0}, {1, 0, -1, 1}, {0, 1, 0, 0}, {0, 0, 0, -1}});
  const auto orders = mat({{1, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 1}});
  return reaction::make(stoichiometry, orders);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto network = enzyme_network();
  const auto k = vec({10.0, 5.0, 5.0, 2.0, 1.0});
  // the approach to the steady state, from an empty cell
  const auto rate = [&](const pluss::Series &x) { return reaction::rate(network, x, k); };
  for (double t : {0.5, 2.0, 8.0, 30.0}) out.writeln("time ", t, ": (E, S, C, P) = ", flat(dynamics::simulate(rate, vec({1.0, 0.0, 0.0, 0.0}), 0.0, t, 4000)));
  // the Jacobian of the rates at the steady state decides stability: all its eigenvalues must have negative real part
  const auto steady = reaction::steady_state(network, k, vec({1.0, 1.0, 0.1, 1.0}), mat({{1.0, 0.0, 1.0, 0.0}}), vec({1.0}));
  out.writeln("Jacobian at the steady state (4 x 4) = ", flat(reaction::jacobian(network, steady.state, k)));
}
```

Output

```text
time 0.5000: (E, S, C, P) = [0.7423, 0.3852, 0.2577, 0.3036]
time 2.0000: (E, S, C, P) = [0.6127, 0.6352, 0.3873, 1.4744]
time 8.0000: (E, S, C, P) = [0.6000, 0.6667, 0.4000, 1.9984]
time 30.0000: (E, S, C, P) = [0.6000, 0.6667, 0.4000, 2.0000]
Jacobian at the steady state (4 x 4) = [-6.6667, -6.0000, 10.0000, 0.0000, -6.6667, -6.0000, 5.0000, 0.0000, 6.6667, 6.0000, -10.0000, 0.0000, 0.0000, 0.0000, 5.0000, -1.0000]
```

From an empty cell the enzyme is quickly taken up by the substrate (the complex rises to `0.2577` by time `0.5`), the product accumulates (`1.4744` at time `2.0`), and by time `30` the state is the steady state `(0.6000, 0.6667, 0.4000, 2.0000)` found directly. The last row of the Jacobian, `0.0000, 0.0000, 5.0000, -1.0000`, reads off the product balance: it is made from the complex at rate `5` and lost at rate `1`.

## Decisions

- To find where a network settles, call `steady_state` with the conservation laws: it is much faster than integrating until nothing changes, and it tells you whether a steady state exists. Integrate with `dynamics::simulate` only when the path to the steady state matters.
- Pass every conservation law as a row of {math}`L` with its total in {math}`T`. A network with a conserved total and no conservation row has a singular Jacobian at its steady state.
- Put the parameter sets you want to compare on a leading axis of `k` and of the starting guess, and make one call instead of a loop.

## Pitfalls

A malformed network, a starting guess with a zero concentration, or a supply that no steady state can match are the usual stopping points. The program tries all three.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace reaction = natsci::common::reaction;
namespace dynamics = natsci::common::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// species (E, S, C, P). Reactions: 1 E + S -> C, 2 C -> E + S, 3 C -> E + P, 4 -> S (supply), 5 P -> (removal)
static reaction::Network enzyme_network() {
  const auto stoichiometry = mat({{-1, -1, 1, 0}, {1, 1, -1, 0}, {1, 0, -1, 1}, {0, 1, 0, 0}, {0, 0, 0, -1}});
  const auto orders = mat({{1, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 1}});
  return reaction::make(stoichiometry, orders);
}

int main() {
  OutputEngine out;
  const auto stoichiometry = mat({{-1, -1, 1, 0}, {1, 1, -1, 0}});
  // the stoichiometry and the order matrix must have the same shape: one row per reaction, one column per species
  try {
    (void)reaction::make(stoichiometry, mat({{1, 1, 0, 0}}));
  } catch (const pluss::Error &e) {
    out.writeln("orders with a missing row: ", e.message());
  }
  // a rate constant must be given for every reaction, and the steady-state search starts from a positive state
  try {
    (void)reaction::steady_state(enzyme_network(), vec({10.0, 5.0, 5.0, 2.0, 1.0}), vec({1.0, 0.0, 0.1, 1.0}), mat({{1.0, 0.0, 1.0, 0.0}}), vec({1.0}));
  } catch (const pluss::Error &e) {
    out.writeln("a zero concentration in the starting guess: ", e.message());
  }
  // a supply of 8 is more than the enzyme can ever process (at most kcat x total enzyme = 5): there is no steady state, and the solver says so
  const auto overloaded = reaction::steady_state(enzyme_network(), vec({10.0, 5.0, 5.0, 8.0, 1.0}), vec({1.0, 1.0, 0.1, 1.0}), mat({{1.0, 0.0, 1.0, 0.0}}), vec({1.0}), 1e-12, 200);
  out.writeln("supply 8, capacity 5: converged = ", overloaded.converged ? "yes" : "no");
}
```

Output

```text
orders with a missing row:  natsci: network: stoichiometry and orders must be [R, S]
a zero concentration in the starting guess:  natsci: steady_state: initial guess must be strictly positive
supply 8, capacity 5: converged =  no
```

The first two are rejected with an error that names the function and the rule. The third is not an error: with a supply of `8` and an enzyme that can process at most `5`, the substrate piles up for ever and no steady state exists, so the solver stops with `converged = no`. Always test `converged` before using `state`.

## Where next

[Biology 5: Enzymes](../bio_05_enzymes.md) gives the rate laws that such a network sums up to, and [Biology 20: Metabolic Control](../bio_20_control.md) asks how the steady flux of a network responds to its rate constants.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
