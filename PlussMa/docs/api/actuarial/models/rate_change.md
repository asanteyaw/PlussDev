<!-- generated from rate_change.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::models::rate_change`

The change from the current premium to the indicated one.

## Signature

```cpp
pluss::Series actuarial::models::rate_change(const pluss::Series &current_premium, const pluss::Series &indicated_premium);
```

## What It Does

Returns {math}`\text{indicated}/\text{current}-1`: positive where the premium now charged is too low, negative where it is too high, zero where it is right. The two series are premium rates per unit of exposure and must have the same length; the current premium must be strictly positive since it is the divisor. It is the third step of [`operators::price`](../pricing/price.md).

## Parameters

`current_premium`
: The premium now charged in each cell, finite and strictly greater than 0.

`indicated_premium`
: The premium indicated in each cell, finite and 0 or more, the same length.

## Returns

A series of the same length as the inputs.

## Errors

Throws `rate_change: current_premium and indicated_premium must have same length`, `rate_change: current_premium must be strictly > 0`, and the `rate_change: <argument> must be defined`, `must be finite` or `must be >= 0` errors.

## Practical Examples

### Example 1: four cells

Three cells need an increase and one a decrease.

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
  out.writeln(actuarial::models::rate_change(pluss::make_series({700.0, 720.0, 740.0, 760.0}), pluss::make_series({712.5, 781.25, 737.5, 826.39})).items<double>());
}
```

Output

```text
[0.017857, 0.085069, -0.003378, 0.087355]
```

### Example 2: by hand

712.5 / 700 - 1.

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
  out.writeln(712.5 / 700.0 - 1.0, actuarial::models::rate_change(pluss::make_series({700.0}), pluss::make_series({712.5})).items<double>()[0]);
}
```

Output

```text
0.017857 0.017857
```

### Example 3: the right rate

An indicated premium equal to the current one is a change of zero.

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
  out.writeln(actuarial::models::rate_change(pluss::make_series({700.0, 800.0}), pluss::make_series({700.0, 800.0})).items<double>());
}
```

Output

```text
[0.000000, 0.000000]
```

### Example 4: from losses to a rate change

The chain from the data to the change, step by step.

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
  const pluss::Series losses = pluss::make_series({520000.0, 690000.0, 810000.0, 1100000.0});
  const pluss::Series exposure = pluss::make_series({1000.0, 1200.0, 1500.0, 1800.0});
  const pluss::Series premium = pluss::make_series({700.0, 720.0, 740.0, 760.0});
  const pluss::Series pure = actuarial::models::premium_curve(losses, exposure);
  const pluss::Series indicated = actuarial::models::indicated_premium(pure, 50.0, 0.15, 0.05);
  out.writeln(actuarial::models::rate_change(premium, indicated).items<double>());
}
```

Output

```text
[0.017857, 0.085069, -0.003378, 0.087354]
```

### Example 5: the errors

A zero current premium and a length mismatch are refused.

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
  report([&] { (void)actuarial::models::rate_change(pluss::make_series({0.0, 800.0}), pluss::make_series({700.0, 800.0})); });
  report([&] { (void)actuarial::models::rate_change(pluss::make_series({700.0, 800.0}), pluss::make_series({700.0})); });
  report([&] { (void)actuarial::models::rate_change(pluss::make_series({700.0}), pluss::make_series({-1.0})); });
}
```

Output

```text
rate_change: current_premium must be strictly > 0
rate_change: current_premium and indicated_premium must have same length
rate_change: indicated_premium must be >= 0
```

## See Also

- [`indicated_premium`](indicated_premium.md): the indicated premium
- [`experience_change`](experience_change.md): the whole chain in one call

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="indicated_premium.html">
    <span class="label">Previous</span>
    <span class="title">indicated_premium</span>
  </a>
  <a class="next-link" href="experience_change.html">
    <span class="label">Next</span>
    <span class="title">experience_change</span>
  </a>
</div>
<!-- apiref:nav:end -->
