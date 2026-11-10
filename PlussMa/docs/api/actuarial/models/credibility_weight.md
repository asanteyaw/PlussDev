<!-- generated from credibility_weight.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::models::credibility_weight`

The credibility weight of each cell.

## Signature

```cpp
pluss::Series actuarial::models::credibility_weight(const pluss::Series &exposure, double k);
```

## What It Does

Returns {math}`Z=E/(E+k)`: the weight to put on a cell's own experience, rising from 0 at no exposure toward 1 as exposure grows, and equal to one half when the exposure equals `k`. It is the weight inside [`credibility_blend`](credibility_blend.md) and [`operators::credibility`](../pricing/credibility_operator.md), exposed so it can be reported or used with another complement. A larger `k` demands more data before the experience is trusted.

## Parameters

`exposure`
: Volume of each cell, finite and 0 or more.

`k`
: The credibility standard, finite and greater than 0: the exposure at which the weight is one half.

## Returns

A series of weights in [0, 1) of the shape of `exposure`.

## Errors

Throws `credibility_weight: k must be finite`, `credibility_weight: k must be > 0`, and `credibility_weight: exposure must be defined`, `must be finite` or `must be >= 0`.

## Practical Examples

### Example 1: weights at five exposures

k = 100.

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
  out.writeln(actuarial::models::credibility_weight(pluss::make_series({0.0, 25.0, 100.0, 400.0, 10000.0}), 100.0).items<double>());
}
```

Output

```text
[0.000000, 0.200000, 0.500000, 0.800000, 0.990099]
```

### Example 2: one half at k

The weight at exposure equal to k.

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
  for (const double k : {50.0, 100.0, 1000.0})
    out.writeln("k", k, " weight at exposure k =", actuarial::models::credibility_weight(pluss::make_series({k}), k).items<double>()[0]);
}
```

Output

```text
k 50.000000  weight at exposure k = 0.500000
k 100.000000  weight at exposure k = 0.500000
k 1000.000000  weight at exposure k = 0.500000
```

### Example 3: the standard decides how much data is enough

The same exposure under three standards.

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
  for (const double k : {10.0, 100.0, 1000.0})
    out.writeln("k", k, " weight at exposure 200 =", actuarial::models::credibility_weight(pluss::make_series({200.0}), k).items<double>()[0]);
}
```

Output

```text
k 10.000000  weight at exposure 200 = 0.952381
k 100.000000  weight at exposure 200 = 0.666667
k 1000.000000  weight at exposure 200 = 0.166667
```

### Example 4: the blend by hand

Weight the experience and the manual rate.

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
  const double z = actuarial::models::credibility_weight(pluss::make_series({200.0}), 100.0).items<double>()[0];
  out.writeln("z", z, " blend", z * 0.80 + (1.0 - z) * 0.65, actuarial::models::credibility_blend(pluss::make_series({0.80}), pluss::make_series({0.65}), pluss::make_series({200.0}), 100.0).items<double>()[0]);
}
```

Output

```text
z 0.666667  blend 0.750000 0.750000
```

### Example 5: the errors

A standard that is not positive and a negative exposure.

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
  report([&] { (void)actuarial::models::credibility_weight(pluss::make_series({100.0}), 0.0); });
  report([&] { (void)actuarial::models::credibility_weight(pluss::make_series({100.0}), std::nan("")); });
  report([&] { (void)actuarial::models::credibility_weight(pluss::make_series({-1.0}), 100.0); });
}
```

Output

```text
credibility_weight: k must be > 0
credibility_weight: k must be finite
credibility_weight: exposure must be >= 0
```

## See Also

- [`credibility_blend`](credibility_blend.md): applies the weight
- [`operators::credibility`](../pricing/credibility_operator.md): the operator form

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ratio_curve.html">
    <span class="label">Previous</span>
    <span class="title">ratio_curve</span>
  </a>
  <a class="next-link" href="credibility_blend.html">
    <span class="label">Next</span>
    <span class="title">credibility_blend</span>
  </a>
</div>
<!-- apiref:nav:end -->
