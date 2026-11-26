# Gillespie ensembles

[Back to Methods 4: Stochastic kinetics](../meth_04_stochastic_kinetics.md)

When a reacting system has few molecules, or a population has few individuals, the smooth rate equations stop describing what happens: each run is a different jagged path, and a large share of runs can end in extinction that the average never shows. This page simulates such systems exactly. You write the reactions down as a stoichiometry matrix and a propensity function, and `ssa::gillespie` runs thousands of independent replicates at once and returns the state of every replicate at your chosen end time.

## How it works

A system with `S` species and `R` reactions is described by two objects. The stoichiometry matrix `nu` has shape `[R, S]`: row `j` is the change in the species counts when reaction `j` fires once. The propensity function maps the current state `x` of shape `[B, S]` (one row per replicate) to a matrix `a` of shape `[B, R]`, where `a[b, j]` is the rate at which reaction `j` fires in replicate `b`.

From these the direct method draws, for every replicate, the time to the next event and which reaction it is. With the total propensity `a0 = sum_j a_j`, the waiting time is exponential and the reaction is chosen in proportion to its propensity:

```{math}
\tau = \frac{\ln(1/r_1)}{a_0}, \qquad \mu = \min\Big\{ j : \sum_{j' \le j} a_{j'} > r_2\, a_0 \Big\}, \qquad x \leftarrow x + \nu_\mu
```

where `r_1` and `r_2` are uniform random numbers. One pass of the event loop fires one event in every replicate that is still running, using whole-ensemble operations, so adding replicates costs far less than running more loops. A replicate stops for good when its next event would fall after `t_end` (the state at `t_end` is then exactly the current state, because the exponential clock has no memory) or when its total propensity is zero (it has gone extinct or reached an absorbing state). `SsaResult` holds `state` of shape `[B, S]` at `t_end`, `events` with the number of events each replicate fired, and `completed`, which is `true` only when every replicate finished within `max_events`.

## An immigration-death ensemble

The simplest open system has one species `X` that arrives at rate `k` and dies at rate `g x`. Its propensities are `k` and `g x`, and its stoichiometry is `(+1, -1)`. The stationary distribution is Poisson with mean `k/g`, so you can check the ensemble against two numbers: a mean of `10` and a variance equal to the mean. The program runs 2000 replicates from an empty start to `t = 20`, and sets `options.seed` so the run can be repeated.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ssa = pluss::ma::natsci::common::ssa;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  // one species X, two reactions: immigration (0 -> X) at rate k and death (X -> 0) at rate g x
  const double k = 5.0, g = 0.5;
  const pluss::Series nu = pluss::series::make({1.0, -1.0}, {2, 1});
  const auto propensity = [&](const pluss::Series &x) {
    const pluss::Series birth = x.mul(0.0).add(k);
    return birth.unite(std::vector<pluss::Series>{x.mul(g)}, -1);
  };
  const pluss::Series x0 = pluss::series::zeros({2000, 1}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  ssa::SsaOptions options;
  options.seed = 12345;
  const auto run = ssa::gillespie(propensity, nu, x0, 20.0, options);
  const double mean = run.state.mean().item<double>();
  const double var = run.state.sub(mean).square().mean().item<double>();
  out.writeln("completed: ", run.completed ? "yes" : "no");
  out.writeln("mean copies at t = 20: ", mean, " (stationary value k/g = ", k / g, ")");
  out.writeln("variance / mean: ", var / mean, " (Poisson: 1)");
  out.writeln("mean events per replicate: ", run.events.mean().item<double>());
}
```

Output

```text
completed: yes
mean copies at t = 20: 10.069 (stationary value k/g = 10.000)
variance / mean: 1.009 (Poisson: 1)
mean events per replicate: 189.941
```

The mean of `10.069` is within sampling error of `k/g = 10`, and the variance-to-mean ratio of `1.009` is the Poisson value of one. A deterministic rate equation would give the mean alone; the spread is information only the ensemble carries. Each replicate fired about 190 events, which matches a total rate near `k + g x = 10` over twenty time units.

## A stochastic epidemic with a fizzling probability

A branching-process argument says that one infected person in a large susceptible population starts a major outbreak only with probability `1 - gamma/beta`, because the first few infections can recover before passing it on. The program tracks `S`, `I` and `R` in a population of 200 with `beta = 1` and `gamma = 0.5`, so `R0 = 2`. It gives the infection propensity `beta S I / N` and the recovery propensity `gamma I` as the two columns, and it counts a run as a major outbreak when at least 20 people recover. It then compares the fraction of such runs with `1 - gamma/beta` and their mean final size with `N z`, where `z` solves the final-size equation `z = 1 - exp(-R0 z)`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  // stochastic SIR in a population of N = 200: infection S + I -> 2 I at rate beta S I / N, recovery I -> R at rate gamma I
  const double beta = 1.0, gamma = 0.5, n = 200.0;  // basic reproduction number beta / gamma = 2
  const pluss::Series nu = pluss::series::make({-1.0, 1.0, 0.0, 0.0, -1.0, 1.0}, {2, 3});
  const auto propensity = [&](const pluss::Series &x) {
    const pluss::Series s = x.select(-1, 0), i = x.select(-1, 1);
    return s.mul(i).mul(beta / n).unsqueeze(-1).unite(std::vector<pluss::Series>{i.mul(gamma).unsqueeze(-1)}, -1);
  };
  const std::int64_t replicates = 4000;
  std::vector<double> start;
  for (std::int64_t r = 0; r < replicates; ++r) start.insert(start.end(), {n - 1.0, 1.0, 0.0});
  const pluss::Series x0 = pluss::series::make(start, {replicates, 3}).to(pluss::ScalarType::Float64);

  common::ssa::SsaOptions options;
  options.seed = 7;
  const auto run = common::ssa::gillespie(propensity, nu, x0, 100.0, options);

  const pluss::Series recovered = run.state.select(-1, 2);
  const pluss::Series major = recovered.ge(20.0);
  const double major_fraction = major.to(pluss::ScalarType::Float64).mean().item<double>();
  const double major_mean = recovered.mul(major.to(pluss::ScalarType::Float64)).sum().item<double>() / major.to(pluss::ScalarType::Float64).sum().item<double>();
  // final-size equation z = 1 - exp(-R0 z) for the large-outbreak attack rate
  double z = 0.5;
  for (int i = 0; i < 200; ++i) z = 1.0 - std::exp(-(beta / gamma) * z);
  out.writeln("completed: ", run.completed ? "yes" : "no");
  out.writeln("fraction of runs with a large outbreak: ", major_fraction, " (branching theory: 1 - gamma/beta = ", 1.0 - gamma / beta, ")");
  out.writeln("mean final size of a large outbreak: ", major_mean, " (deterministic N z = ", n * z, ")");
  out.writeln("runs that fizzled out with fewer than 20 recovered: ", replicates - static_cast<std::int64_t>(major.to(pluss::ScalarType::Float64).sum().item<double>()));
}
```

Output

```text
completed: yes
fraction of runs with a large outbreak: 0.486 (branching theory: 1 - gamma/beta = 0.500)
mean final size of a large outbreak: 156.425 (deterministic N z = 159.362)
runs that fizzled out with fewer than 20 recovered: 2057
```

Of 4000 runs, `0.486` became large outbreaks, close to the predicted `0.500`, and the other `2057` fizzled out. The large outbreaks reached about `156` recovered against the deterministic `159`; the small gap is the finite-population correction. A single deterministic run would give you one number, `159`, and would hide that half the epidemics never take off.

## Reproducing a run and reading the clock

Every random draw in `gillespie` comes from `base + 2 event + 1` and `base + 2 event + 2`, where `base` is `options.seed`. With a fixed seed two calls give identical ensembles; with the default seed of zero the base is taken from the global stream, so each call differs. The program runs a pure-death system, where each of 20 copies dies at rate one, so the survivors at `t = 1` have mean `20/e`. It runs the same seed twice, then a neighbouring seed, and finally asks for `t_end = 0` to show that no event fires.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ssa = pluss::ma::natsci::common::ssa;

// pure death: every copy dies independently at rate 1
static pluss::Series death(const pluss::Series &x) { return x; }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  const pluss::Series nu = pluss::series::make({-1.0}, {1, 1});
  const pluss::Series x0 = pluss::series::full({500, 1}, 20.0, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));

  ssa::SsaOptions fixed;
  fixed.seed = 99;
  const auto a = ssa::gillespie(death, nu, x0, 1.0, fixed);
  const auto b = ssa::gillespie(death, nu, x0, 1.0, fixed);
  out.writeln("same seed, same answer: ", a.state.eq(b.state).all().item<bool>() ? "yes" : "no");

  ssa::SsaOptions other = fixed;
  other.seed = 100;
  const auto c = ssa::gillespie(death, nu, x0, 1.0, other);
  out.writeln("different seed, same answer: ", a.state.eq(c.state).all().item<bool>() ? "yes" : "no");
  out.writeln("survivors at t = 1 (seed 99): mean ", a.state.mean().item<double>(), ", exact 20 / e = ", 20.0 / 2.718281828459045);
  out.writeln("survivors at t = 1 (seed 100): mean ", c.state.mean().item<double>());

  // the state at t = 0 is returned untouched
  const auto zero = ssa::gillespie(death, nu, x0, 0.0, fixed);
  out.writeln("events fired when t_end = 0: ", zero.events.sum().item<double>());
}
```

Output

```text
same seed, same answer: yes
different seed, same answer: no
survivors at t = 1 (seed 99): mean 7.318, exact 20 / e = 7.358
survivors at t = 1 (seed 100): mean 7.342
events fired when t_end = 0: 0.000
```

The two calls with seed `99` agree exactly and the call with seed `100` does not, so `seed` is the only thing that decides the draws. Both ensembles have a mean near `7.36 = 20/e`, so the seed changes the sample, not the law. At `t_end = 0` the loop ends before the first event, so nothing fires.

## Decisions

- Use `gillespie` when counts are small enough that randomness matters: gene expression with a handful of mRNAs, an epidemic seeded by one case, a population close to extinction. When counts are in the thousands the rate equations in the earlier chapters are cheaper and give the same mean.
- Choose the number of replicates for the question you ask. A mean needs a few hundred; a probability near `p` needs about `100 / p` for a relative error of ten percent; a variance needs more than a mean. All replicates run in one call, so a larger ensemble is rarely a reason to change the code.
- Set `options.seed` whenever a result goes into a report or a test. Leave it at zero when you want independent ensembles from repeated calls.
- Raise `options.max_events` when `completed` is `false` and the system really is that busy; look at the propensities first, because a runaway rate that grows with the population is a modelling error and not a budget problem.
- Keep the state in `Float64` for exact counts. A count stored in single precision stops being exact above `16777216`.

## Pitfalls

The three shapes must agree: the state is `[B, S]`, `nu` is `[R, S]`, and the propensity is `[B, R]`. When one of them is wrong, `gillespie` stops with an error that says which. The program feeds it a propensity with one column, a stoichiometry written species-by-reaction, and a single state row.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ssa = pluss::ma::natsci::common::ssa;

int main() {
  OutputEngine out;
  const pluss::Series nu = pluss::series::make({1.0, -1.0}, {2, 1});  // [reactions, species] = [2, 1]
  const pluss::Series x0 = pluss::series::full({4, 1}, 5.0, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  const auto both = [](const pluss::Series &x) { return x.mul(0.0).add(1.0).unite(std::vector<pluss::Series>{x}, -1); };
  try {  // propensity returns one number per replicate instead of one per reaction
    ssa::gillespie([](const pluss::Series &x) { return x.select(-1, 0); }, nu, x0, 5.0);
  } catch (const pluss::Error &e) { out.writeln("propensity [B]: ", e.message()); }
  try {  // stoichiometry written species-by-reaction
    ssa::gillespie(both, nu.transpose(0, 1), x0, 5.0);
  } catch (const pluss::Error &e) { out.writeln("stoichiometry [S, R]: ", e.message()); }
  try {  // the initial state is one row, not a [replicates, species] matrix
    ssa::gillespie(both, nu, x0.select(0, 0), 5.0);
  } catch (const pluss::Error &e) { out.writeln("rank-1 state: ", e.message()); }
}
```

Output

```text
propensity [B]:  natsci: gillespie: propensity must return [replicates, reactions]
stoichiometry [S, R]:  natsci: gillespie: stoichiometry must be [reactions, species]
rank-1 state:  natsci: gillespie: state must be [replicates, species]
```

Each mistake is named in the message. A propensity of shape `[B]` is the easiest to produce, because selecting one species column looks natural; the rate of each reaction needs its own column. The stoichiometry is the other common slip: write it as one row per reaction.

A propensity is a rate, so it must be finite and non-negative in every replicate. A death rate typed with the wrong sign, a rate that divides by a count that can reach zero, or a negative starting count each produce an error rather than a wrong ensemble.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ssa = pluss::ma::natsci::common::ssa;

int main() {
  OutputEngine out;
  const pluss::Series nu = pluss::series::make({1.0, -1.0}, {2, 1});
  const pluss::Series x0 = pluss::series::full({4, 1}, 5.0, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  try {  // a death rate with the wrong sign
    ssa::gillespie([](const pluss::Series &x) { return x.mul(0.0).add(1.0).unite(std::vector<pluss::Series>{x.mul(-0.5)}, -1); }, nu, x0, 5.0);
  } catch (const pluss::Error &e) { out.writeln("negative rate: ", e.message()); }
  try {  // a rate that divides by zero when the population is zero
    ssa::gillespie([](const pluss::Series &x) { return x.mul(0.0).add(1.0).unite(std::vector<pluss::Series>{x.reciprocal()}, -1); }, nu, x0.mul(0.0), 5.0);
  } catch (const pluss::Error &e) { out.writeln("infinite rate: ", e.message()); }
  try {  // a negative initial count
    ssa::gillespie([](const pluss::Series &x) { return x.mul(0.0).add(1.0).unite(std::vector<pluss::Series>{x}, -1); }, nu, x0.neg(), 5.0);
  } catch (const pluss::Error &e) { out.writeln("negative count: ", e.message()); }
}
```

Output

```text
negative rate:  natsci: gillespie: propensities must be finite and non-negative
infinite rate:  natsci: gillespie: propensities must be finite and non-negative
negative count:  natsci: gillespie: initial state must be non-negative
```

The sign error and the division by zero are caught while the loop runs, and the negative count is caught before it starts. A rate such as `1/x` needs a guard that returns zero when `x` is zero; replacing `x` with `max(x, 1)` in the denominator is often what the model means.

`completed` tells you whether every replicate reached `t_end`. With a small `max_events` the loop stops early and the returned state is the state after that many events, which is not the state at `t_end`. The program runs the same immigration-death model to `t = 50` with a budget of 8 events and with a budget of one million.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ssa = pluss::ma::natsci::common::ssa;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2).with_sep(""));
  const pluss::Series nu = pluss::series::make({1.0, -1.0}, {2, 1});
  const pluss::Series x0 = pluss::series::full({4, 1}, 5.0, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  const auto propensity = [](const pluss::Series &x) { return x.mul(0.0).add(5.0).unite(std::vector<pluss::Series>{x.mul(0.5)}, -1); };
  for (std::int64_t budget : {8, 1000000}) {
    ssa::SsaOptions options;
    options.max_events = budget;
    options.seed = 3;
    const auto run = ssa::gillespie(propensity, nu, x0, 50.0, options);
    out.writeln("max_events = ", budget, ": completed ", run.completed ? "yes" : "no", ", most events in one run ", run.events.max().item<double>(), ", mean copies ", run.state.mean().item<double>());
  }
}
```

Output

```text
max_events = 8: completed no, most events in one run 8.00, mean copies 8.50
max_events = 1000000: completed yes, most events in one run 517.00, mean copies 9.25
```

With a budget of 8 the run reports `completed no`, and the mean of `8.50` is the state after eight events, not the stationary value. With the large budget it reports `completed yes` and the busiest replicate used 517 events. Always check `completed` before you use the state.

## Where next

[Methods 5](../meth_05_population_balances.md) covers population balances, which describe a continuous distribution of sizes instead of a handful of counts. The immigration-death model here is the process analysed exactly in [extinction risk](../../biology/bio_03_demography/extinction.md); the epidemic is the stochastic counterpart of the [SIR model](../../biology/bio_06_epidemics/sir_model.md). The reference pages for `natsci::common::ssa::gillespie`, `SsaOptions` and `SsaResult` list every field.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
