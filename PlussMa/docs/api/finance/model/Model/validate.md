<!-- generated from validate.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e79b8f39de28 -->

# `module::Model::validate`

[Back to `module::Model`](../Model.md)

Checks that every parameter is defined, finite and inside its domain.

## Signature

```cpp
void module::Model::validate() const;
```

## What It Does

Throws, naming each offending parameter, unless every parameter is defined, finite and in its domain: volatilities, mean-reversion speeds, long-run variances and volatilities of variance positive; initial variance and jump intensity non-negative; correlations in (−1, 1); SABR's `beta` in [0, 1]; the rough exponent `alpha` in (−0.5, 0), the Hurst exponent minus ½. All problems are reported together. A Heston-type model that violates the Feller condition `2κθ ≥ σ²` is legal and is accepted (it only warns, at the level of a `Field`).

## Returns

Nothing; throws on error.

## Errors

- Any undefined, non-finite or out-of-domain parameter throws (`pluss::Error`), all listed.

## Practical Examples

### Example 1

A legal model passes.

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
  fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}).validate();
  out.writeln("valid");
}
```

Output

```text
valid
```

### Example 2

A default model has no volatility; validating it names the missing parameter.

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
  try {
    (void)(mod::Model{}.validate());
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find("Triggered")));
  }
}
```

Output

```text
subsp::math::finance::module::Model: model.BlackScholes.volatility must be defined
```

### Example 3

Several problems reported together: a negative volatility of variance and a correlation of 1.5.

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
  mod::Model m = fin::model::Heston{.kappa = pluss::make_series({1.5}), .theta = pluss::make_series({0.04}), .sigma = pluss::make_series({-0.3}), .rho = pluss::make_series({1.5}), .v0 = pluss::make_series({0.04})};
  try {
    (void)(m.validate());
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find("Triggered")));
  }
}
```

Output

```text
subsp::math::finance::module::Model (2 problems):
  - model.Heston.sigma must be > 0
  - model.Heston.rho must be in (-1, 1)
```

### Example 4

The rough exponent must lie in (−0.5, 0).

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
  try {
    (void)(fin::model::make(fin::model::Kind::RoughBergomi, {0.04, 1.5, 0.3, -0.7}));
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find("Triggered")));
  }
}
```

Output

```text
subsp::math::finance::module::Model: model.RoughBergomi.alpha must be in (-0.5, 0) (the Hurst exponent minus 1/2)
```

## See Also

- [`Model`](../Model.md): the class
- [`make`](make.md): validates on construction

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="parameters.html">
    <span class="label">Previous</span>
    <span class="title">parameters</span>
  </a>
  <a class="next-link" href="visit.html">
    <span class="label">Next</span>
    <span class="title">visit</span>
  </a>
</div>
<!-- apiref:nav:end -->
