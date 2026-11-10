<!-- generated from AggregateMoments.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::losses::AggregateMoments`

The mean and variance of a loss quantity: a claim count, a claim size or an aggregate.

## Signature

```cpp
actuarial::losses::AggregateMoments actuarial::losses::aggregate_moments(const pluss::Series &frequency_mean, const pluss::Series &frequency_variance, const pluss::Series &severity_mean, const pluss::Series &severity_variance);
pluss::Series actuarial::losses::AggregateMoments::mean;
pluss::Series actuarial::losses::AggregateMoments::variance;
```

Every moment function in this family returns an `AggregateMoments`: the frequency functions ([`poisson_moments`](poisson_moments.md), [`negbin_moments`](negbin_moments.md)), the severity functions ([`lognormal_moments`](lognormal_moments.md), [`gamma_moments`](gamma_moments.md)) and the aggregates ([`aggregate_moments`](aggregate_moments.md), [`compound_poisson`](compound_poisson.md), [`compound_negbin`](compound_negbin.md)). The name is the same whatever the quantity: it is simply a mean and a variance, which is why the output of a frequency function and of a severity function can be passed to [`aggregate_moments`](aggregate_moments.md) field by field. The scalar functions return rank-zero series; [`aggregate_moments`](aggregate_moments.md) returns series of the broadcast shape of its inputs.

## Fields

### `mean`

`pluss::Series`. The expected value. For a count it is the expected number of claims, for a severity the expected claim size, for an aggregate the expected total loss.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
namespace losses = actuarial::losses;
namespace models = actuarial::models;
namespace distr = pluss::subsp::stats::distr;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  out.writeln("count    =", actuarial::losses::poisson_moments(2.0).mean.item<double>());
  out.writeln("severity =", actuarial::losses::gamma_moments(2.0, 3.0).mean.item<double>());
  out.writeln("total    =", actuarial::losses::compound_poisson(2.0, 6.0, 18.0).mean.item<double>());
}
```

Output

```text
count    = 2.000000
severity = 6.000000
total    = 12.000000
```


### `variance`

`pluss::Series`. The variance of the same quantity. Take its square root for the standard deviation; the coefficient of variation is that over the mean.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
namespace losses = actuarial::losses;
namespace models = actuarial::models;
namespace distr = pluss::subsp::stats::distr;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto m = actuarial::losses::compound_poisson(2.0, 6.0, 18.0);
  out.writeln("variance =", m.variance.item<double>(), " standard deviation =", std::sqrt(m.variance.item<double>()), " cv =", std::sqrt(m.variance.item<double>()) / m.mean.item<double>());
}
```

Output

```text
variance = 108.000000  standard deviation = 10.392305  cv = 0.866025
```


## Practical Examples

### Example 1: from moments to an aggregate

The frequency and severity moments are passed to `aggregate_moments` field by field.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
namespace losses = actuarial::losses;
namespace models = actuarial::models;
namespace distr = pluss::subsp::stats::distr;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2));
  const auto n = actuarial::losses::negbin_moments(3.0, 0.4);
  const auto x = actuarial::losses::lognormal_moments(5.0, 0.8);
  const auto s = actuarial::losses::aggregate_moments(n.mean, n.variance, x.mean, x.variance);
  out.writeln("count mean", n.mean.item<double>(), " claim mean", x.mean.item<double>(), " total mean", s.mean.item<double>());
  out.writeln("total standard deviation", std::sqrt(s.variance.item<double>()));
}
```

Output

```text
count mean 4.50  claim mean 204.38  total mean 919.73
total standard deviation 799.04
```

### Example 2: a safety loading from the moments

A premium that exceeds the expected loss by a fraction of its standard deviation.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
namespace losses = actuarial::losses;
namespace models = actuarial::models;
namespace distr = pluss::subsp::stats::distr;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(1));
  const auto s = actuarial::losses::compound_poisson(100.0, 500.0, 250000.0);
  const double sd = std::sqrt(s.variance.item<double>());
  out.writeln("expected loss", s.mean.item<double>(), " standard deviation", sd, " premium at 0.5 sd", s.mean.item<double>() + 0.5 * sd);
}
```

Output

```text
expected loss 50000.0  standard deviation 7071.1  premium at 0.5 sd 53535.5
```

### Example 3: an undefined value

A default-constructed value has undefined fields.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
namespace losses = actuarial::losses;
namespace models = actuarial::models;
namespace distr = pluss::subsp::stats::distr;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const actuarial::losses::AggregateMoments none;
  out.writeln(none.mean.defined(), none.variance.defined());
}
```

Output

```text
false false
```

## Changelog

- **0.1**: initial operator-first actuarial surface.

## See Also

[`aggregate_moments`](aggregate_moments.md) combines a frequency and a severity, and the moment functions of this family produce the inputs.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="poisson_moments.html">
    <span class="label">Next</span>
    <span class="title">poisson_moments</span>
  </a>
</div>
<!-- apiref:nav:end -->
