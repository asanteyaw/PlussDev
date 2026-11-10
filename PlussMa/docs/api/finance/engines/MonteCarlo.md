<!-- generated from MonteCarlo.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pricer::MonteCarlo`

The simulation engine: prices every model by averaging discounted payoffs over simulated paths, with variance reduction and least-squares early exercise.

Simulates `paths` paths of `steps` steps to the longest maturity under the pricing measure and averages the discounted payoff. It needs only a way to draw paths, so it prices every model and, by regression on the paths (least squares, the Longstaff-Schwartz method), American and Bermudan exercise. A simulated price has no error bar of its own: the standard error falls as `1/√paths`, and the discretisation bias of the scheme falls with `steps`. The settings fall in four groups: size (`paths`, `steps`), the time scheme (`scheme`), the random numbers (`rand_method`, `randomized_quasi`, `sobol_scrambling`, `offset`, `seed`), and variance reduction (`antithetic`, `control_variate`, `control_vol`, `importance_sampling`, `importance_shift`); `basis_degree` concerns early exercise.

## Fields

### `paths`

`std::int64_t`, default `40000`.

Number of simulated paths, positive.

### `steps`

`std::int64_t`, default `252`.

Time steps to the longest maturity, positive.

### `scheme`

`volatility::SimScheme`, default `Euler`.

The discretisation of the variance: `Euler`, `Milstein`, `Milstein2`, `QuadExp` or `Transition`. See [`SimScheme`](SimScheme.md).

### `rand_method`

`volatility::RandMethod`, default `Pseudo`.

Source of the normals: pseudo-random or a quasi-random sequence. See [`RandMethod`](RandMethod.md).

### `randomized_quasi`

`bool`, default `false`.

Randomise a quasi-random sequence (a random shift), so that independent replications give an error bar.

### `sobol_scrambling`

`int`, default `0`.

Scrambling of the Sobol sequence: 0 none; other values select a scrambling method.

### `antithetic`

`bool`, default `false`.

Pair every path with its mirror image (shocks `Z` and `−Z`).

### `control_variate`

`bool`, default `false`.

Subtract a correlated Black-Scholes payoff whose mean is known.

### `importance_sampling`

`bool`, default `false`.

Shift the normals by `importance_shift` and reweight, to sample out-of-the-money regions.

### `importance_shift`

`double`, default `0.0`.

The shift of the normal draws when importance sampling is on.

### `control_vol`

`double`, default `0.0`.

Volatility of the Black-Scholes control variate; 0 uses `√θ`, the long-run volatility of the Heston model.

### `offset`

`std::uint64_t`, default `0`.

Starting point in a quasi-random sequence.

### `seed`

`std::uint64_t`, default `0`.

Seed of the random numbers; 0 is a fixed seed too, so runs are reproducible.

### `basis_degree`

`std::int64_t`, default `3`.

American/Bermudan only: polynomial degree of the continuation-value regression, in [1, 8].

## Guidance

`Controls::mc`; also usable directly through `MonteCarlo::price`.

## Practical Examples

### Example 1

The path count. The estimate scatters about the COS price 8.628 and the scatter falls with more paths.

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
  for (const auto v : {1000, 10000, 100000}) {
    fin::pricer::MonteCarlo e;
    e.steps = 50;
    e.seed = 5;
    e.paths = v;
    out.writeln("paths", v, ": price =", e.price(inst, mdl, mkt, fin::pricer::RiskSpec::PriceOnly).price.items<double>()[0]);
  }
}
```

Output

```text
paths 1000 : price = 8.93402
paths 10000 : price = 8.58211
paths 100000 : price = 8.57790
```

### Example 2

The step count. The Euler scheme's discretisation bias falls with the steps: few steps give a visibly biased price.

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
  for (const auto v : {2, 8, 32, 128}) {
    fin::pricer::MonteCarlo e;
    e.paths = 200000;
    e.seed = 5;
    e.steps = v;
    out.writeln("steps", v, ": price =", e.price(inst, mdl, mkt, fin::pricer::RiskSpec::PriceOnly).price.items<double>()[0]);
  }
}
```

Output

```text
steps 2 : price = 8.94471
steps 8 : price = 8.63558
steps 32 : price = 8.61008
steps 128 : price = 8.56082
```

### Example 3

The scheme. Five discretisations of the variance with the same paths and steps. With 200,000 paths the sampling error is about 0.04, so only differences larger than that are real; see [`SimScheme`](SimScheme.md) for a larger run.

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
  for (const auto v : {fin::volatility::SimScheme::Euler, fin::volatility::SimScheme::Milstein, fin::volatility::SimScheme::Milstein2, fin::volatility::SimScheme::QuadExp, fin::volatility::SimScheme::Transition}) {
    fin::pricer::MonteCarlo e;
    e.paths = 200000;
    e.steps = 8;
    e.seed = 5;
    e.scheme = v;
    out.writeln("scheme", static_cast<int>(v), ": price =", e.price(inst, mdl, mkt, fin::pricer::RiskSpec::PriceOnly).price.items<double>()[0]);
  }
}
```

Output

```text
scheme 0 : price = 8.63558
scheme 1 : price = 8.55255
scheme 2 : price = 8.51117
scheme 3 : price = 8.60196
scheme 4 : price = 8.59121
```

### Example 4

The random numbers: pseudo-random against the three quasi-random sequences. At 4096 points the sequences differ from each other by more than their small deterministic errors; see [`RandMethod`](RandMethod.md).

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
  for (const auto v : {fin::volatility::RandMethod::Pseudo, fin::volatility::RandMethod::Sobol, fin::volatility::RandMethod::Halton, fin::volatility::RandMethod::Torus}) {
    fin::pricer::MonteCarlo e;
    e.paths = 4096;
    e.steps = 8;
    e.seed = 5;
    e.rand_method = v;
    out.writeln("rand_method", static_cast<int>(v), ": price =", e.price(inst, mdl, mkt, fin::pricer::RiskSpec::PriceOnly).price.items<double>()[0]);
  }
}
```

Output

```text
rand_method 0 : price = 8.65597
rand_method 1 : price = 8.73287
rand_method 2 : price = 8.47052
rand_method 3 : price = 8.75802
```

### Example 5

Randomising a Sobol sequence gives different, unbiased replications; the plain sequence gives the same number every time.

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

  for (const bool randomize : {false, true}) {
    for (const std::uint64_t seed : {1, 2, 3}) {
      fin::pricer::MonteCarlo e;
      e.paths = 4096;
      e.steps = 8;
      e.rand_method = fin::volatility::RandMethod::Sobol;
      e.randomized_quasi = randomize;
      e.seed = seed;
      out.writeln("randomized", randomize, "seed", seed, ":", e.price(inst, mdl, mkt).price.items<double>()[0]);
    }
  }
}
```

Output

```text
randomized false seed 1 : 8.73287
randomized false seed 2 : 8.73287
randomized false seed 3 : 8.73287
randomized true seed 1 : 8.78298
randomized true seed 2 : 8.93007
randomized true seed 3 : 8.65559
```

### Example 6

The Sobol scrambling method.

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
  for (const auto v : {0, 1, 2}) {
    fin::pricer::MonteCarlo e;
    e.paths = 4096;
    e.steps = 8;
    e.rand_method = fin::volatility::RandMethod::Sobol;
    e.randomized_quasi = true;
    e.seed = 4;
    e.sobol_scrambling = v;
    out.writeln("sobol_scrambling", v, ": price =", e.price(inst, mdl, mkt, fin::pricer::RiskSpec::PriceOnly).price.items<double>()[0]);
  }
}
```

Output

```text
sobol_scrambling 0 : price = 8.72648
sobol_scrambling 1 : price = 8.59411
sobol_scrambling 2 : price = 8.66869
```

### Example 7

The offset into a quasi-random sequence: a different slice of the same sequence.

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
  for (const auto v : {0, 1000, 5000}) {
    fin::pricer::MonteCarlo e;
    e.paths = 4096;
    e.steps = 8;
    e.rand_method = fin::volatility::RandMethod::Sobol;
    e.offset = v;
    out.writeln("offset", v, ": price =", e.price(inst, mdl, mkt, fin::pricer::RiskSpec::PriceOnly).price.items<double>()[0]);
  }
}
```

Output

```text
offset 0 : price = 8.73287
offset 1000 : price = 8.76604
offset 5000 : price = 8.64523
```

### Example 8

The seed. The same seed gives the same price; another seed gives another.

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
  const auto inst = fin::instrument::make(fin::pricing::Call, 100.0, 1.0);
  const auto mkt = fin::market::make(100.0, 0.02, 0.0);
  const auto mdl = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}).as<fin::model::Heston>();
  for (const auto v : {1, 1, 2}) {
    fin::pricer::MonteCarlo e;
    e.paths = 20000;
    e.steps = 20;
    e.seed = v;
    out.writeln("seed", v, ": price =", e.price(inst, mdl, mkt, fin::pricer::RiskSpec::PriceOnly).price.items<double>()[0]);
  }
}
```

Output

```text
seed 1 : price = 8.716790
seed 1 : price = 8.716790
seed 2 : price = 8.670459
```

### Example 9

Antithetic variates: the spread of the price over 16 seeds, plain against antithetic.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <utility>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto inst = fin::instrument::make(fin::pricing::Call, 100.0, 1.0);
  const auto mkt = fin::market::make(100.0, 0.02, 0.0);
  const auto mdl = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}).as<fin::model::Heston>();
  auto spread = [&](const fin::pricer::MonteCarlo &base) {
    double sum = 0.0, sum2 = 0.0;
    for (int seed = 1; seed <= 16; ++seed) {
      fin::pricer::MonteCarlo e = base;
      e.seed = 40 * seed;
      const double p = e.price(inst, mdl, mkt).price.items<double>()[0];
      sum += p; sum2 += p * p;
    }
    const double mean = sum / 16, var = (sum2 - 16 * mean * mean) / (16 - 1);
    return std::pair<double, double>{mean, std::sqrt(var > 0 ? var : 0)};
  };
  fin::pricer::MonteCarlo plain;
  plain.paths = 4000;
  plain.steps = 50;
  fin::pricer::MonteCarlo tuned = plain;
  tuned.antithetic = true;
  const auto a = spread(plain), b = spread(tuned);
  out.writeln("plain     : mean", a.first, ", spread over seeds", a.second);
  out.writeln("antithetic: mean", b.first, ", spread over seeds", b.second);
}
```

Output

```text
plain     : mean 8.6497 , spread over seeds 0.2284
antithetic: mean 8.6177 , spread over seeds 0.1021
```

### Example 10

A control variate: the spread over 16 seeds, plain against controlled (the Black-Scholes payoff at the long-run volatility).

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <utility>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto inst = fin::instrument::make(fin::pricing::Call, 100.0, 1.0);
  const auto mkt = fin::market::make(100.0, 0.02, 0.0);
  const auto mdl = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}).as<fin::model::Heston>();
  auto spread = [&](const fin::pricer::MonteCarlo &base) {
    double sum = 0.0, sum2 = 0.0;
    for (int seed = 1; seed <= 16; ++seed) {
      fin::pricer::MonteCarlo e = base;
      e.seed = 40 * seed;
      const double p = e.price(inst, mdl, mkt).price.items<double>()[0];
      sum += p; sum2 += p * p;
    }
    const double mean = sum / 16, var = (sum2 - 16 * mean * mean) / (16 - 1);
    return std::pair<double, double>{mean, std::sqrt(var > 0 ? var : 0)};
  };
  fin::pricer::MonteCarlo plain;
  plain.paths = 4000;
  plain.steps = 50;
  fin::pricer::MonteCarlo tuned = plain;
  tuned.control_variate = true;
  const auto a = spread(plain), b = spread(tuned);
  out.writeln("plain       : mean", a.first, ", spread over seeds", a.second);
  out.writeln("control variate: mean", b.first, ", spread over seeds", b.second);
}
```

Output

```text
plain       : mean 8.6497 , spread over seeds 0.2284
control variate: mean 8.6049 , spread over seeds 0.0690
```

### Example 11

The control volatility: a control at the right volatility removes more variance. Spread over 16 seeds for three choices.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <utility>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto inst = fin::instrument::make(fin::pricing::Call, 100.0, 1.0);
  const auto mkt = fin::market::make(100.0, 0.02, 0.0);
  const auto mdl = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}).as<fin::model::Heston>();

  for (const double control_vol : {0.0, 0.1, 0.4}) {
    double sum = 0.0, sum2 = 0.0;
    for (int seed = 1; seed <= 16; ++seed) {
      fin::pricer::MonteCarlo e;
      e.paths = 4000;
      e.steps = 50;
      e.control_variate = true;
      e.control_vol = control_vol;
      e.seed = 40 * seed;
      const double p = e.price(inst, mdl, mkt).price.items<double>()[0];
      sum += p; sum2 += p * p;
    }
    const double mean = sum / 16.0;
    out.writeln("control_vol", control_vol, ": mean", mean, ", spread", std::sqrt((sum2 - 16.0 * mean * mean) / 15.0));
  }
}
```

Output

```text
control_vol 0.0000 : mean 8.6049 , spread 0.0690
control_vol 0.1000 : mean 8.6065 , spread 0.0558
control_vol 0.4000 : mean 8.6064 , spread 0.0933
```

### Example 12

Importance sampling for a far-out-of-the-money call (strike 160): the spread over 16 seeds, plain against shifted.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <utility>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto inst = fin::instrument::make(fin::pricing::Call, 160.0, 1.0);
  const auto mkt = fin::market::make(100.0, 0.02, 0.0);
  const auto mdl = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}).as<fin::model::Heston>();
  auto spread = [&](const fin::pricer::MonteCarlo &base) {
    double sum = 0.0, sum2 = 0.0;
    for (int seed = 1; seed <= 16; ++seed) {
      fin::pricer::MonteCarlo e = base;
      e.seed = 40 * seed;
      const double p = e.price(inst, mdl, mkt).price.items<double>()[0];
      sum += p; sum2 += p * p;
    }
    const double mean = sum / 16, var = (sum2 - 16 * mean * mean) / (16 - 1);
    return std::pair<double, double>{mean, std::sqrt(var > 0 ? var : 0)};
  };
  fin::pricer::MonteCarlo plain;
  plain.paths = 4000;
  plain.steps = 50;
  fin::pricer::MonteCarlo tuned = plain;
  tuned.importance_sampling = true;
  tuned.importance_shift = 0.05;
  const auto a = spread(plain), b = spread(tuned);
  out.writeln("plain     : mean", a.first, ", spread over seeds", a.second);
  out.writeln("importance: mean", b.first, ", spread over seeds", b.second);
}
```

Output

```text
plain     : mean 0.00406 , spread over seeds 0.00385
importance: mean 0.00468 , spread over seeds 0.00297
```

### Example 13

The importance shift, which is applied to the normal draw at **every step**: a shift of 0.05 per step over 50 steps moves the terminal log-price by about 0.05·√(v·dt)·50 ≈ 0.35, towards the strike (ln 1.6 = 0.47). A shift that is too small does little, a good one helps, and one that is too large (1.0 per step is a 50-standard-deviation push) puts all the weight on a few paths and the estimate collapses. The exact price is 0.00461 (COS). Spread over 16 seeds.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <utility>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto inst = fin::instrument::make(fin::pricing::Call, 160.0, 1.0);
  const auto mkt = fin::market::make(100.0, 0.02, 0.0);
  const auto mdl = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}).as<fin::model::Heston>();

  for (const double shift : {0.0, 0.02, 0.05, 0.1, 1.0}) {
    double sum = 0.0, sum2 = 0.0;
    for (int seed = 1; seed <= 16; ++seed) {
      fin::pricer::MonteCarlo e;
      e.paths = 4000;
      e.steps = 50;
      e.importance_sampling = true;
      e.importance_shift = shift;
      e.seed = 40 * seed;
      const double p = e.price(inst, mdl, mkt).price.items<double>()[0];
      sum += p; sum2 += p * p;
    }
    const double mean = sum / 16.0;
    out.writeln("shift", shift, ": mean", mean, ", spread", std::sqrt((sum2 - 16.0 * mean * mean) / 15.0));
  }
}
```

Output

```text
shift 0.00000 : mean 0.00406 , spread 0.00385
shift 0.02000 : mean 0.00442 , spread 0.00340
shift 0.05000 : mean 0.00468 , spread 0.00297
shift 0.10000 : mean 0.00483 , spread 0.00231
shift 1.00000 : mean 0.00020 , spread 0.00029
```

### Example 14

The regression degree for an American put (least-squares Monte Carlo).

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
  const auto inst = fin::instrument::american(fin::pricing::Put, 100.0, 1.0);
  const auto mkt = fin::market::make(100.0, 0.05, 0.0);
  const auto mdl = fin::model::make(fin::model::Kind::BlackScholes, {0.2}).as<fin::model::BlackScholes>();
  for (const auto v : {1, 2, 3, 6}) {
    fin::pricer::MonteCarlo e;
    e.paths = 50000;
    e.steps = 50;
    e.seed = 3;
    e.basis_degree = v;
    out.writeln("basis_degree", v, ": price =", e.price(inst, mdl, mkt, fin::pricer::RiskSpec::PriceOnly).price.items<double>()[0]);
  }
}
```

Output

```text
basis_degree 1 : price = 6.00462
basis_degree 2 : price = 6.08985
basis_degree 3 : price = 6.09324
basis_degree 6 : price = 6.09887
```

## See Also

- [`Controls`](../field/Controls.md): holds one of each
- [`Engine`](../field/Engine.md): the engines

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Lattice.html">
    <span class="label">Previous</span>
    <span class="title">Lattice</span>
  </a>
  <a class="next-link" href="Pde.html">
    <span class="label">Next</span>
    <span class="title">Pde</span>
  </a>
</div>
<!-- apiref:nav:end -->
