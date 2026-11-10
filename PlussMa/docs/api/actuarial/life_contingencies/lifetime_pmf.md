<!-- generated from lifetime_pmf.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::life_contingencies::lifetime_pmf`

The distribution of the curtate lifetime from death probabilities.

## Signature

```cpp
pluss::Series actuarial::life_contingencies::lifetime_pmf(const pluss::Series &qx);
```

## What It Does

For probabilities {math}`q_0,\ldots,q_{n-1}` returns {math}`[q_0,\;p_0q_1,\;p_0p_1q_2,\;\ldots,\;{}_np_x]`: the probability of dying in each of the `n` years followed by the probability of surviving the whole horizon. It has `n + 1` values and sums to one. It is [`Mortality::lifetime_pmf`](../mortality/Mortality/lifetime_pmf.md) as a free function. Use it for the exact distribution that expected values and variances of contingent payments are taken against.

## Parameters

`qx`
: One-dimensional series of probabilities in {math}`[0,1]`.

## Returns

A series of `qx.numel() + 1` probabilities summing to 1.

## Errors

Throws `survival_curve: qx must be 1D`, `survival_curve: qx must be finite` and `survival_curve: qx values must lie in [0, 1]`.

## Practical Examples

### Example 1: the distribution

Five years of probabilities.

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
  const pluss::Series qx = pluss::make_series({0.010, 0.012, 0.015, 0.019, 0.024});
  out.writeln(life::lifetime_pmf(qx).items<double>());
}
```

Output

```text
[0.010000, 0.011880, 0.014672, 0.018306, 0.022683, 0.922459]
```

### Example 2: it sums to one

The masses form a distribution.

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
  const pluss::Series qx = pluss::make_series({0.010, 0.012, 0.015, 0.019, 0.024});
  out.writeln(life::lifetime_pmf(qx).sum().item<double>());
}
```

Output

```text
1.000000
```

### Example 3: the expected lifetime

Weighting the years by the masses gives the curtate mean within the horizon.

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
  const pluss::Series qx = pluss::make_series({0.010, 0.012, 0.015, 0.019, 0.024});
  const auto pmf = life::lifetime_pmf(qx).items<double>();
  double mean = 0.0;
  for (std::size_t k = 0; k < pmf.size(); ++k) mean += static_cast<double>(k) * pmf[k];
  out.writeln("mean =", mean, " curtate_mean =", life::curtate_mean(qx).item<double>());
}
```

Output

```text
mean = 4.799170  curtate_mean = 4.799170
```

### Example 4: certain death in the first year

All the mass is on year one.

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
  out.writeln(life::lifetime_pmf(pluss::make_series({1.0, 0.5})).items<double>());
}
```

Output

```text
[1.000000, 0.000000, 0.000000]
```

### Example 5: the errors

A matrix or an invalid probability.

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
  report([&] { (void)life::lifetime_pmf(pluss::make_series({0.1, 0.2}).reshape({1, 2})); });
  report([&] { (void)life::lifetime_pmf(pluss::make_series({0.1, 1.5})); });
}
```

Output

```text
survival_curve: qx must be 1D
survival_curve: qx values must lie in [0, 1]
```

## See Also

- [`curtate_mean`](curtate_mean.md): its mean- [`Mortality`](../mortality/Mortality.md): the same conversions as accessors

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="hazard_qx.html">
    <span class="label">Previous</span>
    <span class="title">hazard_qx</span>
  </a>
  <a class="next-link" href="lx_px.html">
    <span class="label">Next</span>
    <span class="title">lx_px</span>
  </a>
</div>
<!-- apiref:nav:end -->
