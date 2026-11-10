<!-- generated from hjm_zcb_mc.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `rates::hjm_zcb_mc`

The price of a zero-coupon bond in the HJM model, by simulation.

## Signature

```cpp
double rates::hjm_zcb_mc(double maturity, const rates::HjmParams &params, std::int64_t paths = 20000, std::int64_t steps = 252, rates::SimOptions options = {}, pluss::Config cfg = {});
```

## What It Does

Simulates the short rate to `maturity` in `steps` steps and returns the average of `exp(−∫r dt)` over the paths, the integral being the left-point sum. There is no closed form for the model. Because the dynamics are arbitrage-free, a flat starting curve prices the bond at `e^{−r₀T}` up to sampling and discretisation error; the sampling error falls as `1/√paths`. The step is `maturity/steps` and must not exceed the smallest tenor spacing.

## Parameters

`double maturity`
: Years, positive.

`const rates::HjmParams &params`
: The model.

`std::int64_t paths`
: Number of paths.

`std::int64_t steps`
: Number of steps to maturity.

`rates::SimOptions options`
: Random-number settings.

`pluss::Config cfg`
: Device.

## Returns

The bond price.

## Errors

- A non-positive maturity, paths or steps, invalid parameters, or a step above the smallest tenor spacing throw.

## Practical Examples

### Example 1

A flat curve prices bonds at `e^{−r₀ T}`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace rt = fin::rates;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const rt::HjmParams p;
  rt::SimOptions options;
  options.seed = 3;
  for (const double T : {0.5, 1.0, 2.0, 5.0}) {
    const double price = rt::hjm_zcb_mc(T, p, 200000, 252, options);
    out.writeln("T", T, ": simulated", price, ", exp(-r0 T)", std::exp(-0.03 * T));
  }
}
```

Output

```text
T 0.50000 : simulated 0.98512 , exp(-r0 T) 0.98511
T 1.00000 : simulated 0.97047 , exp(-r0 T) 0.97045
T 2.00000 : simulated 0.94183 , exp(-r0 T) 0.94176
T 5.00000 : simulated 0.86108 , exp(-r0 T) 0.86071
```

### Example 2

More paths, smaller sampling error: the spread of the price over seeds falls as `1/√paths`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace rt = fin::rates;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const rt::HjmParams p;
  for (const std::int64_t paths : {2000, 20000}) {
    double sum = 0.0, sum2 = 0.0;
    for (std::uint64_t seed = 1; seed <= 8; ++seed) {
      rt::SimOptions options;
      options.seed = seed;
      const double price = rt::hjm_zcb_mc(2.0, p, paths, 100, options);
      sum += price; sum2 += price * price;
    }
    const double mean = sum / 8.0;
    out.writeln("paths", paths, ": mean", mean, ", spread over 8 seeds", std::sqrt((sum2 - 8.0 * mean * mean) / 7.0));
  }
}
```

Output

```text
paths 2000 : mean 0.942027 , spread over 8 seeds 0.000365
paths 20000 : mean 0.941848 , spread over 8 seeds 0.000111
```

### Example 3

The model starts from the curve, so volatility does not change the price in principle: `P(0, T) = e^{−r₀T}` for any volatilities. The simulated prices with the volatilities doubled differ from the flat value only by discretisation and sampling error (about a tenth of a percent here).

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace rt = fin::rates;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  rt::HjmParams low, high;
  for (double &v : high.vol) v *= 2.0;
  rt::SimOptions options;
  options.seed = 3;
  out.writeln("base", rt::hjm_zcb_mc(5.0, low, 200000, 250, options), "doubled", rt::hjm_zcb_mc(5.0, high, 200000, 250, options), "flat", std::exp(-0.15));
}
```

Output

```text
base 0.86108 doubled 0.86189 flat 0.86071
```

### Example 4

Invalid arguments: a non-positive maturity, and a step larger than the tenor spacing.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace rt = fin::rates;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const rt::HjmParams p;
  try {
    (void)(rt::hjm_zcb_mc(0.0, p));
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
  try {
    (void)(rt::hjm_zcb_mc(10.0, p, 100, 10));
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
subsp::math::finance::rates::hjm_zcb_mc: maturity must be finite and > 0
subsp::math::finance::rates::hjm_paths: dt must not exceed the smallest spacing of the tenor grid (0.250000), or the transport of the forward curve along the tenor axis is unstable
```

## See Also

- [`hjm_paths`](hjm_paths.md): the simulation
- [`HjmParams`](HjmParams.md): the model

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="hjm_paths.html">
    <span class="label">Previous</span>
    <span class="title">hjm_paths</span>
  </a>
  <a class="next-link" href="hull_white_paths.html">
    <span class="label">Next</span>
    <span class="title">hull_white_paths</span>
  </a>
</div>
<!-- apiref:nav:end -->
