<!-- generated from COS.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pricer::COS`

The Fourier-cosine engine: Black-Scholes, Heston and Bates by expanding the density in cosines, with Bermudan and American exercise.

The risk-neutral density of the log-asset is expanded in a cosine series on an interval `[a, b]` set from the model's cumulants; the characteristic function gives the coefficients, and a European price is a short sum. It is very fast and exponentially accurate for smooth densities. Early exercise is handled by a backward recursion over exercise dates with `exercise_terms` cosines on a range of `exercise_truncation` standard deviations; an American price is a Richardson extrapolation of Bermudans (`extrapolation` sets the depth). For Heston and Bates the early-exercise recursion also integrates over the variance with `variance_nodes` Gauss-Legendre nodes. The first three settings concern European pricing, the rest early exercise.

## Fields

### `num_terms`

`std::int64_t`, default `1024`.

Cosine terms of the expansion, positive. More terms give a smaller truncation error until rounding dominates.

### `truncation`

`double`, default `12.0`.

The half-width of the interval in standard deviations of the log-asset (from the cumulants), positive. Too narrow cuts the tails; too wide wastes terms.

### `cumulant_step`

`double`, default `1e-3`.

Step of the finite differences used to estimate the cumulants of the log-asset that set the interval.

### `exercise_truncation`

`double`, default `9.0`.

Early exercise: half-width of the log-asset range, in standard deviations.

### `exercise_terms`

`std::int64_t`, default `128`.

Early exercise: cosine terms of the recursion, at least 8. The value has a kink at the exercise point.

### `extrapolation`

`std::int64_t`, default `-1`.

American: depth `d` of the Richardson extrapolation over Bermudans with `2^d … 2^(d+3)` dates, in [−1, 6]; −1 picks 3 for Black-Scholes and 2 for Heston and Bates.

### `variance_nodes`

`std::int64_t`, default `128`.

Heston/Bates early exercise: Gauss-Legendre nodes in the log-variance, at least 8.

### `variance_tolerance`

`double`, default `1e-8`.

Heston/Bates early exercise: probability left outside the log-variance range; positive.

## Guidance

`Controls::cos`; also usable directly through `COS::price`.

## Practical Examples

### Example 1

The number of terms. A coarse expansion is far off, then converges and stays put.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(9));
  const auto inst = fin::instrument::make(fin::pricing::Call, 100.0, 1.0);
  const auto mkt = fin::market::make(100.0, 0.02, 0.0);
  const auto mdl = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}).as<fin::model::Heston>();
  for (const auto v : {8, 16, 32, 64, 1024}) {
    fin::pricer::COS e;
    e.num_terms = v;
    out.writeln("num_terms", v, ": price =", e.price(inst, mdl, mkt, fin::pricer::RiskSpec::PriceOnly).price.items<double>()[0]);
  }
}
```

Output

```text
num_terms 8 : price = 2.343088230
num_terms 16 : price = 7.797185740
num_terms 32 : price = 8.636190547
num_terms 64 : price = 8.628033866
num_terms 1024 : price = 8.628108103
```

### Example 2

The truncation range. Too narrow an interval cuts off the density and corrupts the price; the default 12 is generous.

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
  const auto inst = fin::instrument::make(fin::pricing::Call, 100.0, 1.0);
  const auto mkt = fin::market::make(100.0, 0.02, 0.0);
  const auto mdl = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}).as<fin::model::Heston>();
  for (const auto v : {1.0, 2.0, 4.0, 12.0}) {
    fin::pricer::COS e;
    e.truncation = v;
    out.writeln("truncation", v, ": price =", e.price(inst, mdl, mkt, fin::pricer::RiskSpec::PriceOnly).price.items<double>()[0]);
  }
}
```

Output

```text
truncation 1.00000000 : price = 7.02744852
truncation 2.00000000 : price = 8.63445033
truncation 4.00000000 : price = 8.62822991
truncation 12.00000000 : price = 8.62810810
```

### Example 3

The cumulant step. It only influences the width of the interval, and the price is insensitive to it: identical to ten decimals over two orders of magnitude.

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
  const auto inst = fin::instrument::make(fin::pricing::Call, 100.0, 1.0);
  const auto mkt = fin::market::make(100.0, 0.02, 0.0);
  const auto mdl = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}).as<fin::model::Heston>();
  for (const auto v : {0.01, 0.001, 0.0001}) {
    fin::pricer::COS e;
    e.cumulant_step = v;
    out.writeln("cumulant_step", v, ": price =", e.price(inst, mdl, mkt, fin::pricer::RiskSpec::PriceOnly).price.items<double>()[0]);
  }
}
```

Output

```text
cumulant_step 0.0100000000 : price = 8.6281081031
cumulant_step 0.0010000000 : price = 8.6281081031
cumulant_step 0.0001000000 : price = 8.6281081031
```

### Example 4

The exercise range, for an American put under Black-Scholes. The price varies only in the fifth decimal: a narrower range truncates the tails, a wider one lets the growing payoff leak error into the recursion, so wider is not better.

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
  const auto inst = fin::instrument::american(fin::pricing::Put, 100.0, 1.0);
  const auto mkt = fin::market::make(100.0, 0.05, 0.0);
  const auto mdl = fin::model::make(fin::model::Kind::BlackScholes, {0.2}).as<fin::model::BlackScholes>();
  for (const auto v : {3.0, 6.0, 9.0, 12.0}) {
    fin::pricer::COS e;
    e.exercise_truncation = v;
    out.writeln("exercise_truncation", v, ": price =", e.price(inst, mdl, mkt, fin::pricer::RiskSpec::PriceOnly).price.items<double>()[0]);
  }
}
```

Output

```text
exercise_truncation 3.000000 : price = 6.090350
exercise_truncation 6.000000 : price = 6.090350
exercise_truncation 9.000000 : price = 6.090345
exercise_truncation 12.000000 : price = 6.090378
```

### Example 5

The exercise terms. The value function has a kink, so more terms matter here.

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
  const auto inst = fin::instrument::american(fin::pricing::Put, 100.0, 1.0);
  const auto mkt = fin::market::make(100.0, 0.05, 0.0);
  const auto mdl = fin::model::make(fin::model::Kind::BlackScholes, {0.2}).as<fin::model::BlackScholes>();
  for (const auto v : {16, 32, 128, 256}) {
    fin::pricer::COS e;
    e.exercise_terms = v;
    out.writeln("exercise_terms", v, ": price =", e.price(inst, mdl, mkt, fin::pricer::RiskSpec::PriceOnly).price.items<double>()[0]);
  }
}
```

Output

```text
exercise_terms 16 : price = 6.231472
exercise_terms 32 : price = 6.085488
exercise_terms 128 : price = 6.090345
exercise_terms 256 : price = 6.090350
```

### Example 6

The extrapolation depth. Every depth is within 1e-3 of the American value 6.09037 (the collocation engine); the default depth 3 for Black-Scholes is the closest.

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
  const auto inst = fin::instrument::american(fin::pricing::Put, 100.0, 1.0);
  const auto mkt = fin::market::make(100.0, 0.05, 0.0);
  const auto mdl = fin::model::make(fin::model::Kind::BlackScholes, {0.2}).as<fin::model::BlackScholes>();
  for (const auto v : {0, 1, 2, 3, 4}) {
    fin::pricer::COS e;
    e.extrapolation = v;
    out.writeln("extrapolation", v, ": price =", e.price(inst, mdl, mkt, fin::pricer::RiskSpec::PriceOnly).price.items<double>()[0]);
  }
}
```

Output

```text
extrapolation 0 : price = 6.089600
extrapolation 1 : price = 6.091081
extrapolation 2 : price = 6.090194
extrapolation 3 : price = 6.090345
extrapolation 4 : price = 6.090499
```

### Example 7

The variance quadrature for Heston early exercise. Too few nodes cannot resolve the transition density of the variance, and the engine says so rather than return a wrong price.

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
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto inst = fin::instrument::american(fin::pricing::Put, 100.0, 1.0);
  const auto mkt = fin::market::make(100.0, 0.05, 0.0);
  const auto mdl = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}).as<fin::model::Heston>();
  for (const auto v : {16, 32, 128, 256}) {
    fin::pricer::COS e;
    e.variance_nodes = v;
    try {
      out.writeln("variance_nodes", v, ": price =", e.price(inst, mdl, mkt, fin::pricer::RiskSpec::PriceOnly).price.items<double>()[0]);
    } catch (const std::exception &error) {
      const std::string text(error.what());
      const std::string rest = text.substr(text.find("::", 20) + 2);
      out.writeln("variance_nodes", v, ": refused:", rest.substr(0, rest.find('\n')));
    }
  }
}
```

Output

```text
variance_nodes 16 : refused: pricing::heston_price_cos_exercise: the variance quadrature does not resolve the transition density for a step of 0.031250 years (it integrates to 0.978753 at log-variance -11.758644); raise variance_nodes or use fewer exercise dates / a smaller extrapolation depth
variance_nodes 32 : refused: pricing::heston_price_cos_exercise: the variance quadrature does not resolve the transition density for a step of 0.031250 years (it integrates to 1.000691 at log-variance -5.377444); raise variance_nodes or use fewer exercise dates / a smaller extrapolation depth
variance_nodes 128 : price = 5.986885
variance_nodes 256 : price = 5.986885
```

### Example 8

The variance range tolerance for Heston early exercise.

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
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto inst = fin::instrument::american(fin::pricing::Put, 100.0, 1.0);
  const auto mkt = fin::market::make(100.0, 0.05, 0.0);
  const auto mdl = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}).as<fin::model::Heston>();
  for (const auto v : {0.001, 1e-05, 1e-08}) {
    fin::pricer::COS e;
    e.variance_tolerance = v;
    try {
      out.writeln("variance_tolerance", v, ": price =", e.price(inst, mdl, mkt, fin::pricer::RiskSpec::PriceOnly).price.items<double>()[0]);
    } catch (const std::exception &error) {
      const std::string text(error.what());
      const std::string rest = text.substr(text.find("::", 20) + 2);
      out.writeln("variance_tolerance", v, ": refused:", rest.substr(0, rest.find('\n')));
    }
  }
}
```

Output

```text
variance_tolerance 0.001000 : refused: pricing::heston_price_cos_exercise: the variance quadrature does not resolve the transition density for a step of 0.031250 years (it integrates to 0.949353 at log-variance -8.350166); raise variance_nodes or use fewer exercise dates / a smaller extrapolation depth
variance_tolerance 0.000010 : refused: pricing::heston_price_cos_exercise: the variance quadrature does not resolve the transition density for a step of 0.031250 years (it integrates to 0.999366 at log-variance -11.712278); raise variance_nodes or use fewer exercise dates / a smaller extrapolation depth
variance_tolerance 0.000000 : price = 5.986885
```

## See Also

- [`Controls`](../field/Controls.md): holds one of each
- [`Engine`](../field/Engine.md): the engines

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Analytic.html">
    <span class="label">Previous</span>
    <span class="title">Analytic</span>
  </a>
  <a class="next-link" href="Fft.html">
    <span class="label">Next</span>
    <span class="title">Fft</span>
  </a>
</div>
<!-- apiref:nav:end -->
