<!-- generated from from_model_params.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a9b5a2a522dd -->

# `module::Chord::from_model_params`

[Back to `module::Chord`](../Chord.md)

Builds a Chord of assets of any of the six models from a table of their model parameters.

## Signature

```cpp
static module::Chord module::Chord::from_model_params(const module::Chord::Parameters &parameters);
```

## What It Does

Takes the parameters of several assets as columns, a [`Chord::Parameters`](../Parameters.md), and returns a Chord with one asset per row, of the model the `kind` column names (Black-Scholes, SABR, Heston, Bates, rough Heston or rough Bergomi; Heston when there is no `kind` column, Bates when the three jump columns are given without one): you do not write a `Field` for each. Each asset gets a European call at its strike (the spot unless a strike column is given) and the maturity, the market of its spot, rate and yield, its model, and the shared controls, which default to the Monte Carlo engine. The assets are named by the `names` column, or `asset1`, `asset2`, ... . Each asset reads only the columns its model uses, so a table of mixed models holds a filler (NaN by custom) wherever an asset does not use a column. The cross-asset correlation matrices of the table are set on the Chord. Use it when the parameters are already numbers, from a calibration or a file you read, and the Chord is wanted for what it does with several assets: simulating them together, or pricing their options in one call.

**How the shocks and correlations are built.** `price_corr` is the correlation between the price shocks of the assets, `var_corr` the correlation between their variance shocks, and `price_var_cross` the cross correlation between one asset's price shock and another's variance shock. Nothing is estimated: an undefined matrix means identity (independent). If you give the first two, the cross correlations are built from them. The model of each asset fixes the correlation `ρ` between its own price shock `dW^S` and variance shock `dW^v`; the matrices relate assets to one another, and an undefined matrix means identity (independent), not an estimate:

| What you give | The shocks |
|---|---|
| Nothing | The assets are independent. Within an asset, `dW^v = ρ dW^S + √(1 − ρ²) dW^⊥`, and the `dW^⊥` of different assets are independent. |
| `price_corr` (R_s) | The price shocks correlate by R_s. The variance shocks inherit it through `ρ`: corr(`dW^{v,a}`, `dW^{v,b}`) = ρ_aρ_b R_s[a][b], and the cross terms are corr(`dW^{S,a}`, `dW^{v,b}`) = ρ_b R_s[a][b]. |
| `price_corr` and `var_corr` (R_v) | The variance shocks correlate by exactly R_v. The cross terms are still built as ρ_b R_s[a][b], and the independent remainders `dW^⊥` are given the correlation (R_v − ρ_aρ_b R_s) / (√(1 − ρ_a²) √(1 − ρ_b²)), which must be a valid correlation matrix: R_v must be one that R_s and `ρ` allow, else `simulate_paths` refuses it. |
| also `price_var_cross` (C) | The cross block is used as given instead of built: the explicit 2A × 2A matrix [[R_s, C], [Cᵀ, R_v]] with the diagonal of C replaced by each `ρ`, which you must make valid. |

The shocks themselves are drawn when [`simulate_paths`](simulate_paths.md) runs, from the seed in the controls, with the Cholesky factor of the matrix (or of the joint matrix) applied to independent normals. The correlations are validated there, not here: a matrix of the wrong size is refused at construction, but one that is not symmetric or not positive definite is reported by `simulate_paths`.

## Parameters

`const module::Chord::Parameters &parameters`
: The table. `spot` is always required, and so is every column that the model of some asset reads (the list is on the Parameters page); a column has one entry (applied to every asset) or one per asset. See [`Chord::Parameters`](../Parameters.md) for every field.

## Returns

A Chord with `parallel` true and the three correlation members set from the table (undefined if not given).

## Errors

Throws `subsp::math::finance::module::Chord::from_model_params:` followed by one of: `the column '<name>' is required (the <Model> model of asset '<asset>' uses it)` for a missing column that an asset's model reads; `the column '<name>' has <n> entries; it needs 1 or <assets>` for a column of the wrong length (the number of assets is the longest column's length, so the error may name a shorter column when another is too long); `names has <n> entries; it needs <assets>`; `jump_intensity, jump_mean and jump_vol must be given together (all three for Bates, none for Heston)` when there is no `kind` column; `kind has <n> entries; it needs 1 or <assets>`; `entry <i> of the column '<name>' (asset '<asset>') must be finite` for a NaN or infinite entry that the model of that asset reads; `the column '<name>' is not used by the model of any asset` for a column that nothing reads; `there are no assets`; or `<matrix> must be an [<assets>, <assets>] matrix, one row and column per asset` for a correlation matrix of the wrong size. Parameters outside their domain (a negative `kappa`, a `rho` outside (−1, 1)) are reported by `validate`, `price` or `simulate_paths`, naming the asset.

## Practical Examples

### Example 1: three Heston assets

The parameters as columns; one entry for the rate and the yield applies to all three. The assets are named by position and each has an at-the-money call.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Chord::Parameters p;
  p.spot = {100.0, 50.0, 80.0};
  p.rate = {0.03};
  p.dividend_yield = {0.01};
  p.kappa = {1.5, 2.0, 1.0};
  p.theta = {0.04, 0.06, 0.05};
  p.sigma = {0.3, 0.4, 0.25};
  p.rho = {-0.7, 0.0, -0.4};
  p.v0 = {0.09, 0.03, 0.05};
  p.controls.mc.paths = 20000;
  p.controls.mc.steps = 20;
  p.controls.mc.seed = 5;
  const mod::Chord book = mod::Chord::from_model_params(p);
  out.writeln(book.names(), " size", book.size());
  for (const auto &name : book.names()) out.writeln(name, book.at(name).model.name(), "strike", book.at(name).instrument.strike.items<double>(), "kappa", book.at(name).model.parameters().items<double>()[0]);
}
```

Output

```text
[asset1, asset2, asset3]  size 3
asset1 Heston strike [100.00000] kappa 1.50000
asset2 Heston strike [50.00000] kappa 2.00000
asset3 Heston strike [80.00000] kappa 1.00000
```

### Example 2: correlating only the prices

One price correlation of 0.6 correlates the whole system: the variance shocks follow through each rho.

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
  out.print_defaults(PrintOptions{}.fixed(3));
  mod::Chord::Parameters p = base();
  p.price_corr = pluss::make_series({1.0, 0.6, 0.6, 1.0}, {2, 2});
  const auto paths = mod::Chord::from_model_params(p).simulate_paths();
  out.writeln("price shocks             ", corr(paths.asset_shocks.select(2, 0), paths.asset_shocks.select(2, 1)), " (0.6)");
  out.writeln("variance shocks          ", corr(paths.vol_shocks.select(2, 0), paths.vol_shocks.select(2, 1)), " (0.49 x 0.6 = 0.294)");
  out.writeln("price a, variance b      ", corr(paths.asset_shocks.select(2, 0), paths.vol_shocks.select(2, 1)), " (-0.7 x 0.6 = -0.42)");
}
```

Output

```text
price shocks              0.602  (0.6)
variance shocks           0.295  (0.49 x 0.6 = 0.294)
price a, variance b       -0.422  (-0.7 x 0.6 = -0.42)
```

### Example 3: no correlation at all

With no matrices the assets are independent; only each asset's own rho remains.

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
  out.print_defaults(PrintOptions{}.fixed(3));
  const auto paths = mod::Chord::from_model_params(base()).simulate_paths();
  out.writeln("price shocks a with b   ", corr(paths.asset_shocks.select(2, 0), paths.asset_shocks.select(2, 1)), " (0)");
  out.writeln("variance shocks a with b", corr(paths.vol_shocks.select(2, 0), paths.vol_shocks.select(2, 1)), " (0)");
  out.writeln("price a with variance a ", corr(paths.asset_shocks.select(2, 0), paths.vol_shocks.select(2, 0)), " (rho = -0.7)");
}
```

Output

```text
price shocks a with b    0.000  (0)
variance shocks a with b -0.000  (0)
price a with variance a  -0.703  (rho = -0.7)
```

### Example 4: adding a variance correlation

With a variance correlation of 0.8 the variance shocks correlate at exactly 0.8, which the price correlation and the rho of the assets allow here; the cross terms are built.

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
  out.print_defaults(PrintOptions{}.fixed(3));
  mod::Chord::Parameters p = base();
  p.price_corr = pluss::make_series({1.0, 0.6, 0.6, 1.0}, {2, 2});
  p.var_corr = pluss::make_series({1.0, 0.8, 0.8, 1.0}, {2, 2});
  const auto paths = mod::Chord::from_model_params(p).simulate_paths();
  out.writeln("variance shocks a with b", corr(paths.vol_shocks.select(2, 0), paths.vol_shocks.select(2, 1)), " (0.8)");
}
```

Output

```text
variance shocks a with b 0.802  (0.8)
```

### Example 5: a variance correlation the prices cannot support

Two assets with rho = -0.7 and independent prices cannot have variance shocks correlated above 0.51; asking for 0.9 is refused when the paths are simulated, naming var_corr.

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
  p.var_corr = pluss::make_series({1.0, 0.9, 0.9, 1.0}, {2, 2});
  report([&] { (void)mod::Chord::from_model_params(p).simulate_paths(); });
}
```

Output

```text
subsp::math::finance::module::Chord::simulate_paths: the correlation of the independent remainders of the variance shocks implied by var_corr, price_corr and rho (var_corr is too far from what the price correlation and rho allow; give price_var_cross to set the cross terms yourself) must be positive definite
```

### Example 6: an explicit cross correlation

With price_var_cross the matrices become the blocks of one joint correlation matrix; an impossible combination is refused when the paths are simulated.

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
  out.print_defaults(PrintOptions{}.fixed(3));
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
  out.writeln("price a, variance b     ", corr(paths.asset_shocks.select(2, 0), paths.vol_shocks.select(2, 1)), " (-0.4)");
  out.writeln("variance a with b       ", corr(paths.vol_shocks.select(2, 0), paths.vol_shocks.select(2, 1)), " (0.5)");
  p.price_var_cross = pluss::make_series({-0.7, 0.99, 0.99, -0.7}, {2, 2});
  report([&] { (void)mod::Chord::from_model_params(p).simulate_paths(); });
}
```

Output

```text
price a, variance b      -0.402  (-0.4)
variance a with b        0.501  (0.5)
subsp::math::finance::module::Chord::simulate_paths: the joint correlation of the price and variance shocks (price_corr, var_corr, price_var_cross and rho) must be positive definite
```

### Example 7: six models in one table

A book with one asset of each model, from one table. The `kind` column names the model; each asset reads its own columns and the entries it does not read are NaN. All six are simulated together.

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
  out.print_defaults(PrintOptions{}.fixed(3));
  const double nan = std::numeric_limits<double>::quiet_NaN();
  using K = fin::model::Kind;
  mod::Chord::Parameters p;
  p.names = {"bs", "sabr", "heston", "bates", "rough_heston", "rough_bergomi"};
  p.kind = {K::BlackScholes, K::Sabr, K::Heston, K::Bates, K::RoughHeston, K::RoughBergomi};
  p.spot = {100.0, 100.0, 100.0, 100.0, 100.0, 100.0};
  p.volatility = {0.2, nan, nan, nan, nan, nan};
  p.alpha0 = {nan, 0.3, nan, nan, nan, nan};
  p.beta = {nan, 0.7, nan, nan, nan, nan};
  p.nu = {nan, 0.5, nan, nan, nan, nan};
  p.kappa = {nan, nan, 1.5, 1.5, 1.5, nan};
  p.theta = {nan, nan, 0.04, 0.04, 0.04, nan};
  p.sigma = {nan, nan, 0.3, 0.3, 0.3, nan};
  p.v0 = {nan, nan, 0.04, 0.04, 0.04, nan};
  p.jump_intensity = {nan, nan, nan, 0.15, nan, nan};
  p.jump_mean = {nan, nan, nan, -0.08, nan, nan};
  p.jump_vol = {nan, nan, nan, 0.2, nan, nan};
  p.alpha = {nan, nan, nan, nan, -0.1, -0.1};
  p.xi = {nan, nan, nan, nan, nan, 0.04};
  p.eta = {nan, nan, nan, nan, nan, 1.2};
  p.rho = {nan, -0.3, -0.7, -0.7, -0.7, -0.7};
  p.controls.mc.paths = 20000;
  p.controls.mc.steps = 20;
  p.controls.mc.seed = 3;
  const mod::Chord book = mod::Chord::from_model_params(p);
  for (const auto &name : book.names()) out.writeln(name, book.at(name).model.name());
  const auto paths = book.simulate_paths();
  out.writeln("paths", paths.asset.sizes());
  for (int a = 0; a < 6; ++a) out.writeln(book.names()[a], "mean at the horizon", paths.asset.select(1, 20).select(1, a).mean().item<double>(), " forward", 100.0);
}
```

Output

```text
bs BlackScholes
sabr Sabr
heston Heston
bates Bates
rough_heston RoughHeston
rough_bergomi RoughBergomi
paths [20000, 21, 6]
bs mean at the horizon 100.187  forward 100.000
sabr mean at the horizon 100.046  forward 100.000
heston mean at the horizon 100.198  forward 100.000
bates mean at the horizon 99.999  forward 100.000
rough_heston mean at the horizon 99.774  forward 100.000
rough_bergomi mean at the horizon 100.090  forward 100.000
```

### Example 8: a model per asset with one shared column

A column with one entry applies to every asset that reads it: `rho` here is shared by the SABR and the Heston asset, and `kind` has one entry per asset.

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
  out.print_defaults(PrintOptions{}.fixed(3));
  const double nan = std::numeric_limits<double>::quiet_NaN();
  using K = fin::model::Kind;
  mod::Chord::Parameters p;
  p.kind = {K::Sabr, K::Heston};
  p.spot = {100.0, 100.0};
  p.alpha0 = {0.3, nan};
  p.beta = {0.7, nan};
  p.nu = {0.5, nan};
  p.kappa = {nan, 1.5};
  p.theta = {nan, 0.04};
  p.sigma = {nan, 0.3};
  p.v0 = {nan, 0.04};
  p.rho = {-0.6};
  const mod::Chord book = mod::Chord::from_model_params(p);
  for (const auto &name : book.names()) out.writeln(name, book.at(name).model.name(), book.at(name).model.parameters().items<double>());
}
```

Output

```text
asset1 Sabr [0.300, 0.700, 0.500, -0.600]
asset2 Heston [1.500, 0.040, 0.300, -0.600, 0.040]
```

### Example 9: pricing the book

Each asset carries an at-the-money call, so the Chord can price it: the Monte Carlo assets share one simulation.

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
  out.print_defaults(PrintOptions{}.fixed(3));
  mod::Chord::Parameters p = base();
  p.rate = {0.02};
  p.controls.risk = mod::Risk::PriceOnly;
  const mod::Chord book = mod::Chord::from_model_params(p);
  const auto result = book.price();
  out.writeln("engine calls", book.batches());
  for (const auto &name : book.names()) out.writeln(name, result[name].price.items<double>());
}
```

Output

```text
engine calls 1
asset1 [8.684]
asset2 [7.814]
```

### Example 10: the errors

Missing, mis-sized and unused columns, a NaN the model reads, and a mis-sized matrix are refused, naming the column.

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
  mod::Chord::Parameters missing = base();
  missing.theta.clear();
  report([&] { (void)mod::Chord::from_model_params(missing); });
  mod::Chord::Parameters wrong = base();
  wrong.sigma = {0.3, 0.4, 0.5};
  report([&] { (void)mod::Chord::from_model_params(wrong); });
  mod::Chord::Parameters partial = base();
  partial.jump_intensity = {0.1};
  report([&] { (void)mod::Chord::from_model_params(partial); });
  mod::Chord::Parameters unused = base();
  unused.volatility = {0.2};
  report([&] { (void)mod::Chord::from_model_params(unused); });
  mod::Chord::Parameters kind_missing = base();
  kind_missing.kind = {fin::model::Kind::RoughBergomi};
  report([&] { (void)mod::Chord::from_model_params(kind_missing); });
  mod::Chord::Parameters not_finite = base();
  not_finite.kappa = {1.5, std::numeric_limits<double>::quiet_NaN()};
  report([&] { (void)mod::Chord::from_model_params(not_finite); });
  mod::Chord::Parameters shape = base();
  shape.var_corr = pluss::make_series({1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0}, {3, 3});
  report([&] { (void)mod::Chord::from_model_params(shape); });
}
```

Output

```text
subsp::math::finance::module::Chord::from_model_params: the column 'theta' is required (the Heston model of asset 'asset1' uses it)
subsp::math::finance::module::Chord::from_model_params: the column 'spot' has 2 entries; it needs 1 or 3
subsp::math::finance::module::Chord::from_model_params: jump_intensity, jump_mean and jump_vol must be given together (all three for Bates, none for Heston)
subsp::math::finance::module::Chord::from_model_params: the column 'volatility' is not used by the model of any asset
subsp::math::finance::module::Chord::from_model_params: the column 'xi' is required (the RoughBergomi model of asset 'asset1' uses it)
subsp::math::finance::module::Chord::from_model_params: entry 1 of the column 'kappa' (asset 'asset2') must be finite
subsp::math::finance::module::Chord::from_model_params: var_corr must be an [2, 2] matrix, one row and column per asset
```

## See Also

- [`Chord::Parameters`](../Parameters.md): every column of the table
- [`Chord`](../Chord.md): the class, and the correlation members
- [`simulate_paths`](simulate_paths.md): simulates the assets together with these correlations
- [`Chord::Chord`](Chord.md): builds a Chord from Fields written out

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="contains.html">
    <span class="label">Previous</span>
    <span class="title">contains</span>
  </a>
  <a class="next-link" href="implied_vol.html">
    <span class="label">Next</span>
    <span class="title">implied_vol</span>
  </a>
</div>
<!-- apiref:nav:end -->
