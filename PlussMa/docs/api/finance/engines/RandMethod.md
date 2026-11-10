<!-- generated from RandMethod.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `volatility::RandMethod`

The source of the random numbers of a simulation: pseudo-random or a quasi-random sequence.

`Pseudo` is the pseudo-random generator seeded by `seed`. `Sobol`, `Halton` and `Torus` are low-discrepancy sequences, which fill the space of shocks more evenly than random points and can reduce the error well below `1/√paths` when the number of random dimensions (steps × factors) is modest. A plain quasi-random estimate has no error bar; randomise it (`randomized_quasi`) and replicate to get one.

## Fields

### `Pseudo`

`enumerator`, default `0`.

Pseudo-random numbers from `seed`.

### `Sobol`

`enumerator`, default `1`.

Sobol low-discrepancy sequence.

### `Halton`

`enumerator`, default `2`.

Halton sequence.

### `Torus`

`enumerator`, default `3`.

Kronecker/torus sequence.

## Guidance

Set as `MonteCarlo::rand_method`.

## Practical Examples

### Example 1

The four methods on the same problem.

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
  const auto inst = fin::instrument::make(fin::pricing::Call, 100.0, 1.0);
  const auto mkt = fin::market::make(100.0, 0.02, 0.0);
  const auto mdl = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}).as<fin::model::Heston>();

  for (const auto method : {fin::volatility::RandMethod::Pseudo, fin::volatility::RandMethod::Sobol, fin::volatility::RandMethod::Halton, fin::volatility::RandMethod::Torus}) {
    fin::pricer::MonteCarlo e;
    e.rand_method = method;
    e.paths = 4096;
    e.steps = 4;
    e.seed = 2;
    out.writeln(static_cast<int>(method), ":", e.price(inst, mdl, mkt).price.items<double>()[0]);
  }
}
```

Output

```text
0 : 8.77946
1 : 8.75656
2 : 8.67349
3 : 8.76323
```

### Example 2

Does quasi-random help? The worst error over 8 randomised replications at 4096 paths, against the COS price 8.628, for 1, 2 and 4 steps. On this problem there is no systematic gain over pseudo-random numbers at this size; the benefit of low-discrepancy sequences is problem dependent and shows with few random dimensions and more points.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

static double bs(fin::pricing::OptionType type, double S, double K, double r, double q, double T, double vol) {
  fin::module::Field f{.instrument = fin::instrument::make(type, K, T), .market = fin::market::make(S, r, q),
                       .model = fin::model::BlackScholes{.volatility = pluss::make_series({vol})},
                       .controls = {.engine = fin::module::Engine::Analytic, .risk = fin::module::Risk::PriceOnly}};
  return f.price().price.items<double>()[0];
}
static pluss::Series bsv(fin::pricing::OptionType type, const pluss::Series &spots, double K, double r, double q, double T, double vol) {
  fin::module::Field f{.instrument = fin::instrument::make(type, K, T), .market = {.spot = spots, .rate = pluss::make_series({r}), .dividend_yield = pluss::make_series({q})},
                       .model = fin::model::BlackScholes{.volatility = pluss::make_series({vol})},
                       .controls = {.engine = fin::module::Engine::Analytic, .risk = fin::module::Risk::PriceOnly}};
  return f.price().price;
}
static pluss::Series bst(fin::pricing::OptionType type, const pluss::Series &spots, const pluss::Series &tau, double K, double r, double q, double vol) {
  fin::module::Field f{.instrument = {.type = type, .strike = pluss::make_series({K}), .maturity = tau}, .market = {.spot = spots, .rate = pluss::make_series({r}), .dividend_yield = pluss::make_series({q})},
                       .model = fin::model::BlackScholes{.volatility = pluss::make_series({vol})},
                       .controls = {.engine = fin::module::Engine::Analytic, .risk = fin::module::Risk::PriceOnly}};
  return f.price().price;
}
static fin::hedging::DeltaHedgeResult hedge(const pluss::Series &path, fin::pricing::OptionType type, double K, double r, double q, double T, double vol) {
  fin::module::Field f{.instrument = fin::instrument::make(type, K, T), .market = fin::market::make(100.0, r, q),
                       .model = fin::model::BlackScholes{.volatility = pluss::make_series({vol})},
                       .controls = {.engine = fin::module::Engine::Analytic}};
  return fin::hedging::delta_hedge(f, path);
}
static double iv(double price, double S, double K, double r, double q, double T, const fin::pricing::ImpliedVolOptions &o = {}) {
  fin::module::Field f{.instrument = fin::instrument::make(fin::pricing::Call, K, T), .market = fin::market::make(S, r, q),
                       .model = fin::model::BlackScholes{.volatility = pluss::make_series({0.2})},
                       .controls = {.engine = fin::module::Engine::Analytic, .risk = fin::module::Risk::PriceOnly}};
  return f.implied_vol(pluss::make_series({price}), o).items<double>()[0];
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto inst = fin::instrument::make(fin::pricing::Call, 100.0, 1.0);
  const auto mkt = fin::market::make(100.0, 0.02, 0.0);
  const auto mdl = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}).as<fin::model::Heston>();

  for (const int steps : {1, 2, 4}) {
    for (const auto method : {fin::volatility::RandMethod::Pseudo, fin::volatility::RandMethod::Sobol}) {
      double worst = 0.0;
      for (int seed = 1; seed <= 8; ++seed) {
        fin::pricer::MonteCarlo e;
        e.rand_method = method;
        e.randomized_quasi = method != fin::volatility::RandMethod::Pseudo;
        e.paths = 4096;
        e.steps = steps;
        e.seed = seed;
        worst = std::max(worst, std::abs(e.price(inst, mdl, mkt).price.items<double>()[0] - 8.62811));
      }
      out.writeln("steps", steps, method == fin::volatility::RandMethod::Pseudo ? "pseudo" : "Sobol ", ": worst error =", worst);
    }
  }
}
```

Output

```text
steps 1 pseudo : worst error = 0.5814
steps 1 Sobol  : worst error = 0.6002
steps 2 pseudo : worst error = 0.4639
steps 2 Sobol  : worst error = 0.7361
steps 4 pseudo : worst error = 0.3770
steps 4 Sobol  : worst error = 0.5589
```

### Example 3

Through a Field.

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

  field.controls.mc.paths = 4096;
  field.controls.mc.steps = 4;
  field.controls.mc.rand_method = fin::volatility::RandMethod::Halton;
  out.writeln("Halton:", field.price().price.items<double>()[0]);
}
```

Output

```text
Halton: 8.6735
```

## See Also

- [`MonteCarlo`](MonteCarlo.md): uses it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Pde.html">
    <span class="label">Previous</span>
    <span class="title">Pde</span>
  </a>
  <a class="next-link" href="RiskSpec.html">
    <span class="label">Next</span>
    <span class="title">RiskSpec</span>
  </a>
</div>
<!-- apiref:nav:end -->
