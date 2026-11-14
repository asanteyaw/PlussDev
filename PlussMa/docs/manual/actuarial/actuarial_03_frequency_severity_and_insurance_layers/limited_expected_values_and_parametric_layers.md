<!-- generated from limited_expected_values_and_parametric_layers.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Limited Expected Values and Parametric Layers

[Back to Frequency, Severity and Insurance Layers](../actuarial_03_frequency_severity_and_insurance_layers.md)

When the severity is a known law, the expected payment under a limit or a layer can be computed from the law, with no sample. The **limited expected value** at a cap {math}`c` is the expected loss when each loss is capped at {math}`c`,

```{math}
\mathrm{LEV}(c)=E\bigl[\min(X,c)\bigr]=\int_0^c \bigl(1-F(x)\bigr)\,dx ,
```

and the expected payment of the layer of {math}`\ell` excess of {math}`a` is a difference of two of them,

```{math}
E\bigl[\min\bigl((X-a)_+,\ell\bigr)\bigr]=\mathrm{LEV}(a+\ell)-\mathrm{LEV}(a).
```

It follows that expected layer payments of adjacent layers add up to the limited expected value over the whole range, and that {math}`E[X]=\mathrm{LEV}(c)+E[(X-c)_+]`: the mean splits into what is paid up to a cap and the excess above it. The **loss elimination ratio** of a deductible {math}`d` is {math}`\mathrm{LEV}(d)/E[X]`, the share of the ground-up loss the deductible removes.

The functions evaluate the integral numerically, with a number of quadrature steps that can be raised for accuracy.

```cpp
pluss::Series losses::lognormal_lev(double mean_log, double stddev_log, double cap, std::int64_t quadrature_steps = 2048, pluss::Platform platform = pluss::Platform::CPU);
pluss::Series losses::gamma_lev(double shape, double scale, double location, double cap, std::int64_t quadrature_steps = 2048, pluss::Platform platform = pluss::Platform::CPU);
pluss::Series losses::lognormal_layer(double mean_log, double stddev_log, double attachment, double limit, std::int64_t quadrature_steps = 2048, pluss::Platform platform = pluss::Platform::CPU);
pluss::Series losses::gamma_layer(double shape, double scale, double location, double attachment, double limit, std::int64_t quadrature_steps = 2048, pluss::Platform platform = pluss::Platform::CPU);
```

| Function | Returns |
|---|---|
| `lognormal_lev(mean_log, stddev_log, cap)` | {math}`E[\min(X,c)]` for a lognormal severity |
| `gamma_lev(shape, scale, location, cap)` | {math}`E[\min(X,c)]` for a gamma severity with a minimum loss `location` |
| `lognormal_layer(mean_log, stddev_log, attachment, limit)` | the expected payment of the layer for a lognormal severity |
| `gamma_layer(shape, scale, location, attachment, limit)` | the expected payment of the layer for a gamma severity |
| `quadrature_steps` | the resolution of the numerical integral, 2048 by default; the values settle to the digits shown below within a few hundred steps |

Each returns a rank-zero `Series`.

## The limited expected value against a sample

A lognormal severity with `mean_log = 8` and `stddev_log = 1` has mean 4,914.77. The program compares its limited expected value at a cap of 20,000 and the expected payment of the layer of 20,000 excess of 5,000 with the empirical values of a sample of 400,000 losses drawn from the same law. They agree to the accuracy of the sample.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
namespace losses = actuarial::losses;
namespace distr = pluss::subsp::stats::distr;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const distr::Lognormal law{8.0, 1.0};
  const pluss::Series sample = law.sample({400000}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 17ULL);
  out.writeln("mean              exact", std::exp(8.5), " sample", sample.mean().item<double>());
  out.writeln("LEV at 20000      law  ", losses::lognormal_lev(8.0, 1.0, 20000.0).item<double>(), " sample", sample.clamp_max(20000.0).mean().item<double>());
  out.writeln("20000 xs 5000     law  ", losses::lognormal_layer(8.0, 1.0, 5000.0, 20000.0).item<double>(), " sample", losses::layer_premium(sample, 5000.0, 20000.0).item<double>());
}
```

Output

```text
mean              exact 4914.768840  sample 4908.016220
LEV at 20000      law   4584.479127  sample 4584.034818
20000 xs 5000     law   1635.412251  sample 1631.250302
```

## Layers add up

The expected payments of the layers 0 to 5,000 and 5,000 to 20,000 add to the limited expected value at 20,000, and the layer from zero is itself the limited expected value at its top. Both are exact to the quadrature: the layer function is the difference of two limited expected values.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
namespace losses = actuarial::losses;
namespace distr = pluss::subsp::stats::distr;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const double first = losses::lognormal_layer(8.0, 1.0, 0.0, 5000.0).item<double>();
  const double second = losses::lognormal_layer(8.0, 1.0, 5000.0, 15000.0).item<double>();
  out.writeln("0 to 5000       ", first, " LEV(5000)  ", losses::lognormal_lev(8.0, 1.0, 5000.0).item<double>());
  out.writeln("5000 to 20000   ", second);
  out.writeln("their sum       ", first + second, " LEV(20000)", losses::lognormal_lev(8.0, 1.0, 20000.0).item<double>());
}
```

Output

```text
0 to 5000        3058.820443  LEV(5000)   3058.820443
5000 to 20000    1525.658684
their sum        4584.479127  LEV(20000) 4584.479127
```

## The mean splits at a cap

For any sample the mean is the average of the capped losses plus the stop-loss premium at the cap, exactly. The program checks it at a cap of 5,000 on the same sample, and prints the loss elimination ratio of a few deductibles under the lognormal law.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
namespace losses = actuarial::losses;
namespace distr = pluss::subsp::stats::distr;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const distr::Lognormal law{8.0, 1.0};
  const pluss::Series sample = law.sample({400000}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 17ULL);
  out.writeln("mean of the sample       ", sample.mean().item<double>());
  out.writeln("capped mean + stop-loss  ", sample.clamp_max(5000.0).mean().item<double>() + losses::stoploss(sample, 5000.0).item<double>());
  const double mean = losses::lognormal_moments(8.0, 1.0).mean.item<double>();
  for (const double d : {500.0, 2000.0, 10000.0}) {
    out.writeln("loss elimination ratio of a deductible of", d, "=", losses::lognormal_lev(8.0, 1.0, d).item<double>() / mean);
  }
}
```

Output

```text
mean of the sample        4908.016220
capped mean + stop-loss   4908.016220
loss elimination ratio of a deductible of 500.000000 = 0.100633
loss elimination ratio of a deductible of 2000.000000 = 0.347472
loss elimination ratio of a deductible of 10000.000000 = 0.813369
```

## A gamma severity and its location

The gamma functions take a `location`, a minimum loss. The program compares the limited expected value at 8,000 and the layer of 6,000 excess of 2,000 for a gamma severity of shape 2 and scale 3,000 with a sample of 400,000 from the same law.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
namespace losses = actuarial::losses;
namespace distr = pluss::subsp::stats::distr;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  distr::Gamma law;
  law.shape = 2.0;
  law.scale = 3000.0;
  const pluss::Series sample = law.sample({400000}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 5ULL);
  out.writeln("LEV at 8000      law", losses::gamma_lev(2.0, 3000.0, 0.0, 8000.0).item<double>(), " sample", sample.clamp_max(8000.0).mean().item<double>());
  out.writeln("6000 xs 2000     law", losses::gamma_layer(2.0, 3000.0, 0.0, 2000.0, 6000.0).item<double>(), " sample", losses::layer_premium(sample, 2000.0, 6000.0).item<double>());
}
```

Output

```text
LEV at 8000      law 5027.231722  sample 5028.937493
6000 xs 2000     law 3134.568670  sample 3135.533559
```

## The quadrature resolution

The integral is evaluated numerically, so a coarse resolution is slightly off. The program computes the layer of 20,000 excess of 5,000 with a few step counts: 16 steps are off in the third digit, 64 overshoot slightly, and from 256 steps on the value is stable to four decimals, so the default is far more resolution than these digits need.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
namespace losses = actuarial::losses;
namespace distr = pluss::subsp::stats::distr;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  for (const std::int64_t steps : {16, 64, 256, 2048, 8192}) {
    out.writeln("steps", steps, " layer", losses::lognormal_layer(8.0, 1.0, 5000.0, 20000.0, steps).item<double>());
  }
}
```

Output

```text
steps 16  layer 1632.6648
steps 64  layer 1635.5655
steps 256  layer 1635.4118
steps 2048  layer 1635.4123
steps 8192  layer 1635.4123
```

## Where to go next

Expected payments, frequency and severity together give the cost of a policy ([A Policy from Frequency, Severity and Terms](a_policy_from_frequency_severity_and_terms.md)).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="layers_and_stop_loss.html">
    <span class="label">Previous</span>
    <span class="title">Layers and Stop-Loss</span>
  </a>
  <a class="next-link" href="a_policy_from_frequency_severity_and_terms.html">
    <span class="label">Next</span>
    <span class="title">A Policy from Frequency, Severity and Terms</span>
  </a>
</div>
<!-- apiref:nav:end -->
