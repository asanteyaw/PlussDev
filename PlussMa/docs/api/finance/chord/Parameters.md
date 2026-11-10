<!-- generated from Parameters.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `module::Chord::Parameters`

The model parameters of several assets, of any of the six models, as columns, one entry per asset, and the correlations between the assets.

## Signature

```cpp
static module::Chord module::Chord::from_model_params(const module::Chord::Parameters &parameters);
```

A `Chord::Parameters` is the argument of [`Chord::from_model_params`](Chord/from_model_params.md): a table with one column per parameter and one row per asset, filled with designated initializers, e.g. `mod::Chord::from_model_params({.spot = {100.0, 90.0}, .kappa = {1.5}, ...})`. All columns are `std::vector<double>`, so the numbers can come from an array, a file you parsed, or a calibration, with no `Series` to build. **The number of assets** is the length of the longest column (or of `names`). Every column then has either one entry, which applies to all assets, or exactly one per asset; any other length is refused, naming the column. Because the number of assets is the length of the longest column, a column that is too long makes the shorter ones the offenders: with `spot` of 2 entries and `sigma` of 3, the error names `spot` ("has 2 entries; it needs 1 or 3"). The `kind` column gives the model of each asset (`BlackScholes`, `Sabr`, `Heston`, `Bates`, `RoughHeston` or `RoughBergomi`; Heston when it is absent, Bates when the three jump columns are given without it). Each model reads its own columns: `volatility` for Black-Scholes; `alpha0`, `beta`, `nu` and `rho` for SABR; `kappa`, `theta`, `sigma`, `rho` and `v0` for Heston; those and the jump columns for Bates; those and `alpha` for RoughHeston; `xi`, `eta`, `alpha` and `rho` for RoughBergomi. `spot` is always required, and a column is required exactly when an asset's model reads it. In a table of mixed models an asset ignores the entries of the columns its model does not read, so they are fillers (NaN by custom), and a column that no asset's model reads is refused.

The cross-asset correlations are `price_corr`, `var_corr` and `price_var_cross`, matrices with one row and column per asset, in the order of the columns. `price_corr` is the correlation between the assets' price shocks, `var_corr` that between their variance shocks, and `price_var_cross` the cross correlation of one asset's price shock with another's variance shock; if the last is not given it is built from the other two and the `rho` of each asset. They are the same members as on the [`Chord`](Chord.md), set on the Chord that is built. Every field example below starts from the same two-asset table (spot 100 and 90, one set of Heston parameters shared by both, 20,000 paths and 20 steps) and changes one field.

## Fields

### `names`

`std::vector<std::string>`, default empty. The name of each asset, in order. Left empty, the assets are called `asset1`, `asset2`, ... . The names are the keys of the resulting [`Chord`](Chord.md) (`at`, `Chord::Result`), so they should mean something to the caller; they must be unique and non-empty, and when given there must be one per asset.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Chord::Parameters base() {
  mod::Chord::Parameters p;
  p.spot = {100.0, 90.0};
  p.kappa = {1.5};
  p.theta = {0.04};
  p.sigma = {0.3};
  p.rho = {-0.7};
  p.v0 = {0.04};
  p.controls.mc.paths = 20000;
  p.controls.mc.steps = 20;
  p.controls.mc.seed = 3;
  return p;
}
static double corr(const pluss::Series &a, const pluss::Series &b) { return a.mul(b).mean().item<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Chord::Parameters p = base();
  out.writeln("default:", mod::Chord::from_model_params(p).names());
  p.names = {"spx", "ndx"};
  out.writeln("given:  ", mod::Chord::from_model_params(p).names());
}
```

Output

```text
default: [asset1, asset2]
given:   [spx, ndx]
```


### `spot`

`std::vector<double>`, required. The price of each asset today (for a SABR asset, its forward). It is the column that sets the number of assets when no other column is longer; a single entry applies to every asset. It is also the starting value of the simulated paths and, by default, the strike of the call each Field carries.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Chord::Parameters base() {
  mod::Chord::Parameters p;
  p.spot = {100.0, 90.0};
  p.kappa = {1.5};
  p.theta = {0.04};
  p.sigma = {0.3};
  p.rho = {-0.7};
  p.v0 = {0.04};
  p.controls.mc.paths = 20000;
  p.controls.mc.steps = 20;
  p.controls.mc.seed = 3;
  return p;
}
static double corr(const pluss::Series &a, const pluss::Series &b) { return a.mul(b).mean().item<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Chord::Parameters p = base();   // spot = {100, 90}
  const mod::Chord chord = mod::Chord::from_model_params(p);
  for (const auto &name : chord.names()) out.writeln(name, "spot", chord.at(name).market.spot.items<double>());
  p.spot = {250.0};
  out.writeln("one entry for the table's single asset:", mod::Chord::from_model_params(p).at("asset1").market.spot.items<double>(), " assets:", mod::Chord::from_model_params(p).size());
}
```

Output

```text
asset1 spot [100.00000]
asset2 spot [90.00000]
one entry for the table's single asset: [250.00000]  assets: 1
```


### `rate`

`std::vector<double>`, default {0}. The continuously compounded risk-free rate of each asset. One entry applies to every asset; the default is zero. The path simulation grows each asset at `rate − dividend_yield`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Chord::Parameters base() {
  mod::Chord::Parameters p;
  p.spot = {100.0, 90.0};
  p.kappa = {1.5};
  p.theta = {0.04};
  p.sigma = {0.3};
  p.rho = {-0.7};
  p.v0 = {0.04};
  p.controls.mc.paths = 20000;
  p.controls.mc.steps = 20;
  p.controls.mc.seed = 3;
  return p;
}
static double corr(const pluss::Series &a, const pluss::Series &b) { return a.mul(b).mean().item<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Chord::Parameters p = base();
  p.rate = {0.03, 0.01};
  const mod::Chord chord = mod::Chord::from_model_params(p);
  for (const auto &name : chord.names()) out.writeln(name, "rate", chord.at(name).market.rate.items<double>());
}
```

Output

```text
asset1 rate [0.03000]
asset2 rate [0.01000]
```


### `dividend_yield`

`std::vector<double>`, default {0}. The continuous dividend (or carry) yield of each asset; one entry applies to all. The default is zero.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Chord::Parameters base() {
  mod::Chord::Parameters p;
  p.spot = {100.0, 90.0};
  p.kappa = {1.5};
  p.theta = {0.04};
  p.sigma = {0.3};
  p.rho = {-0.7};
  p.v0 = {0.04};
  p.controls.mc.paths = 20000;
  p.controls.mc.steps = 20;
  p.controls.mc.seed = 3;
  return p;
}
static double corr(const pluss::Series &a, const pluss::Series &b) { return a.mul(b).mean().item<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Chord::Parameters p = base();
  p.rate = {0.03};
  p.dividend_yield = {0.0, 0.012};
  const mod::Chord chord = mod::Chord::from_model_params(p);
  for (const auto &name : chord.names()) out.writeln(name, "yield", chord.at(name).market.dividend_yield.items<double>());
  out.writeln("growth of each asset = rate - yield:", 0.03 - 0.0, 0.03 - 0.012);
}
```

Output

```text
asset1 yield [0.00000]
asset2 yield [0.01200]
growth of each asset = rate - yield: 0.03000 0.01800
```


### `kappa`

`std::vector<double>`, read by the models that use it. The speed at which each asset's variance reverts to its long-run level, greater than 0. Read by the Heston, Bates and RoughHeston models. One entry applies to all.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Chord::Parameters base() {
  mod::Chord::Parameters p;
  p.spot = {100.0, 90.0};
  p.kappa = {1.5};
  p.theta = {0.04};
  p.sigma = {0.3};
  p.rho = {-0.7};
  p.v0 = {0.04};
  p.controls.mc.paths = 20000;
  p.controls.mc.steps = 20;
  p.controls.mc.seed = 3;
  return p;
}
static double corr(const pluss::Series &a, const pluss::Series &b) { return a.mul(b).mean().item<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Chord::Parameters p = base();
  p.kappa = {1.5, 2.1};
  const auto parameters_of = [&](const mod::Chord &c) {
    for (const auto &name : c.names()) out.writeln(name, " (kappa, theta, sigma, rho, v0, ...) =", c.at(name).model.parameters().items<double>());
  };
  parameters_of(mod::Chord::from_model_params(p));
}
```

Output

```text
asset1  (kappa, theta, sigma, rho, v0, ...) = [1.50000, 0.04000, 0.30000, -0.70000, 0.04000]
asset2  (kappa, theta, sigma, rho, v0, ...) = [2.10000, 0.04000, 0.30000, -0.70000, 0.04000]
```


### `theta`

`std::vector<double>`, read by the models that use it. The long-run variance of each asset, greater than 0 (a volatility of 20% is a variance of 0.04). Read by the Heston, Bates and RoughHeston models.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Chord::Parameters base() {
  mod::Chord::Parameters p;
  p.spot = {100.0, 90.0};
  p.kappa = {1.5};
  p.theta = {0.04};
  p.sigma = {0.3};
  p.rho = {-0.7};
  p.v0 = {0.04};
  p.controls.mc.paths = 20000;
  p.controls.mc.steps = 20;
  p.controls.mc.seed = 3;
  return p;
}
static double corr(const pluss::Series &a, const pluss::Series &b) { return a.mul(b).mean().item<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Chord::Parameters p = base();
  p.theta = {0.04, 0.055};
  const auto parameters_of = [&](const mod::Chord &c) {
    for (const auto &name : c.names()) out.writeln(name, " (kappa, theta, sigma, rho, v0, ...) =", c.at(name).model.parameters().items<double>());
  };
  parameters_of(mod::Chord::from_model_params(p));
}
```

Output

```text
asset1  (kappa, theta, sigma, rho, v0, ...) = [1.50000, 0.04000, 0.30000, -0.70000, 0.04000]
asset2  (kappa, theta, sigma, rho, v0, ...) = [1.50000, 0.05500, 0.30000, -0.70000, 0.04000]
```


### `sigma`

`std::vector<double>`, read by the models that use it. The volatility of the variance (the "vol of vol") of each asset, greater than 0. Read by the Heston, Bates and RoughHeston models.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Chord::Parameters base() {
  mod::Chord::Parameters p;
  p.spot = {100.0, 90.0};
  p.kappa = {1.5};
  p.theta = {0.04};
  p.sigma = {0.3};
  p.rho = {-0.7};
  p.v0 = {0.04};
  p.controls.mc.paths = 20000;
  p.controls.mc.steps = 20;
  p.controls.mc.seed = 3;
  return p;
}
static double corr(const pluss::Series &a, const pluss::Series &b) { return a.mul(b).mean().item<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Chord::Parameters p = base();
  p.sigma = {0.3, 0.45};
  const auto parameters_of = [&](const mod::Chord &c) {
    for (const auto &name : c.names()) out.writeln(name, " (kappa, theta, sigma, rho, v0, ...) =", c.at(name).model.parameters().items<double>());
  };
  parameters_of(mod::Chord::from_model_params(p));
}
```

Output

```text
asset1  (kappa, theta, sigma, rho, v0, ...) = [1.50000, 0.04000, 0.30000, -0.70000, 0.04000]
asset2  (kappa, theta, sigma, rho, v0, ...) = [1.50000, 0.04000, 0.45000, -0.70000, 0.04000]
```


### `rho`

`std::vector<double>`, read by the models that use it. The correlation between each asset's own price shock and its second-factor shock (the variance, or for SABR the volatility), strictly between −1 and 1; read by every model except Black-Scholes; negative values give the leverage effect and the skew. Within an asset this is fixed by `rho`; the cross-asset matrices `price_corr`, `var_corr` and `price_var_cross` relate the assets to one another.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Chord::Parameters base() {
  mod::Chord::Parameters p;
  p.spot = {100.0, 90.0};
  p.kappa = {1.5};
  p.theta = {0.04};
  p.sigma = {0.3};
  p.rho = {-0.7};
  p.v0 = {0.04};
  p.controls.mc.paths = 20000;
  p.controls.mc.steps = 20;
  p.controls.mc.seed = 3;
  return p;
}
static double corr(const pluss::Series &a, const pluss::Series &b) { return a.mul(b).mean().item<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Chord::Parameters p = base();
  p.rho = {-0.7, -0.5};
  const auto parameters_of = [&](const mod::Chord &c) {
    for (const auto &name : c.names()) out.writeln(name, " (kappa, theta, sigma, rho, v0, ...) =", c.at(name).model.parameters().items<double>());
  };
  parameters_of(mod::Chord::from_model_params(p));
}
```

Output

```text
asset1  (kappa, theta, sigma, rho, v0, ...) = [1.50000, 0.04000, 0.30000, -0.70000, 0.04000]
asset2  (kappa, theta, sigma, rho, v0, ...) = [1.50000, 0.04000, 0.30000, -0.50000, 0.04000]
```


### `v0`

`std::vector<double>`, read by the models that use it. The variance of each asset today, 0 or more. Read by the Heston, Bates and RoughHeston models.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Chord::Parameters base() {
  mod::Chord::Parameters p;
  p.spot = {100.0, 90.0};
  p.kappa = {1.5};
  p.theta = {0.04};
  p.sigma = {0.3};
  p.rho = {-0.7};
  p.v0 = {0.04};
  p.controls.mc.paths = 20000;
  p.controls.mc.steps = 20;
  p.controls.mc.seed = 3;
  return p;
}
static double corr(const pluss::Series &a, const pluss::Series &b) { return a.mul(b).mean().item<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Chord::Parameters p = base();
  p.v0 = {0.035, 0.05};
  const auto parameters_of = [&](const mod::Chord &c) {
    for (const auto &name : c.names()) out.writeln(name, " (kappa, theta, sigma, rho, v0, ...) =", c.at(name).model.parameters().items<double>());
  };
  parameters_of(mod::Chord::from_model_params(p));
}
```

Output

```text
asset1  (kappa, theta, sigma, rho, v0, ...) = [1.50000, 0.04000, 0.30000, -0.70000, 0.03500]
asset2  (kappa, theta, sigma, rho, v0, ...) = [1.50000, 0.04000, 0.30000, -0.70000, 0.05000]
```


### `kind`

`std::vector<model::Kind>`, default Heston (Bates with the jump columns). The model of each asset: `BlackScholes`, `Sabr`, `Heston`, `Bates`, `RoughHeston` or `RoughBergomi`; one entry applies to every asset. Each model reads its own columns (see the list above); an entry of a column is read only for the assets whose model uses it, so in a table of mixed models the other entries are fillers, and NaN is the customary filler. A column that no asset's model uses is refused.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Chord::Parameters base() {
  mod::Chord::Parameters p;
  p.spot = {100.0, 90.0};
  p.kappa = {1.5};
  p.theta = {0.04};
  p.sigma = {0.3};
  p.rho = {-0.7};
  p.v0 = {0.04};
  p.controls.mc.paths = 20000;
  p.controls.mc.steps = 20;
  p.controls.mc.seed = 3;
  return p;
}
static double corr(const pluss::Series &a, const pluss::Series &b) { return a.mul(b).mean().item<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const double nan = std::numeric_limits<double>::quiet_NaN();
  mod::Chord::Parameters p;
  p.names = {"smooth", "flat", "smile"};
  p.kind = {fin::model::Kind::Heston, fin::model::Kind::BlackScholes, fin::model::Kind::Sabr};
  p.spot = {100.0, 80.0, 120.0};
  p.volatility = {nan, 0.2, nan};
  p.kappa = {1.5, nan, nan};
  p.theta = {0.04, nan, nan};
  p.sigma = {0.3, nan, nan};
  p.v0 = {0.04, nan, nan};
  p.alpha0 = {nan, nan, 0.3};
  p.beta = {nan, nan, 0.7};
  p.nu = {nan, nan, 0.5};
  p.rho = {-0.7, nan, -0.3};
  const mod::Chord chord = mod::Chord::from_model_params(p);
  for (const auto &name : chord.names()) out.writeln(name, chord.at(name).model.name(), chord.at(name).model.parameters().items<double>());
}
```

Output

```text
smooth Heston [1.50000, 0.04000, 0.30000, -0.70000, 0.04000]
flat BlackScholes [0.20000]
smile Sabr [0.30000, 0.70000, 0.50000, -0.30000]
```


### `volatility`

`std::vector<double>`, read by BlackScholes. The constant volatility of a Black-Scholes asset, 0 or more. Its price has no second factor and its variance is the constant `volatility²`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Chord::Parameters base() {
  mod::Chord::Parameters p;
  p.spot = {100.0, 90.0};
  p.kappa = {1.5};
  p.theta = {0.04};
  p.sigma = {0.3};
  p.rho = {-0.7};
  p.v0 = {0.04};
  p.controls.mc.paths = 20000;
  p.controls.mc.steps = 20;
  p.controls.mc.seed = 3;
  return p;
}
static double corr(const pluss::Series &a, const pluss::Series &b) { return a.mul(b).mean().item<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Chord::Parameters p;
  p.kind = {fin::model::Kind::BlackScholes};
  p.spot = {100.0, 90.0};
  p.volatility = {0.2, 0.3};
  const mod::Chord chord = mod::Chord::from_model_params(p);
  for (const auto &name : chord.names()) out.writeln(name, chord.at(name).model.name(), "volatility", chord.at(name).model.parameters().items<double>());
}
```

Output

```text
asset1 BlackScholes volatility [0.20000]
asset2 BlackScholes volatility [0.30000]
```


### `alpha0`

`std::vector<double>`, read by Sabr. The initial level of the volatility factor `α` of a SABR asset, greater than 0. The asset price of a SABR asset is its forward, and `α` is a lognormal martingale with volatility `nu`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Chord::Parameters base() {
  mod::Chord::Parameters p;
  p.spot = {100.0, 90.0};
  p.kappa = {1.5};
  p.theta = {0.04};
  p.sigma = {0.3};
  p.rho = {-0.7};
  p.v0 = {0.04};
  p.controls.mc.paths = 20000;
  p.controls.mc.steps = 20;
  p.controls.mc.seed = 3;
  return p;
}
static double corr(const pluss::Series &a, const pluss::Series &b) { return a.mul(b).mean().item<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Chord::Parameters p;
  p.kind = {fin::model::Kind::Sabr};
  p.spot = {100.0, 100.0};
  p.alpha0 = {0.2, 0.4};
  p.beta = {0.7};
  p.nu = {0.5};
  p.rho = {-0.3};
  const mod::Chord chord = mod::Chord::from_model_params(p);
  for (const auto &name : chord.names()) out.writeln(name, "(alpha0, beta, nu, rho) =", chord.at(name).model.parameters().items<double>());
}
```

Output

```text
asset1 (alpha0, beta, nu, rho) = [0.20000, 0.70000, 0.50000, -0.30000]
asset2 (alpha0, beta, nu, rho) = [0.40000, 0.70000, 0.50000, -0.30000]
```


### `beta`

`std::vector<double>`, read by Sabr. The CEV exponent of a SABR asset, in [0, 1]: the forward diffuses with volatility `α F^β`, so 1 is lognormal and 0 is normal.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Chord::Parameters base() {
  mod::Chord::Parameters p;
  p.spot = {100.0, 90.0};
  p.kappa = {1.5};
  p.theta = {0.04};
  p.sigma = {0.3};
  p.rho = {-0.7};
  p.v0 = {0.04};
  p.controls.mc.paths = 20000;
  p.controls.mc.steps = 20;
  p.controls.mc.seed = 3;
  return p;
}
static double corr(const pluss::Series &a, const pluss::Series &b) { return a.mul(b).mean().item<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Chord::Parameters p;
  p.kind = {fin::model::Kind::Sabr};
  p.spot = {100.0, 100.0};
  p.alpha0 = {0.3};
  p.beta = {0.0, 1.0};
  p.nu = {0.5};
  p.rho = {-0.3};
  const mod::Chord chord = mod::Chord::from_model_params(p);
  for (const auto &name : chord.names()) out.writeln(name, "beta =", chord.at(name).model.parameters().items<double>()[1]);
}
```

Output

```text
asset1 beta = 0.00000
asset2 beta = 1.00000
```


### `nu`

`std::vector<double>`, read by Sabr. The volatility of the volatility factor of a SABR asset, 0 or more.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Chord::Parameters base() {
  mod::Chord::Parameters p;
  p.spot = {100.0, 90.0};
  p.kappa = {1.5};
  p.theta = {0.04};
  p.sigma = {0.3};
  p.rho = {-0.7};
  p.v0 = {0.04};
  p.controls.mc.paths = 20000;
  p.controls.mc.steps = 20;
  p.controls.mc.seed = 3;
  return p;
}
static double corr(const pluss::Series &a, const pluss::Series &b) { return a.mul(b).mean().item<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Chord::Parameters p;
  p.kind = {fin::model::Kind::Sabr};
  p.spot = {100.0, 100.0};
  p.alpha0 = {0.3};
  p.beta = {0.7};
  p.nu = {0.2, 0.8};
  p.rho = {-0.3};
  const mod::Chord chord = mod::Chord::from_model_params(p);
  for (const auto &name : chord.names()) out.writeln(name, "nu =", chord.at(name).model.parameters().items<double>()[2]);
}
```

Output

```text
asset1 nu = 0.20000
asset2 nu = 0.80000
```


### `jump_intensity`

`std::vector<double>`, default empty. The expected number of jumps a year of each asset. Read by the Bates model. Without a `kind` column, giving this column together with `jump_mean` and `jump_vol` makes the assets Bates instead of Heston, and the three are given together or not at all; with a `kind` column the Bates assets read them.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Chord::Parameters base() {
  mod::Chord::Parameters p;
  p.spot = {100.0, 90.0};
  p.kappa = {1.5};
  p.theta = {0.04};
  p.sigma = {0.3};
  p.rho = {-0.7};
  p.v0 = {0.04};
  p.controls.mc.paths = 20000;
  p.controls.mc.steps = 20;
  p.controls.mc.seed = 3;
  return p;
}
static double corr(const pluss::Series &a, const pluss::Series &b) { return a.mul(b).mean().item<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Chord::Parameters p = base();
  p.jump_intensity = {0.15, 0.12};
  p.jump_mean = {-0.08};
  p.jump_vol = {0.2};
  const mod::Chord chord = mod::Chord::from_model_params(p);
  for (const auto &name : chord.names()) out.writeln(name, chord.at(name).model.name(), "lambda =", chord.at(name).model.parameters().items<double>()[5]);
}
```

Output

```text
asset1 Bates lambda = 0.15000
asset2 Bates lambda = 0.12000
```


### `jump_mean`

`std::vector<double>`, default empty. The mean of the log jump size of each asset (negative for downward jumps). Given with the other two jump columns.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Chord::Parameters base() {
  mod::Chord::Parameters p;
  p.spot = {100.0, 90.0};
  p.kappa = {1.5};
  p.theta = {0.04};
  p.sigma = {0.3};
  p.rho = {-0.7};
  p.v0 = {0.04};
  p.controls.mc.paths = 20000;
  p.controls.mc.steps = 20;
  p.controls.mc.seed = 3;
  return p;
}
static double corr(const pluss::Series &a, const pluss::Series &b) { return a.mul(b).mean().item<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Chord::Parameters p = base();
  p.jump_intensity = {0.15};
  p.jump_mean = {-0.08, -0.02};
  p.jump_vol = {0.2};
  const mod::Chord chord = mod::Chord::from_model_params(p);
  for (const auto &name : chord.names()) out.writeln(name, "jump mean =", chord.at(name).model.parameters().items<double>()[6]);
}
```

Output

```text
asset1 jump mean = -0.08000
asset2 jump mean = -0.02000
```


### `jump_vol`

`std::vector<double>`, default empty. The standard deviation of the log jump size of each asset, 0 or more. Given with the other two jump columns; giving only some of the three is refused.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Chord::Parameters base() {
  mod::Chord::Parameters p;
  p.spot = {100.0, 90.0};
  p.kappa = {1.5};
  p.theta = {0.04};
  p.sigma = {0.3};
  p.rho = {-0.7};
  p.v0 = {0.04};
  p.controls.mc.paths = 20000;
  p.controls.mc.steps = 20;
  p.controls.mc.seed = 3;
  return p;
}
static double corr(const pluss::Series &a, const pluss::Series &b) { return a.mul(b).mean().item<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const std::exception &e) {
      std::string text(e.what());
      if (const auto cut = text.find("Triggered"); cut != std::string::npos) text.erase(cut);
      while (!text.empty() && text.back() == '\n') text.pop_back();
      out.writeln(text);
    }
  };
  mod::Chord::Parameters p = base();
  p.jump_intensity = {0.15};
  p.jump_mean = {-0.08};
  p.jump_vol = {0.2, 0.1};
  const mod::Chord chord = mod::Chord::from_model_params(p);
  for (const auto &name : chord.names()) out.writeln(name, "jump vol =", chord.at(name).model.parameters().items<double>()[7]);
  mod::Chord::Parameters partial = base();
  partial.jump_vol = {0.2};
  report([&] { (void)mod::Chord::from_model_params(partial); });
}
```

Output

```text
asset1 jump vol = 0.20000
asset2 jump vol = 0.10000
subsp::math::finance::module::Chord::from_model_params: jump_intensity, jump_mean and jump_vol must be given together (all three for Bates, none for Heston)
```


### `alpha`

`std::vector<double>`, read by RoughHeston and RoughBergomi. The roughness of the variance, `H − 1/2` with `H` the Hurst exponent, in (−0.5, 0): the closer to −0.5, the rougher. It is the kernel exponent of the Volterra variance. Not to be confused with `alpha0`, the initial volatility of a SABR asset.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Chord::Parameters base() {
  mod::Chord::Parameters p;
  p.spot = {100.0, 90.0};
  p.kappa = {1.5};
  p.theta = {0.04};
  p.sigma = {0.3};
  p.rho = {-0.7};
  p.v0 = {0.04};
  p.controls.mc.paths = 20000;
  p.controls.mc.steps = 20;
  p.controls.mc.seed = 3;
  return p;
}
static double corr(const pluss::Series &a, const pluss::Series &b) { return a.mul(b).mean().item<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Chord::Parameters p;
  p.kind = {fin::model::Kind::RoughBergomi};
  p.spot = {100.0, 100.0};
  p.xi = {0.04};
  p.eta = {1.2};
  p.alpha = {-0.4, -0.1};
  p.rho = {-0.7};
  const mod::Chord chord = mod::Chord::from_model_params(p);
  for (const auto &name : chord.names()) out.writeln(name, "alpha", chord.at(name).model.parameters().items<double>()[2], " Hurst exponent", chord.at(name).model.parameters().items<double>()[2] + 0.5);
}
```

Output

```text
asset1 alpha -0.40000  Hurst exponent 0.10000
asset2 alpha -0.10000  Hurst exponent 0.40000
```


### `xi`

`std::vector<double>`, read by RoughBergomi. The initial forward variance of a rough Bergomi asset, greater than 0: its variance process has mean `xi` at every date.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Chord::Parameters base() {
  mod::Chord::Parameters p;
  p.spot = {100.0, 90.0};
  p.kappa = {1.5};
  p.theta = {0.04};
  p.sigma = {0.3};
  p.rho = {-0.7};
  p.v0 = {0.04};
  p.controls.mc.paths = 20000;
  p.controls.mc.steps = 20;
  p.controls.mc.seed = 3;
  return p;
}
static double corr(const pluss::Series &a, const pluss::Series &b) { return a.mul(b).mean().item<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Chord::Parameters p;
  p.kind = {fin::model::Kind::RoughBergomi};
  p.spot = {100.0, 100.0};
  p.xi = {0.04, 0.09};
  p.eta = {1.2};
  p.alpha = {-0.3};
  p.rho = {-0.7};
  p.controls.mc.paths = 20000;
  p.controls.mc.steps = 20;
  p.controls.mc.seed = 3;
  const mod::Chord chord = mod::Chord::from_model_params(p);
  const auto paths = chord.simulate_paths();
  for (int a = 0; a < 2; ++a) out.writeln(chord.names()[a], "xi =", chord.at(chord.names()[a]).model.parameters().items<double>()[0], " mean variance at T", paths.variance.select(2, a).select(1, 20).mean().item<double>());
}
```

Output

```text
asset1 xi = 0.04000  mean variance at T 0.03933
asset2 xi = 0.09000  mean variance at T 0.08994
```


### `eta`

`std::vector<double>`, read by RoughBergomi. The volatility of the log variance of a rough Bergomi asset, greater than 0: the log variance at `T` has standard deviation `eta T^H`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Chord::Parameters base() {
  mod::Chord::Parameters p;
  p.spot = {100.0, 90.0};
  p.kappa = {1.5};
  p.theta = {0.04};
  p.sigma = {0.3};
  p.rho = {-0.7};
  p.v0 = {0.04};
  p.controls.mc.paths = 20000;
  p.controls.mc.steps = 20;
  p.controls.mc.seed = 3;
  return p;
}
static double corr(const pluss::Series &a, const pluss::Series &b) { return a.mul(b).mean().item<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Chord::Parameters p;
  p.kind = {fin::model::Kind::RoughBergomi};
  p.spot = {100.0, 100.0};
  p.xi = {0.04};
  p.eta = {0.6, 1.2};
  p.alpha = {-0.3};
  p.rho = {-0.7};
  p.controls.mc.paths = 20000;
  p.controls.mc.steps = 20;
  p.controls.mc.seed = 3;
  const auto paths = mod::Chord::from_model_params(p).simulate_paths();
  for (int a = 0; a < 2; ++a) {
    const pluss::Series log_v = paths.variance.select(2, a).select(1, 20).log();
    out.writeln("eta", std::vector<double>{0.6, 1.2}[a], " sd of log variance at T", std::sqrt(log_v.sub(log_v.mean()).square().mean().item<double>()), " (eta T^H, H = 0.2: eta)");
  }
}
```

Output

```text
eta 0.60000  sd of log variance at T 0.59951  (eta T^H, H = 0.2: eta)
eta 1.20000  sd of log variance at T 1.19789  (eta T^H, H = 0.2: eta)
```


### `maturity`

`std::vector<double>`, default {1}. The horizon of each asset in years: the maturity of the call each Field carries, and, through the longest, the time to which `simulate_paths` simulates all assets. One entry applies to all.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Chord::Parameters base() {
  mod::Chord::Parameters p;
  p.spot = {100.0, 90.0};
  p.kappa = {1.5};
  p.theta = {0.04};
  p.sigma = {0.3};
  p.rho = {-0.7};
  p.v0 = {0.04};
  p.controls.mc.paths = 20000;
  p.controls.mc.steps = 20;
  p.controls.mc.seed = 3;
  return p;
}
static double corr(const pluss::Series &a, const pluss::Series &b) { return a.mul(b).mean().item<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Chord::Parameters p = base();
  p.maturity = {0.5, 2.0};
  const mod::Chord chord = mod::Chord::from_model_params(p);
  for (const auto &name : chord.names()) out.writeln(name, "maturity", chord.at(name).instrument.maturity.items<double>());
  out.writeln("horizon of the simulation (the longest):", chord.simulate_paths().horizon);
}
```

Output

```text
asset1 maturity [0.50000]
asset2 maturity [2.00000]
horizon of the simulation (the longest): 2.00000
```


### `strike`

`std::vector<double>`, default the spot. The strike of the European call each Field carries, which is what `price` values. Left empty it is the spot, so the call is at the money. It does not affect the simulated paths.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Chord::Parameters base() {
  mod::Chord::Parameters p;
  p.spot = {100.0, 90.0};
  p.kappa = {1.5};
  p.theta = {0.04};
  p.sigma = {0.3};
  p.rho = {-0.7};
  p.v0 = {0.04};
  p.controls.mc.paths = 20000;
  p.controls.mc.steps = 20;
  p.controls.mc.seed = 3;
  return p;
}
static double corr(const pluss::Series &a, const pluss::Series &b) { return a.mul(b).mean().item<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Chord::Parameters p = base();
  mod::Chord atm = mod::Chord::from_model_params(p);
  p.strike = {110.0, 80.0};
  mod::Chord custom = mod::Chord::from_model_params(p);
  for (const auto &name : atm.names()) out.writeln(name, "default strike", atm.at(name).instrument.strike.items<double>(), " given", custom.at(name).instrument.strike.items<double>());
}
```

Output

```text
asset1 default strike [100.00000]  given [110.00000]
asset2 default strike [90.00000]  given [80.00000]
```


### `controls`

`module::Controls`, default the Monte Carlo engine. The controls every asset carries: the engine, the Monte Carlo paths, steps, scheme, random numbers and seed, and the risk setting. `simulate_paths` needs equal `controls.mc` across assets, which a table guarantees by sharing one `Controls`. Change the engine to price the book with another method: the parameters are the same.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Chord::Parameters base() {
  mod::Chord::Parameters p;
  p.spot = {100.0, 90.0};
  p.kappa = {1.5};
  p.theta = {0.04};
  p.sigma = {0.3};
  p.rho = {-0.7};
  p.v0 = {0.04};
  p.controls.mc.paths = 20000;
  p.controls.mc.steps = 20;
  p.controls.mc.seed = 3;
  return p;
}
static double corr(const pluss::Series &a, const pluss::Series &b) { return a.mul(b).mean().item<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Chord::Parameters p = base();
  p.controls.mc.paths = 5000;
  p.controls.mc.steps = 10;
  p.controls.mc.seed = 11;
  const mod::Chord chord = mod::Chord::from_model_params(p);
  out.writeln("engine", chord.at("asset1").engine_name(), " paths", chord.at("asset1").controls.mc.paths, " steps", chord.at("asset1").controls.mc.steps, " seed", chord.at("asset1").controls.mc.seed);
  p.controls.engine = mod::Engine::Cos;
  p.controls.risk = mod::Risk::PriceOnly;
  const mod::Chord cos = mod::Chord::from_model_params(p);
  out.writeln("with the COS engine the book prices in", cos.batches(), "call:", cos.price().price.items<double>());
}
```

Output

```text
engine MonteCarlo  paths 5000  steps 10  seed 11
with the COS engine the book prices in 1 call: [7.54027, 6.78624]
```


### `price_corr`

`pluss::Series`, default undefined. The `[assets, assets]` correlation matrix of the price shocks `dW^S` of the assets, with a unit diagonal, symmetric and positive definite. Undefined means independent. This single matrix is enough to correlate the whole system: the variance shocks inherit correlation through each asset's `rho`, so that the variance shocks of two assets correlate at `ρ_aρ_b` times the price correlation. The program sets 0.6 and measures the price shocks and the variance shocks of the two assets.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Chord::Parameters base() {
  mod::Chord::Parameters p;
  p.spot = {100.0, 90.0};
  p.kappa = {1.5};
  p.theta = {0.04};
  p.sigma = {0.3};
  p.rho = {-0.7};
  p.v0 = {0.04};
  p.controls.mc.paths = 20000;
  p.controls.mc.steps = 20;
  p.controls.mc.seed = 3;
  return p;
}
static double corr(const pluss::Series &a, const pluss::Series &b) { return a.mul(b).mean().item<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Chord::Parameters p = base();
  p.price_corr = pluss::make_series({1.0, 0.6, 0.6, 1.0}, {2, 2});
  const auto paths = mod::Chord::from_model_params(p).simulate_paths();
  out.writeln("price shocks a with b      ", corr(paths.asset_shocks.select(2, 0), paths.asset_shocks.select(2, 1)), " (0.6)");
  out.writeln("variance shocks a with b   ", corr(paths.vol_shocks.select(2, 0), paths.vol_shocks.select(2, 1)), " (rho_a rho_b R_s = 0.49 x 0.6 = 0.294)");
  out.writeln("price a with variance b    ", corr(paths.asset_shocks.select(2, 0), paths.vol_shocks.select(2, 1)), " (rho_b R_s = -0.7 x 0.6 = -0.42)");
}
```

Output

```text
price shocks a with b       0.60170  (0.6)
variance shocks a with b    0.29477  (rho_a rho_b R_s = 0.49 x 0.6 = 0.294)
price a with variance b     -0.42200  (rho_b R_s = -0.7 x 0.6 = -0.42)
```


### `var_corr`

`pluss::Series`, default undefined. The `[assets, assets]` correlation matrix of the variance shocks `dW^v` of the assets, with a unit diagonal, symmetric and positive definite: the correlation between the variance Brownians themselves. Undefined means the variance shocks are related only through the price shocks. With `price_corr` and each `ρ`, the cross terms are built (see `price_var_cross`) and the part of each variance shock that its price shock does not explain is correlated so that the variance shocks correlate by exactly this matrix. That needs `var_corr` to be one that `price_corr` and `ρ` allow: the variance shocks of two assets with ρ = −0.7 and independent prices can correlate at most 0.51. A matrix that asks for more is refused when the paths are simulated.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Chord::Parameters base() {
  mod::Chord::Parameters p;
  p.spot = {100.0, 90.0};
  p.kappa = {1.5};
  p.theta = {0.04};
  p.sigma = {0.3};
  p.rho = {-0.7};
  p.v0 = {0.04};
  p.controls.mc.paths = 20000;
  p.controls.mc.steps = 20;
  p.controls.mc.seed = 3;
  return p;
}
static double corr(const pluss::Series &a, const pluss::Series &b) { return a.mul(b).mean().item<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const std::exception &e) {
      std::string text(e.what());
      if (const auto cut = text.find("Triggered"); cut != std::string::npos) text.erase(cut);
      while (!text.empty() && text.back() == '\n') text.pop_back();
      out.writeln(text);
    }
  };
  mod::Chord::Parameters p = base();
  p.var_corr = pluss::make_series({1.0, 0.4, 0.4, 1.0}, {2, 2});
  const auto paths = mod::Chord::from_model_params(p).simulate_paths();
  out.writeln("variance shocks a with b", corr(paths.vol_shocks.select(2, 0), paths.vol_shocks.select(2, 1)), " (0.4)");
  out.writeln("price shocks a with b   ", corr(paths.asset_shocks.select(2, 0), paths.asset_shocks.select(2, 1)), " (independent)");
  p.var_corr = pluss::make_series({1.0, 0.9, 0.9, 1.0}, {2, 2});   // more than the 0.51 that independent prices and rho = -0.7 allow
  report([&] { (void)mod::Chord::from_model_params(p).simulate_paths(); });
}
```

Output

```text
variance shocks a with b 0.40062  (0.4)
price shocks a with b    0.00012  (independent)
subsp::math::finance::module::Chord::simulate_paths: the correlation of the independent remainders of the variance shocks implied by var_corr, price_corr and rho (var_corr is too far from what the price correlation and rho allow; give price_var_cross to set the cross terms yourself) must be positive definite
```


### `price_var_cross`

`pluss::Series`, default undefined. The `[assets, assets]` cross correlation between the price shock of asset a and the variance shock of asset b. When undefined it is built: each variance shock is `ρ` times its own price shock plus an independent remainder, so the cross terms are `ρ_b` times `price_corr[a][b]`. When given it is used as it is, the diagonal is replaced by each asset's `ρ`, and `price_corr`, `var_corr` and this matrix together must form one valid 2A × 2A correlation matrix, which is checked when the paths are simulated. Not every combination is valid: with ρ = −0.7, a variance correlation of 0.8 and a cross correlation of +0.1 is impossible, because assets whose variances move together while each has a strong negative price-variance link need negative cross terms. Use it when you have estimated the cross correlations.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Chord::Parameters base() {
  mod::Chord::Parameters p;
  p.spot = {100.0, 90.0};
  p.kappa = {1.5};
  p.theta = {0.04};
  p.sigma = {0.3};
  p.rho = {-0.7};
  p.v0 = {0.04};
  p.controls.mc.paths = 20000;
  p.controls.mc.steps = 20;
  p.controls.mc.seed = 3;
  return p;
}
static double corr(const pluss::Series &a, const pluss::Series &b) { return a.mul(b).mean().item<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const std::exception &e) {
      std::string text(e.what());
      if (const auto cut = text.find("Triggered"); cut != std::string::npos) text.erase(cut);
      while (!text.empty() && text.back() == '\n') text.pop_back();
      out.writeln(text);
    }
  };
  mod::Chord::Parameters p = base();
  p.price_corr = pluss::make_series({1.0, 0.6, 0.6, 1.0}, {2, 2});
  p.var_corr = pluss::make_series({1.0, 0.5, 0.5, 1.0}, {2, 2});
  p.price_var_cross = pluss::make_series({-0.7, -0.4, -0.4, -0.7}, {2, 2});
  const auto paths = mod::Chord::from_model_params(p).simulate_paths();
  out.writeln("price a with variance b   ", corr(paths.asset_shocks.select(2, 0), paths.vol_shocks.select(2, 1)), " (-0.4)");
  out.writeln("variance a with variance b", corr(paths.vol_shocks.select(2, 0), paths.vol_shocks.select(2, 1)), " (0.5, now the correlation of the variance shocks)");
  p.price_var_cross = pluss::make_series({-0.7, 0.99, 0.99, -0.7}, {2, 2});
  report([&] { (void)mod::Chord::from_model_params(p).simulate_paths(); });
}
```

Output

```text
price a with variance b    -0.40217  (-0.4)
variance a with variance b 0.50112  (0.5, now the correlation of the variance shocks)
subsp::math::finance::module::Chord::simulate_paths: the joint correlation of the price and variance shocks (price_corr, var_corr, price_var_cross and rho) must be positive definite
```


## Practical Examples

### Example 1: a table of five assets

Five Heston assets from columns of numbers, with a price correlation. One entry for the rate and the yield applies to all five.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static double corr(const pluss::Series &a, const pluss::Series &b) { return a.mul(b).mean().item<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  mod::Chord::Parameters p;
  p.spot = {1290, 1370, 1302, 1260, 1405};
  p.rate = {0.019};
  p.dividend_yield = {0.012};
  p.kappa = {1.6, 2.1, 1.9, 1.4, 2.3};
  p.theta = {0.045, 0.04, 0.055, 0.035, 0.05};
  p.sigma = {0.35, 0.4, 0.45, 0.3, 0.45};
  p.rho = {-0.7, -0.5, -0.65, -0.55, -0.6};
  p.v0 = {0.035, 0.05, 0.025, 0.04, 0.03};
  p.controls.mc.paths = 5000;
  p.controls.mc.steps = 20;
  p.controls.mc.seed = 7;
  p.price_corr = pluss::make_series({1.0, 0.35, 0.25, 0.30, 0.20,
                                     0.35, 1.0, 0.15, 0.25, 0.10,
                                     0.25, 0.15, 1.0, 0.20, 0.15,
                                     0.30, 0.25, 0.20, 1.0, 0.25,
                                     0.20, 0.10, 0.15, 0.25, 1.0}, {5, 5}, f64);
  const mod::Chord book = mod::Chord::from_model_params(p);
  const auto paths = book.simulate_paths();
  out.writeln(book.names());
  out.writeln("paths", paths.asset.sizes(), " price shock a with c", corr(paths.asset_shocks.select(2, 0), paths.asset_shocks.select(2, 2)), " (0.25)");
}
```

Output

```text
[asset1, asset2, asset3, asset4, asset5]
paths [5000, 21, 5]  price shock a with c 0.250  (0.25)
```

### Example 2: Bates assets and names

Adding the three jump columns makes the assets Bates; names label them.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Chord::Parameters base() {
  mod::Chord::Parameters p;
  p.spot = {100.0, 90.0};
  p.kappa = {1.5};
  p.theta = {0.04};
  p.sigma = {0.3};
  p.rho = {-0.7};
  p.v0 = {0.04};
  p.controls.mc.paths = 20000;
  p.controls.mc.steps = 20;
  p.controls.mc.seed = 3;
  return p;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Chord::Parameters p = base();
  p.names = {"spx", "ndx"};
  p.jump_intensity = {0.15, 0.12};
  p.jump_mean = {-0.08, -0.05};
  p.jump_vol = {0.2, 0.1};
  const mod::Chord book = mod::Chord::from_model_params(p);
  for (const auto &name : book.names()) out.writeln(name, book.at(name).model.name());
}
```

Output

```text
spx Bates
ndx Bates
```

### Example 3: the table builds the same Chord as writing every Field

Fields written by hand and the table give identical simulated paths.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Chord::Parameters base() {
  mod::Chord::Parameters p;
  p.spot = {100.0, 90.0};
  p.kappa = {1.5};
  p.theta = {0.04};
  p.sigma = {0.3};
  p.rho = {-0.7};
  p.v0 = {0.04};
  p.controls.mc.paths = 20000;
  p.controls.mc.steps = 20;
  p.controls.mc.seed = 3;
  return p;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Chord::Parameters p = base();
  p.rate = {0.02};
  p.dividend_yield = {0.0};
  const mod::Chord from_table = mod::Chord::from_model_params(p);
  const auto field = [&](double spot) {
    mod::Field f{.instrument = fin::instrument::make(fin::pricing::Call, spot, 1.0), .market = fin::market::make(spot, 0.02, 0.0),
                 .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}), .controls = p.controls};
    return f;
  };
  const mod::Chord by_hand{{"asset1", field(100.0)}, {"asset2", field(90.0)}};
  const auto a = from_table.simulate_paths(), b = by_hand.simulate_paths();
  out.writeln("largest difference in the simulated paths:", a.asset.sub(b.asset).abs().max().item<double>());
}
```

Output

```text
largest difference in the simulated paths: 0.00000
```

### Example 4: the errors

A missing required column, a column of the wrong length, partial jump columns and a correlation matrix of the wrong size are refused, naming the problem.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Chord::Parameters base() {
  mod::Chord::Parameters p;
  p.spot = {100.0, 90.0};
  p.kappa = {1.5};
  p.theta = {0.04};
  p.sigma = {0.3};
  p.rho = {-0.7};
  p.v0 = {0.04};
  p.controls.mc.paths = 20000;
  p.controls.mc.steps = 20;
  p.controls.mc.seed = 3;
  return p;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const std::exception &e) {
      std::string text(e.what());
      if (const auto cut = text.find("Triggered"); cut != std::string::npos) text.erase(cut);
      while (!text.empty() && text.back() == '\n') text.pop_back();
      out.writeln(text);
    }
  };
  mod::Chord::Parameters missing = base();
  missing.v0.clear();
  report([&] { (void)mod::Chord::from_model_params(missing); });
  mod::Chord::Parameters wrong = base();
  wrong.kappa = {1.5, 2.0, 2.5};
  report([&] { (void)mod::Chord::from_model_params(wrong); });
  mod::Chord::Parameters shape = base();
  shape.price_corr = pluss::make_series({1.0, 0.5, 0.5, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0}, {3, 3});
  report([&] { (void)mod::Chord::from_model_params(shape); });
}
```

Output

```text
subsp::math::finance::module::Chord::from_model_params: the column 'v0' is required (the Heston model of asset 'asset1' uses it)
subsp::math::finance::module::Chord::from_model_params: the column 'spot' has 2 entries; it needs 1 or 3
subsp::math::finance::module::Chord::from_model_params: price_corr must be an [2, 2] matrix, one row and column per asset
```

## Changelog

- **0.1**: initial documented surface.

## See Also

- [`Chord::from_model_params`](Chord/from_model_params.md): builds the Chord from this table
- [`Chord`](Chord.md): the class, and the correlation members that the table sets
- [`Chord::simulate_paths`](Chord/simulate_paths.md): the call that uses the correlations

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Result.html">
    <span class="label">Previous</span>
    <span class="title">Result</span>
  </a>
  <a class="next-link" href="Paths.html">
    <span class="label">Next</span>
    <span class="title">Paths</span>
  </a>
</div>
<!-- apiref:nav:end -->
