# Competing species

[Back to Biology 2: Interacting Species](../bio_02_interactions.md)

With several species the question is which ones coexist. In the generalised Lotka-Volterra model every species has a growth rate and every pair a coefficient for how much one slows the other, and the whole community is a matrix. This page finds the interior equilibrium of a three-species community, checks that it is feasible and stable, and asks whether an absent species could invade.

## How it works

`glv_rate(n, r, a)` is the rate `n_i (r_i + sum_j a_ij n_j)`. `glv_equilibrium(r, a)` solves `a n = -r` for the interior equilibrium, and `is_feasible` reports whether every density is positive; an equilibrium with a negative density does not exist as a community. `glv_jacobian_at(n_star, a)` is the community matrix there, and `community::spectral_abscissa` of it is the largest real part among its eigenvalues: negative means the equilibrium is stable. `community::invasion_growth_rate(r, a, resident)` takes a mask of the resident species, settles those at their own equilibrium and returns the per-capita growth rate of every species in that community, so a positive value for an absent species means it can invade.

## Feasible, stable and invadable

The program solves the community, evaluates the rate and the stability there, then repeats the solve for a community in which species `3` competes much harder with the others, and last asks whether species `3` can invade the community of species `1` and `2`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace inter = biology::population::interaction;
namespace community = biology::population::community;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // three species with growth rates r and interaction matrix A: dn_i = n_i (r_i + sum_j A_ij n_j)
  const auto r = vec({1.0, 0.8, 0.6});
  const auto a = mat({{-1.0, -0.4, -0.2}, {-0.3, -1.0, -0.3}, {-0.2, -0.5, -1.0}});
  const auto n_star = inter::glv_equilibrium(r, a);
  out.writeln("interior equilibrium       = ", flat(n_star));
  out.writeln("all positive (1 = yes)    = ", flat(inter::is_feasible(n_star).to(pluss::ScalarType::Float64)));
  out.writeln("rate at the equilibrium    = ", flat(inter::glv_rate(n_star, r, a)));
  const auto jac = inter::glv_jacobian_at(n_star, a);
  out.writeln("spectral abscissa of J     = ", flat(community::spectral_abscissa(jac)));

  // a second community whose strong competition makes the interior equilibrium infeasible
  const auto harsh = mat({{-1.0, -0.4, -0.2}, {-0.3, -1.0, -0.3}, {-1.5, -1.5, -1.0}});
  const auto n_harsh = inter::glv_equilibrium(r, harsh);
  out.writeln("harsh equilibrium          = ", flat(n_harsh));
  out.writeln("all positive (1 = yes)    = ", flat(inter::is_feasible(n_harsh).to(pluss::ScalarType::Float64)));

  // can species 3 invade when only species 1 and 2 are resident?
  const auto resident = vec({1.0, 1.0, 0.0});
  out.writeln("invasion growth rates      = ", flat(community::invasion_growth_rate(r, a, resident)));
}
```

Output

```text
interior equilibrium       = [0.7554, 0.5161, 0.1909]
all positive (1 = yes)    = [1.0000]
rate at the equilibrium    = [0.0000, 0.0000, 0.0000]
spectral abscissa of J     = [-0.1506]
harsh equilibrium          = [1.0550, 1.4150, -3.1050]
all positive (1 = yes)    = [0.0000]
invasion growth rates      = [0.0000, 0.0000, 0.1614]
```

The interior equilibrium has densities `0.7554`, `0.5161` and `0.1909` with zero rate, and its community matrix has spectral abscissa `-0.1506`, so it is feasible and stable. With heavy competition on species `3` the same solve returns `-3.1050` for that species and the feasibility flag is `0`: no coexistence equilibrium exists. In the last line the residents have growth rate `0.0000` (they are at equilibrium) and species `3` has `0.1614` while rare, so it can invade.

<!-- addex:start -->
## Two competitors

For two species competing for the same resource, the outcome depends on whether each species limits the other more than itself. `competitive_rate(n, r, K, alpha)` is the rate of the competitive Lotka-Volterra model {math}`r_i n_i(1 - \sum_j \alpha_{ij} n_j / K_i)`. The program starts the species at {math}`10` and {math}`12` with {math}`K = 100` and runs three interaction matrices to time {math}`60`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace population = biology::population;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace inter = population::interaction;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // two competitors, dn_i = r_i n_i (1 - sum_j alpha_ij n_j / K_i), with K = 100 for both, started at 10 each
  const auto r = vec({1.0, 1.0}), k = vec({100.0, 100.0});
  const auto start = vec({10.0, 12.0});
  const auto run = [&](const char *label, const pluss::Series &alpha) {
    const auto rate = [&](const pluss::Series &n) { return inter::competitive_rate(n, r, k, alpha); };
    out.writeln(label, flat(dynamics::simulate(rate, start, 0.0, 60.0, 3000)));
  };
  run("weak competition  (alpha = 0.5): ", mat({{1.0, 0.5}, {0.5, 1.0}}));
  run("strong competition (alpha = 1.5): ", mat({{1.0, 1.5}, {1.5, 1.0}}));
  run("species 1 dominant (a12 = 0.4, a21 = 1.6): ", mat({{1.0, 0.4}, {1.6, 1.0}}));
}
```

Output

```text
weak competition  (alpha = 0.5): [66.6667, 66.6667]
strong competition (alpha = 1.5): [0.0000, 100.0000]
species 1 dominant (a12 = 0.4, a21 = 1.6): [100.0000, 0.0000]
```

Weak competition (`0.5`) lets both species persist at `66.6667` each, which is {math}`K/(1 + \alpha)`. With strong competition (`1.5`) the species with the small head start wins and the other is excluded (`0.0000`, `100.0000`); the same happens when one species is much more harmful to the other than the other is to it, here species `1` excludes species `2`.

## Competition for one resource

When species compete for a single nutrient, the one that can live on the least of it wins: its break-even level {math}`R^* = m K_s / (\mu_{max} - m)` is the lowest. `community::r_star(mu_max, half, loss)` computes it for all species at once and `community::chemostat_rate` is the rate function of the nutrient and the populations in a chemostat. The program computes {math}`R^*` for three microbes and follows the chemostat for {math}`200` days.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace population = biology::population;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace community = population::community;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // three microbes sharing one nutrient: maximum growth rate, half-saturation constant and loss rate (per day)
  const auto mu_max = vec({1.0, 1.4, 0.9});
  const auto half = vec({2.0, 4.0, 0.8});
  const auto loss = vec({0.3, 0.3, 0.3});
  // R*: the nutrient level at which a species just breaks even; the species with the lowest R* wins
  const auto r_star = community::r_star(mu_max, half, loss);
  out.writeln("break-even nutrient R*       ", flat(r_star));
  // follow the nutrient and the three populations in a chemostat with dilution 0.3 and supply 10 (all yields 1)
  const auto yield = vec({1.0, 1.0, 1.0});
  const auto rate = [&](const pluss::Series &s) { return community::chemostat_rate(s, 0.3, 10.0, mu_max, half, loss, yield); };
  const auto start = vec({10.0, 0.5, 0.5, 0.5});
  for (double t : {10.0, 50.0, 200.0}) out.writeln("day ", t, "  (nutrient, populations) = ", flat(dynamics::simulate(rate, start, 0.0, t, 8000)));
}
```

Output

```text
break-even nutrient R*       [0.8571, 1.0909, 0.4000]
day 10.0000  (nutrient, populations) = [0.5499, 1.8148, 1.8425, 5.8675]
day 50.0000  (nutrient, populations) = [0.4004, 0.0124, 0.0025, 9.5846]
day 200.0000  (nutrient, populations) = [0.4000, 0.0000, 0.0000, 9.6000]
```

The three break-even levels are `0.8571`, `1.0909` and `0.4000`, so the third microbe is the best competitor. In the chemostat all three grow at first (day `10`), the nutrient is drawn down to `0.4004` by day `50`, and by day `200` only the third microbe is left (`9.6000`) with the nutrient at exactly its {math}`R^* = 0.4000`.

## When coexistence stops being feasible

The interior equilibrium of the generalised Lotka-Volterra model has a density for every species only for some interaction strengths. The program raises the competition between species `1` and `2` through five values and prints the equilibrium and whether all densities are positive.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace population = biology::population;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace inter = population::interaction;
namespace community = population::community;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // as competition between species 1 and 2 grows stronger, the interior equilibrium of the three-species community changes; find where it stops being feasible
  const auto r = vec({1.0, 0.8, 0.6});
  std::vector<double> strengths = {0.2, 0.5, 0.8, 1.0, 1.2};
  for (double c : strengths) {
    const auto a = mat({{-1.0, -c, -0.2}, {-c, -1.0, -0.3}, {-0.2, -0.5, -1.0}});
    const auto n = inter::glv_equilibrium(r, a);
    out.writeln("competition ", c, ": equilibrium = ", flat(n), ", feasible = ", flat(inter::is_feasible(n).to(pluss::ScalarType::Float64))[0] > 0.5 ? "yes" : "no");
  }
}
```

Output

```text
competition 0.2000: equilibrium = [0.8554, 0.5885, 0.1347], feasible = yes
competition 0.5000: equilibrium = [0.7812, 0.3250, 0.2812], feasible = yes
competition 0.8000: equilibrium = [1.0537, -0.1879, 0.4832], feasible = no
competition 1.0000: equilibrium = [-6.3333, 7.7333, -2.0000], feasible = no
competition 1.2000: equilibrium = [-0.1507, 0.9315, 0.1644], feasible = no
```

The equilibrium is feasible at competition `0.2` and `0.5`; from `0.8` one density is negative (`-0.1879` for species `2`), so no coexistence point exists and at least one species is lost. The test is only the sign of the solution, and the loss of feasibility is abrupt.

<!-- addex:end -->

## Decisions

- Use `glv_equilibrium` with `is_feasible` and `spectral_abscissa` to test a community in one pass; use `invasion_growth_rate` when the question is about a species that is absent.
- Use `competitive_rate` when each species has its own capacity `K` and coefficients `alpha` are relative strengths; use the generalised form when you have signed rates per individual.
- Use `r_star` for a single limiting resource: the species with the smallest break-even level wins, and no matrix is needed.

## Pitfalls

In the generalised model the interaction matrix holds signed effects, so competition is negative. Entering the strengths as positive numbers builds a mutualistic community instead, and nothing in the call objects. The program solves the page's community with the negative matrix and with the same numbers made positive.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace inter = biology::population::interaction;
namespace community = biology::population::community;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the interaction matrix holds signed effects: competition is NEGATIVE. Entering the strengths as positive numbers is a different community
  const auto r = vec({1.0, 0.8, 0.6});
  const auto right = mat({{-1.0, -0.4, -0.2}, {-0.3, -1.0, -0.3}, {-0.2, -0.5, -1.0}});
  const auto wrong = mat({{1.0, 0.4, 0.2}, {0.3, 1.0, 0.3}, {0.2, 0.5, 1.0}});
  for (const auto &a : {right, wrong}) {
    const auto n = inter::glv_equilibrium(r, a);
    out.writeln("equilibrium ", flat(n), ", feasible ", flat(inter::is_feasible(n).to(pluss::ScalarType::Float64)), ", spectral abscissa ", flat(community::spectral_abscissa(inter::glv_jacobian_at(n, a))));
  }
}
```

Output

```text
equilibrium [0.7554, 0.5161, 0.1909], feasible [1.0000], spectral abscissa [-0.1506]
equilibrium [-0.7554, -0.5161, -0.1909], feasible [0.0000], spectral abscissa [-0.1506]
```

The correct matrix gives the feasible, stable community `(0.7554, 0.5161, 0.1909)`. The positive matrix gives the same numbers with the opposite sign, so `is_feasible` reports `0`. The spectral abscissa is the same `-0.1506` in both, so a stability check alone would not have caught the mistake. If every density comes out negative, check the sign of `A` first.

## Where next

[Predators and prey](predators_and_prey.md) covers interactions that help one species and hurt the other, and [Biology 26: Adaptive Dynamics](../bio_26_adaptation.md) asks which competitors can invade and branch.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="predators_and_prey.html">
    <span class="label">Previous</span>
    <span class="title">Predators and prey</span>
  </a>
</div>
<!-- apiref:nav:end -->
