<!-- generated from negbin_moments.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::losses::negbin_moments`

Mean and variance of a negative binomial claim count.

## Signature

```cpp
actuarial::losses::AggregateMoments negbin_moments(double r, double p, pluss::Platform platform = pluss::Platform::CPU);
```

## What It Does

The negative binomial count is the number of failures before the `r`-th success when each trial succeeds with probability `p`. Its mean is {math}`r(1-p)/p` and its variance {math}`r(1-p)/p^2`, which exceeds the mean by the factor {math}`1/p`: it is the standard frequency model when claim counts are more variable than Poisson, for instance when the rate itself varies between risks. As `p` approaches 1 the variance approaches the mean. Use it as the frequency of [`aggregate_moments`](aggregate_moments.md), or call [`compound_negbin`](compound_negbin.md) to do both steps.

## Parameters

`r`
: The number of successes, finite and greater than 0 (it need not be an integer).

`p`
: The success probability, strictly between 0 and 1.

`platform`
: Where the result is placed. Default `pluss::Platform::CPU`.

## Returns

An [`AggregateMoments`](AggregateMoments.md) of two rank-zero series.

## Errors

Throws `negbin_moments: r must be finite`, `negbin_moments: r must be > 0`, `negbin_moments: p must be finite` and `negbin_moments: p must lie in (0,1)`.

## Practical Examples

### Example 1: r = 3, p = 0.4

The variance is larger than the mean.

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
  const auto m = actuarial::losses::negbin_moments(3.0, 0.4);
  out.writeln("mean =", m.mean.item<double>(), " variance =", m.variance.item<double>());
}
```

Output

```text
mean = 4.500000  variance = 11.250000
```

### Example 2: by hand

The closed forms.

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
  const auto m = actuarial::losses::negbin_moments(3.0, 0.4);
  out.writeln(m.mean.item<double>(), 3.0 * 0.6 / 0.4);
  out.writeln(m.variance.item<double>(), 3.0 * 0.6 / (0.4 * 0.4));
}
```

Output

```text
4.500000 4.500000
11.250000 11.250000
```

### Example 3: overdispersion relative to Poisson

At the same mean, the negative binomial has the larger variance; the gap closes as p rises.

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
  for (const double p : {0.2, 0.5, 0.9}) {
    const double r = 3.0 * p / (1.0 - p);  // chosen so that the mean is 3
    const auto m = actuarial::losses::negbin_moments(r, p);
    out.writeln("p", p, " mean", m.mean.item<double>(), " variance", m.variance.item<double>());
  }
}
```

Output

```text
p 0.200000  mean 3.000000  variance 15.000000
p 0.500000  mean 3.000000  variance 6.000000
p 0.900000  mean 3.000000  variance 3.333333
```

### Example 4: a non-integer r

The parameter can be fractional.

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
  const auto m = actuarial::losses::negbin_moments(2.5, 0.5);
  out.writeln("mean =", m.mean.item<double>(), " variance =", m.variance.item<double>());
}
```

Output

```text
mean = 2.500000  variance = 5.000000
```

### Example 5: the errors

Parameters outside their ranges are refused.

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
  report([&] { (void)actuarial::losses::negbin_moments(0.0, 0.5); });
  report([&] { (void)actuarial::losses::negbin_moments(3.0, 1.0); });
  report([&] { (void)actuarial::losses::negbin_moments(3.0, 0.0); });
  report([&] { (void)actuarial::losses::negbin_moments(3.0, std::nan("")); });
}
```

Output

```text
negbin_moments: r must be > 0
negbin_moments: p must lie in (0,1)
negbin_moments: p must lie in (0,1)
negbin_moments: p must be finite
```

## See Also

- [`AggregateMoments`](AggregateMoments.md): the result type
- [`aggregate_moments`](aggregate_moments.md): combine a frequency and a severity
- [`poisson_moments`](poisson_moments.md): the equidispersed alternative

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="poisson_moments.html">
    <span class="label">Previous</span>
    <span class="title">poisson_moments</span>
  </a>
  <a class="next-link" href="lognormal_moments.html">
    <span class="label">Next</span>
    <span class="title">lognormal_moments</span>
  </a>
</div>
<!-- apiref:nav:end -->
