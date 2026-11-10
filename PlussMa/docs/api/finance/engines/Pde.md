<!-- generated from Pde.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pricer::Pde`

The two-dimensional finite-difference engine for Heston, with European, Bermudan and American exercise.

Solves the Heston PDE in spot and variance by an alternating-direction (ADI) scheme on a grid of `spot_nodes × variance_nodes` with `time_steps` steps. The error shrinks with all three; the cost grows with their product. It is slower than the transform engines for a European option and is the engine of choice for American and Bermudan exercise under Heston when you want a grid method rather than regression.

## Fields

### `spot_nodes`

`std::int64_t`, default `240`.

Grid points in the spot direction, at least 8.

### `variance_nodes`

`std::int64_t`, default `60`.

Grid points in the variance direction, at least 6.

### `time_steps`

`std::int64_t`, default `100`.

Time steps, at least 3.

## Guidance

`Controls::pde`; also usable directly through `Pde::price`.

## Practical Examples

### Example 1

The spot grid. A finer grid approaches the COS price 8.62811.

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
  for (const auto v : {60, 120, 240, 480}) {
    fin::pricer::Pde e;
    e.spot_nodes = v;
    out.writeln("spot_nodes", v, ": price =", e.price(inst, mdl, mkt, fin::pricer::RiskSpec::PriceOnly).price.items<double>()[0]);
  }
}
```

Output

```text
spot_nodes 60 : price = 8.618509
spot_nodes 120 : price = 8.625652
spot_nodes 240 : price = 8.627448
spot_nodes 480 : price = 8.627885
```

### Example 2

The variance grid.

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
  for (const auto v : {15, 30, 60, 120}) {
    fin::pricer::Pde e;
    e.variance_nodes = v;
    out.writeln("variance_nodes", v, ": price =", e.price(inst, mdl, mkt, fin::pricer::RiskSpec::PriceOnly).price.items<double>()[0]);
  }
}
```

Output

```text
variance_nodes 15 : price = 8.627477
variance_nodes 30 : price = 8.627397
variance_nodes 60 : price = 8.627448
variance_nodes 120 : price = 8.627465
```

### Example 3

The time steps.

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
  for (const auto v : {10, 25, 100, 400}) {
    fin::pricer::Pde e;
    e.time_steps = v;
    out.writeln("time_steps", v, ": price =", e.price(inst, mdl, mkt, fin::pricer::RiskSpec::PriceOnly).price.items<double>()[0]);
  }
}
```

Output

```text
time_steps 10 : price = 8.621759
time_steps 25 : price = 8.626565
time_steps 100 : price = 8.627448
time_steps 400 : price = 8.627500
```

### Example 4

An American put under Heston: the PDE handles early exercise.

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
  const auto mdl = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}).as<fin::model::Heston>();

  fin::pricer::Pde e;
  out.writeln("American put under Heston:", e.price(inst, mdl, mkt, fin::pricer::RiskSpec::PriceOnly).price.items<double>()[0]);
}
```

Output

```text
American put under Heston: 5.98532
```

## See Also

- [`Controls`](../field/Controls.md): holds one of each
- [`Engine`](../field/Engine.md): the engines

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="MonteCarlo.html">
    <span class="label">Previous</span>
    <span class="title">MonteCarlo</span>
  </a>
  <a class="next-link" href="RandMethod.html">
    <span class="label">Next</span>
    <span class="title">RandMethod</span>
  </a>
</div>
<!-- apiref:nav:end -->
