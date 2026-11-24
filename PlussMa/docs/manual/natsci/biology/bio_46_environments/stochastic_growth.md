# The stochastic growth rate

[Back to Biology 46: Random Environments](../bio_46_environments.md)

A population in a variable environment does not grow at the rate of its average year. You want to know the long-run growth rate when good and bad years alternate at random, whether it matters if bad years come in runs, and how likely the population is to fall below a safe size. This page computes the stochastic growth rate of a stage-structured population with a small-noise formula and by simulation, compares independent with persistent environments, and estimates the risk of decline from simulated paths.

## How it works

The code is in `pluss::ma::natsci::biology::population::demography` (`demography::` below). The environment is a set of projection matrices $A_1, \dots, A_K$ with probabilities $p_k$, collected in `demography::Environment` (fields `matrices` of shape $[K, s, s]$, `prob` of shape $[K]$, and optionally `transition`, a row-stochastic $K\times K$ matrix that makes the sequence of years a Markov chain). The long-run growth of $\ln N_t$ per year is the stochastic growth rate,

```{math}
\ln\lambda_s = \lim_{T\to\infty}\frac1T\ln\|A_{k_T}\cdots A_{k_1}n_0\|
```

which is always below the log of the dominant eigenvalue of the average matrix $\bar A$ (a randomly varying environment hurts). For small variation Tuljapurkar's approximation is

```{math}
\ln\lambda_s \approx \ln\lambda(\bar A) - \frac{\tau^2}{2\lambda^2},\qquad \tau^2 = \sum_k p_k\,\langle S, A_k - \bar A\rangle^2
```

where $S$ is the sensitivity matrix of $\bar A$. `demography::tuljapurkar(matrices, prob)` returns $\ln\lambda(\bar A)$, $\tau^2$ and the approximation. `demography::simulate_growth(env, steps, replicates, seed)` estimates $\ln\lambda_s$ directly, with a standard error from the spread of the replicates, and `demography::simulate_paths(env, n0, steps, replicates, seed)` returns the total population size of each replicate year by year. `demography::quasi_extinction_probability(paths, threshold)` turns those paths into the probability that the population has fallen to the threshold by each year.

## The growth rate of a variable environment

A two-stage population (juveniles, adults) has a good year and a bad year, equally likely and independent. The program compares the growth rate of the average matrix, the small-noise approximation and a simulation of `400` replicates of `2000` years.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace biology = natsci::biology;
namespace demography = biology::population::demography;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series cube(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::make_series(v, shape).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
// two environments for a two-stage population (juveniles, adults): a good year and a bad year
static demography::Environment years(const std::vector<double> &prob, bool markov, double stay) {
  const auto matrices = cube({0.3, 2.0, 0.4, 0.8,     0.2, 0.5, 0.3, 0.6}, {2, 2, 2});
  demography::Environment env;
  env.matrices = matrices;
  env.prob = vec(prob);
  if (markov) env.transition = cube({stay, 1.0 - stay, 1.0 - stay, stay}, {2, 2});
  return env;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto env = years({0.5, 0.5}, false, 0.0);
  const auto mean = env.matrices.mul(env.prob.unsqueeze(-1).unsqueeze(-1)).sum(0, false);
  const auto approx = demography::tuljapurkar(env.matrices, env.prob);
  out.writeln("ln lambda of the average matrix        = ", flat(approx.log_lambda_mean)[0]);
  out.writeln("variance term tau^2                    = ", flat(approx.tau2)[0]);
  out.writeln("small-noise ln lambda_s (Tuljapurkar)  = ", flat(approx.log_lambda_s)[0]);
  const auto sim = demography::simulate_growth(env, 2000, 400, 7);
  out.writeln("simulated ln lambda_s                  = ", sim.mean, " +- ", sim.standard_error, " (400 replicates of 2000 years)");
  out.writeln("growth rate of the good year alone     = ", flat(demography::spectral_radius(env.matrices.select(0, 0)))[0], ", bad year alone = ", flat(demography::spectral_radius(env.matrices.select(0, 1)))[0]);
}
```

Output

```text
ln lambda of the average matrix        = 0.1601
variance term tau^2                    = 0.0996
small-noise ln lambda_s (Tuljapurkar)  = 0.1240
simulated ln lambda_s                  = 0.1216 +- 0.0003 (400 replicates of 2000 years)
growth rate of the good year alone     = 1.4787, bad year alone = 0.8359
```

The average matrix would grow at $\ln\lambda = 0.1601$ per year, but the variance term $\tau^2 = 0.0996$ reduces this to `0.1240` by the approximation, and the simulation gives `0.1216 +- 0.0003`. The good year alone has $\lambda = 1.4787$ and the bad year alone $0.8359$: the population grows in the long run, but more slowly than the average year suggests.

## Runs of bad years

The same two kinds of year, each half the time, can come independently or in runs. The program makes the next year the same kind as this one with probability `0.1` (they alternate), `0.5` (independent) and `0.9` (long runs).

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace biology = natsci::biology;
namespace demography = biology::population::demography;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series cube(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::make_series(v, shape).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
// two environments for a two-stage population (juveniles, adults): a good year and a bad year
static demography::Environment years(const std::vector<double> &prob, bool markov, double stay) {
  const auto matrices = cube({0.3, 2.0, 0.4, 0.8,     0.2, 0.5, 0.3, 0.6}, {2, 2, 2});
  demography::Environment env;
  env.matrices = matrices;
  env.prob = vec(prob);
  if (markov) env.transition = cube({stay, 1.0 - stay, 1.0 - stay, stay}, {2, 2});
  return env;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // good and bad years each occur half the time; what changes is whether a year tends to be followed by the same kind of year
  for (double stay : {0.1, 0.5, 0.9}) {
    const auto env = years({0.5, 0.5}, true, stay);
    const auto sim = demography::simulate_growth(env, 2000, 400, 11);
    out.writeln("probability the next year is the same kind = ", stay, ": ln lambda_s = ", sim.mean, " +- ", sim.standard_error);
  }
}
```

Output

```text
probability the next year is the same kind = 0.1000: ln lambda_s = 0.1371 +- 0.0001
probability the next year is the same kind = 0.5000: ln lambda_s = 0.1216 +- 0.0003
probability the next year is the same kind = 0.9000: ln lambda_s = 0.1090 +- 0.0009
```

The growth rate falls as the years become more persistent: `0.1371` when good and bad years alternate, `0.1216` when they are independent and `0.1090` when they come in long runs. In a run of bad years the population shrinks for several years in a row and the good years cannot make up the loss, even though the average year is the same.

## The risk of decline

A population starts with `10` juveniles and `10` adults. The program simulates `2000` populations for `50` years, and reports the probability that the total has fallen to `5` or fewer by each year and the spread of the sizes at the end.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace biology = natsci::biology;
namespace demography = biology::population::demography;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series cube(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::make_series(v, shape).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
// two environments for a two-stage population (juveniles, adults): a good year and a bad year
static demography::Environment years(const std::vector<double> &prob, bool markov, double stay) {
  const auto matrices = cube({0.3, 2.0, 0.4, 0.8,     0.2, 0.5, 0.3, 0.6}, {2, 2, 2});
  demography::Environment env;
  env.matrices = matrices;
  env.prob = vec(prob);
  if (markov) env.transition = cube({stay, 1.0 - stay, 1.0 - stay, stay}, {2, 2});
  return env;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // risk: simulate 2000 populations from 10 juveniles and 10 adults and ask how many have dropped to 5 animals or fewer by year t
  const auto env = years({0.5, 0.5}, false, 0.0);
  const auto paths = demography::simulate_paths(env, vec({10.0, 10.0}), 50, 2000, 3);
  const auto risk = demography::quasi_extinction_probability(paths, 5.0);
  for (int t : {0, 10, 20, 30, 40, 50}) out.writeln("year ", t, ": probability of having fallen to 5 or fewer = ", flat(risk)[t]);
  const auto last = flat(paths.select(1, 50));
  std::vector<double> sorted = last; std::sort(sorted.begin(), sorted.end());
  out.writeln("total size in year 50: median ", sorted[sorted.size() / 2], ", 10th percentile ", sorted[sorted.size() / 10], ", 90th percentile ", sorted[9 * sorted.size() / 10]);
}
```

Output

```text
year 0: probability of having fallen to 5 or fewer = 0.0000
year 10: probability of having fallen to 5 or fewer = 0.0040
year 20: probability of having fallen to 5 or fewer = 0.0080
year 30: probability of having fallen to 5 or fewer = 0.0100
year 40: probability of having fallen to 5 or fewer = 0.0110
year 50: probability of having fallen to 5 or fewer = 0.0110
total size in year 50: median 9470.0684, 10th percentile 604.3032, 90th percentile 150632.2572
```

The probability of having fallen to five or fewer rises from `0.0000` to `0.0110` by year `30` and then stops rising: a population that has survived the early years is large enough to be safe. At year `50` the median size is `9470`, but the 10th percentile is `604` and the 90th is `150632`: the outcomes are spread over more than two orders of magnitude, which is the effect of multiplicative growth.

## Decisions

- Use `tuljapurkar` when you need a quick answer or the derivative with respect to a parameter, and `simulate_growth` when the years differ strongly or are autocorrelated, which the approximation does not allow for.
- Give `Environment` a `transition` matrix when the years are correlated, and say so when you quote a growth rate: the same set of years gives different growth rates for different persistence.
- Report the risk from `simulate_paths` with enough replicates for the probability you want to resolve; a risk of one per cent needs at least a few thousand.

## Pitfalls

The small-noise formula assumes the years differ little. The program makes the good and bad years differ more and more (the contrast `0.2`, `1.0`, `3.0` scales the difference between the two matrices) and compares the formula with a simulation each time.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace biology = natsci::biology;
namespace demography = biology::population::demography;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series cube(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::make_series(v, shape).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
// two environments for a two-stage population (juveniles, adults): a good year and a bad year
static demography::Environment years(const std::vector<double> &prob, bool markov, double stay) {
  const auto matrices = cube({0.3, 2.0, 0.4, 0.8,     0.2, 0.5, 0.3, 0.6}, {2, 2, 2});
  demography::Environment env;
  env.matrices = matrices;
  env.prob = vec(prob);
  if (markov) env.transition = cube({stay, 1.0 - stay, 1.0 - stay, stay}, {2, 2});
  return env;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the small-noise formula is only exact for years that differ little; make the two years differ strongly and compare with simulation
  for (double gap : {0.2, 1.0, 3.0}) {
    const auto matrices = cube({0.5 + gap * 0.3, 1.4 + gap * 0.6, 0.45, 0.7,     std::max(0.0, 0.5 - gap * 0.3), std::max(0.0, 1.4 - gap * 0.6), 0.45, 0.7}, {2, 2, 2});
    demography::Environment env; env.matrices = matrices; env.prob = vec({0.5, 0.5});
    const auto approx = demography::tuljapurkar(matrices, env.prob);
    const auto sim = demography::simulate_growth(env, 2000, 400, 5);
    out.writeln("year-to-year contrast ", gap, ": small-noise ", flat(approx.log_lambda_s)[0], ", simulated ", sim.mean, " +- ", sim.standard_error);
  }
}
```

Output

```text
year-to-year contrast 0.2000: small-noise 0.3356, simulated 0.3356 +- 0.0000
year-to-year contrast 1.0000: small-noise 0.3135, simulated 0.3131 +- 0.0002
year-to-year contrast 3.0000: small-noise 0.3123, simulated 0.2820 +- 0.0007
```

For a contrast of `0.2` the formula and the simulation agree to four digits (`0.3356`). By `1.0` they differ by `0.0004`, and at `3.0` the formula gives `0.3123` while the simulation gives `0.2820`: when the years differ strongly the approximation overstates the growth rate by about ten per cent. Check it against `simulate_growth` before you rely on it.

## Where next

[Biology 3: Demography](../bio_03_demography.md) introduces projection matrices and extinction, [Biology 23: Census Data](../bio_23_census.md) fits a population model to counts, and [Biology 40: Discrete-Time Maps and Chaos](../bio_40_maps.md) treats the one-species case.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
