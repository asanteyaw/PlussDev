<!-- generated from limit.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::operators::limit`

Creates the policy-limit transform.

## Signature

```cpp
actuarial::LossTransform actuarial::operators::limit(double amount);
```

## What It Does

Returns the operator {math}`X\mapsto\min(X,u)`: each loss capped at the limit `u`, which is what the insurer pays on a policy that covers at most `u`. Losses below the limit are untouched; larger ones all give the same payment. It bounds the payment, so the tail of the result is finite even when the loss distribution has an infinite mean. The mean of the capped loss is the limited expected value; compare it with [`deductible`](deductible.md), which cuts from below. The operator is elementwise and keeps the shape and platform of its input.

## Parameters

`amount`
: The limit `u`, finite and 0 or more.

`losses` (of the returned operator)
: One-dimensional, finite, non-negative, defined series of loss amounts.

## Returns

A `LossTransform` with key `"limit"`.

## Errors

Throws `operators::limit: amount must be >= 0` when the operator is built. Calling the returned operator throws `<name>: losses must be defined`, `<name>: losses must be 1D`, `<name>: losses must be finite` or `<name>: losses must be >= 0`, with the operator's own name.

## Practical Examples

### Example 1: capped losses

A limit of 500.

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
  out.writeln("loss =", x.items<double>());
  out.writeln("paid =", actuarial::operators::limit(500.0)(x).items<double>());
}
```

Output

```text
loss = [80.000000, 150.000000, 300.000000, 700.000000, 1200.000000, 4000.000000]
paid = [80.000000, 150.000000, 300.000000, 500.000000, 500.000000, 500.000000]
```

### Example 2: limited expected value

The mean payment rises with the limit, with diminishing increments, and reaches the mean loss once the limit exceeds every loss.

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
  for (const double u : {100.0, 500.0, 2000.0, 5000.0})
    out.writeln("u =", u, " mean paid =", actuarial::operators::mean()(actuarial::operators::limit(u)(x)).item<double>());
  out.writeln("mean loss =", actuarial::operators::mean()(x).item<double>());
}
```

Output

```text
u = 100.000000  mean paid = 96.666667
u = 500.000000  mean paid = 338.333333
u = 2000.000000  mean paid = 738.333333
u = 5000.000000  mean paid = 1071.666667
mean loss = 1071.666667
```

### Example 3: deductible and limit are complements

Split a loss at 500: what a limit of 500 pays plus what a deductible of 500 pays is the loss.

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
  out.writeln(actuarial::operators::limit(500.0)(x).add(actuarial::operators::deductible(500.0)(x)).items<double>());
  out.writeln(x.items<double>());
}
```

Output

```text
[80.000000, 150.000000, 300.000000, 700.000000, 1200.000000, 4000.000000]
[80.000000, 150.000000, 300.000000, 700.000000, 1200.000000, 4000.000000]
```

### Example 4: a zero limit

Nothing is paid.

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
  out.writeln(actuarial::operators::limit(0.0)(x).items<double>());
}
```

Output

```text
[0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000]
```

### Example 5: the errors

A negative limit and a loss series that is not finite.

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
  report([&] { (void)actuarial::operators::limit(-1.0); });
  report([&] { (void)actuarial::operators::limit(100.0)(pluss::make_series({1.0, std::nan("")})); });
}
```

Output

```text
operators::limit: amount must be >= 0
limit: losses must be finite
```

## See Also

- [`LossTransform`](LossTransform.md): the type
- [`layer`](layer.md): deductible and limit in one step

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="deductible.html">
    <span class="label">Previous</span>
    <span class="title">deductible</span>
  </a>
  <a class="next-link" href="coinsurance.html">
    <span class="label">Next</span>
    <span class="title">coinsurance</span>
  </a>
</div>
<!-- apiref:nav:end -->
