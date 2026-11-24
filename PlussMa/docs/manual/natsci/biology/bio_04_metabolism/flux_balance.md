# Flux balance

[Back to Biology 4: Metabolism](../bio_04_metabolism.md)

A cell converts nutrients into biomass through a network of reactions. Flux balance analysis asks what the network can do without knowing any rate laws, only which metabolites each reaction consumes and produces and how fast each reaction can run. This page builds a small network with a bypass, finds its best biomass flux, reads how much each flux can vary at that optimum, and knocks out each reaction in turn.

## How it works

A `metabolism::Network` holds the stoichiometric matrix `S`, with a row per metabolite and a column per reaction, and the lower and upper bound of every flux. The cell is at steady state, so `S v = 0` for the flux vector `v`: every metabolite is produced as fast as it is consumed. `flux_balance(net, objective)` maximises the linear objective, here the biomass flux, under that constraint and the bounds. `flux_variability` solves the range of each flux among all solutions that reach the optimum, and `single_knockouts` forces each reaction to zero in turn; all of the knockouts are solved as one batch.

## A network with a bypass

Metabolite `A` is taken up (at most `10`) and reaches `B` either directly (at most `6`) or through `C` (at most `8`); `B` becomes biomass. The program prints the optimum and its fluxes, the lowest and highest value of each flux among the optimal solutions, and the biomass left when each reaction is removed.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace metabolism = biology::metabolism;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // metabolites A, B, C; reactions: 0 uptake -> A, 1 A -> B, 2 A -> C, 3 C -> B, 4 B -> biomass
  metabolism::Network net;
  net.stoichiometry = mat({{1.0, -1.0, -1.0, 0.0, 0.0},    // A
                           {0.0, 1.0, 0.0, 1.0, -1.0},     // B
                           {0.0, 0.0, 1.0, -1.0, 0.0}});   // C
  net.lower = vec({0.0, 0.0, 0.0, 0.0, 0.0});
  net.upper = vec({10.0, 6.0, 100.0, 8.0, 100.0});         // A -> B is limited to 6, C -> B to 8
  const auto biomass = vec({0.0, 0.0, 0.0, 0.0, 1.0});

  const auto best = metabolism::flux_balance(net, biomass);
  out.writeln("maximum biomass flux = ", flat(best.objective));
  out.writeln("fluxes (uptake, A->B, A->C, C->B, biomass) = ", flat(best.flux));

  const auto range = metabolism::flux_variability(net, biomass);
  out.writeln("lowest flux of each reaction at the optimum  = ", flat(range.minimum));
  out.writeln("highest flux of each reaction at the optimum = ", flat(range.maximum));

  const auto ko = metabolism::single_knockouts(net, biomass);
  out.writeln("biomass as each reaction is knocked out      = ", flat(ko.objective));
  out.writeln("fraction of the wild-type biomass            = ", flat(ko.ratio));
}
```

Output

```text
maximum biomass flux = [10.0000]
fluxes (uptake, A->B, A->C, C->B, biomass) = [10.0000, 6.0000, 4.0000, 4.0000, 10.0000]
lowest flux of each reaction at the optimum  = [10.0000, 2.0000, 4.0000, 4.0000, 10.0000]
highest flux of each reaction at the optimum = [10.0000, 6.0000, 8.0000, 8.0000, 10.0000]
biomass as each reaction is knocked out      = [0.0000, 8.0000, 6.0000, 6.0000, 0.0000]
fraction of the wild-type biomass            = [0.0000, 0.8000, 0.6000, 0.6000, 0.0000]
```

The best biomass flux is `10`, the uptake limit, shared as `6` on the direct step and `4` through `C`. That split is not unique: the variability shows the direct step can carry anything from `2` to `6` and the route through `C` from `4` to `8`, while uptake and biomass are pinned at `10`. The knockouts show what each reaction is worth: removing the direct step leaves `8` (`0.8` of the optimum, carried entirely by the bypass), removing either step of the bypass leaves `6`, and removing uptake or biomass stops growth. Both routes are useful but neither is essential.

<!-- addex:start -->
## A sweep over the uptake

A whole family of constraint changes can be solved at once. `metabolism::sweep_flux(net, objective, index, values)` solves the problem once for each value, with the bounds of reaction `index` replaced by that value, and returns the objective and a solver status for each. The program sweeps the uptake of the network above over seven values.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace metabolism = biology::metabolism;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // metabolites A, B, C; reactions 0 uptake -> A, 1 A -> B, 2 A -> C, 3 C -> B, 4 B -> biomass
  biology::metabolism::Network net;
  net.stoichiometry = mat({{1.0, -1.0, -1.0, 0.0, 0.0}, {0.0, 1.0, 0.0, 1.0, -1.0}, {0.0, 0.0, 1.0, -1.0, 0.0}});
  net.lower = vec({0.0, 0.0, 0.0, 0.0, 0.0});
  net.upper = vec({10.0, 6.0, 100.0, 8.0, 100.0});
  const auto biomass = vec({0.0, 0.0, 0.0, 0.0, 1.0});
  // how does the best biomass depend on the uptake? the sweep replaces the bounds of reaction 0 by each value in one batched solve
  const auto uptake = vec({0.0, 4.0, 8.0, 12.0, 14.0, 16.0, 20.0});
  const auto sweep = metabolism::sweep_flux(net, biomass, 0, uptake);
  out.writeln("uptake limit      ", flat(uptake));
  out.writeln("best biomass      ", flat(sweep.objective));
  out.writeln("solver status     ", flat(sweep.status.to(pluss::ScalarType::Float64)), "   (0 = optimal, anything else = no solution)");
}
```

Output

```text
uptake limit      [0.0000, 4.0000, 8.0000, 12.0000, 14.0000, 16.0000, 20.0000]
best biomass      [0.0000, 4.0000, 8.0000, 12.0000, 14.0000, 0.0000, 0.0000]
solver status     [0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 1.0000, 1.0000]   (0 = optimal, anything else = no solution)
```

The best biomass rises one for one with the uptake, `4`, `8`, `12` and `14` for uptakes of `4`, `8`, `12` and `14`. The sweep replaces the bound of the reaction, so it can exceed the original limit of `10`. Above `14` the status is `1` and no solution exists: the two routes from `A` to `B` can carry at most `6 + 8 = 14`, and with the uptake fixed at `16` or `20` the metabolites cannot be balanced.

## What limits growth

The solution also records the dual of the problem. `shadow_price` gives, for each metabolite, how much the optimum changes per unit change in the steady-state balance of that metabolite. The program prints it, and then loosens, one at a time, the limit of the direct step and the limit of the uptake, to find out which of them is the bottleneck.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace metabolism = biology::metabolism;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  biology::metabolism::Network net;
  net.stoichiometry = mat({{1.0, -1.0, -1.0, 0.0, 0.0}, {0.0, 1.0, 0.0, 1.0, -1.0}, {0.0, 0.0, 1.0, -1.0, 0.0}});
  net.lower = vec({0.0, 0.0, 0.0, 0.0, 0.0});
  net.upper = vec({10.0, 6.0, 100.0, 8.0, 100.0});
  const auto biomass = vec({0.0, 0.0, 0.0, 0.0, 1.0});
  // the dual of the problem: how much would one more unit of each metabolite be worth?
  const auto best = metabolism::flux_balance(net, biomass);
  out.writeln("best biomass                    = ", flat(best.objective));
  out.writeln("shadow prices of A, B, C        = ", flat(best.shadow_price));
  // loosening the direct step A -> B from 6 to 7 raises the optimum only if that step is the bottleneck
  auto looser = net;
  looser.upper = vec({10.0, 7.0, 100.0, 8.0, 100.0});
  out.writeln("best biomass with A -> B up to 7 = ", flat(metabolism::flux_balance(looser, biomass).objective));
  auto more = net;
  more.upper = vec({12.0, 6.0, 100.0, 8.0, 100.0});
  out.writeln("best biomass with uptake up to 12 = ", flat(metabolism::flux_balance(more, biomass).objective));
}
```

Output

```text
best biomass                    = [10.0000]
shadow prices of A, B, C        = [-1.0000, -1.0000, -1.0000]
best biomass with A -> B up to 7 = [10.0000]
best biomass with uptake up to 12 = [12.0000]
```

The optimum is `10`, and the shadow prices of the three metabolites are all `-1.0000`, which says that each unit of material is worth one unit of biomass because every unit that enters ends up in the single biomass reaction. Raising the limit of the direct step from `6` to `7` changes nothing (`10.0000`), since the bypass can carry the difference; raising the uptake limit from `10` to `12` raises the optimum to `12.0000`. The uptake is the bottleneck.

## Suboptimal growth

A cell need not run at its optimum, and `flux_variability(net, objective, fraction)` takes the fraction of the optimal objective that the solutions must still reach. The program calls it at `100%`, `90%` and `50%` of the optimum.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace metabolism = biology::metabolism;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  biology::metabolism::Network net;
  net.stoichiometry = mat({{1.0, -1.0, -1.0, 0.0, 0.0}, {0.0, 1.0, 0.0, 1.0, -1.0}, {0.0, 0.0, 1.0, -1.0, 0.0}});
  net.lower = vec({0.0, 0.0, 0.0, 0.0, 0.0});
  net.upper = vec({10.0, 6.0, 100.0, 8.0, 100.0});
  const auto biomass = vec({0.0, 0.0, 0.0, 0.0, 1.0});
  // flux variability allowing the cell to run below its optimum: at 100%, 90% and 50% of the best biomass
  for (double fraction : {1.0, 0.9, 0.5}) {
    const auto range = metabolism::flux_variability(net, biomass, fraction);
    out.writeln("at ", fraction * 100.0, "% of the optimum: lowest = ", flat(range.minimum), ", highest = ", flat(range.maximum));
  }
}
```

Output

```text
at 100.0000% of the optimum: lowest = [10.0000, 2.0000, 4.0000, 4.0000, 10.0000], highest = [10.0000, 6.0000, 8.0000, 8.0000, 10.0000]
at 90.0000% of the optimum: lowest = [9.0000, 1.0000, 3.0000, 3.0000, 9.0000], highest = [10.0000, 6.0000, 8.0000, 8.0000, 10.0000]
at 50.0000% of the optimum: lowest = [5.0000, 0.0000, 0.0000, 0.0000, 5.0000], highest = [10.0000, 6.0000, 8.0000, 8.0000, 10.0000]
```

At the full optimum the direct step must carry between `2` and `6`. If the cell may drop to `90%` of the best biomass, the range widens to `1` to `6` and the uptake may fall to `9`; at `50%` the ranges include zero for all three internal reactions, `0` to `6` for the direct step and `0` to `8` for the bypass, because half of the biomass can be made by either route alone.

<!-- addex:end -->

## Decisions

- Use `flux_balance` for the optimum, `flux_variability` before you read anything into a single flux, and `single_knockouts` to rank reactions.
- Use `sweep_flux` for a family of bound changes instead of rebuilding the network in a loop.
- Check the sign convention of every column of `S` once, with a direct reaction whose answer you know.

## Pitfalls

A column of the stoichiometry matrix is signed: positive where the reaction makes a metabolite, negative where it uses it. A sign error does not raise anything; it changes the problem. The program writes the uptake of the page's network once as a source of `A` and once as a sink.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace metabolism = biology::metabolism;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a reaction written backwards: uptake that removes A instead of adding it leaves nothing for growth
  for (double sign : {1.0, -1.0}) {
    metabolism::Network net;
    net.stoichiometry = mat({{sign, -1.0, -1.0, 0.0, 0.0}, {0.0, 1.0, 0.0, 1.0, -1.0}, {0.0, 0.0, 1.0, -1.0, 0.0}});
    net.lower = vec({0.0, 0.0, 0.0, 0.0, 0.0});
    net.upper = vec({10.0, 6.0, 100.0, 8.0, 100.0});
    const auto best = metabolism::flux_balance(net, vec({0.0, 0.0, 0.0, 0.0, 1.0}));
    out.writeln("uptake coefficient ", sign, ": biomass ", flat(best.objective), ", fluxes ", flat(best.flux));
  }
}
```

Output

```text
uptake coefficient 1.0000: biomass [10.0000], fluxes [10.0000, 6.0000, 4.0000, 4.0000, 10.0000]
uptake coefficient -1.0000: biomass [0.0000], fluxes [0.0000, 0.0000, 0.0000, 0.0000, 0.0000]
```

With the correct sign the best biomass is `10.0000`, with the fluxes of the page. With the sign reversed the uptake would have to consume `A`, which no reaction supplies, so the only steady state is the empty one: biomass `0.0000` and every flux zero. A network that grows nothing at all usually has a wrongly signed or missing reaction, not a biological reason.

## Where next

[Biology 20: Metabolic Control](../bio_20_control.md) asks how the optimum responds to changes in the bounds, and [Mass-action networks](../bio_25_networks/mass_action_networks.md) follows the same reactions with rate laws.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
