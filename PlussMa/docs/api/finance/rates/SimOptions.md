<!-- generated from SimOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `rates::SimOptions`

The scheme, random-number source, layout and seed of the rate simulators.

`scheme` picks the time step ([`Scheme`](Scheme.md)). `method` is the source of the normals (pseudo-random or a quasi-random sequence, `volatility::RandMethod`), with `randomized_quasi`, `sobol_scrambling` and `offset` controlling the quasi-random sequence. `paths_first` transposes the result so that the paths come first. `seed` fixes the pseudo-random stream; the same options give the same paths.

## Fields

### `scheme`

`rates::Scheme`, default `Euler`.

Time stepping.

### `method`

`volatility::RandMethod`, default `Pseudo`.

Source of the normals.

### `randomized_quasi`

`bool`, default `false`.

Randomise a quasi-random sequence; requires a quasi-random `method`.

### `sobol_scrambling`

`int`, default `0`.

Scrambling of the Sobol sequence, in [0, 3].

### `paths_first`

`bool`, default `false`.

Return `[paths, steps + 1]` instead of `[steps + 1, paths]`.

### `seed`

`std::uint64_t`, default `0`.

Seed of the pseudo-random stream.

### `offset`

`std::uint64_t`, default `0`.

Starting point in a quasi-random sequence.

## Guidance

Passed to the path functions.

## Practical Examples

### Example 1

The scheme: the exact-transition scheme matches the mean of a Vasicek rate with a coarse step.

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
  rt::SimOptions options;
  options.scheme = rt::Scheme::Transition;
  options.seed = 4;
  const rt::VasicekParams p{.kappa = 1.0, .theta = 0.03, .sigma = 0.01, .r0 = 0.06};
  out.writeln("mean r_1 =", rt::vasicek_paths(1, 100000, 1.0, p, options).rate.select(0, 1).mean().item<double>(), ", exact =", 0.03 + 0.03 * std::exp(-1.0));
}
```

Output

```text
mean r_1 = 0.04103 , exact = 0.04104
```

### Example 2

The random numbers: a Sobol sequence against pseudo-random, for the same problem. Quasi-random numbers fill the space more evenly; the mean is usually closer to the exact one.

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
  out.print_defaults(PrintOptions{}.fixed(6));
  const rt::VasicekParams p{.kappa = 1.0, .theta = 0.03, .sigma = 0.02, .r0 = 0.06};
  const double exact = 0.03 + 0.03 * std::exp(-0.5);
  for (const auto method : {fin::volatility::RandMethod::Pseudo, fin::volatility::RandMethod::Sobol}) {
    rt::SimOptions options;
    options.method = method;
    options.scheme = rt::Scheme::Transition;
    options.seed = 4;
    out.writeln(static_cast<int>(method), ": mean error at T = 0.5:", rt::vasicek_paths(1, 4096, 0.5, p, options).rate.select(0, 1).mean().item<double>() - exact);
  }
}
```

Output

```text
0 : mean error at T = 0.5: 0.000061
1 : mean error at T = 0.5: -0.000006
```

### Example 3

Randomised quasi-random numbers give replications that differ by seed; the plain sequence does not.

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
  const rt::VasicekParams p{.kappa = 1.0, .theta = 0.03, .sigma = 0.02, .r0 = 0.06};
  for (const bool randomize : {false, true}) {
    for (const std::uint64_t seed : {1, 2}) {
      rt::SimOptions options;
      options.method = fin::volatility::RandMethod::Sobol;
      options.randomized_quasi = randomize;
      options.seed = seed;
      out.writeln("randomized", randomize, "seed", seed, ":", rt::vasicek_paths(2, 1024, 0.25, p, options).rate.select(0, 2).mean().item<double>());
    }
  }
}
```

Output

```text
randomized false seed 1 : 0.046880
randomized false seed 2 : 0.046880
randomized true seed 1 : 0.046869
randomized true seed 2 : 0.046818
```

### Example 4

The scrambling method of the Sobol sequence, 0 to 3; values outside are refused.

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
  out.print_defaults(PrintOptions{}.fixed(6));
  const rt::VasicekParams p{.kappa = 1.0, .theta = 0.03, .sigma = 0.02, .r0 = 0.06};
  rt::SimOptions options;
  options.method = fin::volatility::RandMethod::Sobol;
  options.randomized_quasi = true;
  for (const int scrambling : {0, 1, 3}) {
    options.sobol_scrambling = scrambling;
    out.writeln("scrambling", scrambling, ":", rt::vasicek_paths(2, 1024, 0.25, p, options).rate.select(0, 2).mean().item<double>());
  }
  options.sobol_scrambling = 7;
  try {
    (void)(rt::vasicek_paths(2, 1024, 0.25, p, options));
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
scrambling 0 : 0.046823
scrambling 1 : 0.046893
scrambling 3 : 0.046996
subsp::math::finance::rates::vasicek_paths: sobol_scrambling must be in [0,3]
```

### Example 5

The offset into the sequence: a different slice of the same Sobol points.

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
  const rt::VasicekParams p{.kappa = 1.0, .theta = 0.03, .sigma = 0.02, .r0 = 0.06};
  for (const std::uint64_t offset : {0, 500, 5000}) {
    rt::SimOptions options;
    options.method = fin::volatility::RandMethod::Sobol;
    options.offset = offset;
    out.writeln("offset", offset, ":", rt::vasicek_paths(2, 1024, 0.25, p, options).rate.select(0, 2).mean().item<double>());
  }
}
```

Output

```text
offset 0 : 0.046880
offset 500 : 0.046900
offset 5000 : 0.046913
```

### Example 6

The layout: with `paths_first` the paths come first.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace rt = fin::rates;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  const rt::VasicekParams p{};
  for (const bool first : {false, true}) {
    rt::SimOptions options;
    options.paths_first = first;
    out.writeln("paths_first", first, ": rate shape", rt::vasicek_paths(10, 5, 0.1, p, options).rate.sizes());
  }
}
```

Output

```text
paths_first false : rate shape [11, 5]
paths_first true : rate shape [5, 11]
```

### Example 7

The seed: the same seed reproduces the paths.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace rt = fin::rates;
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
  out.print_defaults(PrintOptions{}.fixed(6));
  const rt::VasicekParams p{};
  rt::SimOptions a, b;
  a.seed = 1; b.seed = 2;
  const Series r1 = rt::vasicek_paths(10, 100, 0.1, p, a).rate, r2 = rt::vasicek_paths(10, 100, 0.1, p, a).rate, r3 = rt::vasicek_paths(10, 100, 0.1, p, b).rate;
  out.writeln("same seed:", (r1 - r2).abs().max().item<double>(), ", other seed:", (r1 - r3).abs().max().item<double>());
}
```

Output

```text
same seed: 0.000000 , other seed: 0.028896
```

### Example 8

A pseudo-random method cannot be randomised: `randomized_quasi` needs a quasi-random sequence.

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
  rt::SimOptions options;
  options.randomized_quasi = true;
  try {
    (void)(rt::vasicek_paths(10, 100, 0.1, p, options));
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
subsp::math::finance::rates::vasicek_paths: randomized_quasi requires Sobol/Halton/Torus method
```

## See Also

- [`Scheme`](Scheme.md): the schemes

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Scheme.html">
    <span class="label">Previous</span>
    <span class="title">Scheme</span>
  </a>
  <a class="next-link" href="VasicekParams.html">
    <span class="label">Next</span>
    <span class="title">VasicekParams</span>
  </a>
</div>
<!-- apiref:nav:end -->
