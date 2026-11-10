<!-- generated from lognormal_layer.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::losses::lognormal_layer`

Expected loss to a layer under a lognormal severity.

## Signature

```cpp
actuarial::losses::pluss::Series lognormal_layer(double mean_log, double stddev_log, double attachment, double limit, std::int64_t quadrature_steps = 2048, pluss::Platform platform = pluss::Platform::CPU);
```

## What It Does

The expected payment per claim of a layer of width `l` above attachment `a`: {math}`E[\min(X,a+l)]-E[\min(X,a)]`, the difference of two [`lognormal_lev`](lognormal_lev.md) values with the same quadrature. It is the parametric counterpart of [`layer_premium`](layer_premium.md): the same quantity from the distribution instead of a sample. The result is between 0 and the limit.

## Parameters

`mean_log`, `stddev_log`
: The mean and standard deviation of the log-claim: finite, and `stddev_log` 0 or more.

`attachment`
: Where the layer starts, 0 or more.

`limit`
: The width of the layer, 0 or more.

`quadrature_steps`
: Number of equal subintervals of `[0, cap]` in the midpoint rule, greater than 0. Default 2048. More steps give a more accurate value.

`platform`
: Where the result is placed. Default `pluss::Platform::CPU`.

## Returns

A rank-zero series.

## Errors

Throws `lognormal_layer: attachment must be >= 0`, `lognormal_layer: limit must be >= 0` and the `lognormal_lev` errors for the other arguments.

## Practical Examples

### Example 1: 2 xs 1

The standard lognormal, layer from 1 to 3.

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
  out.writeln(actuarial::losses::lognormal_layer(0.0, 1.0, 1.0, 2.0).item<double>());
}
```

Output

```text
0.535445
```

### Example 2: a difference of limited values

The definition.

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
  out.print_defaults(PrintOptions{}.fixed(10));
  out.writeln(actuarial::losses::lognormal_layer(0.0, 1.0, 1.0, 2.0).item<double>(), actuarial::losses::lognormal_lev(0.0, 1.0, 3.0).item<double>() - actuarial::losses::lognormal_lev(0.0, 1.0, 1.0).item<double>());
}
```

Output

```text
0.5354449014 0.5354449014
```

### Example 3: a ladder of layers

Layers that meet add up to the limited value at the top.

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
  const double a = actuarial::losses::lognormal_layer(0.0, 1.0, 0.0, 1.0).item<double>();
  const double b = actuarial::losses::lognormal_layer(0.0, 1.0, 1.0, 2.0).item<double>();
  const double c = actuarial::losses::lognormal_layer(0.0, 1.0, 3.0, 7.0).item<double>();
  out.writeln(a + b + c, actuarial::losses::lognormal_lev(0.0, 1.0, 10.0).item<double>());
}
```

Output

```text
1.59636441 1.59636441
```

### Example 4: the effect of the quadrature steps

The layer converges as the number of steps grows.

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
  for (const int n : {16, 256, 4096})
    out.writeln("steps", n, " layer =", actuarial::losses::lognormal_layer(0.0, 1.0, 1.0, 2.0, n).item<double>());
}
```

Output

```text
steps 16  layer = 0.53565505
steps 256  layer = 0.53544506
steps 4096  layer = 0.53544490
```

### Example 5: a higher layer

The expected loss to a layer falls as it sits further out.

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
  for (const double a : {0.0, 2.0, 5.0, 10.0})
    out.writeln("attachment", a, " layer of width 2 =", actuarial::losses::lognormal_layer(0.0, 1.0, a, 2.0).item<double>());
}
```

Output

```text
attachment 0.000000  layer of width 2 = 1.113870
attachment 2.000000  layer of width 2 = 0.289707
attachment 5.000000  layer of width 2 = 0.075284
attachment 10.000000  layer of width 2 = 0.016702
```

### Example 6: the errors

A negative attachment or limit.

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
  report([&] { (void)actuarial::losses::lognormal_layer(0.0, 1.0, -1.0, 2.0); });
  report([&] { (void)actuarial::losses::lognormal_layer(0.0, 1.0, 1.0, -2.0); });
}
```

Output

```text
lognormal_layer: attachment must be >= 0
lognormal_layer: limit must be >= 0
```

## See Also

- [`lognormal_lev`](lognormal_lev.md): the limited expected value
- [`layer_premium`](layer_premium.md): the sample-based counterpart

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="gamma_lev.html">
    <span class="label">Previous</span>
    <span class="title">gamma_lev</span>
  </a>
  <a class="next-link" href="gamma_layer.html">
    <span class="label">Next</span>
    <span class="title">gamma_layer</span>
  </a>
</div>
<!-- apiref:nav:end -->
