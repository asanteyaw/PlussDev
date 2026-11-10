<!-- generated from negbin_pmf.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::losses::negbin_pmf`

Negative binomial probabilities of 0 to a maximum count.

## Signature

```cpp
actuarial::losses::pluss::Series negbin_pmf(double r, double p, std::int64_t max_count, pluss::Platform platform = pluss::Platform::CPU);
```

## What It Does

Returns {math}`\Pr(N=k)=\binom{k+r-1}{k}p^r(1-p)^k` for `k = 0, ..., max_count`, a series of `max_count + 1` values: the probability of `k` failures before the `r`-th success. The probabilities beyond `max_count` are left out, so the series sums to slightly less than one. It is the distribution behind [`negbin_moments`](negbin_moments.md), with a longer right tail than the Poisson of the same mean.

## Parameters

`r`
: The number of successes, greater than 0.

`p`
: The success probability, strictly between 0 and 1.

`max_count`
: The largest count, 0 or more.

`platform`
: Where the result is placed. Default `pluss::Platform::CPU`.

## Returns

A series of `max_count + 1` probabilities.

## Errors

Throws `negbin_pmf: r must be > 0`, `negbin_pmf: p must lie in (0,1)` and `negbin_pmf: max_count must be >= 0`.

## Practical Examples

### Example 1: the first six counts

r = 3, p = 0.4.

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
  out.writeln(actuarial::losses::negbin_pmf(3.0, 0.4, 5).items<double>());
}
```

Output

```text
[0.064000, 0.115200, 0.138240, 0.138240, 0.124416, 0.104509]
```

### Example 2: by hand

p(0) = p^r and p(1) = r p^r (1 - p).

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
  out.writeln(std::pow(0.4, 3.0), 3.0 * std::pow(0.4, 3.0) * 0.6);
  out.writeln(actuarial::losses::negbin_pmf(3.0, 0.4, 1).items<double>());
}
```

Output

```text
0.064000 0.115200
[0.064000, 0.115200]
```

### Example 3: the tail that is left out

The sum approaches one as the maximum count grows.

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
  out.print_defaults(PrintOptions{}.fixed(8));
  for (const int n : {5, 20, 60})
    out.writeln("max_count", n, " sum =", actuarial::losses::negbin_pmf(3.0, 0.4, n).sum().item<double>());
}
```

Output

```text
max_count 5  sum = 0.68460544
max_count 20  sum = 0.99898300
max_count 60  sum = 1.00000000
```

### Example 4: a heavier tail than the Poisson

At the same mean of 4.5 the negative binomial puts more mass on large counts.

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
  const pluss::Series nb = actuarial::losses::negbin_pmf(3.0, 0.4, 20);
  const pluss::Series po = actuarial::losses::poisson_pmf(4.5, 20);
  out.writeln("P(N = 15): negbin", nb.items<double>()[15], " poisson", po.items<double>()[15]);
}
```

Output

```text
P(N = 15): negbin 0.004092  poisson 0.000053
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
  report([&] { (void)actuarial::losses::negbin_pmf(3.0, 1.0, 5); });
  report([&] { (void)actuarial::losses::negbin_pmf(0.0, 0.4, 5); });
  report([&] { (void)actuarial::losses::negbin_pmf(3.0, 0.4, -1); });
}
```

Output

```text
negbin_pmf: p must lie in (0,1)
negbin_pmf: r must be > 0
negbin_pmf: max_count must be >= 0
```

## See Also

- [`negbin_moments`](negbin_moments.md): the mean and variance
- [`poisson_pmf`](poisson_pmf.md): the equidispersed alternative

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="poisson_pmf.html">
    <span class="label">Previous</span>
    <span class="title">poisson_pmf</span>
  </a>
  <a class="next-link" href="excess_loss.html">
    <span class="label">Next</span>
    <span class="title">excess_loss</span>
  </a>
</div>
<!-- apiref:nav:end -->
