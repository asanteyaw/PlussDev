<!-- generated from a_policy_from_frequency_severity_and_terms.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# A Policy from Frequency, Severity and Terms

[Back to Frequency, Severity and Insurance Layers](../actuarial_03_frequency_severity_and_insurance_layers.md)

The pieces of this chapter combine into the expected annual cost of a policy. A policy has a claim frequency, a severity law for the ground-up loss, and contract terms: a deductible {math}`d`, a limit {math}`u` on what is paid after the deductible, and a coinsurance share {math}`\alpha`. The insurer pays {math}`\alpha\,\min\bigl((X-d)_+,u\bigr)` on each loss, and with independent frequency and severity

```{math}
E[\text{annual cost}]=E[N]\;\alpha\;E\bigl[\min\bigl((X-d)_+,u\bigr)\bigr]
=E[N]\;\alpha\;\bigl(\mathrm{LEV}(d+u)-\mathrm{LEV}(d)\bigr).
```

The expected annual cost is the **pure premium** of the policy: the premium before expenses and margins. It is linear in the frequency and in the share, and a nonlinear function of the deductible and the limit through the severity law.

This page works one policy through, checks the parametric answer against a sample, and shows how the terms change the premium.

## The expected cost of a policy

The policy expects 0.12 claims a year. A claim has a lognormal severity with `mean_log = 8` and `stddev_log = 1`, a mean of 4,914.77. The policy has a deductible of 500, pays at most 25,000 after the deductible and shares 80%. The program computes the expected cost per claim and per year from the parametric layer, the premium without any terms, and the share of the ground-up cost the terms leave to the insurer.

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
  const double lambda = 0.12, deductible = 500.0, limit = 25000.0, share = 0.8;
  const double mean = losses::lognormal_moments(8.0, 1.0).mean.item<double>();
  const double layer = losses::lognormal_layer(8.0, 1.0, deductible, limit).item<double>();
  out.writeln("ground-up cost per claim            ", mean, " per year", lambda * mean);
  out.writeln("expected layer payment per claim    ", layer);
  out.writeln("insurer's cost per claim (80% share)", share * layer, " per year (pure premium)", lambda * share * layer);
  out.writeln("share of the ground-up cost retained by the insurer", share * layer / mean);
}
```

Output

```text
ground-up cost per claim             4914.768840  per year 589.772261
expected layer payment per claim     4207.803262
insurer's cost per claim (80% share) 3366.242610  per year (pure premium) 403.949113
share of the ground-up cost retained by the insurer 0.684924
```

## Checking against a sample

The same cost from a sample of 400,000 claims drawn from the severity law: the loss transforms are applied in the order of the contract and averaged. The sample cost and the parametric cost agree to the sampling error.

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
  const double lambda = 0.12;
  const distr::Lognormal law{8.0, 1.0};
  const pluss::Series claims = law.sample({400000}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 17ULL);
  const auto mean = actuarial::operators::mean();
  const pluss::Series paid = actuarial::operators::coinsurance(0.8)(actuarial::operators::limit(25000.0)(actuarial::operators::deductible(500.0)(claims)));
  out.writeln("sample: expected payment per claim", mean(paid).item<double>(), " per year", lambda * mean(paid).item<double>());
  out.writeln("law   : expected payment per claim", 0.8 * losses::lognormal_layer(8.0, 1.0, 500.0, 25000.0).item<double>(), " per year", lambda * 0.8 * losses::lognormal_layer(8.0, 1.0, 500.0, 25000.0).item<double>());
}
```

Output

```text
sample: expected payment per claim 3363.859460  per year 403.663135
law   : expected payment per claim 3366.242610  per year 403.949113
```

## What each term does to the premium

The pure premium of the same policy as the deductible rises and as the limit falls, with the share at 100% so that only the terms act. A larger deductible removes more of the small and frequent losses; a smaller limit removes the large and rare ones, and the large losses of a long-tailed severity are a much bigger part of the mean than their number suggests: a limit of 5,000 alone removes 38% of the expected cost.

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
  out.print_defaults(PrintOptions{}.fixed(2));
  const double lambda = 0.12;
  out.writeln("no terms                              ", lambda * losses::lognormal_moments(8.0, 1.0).mean.item<double>());
  for (const double d : {250.0, 1000.0, 5000.0}) {
    out.writeln("deductible", d, ", no limit             ", lambda * (losses::lognormal_moments(8.0, 1.0).mean.item<double>() - losses::lognormal_lev(8.0, 1.0, d).item<double>()));
  }
  for (const double u : {50000.0, 20000.0, 5000.0}) {
    out.writeln("limit", u, ", no deductible          ", lambda * losses::lognormal_lev(8.0, 1.0, u).item<double>());
  }
}
```

Output

```text
no terms                               589.77
deductible 250.00 , no limit              559.82
deductible 1000.00 , no limit              475.52
deductible 5000.00 , no limit              222.71
limit 50000.00 , no deductible           583.90
limit 20000.00 , no deductible           550.14
limit 5000.00 , no deductible           367.06
```

## Frequency uncertainty and the variance of the cost

The expected cost needs only the means, but the variability of the annual cost depends on the frequency law, as the previous pages show. For the policy of the first example the ground-up annual cost has a mean and a variance that follow from the claim frequency and severity: a Poisson count against a negative binomial of the same mean differ in the variance of the aggregate and not in the premium.

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
  out.print_defaults(PrintOptions{}.fixed(2));
  const losses::AggregateMoments severity = losses::lognormal_moments(8.0, 1.0);
  const double mean = severity.mean.item<double>(), variance = severity.variance.item<double>();
  const auto poisson = losses::compound_poisson(0.12, mean, variance);
  const auto negbin = losses::compound_negbin(1.0, 1.0 / 1.12, mean, variance);   // mean 0.12 claims, variance 0.1344
  out.writeln("Poisson            expected cost", poisson.mean.item<double>(), " standard deviation", std::sqrt(poisson.variance.item<double>()));
  out.writeln("negative binomial  expected cost", negbin.mean.item<double>(), " standard deviation", std::sqrt(negbin.variance.item<double>()));
}
```

Output

```text
Poisson            expected cost 589.77  standard deviation 2806.99
negative binomial  expected cost 589.77  standard deviation 2868.28
```

## Where to go next

Experience, credibility and the reserves for losses that have already occurred turn these expected costs into rates; they are the subject of Chapter 4.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="limited_expected_values_and_parametric_layers.html">
    <span class="label">Previous</span>
    <span class="title">Limited Expected Values and Parametric Layers</span>
  </a>
</div>
<!-- apiref:nav:end -->
