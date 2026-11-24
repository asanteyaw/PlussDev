# Extinction

[Back to Biology 3: Demography and Extinction](../bio_03_demography.md)

A population that grows on average can still die out by chance, and a bounded population can last a long time without being safe. This page models births and deaths as random events: the expected time to extinction of a bounded population, the state it lingers in before extinction, and the chance that a growing population started from a few founders still goes extinct.

## How it works

A birth-death chain gives each population size `n` a birth rate and a death rate; `generator(birth, death)` turns the two vectors into the transition-rate matrix. `mean_time_to_extinction(q)` is the expected time to reach zero from each size `1..N`. `quasi_stationary(q)` finds the distribution a population settles into while it has not yet gone extinct and its `decay_rate`, the constant rate at which it then goes extinct. For an unbounded population with per-capita rates `birth` and `death`, `linear_ultimate_extinction` is `(death/birth)^n` from `n` founders and `linear_extinction_probability` the chance that extinction has happened by time `t`.

## Time to extinction

The first part of the program bounds a population at `20` animals with births at `0.6` per animal and deaths at `0.5` and prints the expected time to extinction and the quasi-stationary state. The second part removes the bound and prints the extinction probability for one and for five founders, and by three times for five founders.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace bd = biology::population::birth_death;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a population of at most 20 animals. Each breeds at rate 0.6 while there is room; each dies at rate 0.5.
  const int capacity = 20;
  std::vector<double> births, deaths;
  for (int n = 0; n <= capacity; ++n) {
    births.push_back(n == capacity ? 0.0 : 0.6 * n);
    deaths.push_back(0.5 * n);
  }
  const auto q = bd::generator(vec(births), vec(deaths));
  const auto tau = flat(bd::mean_time_to_extinction(q));
  out.writeln("expected time to extinction from 1, 5, 10, 20 animals: ", std::vector<double>{tau[0], tau[4], tau[9], tau[19]});
  const auto qsd = bd::quasi_stationary(q);
  out.writeln("quasi-stationary extinction rate = ", flat(qsd.decay_rate));
  const auto mode = flat(qsd.distribution);
  out.writeln("probability of 1, 10, 20 animals in the quasi-stationary state: ", std::vector<double>{mode[0], mode[9], mode[19]});

  // the unbounded linear process: extinction chance for a founder group of 1 and of 5
  out.writeln("ultimate extinction, birth 0.6 death 0.5, from 1 and from 5 = ",
              std::vector<double>{bd::linear_ultimate_extinction(1, 0.6, 0.5), bd::linear_ultimate_extinction(5, 0.6, 0.5)});
  // extinction chance by time t for 5 founders
  out.writeln("extinct by t = 5, 20, 100: ", flat(bd::linear_extinction_probability(vec({5.0, 5.0, 5.0}), 0.6, 0.5, vec({5.0, 20.0, 100.0}))));
}
```

Output

```text
expected time to extinction from 1, 5, 10, 20 animals: [29.9588, 98.7952, 131.5289, 143.7757]
quasi-stationary extinction rate = [0.0075]
probability of 1, 10, 20 animals in the quasi-stationary state: [0.0151, 0.0369, 0.1258]
ultimate extinction, birth 0.6 death 0.5, from 1 and from 5 = [0.8333, 0.4019]
extinct by t = 5, 20, 100: [0.1281, 0.3533, 0.4019]
```

Births exceed deaths, yet a single founder in a population limited to `20` animals goes extinct after `29.9588` time units on average, while a population of `20` lasts `143.7757`. Before extinction the population lingers in a quasi-stationary state in which the chance of going extinct is `0.0075` per unit time. Without the bound, a single founder leaves no descendants with probability `0.8333` (`0.5/0.6`) and five founders with probability `0.4019`; the extinction by time `100` for five founders is `0.4019`, matching the limit, and most of it has happened by time `20` (`0.3533`).

<!-- addex:start -->
## A branching process

Extinction of a line of descent is the discrete case of the same question: each individual leaves a random number of offspring. `birth_death::offspring_mean`, `extinction_by_generation(p, generations)` and `ultimate_extinction(p)` take the probabilities `p[k]` of leaving {math}`k` offspring, and the last returns the smallest fixed point of the generating function, the probability of eventual extinction. The program uses the probabilities `0.2`, `0.3`, `0.3`, `0.2` for {math}`0` to {math}`3` offspring and then a distribution with mean below one.

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

namespace birth_death = population::birth_death;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a branching process: each individual leaves 0, 1, 2 or 3 offspring with probabilities 0.2, 0.3, 0.3, 0.2
  const auto offspring = vec({0.2, 0.3, 0.3, 0.2});
  out.writeln("mean offspring per individual     = ", flat(birth_death::offspring_mean(offspring)));
  out.writeln("probability of ultimate extinction = ", flat(birth_death::ultimate_extinction(offspring)));
  out.writeln("extinct by generation 0 to 8      = ", flat(birth_death::extinction_by_generation(offspring, 8)));
  // a harsher offspring distribution with mean below one dies out for certain
  out.writeln("mean 0.8: ultimate extinction     = ", flat(birth_death::ultimate_extinction(vec({0.5, 0.2, 0.3}))));
}
```

Output

```text
mean offspring per individual     = [1.5000]
probability of ultimate extinction = [0.3508]
extinct by generation 0 to 8      = [0.0000, 0.2000, 0.2736, 0.3086, 0.3270, 0.3372, 0.3429, 0.3462, 0.3481]
mean 0.8: ultimate extinction     = [1.0000]
```

Each individual leaves `1.5` offspring on average, so the line can survive, but it still dies out with probability `0.3508`. The probability of extinction by each generation climbs `0.2000`, `0.2736`, `0.3086`, ... and settles at that limit. When the mean offspring number is below one (`0.8` here) extinction is certain (`1.0000`).

## Risk over a horizon

For a large population with random year-to-year growth the log abundance behaves like a random walk with drift {math}`\mu` and variance {math}`\sigma^2`, and the chance of falling to a threshold within a time {math}`t` has a closed form, `quasi_extinction_cdf(mu, sigma2, x_d, t)`, where {math}`x_d = \ln(N_0/N_c)` is the distance to the threshold. `ultimate_quasi_extinction` gives the limit as {math}`t \to \infty`. The program takes a population of `500` with a threshold of `50`, {math}`\sigma^2 = 0.04` and three growth rates, and prints the risk at four horizons.

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

namespace birth_death = population::birth_death;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a population of 500 with a threshold of 50 (so x_d = ln 10), growth rate mu and variability sigma^2 per year
  const auto x_d = vec({std::log(10.0)});
  for (double mu : {0.0, 0.02, -0.02}) {
    const auto horizons = vec({10.0, 25.0, 50.0, 100.0});
    out.writeln("mu = ", mu, ": risk by 10, 25, 50, 100 years = ",
                flat(birth_death::quasi_extinction_cdf(vec({mu, mu, mu, mu}), vec({0.04, 0.04, 0.04, 0.04}), vec({std::log(10.0), std::log(10.0), std::log(10.0), std::log(10.0)}), horizons)),
                ", ultimate = ", flat(birth_death::ultimate_quasi_extinction(vec({mu}), vec({0.04}), x_d)));
  }
}
```

Output

```text
mu = 0.0000: risk by 10, 25, 50, 100 years = [0.0003, 0.0213, 0.1035, 0.2496], ultimate = [1.0000]
mu = 0.0200: risk by 10, 25, 50, 100 years = [0.0001, 0.0061, 0.0276, 0.0597], ultimate = [0.1000]
mu = -0.0200: risk by 10, 25, 50, 100 years = [0.0008, 0.0611, 0.2762, 0.5971], ultimate = [1.0000]
```

With no drift the risk is `0.0003` in `10` years but `0.2496` in `100`, and it ends at certainty (`1.0000`). With a growth of `0.02` per year it stays small, `0.0597` after `100` years, and the ultimate risk is only `0.1000`. With a decline of `0.02` the same horizons give `0.0008`, `0.0611`, `0.2762` and `0.5971`, and extinction is certain. The risk depends on the horizon much more than on the first few years suggest.

## From counts to risk

The drift and variance are estimated from a sequence of censuses by `birth_death::dennis_estimate(counts, times)`, a regression of the log ratios of successive counts. The program fits ten yearly counts that grow noisily from `200` to `281` and then asks for the risk of falling below `100`.

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

namespace birth_death = population::birth_death;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // ten yearly counts of a population; the diffusion estimates of its drift and variability come from the log ratios
  const auto counts = vec({200.0, 214.0, 196.0, 230.0, 247.0, 221.0, 256.0, 268.0, 249.0, 281.0});
  const auto years = vec({0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0});
  const auto fit = birth_death::dennis_estimate(counts, years);
  out.writeln("drift mu          = ", flat(fit.mu), " (standard error ", flat(fit.mu_standard_error), ")");
  out.writeln("variance sigma^2  = ", flat(fit.sigma2));
  // risk that the population falls to 100 within 20 and 50 years
  const auto xd = vec({std::log(281.0 / 100.0), std::log(281.0 / 100.0)});
  out.writeln("risk of falling below 100 in 20 and 50 years = ", flat(birth_death::quasi_extinction_cdf(vec({flat(fit.mu)[0], flat(fit.mu)[0]}), vec({flat(fit.sigma2)[0], flat(fit.sigma2)[0]}), xd, vec({20.0, 50.0}))));
}
```

Output

```text
drift mu          = [0.0378] (standard error [0.0346])
variance sigma^2  = [0.0108]
risk of falling below 100 in 20 and 50 years = [0.0003, 0.0007]
```

The estimated drift is `0.0378` per year with a standard error of `0.0346`, which is almost as large as the estimate itself, and the variance is `0.0108`. The resulting risk of falling below `100` is `0.0003` within `20` years and `0.0007` within `50`. The risks treat the drift as if it were exactly known; since its standard error is nearly as large as the estimate, the true risk is far less certain than these small numbers suggest.

<!-- addex:end -->

## Decisions

- Use `generator`, `mean_time_to_extinction` and `quasi_stationary` for a bounded population with explicit rates; use the `linear_*` functions when the population is unbounded and the rates are per individual.
- Use `quasi_extinction_cdf` for large populations with year-to-year noise, and `dennis_estimate` to get its drift and variance from counts.
- Report the standard error of the drift next to any risk computed from it.

## Pitfalls

Two things are easy to misread. In the chain, size `0` is absorbing, so a birth rate entered for an empty population is not used. And the risk formula needs a positive horizon. The program changes the size-zero birth rate from `0` to `0.05` and then asks for the risk over a negative horizon.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace bd = biology::population::birth_death;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // size 0 is absorbing: a birth rate entered for an empty population is not used, so immigration cannot be modelled this way
  for (double immigration : {0.0, 0.05}) {
    std::vector<double> births, deaths;
    for (int n = 0; n <= 20; ++n) {
      births.push_back(n == 20 ? 0.0 : (n == 0 ? immigration : 0.6 * n));
      deaths.push_back(0.5 * n);
    }
    const auto tau = flat(bd::mean_time_to_extinction(bd::generator(vec(births), vec(deaths))));
    out.writeln("birth rate at size 0 = ", immigration, ": ", tau.size(), " values, from 1 animal ", tau[0], ", from 20 animals ", tau[19]);
  }
  // a risk from a single threshold: the horizon must be positive
  try {
    (void)bd::quasi_extinction_cdf(vec({0.0}), vec({0.04}), vec({2.3}), vec({-5.0}));
  } catch (const pluss::Error &e) {
    out.writeln("negative horizon: ", e.message());
  }
}
```

Output

```text
birth rate at size 0 = 0.0000: 20 values, from 1 animal 29.9588, from 20 animals 143.7757
birth rate at size 0 = 0.0500: 20 values, from 1 animal 29.9588, from 20 animals 143.7757
negative horizon: natsci: quasi_extinction_cdf: t must be strictly positive
```

The expected extinction times are the same in both runs (`29.9588` from one animal, `143.7757` from twenty), because an empty population stays empty; immigration cannot be modelled by a birth rate at size zero. The vector has `20` values, for sizes `1` to `20`, so size `n` is at index `n - 1`. The negative horizon is rejected with a `pluss::Error` that names the argument.

## Where next

[Stage matrices](stage_matrices.md) takes the same population apart by stage, and [Biology 23: Census Data](../bio_23_census.md) fits the hidden abundance behind noisy counts.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="stage_matrices.html">
    <span class="label">Previous</span>
    <span class="title">Stage matrices</span>
  </a>
</div>
<!-- apiref:nav:end -->
