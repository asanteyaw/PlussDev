<!-- generated from Engine.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `module::Engine`

How the price of an option is computed: closed form, Fourier transforms, simulation, a tree, collocation or a PDE.

`controls.engine` decides how the expectation `e^{−rT} E^Q[payoff]` is evaluated. Each engine supports only some models; a [`Field`](Field.md) never switches engines silently, and the error for an unsupported pair lists what is available.

| Engine | How the price is obtained | Models | Early exercise |
|---|---|---|---|
| `Analytic` | a closed formula: Black-Scholes, or the SABR implied-volatility expansion priced through Black | BlackScholes, Sabr | no |
| `Fft` | Fourier inversion of the characteristic function on a grid, all strikes from one transform | Heston, Bates | no |
| `Cos` | the Fourier-cosine expansion of the density | BlackScholes, Heston, Bates | Bermudan, American |
| `MonteCarlo` | the average of the discounted payoff over simulated paths | all six | yes (least squares) |
| `Lattice` | a binomial tree | BlackScholes | Bermudan, American |
| `Spectral` | the integral equation of the American exercise boundary by Chebyshev collocation | BlackScholes | American (and European) |
| `Pde` | a two-dimensional finite-difference solution with ADI time stepping | Heston | Bermudan, American |

The transform engines need the model's characteristic function in closed form (Heston and Bates have it; the rough models do not); simulation needs only a way to draw paths, which is why it prices every model. The settings of every engine live side by side in [`Controls`](Controls.md), so switching the engine never loses the settings of another.

## Fields

### `Fft`

`enumerator`, default `0`.

Fourier inversion of the characteristic function; Heston and Bates.

### `Cos`

`enumerator`, default `1`.

Fourier-cosine expansion; Black-Scholes, Heston, Bates; supports early exercise.

### `MonteCarlo`

`enumerator`, default `2`.

Simulation; all six models.

### `Analytic`

`enumerator`, default `3`.

Closed formula; Black-Scholes and SABR. The default.

### `Lattice`

`enumerator`, default `4`.

Binomial tree; Black-Scholes; supports early exercise.

### `Spectral`

`enumerator`, default `5`.

Chebyshev collocation of the exercise boundary; Black-Scholes.

### `Pde`

`enumerator`, default `6`.

Two-dimensional finite differences; Heston; supports early exercise.

## Guidance

Set as `Controls::engine`.

## Practical Examples

### Example 1

The same Black-Scholes option under five engines. Closed form, cosine expansion, tree, collocation and simulation agree to their accuracy.

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
                   .market = fin::market::make(100.0, 0.05, 0.02),
                   .model = fin::model::make(fin::model::Kind::BlackScholes, {0.2}),
                   .controls = {.engine = mod::Engine::Analytic, .risk = mod::Risk::PriceOnly}};

  field.controls.mc.paths = 200000;
  field.controls.mc.seed = 3;
  for (const mod::Engine engine : {mod::Engine::Analytic, mod::Engine::Cos, mod::Engine::Lattice, mod::Engine::Spectral, mod::Engine::MonteCarlo}) {
    field.controls.engine = engine;
    out.writeln(field.engine_name(), ":", field.price().price.items<double>()[0]);
  }
}
```

Output

```text
Analytic : 9.22701
Cos : 9.22701
Lattice : 9.22506
Spectral : 9.22701
MonteCarlo : 9.21465
```

### Example 2

Heston by its four engines.

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
                   .controls = {.engine = mod::Engine::Fft, .risk = mod::Risk::PriceOnly}};

  field.controls.mc.paths = 100000;
  field.controls.mc.seed = 3;
  for (const mod::Engine engine : {mod::Engine::Fft, mod::Engine::Cos, mod::Engine::Pde, mod::Engine::MonteCarlo}) {
    field.controls.engine = engine;
    out.writeln(field.engine_name(), ":", field.price().price.items<double>()[0]);
  }
}
```

Output

```text
Fft : 8.62811
Cos : 8.62811
Pde : 8.62745
MonteCarlo : 8.67246
```

### Example 3

Only simulation prices the rough models. The error for any other engine names what is available.

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
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 100.0, 1.0),
                   .market = fin::market::make(100.0, 0.02, 0.0),
                   .model = fin::model::make(fin::model::Kind::RoughBergomi, {0.04, 1.5, -0.1, -0.7}),
                   .controls = {.engine = mod::Engine::Cos}};
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
subsp::math::finance::module::Field: controls.engine Cos is not supported for model RoughBergomi. Supported engines: MonteCarlo
```

### Example 4

SABR: the closed-form expansion and simulation.

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
                   .model = fin::model::make(fin::model::Kind::Sabr, {0.2, 1.0, 0.4, -0.3}),
                   .controls = {.engine = mod::Engine::Analytic, .risk = mod::Risk::PriceOnly}};
  out.writeln("Analytic:", field.price().price.items<double>()[0]);
  field.controls.engine = mod::Engine::MonteCarlo;
  field.controls.mc.paths = 100000;
  field.controls.mc.steps = 100;
  out.writeln("MonteCarlo:", field.price().price.items<double>()[0]);
}
```

Output

```text
Analytic: 9.0083
MonteCarlo: 8.9498
```

## See Also

- [`Controls`](Controls.md): holds the choice

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Controls.html">
    <span class="label">Previous</span>
    <span class="title">Controls</span>
  </a>
  <a class="next-link" href="Field.html">
    <span class="label">Next</span>
    <span class="title">Field</span>
  </a>
</div>
<!-- apiref:nav:end -->
