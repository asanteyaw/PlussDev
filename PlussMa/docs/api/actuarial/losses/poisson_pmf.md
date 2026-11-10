<!-- generated from poisson_pmf.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::losses::poisson_pmf`

Poisson probabilities of 0 to a maximum count.

## Signature

```cpp
actuarial::losses::pluss::Series poisson_pmf(double lambda, std::int64_t max_count, pluss::Platform platform = pluss::Platform::CPU);
```

## What It Does

Returns {math}`\Pr(N=k)=e^{-\lambda}\lambda^k/k!` for `k = 0, ..., max_count`, a series of `max_count + 1` values. The probabilities beyond `max_count` are left out, so the series sums to slightly less than one; choose `max_count` far enough into the tail for the use. It is the distribution behind [`poisson_moments`](poisson_moments.md) and the input to any calculation that sums over the number of claims, such as the exact distribution of an aggregate loss.

## Parameters

`lambda`
: The expected number of claims, finite and 0 or more.

`max_count`
: The largest count, 0 or more.

`platform`
: Where the result is placed. Default `pluss::Platform::CPU`.

## Returns

A series of `max_count + 1` probabilities.

## Errors

Throws `poisson_pmf: lambda must be finite`, `poisson_pmf: lambda must be >= 0` and `poisson_pmf: max_count must be >= 0`.

## Practical Examples

### Example 1: the first six counts

A rate of 2.

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
  out.writeln(actuarial::losses::poisson_pmf(2.0, 5).items<double>());
}
```

Output

```text
[0.135335, 0.270671, 0.270671, 0.180447, 0.090224, 0.036089]
```

### Example 2: by hand

The recursion p(k+1) = p(k) lambda / (k+1).

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
  const double p0 = std::exp(-2.0);
  out.writeln(p0, p0 * 2.0, p0 * 2.0 * 2.0 / 2.0, p0 * 2.0 * 2.0 * 2.0 / 6.0);
  out.writeln(actuarial::losses::poisson_pmf(2.0, 3).items<double>());
}
```

Output

```text
0.135335 0.270671 0.270671 0.180447
[0.135335, 0.270671, 0.270671, 0.180447]
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
  for (const int n : {2, 5, 10, 30})
    out.writeln("max_count", n, " sum =", actuarial::losses::poisson_pmf(2.0, n).sum().item<double>());
}
```

Output

```text
max_count 2  sum = 0.67667642
max_count 5  sum = 0.98343639
max_count 10  sum = 0.99999169
max_count 30  sum = 1.00000000
```

### Example 4: the mean from the probabilities

The weighted sum of the counts matches the rate.

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
  const pluss::Series p = actuarial::losses::poisson_pmf(2.0, 40);
  const pluss::Series k = pluss::series::arange(0, 41);
  out.writeln(p.mul(k.to(pluss::ScalarType::Float64)).sum().item<double>());
}
```

Output

```text
2.00000000
```

### Example 5: only the zero count

A maximum count of 0 returns the probability of no claims.

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
  out.writeln(actuarial::losses::poisson_pmf(2.0, 0).items<double>(), std::exp(-2.0));
}
```

Output

```text
[0.135335] 0.135335
```

### Example 6: the errors

A negative rate or maximum count is refused.

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
  report([&] { (void)actuarial::losses::poisson_pmf(-1.0, 5); });
  report([&] { (void)actuarial::losses::poisson_pmf(2.0, -1); });
}
```

Output

```text
poisson_pmf: lambda must be >= 0
poisson_pmf: max_count must be >= 0
```

## See Also

- [`poisson_moments`](poisson_moments.md): the mean and variance
- [`negbin_pmf`](negbin_pmf.md): the overdispersed alternative

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="compound_negbin.html">
    <span class="label">Previous</span>
    <span class="title">compound_negbin</span>
  </a>
  <a class="next-link" href="negbin_pmf.html">
    <span class="label">Next</span>
    <span class="title">negbin_pmf</span>
  </a>
</div>
<!-- apiref:nav:end -->
