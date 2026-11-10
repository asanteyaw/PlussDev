<!-- generated from gamma_lev.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::losses::gamma_lev`

Limited expected value of a gamma severity.

## Signature

```cpp
actuarial::losses::pluss::Series gamma_lev(double shape, double scale, double location, double cap, std::int64_t quadrature_steps = 2048, pluss::Platform platform = pluss::Platform::CPU);
```

## What It Does

The limited expected value is {math}`E[\min(X,c)]=\int_0^{c}S(x)\,dx` with {math}`S` the survival function, evaluated by the midpoint rule on `quadrature_steps` equal subintervals; the error falls as the square of the step. The gamma severity has shape {math}`k`, scale {math}`\theta` and a **location** shift: no claim is below the location, so a cap at or below it gives exactly the cap. It is the expected payment per claim of a policy with limit `c` and the building block of [`gamma_layer`](gamma_layer.md). Note that the location comes before the cap in the argument list, unlike [`gamma_moments`](gamma_moments.md) where it is optional and last.

## Parameters

`shape`, `scale`
: Gamma parameters, finite and greater than 0.

`location`
: The shift, finite and 0 or more. Pass 0 for none.

`cap`
: The limit, finite and 0 or more.

`quadrature_steps`
: Number of equal subintervals of `[0, cap]` in the midpoint rule, greater than 0. Default 2048. More steps give a more accurate value.

`platform`
: Where the result is placed. Default `pluss::Platform::CPU`.

## Returns

A rank-zero series.

## Errors

Throws `gamma_lev: shape must be > 0`, `gamma_lev: scale must be > 0`, `gamma_lev: location must be >= 0`, `gamma_lev: cap must be >= 0` and `gamma_lev: quadrature_steps must be > 0`.

## Practical Examples

### Example 1: a limit of 6

Shape 2, scale 3: the mean is 6.

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
  out.writeln(actuarial::losses::gamma_lev(2.0, 3.0, 0.0, 6.0).item<double>());
}
```

Output

```text
4.375977
```

### Example 2: the exponential closed form

With shape 1 the limited expected value is theta (1 - exp(-cap / theta)).

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
  out.writeln(4.0 * (1.0 - std::exp(-6.0 / 4.0)), actuarial::losses::gamma_lev(1.0, 4.0, 0.0, 6.0).item<double>());
}
```

Output

```text
3.10747936 3.10747929
```

### Example 3: the effect of the quadrature steps

The value converges as the number of steps grows.

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
  for (const int n : {4, 16, 64, 256, 2048, 16384})
    out.writeln("steps", n, " value =", actuarial::losses::gamma_lev(2.0, 3.0, 0.0, 6.0, n).item<double>());
}
```

Output

```text
steps 4  value = 4.38398488
steps 16  value = 4.37650348
steps 64  value = 4.37600964
steps 256  value = 4.37597867
steps 2048  value = 4.37597663
steps 16384  value = 4.37597660
```

### Example 4: with a location

No claim is below 5, so a cap of 3 pays exactly 3, and a large cap tends to the mean 5 + 6 = 11.

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
  out.writeln("cap 3:", actuarial::losses::gamma_lev(2.0, 3.0, 5.0, 3.0).item<double>());
  out.writeln("cap 20:", actuarial::losses::gamma_lev(2.0, 3.0, 5.0, 20.0).item<double>());
  out.writeln("cap 200:", actuarial::losses::gamma_lev(2.0, 3.0, 5.0, 200.0).item<double>());
}
```

Output

```text
cap 3: 3.000000
cap 20: 10.858503
cap 200: 11.000001
```

### Example 5: the stop-loss premium from the limited value

E[(X - d)+] = E[X] - E[min(X, d)].

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
  const double mean = actuarial::losses::gamma_moments(2.0, 3.0).mean.item<double>();
  out.writeln("stop-loss at 6:", mean - actuarial::losses::gamma_lev(2.0, 3.0, 0.0, 6.0).item<double>());
}
```

Output

```text
stop-loss at 6: 1.624023
```

### Example 6: the errors

Invalid parameters and caps are refused.

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
  report([&] { (void)actuarial::losses::gamma_lev(0.0, 3.0, 0.0, 6.0); });
  report([&] { (void)actuarial::losses::gamma_lev(2.0, 3.0, -1.0, 6.0); });
  report([&] { (void)actuarial::losses::gamma_lev(2.0, 3.0, 0.0, -6.0); });
  report([&] { (void)actuarial::losses::gamma_lev(2.0, 3.0, 0.0, 6.0, 0); });
}
```

Output

```text
gamma_lev: shape must be > 0
gamma_lev: location must be >= 0
gamma_lev: cap must be >= 0
gamma_lev: quadrature_steps must be > 0
```

## See Also

- [`gamma_layer`](gamma_layer.md): a layer from the same law
- [`gamma_moments`](gamma_moments.md): the unlimited mean

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lognormal_lev.html">
    <span class="label">Previous</span>
    <span class="title">lognormal_lev</span>
  </a>
  <a class="next-link" href="lognormal_layer.html">
    <span class="label">Next</span>
    <span class="title">lognormal_layer</span>
  </a>
</div>
<!-- apiref:nav:end -->
