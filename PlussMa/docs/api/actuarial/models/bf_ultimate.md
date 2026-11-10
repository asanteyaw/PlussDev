<!-- generated from bf_ultimate.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::models::bf_ultimate`

Bornhuetter-Ferguson ultimate losses.

## Signature

```cpp
pluss::Series actuarial::models::bf_ultimate(const pluss::Series &reported_losses, const pluss::Series &expected_ultimate_losses, const pluss::Series &percent_reported);
```

## What It Does

Returns {math}`\text{reported}+\text{expected ultimate}\times(1-\text{percent reported})`: the losses reported so far plus the unreported share of an a priori ultimate. The reported amount is not projected, so an unusual diagonal moves the ultimate by exactly its own size and no more, which is why the method is preferred for immature periods. It equals the `ultimate` of [`operators::bornhuetter_ferguson`](../reserving/bornhuetter_ferguson.md) and is the free-function form for vectors of reported amounts.

## Parameters

`reported_losses`
: Latest reported (or paid) amount of each origin, finite and 0 or more.

`expected_ultimate_losses`
: A priori ultimate of each origin, the same length, finite and 0 or more.

`percent_reported`
: Fraction of the ultimate expected to be reported by now, the same length, between 0 and 1.

## Returns

A series of ultimates, one per origin.

## Errors

Throws `bf_ultimate: reported_losses and expected_ultimate_losses must have same length`, `bf_ultimate: percent_reported must have same length`, `bf_ultimate: percent_reported must be <= 1` and the `must be defined/finite/>= 0` errors for each argument.

## Practical Examples

### Example 1: four origins

Reported losses, a priori ultimates and the share reported.

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
  const pluss::Series reported = pluss::make_series({240.0, 250.0, 215.0, 130.0});
  const pluss::Series expected = pluss::make_series({250.0, 280.0, 300.0, 330.0});
  const pluss::Series pct = pluss::make_series({0.96, 0.90, 0.75, 0.50});
  out.writeln(actuarial::models::bf_ultimate(reported, expected, pct).items<double>());
}
```

Output

```text
[250.00, 278.00, 290.00, 295.00]
```

### Example 2: by hand

Origin 3: 130 + 330 x (1 - 0.5).

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
  const pluss::Series reported = pluss::make_series({240.0, 250.0, 215.0, 130.0});
  const pluss::Series expected = pluss::make_series({250.0, 280.0, 300.0, 330.0});
  const pluss::Series pct = pluss::make_series({0.96, 0.90, 0.75, 0.50});
  out.writeln(130.0 + 330.0 * 0.5, actuarial::models::bf_ultimate(reported, expected, pct).items<double>()[3]);
}
```

Output

```text
295.00 295.00
```

### Example 3: reported plus reserve

The ultimate is the reported amount plus bf_reserve.

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
  const pluss::Series reported = pluss::make_series({240.0, 250.0, 215.0, 130.0});
  const pluss::Series expected = pluss::make_series({250.0, 280.0, 300.0, 330.0});
  const pluss::Series pct = pluss::make_series({0.96, 0.90, 0.75, 0.50});
  out.writeln(actuarial::models::bf_ultimate(reported, expected, pct).items<double>());
  out.writeln(reported.add(actuarial::models::bf_reserve(expected, pct)).items<double>());
}
```

Output

```text
[250.00, 278.00, 290.00, 295.00]
[250.00, 278.00, 290.00, 295.00]
```

### Example 4: the same as the operator

On a triangle, the Bornhuetter-Ferguson operator gives the same ultimates.

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
  const pluss::Series triangle = pluss::make_series({100.0, 180.0, 220.0, 240.0,
                                                     110.0, 200.0, 250.0, 0.0,
                                                     120.0, 215.0, 0.0, 0.0,
                                                     130.0, 0.0, 0.0, 0.0}, {4, 4});
  const pluss::Series reported = pluss::make_series({240.0, 250.0, 215.0, 130.0});
  const pluss::Series expected = pluss::make_series({250.0, 280.0, 300.0, 330.0});
  const pluss::Series pct = pluss::make_series({0.96, 0.90, 0.75, 0.50});
  const actuarial::Triangle t = actuarial::triangle::cumulative(triangle);
  out.writeln(actuarial::operators::bornhuetter_ferguson(expected, pct)(t).ultimate.items<double>());
  out.writeln(actuarial::models::bf_ultimate(reported, expected, pct).items<double>());
}
```

Output

```text
[250.00, 278.00, 290.00, 295.00]
[250.00, 278.00, 290.00, 295.00]
```

### Example 5: fully reported origins

At 100% reported the ultimate is the reported amount.

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
  out.writeln(actuarial::models::bf_ultimate(pluss::make_series({240.0, 250.0}), pluss::make_series({999.0, 999.0}), pluss::make_series({1.0, 1.0})).items<double>());
}
```

Output

```text
[240.00, 250.00]
```

### Example 6: the errors

Mismatched lengths and a percentage above 1 are refused.

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
  report([&] { (void)actuarial::models::bf_ultimate(pluss::make_series({1.0, 2.0}), pluss::make_series({1.0}), pluss::make_series({0.5, 0.5})); });
  report([&] { (void)actuarial::models::bf_ultimate(pluss::make_series({1.0, 2.0}), pluss::make_series({1.0, 2.0}), pluss::make_series({0.5})); });
  report([&] { (void)actuarial::models::bf_ultimate(pluss::make_series({1.0}), pluss::make_series({1.0}), pluss::make_series({1.5})); });
}
```

Output

```text
bf_ultimate: reported_losses and expected_ultimate_losses must have same length
bf_ultimate: percent_reported must have same length
bf_ultimate: percent_reported must be <= 1
```

## See Also

- [`bf_reserve`](bf_reserve.md): the reserve alone
- [`operators::bornhuetter_ferguson`](../reserving/bornhuetter_ferguson.md): the operator form

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="experience_change.html">
    <span class="label">Previous</span>
    <span class="title">experience_change</span>
  </a>
  <a class="next-link" href="bf_reserve.html">
    <span class="label">Next</span>
    <span class="title">bf_reserve</span>
  </a>
</div>
<!-- apiref:nav:end -->
