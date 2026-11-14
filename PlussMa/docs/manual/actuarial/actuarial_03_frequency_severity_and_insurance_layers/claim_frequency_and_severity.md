<!-- generated from claim_frequency_and_severity.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Claim Frequency and Severity

[Back to Frequency, Severity and Insurance Layers](../actuarial_03_frequency_severity_and_insurance_layers.md)

Insurance losses have two sources of randomness. **How many** claims arrive in a period is the *frequency* `N`, a count; **how large** each one is is the *severity* `X`, a positive amount. Keeping them apart is the foundation of the chapter, because frequency is driven by exposure and severity by the nature of the loss, and because a contract term such as a deductible acts on severity alone.

This page describes the standard laws for each and the numbers that summarise them: the mean and variance, and the probability mass function of a count.

## Frequency: Poisson and negative binomial

A **Poisson** count with rate {math}`\lambda` has equal mean and variance,

```{math}
E[N]=\mathrm{Var}[N]=\lambda,\qquad \Pr(N=k)=e^{-\lambda}\frac{\lambda^k}{k!}.
```

Real claim counts are usually more dispersed than that, because the risk varies from policy to policy: a portfolio is a mixture of rates. A **negative binomial** count captures it. With the parameterisation used here, the number of failures before the {math}`r`-th success when each trial succeeds with probability {math}`p`,

```{math}
E[N]=\frac{r(1-p)}{p},\qquad \mathrm{Var}[N]=\frac{r(1-p)}{p^2}=\frac{E[N]}{p}>E[N].
```

The variance exceeds the mean by the factor {math}`1/p`, which is the overdispersion.

## Severity: lognormal and gamma

A **lognormal** severity with parameters {math}`\mu` and {math}`\sigma` (the mean and standard deviation of the logarithm of the loss) has a long right tail,

```{math}
E[X]=e^{\mu+\sigma^2/2},\qquad \mathrm{Var}[X]=\left(e^{\sigma^2}-1\right)e^{2\mu+\sigma^2}.
```

A **gamma** severity with shape {math}`a`, scale {math}`\theta` and an optional location {math}`c` (a minimum loss) has

```{math}
E[X]=a\theta+c,\qquad \mathrm{Var}[X]=a\theta^2.
```

The location shifts every loss and so moves the mean and leaves the variance alone.

## The functions

```cpp
losses::AggregateMoments losses::poisson_moments(double lambda, pluss::Platform platform = pluss::Platform::CPU);
losses::AggregateMoments losses::negbin_moments(double r, double p, pluss::Platform platform = pluss::Platform::CPU);
losses::AggregateMoments losses::lognormal_moments(double mean_log, double stddev_log, pluss::Platform platform = pluss::Platform::CPU);
losses::AggregateMoments losses::gamma_moments(double shape, double scale, double location = 0.0, pluss::Platform platform = pluss::Platform::CPU);
pluss::Series losses::poisson_pmf(double lambda, std::int64_t max_count, pluss::Platform platform = pluss::Platform::CPU);
pluss::Series losses::negbin_pmf(double r, double p, std::int64_t max_count, pluss::Platform platform = pluss::Platform::CPU);
```

| Function | Returns | Domain |
|---|---|---|
| `poisson_moments(lambda)` | mean and variance of the count | `lambda` ≥ 0 |
| `negbin_moments(r, p)` | mean and variance of the count | `p` in (0, 1) |
| `lognormal_moments(mean_log, stddev_log)` | mean and variance of the loss | |
| `gamma_moments(shape, scale, location)` | mean and variance of the loss | `shape` > 0 |
| `poisson_pmf(lambda, max_count)` | the masses at `0..max_count` | `max_count` ≥ 0 |
| `negbin_pmf(r, p, max_count)` | the masses at `0..max_count` | `p` in (0, 1) |

The moment functions return an `AggregateMoments`, a struct of two rank-zero `Series`, `mean` and `variance`. The same struct carries the aggregate moments of the next page.

## The moments of the four laws

The program prints the moments and checks the lognormal against its formula.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
namespace losses = actuarial::losses;
namespace distr = pluss::subsp::stats::distr;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto show = [&](const char *name, const losses::AggregateMoments &m) {
    out.writeln(name, "mean", m.mean.item<double>(), " variance", m.variance.item<double>());
  };
  show("Poisson(2.5)               ", losses::poisson_moments(2.5));
  show("negative binomial(3, 0.5)  ", losses::negbin_moments(3.0, 0.5));
  show("lognormal(8, 1)            ", losses::lognormal_moments(8.0, 1.0));
  show("gamma(2, 3000)             ", losses::gamma_moments(2.0, 3000.0));
  show("gamma(2, 3000) from 500    ", losses::gamma_moments(2.0, 3000.0, 500.0));
  out.writeln("lognormal by the formulas:", std::exp(8.5), (std::exp(1.0) - 1.0) * std::exp(17.0));
}
```

Output

```text
Poisson(2.5)                mean 2.500000  variance 2.500000
negative binomial(3, 0.5)   mean 3.000000  variance 6.000000
lognormal(8, 1)             mean 4914.768840  variance 41505016.383755
gamma(2, 3000)              mean 6000.000000  variance 18000000.000000
gamma(2, 3000) from 500     mean 6500.000000  variance 18000000.000000
lognormal by the formulas: 4914.768840 41505016.383755
```

The negative binomial has mean 3 and variance 6, twice a Poisson with the same mean, as `1/p = 2` says. The gamma shifted by a location of 500 has mean 6,500 against 6,000 and the same variance of 18 million.

## The mass functions

`poisson_pmf` and `negbin_pmf` return the probabilities of the counts `0, 1, ..., max_count`. They are truncated: the masses sum to less than one by the probability of a larger count, and the mean read off them is a little below the true mean for the same reason.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
namespace losses = actuarial::losses;
namespace distr = pluss::subsp::stats::distr;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const pluss::Series poisson = losses::poisson_pmf(2.5, 10);
  const pluss::Series negbin = losses::negbin_pmf(3.0, 0.5, 10);
  std::vector<double> counts(11);
  for (int k = 0; k <= 10; ++k) counts[k] = k;
  const pluss::Series k = pluss::make_series(counts);
  out.writeln("Poisson masses ", poisson.items<double>());
  out.writeln("negbin masses  ", negbin.items<double>());
  out.writeln("total mass      Poisson", poisson.sum().item<double>(), " negbin", negbin.sum().item<double>());
  out.writeln("mean from masses Poisson", poisson.mul(k).sum().item<double>(), " negbin", negbin.mul(k).sum().item<double>());
}
```

Output

```text
Poisson masses  [0.082085, 0.205212, 0.256516, 0.213763, 0.133602, 0.066801, 0.027834, 0.009941, 0.003106, 0.000863, 0.000216]
negbin masses   [0.125000, 0.187500, 0.187500, 0.156250, 0.117188, 0.082031, 0.054687, 0.035156, 0.021973, 0.013428, 0.008057]
total mass      Poisson 0.999938  negbin 0.988770
mean from masses Poisson 2.499307  negbin 2.861572
```

## Overdispersion in the tail

A Poisson with the same mean puts almost no mass on large counts, and the negative binomial puts much more. The program compares the probability of eight or more claims for a Poisson with mean 3 and a negative binomial with the same mean, computed as one minus the masses below eight.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
namespace losses = actuarial::losses;
namespace distr = pluss::subsp::stats::distr;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const pluss::Series poisson = losses::poisson_pmf(3.0, 7);
  const pluss::Series negbin = losses::negbin_pmf(3.0, 0.5, 7);
  out.writeln("means          Poisson", losses::poisson_moments(3.0).mean.item<double>(), " negbin", losses::negbin_moments(3.0, 0.5).mean.item<double>());
  out.writeln("variances      Poisson", losses::poisson_moments(3.0).variance.item<double>(), " negbin", losses::negbin_moments(3.0, 0.5).variance.item<double>());
  out.writeln("P(8 or more)   Poisson", 1.0 - poisson.sum().item<double>(), " negbin", 1.0 - negbin.sum().item<double>());
}
```

Output

```text
means          Poisson 3.000000  negbin 3.000000
variances      Poisson 3.000000  negbin 6.000000
P(8 or more)   Poisson 0.011905  negbin 0.054688
```

## Invalid parameters fail explicitly

The functions check their parameters before computing: a negative Poisson rate, a negative binomial success probability outside the open interval (0, 1), a gamma shape that is not positive, and a negative maximum count are refused.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
namespace losses = actuarial::losses;
namespace distr = pluss::subsp::stats::distr;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const std::exception &e) {
      std::string text(e.what());
      if (const auto cut = text.find("Triggered"); cut != std::string::npos) text.erase(cut);
      while (!text.empty() && text.back() == '\n') text.pop_back();
      out.writeln(text);
    }
  };
  report([&] { (void)losses::poisson_moments(-1.0); });
  report([&] { (void)losses::negbin_moments(3.0, 0.0); });
  report([&] { (void)losses::negbin_moments(3.0, 1.2); });
  report([&] { (void)losses::gamma_moments(0.0, 3000.0); });
  report([&] { (void)losses::poisson_pmf(2.0, -1); });
}
```

Output

```text
poisson_moments: lambda must be >= 0
negbin_moments: p must lie in (0,1)
negbin_moments: p must lie in (0,1)
gamma_moments: shape must be > 0
poisson_pmf: max_count must be >= 0
```

## Where to go next

Frequency and severity combine into the total loss of a period ([Aggregate Losses](aggregate_losses.md)), and a contract changes the severity before it is combined ([Deductibles, Limits and Coinsurance](deductibles_limits_and_coinsurance.md)).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="aggregate_losses.html">
    <span class="label">Next</span>
    <span class="title">Aggregate Losses</span>
  </a>
</div>
<!-- apiref:nav:end -->
