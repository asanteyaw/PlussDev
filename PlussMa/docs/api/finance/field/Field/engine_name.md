<!-- generated from engine_name.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6ac5641828ef -->

# `module::Field::engine_name`

[Back to `module::Field`](../Field.md)

The name of the engine the field is set to use, as text.

## Signature

```cpp
std::string module::Field::engine_name() const;
```

## What It Does

Returns one of `"Analytic"`, `"Fft"`, `"Cos"`, `"MonteCarlo"`, `"Lattice"`, `"Spectral"`, `"Pde"`, the text of `controls.engine`. Use it to label output, so a printed price says how it was obtained.

## Returns

The engine name.

## Practical Examples

### Example 1

The name of the default engine and of a chosen one.

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
                   .market = fin::market::make(100.0, 0.05, 0.02),
                   .model = fin::model::make(fin::model::Kind::BlackScholes, {0.2}),
                   .controls = {.engine = mod::Engine::Analytic, .risk = mod::Risk::All}};

  out.writeln(field.engine_name());
  field.controls.engine = mod::Engine::Cos;
  out.writeln(field.engine_name());
}
```

Output

```text
Analytic
Cos
```

### Example 2

Labelling a comparison. The same Heston option priced by three engines, each printed with its name.

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
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 100.0, 1.0),
                   .market = fin::market::make(100.0, 0.02, 0.0),
                   .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::Cos, .risk = mod::Risk::PriceOnly}};

  for (const mod::Engine engine : {mod::Engine::Cos, mod::Engine::Fft, mod::Engine::Pde}) {
    field.controls.engine = engine;
    out.writeln(field.engine_name(), ":", field.price().price.items<double>()[0]);
  }
}
```

Output

```text
Cos : 8.62811
Fft : 8.62811
Pde : 8.62745
```

### Example 3

The name is available before pricing and does not validate: a combination that cannot be priced still has a name.

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
                   .market = fin::market::make(100.0, 0.05, 0.02),
                   .model = fin::model::make(fin::model::Kind::BlackScholes, {0.2}),
                   .controls = {.engine = mod::Engine::Analytic, .risk = mod::Risk::All}};

  field.controls.engine = mod::Engine::Fft;
  out.writeln(field.model_name(), "with", field.engine_name());
}
```

Output

```text
BlackScholes with Fft
```

## See Also

- [`Field`](../Field.md): the class
- [`model_name`](model_name.md): the model's name

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="clear_paths.html">
    <span class="label">Previous</span>
    <span class="title">clear_paths</span>
  </a>
  <a class="next-link" href="exercise_boundary.html">
    <span class="label">Next</span>
    <span class="title">exercise_boundary</span>
  </a>
</div>
<!-- apiref:nav:end -->
