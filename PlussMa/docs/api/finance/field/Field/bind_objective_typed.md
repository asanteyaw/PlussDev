<!-- generated from bind_objective_typed.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 39078e5fb989 -->

# `module::Field::bind_objective_typed`

[Back to `module::Field`](../Field.md)

Like `bind_objective`, taking the concrete model struct instead of a `Model`.

## Signature

```cpp
template <SupportedModel M> std::function<double(const M &)> module::Field::bind_objective_typed(const pluss::Series &target_prices, pluss::Config cfg = {}) const;
```

## What It Does

The same frozen objective, but the callable takes a `model::Heston` (or whichever `M` you name) directly, saving the wrapping into a `Model`. Use it when your minimiser already manipulates the typed struct.

## Parameters

`const pluss::Series &target_prices`
: One quote per option.

`pluss::Config cfg`
: Device.

## Returns

A `std::function<double(const M&)>`.

## Errors

- As for `bind_objective`.

## Practical Examples

### Example 1

A typed objective evaluated at the generating Heston and at a perturbed one.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  mod::Field field{.instrument = {.type = fin::pricing::Call, .strike = pluss::series::linspace(80.0, 120.0, 5, pluss::ScalarType::Float64), .maturity = pluss::make_series({1.0})},
                   .market = {.spot = pluss::make_series({100.0}), .rate = pluss::make_series({0.02}), .dividend_yield = pluss::make_series({0.0})},
                   .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::Cos, .risk = mod::Risk::PriceOnly}};

  const Series quotes = field.price().price;
  const auto f = field.bind_objective_typed<fin::model::Heston>(quotes);
  fin::model::Heston h = field.model.as<fin::model::Heston>();
  const double at_truth = f(h);
  h.sigma = pluss::make_series({0.6});
  out.writeln("at the truth :", at_truth, ", with sigma = 0.6 :", f(h));
}
```

Output

```text
at the truth : 0.0000000000 , with sigma = 0.6 : 0.4165572711
```

### Example 2

It agrees with the untyped form.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  mod::Field field{.instrument = {.type = fin::pricing::Call, .strike = pluss::series::linspace(80.0, 120.0, 5, pluss::ScalarType::Float64), .maturity = pluss::make_series({1.0})},
                   .market = {.spot = pluss::make_series({100.0}), .rate = pluss::make_series({0.02}), .dividend_yield = pluss::make_series({0.0})},
                   .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::Cos, .risk = mod::Risk::PriceOnly}};
  const Series prices = field.price().price;
  const Series quotes = prices + pluss::make_series({0.30, -0.20, 0.10, -0.40, 0.25});

  const auto typed = field.bind_objective_typed<fin::model::Heston>(quotes);
  const auto untyped = field.bind_objective(quotes);
  fin::model::Heston h = field.model.as<fin::model::Heston>();
  h.theta = pluss::make_series({0.05});
  out.writeln("typed =", typed(h), ", untyped =", untyped(fin::module::Model{h}));
}
```

Output

```text
typed = 0.2509436441 , untyped = 0.2509436441
```

### Example 3

Black-Scholes: the objective over the volatility has its minimum at the volatility that made the quotes.

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
                   .market = {.spot = pluss::make_series({100.0}), .rate = pluss::make_series({0.05}), .dividend_yield = pluss::make_series({0.0})},
                   .model = fin::model::make(fin::model::Kind::BlackScholes, {0.25}),
                   .controls = {.engine = mod::Engine::Analytic, .risk = mod::Risk::PriceOnly}};

  const Series quotes = field.price().price;
  const auto f = field.bind_objective_typed<fin::model::BlackScholes>(quotes);
  for (const double vol : {0.15, 0.20, 0.25, 0.30})
    out.writeln("vol", vol, ": objective =", f(fin::model::BlackScholes{.volatility = pluss::make_series({vol})}));
}
```

Output

```text
vol 0.15000000 : objective = 9.97610124
vol 0.20000000 : objective = 2.68341049
vol 0.25000000 : objective = 0.00000000
vol 0.30000000 : objective = 2.92414785
```

## See Also

- [`Field`](../Field.md): the class
- [`bind_objective`](bind_objective.md): the untyped form

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bind_objective.html">
    <span class="label">Previous</span>
    <span class="title">bind_objective</span>
  </a>
  <a class="next-link" href="clear_paths.html">
    <span class="label">Next</span>
    <span class="title">clear_paths</span>
  </a>
</div>
<!-- apiref:nav:end -->
