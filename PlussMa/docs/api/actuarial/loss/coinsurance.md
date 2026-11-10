<!-- generated from coinsurance.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::operators::coinsurance`

Creates the proportional-participation transform.

## Signature

```cpp
actuarial::LossTransform actuarial::operators::coinsurance(double share);
```

## What It Does

Returns the operator {math}`X\mapsto\alpha X` for a share {math}`\alpha\in[0,1]`: the insurer pays the fraction `α` of each amount and the policyholder the rest. It is applied to ground-up losses or, more usefully, to the output of another transform, since the order matters: coinsurance of a deductible payment is {math}`\alpha(X-d)_+` and is generally not {math}`(\alpha X-d)_+`. [`layer`](layer.md) carries its own share. The operator is elementwise and keeps the shape and platform of its input.

## Parameters

`share`
: The participation `α`, between 0 and 1 inclusive.

`losses` (of the returned operator)
: One-dimensional, finite, non-negative, defined series of loss amounts.

## Returns

A `LossTransform` with key `"coinsurance"`.

## Errors

Throws `operators::coinsurance: share must be >= 0` and `operators::coinsurance: share must be <= 1` when the operator is built. Calling the returned operator throws `<name>: losses must be defined`, `<name>: losses must be 1D`, `<name>: losses must be finite` or `<name>: losses must be >= 0`, with the operator's own name.

## Practical Examples

### Example 1: an 80 percent share

The insurer pays 80% of each loss.

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
  const pluss::Series x = pluss::make_series({80.0, 150.0, 300.0, 700.0, 1200.0, 4000.0});
  out.writeln(actuarial::operators::coinsurance(0.8)(x).items<double>());
}
```

Output

```text
[64.000000, 120.000000, 240.000000, 560.000000, 960.000000, 3200.000000]
```

### Example 2: the order matters

Share of a deductible payment against deductible of a share.

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
  const pluss::Series x = pluss::make_series({80.0, 150.0, 300.0, 700.0, 1200.0, 4000.0});
  const auto share = actuarial::operators::coinsurance(0.8);
  const auto ded = actuarial::operators::deductible(100.0);
  out.writeln("share(deductible(x)) =", share(ded(x)).items<double>());
  out.writeln("deductible(share(x)) =", ded(share(x)).items<double>());
}
```

Output

```text
share(deductible(x)) = [0.000000, 40.000000, 160.000000, 480.000000, 880.000000, 3120.000000]
deductible(share(x)) = [0.000000, 20.000000, 140.000000, 460.000000, 860.000000, 3100.000000]
```

### Example 3: the boundary shares

Zero pays nothing, one pays everything.

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
  const pluss::Series x = pluss::make_series({80.0, 150.0, 300.0, 700.0, 1200.0, 4000.0});
  out.writeln("share 0:", actuarial::operators::coinsurance(0.0)(x).items<double>());
  out.writeln("share 1:", actuarial::operators::coinsurance(1.0)(x).items<double>());
}
```

Output

```text
share 0: [0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000]
share 1: [80.000000, 150.000000, 300.000000, 700.000000, 1200.000000, 4000.000000]
```

### Example 4: linearity in the mean

The mean payment is `α` times the mean of the amounts it is applied to.

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
  const pluss::Series x = pluss::make_series({80.0, 150.0, 300.0, 700.0, 1200.0, 4000.0});
  const actuarial::LossMeasure mean = actuarial::operators::mean();
  out.writeln(mean(actuarial::operators::coinsurance(0.8)(x)).item<double>(), 0.8 * mean(x).item<double>());
}
```

Output

```text
857.333333 857.333333
```

### Example 5: the errors

A share outside [0,1] is refused.

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
  report([&] { (void)actuarial::operators::coinsurance(-0.1); });
  report([&] { (void)actuarial::operators::coinsurance(1.5); });
}
```

Output

```text
operators::coinsurance: share must be >= 0
operators::coinsurance: share must be <= 1
```

## See Also

- [`LossTransform`](LossTransform.md): the type
- [`layer`](layer.md): deductible and limit in one step

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="limit.html">
    <span class="label">Previous</span>
    <span class="title">limit</span>
  </a>
  <a class="next-link" href="layer.html">
    <span class="label">Next</span>
    <span class="title">layer</span>
  </a>
</div>
<!-- apiref:nav:end -->
