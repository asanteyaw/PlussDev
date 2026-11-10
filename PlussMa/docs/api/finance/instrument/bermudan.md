<!-- generated from bermudan.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `instrument::bermudan`

A Bermudan call or put: exercisable on a schedule of dates and at maturity.

## Signature

```cpp
instrument::Vanilla instrument::bermudan(pricing::OptionType type, double strike, double maturity, std::span<const double> dates, pluss::Config cfg = f64_default());
```

## What It Does

Like [`make`](make.md) but with `exercise = Bermudan` and the given interim exercise `dates` (in years, between 0 and the maturity); maturity is always exercisable. At least one date is required. A Bermudan is worth at least the European and at most the American; with more dates it approaches the American. Engines: `Lattice` and `Cos` (Black-Scholes), `Pde` (Heston), `Cos` (Heston, Bates), `MonteCarlo` (any model).

## Parameters

`pricing::OptionType type`
: `Call` or `Put`.

`double strike, maturity`
: As for `make`.

`std::span<const double> dates`
: Interim exercise dates in years; at least one.

`pluss::Config cfg`
: Type and device.

## Returns

A `Vanilla`.

## Errors

- An unsupported combination is reported when the field is priced, not when the instrument is built.

## Practical Examples

### Example 1

The value rises with the number of dates, toward the American.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <vector>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  for (const std::vector<double> &dates : {std::vector<double>{0.5}, std::vector<double>{0.25, 0.5, 0.75}, std::vector<double>{0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9}}) {
    mod::Field field{.instrument = fin::instrument::bermudan(fin::pricing::Put, 100.0, 1.0, dates), .market = fin::market::make(100.0, 0.05, 0.0),
                     .model = fin::model::make(fin::model::Kind::BlackScholes, {0.2}), .controls = {.engine = mod::Engine::Lattice, .risk = mod::Risk::PriceOnly}};
    field.controls.lattice.steps = 3000;
    out.writeln(dates.size(), "dates:", field.price().price.items<double>()[0]);
  }
}
```

Output

```text
1 dates: 5.83819
3 dates: 5.95624
9 dates: 6.03346
```

### Example 2

The cosine engine prices it as well.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <vector>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const std::vector<double> dates = {0.25, 0.5, 0.75};
  mod::Field field{.instrument = fin::instrument::bermudan(fin::pricing::Put, 100.0, 1.0, dates), .market = fin::market::make(100.0, 0.05, 0.0),
                   .model = fin::model::make(fin::model::Kind::BlackScholes, {0.2}), .controls = {.engine = mod::Engine::Cos, .risk = mod::Risk::PriceOnly}};
  out.writeln("Cos Bermudan put:", field.price().price.items<double>()[0]);
}
```

Output

```text
Cos Bermudan put: 5.95663
```

### Example 3

No dates is refused when the field is priced.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <vector>
#include <string>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const std::vector<double> none;
  mod::Field field{.instrument = fin::instrument::bermudan(fin::pricing::Put, 100.0, 1.0, none), .market = fin::market::make(100.0, 0.05, 0.0),
                   .model = fin::model::make(fin::model::Kind::BlackScholes, {0.2}), .controls = {.engine = mod::Engine::Lattice}};
  try {
    (void)(field.price());
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find("Triggered")));
  }
}
```

Output

```text
subsp::math::finance::module::Field (2 problems):
  - instrument.exercise_times must not be empty
  - instrument.exercise_times a Bermudan option needs at least one date
```

## See Also

- [`Vanilla`](Vanilla.md): what it builds

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="american.html">
    <span class="label">Previous</span>
    <span class="title">american</span>
  </a>
  <a class="next-link" href="make.html">
    <span class="label">Next</span>
    <span class="title">make</span>
  </a>
</div>
<!-- apiref:nav:end -->
