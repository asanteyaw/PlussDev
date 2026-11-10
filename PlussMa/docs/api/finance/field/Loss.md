<!-- generated from Loss.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `module::Loss`

How the residuals of a fit are reduced to one number.

With residuals `rᵢ = Pᵢ − Qᵢ` (model price minus quote) over `n` options, all four losses are means with no weights: `MSE` is `(1/n)Σ rᵢ²`; `RelativeMSE` is `(1/n)Σ (rᵢ/Qᵢ)²`, which weighs cheap options (wings) as heavily as expensive ones; `MAE` is `(1/n)Σ |rᵢ|`; `Huber` is quadratic for `|r| ≤ δ` and linear beyond, with `δ = controls.huber_delta`. MSE lets one wild quote dominate; MAE and Huber are robust to it; RelativeMSE is the natural choice when prices span orders of magnitude.

## Fields

### `MSE`

`enumerator`, default `0`.

Mean squared error.

### `RelativeMSE`

`enumerator`, default `1`.

Mean squared relative error `(r/Q)²`.

### `MAE`

`enumerator`, default `2`.

Mean absolute error.

### `Huber`

`enumerator`, default `3`.

Quadratic within `huber_delta`, linear beyond.

## Guidance

Set as `Controls::loss`.

## Practical Examples

### Example 1

The four losses of one set of residuals.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  mod::Field field{.instrument = {.type = fin::pricing::Call, .strike = pluss::series::linspace(80.0, 120.0, 5, pluss::ScalarType::Float64), .maturity = pluss::make_series({1.0})},
                   .market = {.spot = pluss::make_series({100.0}), .rate = pluss::make_series({0.02}), .dividend_yield = pluss::make_series({0.0})},
                   .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::Cos, .risk = mod::Risk::PriceOnly}};

  const Series quotes = field.price().price + pluss::make_series({0.30, -0.20, 0.10, -0.40, 0.25});
  for (const mod::Loss loss : {mod::Loss::MSE, mod::Loss::RelativeMSE, mod::Loss::MAE, mod::Loss::Huber}) {
    field.controls.loss = loss;
    out.writeln(static_cast<int>(loss), ":", field.loss_value(quotes));
  }
}
```

Output

```text
0 : 0.07250000
1 : 0.00660492
2 : 0.25000000
3 : 0.01125000
```

### Example 2

MSE against Huber with an outlier. One quote is 5.0 off: MSE is dominated by it, Huber grows only linearly.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  mod::Field field{.instrument = {.type = fin::pricing::Call, .strike = pluss::series::linspace(80.0, 120.0, 5, pluss::ScalarType::Float64), .maturity = pluss::make_series({1.0})},
                   .market = {.spot = pluss::make_series({100.0}), .rate = pluss::make_series({0.02}), .dividend_yield = pluss::make_series({0.0})},
                   .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::Cos, .risk = mod::Risk::PriceOnly}};

  const Series quotes = field.price().price + pluss::make_series({0.0, 0.0, 5.0, 0.0, 0.0});
  for (const mod::Loss loss : {mod::Loss::MSE, mod::Loss::MAE, mod::Loss::Huber}) {
    field.controls.loss = loss;
    out.writeln(static_cast<int>(loss), ":", field.loss_value(quotes));
  }
}
```

Output

```text
0 : 5.000000
2 : 1.000000
3 : 0.049750
```

### Example 3

Relative weighting. The same absolute error on a cheap and an expensive option costs more under RelativeMSE for the cheap one.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  mod::Field field{.instrument = {.type = fin::pricing::Call, .strike = pluss::series::linspace(80.0, 120.0, 5, pluss::ScalarType::Float64), .maturity = pluss::make_series({1.0})},
                   .market = {.spot = pluss::make_series({100.0}), .rate = pluss::make_series({0.02}), .dividend_yield = pluss::make_series({0.0})},
                   .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::Cos, .risk = mod::Risk::PriceOnly}};

  const Series base = field.price().price;
  field.controls.loss = mod::Loss::RelativeMSE;
  out.writeln("error 0.1 on the dearest option :", field.loss_value(base + pluss::make_series({0.1, 0.0, 0.0, 0.0, 0.0})));
  out.writeln("error 0.1 on the cheapest option:", field.loss_value(base + pluss::make_series({0.0, 0.0, 0.0, 0.0, 0.1})));
}
```

Output

```text
error 0.1 on the dearest option : 0.00000373
error 0.1 on the cheapest option: 0.00078935
```

### Example 4

The Huber loss needs a positive `huber_delta`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  mod::Field field{.instrument = {.type = fin::pricing::Call, .strike = pluss::series::linspace(80.0, 120.0, 5, pluss::ScalarType::Float64), .maturity = pluss::make_series({1.0})},
                   .market = {.spot = pluss::make_series({100.0}), .rate = pluss::make_series({0.02}), .dividend_yield = pluss::make_series({0.0})},
                   .model = fin::model::make(fin::model::Kind::BlackScholes, {0.2}),
                   .controls = {.engine = mod::Engine::Analytic, .risk = mod::Risk::PriceOnly}};

  field.controls.loss = mod::Loss::Huber;
  field.controls.huber_delta = 0.0;
  try {
    (void)(field.loss_value(field.price().price));
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
subsp::math::finance::module::Field: controls.huber_delta must be finite and > 0 for the Huber loss
```

## See Also

- [`loss`](Field/loss.md): uses it
- [`Controls`](Controls.md): holds the choice

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Greeks.html">
    <span class="label">Previous</span>
    <span class="title">Greeks</span>
  </a>
  <a class="next-link" href="Paths.html">
    <span class="label">Next</span>
    <span class="title">Paths</span>
  </a>
</div>
<!-- apiref:nav:end -->
