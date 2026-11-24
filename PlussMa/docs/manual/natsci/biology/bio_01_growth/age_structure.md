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

<!-- addex:start -->
## Survival from a mortality schedule

The survivorship {math}`l(a) = \exp(-\int_0^a \mu)` is the fraction of a cohort alive at age {math}`a`. `age_structured::survivorship(mu, grid)` computes it from the mortality at the grid ages with the trapezoid rule. The program uses a mortality that rises with age, {math}`\mu(a) = 0.05 + 0.002 a^2`, on a grid of {math}`41` ages with step {math}`0.5`, prints the survivorship at six ages, and integrates it to get the expected years lived before age {math}`20`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace age = biology::population::age_structured;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const age::Grid grid{.step = 0.5, .nodes = 41};   // ages 0 to 20
  const auto ages = grid.ages();
  // senescent mortality that rises with age, mu(a) = 0.05 + 0.002 a^2, and the survivorship it gives
  const auto mu = ages.square().mul(0.002).add(0.05);
  const auto survival = age::survivorship(mu, grid);
  for (double a : {0.0, 2.0, 5.0, 10.0, 15.0, 20.0}) out.writeln("age ", a, ": survivorship = ", flat(survival.select(-1, static_cast<std::int64_t>(a / 0.5)))[0]);
  // life expectancy is the area under the survivorship curve
  out.writeln("expected years lived up to age 20 = ", flat(survival.antidiff().trapezoid(0.5, -1))[0]);
}
```

Output

```text
age 0.0000: survivorship = 1.0000
age 2.0000: survivorship = 0.8999
age 5.0000: survivorship = 0.7162
age 10.0000: survivorship = 0.3111
age 15.0000: survivorship = 0.0497
age 20.0000: survivorship = 0.0018
expected years lived up to age 20 = 7.7927
```

Survivorship starts at `1.0000`, is `0.7162` at age `5` and `0.3111` at age `10`, and then falls quickly to `0.0497` at `15` and `0.0018` at `20`, the pattern of a population that ages. The area under the curve, `7.7927`, is the expected number of years lived up to age `20`.

## Density dependence

When the rates depend on the total population {math}`N`, the model is nonlinear and the population settles at a level set by the rates rather than growing without bound. Setting `density_dependent = true` makes the solver evaluate the rates with the current total. The program uses a constant mortality {math}`0.4` and fecundity {math}`3/(1 + N/5)` for ages of {math}`1` and above, and follows the total population for {math}`40` years.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace age = biology::population::age_structured;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  const age::Grid grid{.step = 0.1, .nodes = 150};
  const auto ages = grid.ages();
  // density dependence: births fall as the total population N grows, beta(a, N) = 3 (a >= 1) / (1 + N / 5)
  const age::Model model{
      .mortality = [](const pluss::Series &a, const pluss::Series &) { return a.mul(0.0).add(0.4); },
      .fecundity = [](const pluss::Series &a, const pluss::Series &total) {
        const auto adult = a.ge(1.0).to(pluss::ScalarType::Float64);
        return adult.mul(3.0).div(total.unsqueeze(-1).div(5.0).add(1.0));
      },
      .density_dependent = true};
  auto n = ages.mul(-0.5).exp();
  for (int year = 0; year <= 40; year += 10) {
    out.writeln("year ", year, ": total population = ", flat(age::total(n, grid))[0]);
    n = age::simulate(n, model, grid, 100);
  }
}
```

Output

```text
year 0: total population = 1.999
year 10: total population = 16.593
year 20: total population = 20.178
year 30: total population = 20.554
year 40: total population = 20.590
```

The population starts at `1.999`, grows to `16.593` by year `10` and settles near `20.59`: births fall as the population grows until they balance the deaths. The total changes by under `0.05` between year `30` and year `40`, so the population has reached its equilibrium.

<!-- addex:end -->

## Decisions

- Use `euler_lotka` when you have schedules by age and want the long-run rate; use `simulate` when density dependence or a changing environment means the rates are not fixed.
- Make the grid long enough to cover the whole life and the whole breeding span: births or deaths beyond the last node are not seen.
- Read `r0` before `r`: `r0` below `1` means a lifetime does not replace the parent and `r` will be negative.

## Pitfalls

A schedule that does not replace the parent, or a grid that stops before breeding starts, is accepted without complaint. The program computes `r0` and `r` for the page's fecundity of `2.5` and for `0.4`, and then for a grid that ends at age `1.4`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace age = biology::population::age_structured;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const age::Grid grid{.step = 0.1, .nodes = 200};
  const auto ages = grid.ages();
  const auto mu = ages.mul(0.0).add(0.5);
  // fecundity 0.4 instead of 2.5 between ages 2 and 10: a lifetime does not replace the parent
  for (double b : {2.5, 0.4}) {
    const auto beta = ages.ge(2.0).logical_and(ages.le(10.0)).to(pluss::ScalarType::Float64).mul(b);
    const auto table = age::euler_lotka(mu, beta, grid);
    out.writeln("fecundity ", b, ": R0 = ", flat(table.r0), ", r = ", flat(table.r));
  }
  // a grid that ends before breeding starts cannot see any births at all
  const age::Grid short_grid{.step = 0.1, .nodes = 15};
  const auto short_ages = short_grid.ages();
  const auto short_beta = short_ages.ge(2.0).to(pluss::ScalarType::Float64).mul(2.5);
  out.writeln("grid ending at age 1.4: R0 = ", flat(age::euler_lotka(short_ages.mul(0.0).add(0.5), short_beta, short_grid).r0));
}
```

Output

```text
fecundity 2.5000: R0 = [1.8529], r = [0.1717]
fecundity 0.4000: R0 = [0.2965], r = [-0.2832]
grid ending at age 1.4: R0 = [0.0000]
```

With fecundity `2.5` the population grows (`R0 = 1.8529`, `r = 0.1717`); with `0.4` each individual leaves only `0.2965` offspring and the intrinsic rate is `-0.2832`, a decline, not an error. The short grid gives `R0 = 0.0000`: no births are visible before the grid ends, so the population looks sterile. Print `r0` whenever a rate looks wrong.

## Where next

[Growth laws](growth_laws.md) is the one-number version of the same question, and [Stage matrices](../bio_03_demography/stage_matrices.md) handles populations whose classes are stages rather than ages.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="allee_and_theta.html">
    <span class="label">Previous</span>
    <span class="title">Thresholds and shape</span>
  </a>
</div>
<!-- apiref:nav:end -->
