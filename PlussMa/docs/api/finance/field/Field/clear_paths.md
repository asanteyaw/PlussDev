<!-- generated from clear_paths.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5f88864b7747 -->

# `module::Field::clear_paths`

[Back to `module::Field`](../Field.md)

Empties `field.paths`.

## Signature

```cpp
void module::Field::clear_paths();
```

## What It Does

Replaces the stored paths by an empty `Paths`, releasing the (possibly large) arrays. Nothing else in the field changes. Calling it on a field with no stored paths is harmless.

## Returns

Nothing.

## Practical Examples

### Example 1

Simulate, then clear.

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
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 100.0, 1.0),
                   .market = fin::market::make(100.0, 0.02, 0.0),
                   .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  field.controls.mc.paths = 2000;
  field.controls.mc.steps = 50;
  field.controls.mc.seed = 7;

  field.simulate();
  out.writeln("stored:", field.paths.asset.defined());
  field.clear_paths();
  out.writeln("after clear:", field.paths.asset.defined());
}
```

Output

```text
stored: true
after clear: false
```

### Example 2

Clearing leaves the field priceable.

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
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 100.0, 1.0),
                   .market = fin::market::make(100.0, 0.02, 0.0),
                   .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  field.controls.mc.paths = 2000;
  field.controls.mc.steps = 50;
  field.controls.mc.seed = 7;

  field.controls.risk = mod::Risk::PriceOnly;
  field.simulate();
  field.clear_paths();
  out.writeln("price still available:", field.price().price.items<double>());
}
```

Output

```text
price still available: [8.646]
```

### Example 3

Clearing an empty field is harmless.

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
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 100.0, 1.0),
                   .market = fin::market::make(100.0, 0.02, 0.0),
                   .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  field.controls.mc.paths = 2000;
  field.controls.mc.steps = 50;
  field.controls.mc.seed = 7;

  field.clear_paths();
  field.clear_paths();
  out.writeln("defined:", field.paths.asset.defined());
}
```

Output

```text
defined: false
```

## See Also

- [`Field`](../Field.md): the class
- [`simulate`](simulate.md): stores paths

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bind_objective_typed.html">
    <span class="label">Previous</span>
    <span class="title">bind_objective_typed</span>
  </a>
  <a class="next-link" href="engine_name.html">
    <span class="label">Next</span>
    <span class="title">engine_name</span>
  </a>
</div>
<!-- apiref:nav:end -->
