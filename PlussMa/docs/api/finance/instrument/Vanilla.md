<!-- generated from Vanilla.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `instrument::Vanilla`

A strip of vanilla calls and puts: types, strikes, maturities and exercise style.

Every option in the strip has a strike and a maturity (and a call/put flag). `strike` and `maturity` are `Series`; each must have one element (shared by all options) or the same number of elements as the others in the field (strike, maturity, spot, rate, yield), so a single call prices a grid of strikes at one maturity, a term structure at one strike, or paired strikes and maturities. `type` is `Call` or `Put` for the whole strip; set it to `Auto` and supply `is_call` (1 for a call, 0 for a put) to mix calls and puts in one strip. `exercise` selects the style ([`ExerciseStyle`](ExerciseStyle.md)) and `exercise_times` lists the interim dates (in years) of a Bermudan; maturity is always exercisable. The factories [`make`](make.md), [`american`](american.md) and [`bermudan`](bermudan.md) build the common cases from numbers.

## Fields

### `type`

`pricing::OptionType`, default `Call`.

`Call`, `Put`, or `Auto` (then `is_call` decides per option).

### `is_call`

`pluss::Series`, default `undefined`.

Per-option flag, 1 for a call and 0 for a put; required when `type` is `Auto`.

### `strike`

`pluss::Series`, default `undefined`.

Strikes, positive; one element or one per option.

### `maturity`

`pluss::Series`, default `undefined`.

Maturities in years, non-negative for European (positive for early exercise); one element or one per option.

### `exercise`

`pricing::ExerciseStyle`, default `European`.

The exercise style.

### `exercise_times`

`pluss::Series`, default `undefined`.

Bermudan interim exercise dates in years, positive; ignored (with a warning) unless `exercise` is `Bermudan`.

## Guidance

Member `Field::instrument`.

## Practical Examples

### Example 1

The type. A strip of puts instead of calls: the same strikes priced as puts.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  mod::Field field{.instrument = {.type = fin::pricing::Call, .strike = pluss::series::linspace(80.0, 120.0, 5, pluss::ScalarType::Float64), .maturity = pluss::make_series({1.0})},
                   .market = {.spot = pluss::make_series({100.0}), .rate = pluss::make_series({0.05}), .dividend_yield = pluss::make_series({0.0})},
                   .model = fin::model::make(fin::model::Kind::BlackScholes, {0.2}),
                   .controls = {.engine = mod::Engine::Analytic, .risk = mod::Risk::PriceOnly}};

  const Series calls = field.price().price;
  field.instrument.type = fin::pricing::Put;
  out.writeln("calls :", calls.items<double>());
  out.writeln("puts  :", field.price().price.items<double>());
}
```

Output

```text
calls : [24.5888, 16.6994, 10.4506, 6.0401, 3.2475]
puts  : [0.6872, 2.3101, 5.5735, 10.6753, 17.3950]
```

### Example 2

`is_call` with type `Auto`: one strip mixing calls and puts, a straddle's two legs priced in one call.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Field field{.instrument = {.type = fin::pricing::OptionType::Auto, .is_call = pluss::make_series({1.0, 0.0}), .strike = pluss::make_series({100.0, 100.0}), .maturity = pluss::make_series({1.0})},
                   .market = fin::market::make(100.0, 0.05, 0.0), .model = fin::model::make(fin::model::Kind::BlackScholes, {0.2}),
                   .controls = {.engine = mod::Engine::Analytic, .risk = mod::Risk::PriceOnly}};
  out.writeln("call, put:", field.price().price.items<double>());
}
```

Output

```text
call, put: [10.45058, 5.57353]
```

### Example 3

`Auto` without `is_call` is refused.

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
  mod::Field field{.instrument = {.type = fin::pricing::OptionType::Auto, .strike = pluss::make_series({100.0}), .maturity = pluss::make_series({1.0})},
                   .market = fin::market::make(100.0, 0.05, 0.0), .model = fin::model::make(fin::model::Kind::BlackScholes, {0.2}), .controls = {.engine = mod::Engine::Analytic}};
  try {
    (void)(field.price());
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
subsp::math::finance::module::Field: instrument.is_call must be defined when instrument.type is Auto
```

### Example 4

The strike: a strip of five strikes at one maturity, the usual smile strip.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  mod::Field field{.instrument = {.type = fin::pricing::Call, .strike = pluss::series::linspace(80.0, 120.0, 5, pluss::ScalarType::Float64), .maturity = pluss::make_series({1.0})},
                   .market = {.spot = pluss::make_series({100.0}), .rate = pluss::make_series({0.05}), .dividend_yield = pluss::make_series({0.0})},
                   .model = fin::model::make(fin::model::Kind::BlackScholes, {0.2}),
                   .controls = {.engine = mod::Engine::Analytic, .risk = mod::Risk::PriceOnly}};

  out.writeln("strikes:", field.instrument.strike.items<double>());
  out.writeln("prices :", field.price().price.items<double>());
}
```

Output

```text
strikes: [80.000, 90.000, 100.000, 110.000, 120.000]
prices : [24.589, 16.699, 10.451, 6.040, 3.247]
```

### Example 5

The maturity: a term structure at one strike. Four maturities with a one-element strike.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  mod::Field field{.instrument = {.type = fin::pricing::Call, .strike = pluss::make_series({100.0}), .maturity = pluss::make_series({0.25, 0.5, 1.0, 2.0})},
                   .market = fin::market::make(100.0, 0.05, 0.0), .model = fin::model::make(fin::model::Kind::BlackScholes, {0.2}),
                   .controls = {.engine = mod::Engine::Analytic, .risk = mod::Risk::PriceOnly}};
  out.writeln("maturities:", field.instrument.maturity.items<double>());
  out.writeln("prices    :", field.price().price.items<double>());
}
```

Output

```text
maturities: [0.2500, 0.5000, 1.0000, 2.0000]
prices    : [4.6150, 6.8887, 10.4506, 16.1268]
```

### Example 6

Pairs: three strikes with three maturities are priced element by element, not as a 3×3 grid.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  mod::Field field{.instrument = {.type = fin::pricing::Call, .strike = pluss::make_series({90.0, 100.0, 110.0}), .maturity = pluss::make_series({0.5, 1.0, 2.0})},
                   .market = fin::market::make(100.0, 0.05, 0.0), .model = fin::model::make(fin::model::Kind::BlackScholes, {0.2}),
                   .controls = {.engine = mod::Engine::Analytic, .risk = mod::Risk::PriceOnly}};
  out.writeln("prices:", field.price().price.items<double>());
}
```

Output

```text
prices: [13.4985, 10.4506, 11.4555]
```

### Example 7

The exercise style and the interim dates of a Bermudan, set field by field.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Put, 100.0, 1.0),
                   .market = fin::market::make(100.0, 0.05, 0.0),
                   .model = fin::model::make(fin::model::Kind::BlackScholes, {0.2}),
                   .controls = {.engine = mod::Engine::Lattice, .risk = mod::Risk::PriceOnly}};

  field.instrument.exercise = fin::pricing::ExerciseStyle::Bermudan;
  field.instrument.exercise_times = pluss::make_series({0.5});
  field.controls.lattice.steps = 2000;
  out.writeln("Bermudan with one interim date:", field.price().price.items<double>()[0]);
  field.instrument.exercise = fin::pricing::ExerciseStyle::European;
  out.writeln("European                      :", field.price().price.items<double>()[0]);
}
```

Output

```text
Bermudan with one interim date: 5.83810
European                      : 5.57253
```

## See Also

- [`make`](make.md): build from numbers
- [`ExerciseStyle`](ExerciseStyle.md): the styles

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="State.html">
    <span class="label">Previous</span>
    <span class="title">State</span>
  </a>
  <a class="next-link" href="american.html">
    <span class="label">Next</span>
    <span class="title">american</span>
  </a>
</div>
<!-- apiref:nav:end -->
