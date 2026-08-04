# PlussMa Finance User Guide

This guide covers the full Finance machinery in PlussMa with task-first workflows.

Primary namespace:
- `pluss::ma::finance` (alias examples below use `fin`)

Major subsurfaces:
- functional pricing and implied volatility
- object/pricer surface (`instrument`, `market`, `model`, `pricer`)
- workflow field/pipeline (`module::Field`)
- stochastic-volatility and rate simulators
- calibration (options + rates)
- filtering (Kalman/EKF/UKF/particle)
- hedging
- volatility bridge from Stats time-series models

## Include And Namespace

### API Signature

- `#include <pluss/ma.h>`
- `namespace fin = pluss::ma::finance`
- `namespace vol = pluss::ma::vol`
- `namespace rate = pluss::ma::rate`
- `namespace cal = pluss::ma::cal`
- `namespace mdl = pluss::ma::mdl`
- `namespace mod = pluss::ma::mod`
- `namespace obj = pluss::ma::obj`
- `namespace prc = pluss::ma::prc`
- `namespace hdg = pluss::ma::hdg`
- `namespace flt = pluss::ma::flt`

### Notes

Use `pluss::ma::finance` when building end-user code. It aliases the full `subsp::math::finance` surface.

### Inputs

- PlussMa headers
- `Series` inputs for vectorized workflows

### Output

- access to all pricing, calibration, filtering, and risk APIs

### Options

Recommended numeric baseline:
- `Float64`
- explicit platform (`pCPU`/other) through `Config`

### Examples

```cpp
#include <pluss/ma.h>

namespace fin = pluss::ma::finance;
namespace vol = pluss::ma::vol;
namespace cal = pluss::ma::cal;
```

### Expected output

- all Finance symbols are available in the translation unit.

## 1) Mental Model: Which Surface To Use

### API Signature

- Functional: `fin::pricing::*`, `fin::volatility::*`, `fin::rates::*`, `fin::calibration::*`
- Object/pricer: `fin::instrument`, `fin::market`, `fin::model`, `fin::pricer`
- Workflow field: `fin::module::Field`
- OOP models: `fin::models::*`

### Notes

Use this split:
- fast direct calculations -> functional APIs
- expressive portfolio-style pricing -> object/pricer APIs
- pricing + residual + loss/objective in one place -> `module::Field`
- stateful model objects with `params()/calibrate_inplace()` -> `models::*`

### Inputs

- instrument fields (type/strike/maturity)
- market fields (spot/rate/dividend)
- model parameters

### Output

- prices, Greeks, paths, calibration metrics, filter states

### Options

- engine choice (`Analytic`, `Fft`, `COS`, `MonteCarlo`)
- risk choice (`PriceOnly`, `All`)

### Examples

```cpp
auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
```

### Expected output

- deterministic and stable baseline for finance workloads.

## 2) Functional Pricing Surface

### API Signature

- `black_scholes_price(...) -> double | Series`
- `black_scholes_greeks(...) -> BlackScholesGreeks | BlackScholesSeriesGreeks`
- `implied_vol(...) -> double | Series`
- `heston_price(...) -> double`
- `heston_price_fft(...) -> double`
- `heston_price_fft_grid(...) -> HestonFftResult`
- `heston_price_cos(...) -> double`
- `heston_price_mc(...) -> double`
- `bates_price_fft/cos/mc(...) -> double`
- `heston_nandi_price(...) -> double`
- `heston_nandi_price_mc(...) -> double`
- `sabr_black_vol(...) -> double`
- `sabr_normal_vol(...) -> double`
- `sabr_price_approx(...) -> double`
- `sabr_price_mc(...) -> double`
- `sv_price_mc(...) -> double`
- `slv_price_mc(...) -> double`
- `rough_heston_price_mc(...) -> double`
- `rough_bergomi_price_mc(...) -> double`

### Notes

This layer is best for compact pricing kernels and direct research loops.

### Inputs

- `OptionType`
- market scalars or `Series`
- model parameter structs (`HestonParams`, `BatesParams`, etc)
- method-specific options (`ImpliedVolOptions`, `HestonFftOptions`, `CosOptions`, `MonteCarloOptions`)

### Output

- scalar prices/vols for single contracts
- vectorized `Series` outputs for batched contracts

### Options

Key option structs:
- `ImpliedVolOptions`: method/bounds/tolerance/iterations
- `HestonIntegrationOptions`: integration steps and mode
- `HestonFftOptions`: FFT grid, damping, quadrature
- `CosOptions`: series terms and truncation
- `MonteCarloOptions`: paths/steps/scheme/rng/variance reduction

### Examples

```cpp
auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

double bs = fin::pricing::black_scholes_price(
    fin::pricing::OptionType::Call,
    100.0, 100.0, 0.03, 0.01, 1.0, 0.2);

double iv = fin::pricing::implied_vol(
    fin::pricing::OptionType::Call,
    bs,
    100.0, 100.0, 0.03, 0.01, 1.0,
    fin::pricing::ImpliedVolOptions{.method = fin::pricing::ImpliedVolMethod::Brent});
```

```cpp
fin::volatility::HestonParams hp{};
auto fft = fin::pricing::HestonFftOptions{.num_fft = 4096, .characteristic_fcn_step = 0.01, .damping_factor = 1.5};

double hfft = fin::pricing::heston_price_fft(
    fin::pricing::OptionType::Call,
    100.0, 100.0, 0.03, 0.01, 1.0,
    hp,
    fft,
    cfg);
```

### Expected output

- `bs` positive call value
- `iv` close to input volatility when using consistent price/model
- `hfft` finite model-consistent option value

## 3) Object + Pricer Surface

### API Signature

- `fin::instrument::make(type, strike, maturity, cfg) -> instrument::Vanilla`
- `fin::market::make(spot, rate, dividend_yield, cfg) -> market::State`
- `fin::model::make(...) -> model::Heston | model::Bates | model::Sabr`
- `fin::pricer::Analytic::price(...) -> fin::pricer::Result`
- `fin::pricer::MonteCarlo::price(...) -> fin::pricer::Result`
- `fin::pricer::Fft::price(...) -> fin::pricer::Result`
- `fin::pricer::COS::price(...) -> fin::pricer::Result`

### Notes

This surface is the most ergonomic for batched option pricing with typed engines and risk controls.

### Inputs

- `instrument::Vanilla` (supports auto type through `is_call` when type is `OptionType::Auto`)
- `market::State`
- typed model object
- typed pricer engine object

### Output

- `fin::pricer::Result` with:
  - `price`
  - `greeks` (`delta`, `gamma`, `vega`, `theta`, `rho`)
  - optional `strike_grid` (FFT grid mode)

### Options

- `fin::pricer::RiskSpec::{PriceOnly, All}`
- engine options:
  - `MonteCarlo`: paths/steps/scheme/rng
  - `Fft`: FFT resolution/damping/grid
  - `COS`: terms/truncation

### Examples

```cpp
auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

auto inst = fin::instrument::make(fin::pricing::OptionType::Call, 100.0, 1.0, cfg);
inst.strike = pluss::series::full({7}, 100.0, cfg);
inst.maturity = pluss::series::full({7}, 1.0, cfg);

auto mkt = fin::market::make(100.0, 0.03, 0.01, cfg);
mkt.spot = pluss::series::linspace(70.0, 130.0, 7, cfg);

fin::model::BlackScholes bs_mdl{.volatility = pluss::series::full({7}, 0.22, cfg)};
fin::pricer::Analytic an;

auto out = an.price(inst, bs_mdl, mkt, fin::pricer::RiskSpec::All, cfg);
```

### Expected output

- `out.price` shape aligned to batch size
- Greek surfaces defined when risk is `All` and engine supports them

## 4) Workflow Field (`module::Field`) For End-To-End Pricing + Objective

### API Signature

- `module::Field::price(cfg) -> fin::pricer::Result`
- `module::Field::residuals(target_prices, cfg) -> Series`
- `module::Field::loss(target_prices, cfg) -> Series`
- `module::Field::loss_value(target_prices, cfg) -> double`
- `module::Field::objective(target_prices, cfg) -> double`
- `module::Field::bind_objective(target_prices, cfg) -> function<double(const module::Model&)>`
- `module::Field::simulate/simulate_paths/clear_paths(...)`

### Notes

`Field` is the cleanest surface for calibration-style workflows because it bundles instrument, market, model, engine/risk/loss controls, and path cache.

### Inputs

- `module::Field` with populated `.instrument`, `.market`, `.model`, `.controls`
- optional target market prices for loss/objective

### Output

- direct pricing result
- residual/loss series
- scalar objective for optimizers

### Options

`module::Controls`:
- `engine`: `Fft | Cos | MonteCarlo | Analytic`
- `risk`: `PriceOnly | All`
- `loss`: `MSE | RelativeMSE | MAE | Huber`
- `huber_delta` for robust loss

### Examples

```cpp
namespace mod = fin::module;

auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

mod::Field pipe{
  .instrument = fin::instrument::Vanilla{
    .type = fin::pricing::OptionType::Call,
    .strike = pluss::series::linspace(80.0, 120.0, 9, cfg),
    .maturity = pluss::series::full({9}, 1.0, cfg),
  },
  .market = fin::market::State{
    .spot = pluss::series::full({9}, 100.0, cfg),
    .rate = pluss::series::full({9}, 0.02, cfg),
    .dividend_yield = pluss::series::full({9}, 0.0, cfg),
  },
  .model = fin::model::make(1.5, 0.04, 0.30, -0.70, 0.04, cfg),
  .controls = mod::Controls{
    .engine = mod::Engine::Cos,
    .risk = mod::Risk::All,
    .loss = mod::Loss::Huber,
    .huber_delta = 0.05,
  },
};

auto priced = pipe.price(cfg);
auto market_px = priced.price;
double self_loss = pipe.loss_value(market_px, cfg);
```

### Expected output

- `priced.price` and Greeks for configured engine/risk
- `self_loss` near zero when target equals model price

## 5) Stochastic-Volatility Path Generation

### API Signature

- `vol::gbm_paths(...) -> vol::StochVolPaths`
- `vol::heston_paths(...) -> vol::StochVolPaths`
- `vol::bates_paths(...) -> vol::StochVolPaths`
- `vol::sv_paths(...) -> vol::StochVolPaths`
- `vol::heston_nandi_paths(...) -> vol::StochVolPaths`
- `vol::sabr_paths(...) -> vol::StochVolPaths`
- `vol::slv_paths(...) -> vol::StochVolPaths`
- `vol::rough_heston_paths(...) -> vol::StochVolPaths`
- `vol::rough_bergomi_paths(...) -> vol::StochVolPaths`

### Notes

Use this surface for simulation-driven research, stress testing, and Monte Carlo diagnostics.

### Inputs

- `steps`, `paths`, `spot0`, `dt`
- model params struct
- optional `vol::SimOptions`

### Output

- `StochVolPaths` with `spot`, `variance`, and innovations

### Options

`vol::SimOptions`:
- scheme: `Euler`, `Milstein`, `Milstein2`, `QuadExp`, `Transition`
- random method: `Pseudo`, `Sobol`, `Halton`, `Torus`
- antithetic and quasi-random controls

### Examples

```cpp
vol::HestonParams hp{};
vol::SimOptions so{};
so.scheme = vol::SimScheme::QuadExp;
so.method = vol::RandMethod::Sobol;
so.randomized_quasi = true;
so.sobol_scrambling = 3;
so.seed = 20260642ULL;

auto paths = vol::heston_paths(64, 1024, 100.0, 1.0 / 252.0, hp, so, cfg, so.seed);
```

### Expected output

- `paths.spot` and `paths.variance` are finite path cubes/matrices with configured dimensions.

## 6) Rate Models (Functional)

### API Signature

- `rate::vasicek_paths/cir_paths/hull_white_paths/hjm_paths(...)`
- `rate::vasicek_zcb/cir_zcb/hull_white_zcb(...) -> double`
- `rate::hjm_zcb_mc(...) -> double`

### Notes

Rates APIs cover short-rate simulations and bond pricing under multiple classic models.

### Inputs

- `steps`, `paths`, `dt`
- params struct (`VasicekParams`, `CirParams`, `HullWhiteParams`, `HjmParams`)
- optional `rate::SimOptions`

### Output

- path structs (`RatePaths`, `HjmPaths`)
- ZCB prices

### Options

`rate::SimOptions`:
- discretization scheme (`Euler`, `Milstein`, `Transition`)
- pseudo/quasi random controls

### Examples

```cpp
rate::VasicekParams vp{.kappa = 0.75, .theta = 0.03, .sigma = 0.01, .r0 = 0.028};
rate::SimOptions ro{};
ro.scheme = rate::Scheme::Transition;
ro.method = vol::RandMethod::Sobol;
ro.randomized_quasi = true;
ro.seed = 20260651ULL;

auto vp_paths = rate::vasicek_paths(60, 6, 1.0 / 252.0, vp, ro, cfg, ro.seed);
double zcb2y = rate::vasicek_zcb(2.0, vp.r0, vp);
```

### Expected output

- valid rate paths and finite ZCB price.

## 7) Calibration Surface

### API Signature

- Option-side:
  - `cal::heston_sse`, `cal::calibrate_heston`
  - `cal::bates_sse`, `cal::calibrate_bates`
  - `cal::sv_sse`, `cal::calibrate_sv`
  - `cal::heston_nandi_returns_nll`, `cal::heston_nandi_sse`, `cal::calibrate_heston_nandi`
- Rate-side:
  - `cal::vasicek_sse`, `cal::calibrate_vasicek`
  - `cal::cir_sse`, `cal::calibrate_cir`
  - `cal::hull_white_sse`, `cal::calibrate_hull_white`
  - `cal::hjm_sse`, `cal::calibrate_hjm`

### Notes

Calibration APIs are direct and fail-loud: you pass quotes, a starting point, and typed option bounds/search controls.

### Inputs

- `vector<cal::OptionQuote>` or `vector<cal::RateQuote>`
- spot/rate/dividend where needed
- initial params + options

### Output

- typed calibration result with:
  - fitted params
  - objective/rmse
  - evaluations
  - convergence flag

### Options

Each model has dedicated bounds + search controls (`random_starts`, `max_iterations`, `step_fraction`, `tolerance`, `seed`).

### Examples

```cpp
std::vector<cal::OptionQuote> quotes = {
  {.type = fin::pricing::OptionType::Call, .strike = 90.0, .maturity = 0.5, .market_price = 13.2, .weight = 1.0},
  {.type = fin::pricing::OptionType::Call, .strike = 100.0, .maturity = 1.0, .market_price = 8.5, .weight = 1.0},
  {.type = fin::pricing::OptionType::Call, .strike = 110.0, .maturity = 1.0, .market_price = 5.1, .weight = 1.0},
};

fin::volatility::HestonParams init{};
cal::HestonCalibOptions copt{};
copt.random_starts = 8;
copt.max_iterations = 160;

auto fit = cal::calibrate_heston(quotes, 100.0, 0.02, 0.0, init, copt, cfg);
```

### Expected output

- `fit.converged` indicates solve status.
- `fit.objective` and `fit.rmse` quantify market fit quality.

## 8) OOP Model Classes (`fin::mdl`)

### API Signature

- Stoch-vol classes: `mdl::Heston`, `mdl::Bates`, `mdl::Sv`, `mdl::HestonNandi`, `mdl::Sabr`, `mdl::Slv`, `mdl::RoughHeston`, `mdl::RoughBergomi`
  - `params()`, `set_params(...)`, `paths(...)`, `price_mc(...)`
  - model-specific: `price(...)`, `sse(...)`, `calibrate(...)`, `calibrate_inplace(...)`
- Rate classes: `mdl::VasicekRate`, `mdl::CirRate`, `mdl::HullWhiteRate`, `mdl::HjmRate`
  - `paths(...)`, `zcb(...)` / `zcb_mc(...)`, `calibrate(...)`, `calibrate_inplace(...)`

### Notes

Use these classes for stateful workflows where parameter ownership and in-place recalibration matter.

### Inputs

- params at construction
- quotes and calibration options for fitting

### Output

- model-local parameter updates and calibration reports

### Options

- same calibration options as functional layer

### Examples

```cpp
mdl::VasicekRate model{rate::VasicekParams{.kappa = 0.7, .theta = 0.03, .sigma = 0.01, .r0 = 0.03}};

auto p = model.paths(60, 8, 1.0 / 252.0, rate::SimOptions{}, cfg, 42);
double z = model.zcb(2.0, model.params().r0);
```

### Expected output

- class-managed paths and bond values computed from current parameter state.

## 9) Filtering Surface (`fin::flt`)

### API Signature

- `flt::kalman_filter(observations, KalmanModel, cfg) -> KalmanResult`
- `flt::ekf_filter(observations, EkfModel, cfg) -> KalmanResult`
- `flt::ukf_filter(observations, UkfModel, cfg) -> KalmanResult`
- `flt::particle_filter(observations, ParticleFilterModel, cfg) -> ParticleFilterResult`
- `flt::sv_particle_filter(log_returns, SvParams, num_particles, dt, ess_threshold, cfg, seed) -> ParticleFilterResult`

### Notes

All filter outputs expose path-wise filtered statistics and total log-likelihood for model-comparison loops.
Use this order in practice:

1. `kalman_filter` for linear-Gaussian systems.
2. `ekf_filter` when dynamics/observation are nonlinear and Jacobians are stable.
3. `ukf_filter` when you want derivative-free nonlinear filtering.
4. `particle_filter` for non-Gaussian/multimodal state posteriors.
5. `sv_particle_filter` for stochastic-volatility return filtering.

### Inputs

- observed series
- state/obs transition callbacks (EKF/UKF/PF)
- covariance and initialization settings

### Output

- Kalman-like results with filtered/predicted states and innovations
- particle results with weights, ESS, and optional particle tensor

### Options

Particle options:
- resampling method (`Systematic`, `Multinomial`, `Stratified`, `Residual`)
- trigger (`Ratio`, `Interval`)
- ESS threshold, interval, and seed

UKF options:
- `alpha`, `beta`, `kappa`, `jitter`

Practical defaults:
- UKF: `alpha=1e-3`, `beta=2`, `kappa=0`, positive `jitter`
- PF: `ess_threshold` in `0.4-0.7`, systematic/stratified resampling for stable variance

### Examples

KF (linear-Gaussian):

```cpp
flt::KalmanModel km;
km.transition = pluss::series::full({1,1}, 1.0, cfg);
km.observation = pluss::series::full({1,1}, 1.0, cfg);
km.process_cov = pluss::series::full({1,1}, 0.01, cfg);
km.observation_cov = pluss::series::full({1,1}, 0.04, cfg);
km.initial_mean = pluss::series::full({1}, 0.0, cfg);
km.initial_cov = pluss::series::full({1,1}, 1.0, cfg);

auto y = pluss::series::full({40}, 1.2, cfg);
auto kf = flt::kalman_filter(y, km, cfg);
```

EKF (nonlinear transition/observation):

```cpp
flt::EkfModel ekm;
ekm.transition = [](const pluss::Series& x, std::int64_t) { return x.mul(0.96).add(0.03); };
ekm.observation = [](const pluss::Series& x, std::int64_t) { return x.square(); };
ekm.transition_jacobian = [](const pluss::Series& x, std::int64_t) {
  return pluss::series::full({1, 1}, 0.96, x.options());
};
ekm.observation_jacobian = [](const pluss::Series& x, std::int64_t) {
  return x.mul(2.0).reshape({1, 1});
};
ekm.process_cov = pluss::series::full({1,1}, 0.01, cfg);
ekm.observation_cov = pluss::series::full({1,1}, 0.05, cfg);
ekm.initial_mean = pluss::series::full({1}, 0.8, cfg);
ekm.initial_cov = pluss::series::full({1,1}, 0.3, cfg);

auto ekf = flt::ekf_filter(y, ekm, cfg);
```

UKF (nonlinear derivative-free):

```cpp
flt::UkfModel ukm;
ukm.transition = [](const pluss::Series& x, std::int64_t) { return x.mul(0.95); };
ukm.observation = [](const pluss::Series& x, std::int64_t) { return x.square(); };
ukm.process_cov = pluss::series::full({1,1}, 0.01, cfg);
ukm.observation_cov = pluss::series::full({1,1}, 0.05, cfg);
ukm.initial_mean = pluss::series::full({1}, 1.0, cfg);
ukm.initial_cov = pluss::series::full({1,1}, 0.5, cfg);
ukm.options = flt::UkfOptions{.alpha = 1e-3, .beta = 2.0, .kappa = 0.0, .jitter = 1e-9};

auto ukf = flt::ukf_filter(y, ukm, cfg);
```

Particle filter (generic state-space):

```cpp
flt::ParticleFilterModel pm;
pm.initial_particles = pluss::series::randn({1500, 1}, cfg, 20260711ULL);
pm.options.resample = true;
pm.options.ess_threshold = 0.6;
pm.options.resample_method = flt::ParticleResampleMethod::Stratified;
pm.options.resample_trigger = flt::ParticleResampleTrigger::Ratio;
pm.options.resample_seed = 20260712ULL;

pm.transition = [=](const pluss::Series& particles, std::int64_t t) {
  return particles.mul(0.98).add(
      pluss::series::randn({particles.sizes()[0], 1}, cfg, 5000 + static_cast<std::uint64_t>(t)).mul(0.08));
};
pm.loglik = [](const pluss::Series& particles, const pluss::Series& obs, std::int64_t) {
  const auto resid = obs.reshape({1,1}).sub(particles);
  return resid.square().mul(-0.5).reshape({particles.sizes()[0]});
};

auto pf = flt::particle_filter(y, pm, cfg);
```

SV particle filter (specialized convenience):

```cpp
auto r = pluss::series::randn({120}, cfg, 20260801ULL).mul(0.01);
auto sv = flt::sv_particle_filter(
    r,
    fin::volatility::SvParams{},
    3000,   // num_particles
    1.0/252.0,
    0.55,   // ess_threshold
    cfg,
    20260802ULL);
```

### Expected output

- `kf/ekf/ukf.filtered_mean` and `*.loglik` finite under valid model settings.
- `pf.filtered_mean`, `pf.ess`, and weight tensors defined for each observation step.
- `sv` provides PF-style diagnostics for SV state tracking.

## 10) Hedging Surface

### API Signature

- `hdg::delta_hedge(spot_path, type, strike, rate, dividend_yield, maturity, volatility, cfg) -> hdg::DeltaHedgeResult`

### Notes

Computes Black-Scholes delta-hedging trajectory on a realized spot path.

### Inputs

- path of spots
- option contract details
- volatility/rate/dividend assumptions

### Output

- `DeltaHedgeResult` with option value, hedge delta, cash, portfolio, hedge error, and terminal PnL

### Options

- use dense path sampling to reduce rebalance discretization error

### Examples

```cpp
auto spot_path = pluss::series::linspace(95.0, 110.0, 64, cfg);
auto hedged = fin::hdg::delta_hedge(
    spot_path,
    fin::pricing::OptionType::Call,
    100.0,
    0.02,
    0.0,
    0.5,
    0.25,
    cfg);
```

### Expected output

- finite `hedged.terminal_pnl` and path-wise hedge-account series.

## 11) Vol Bridge (Stats Time-Series -> Pricing-Oriented Volatility)

### API Signature

- `fin::vol::to_pricing_params(ts::GarchFit|ts::Garch11Fit, r, lambda) -> fin::vol::Ngarch11Params`
- `fin::cal::returns_metrics(...) -> ReturnsMetrics`
- `fin::cal::variance_forecast(...) -> Series`
- `fin::prc::pricing_metrics(...) -> OptionVolMetrics`
- `fin::prc::pricing_variance_forecast(...) -> Series`
- `fin::cal::weighted_joint_loglik / weighted_joint_nll(...) -> double`

### Notes

Bridge functions let you blend return-dynamics evidence with option-side pricing evidence in one coherent objective.

### Inputs

- fitted TS volatility models (`GarchFit`, `EgarchFit`, `GjrFit`)
- returns series
- optional weighting scheme

### Output

- standardized metrics, variance forecasts, and weighted joint criteria

### Options

- `JointLikelihoodWeights` controls return-vs-option emphasis

### Examples

```cpp
fin::cal::JointLikelihoodWeights w{.returns_weight = 1.0, .options_weight = 2.0};
double joint = fin::cal::weighted_joint_nll(-1250.0, -430.0, w);
```

### Expected output

- scalar joint objective suitable for optimizer integration.

## 12) Reproducibility And Numerical Reliability

### API Signature

- deterministic seeds via option structs (`seed`, `offset`, `resample_seed`)

### Notes

Finance pipelines mix simulation and optimization. Explicit seeds and strict tolerances are required for reproducible research.

### Inputs

- deterministic seed values in pricing/sim/calibration/filter options

### Output

- repeatable path/pricing/calibration/filter outcomes across runs (same platform/config)

### Options

- prefer:
  - `Float64`
  - explicit `max_iterations`, `tolerance`
  - explicit bounds in calibrations

### Examples

```cpp
fin::pricing::MonteCarloOptions mc;
mc.paths = 40000;
mc.steps = 252;
mc.seed = 42;
mc.antithetic = true;
```

### Expected output

- stable Monte Carlo estimates with predictable variance profile across repeated runs.

## 13) Failure-Mode Guidance

### API Signature

- common runtime contract checks across pricing/calibration/filtering surfaces

### Notes

These checks are expected and useful when assembling large workflows.

### Inputs

Typical problematic inputs:
- incompatible series shapes
- invalid option bounds (`min > max`, non-positive tolerances)
- invalid domain values (negative variance-like parameters, invalid binning)

### Output

- fail-loud status/exception or warning-like result behavior depending on API

### Options

- validate contract dimensions early using small pilot runs
- build deterministic smoke cases before full calibration sweeps

### Examples

```cpp
// Practical pattern: run a tiny deterministic sanity pass before large jobs.
auto quick_mc = fin::pricing::MonteCarloOptions{.paths = 2000, .steps = 32, .seed = 7};
```

### Expected output

- early detection of data/shape/options issues before costly full-scale runs.
