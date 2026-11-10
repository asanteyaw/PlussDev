<!-- generated from vasicek_zcb.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `rates::vasicek_zcb`

The closed-form Vasicek price of a zero-coupon bond.

## Signature

```cpp
double rates::vasicek_zcb(double maturity, double short_rate, const rates::VasicekParams &params);
```

## What It Does

`P(0, T) = A(T) e^{−B(T) r}` with `B = (1 − e^{−κT})/κ` and `ln A = (θ − σ²/(2κ²))(B − T) − σ²B²/(4κ)`. It is exact, so it is the check for a simulation of the model: the simulated average of `exp(−∫r)` should equal it.

## Parameters

`double maturity`
: Years to maturity, non-negative; zero returns 1.

`double short_rate`
: The current short rate.

`const rates::VasicekParams &params`
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
  const rt::VasicekParams p{.kappa = 0.7, .theta = 0.03, .sigma = 0.01, .r0 = 0.05};
  for (const double T : {0.25, 1.0, 5.0, 30.0}) {
    const double price = rt::vasicek_zcb(T, 0.05, p);
    out.writeln("T", T, ": price", price, ", yield", -std::log(price) / T);
  }
}
```

Output

```text
T 0.25000 : price 0.98799 , yield 0.04835
T 1.00000 : price 0.95660 , yield 0.04437
T 5.00000 : price 0.83744 , yield 0.03548
T 30.00000 : price 0.39624 , yield 0.03086
```

### Example 2

Against simulation: the average discount factor over exact paths.

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
  const rt::VasicekParams p{.kappa = 0.7, .theta = 0.03, .sigma = 0.01, .r0 = 0.05};
  rt::SimOptions options;
  options.seed = 3;
  const double dt = 1.0 / 252.0;
  const Series rate = rt::vasicek_paths(1260, 20000, dt, p, options).rate;
  const Series integral = rate.slice(0, 0, 1260, 1).sum(0, false) * dt;
  out.writeln("simulated", (integral * -1.0).exp().mean().item<double>(), ", closed form", rt::vasicek_zcb(5.0, 0.05, p));
}
```

Output

```text
simulated 0.83737 , closed form 0.83744
```

### Example 3

Zero maturity gives 1; a higher rate gives a lower price.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace rt = fin::rates;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const rt::VasicekParams p{};
  out.writeln("T = 0:", rt::vasicek_zcb(0.0, 0.03, p), "; r = 0.02, 0.05 at T = 2:", rt::vasicek_zcb(2.0, 0.02, p), rt::vasicek_zcb(2.0, 0.05, p));
}
```

Output

```text
T = 0: 1.000000 ; r = 0.02, 0.05 at T = 2: 0.952006 0.921758
```

### Example 4

A negative maturity is refused.

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
  const rt::VasicekParams p{};
  try {
    (void)(rt::vasicek_zcb(-1.0, 0.03, p));
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
subsp::math::finance::rates::vasicek_zcb: maturity must be finite and >= 0
```

## See Also

- [`VasicekParams`](VasicekParams.md): the model
- [`vasicek_paths`](vasicek_paths.md): simulation of the same model

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="vasicek_paths.html">
    <span class="label">Previous</span>
    <span class="title">vasicek_paths</span>
  </a>
</div>
<!-- apiref:nav:end -->
