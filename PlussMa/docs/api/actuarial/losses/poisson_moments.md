<!-- generated from poisson_moments.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::losses::poisson_moments`

Mean and variance of a Poisson claim count.

## Signature

```cpp
actuarial::losses::AggregateMoments poisson_moments(double lambda, pluss::Platform platform = pluss::Platform::CPU);
```

## What It Does

A Poisson count with rate {math}`\lambda` has mean and variance both equal to {math}`\lambda`: the variance-to-mean ratio of 1 is the Poisson signature, and a count with more spread calls for [`negbin_moments`](negbin_moments.md). Use the pair as the frequency input of [`aggregate_moments`](aggregate_moments.md), or call [`compound_poisson`](compound_poisson.md) to do both steps.

## Parameters

`lambda`
: The expected number of claims, finite and 0 or more.

`platform`
: Where the result is placed. Default `pluss::Platform::CPU`.

## Returns

An [`AggregateMoments`](AggregateMoments.md) of two rank-zero series.

## Errors

Throws `poisson_moments: lambda must be finite` and `poisson_moments: lambda must be >= 0`.

## Practical Examples

### Example 1: a rate of 2.5

Mean and variance coincide.

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
  const auto m = actuarial::losses::poisson_moments(2.5);
  out.writeln("mean =", m.mean.item<double>(), " variance =", m.variance.item<double>());
}
```

Output

```text
mean = 2.500000  variance = 2.500000
```

### Example 2: no claims

A rate of zero is valid and gives zero for both.

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
  const auto m = actuarial::losses::poisson_moments(0.0);
  out.writeln("mean =", m.mean.item<double>(), " variance =", m.variance.item<double>());
}
```

Output

```text
mean = 0.000000  variance = 0.000000
```

### Example 3: on an explicit platform

The optional argument places the result; the values are the same.

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
  const auto m = actuarial::losses::poisson_moments(2.5, pluss::Platform::CPU);
  out.writeln(m.mean.item<double>(), m.variance.item<double>());
}
```

Output

```text
2.500000 2.500000
```

### Example 4: mean equals variance for any rate

The variance-to-mean ratio is one.

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
  for (const double lambda : {0.5, 2.0, 10.0}) {
    const auto m = actuarial::losses::poisson_moments(lambda);
    out.writeln("lambda", lambda, " variance / mean =", m.variance.item<double>() / m.mean.item<double>());
  }
}
```

Output

```text
lambda 0.500000  variance / mean = 1.000000
lambda 2.000000  variance / mean = 1.000000
lambda 10.000000  variance / mean = 1.000000
```

### Example 5: the errors

A negative or non-finite rate is refused.

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
  report([&] { (void)actuarial::losses::poisson_moments(-1.0); });
  report([&] { (void)actuarial::losses::poisson_moments(std::nan("")); });
}
```

Output

```text
poisson_moments: lambda must be >= 0
poisson_moments: lambda must be finite
```

## See Also

- [`AggregateMoments`](AggregateMoments.md): the result type
- [`aggregate_moments`](aggregate_moments.md): combine a frequency and a severity

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="AggregateMoments.html">
    <span class="label">Previous</span>
    <span class="title">AggregateMoments</span>
  </a>
  <a class="next-link" href="negbin_moments.html">
    <span class="label">Next</span>
    <span class="title">negbin_moments</span>
  </a>
</div>
<!-- apiref:nav:end -->
