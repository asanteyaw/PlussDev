<!-- generated from age_structure.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Age structure

[Back to Biology 1: Growth](../bio_01_growth.md)

A population is not a single number: young individuals do not breed and old ones die. Describing growth by age gives a rate that comes from the age schedule of survival and births rather than from one coefficient. This page sets a mortality and a fecundity schedule on an age grid, finds the intrinsic growth rate from the Euler-Lotka equation, and then simulates a population to confirm that its growth settles onto that rate.

## How it works

An `age_structured::Grid` is a step and a number of nodes; `grid.ages()` returns the age at every node. Mortality `mu(a)` and fecundity `beta(a)` are given as functions of age (and of total population, for density dependence). `euler_lotka(mu, beta, grid)` takes both at the nodes and returns the net reproductive rate `r0`, which is the expected number of offspring over a lifetime, and the intrinsic rate `r` that solves the Euler-Lotka equation, the long-run growth rate per unit time. `simulate(n, model, grid, steps)` advances the age density `n` by that many steps, and `total(n, grid)` integrates it over age.

## From schedules to a growth rate

Individuals die at a constant rate `0.5`, and breed only between ages `2` and `10` at rate `2.5`. The program prints `r0` and `r` from the schedules, then starts from an unrelated age distribution and, once a year (ten steps of `0.1`), reports the growth rate of the total population over that year.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace age = biology::population::age_structured;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const age::Grid grid{.step = 0.1, .nodes = 200};
  const auto ages = grid.ages();
  // constant death rate 0.5; births only between ages 2 and 10, at rate 1.2
  const auto mu = ages.mul(0.0).add(0.5);
  const auto beta = ages.ge(2.0).logical_and(ages.le(10.0)).to(pluss::ScalarType::Float64).mul(2.5);
  const auto table = age::euler_lotka(mu, beta, grid);
  out.writeln("net reproductive rate R0 = ", flat(table.r0));
  out.writeln("intrinsic rate r         = ", flat(table.r));
  // simulate from an arbitrary age distribution and watch the total settle onto exp(r t)
  const age::Model model{.mortality = [&](const pluss::Series &a, const pluss::Series &) { return a.mul(0.0).add(0.5); },
                         .fecundity = [&](const pluss::Series &a, const pluss::Series &) {
                           return a.ge(2.0).logical_and(a.le(10.0)).to(pluss::ScalarType::Float64).mul(2.5);
                         }};
  auto n = ages.mul(-0.5).exp();
  double previous = flat(age::total(n, grid))[0];
  for (int year = 1; year <= 20; ++year) {
    n = age::simulate(n, model, grid, 10);
    const double now = flat(age::total(n, grid))[0];
    if (year % 5 == 0) out.writeln("year ", year, ": growth rate over the last year = ", std::log(now / previous));
    previous = now;
  }
}
```

Output

```text
net reproductive rate R0 = [1.8529]
intrinsic rate r         = [0.1717]
year 5: growth rate over the last year = 0.1630
year 10: growth rate over the last year = 0.1715
year 15: growth rate over the last year = 0.1717
year 20: growth rate over the last year = 0.1717
```

`r0` of `1.8529` means each individual leaves about 1.85 offspring over its life, so the population grows, and the schedule gives `r = 0.1717` per unit time. The simulated growth rate starts at `0.1630` while the starting age distribution is still unsettled, and by year 15 it matches the Euler-Lotka rate to four digits: whatever the starting ages, the population settles onto the growth rate that its schedules imply.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="allee_and_theta.html">
    <span class="label">Previous</span>
    <span class="title">Thresholds and shape</span>
  </a>
</div>
<!-- apiref:nav:end -->
