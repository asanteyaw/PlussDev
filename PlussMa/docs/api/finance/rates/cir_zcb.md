<!-- generated from cir_zcb.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `rates::cir_zcb`

The closed-form Cox-Ingersoll-Ross price of a zero-coupon bond.

## Signature

```cpp
double rates::cir_zcb(double maturity, double short_rate, const rates::CirParams &params);
```

## What It Does

`P(0, T) = A(T) e^{−B(T) r}` with `γ = √(κ² + 2σ²)`, `B = 2(e^{γT} − 1)/[(γ + κ)(e^{γT} − 1) + 2γ]` and `A = [2γ e^{(κ+γ)T/2}/((γ + κ)(e^{γT} − 1) + 2γ)]^{2κθ/σ²}`. The rate must be at least `rate_floor`.

## Parameters

`double maturity`
: Years to maturity, non-negative; zero returns 1.

`double short_rate`
: The current short rate.

`const rates::CirParams &params`
: The model.

## Returns

The price of a bond paying 1 at maturity.

## Errors

- A negative or non-finite maturity, a non-finite rate or invalid parameters throw (`pluss::Error`).

## Practical Examples

### Example 1

A term structure of prices and yields.

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
  const rt::CirParams p{.kappa = 0.7, .theta = 0.03, .sigma = 0.12, .r0 = 0.05, .rate_floor = 1e-12};
  for (const double T : {0.25, 1.0, 5.0, 30.0}) {
    const double price = rt::cir_zcb(T, 0.05, p);
    out.writeln("T", T, ": price", price, ", yield", -std::log(price) / T);
  }
}
```

Output

```text
T 0.25000 : price 0.98799 , yield 0.04834
T 1.00000 : price 0.95665 , yield 0.04431
T 5.00000 : price 0.83853 , yield 0.03522
T 30.00000 : price 0.40003 , yield 0.03054
```

### Example 2

Against simulation by the exact transition scheme.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace rt = fin::rates;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const rt::CirParams p{.kappa = 0.7, .theta = 0.03, .sigma = 0.12, .r0 = 0.05, .rate_floor = 1e-12};
  rt::SimOptions options;
  options.scheme = rt::Scheme::Transition;
  options.seed = 3;
  const double dt = 1.0 / 52.0;
  const Series rate = rt::cir_paths(260, 20000, dt, p, options).rate;
  const Series integral = (rate.slice(0, 0, 260, 1) + rate.slice(0, 1, 261, 1)).sum(0, false) * (0.5 * dt);
  out.writeln("simulated", (integral * -1.0).exp().mean().item<double>(), ", closed form", rt::cir_zcb(5.0, 0.05, p));
}
```

Output

```text
simulated 0.83834 , closed form 0.83853
```

### Example 3

Zero maturity and the rate floor: a rate below the floor is refused.

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
  const rt::CirParams p{};
  out.writeln("T = 0:", rt::cir_zcb(0.0, 0.03, p));
  try {
    (void)(rt::cir_zcb(1.0, 0.0, p));
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
T = 0: 1.0000
subsp::math::finance::rates::cir_zcb: short_rate must be finite and >= rate_floor
```

## See Also

- [`CirParams`](CirParams.md): the model
- [`cir_paths`](cir_paths.md): simulation of the same model

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cir_paths.html">
    <span class="label">Previous</span>
    <span class="title">cir_paths</span>
  </a>
  <a class="next-link" href="hjm_paths.html">
    <span class="label">Next</span>
    <span class="title">hjm_paths</span>
  </a>
</div>
<!-- apiref:nav:end -->
