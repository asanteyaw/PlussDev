# Assembly and chemostats

[Back to Biology 29: Community Stability and Assembly](../bio_29_communities.md)

You have a pool of species with known growth rates and known interactions, and you want to know which of them end up together: who can enter a community, what the community looks like once it has, and in a chemostat how a single consumer holds a nutrient down. This page computes the equilibrium of any chosen subset of species, builds a community by successive invasion, and follows one consumer on one nutrient.

## How it works

In the generalised Lotka-Volterra model species $i$ grows as $\dot n_i = n_i\,(r_i + \sum_j A_{ij}n_j)$. For a set $R$ of resident species the equilibrium solves $A_{RR}\,n_R = -r_R$, and an absent species $j$ can invade when its growth rate while rare,

```{math}
\lambda_j = r_j + \sum_{i\in R} A_{ji}\,n_i^{*},
```

is positive. `community::resident_equilibrium(r, a, resident)` returns the equilibrium of the residents (zero for absent species), with `resident` a mask of ones and zeros, and `community::invasion_growth_rate(r, a, resident)` returns $\lambda$ for every species (it is zero for the residents themselves). For one consumer on a nutrient in a chemostat with dilution $D$, supply $S_0$, Monod uptake $\mu(R) = \mu_{max}R/(K+R)$, mortality $m$ and yield $y$, the nutrient settles at $R^* = mK/(\mu_{max} - m)$, the lowest level the species can live on, and the consumer at

```{math}
N^{*} = \frac{y\,D\,(S_0 - R^{*})}{m}.
```

`community::monod`, `community::r_star` and `community::single_species_density` compute these, and `community::chemostat_rate` is the rate of the nutrient and the consumer, to integrate with `dynamics::simulate`.

## Equilibria of chosen subsets

The program defines four species and prints the equilibrium of the community of species `1` and `2`, the growth rates of all four species in it, and the equilibria of the communities `{1}` and `{1,2,3}`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace community = biology::population::community;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // three resident species and one outsider; A_ij is the effect of species j on the growth of species i
  const auto r = vec({1.0, 0.8, 0.6, 0.5});
  const auto a = pluss::series::make({{-1.0, -0.4, -0.2, -0.3}, {-0.3, -1.0, -0.3, -0.2}, {-0.2, -0.5, -1.0, -0.4}, {-0.6, -0.6, -0.5, -1.0}}).to(pluss::ScalarType::Float64);
  // equilibrium of the community that consists of species 1 and 2 only: the entries of absent species stay zero
  const auto both = vec({1.0, 1.0, 0.0, 0.0});
  out.writeln("equilibrium of {1,2}          ", flat(community::resident_equilibrium(r, a, both)));
  out.writeln("growth rates of all four there ", flat(community::invasion_growth_rate(r, a, both)));
  // the same for {1} alone and for {1,2,3}
  out.writeln("equilibrium of {1}            ", flat(community::resident_equilibrium(r, a, vec({1.0, 0.0, 0.0, 0.0}))));
  out.writeln("equilibrium of {1,2,3}        ", flat(community::resident_equilibrium(r, a, vec({1.0, 1.0, 1.0, 0.0}))));
  out.writeln("growth rate of species 4 in {1,2,3} ", std::vector<double>{flat(community::invasion_growth_rate(r, a, vec({1.0, 1.0, 1.0, 0.0})))[3]});
}
```

Output

```text
equilibrium of {1,2}          [0.7727, 0.5682, 0.0000, 0.0000]
growth rates of all four there [0.0000, 0.0000, 0.1614, -0.3045]
equilibrium of {1}            [1.0000, 0.0000, 0.0000, 0.0000]
equilibrium of {1,2,3}        [0.7554, 0.5161, 0.1909, 0.0000]
growth rate of species 4 in {1,2,3} [-0.3583]
```

In the community of species `1` and `2` the equilibrium is `(0.7727, 0.5682)` with zeros for the absent species, and the growth rates are `0.0000` for the residents (they are at equilibrium), `0.1614` for species `3` and `-0.3045` for species `4`: species `3` can invade this community, species `4` cannot. Adding species `3` moves the equilibrium to `(0.7554, 0.5161, 0.1909)`, and species `4` is still excluded, with growth rate `-0.3583`.

## Building a community by invasion

The program lets the four species arrive one at a time. Each one joins if its growth rate when rare is positive, and the equilibrium of the residents is printed after every arrival.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace community = biology::population::community;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // build the community by invasion: species arrive one at a time, and join if their growth rate when rare in the current residents is positive
  const auto r = vec({1.0, 0.8, 0.6, 0.5});
  const auto a = pluss::series::make({{-1.0, -0.4, -0.2, -0.3}, {-0.3, -1.0, -0.3, -0.2}, {-0.2, -0.5, -1.0, -0.4}, {-0.6, -0.6, -0.5, -1.0}}).to(pluss::ScalarType::Float64);
  std::vector<double> mask{0.0, 0.0, 0.0, 0.0};
  for (int s : {0, 1, 2, 3}) {
    const double growth = flat(community::invasion_growth_rate(r, a, vec(mask)))[s];
    out.writeln("species ", std::vector<double>{static_cast<double>(s + 1)}, " arrives, growth rate when rare ", std::vector<double>{growth}, growth > 0.0 ? "  joins" : "  fails");
    if (growth > 0.0) mask[s] = 1.0;
    out.writeln("   residents ", mask, " equilibrium ", flat(community::resident_equilibrium(r, a, vec(mask))));
  }
}
```

Output

```text
species [1.0000] arrives, growth rate when rare [1.0000]  joins
   residents [1.0000, 0.0000, 0.0000, 0.0000] equilibrium [1.0000, 0.0000, 0.0000, 0.0000]
species [2.0000] arrives, growth rate when rare [0.5000]  joins
   residents [1.0000, 1.0000, 0.0000, 0.0000] equilibrium [0.7727, 0.5682, 0.0000, 0.0000]
species [3.0000] arrives, growth rate when rare [0.1614]  joins
   residents [1.0000, 1.0000, 1.0000, 0.0000] equilibrium [0.7554, 0.5161, 0.1909, 0.0000]
species [4.0000] arrives, growth rate when rare [-0.3583]  fails
   residents [1.0000, 1.0000, 1.0000, 0.0000] equilibrium [0.7554, 0.5161, 0.1909, 0.0000]
```

Species `1` joins an empty community with its own growth rate `1.0000` and settles at `1.0000`. Species `2` (rate `0.5000` when rare) and species `3` (`0.1614`) join, each reducing the others, and species `4` fails (`-0.3583`), so the assembled community is `{1,2,3}` at `(0.7554, 0.5161, 0.1909)`. This is the same community as the interior equilibrium of the three-species model on the [competition page](../bio_02_interactions/competition.md); invasion is a way to find which species belong to it. The order of arrival can matter when the invasion rates change sign, so assembly is worth repeating in other orders.

## One consumer on one nutrient

The program computes the break-even nutrient level of a consumer, checks that the uptake there equals the mortality, computes the equilibrium density, confirms both by integrating the chemostat for `200` time units, and then raises the supply.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace community = biology::population::community;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // one consumer on one nutrient in a chemostat: dilution D = 0.3 equals the mortality, supply S0 = 10, yield 1
  const auto mu_max = vec({1.0}), half = vec({2.0}), mortality = vec({0.3}), yield = vec({1.0});
  const auto rstar = community::r_star(mu_max, half, mortality);
  out.writeln("break-even nutrient R*        ", flat(rstar));
  out.writeln("uptake at R* (equals mortality) ", flat(community::monod(rstar, mu_max, half)));
  out.writeln("equilibrium consumer N*       ", flat(community::single_species_density(0.3, 10.0, rstar, mortality, yield)));
  // confirm by following the state (nutrient, consumer) for a long time
  const auto rate = [&](const pluss::Series &s) { return community::chemostat_rate(s, 0.3, 10.0, mu_max, half, mortality, yield); };
  const auto end = biology::dynamics::simulate(rate, vec({10.0, 0.1}), 0.0, 200.0, 4000);
  out.writeln("state after 200 time units    ", flat(end));
  // richer supply: the consumer density rises, the nutrient level stays at R*
  for (double supply : {5.0, 10.0, 20.0})
    out.writeln("supply ", std::vector<double>{supply}, " N* ", flat(community::single_species_density(0.3, supply, rstar, mortality, yield)));
}
```

Output

```text
break-even nutrient R*        [0.8571]
uptake at R* (equals mortality) [0.3000]
equilibrium consumer N*       [9.1429]
state after 200 time units    [0.8571, 9.1429]
supply [5.0000] N* [4.1429]
supply [10.0000] N* [9.1429]
supply [20.0000] N* [19.1429]
```

The break-even level is $R^* = 0.8571$, where the uptake `0.3000` exactly equals the mortality. The equilibrium density is $N^* = 9.1429$, and the integrated chemostat ends at `(0.8571, 9.1429)`, the same two numbers. The consumer holds the nutrient at $R^*$ whatever the supply, and takes up the difference as population: supplies of `5`, `10` and `20` give `4.1429`, `9.1429` and `19.1429`, each `0.8571` less than the supply.

## Decisions

- Use `resident_equilibrium` to evaluate any community you want to examine, and `invasion_growth_rate` to ask who could enter; build assembly by repeating the two.
- Treat a growth rate when rare near zero with care: the invasion is then slow, and the interaction coefficients decide the result.
- Use `r_star` first when species compete for one nutrient: the lowest $R^*$ wins, as on the competition page, and `single_species_density` only describes the winner alone.
- Remember the yield: it converts nutrient into consumers, and it sets the size of the population but not the break-even level.

## Pitfalls

The density formula is only meaningful above the break-even level. The program gives two consumers of different yield the same supply, and then a supply below the break-even level.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace community = biology::population::community;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the yield matters: the same break-even level supports more consumers when each unit of nutrient makes more of them
  const auto mu_max = vec({1.0, 1.0}), half = vec({2.0, 2.0}), mortality = vec({0.3, 0.3});
  const auto rstar = community::r_star(mu_max, half, mortality);
  out.writeln("break-even levels             ", flat(rstar));
  out.writeln("N* for yields (1, 2)          ", flat(community::single_species_density(0.3, 10.0, rstar, mortality, vec({1.0, 2.0}))));
  // a nutrient supply below R* cannot support the consumer: the formula returns a negative density, which means no population
  out.writeln("supply 0.5 (below R* = 0.857) ", flat(community::single_species_density(0.3, 0.5, rstar, mortality, vec({1.0, 1.0}))));
}
```

Output

```text
break-even levels             [0.8571, 0.8571]
N* for yields (1, 2)          [9.1429, 18.2857]
supply 0.5 (below R* = 0.857) [-0.3571, -0.3571]
```

The two consumers share `R* = 0.8571`, and the one with twice the yield reaches twice the density, `18.2857` against `9.1429`. A supply of `0.5` is below the break-even level, and the formula returns `-0.3571`: a negative density is not a population but a statement that the consumer cannot live on that supply, and you must treat it as zero.

## Where next

Go back to [Random communities](random_communities.md) for stability of large communities, to [Biology 2: Interacting Species](../bio_02_interactions.md) for the three-species example, and to [Biology 26: Adaptive Dynamics](../bio_26_adaptation.md) for how traits evolve once invasion is possible.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="random_communities.html">
    <span class="label">Previous</span>
    <span class="title">Random communities</span>
  </a>
</div>
<!-- apiref:nav:end -->
