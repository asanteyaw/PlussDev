<!-- generated from simulate_paths.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6ffeba1c51b2 -->

# `module::Chord::simulate_paths`

[Back to `module::Chord`](../Chord.md)

Simulates all assets together, whatever their model (Black-Scholes, SABR, Heston, Bates, rough Heston or rough Bergomi), correlated across assets, in one vectorised pass.

## Signature

```cpp
module::Chord::Paths module::Chord::simulate_paths(Config cfg = {}) const;
```

## What It Does

Each asset is simulated with its own model, and the assets are correlated through their shocks. The supported models and what each contributes:

| Model | The asset path | Its second factor | `variance` in the result |
|---|---|---|---|
| Black-Scholes | the exact lognormal step | none | the constant σ² |
| Heston | the variance diffused by the chosen scheme | the variance shock | the variance |
| Bates | as Heston, with independent jumps | the variance shock | the variance |
| SABR | the forward, with a CEV diffusion | the shock of the volatility α | α² |
| Rough Heston | the Volterra variance built from the whole past of its shocks | the variance shock | the variance |
| Rough Bergomi | the exponential of the Riemann–Liouville process, built from the whole past of its shocks | the shock that drives the roughness | the variance |

The assets need not share a model or an engine: only the settings that drive the simulation must agree, `controls.mc` (paths, steps, scheme, random numbers, seed). The horizon is the longest maturity, and the simulation is risk neutral with growth `rate − dividend_yield` per asset (for SABR, the forward drifts at that rate). A Black-Scholes asset needs a single volatility. The rough models depend on the whole past of their shocks, so each keeps a history of its shocks of one column per step for every path (memory grows as paths × steps per rough asset) and weights it by its kernel at every step; they are simulated jointly with the others and are correlated through the same three matrices. Their asset price is advanced by the same exact lognormal step as the Black-Scholes asset, with the variance of the step taken at its start. The result is a [`Chord::Paths`](../Paths.md) with the asset axis last: `[paths, steps + 1, assets]`, in the order the assets were added.

**The correlation.** `price_corr` is the correlation between the price shocks of the assets, `var_corr` the correlation between their second-factor (variance or volatility) shocks and `price_var_cross` the cross correlation between one asset's price shock and another's second-factor shock. Within an asset the price and second-factor shocks keep the correlation ρ of its model: the second-factor shock is `dW^v = ρ dW^S + √(1 − ρ²) dW^⊥` with `dW^⊥` independent of the price shocks. A Black-Scholes asset has no second factor (ρ = 0), so its row and column of `var_corr` must be those of the identity and the entries of `price_var_cross` that name its (absent) second factor must be zero; the call is refused otherwise, naming the entry. The price shocks of the assets are correlated through the Cholesky factor of `price_corr`.

**Built cross correlations.** When `price_var_cross` is undefined it is built: the cross terms are corr(price_a, variance_b) = ρ_b R_s[a, b], and the remainders `dW^⊥` are given the correlation (R_v[a, b] − ρₐρ_b R_s[a, b]) / (√(1 − ρₐ²) √(1 − ρ_b²)), so that the variance shocks correlate by exactly `var_corr`. With `var_corr` undefined the remainders are independent and the variance shocks of two assets correlate at ρₐρ_b R_s[a, b]. The remainder correlation must itself be a valid correlation matrix, so `var_corr` must be one that `price_corr` and the ρ of the assets allow; otherwise the call is refused naming `var_corr`. Each matrix must be symmetric and positive definite with a unit diagonal.

**Explicit cross correlation.** The optional `price_var_cross` matrix replaces the built cross terms: when it is defined the off-diagonal entries are the cross correlations, each diagonal entry is replaced by the asset's own ρ whatever value is given, and the four blocks together must form one positive-definite matrix, or the call is refused.

Heston is simulated as Bates with no jumps; the variance of Heston and Bates assets uses Euler, Milstein, Milstein2 or QuadExp, and the Transition scheme, which needs one asset at a time, is refused. The scheme also reaches a SABR forward (Milstein and Milstein2 add the second-order term); a Black-Scholes asset is exact, and the rough models have a single scheme of their own, so `scheme` does not change them. Each asset's marginal law is the law its own `Field` simulates, whatever the correlation: the correlation only matters for payoffs that depend on several assets at once.

Call it to price or study a payoff on several assets, a basket, a spread or a worst-of, which needs the joint law and not each asset's own.

## Parameters

`Config cfg`
: Device of the paths; their type is Float64.

## Returns

A [`Chord::Paths`](../Paths.md).

## Errors

Throws `subsp::math::finance::module::Chord::simulate_paths:` followed by one of: `the Chord has no assets`;  `asset '<name>' needs a single Black-Scholes volatility to be simulated`; `var_corr[<a>, <b>] must be 0: '<a>' is a Black-Scholes asset and has no variance shock` and the same for `price_var_cross`; `asset '<name>' must have the same controls.mc (paths, steps, scheme, random numbers, seed) as the first asset`; `price_corr must be an [assets, assets] matrix`, `... must be symmetric`, `... must have a unit diagonal`, `... must be positive definite` (and the same for `var_corr`); and `the joint correlation of the price and variance shocks (...) must be positive definite`. The parameters of the models themselves are checked when the model is built, so a rough model whose `alpha` is outside (−0.5, 0) is refused by `model::make` before it reaches the Chord.

## Practical Examples

### Example 1: the shapes

Two assets, 2000 paths and 20 steps: the paths have 21 dates and the shocks 20, with the asset axis last.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Field heston_mc(double spot, double strike) {
  mod::Field f{.instrument = {.type = fin::pricing::Call, .strike = pluss::make_series({strike}), .maturity = pluss::make_series({1.0})}, .market = fin::market::make(spot, 0.02, 0.0),
               .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}), .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  f.controls.mc.paths = 20000;
  f.controls.mc.steps = 20;
  f.controls.mc.seed = 3;
  return f;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Chord chord;
  chord.add("a", heston_mc(100.0, 100.0)).add("b", heston_mc(90.0, 90.0));
  const auto paths = chord.simulate_paths();
  out.writeln("asset", paths.asset.sizes(), " variance", paths.variance.sizes());
  out.writeln("asset_shocks", paths.asset_shocks.sizes(), " vol_shocks", paths.vol_shocks.sizes(), " names", paths.names);
}
```

Output

```text
asset [20000, 21, 2]  variance [20000, 21, 2]
asset_shocks [20000, 20, 2]  vol_shocks [20000, 20, 2]  names [a, b]
```

### Example 2: independent by default

With no matrices set the assets are independent: the sample correlations between the price shocks of two assets, and between the price and variance shocks of different assets, are near zero, while each asset keeps the correlation of its own model.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Field heston_mc(double spot, double strike) {
  mod::Field f{.instrument = {.type = fin::pricing::Call, .strike = pluss::make_series({strike}), .maturity = pluss::make_series({1.0})}, .market = fin::market::make(spot, 0.02, 0.0),
               .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}), .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  f.controls.mc.paths = 20000;
  f.controls.mc.steps = 20;
  f.controls.mc.seed = 3;
  return f;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto corr = [](const pluss::Series &a, const pluss::Series &b) { return a.mul(b).mean().item<double>(); };
  mod::Chord chord;
  chord.add("a", heston_mc(100.0, 100.0)).add("b", heston_mc(90.0, 90.0));
  const auto p = chord.simulate_paths();
  const auto price_a = p.asset_shocks.select(2, 0), price_b = p.asset_shocks.select(2, 1);
  const auto vol_a = p.vol_shocks.select(2, 0), vol_b = p.vol_shocks.select(2, 1);
  out.writeln("price a with price b", corr(price_a, price_b), " price a with variance b", corr(price_a, vol_b), " variance a with variance b", corr(vol_a, vol_b));
  out.writeln("price a with its own variance", corr(price_a, vol_a), " (the model has rho = -0.7)");
}
```

Output

```text
price a with price b 0.0001  price a with variance b -0.0009  variance a with variance b -0.0002
price a with its own variance -0.7025  (the model has rho = -0.7)
```

### Example 3: correlating the prices

Setting `price_corr` to 0.6 correlates the price shocks at 0.6, and through each asset's own ρ it correlates the price of one asset with the variance of the other at ρ × 0.6 = −0.42.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Field heston_mc(double spot, double strike) {
  mod::Field f{.instrument = {.type = fin::pricing::Call, .strike = pluss::make_series({strike}), .maturity = pluss::make_series({1.0})}, .market = fin::market::make(spot, 0.02, 0.0),
               .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}), .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  f.controls.mc.paths = 20000;
  f.controls.mc.steps = 20;
  f.controls.mc.seed = 3;
  return f;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto corr = [](const pluss::Series &a, const pluss::Series &b) { return a.mul(b).mean().item<double>(); };
  mod::Chord chord;
  chord.add("a", heston_mc(100.0, 100.0)).add("b", heston_mc(90.0, 90.0));
  chord.price_corr = pluss::make_series({1.0, 0.6, 0.6, 1.0}, {2, 2});
  const auto p = chord.simulate_paths();
  out.writeln("price a with price b   ", corr(p.asset_shocks.select(2, 0), p.asset_shocks.select(2, 1)), " (0.6)");
  out.writeln("price a with variance b", corr(p.asset_shocks.select(2, 0), p.vol_shocks.select(2, 1)), " (-0.7 * 0.6 = -0.42)");
}
```

Output

```text
price a with price b    0.6017  (0.6)
price a with variance b -0.4220  (-0.7 * 0.6 = -0.42)
```

### Example 4: correlating the variances

Adding `var_corr` of 0.8 gives the variance shocks exactly that correlation. The price correlation of 0.6 already implies 0.49 × 0.6 = 0.294 between the variance shocks, and the remainders are correlated at (0.8 − 0.294) / 0.51 = 0.992 to make up the rest.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Field heston_mc(double spot, double strike) {
  mod::Field f{.instrument = {.type = fin::pricing::Call, .strike = pluss::make_series({strike}), .maturity = pluss::make_series({1.0})}, .market = fin::market::make(spot, 0.02, 0.0),
               .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}), .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  f.controls.mc.paths = 20000;
  f.controls.mc.steps = 20;
  f.controls.mc.seed = 3;
  return f;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto corr = [](const pluss::Series &a, const pluss::Series &b) { return a.mul(b).mean().item<double>(); };
  mod::Chord chord;
  chord.add("a", heston_mc(100.0, 100.0)).add("b", heston_mc(90.0, 90.0));
  chord.price_corr = pluss::make_series({1.0, 0.6, 0.6, 1.0}, {2, 2});
  chord.var_corr = pluss::make_series({1.0, 0.8, 0.8, 1.0}, {2, 2});
  const auto p = chord.simulate_paths();
  out.writeln("variance a with variance b", corr(p.vol_shocks.select(2, 0), p.vol_shocks.select(2, 1)), " (0.8)");
}
```

Output

```text
variance a with variance b 0.8021  (0.8)
```

### Example 5: setting the cross correlation directly

With `price_var_cross` the cross correlations are set directly: here 0.1 between the price of each asset and the variance of the other, with independent prices and variances otherwise. Each diagonal entry is replaced by the asset's own ρ, so the value given there does not matter.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Field heston_mc(double spot, double strike) {
  mod::Field f{.instrument = {.type = fin::pricing::Call, .strike = pluss::make_series({strike}), .maturity = pluss::make_series({1.0})}, .market = fin::market::make(spot, 0.02, 0.0),
               .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}), .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  f.controls.mc.paths = 20000;
  f.controls.mc.steps = 20;
  f.controls.mc.seed = 3;
  return f;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto corr = [](const pluss::Series &a, const pluss::Series &b) { return a.mul(b).mean().item<double>(); };
  mod::Chord chord;
  chord.add("a", heston_mc(100.0, 100.0)).add("b", heston_mc(90.0, 90.0));
  chord.price_var_cross = pluss::make_series({0.5, 0.1, 0.1, 0.5}, {2, 2});
  const auto p = chord.simulate_paths();
  out.writeln("price a with variance b", corr(p.asset_shocks.select(2, 0), p.vol_shocks.select(2, 1)), " (0.1)");
  out.writeln("price a with its own variance", corr(p.asset_shocks.select(2, 0), p.vol_shocks.select(2, 0)), " (the model's rho = -0.7, not 0.5)");
}
```

Output

```text
price a with variance b 0.0992  (0.1)
price a with its own variance -0.7025  (the model's rho = -0.7, not 0.5)
```

### Example 6: a basket payoff

A call on the average of the two assets at maturity, priced by the discounted mean payoff, rises with the correlation of the assets because a basket of positively correlated assets is more volatile.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Field heston_mc(double spot, double strike) {
  mod::Field f{.instrument = {.type = fin::pricing::Call, .strike = pluss::make_series({strike}), .maturity = pluss::make_series({1.0})}, .market = fin::market::make(spot, 0.02, 0.0),
               .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}), .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  f.controls.mc.paths = 20000;
  f.controls.mc.steps = 20;
  f.controls.mc.seed = 3;
  return f;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto basket_call = [](double correlation) {
    mod::Chord chord;
    chord.add("a", heston_mc(100.0, 100.0)).add("b", heston_mc(100.0, 100.0));
    chord.price_corr = pluss::make_series({1.0, correlation, correlation, 1.0}, {2, 2});
    const auto paths = chord.simulate_paths();
    const auto terminal = paths.asset.select(1, 20).mean(1);   // the average of the two assets at maturity
    return std::exp(-0.02) * terminal.sub(100.0).clamp_min(0.0).mean().item<double>();
  };
  out.writeln("basket call, correlation 0.0", basket_call(0.0), " 0.5", basket_call(0.5), " 0.9", basket_call(0.9));
}
```

Output

```text
basket call, correlation 0.0 6.5024  0.5 7.6140  0.9 8.3962
```

### Example 7: four models in one simulation

Black-Scholes, Heston, SABR and Bates assets, interleaved, with a price correlation and a correlation of the second factors. Each correlation the matrices ask for is read back from the shocks; the Black-Scholes asset has no second factor, so its row of `var_corr` is the identity's; its price is related to the Heston variance through the Heston `rho`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Field mc_field(double spot, mod::Model model) {
  mod::Field f{.instrument = fin::instrument::make(fin::pricing::Call, spot, 1.0), .market = fin::market::make(spot, 0.02, 0.01), .model = model,
               .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  f.controls.mc.paths = 40000;
  f.controls.mc.steps = 20;
  f.controls.mc.seed = 5;
  return f;
}
static double corr(const pluss::Series &a, const pluss::Series &b) { return a.mul(b).mean().item<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  mod::Chord chord{{"heston", mc_field(100.0, fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}))},
                   {"bs", mc_field(80.0, fin::model::BlackScholes{.volatility = pluss::make_series({0.2})})},
                   {"sabr", mc_field(120.0, fin::model::make(fin::model::Kind::Sabr, {0.3, 0.7, 0.5, -0.3}))},
                   {"bates", mc_field(90.0, fin::model::make(fin::model::Kind::Bates, {1.0, 0.05, 0.25, -0.4, 0.05, 0.3, -0.1, 0.15}))}};
  chord.price_corr = pluss::make_series({1.0, 0.5, 0.4, 0.3, 0.5, 1.0, 0.2, 0.1, 0.4, 0.2, 1.0, 0.25, 0.3, 0.1, 0.25, 1.0}, {4, 4}, cfg);
  chord.var_corr = pluss::make_series({1.0, 0.0, 0.3, 0.2, 0.0, 1.0, 0.0, 0.0, 0.3, 0.0, 1.0, 0.1, 0.2, 0.0, 0.1, 1.0}, {4, 4}, cfg);
  const auto p = chord.simulate_paths();
  const auto z = [&](int a) { return p.asset_shocks.select(2, a); };
  const auto w = [&](int a) { return p.vol_shocks.select(2, a); };
  out.writeln("assets", p.names, " asset shape", p.asset.sizes());
  out.writeln("price shocks heston/bs", corr(z(0), z(1)), " (0.5)  sabr/bates", corr(z(2), z(3)), " (0.25)");
  out.writeln("second-factor shocks heston/sabr", corr(w(0), w(2)), " (0.3)  heston/bates", corr(w(0), w(3)), " (0.2)  sabr/bates", corr(w(2), w(3)), " (0.1)");
  out.writeln("black-scholes second factor with heston's", corr(w(1), w(0)), " (none: 0)");
  out.writeln("price of bs with heston's variance shock", corr(z(1), w(0)), " (rho_heston x 0.5 = -0.35)");
  out.writeln("variance of the bs asset is constant:", p.variance.select(2, 1).min().item<double>(), p.variance.select(2, 1).max().item<double>());
  out.writeln("sabr alpha at T, mean", p.variance.select(2, 2).select(1, 20).sqrt().mean().item<double>(), " (alpha0 = 0.3, a martingale)");
}
```

Output

```text
assets [heston, bs, sabr, bates]  asset shape [40000, 21, 4]
price shocks heston/bs 0.499  (0.5)  sabr/bates 0.248  (0.25)
second-factor shocks heston/sabr 0.300  (0.3)  heston/bates 0.200  (0.2)  sabr/bates 0.099  (0.1)
black-scholes second factor with heston's 0.001  (none: 0)
price of bs with heston's variance shock -0.350  (rho_heston x 0.5 = -0.35)
variance of the bs asset is constant: 0.040 0.040
sabr alpha at T, mean 0.299  (alpha0 = 0.3, a martingale)
```

### Example 8: a basket across models

A call on the average of a Black-Scholes asset and a Heston asset, each with its own model, priced from the joint paths: it rises with the correlation, since a basket of assets that move together is as dispersed as one asset.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Field mc_field(double spot, mod::Model model) {
  mod::Field f{.instrument = fin::instrument::make(fin::pricing::Call, spot, 1.0), .market = fin::market::make(spot, 0.02, 0.01), .model = model,
               .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  f.controls.mc.paths = 40000;
  f.controls.mc.steps = 20;
  f.controls.mc.seed = 5;
  return f;
}
static double corr(const pluss::Series &a, const pluss::Series &b) { return a.mul(b).mean().item<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const auto average_call = [&](double r) {
    mod::Chord chord{{"bs", mc_field(100.0, fin::model::BlackScholes{.volatility = pluss::make_series({0.2})})},
                     {"heston", mc_field(100.0, fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}))}};
    chord.price_corr = pluss::make_series({1.0, r, r, 1.0}, {2, 2});
    const auto p = chord.simulate_paths();
    const pluss::Series basket = p.asset.select(1, 20).mean(1);
    return std::exp(-0.02) * basket.sub(100.0).clamp_min(0.0).mean().item<double>();
  };
  out.writeln("call on the average of a Black-Scholes and a Heston asset");
  for (const double r : {-0.5, 0.0, 0.5, 0.9}) out.writeln("price correlation", r, " price", average_call(r));
}
```

Output

```text
call on the average of a Black-Scholes and a Heston asset
price correlation -0.500  price 4.445
price correlation 0.000  price 5.938
price correlation 0.500  price 7.094
price correlation 0.900  price 7.894
```

### Example 9: rough models in the mix

A Heston, a rough Heston, a rough Bergomi and a Black-Scholes asset in one simulation, with a price correlation and a correlation of the second factors. The rough assets are correlated through the same matrices: the shocks read back as asked, and each asset keeps its own law, so every terminal mean is its forward {math}`100\,e^{0.01}=101.005` up to the Monte Carlo error of about 0.1, and the variance of every asset stays near its level of 0.04.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Field mc_field(double spot, mod::Model model) {
  mod::Field f{.instrument = fin::instrument::make(fin::pricing::Call, spot, 1.0), .market = fin::market::make(spot, 0.02, 0.01), .model = model,
               .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  f.controls.mc.paths = 40000;
  f.controls.mc.steps = 20;
  f.controls.mc.seed = 5;
  return f;
}
static double corr(const pluss::Series &a, const pluss::Series &b) { return a.mul(b).mean().item<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  mod::Chord chord{{"heston", mc_field(100.0, fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}))},
                   {"rough_heston", mc_field(100.0, fin::model::make(fin::model::Kind::RoughHeston, {1.5, 0.04, 0.3, -0.1, -0.7, 0.04}))},
                   {"rough_bergomi", mc_field(100.0, fin::model::make(fin::model::Kind::RoughBergomi, {0.04, 1.2, -0.3, -0.7}))},
                   {"bs", mc_field(100.0, fin::model::BlackScholes{.volatility = pluss::make_series({0.2})})}};
  chord.price_corr = pluss::make_series({1.0, 0.5, 0.4, 0.3, 0.5, 1.0, 0.2, 0.1, 0.4, 0.2, 1.0, 0.25, 0.3, 0.1, 0.25, 1.0}, {4, 4}, cfg);
  chord.var_corr = pluss::make_series({1.0, 0.3, 0.2, 0.0, 0.3, 1.0, 0.1, 0.0, 0.2, 0.1, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0}, {4, 4}, cfg);
  const auto p = chord.simulate_paths();
  const auto z = [&](int a) { return p.asset_shocks.select(2, a); };
  const auto w = [&](int a) { return p.vol_shocks.select(2, a); };
  out.writeln("assets", p.names, " asset shape", p.asset.sizes());
  out.writeln("price shocks heston/rough_heston", corr(z(0), z(1)), " (0.5)  rough_heston/rough_bergomi", corr(z(1), z(2)), " (0.2)  heston/bs", corr(z(0), z(3)), " (0.3)");
  out.writeln("second-factor shocks heston/rough_heston", corr(w(0), w(1)), " (0.3)  heston/rough_bergomi", corr(w(0), w(2)), " (0.2)  rough_heston/rough_bergomi", corr(w(1), w(2)), " (0.1)");
  for (int a = 0; a < 4; ++a) out.writeln(p.names[a], "mean at the horizon", p.asset.select(1, 20).select(1, a).mean().item<double>(), " variance at the horizon", p.variance.select(1, 20).select(1, a).mean().item<double>());
}
```

Output

```text
assets [heston, rough_heston, rough_bergomi, bs]  asset shape [40000, 21, 4]
price shocks heston/rough_heston 0.499  (0.5)  rough_heston/rough_bergomi 0.199  (0.2)  heston/bs 0.301  (0.3)
second-factor shocks heston/rough_heston 0.301  (0.3)  heston/rough_bergomi 0.200  (0.2)  rough_heston/rough_bergomi 0.099  (0.1)
heston mean at the horizon 100.858  variance at the horizon 0.040
rough_heston mean at the horizon 101.010  variance at the horizon 0.040
rough_bergomi mean at the horizon 101.077  variance at the horizon 0.040
bs mean at the horizon 101.082  variance at the horizon 0.040
```

### Example 10: the errors

A Black-Scholes asset with a vector of volatilities, assets with different controls, a variance correlation for an asset that has no variance, an indefinite correlation and an empty Chord are each refused with a message that names the problem.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using namespace pluss::tils;

static mod::Field mc_field(double spot, mod::Model model) {
  mod::Field f{.instrument = fin::instrument::make(fin::pricing::Call, spot, 1.0), .market = fin::market::make(spot, 0.02, 0.01), .model = model,
               .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};
  f.controls.mc.paths = 40000;
  f.controls.mc.steps = 20;
  f.controls.mc.seed = 5;
  return f;
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
  mod::Field vector_vol = mc_field(100.0, fin::model::BlackScholes{.volatility = pluss::make_series({0.2, 0.25})});
  mod::Chord several{{"bs", vector_vol}};
  report([&] { (void)several.simulate_paths(); });
  mod::Field other = mc_field(90.0, fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}));
  other.controls.mc.paths = 1000;
  mod::Chord different{{"a", mc_field(100.0, fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}))}, {"b", other}};
  report([&] { (void)different.simulate_paths(); });
  mod::Chord no_variance{{"bs", mc_field(100.0, fin::model::BlackScholes{.volatility = pluss::make_series({0.2})})}, {"h", mc_field(100.0, fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}))}};
  no_variance.var_corr = pluss::make_series({1.0, 0.3, 0.3, 1.0}, {2, 2});
  report([&] { (void)no_variance.simulate_paths(); });
  mod::Chord impossible{{"a", mc_field(100.0, fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}))}, {"b", mc_field(90.0, fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}))}};
  impossible.price_corr = pluss::make_series({1.0, 1.5, 1.5, 1.0}, {2, 2});
  report([&] { (void)impossible.simulate_paths(); });
  report([&] { (void)mod::Chord{}.simulate_paths(); });
}
```

Output

```text
subsp::math::finance::module::Chord::simulate_paths: asset 'bs' needs a single Black-Scholes volatility to be simulated (it has 2)
subsp::math::finance::module::Chord::simulate_paths: asset 'b' must have the same controls.mc (paths, steps, scheme, random numbers, seed) as the first asset
subsp::math::finance::module::Chord::simulate_paths: var_corr[bs, h] must be 0: 'bs' is a Black-Scholes asset and has no variance shock
subsp::math::finance::module::Chord::simulate_paths: price_corr must be positive definite
subsp::math::finance::module::Chord::simulate_paths: the Chord has no assets
```

## See Also

- [`Chord`](../Chord.md): the class and the correlation members
- [`Chord::Paths`](../Paths.md): the result
- [`price`](price.md): prices single-asset options

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="price.html">
    <span class="label">Previous</span>
    <span class="title">price</span>
  </a>
  <a class="next-link" href="size.html">
    <span class="label">Next</span>
    <span class="title">size</span>
  </a>
</div>
<!-- apiref:nav:end -->
