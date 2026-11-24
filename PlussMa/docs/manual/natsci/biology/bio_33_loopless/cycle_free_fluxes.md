# Removing thermodynamic loops

[Back to Biology 33: Loopless Flux Analysis](../bio_33_loopless.md)

A flux solution can contain a cycle of reactions that runs round and round at no cost, such as $A\to B\to C\to A$. Such a loop satisfies the steady state but cannot happen in a cell, because no cycle of reactions can run without an energy input. These loops inflate flux ranges and spoil comparisons between flux distributions. This page finds the boundary reactions, removes loops from a given flux vector, and computes flux ranges that leave the loops out.

## How it works

A reaction is a *boundary* reaction when it touches exactly one metabolite (an uptake, an export, a demand): it exchanges matter with the outside. Every other reaction is internal. A loop is a flux through internal reactions alone that satisfies $S v = 0$, so that it can be added to any flux vector without changing the boundary fluxes or the objective. `metabolism::boundary_reactions(net)` marks the boundary reactions. `metabolism::loopless_solution(net, objective, fluxes)` removes the loops from one flux vector, or from a batch `[K, r]`, by the CycleFreeFlux method: it keeps the boundary fluxes and the objective, restricts every internal reaction to the sign of its flux and to a magnitude at most its flux, and minimises the total internal flux. `metabolism::loopless_flux_variability(net, objective, fraction)` returns the same `minimum`, `maximum` and `optimum` as `flux_variability`, but with every range computed over loop-free fluxes. `metabolism::loopless_range_exact(net, objective, reaction, fraction)` is the exact range of one reaction from the mixed-integer loop-law problem, which gives a slower, certain answer to check the fast method against. The programs use a network with a cycle through three internal reactions, `A -> B`, `B -> C` and `C -> A`, with uptake into A and biomass made from B.

## A loop widens the ranges

The program marks the boundary reactions of the looped network, solves for the maximum biomass, and then asks for the range of every flux at the optimum.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace metabolism = biology::metabolism;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
// metabolites A, B, C; reactions: 0 uptake -> A, 1 A -> B, 2 A -> C, 3 C -> B, 4 B -> biomass
static metabolism::Network bypass() {
  metabolism::Network net;
  net.stoichiometry = mat({{1.0, -1.0, -1.0, 0.0, 0.0}, {0.0, 1.0, 0.0, 1.0, -1.0}, {0.0, 0.0, 1.0, -1.0, 0.0}});
  net.lower = vec({0.0, 0.0, 0.0, 0.0, 0.0});
  net.upper = vec({10.0, 6.0, 100.0, 8.0, 100.0});
  return net;
}
// metabolites A, B, C; reactions: 0 uptake -> A, 1 A -> B, 2 B -> C, 3 C -> A, 4 B -> biomass. Reactions 1, 2, 3 form a cycle A -> B -> C -> A
static metabolism::Network looped() {
  metabolism::Network net;
  net.stoichiometry = mat({{1.0, -1.0, 0.0, 1.0, 0.0}, {0.0, 1.0, -1.0, 0.0, -1.0}, {0.0, 0.0, 1.0, -1.0, 0.0}});
  net.lower = vec({0.0, 0.0, 0.0, 0.0, 0.0});
  net.upper = vec({10.0, 1000.0, 1000.0, 1000.0, 1000.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto net = looped();
  // boundary reactions exchange matter with the outside: exactly one metabolite is touched
  out.writeln("boundary reactions (1 = boundary) ", flat(metabolism::boundary_reactions(net)));
  // the objective is the biomass flux. The best flux is 10, but the cycle can carry any amount of extra flux without costing anything
  const auto biomass = vec({0.0, 0.0, 0.0, 0.0, 1.0});
  const auto best = metabolism::flux_balance(net, biomass);
  out.writeln("optimal biomass ", flat(best.objective));
  const auto range = metabolism::flux_variability(net, biomass);
  out.writeln("lowest flux of each reaction  ", flat(range.minimum));
  out.writeln("highest flux of each reaction ", flat(range.maximum));
}
```

Output

```text
boundary reactions (1 = boundary) [1.0000, 0.0000, 0.0000, 0.0000, 1.0000]
optimal biomass [10.0000]
lowest flux of each reaction  [10.0000, 10.0000, 0.0000, 0.0000, 10.0000]
highest flux of each reaction [10.0000, 1000.0000, 990.0000, 990.0000, 10.0000]
```

The boundary reactions are the uptake and the biomass (`1` in the first and last entry). The maximum biomass is `10`. At that optimum the three cycle reactions can take any value from `0` up to `990` or `1000`: the flux from A to B ranges from `10` to `1000`, and the other two from `0` to `990`, because a loop adds flux around the cycle at no cost. The ranges are the bounds of the problem, not information about the network.

## Removing a loop from a flux vector

The program makes a flux vector in which `7` units circulate round the cycle while `10` units go from uptake to biomass, cleans it, and then cleans a batch of three vectors in one call.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace metabolism = biology::metabolism;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
// metabolites A, B, C; reactions: 0 uptake -> A, 1 A -> B, 2 A -> C, 3 C -> B, 4 B -> biomass
static metabolism::Network bypass() {
  metabolism::Network net;
  net.stoichiometry = mat({{1.0, -1.0, -1.0, 0.0, 0.0}, {0.0, 1.0, 0.0, 1.0, -1.0}, {0.0, 0.0, 1.0, -1.0, 0.0}});
  net.lower = vec({0.0, 0.0, 0.0, 0.0, 0.0});
  net.upper = vec({10.0, 6.0, 100.0, 8.0, 100.0});
  return net;
}
// metabolites A, B, C; reactions: 0 uptake -> A, 1 A -> B, 2 B -> C, 3 C -> A, 4 B -> biomass. Reactions 1, 2, 3 form a cycle A -> B -> C -> A
static metabolism::Network looped() {
  metabolism::Network net;
  net.stoichiometry = mat({{1.0, -1.0, 0.0, 1.0, 0.0}, {0.0, 1.0, -1.0, 0.0, -1.0}, {0.0, 0.0, 1.0, -1.0, 0.0}});
  net.lower = vec({0.0, 0.0, 0.0, 0.0, 0.0});
  net.upper = vec({10.0, 1000.0, 1000.0, 1000.0, 1000.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto net = looped();
  const auto biomass = vec({0.0, 0.0, 0.0, 0.0, 1.0});
  // a flux vector with a loop: 10 units go from uptake to biomass, and 7 units circulate through A -> B -> C -> A
  const auto looping = vec({10.0, 17.0, 7.0, 7.0, 10.0});
  out.writeln("steady state S v (should be 0) ", flat(net.stoichiometry.matmul(looping.unsqueeze(-1))));
  const auto clean = metabolism::loopless_solution(net, biomass, looping);
  out.writeln("flux vector with a loop     ", flat(looping));
  out.writeln("after removing the loop     ", flat(clean));
  // a batch: three flux vectors cleaned in one call
  const auto many = pluss::series::make({{10.0, 17.0, 7.0, 7.0, 10.0}, {10.0, 12.0, 2.0, 2.0, 10.0}, {10.0, 10.0, 0.0, 0.0, 10.0}}).to(pluss::ScalarType::Float64);
  const auto cleaned = metabolism::loopless_solution(net, biomass, many);
  out.writeln("three vectors, cycle flux before ", flat(many.select(1, 2)));
  out.writeln("three vectors, cycle flux after  ", flat(cleaned.select(1, 2)));
}
```

Output

```text
steady state S v (should be 0) [0.0000, 0.0000, 0.0000]
flux vector with a loop     [10.0000, 17.0000, 7.0000, 7.0000, 10.0000]
after removing the loop     [10.0000, 10.0000, 0.0000, 0.0000, 10.0000]
three vectors, cycle flux before [7.0000, 2.0000, 0.0000]
three vectors, cycle flux after  [0.0000, 0.0000, 0.0000]
```

The input is a steady state (all entries of $S v$ are `0.0000`). After cleaning, the cycle reactions carry `0`, `(10, 10, 0, 0, 10)`: the `10` units go straight from uptake to biomass and the boundary fluxes are untouched. In the batch of three vectors the cycle flux `7`, `2`, `0` all becomes `0`; a vector without a loop passes through unchanged.

## Loopless ranges

The program computes the ordinary and the loopless flux ranges at the optimum and at `90%` of it.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace metabolism = biology::metabolism;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
// metabolites A, B, C; reactions: 0 uptake -> A, 1 A -> B, 2 A -> C, 3 C -> B, 4 B -> biomass
static metabolism::Network bypass() {
  metabolism::Network net;
  net.stoichiometry = mat({{1.0, -1.0, -1.0, 0.0, 0.0}, {0.0, 1.0, 0.0, 1.0, -1.0}, {0.0, 0.0, 1.0, -1.0, 0.0}});
  net.lower = vec({0.0, 0.0, 0.0, 0.0, 0.0});
  net.upper = vec({10.0, 6.0, 100.0, 8.0, 100.0});
  return net;
}
// metabolites A, B, C; reactions: 0 uptake -> A, 1 A -> B, 2 B -> C, 3 C -> A, 4 B -> biomass. Reactions 1, 2, 3 form a cycle A -> B -> C -> A
static metabolism::Network looped() {
  metabolism::Network net;
  net.stoichiometry = mat({{1.0, -1.0, 0.0, 1.0, 0.0}, {0.0, 1.0, -1.0, 0.0, -1.0}, {0.0, 0.0, 1.0, -1.0, 0.0}});
  net.lower = vec({0.0, 0.0, 0.0, 0.0, 0.0});
  net.upper = vec({10.0, 1000.0, 1000.0, 1000.0, 1000.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto net = looped();
  const auto biomass = vec({0.0, 0.0, 0.0, 0.0, 1.0});
  const auto plain = metabolism::flux_variability(net, biomass);
  const auto clean = metabolism::loopless_flux_variability(net, biomass);
  out.writeln("ordinary range, lowest   ", flat(plain.minimum));
  out.writeln("ordinary range, highest  ", flat(plain.maximum));
  out.writeln("loopless range, lowest   ", flat(clean.minimum));
  out.writeln("loopless range, highest  ", flat(clean.maximum));
  out.writeln("optimum                  ", flat(clean.optimum));
  // allow the cell to give up some biomass: 90% of the optimum is enough
  const auto relaxed = metabolism::loopless_flux_variability(net, biomass, 0.9);
  out.writeln("loopless, 90% of optimum, lowest  ", flat(relaxed.minimum));
  out.writeln("loopless, 90% of optimum, highest ", flat(relaxed.maximum));
}
```

Output

```text
ordinary range, lowest   [10.0000, 10.0000, 0.0000, 0.0000, 10.0000]
ordinary range, highest  [10.0000, 1000.0000, 990.0000, 990.0000, 10.0000]
loopless range, lowest   [10.0000, 10.0000, 0.0000, 0.0000, 10.0000]
loopless range, highest  [10.0000, 10.0000, 0.0000, 0.0000, 10.0000]
optimum                  [10.0000]
loopless, 90% of optimum, lowest  [9.0000, 9.0000, 0.0000, 0.0000, 9.0000]
loopless, 90% of optimum, highest [10.0000, 10.0000, 0.0000, 0.0000, 10.0000]
```

The ordinary range of the cycle reactions is `0` to `990` or `1000`, while the loopless range is the single point `(10, 10, 0, 0, 10)`: the cell has no use for the cycle. At `90%` of the optimum the boundary fluxes may fall to `9` and the loopless ranges remain narrow, `9` to `10` for the uptake path and `0` for the loop reactions, because a fraction of the objective does not create a cycle.

## The exact range for comparison

The program computes the exact loopless range of the reactions `1`, `2` and `4` from the loop-law mixed-integer problem and prints the fast method's result beside it.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace metabolism = biology::metabolism;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
// metabolites A, B, C; reactions: 0 uptake -> A, 1 A -> B, 2 A -> C, 3 C -> B, 4 B -> biomass
static metabolism::Network bypass() {
  metabolism::Network net;
  net.stoichiometry = mat({{1.0, -1.0, -1.0, 0.0, 0.0}, {0.0, 1.0, 0.0, 1.0, -1.0}, {0.0, 0.0, 1.0, -1.0, 0.0}});
  net.lower = vec({0.0, 0.0, 0.0, 0.0, 0.0});
  net.upper = vec({10.0, 6.0, 100.0, 8.0, 100.0});
  return net;
}
// metabolites A, B, C; reactions: 0 uptake -> A, 1 A -> B, 2 B -> C, 3 C -> A, 4 B -> biomass. Reactions 1, 2, 3 form a cycle A -> B -> C -> A
static metabolism::Network looped() {
  metabolism::Network net;
  net.stoichiometry = mat({{1.0, -1.0, 0.0, 1.0, 0.0}, {0.0, 1.0, -1.0, 0.0, -1.0}, {0.0, 0.0, 1.0, -1.0, 0.0}});
  net.lower = vec({0.0, 0.0, 0.0, 0.0, 0.0});
  net.upper = vec({10.0, 1000.0, 1000.0, 1000.0, 1000.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto net = looped();
  const auto biomass = vec({0.0, 0.0, 0.0, 0.0, 1.0});
  // the exact loopless range of one reaction by the loop-law mixed-integer problem: the slow, certain reference
  for (std::int64_t reaction : {1, 2, 4}) {
    const auto range = metabolism::loopless_range_exact(net, biomass, reaction);
    out.writeln("reaction ", std::vector<double>{static_cast<double>(reaction)}, " loopless range ", std::vector<double>{range.first, range.second});
  }
  const auto fast = metabolism::loopless_flux_variability(net, biomass);
  out.writeln("loopless_flux_variability for reactions 1, 2, 4 ", std::vector<double>{flat(fast.minimum)[1], flat(fast.maximum)[1], flat(fast.minimum)[2], flat(fast.maximum)[2], flat(fast.minimum)[4], flat(fast.maximum)[4]});
}
```

Output

```text
reaction [1.0000] loopless range [10.0000, 10.0000]
reaction [2.0000] loopless range [0.0000, 0.0000]
reaction [4.0000] loopless range [10.0000, 10.0000]
loopless_flux_variability for reactions 1, 2, 4 [10.0000, 10.0000, 0.0000, 0.0000, 10.0000, 10.0000]
```

The exact ranges `(10, 10)`, `(0, 0)` and `(10, 10)` for reactions `1`, `2` and `4` match what the fast method gives, with the zero end reported as `0.0000`. The exact method solves a mixed-integer problem per reaction end, which is why the fast method is the one to use for a whole network.

## Decisions

- Use `loopless_solution` on every flux distribution you intend to compare or to report, whether it comes from flux balance, a knockout or sampling.
- Use `loopless_flux_variability` instead of `flux_variability` for ranges you want to interpret biologically: it excludes the loops, whose size is set by the arbitrary bounds.
- Use `loopless_range_exact` only to check a few reactions: it solves a mixed-integer problem and is slow at scale.
- Identify boundary reactions with `boundary_reactions` and check that the exchange reactions of your network are really single-metabolite reactions; a boundary reaction that touches two metabolites would be treated as internal.

## Pitfalls

On a network that has no cycle, loopless analysis changes nothing, and an infeasible request is refused. The program shows both.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace metabolism = biology::metabolism;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
// metabolites A, B, C; reactions: 0 uptake -> A, 1 A -> B, 2 A -> C, 3 C -> B, 4 B -> biomass
static metabolism::Network bypass() {
  metabolism::Network net;
  net.stoichiometry = mat({{1.0, -1.0, -1.0, 0.0, 0.0}, {0.0, 1.0, 0.0, 1.0, -1.0}, {0.0, 0.0, 1.0, -1.0, 0.0}});
  net.lower = vec({0.0, 0.0, 0.0, 0.0, 0.0});
  net.upper = vec({10.0, 6.0, 100.0, 8.0, 100.0});
  return net;
}
// metabolites A, B, C; reactions: 0 uptake -> A, 1 A -> B, 2 B -> C, 3 C -> A, 4 B -> biomass. Reactions 1, 2, 3 form a cycle A -> B -> C -> A
static metabolism::Network looped() {
  metabolism::Network net;
  net.stoichiometry = mat({{1.0, -1.0, 0.0, 1.0, 0.0}, {0.0, 1.0, -1.0, 0.0, -1.0}, {0.0, 0.0, 1.0, -1.0, 0.0}});
  net.lower = vec({0.0, 0.0, 0.0, 0.0, 0.0});
  net.upper = vec({10.0, 1000.0, 1000.0, 1000.0, 1000.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a network without a cycle: the bypass network. Loopless analysis changes nothing, and the exact range agrees
  const auto net = bypass();
  const auto biomass = vec({0.0, 0.0, 0.0, 0.0, 1.0});
  const auto plain = metabolism::flux_variability(net, biomass);
  const auto clean = metabolism::loopless_flux_variability(net, biomass);
  out.writeln("ordinary lowest ", flat(plain.minimum));
  out.writeln("loopless lowest ", flat(clean.minimum));
  out.writeln("ordinary highest ", flat(plain.maximum));
  out.writeln("loopless highest ", flat(clean.maximum));
  // a network whose reference optimum cannot be found is refused
  auto bad = looped();
  bad.lower = vec({0.0, 0.0, 0.0, 0.0, 20.0});
  try {
    metabolism::loopless_flux_variability(bad, biomass);
  } catch (const pluss::Error &e) {
    out.writeln("infeasible biomass floor: ", e.message());
  }
}
```

Output

```text
ordinary lowest [10.0000, 2.0000, 4.0000, 4.0000, 10.0000]
loopless lowest [10.0000, 2.0000, 4.0000, 4.0000, 10.0000]
ordinary highest [10.0000, 6.0000, 8.0000, 8.0000, 10.0000]
loopless highest [10.0000, 6.0000, 8.0000, 8.0000, 10.0000]
infeasible biomass floor: natsci: loopless_flux_variability: reference optimum not found
```

The ordinary and loopless ranges of the bypass network are identical in every entry, so it is safe to use the loopless call as a default. When the biomass is required to be at least `20` but the uptake allows `10`, the reference optimum cannot be found and the call is refused with a message that says so.

## Where next

Go back to [Biology 32: The Flux Space](../bio_32_flux_space.md) for sampling that these loops distort, and to [Biology 4: Metabolism](../bio_04_metabolism.md) for flux balance and variability.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
