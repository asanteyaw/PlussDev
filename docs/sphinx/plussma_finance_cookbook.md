# PlussMa Finance Cookbook

This cookbook is recipe-driven and designed for direct reuse in quant workflows.

## Include And Baseline

### API Signature

- `#include <pluss/ma.h>`
- `namespace fin = pluss::ma::finance`

### Notes

All recipes assume explicit `Float64` config for robust numerical behavior.

### Inputs

- PlussMa headers

### Output

- reusable snippets for pricing, calibration, simulation, filtering, and hedging

### Options

```cpp
auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
```

### Examples

```cpp
#include <pluss/ma.h>
namespace fin = pluss::ma::finance;
```

### Expected output

- code compiles with all finance cookbook recipes available.

## Recipe 1: Scalar Black-Scholes Price + Greeks + Implied Vol

### API Signature

- `black_scholes_price(...) -> double`
- `black_scholes_greeks(...) -> BlackScholesGreeks`
- `implied_vol(...) -> double`

### Notes

Use for single-contract sanity checks and solver parity tests.

### Inputs

- scalar contract and market parameters

### Output

- price, Greeks, and recovered implied vol

### Options

- `ImpliedVolOptions` to choose root-finder and bounds

### Examples

```cpp
double px = fin::pricing::black_scholes_price(
    fin::pricing::OptionType::Call,
    100.0, 100.0, 0.03, 0.01, 1.0, 0.20);

auto g = fin::pricing::black_scholes_greeks(
    fin::pricing::OptionType::Call,
    100.0, 100.0, 0.03, 0.01, 1.0, 0.20);

double iv = fin::pricing::implied_vol(
    fin::pricing::OptionType::Call,
    px,
    100.0, 100.0, 0.03, 0.01, 1.0,
    fin::pricing::ImpliedVolOptions{.method = fin::pricing::ImpliedVolMethod::Brent});
```

### Expected output

- finite `px`
- finite Greeks (`delta/gamma/vega/theta/rho`)
- `iv` close to `0.20`

## Recipe 2: Vectorized Black-Scholes Surface

### API Signature

- `black_scholes_price(type, spot: Series, strike, rate, dividend_yield, maturity, volatility, cfg) -> Series`

### Notes

Useful for smile or spot-sensitivity sweeps.

### Inputs

- vectorized `spot`

### Output

- vectorized price curve

### Options

- use `linspace` for regular grids

### Examples

```cpp
auto spot = pluss::linspace(70.0, 130.0, 13, cfg);
auto curve = fin::pricing::black_scholes_price(
    fin::pricing::OptionType::Call,
    spot,
    100.0,
    0.03,
    0.01,
    1.0,
    0.22,
    cfg);
```

### Expected output

- monotonic-in-spot call prices and shape `[13]`.

## Recipe 3: Heston FFT Price + Grid

### API Signature

- `heston_price_fft(...) -> double`
- `heston_price_fft_grid(...) -> HestonFftResult`

### Notes

Use FFT point price for targeted contracts; grid output for strike-surface diagnostics.

### Inputs

- `HestonParams` + `HestonFftOptions`

### Output

- scalar price and optional strike-grid pairs

### Options

- `num_fft`, `characteristic_fcn_step`, `damping_factor`, `quadrature`

### Examples

```cpp
fin::volatility::HestonParams hp{};
fin::pricing::HestonFftOptions fft{
  .num_fft = 4096,
  .characteristic_fcn_step = 0.01,
  .damping_factor = 1.5,
};

double p = fin::pricing::heston_price_fft(
    fin::pricing::OptionType::Call,
    100.0, 100.0, 0.03, 0.01, 1.0, hp, fft, cfg);

auto grid = fin::pricing::heston_price_fft_grid(
    fin::pricing::OptionType::Call,
    100.0, 0.03, 0.01, 1.0, hp, fft, cfg);
```

### Expected output

- finite `p`
- `grid.price` and `grid.strike` with matching lengths.

## Recipe 4: COS Pricing For Heston/Bates

### API Signature

- `heston_price_cos(...) -> double`
- `bates_price_cos(...) -> double`

### Notes

COS is often a strong deterministic baseline for characteristic-function models.

### Inputs

- params + `CosOptions`

### Output

- deterministic model prices

### Options

- `num_terms`, `truncation`, `cumulant_step`

### Examples

```cpp
fin::pricing::CosOptions cos{.num_terms = 1024, .truncation = 12.0, .cumulant_step = 1e-3};
fin::volatility::HestonParams hp{};
fin::volatility::BatesParams bp{};

double ph = fin::pricing::heston_price_cos(fin::pricing::OptionType::Call, 100.0, 100.0, 0.03, 0.01, 1.0, hp, cos, cfg);
double pb = fin::pricing::bates_price_cos(fin::pricing::OptionType::Call, 100.0, 100.0, 0.03, 0.01, 1.0, bp, cos, cfg);
```

### Expected output

- finite `ph` and `pb` values.

## Recipe 5: Monte Carlo Pricing Across Models

### API Signature

- `heston_price_mc`, `bates_price_mc`, `sv_price_mc`, `heston_nandi_price_mc`, `sabr_price_mc`, `slv_price_mc`, `rough_heston_price_mc`, `rough_bergomi_price_mc`

### Notes

Single MC option struct can be reused across model families.

### Inputs

- model parameter struct + shared `MonteCarloOptions`

### Output

- model-specific MC price estimates

### Options

- `paths`, `steps`, `scheme`, `rand_method`, antithetic/control-variate knobs

### Examples

```cpp
fin::pricing::MonteCarloOptions mc;
mc.paths = 30000;
mc.steps = 252;
mc.scheme = fin::volatility::SimScheme::Euler;
mc.rand_method = fin::volatility::RandMethod::Sobol;
mc.randomized_quasi = true;
mc.seed = 42;

fin::volatility::HestonParams hp{};
double p = fin::pricing::heston_price_mc(
    fin::pricing::OptionType::Call,
    100.0, 100.0, 0.03, 0.01, 1.0,
    hp,
    mc,
    cfg);
```

### Expected output

- finite MC estimate with reproducible value for fixed seed/options.

## Recipe 6: Object Surface Analytic/MC/FFT/COS

### API Signature

- `fin::pricer::Analytic::price(...)`
- `fin::pricer::MonteCarlo::price(...)`
- `fin::pricer::Fft::price(...)`
- `fin::pricer::COS::price(...)`

### Notes

Use object/pricer when you want typed engine objects and consistent return containers.

### Inputs

- `instrument::Vanilla`, `market::State`, typed `model::*`

### Output

- `fin::pricer::Result` containing `price`, `greeks`, optional `strike_grid`

### Options

- choose risk with `fin::pricer::RiskSpec`

### Examples

```cpp
auto inst = fin::instrument::make(fin::pricing::OptionType::Call, 100.0, 1.0, cfg);
inst.strike = pluss::full({6}, 100.0, cfg);
inst.maturity = pluss::full({6}, 1.0, cfg);

auto mkt = fin::market::make(100.0, 0.03, 0.01, cfg);
mkt.spot = pluss::full({6}, 100.0, cfg);

auto hest = fin::model::make(1.5, 0.04, 0.30, -0.70, 0.04, cfg);

fin::pricer::Fft fft{.num_fft = 4096, .du = 0.01, .alpha = 1.5};
auto out_fft = fft.price(inst, hest, mkt, fin::pricer::RiskSpec::PriceOnly, cfg);
```

### Expected output

- `out_fft.price` defined and batched to instrument size.

## Recipe 7: Module Field Workflow (Pricing + Objective)

### API Signature

- `mod::Field::price/residuals/loss/loss_value/objective`
- `mod::Field::simulate/simulate_paths`

### Notes

`module::Field` is the cleanest calibration-ready entrypoint.

### Inputs

- populated field structure
- target market prices for objective

### Output

- price/risk outputs + scalar objective

### Options

- controls include engine/risk/loss family

### Examples

```cpp
namespace mod = fin::module;

mod::Field pipe{
  .instrument = fin::instrument::Vanilla{
    .type = fin::pricing::OptionType::Call,
    .strike = pluss::linspace(80.0, 120.0, 9, cfg),
    .maturity = pluss::full({9}, 1.0, cfg),
  },
  .market = fin::market::State{
    .spot = pluss::full({9}, 100.0, cfg),
    .rate = pluss::full({9}, 0.02, cfg),
    .dividend_yield = pluss::full({9}, 0.0, cfg),
  },
  .model = fin::model::make(1.5, 0.04, 0.30, -0.70, 0.04, cfg),
  .controls = mod::Controls{.engine = mod::Engine::Cos, .risk = mod::Risk::PriceOnly, .loss = mod::Loss::MSE},
};

auto out = pipe.price(cfg);
double obj = pipe.objective(out.price, cfg);
```

### Expected output

- `obj` near zero when target is self-generated model price.

## Recipe 8: Heston Calibration (Functional)

### API Signature

- `cal::calibrate_heston(quotes, spot, rate, dividend_yield, initial, options, cfg) -> HestonCalibResult`

### Notes

Use this when you want direct quote-to-parameter calibration without object wrappers.

### Inputs

- option quote vector + initial params + calibration options

### Output

- calibrated params and diagnostics

### Options

- tune `random_starts`, `max_iterations`, `tolerance`, bounds, and method

### Examples

```cpp
std::vector<fin::calibration::OptionQuote> quotes = {
  {.type = fin::pricing::OptionType::Call, .strike = 90.0, .maturity = 0.5, .market_price = 13.0, .weight = 1.0},
  {.type = fin::pricing::OptionType::Call, .strike = 100.0, .maturity = 1.0, .market_price = 8.0, .weight = 1.0},
  {.type = fin::pricing::OptionType::Call, .strike = 110.0, .maturity = 1.0, .market_price = 5.0, .weight = 1.0},
};

fin::volatility::HestonParams init{};
fin::calibration::HestonCalibOptions opt{};
opt.random_starts = 10;
opt.max_iterations = 160;
opt.seed = 20260652ULL;

auto fit = fin::calibration::calibrate_heston(quotes, 100.0, 0.02, 0.0, init, opt, cfg);
```

### Expected output

- `fit.evaluations > 0`
- `fit.objective` finite
- `fit.params` updated from initial guess when calibration improves objective.

## Recipe 9: Bates/SV/Heston-Nandi Calibration

### API Signature

- `calibrate_bates`, `calibrate_sv`, `calibrate_heston_nandi`

### Notes

Use model-specific calibration APIs to preserve correct parameter constraints.

### Inputs

- model-specific quote/returns inputs

### Output

- model-specific calibration result structs

### Options

- each has model-specific bounds and search settings

### Examples

```cpp
fin::volatility::BatesParams b0{};
fin::calibration::BatesCalibOptions bopt{};
auto bfit = fin::calibration::calibrate_bates(quotes, 100.0, 0.02, 0.0, b0, bopt, cfg);
```

```cpp
auto returns = pluss::randn({252}, cfg, 20260701ULL).mul(0.02);
fin::volatility::HestonNandiParams hn0{};
fin::calibration::HestonNandiCalibOptions hnopt{};
auto hnfit = fin::calibration::calibrate_heston_nandi(returns, quotes, 100.0, 0.02, 0.0, hn0, hnopt, cfg);
```

### Expected output

- finite objective diagnostics and converged flags indicating search outcome.

## Recipe 10: OOP Model Calibrate In Place

### API Signature

- `mdl::<Model>::calibrate_inplace(...) -> <Model>CalibResult`

### Notes

Prefer this when you want class-owned params to mutate directly.

### Inputs

- model object + quote set

### Output

- calibration result and updated internal params

### Options

- same model-specific calib options

### Examples

```cpp
fin::models::Heston heston{fin::volatility::HestonParams{}};
fin::calibration::HestonCalibOptions opt{};
auto fit = heston.calibrate_inplace(quotes, 100.0, 0.02, 0.0, opt, cfg);
auto p = heston.params();
```

### Expected output

- `p` reflects calibrated values when optimization improves objective.

## Recipe 11: Volatility Path Scheme/RNG Sweep

### API Signature

- `vol::heston_paths(..., SimOptions, ...)`

### Notes

Use this to compare discretization and RNG families under common seeds.

### Inputs

- same base params with varying `SimOptions`

### Output

- comparative path tensors for accuracy/performance diagnostics

### Options

- vary `scheme` and `method` while holding steps/paths constant

### Examples

```cpp
fin::volatility::HestonParams hp{};
fin::volatility::SimOptions so{};
so.seed = 20260642ULL;

so.scheme = fin::volatility::SimScheme::Euler;
so.method = fin::volatility::RandMethod::Pseudo;
auto p_euler = fin::volatility::heston_paths(64, 4, 100.0, 1.0 / 252.0, hp, so, cfg, so.seed);

so.scheme = fin::volatility::SimScheme::QuadExp;
so.method = fin::volatility::RandMethod::Sobol;
so.randomized_quasi = true;
auto p_qe = fin::volatility::heston_paths(64, 4, 100.0, 1.0 / 252.0, hp, so, cfg, so.seed);
```

### Expected output

- both path sets defined; distributional behavior differs by scheme/method.

## Recipe 12: Rate Paths + ZCB + Calibration

### API Signature

- `rate::*_paths`, `rate::*_zcb`
- `calibrate_vasicek/cir/hull_white/hjm`

### Notes

Rates workflow mirrors equity-vol workflow: simulate, price curve points, calibrate back.

### Inputs

- rate params and curve quotes

### Output

- path simulation, bond prices, calibrated params

### Options

- transition scheme is usually preferred for mean-reverting short-rate models when available

### Examples

```cpp
fin::rates::VasicekParams vp{.kappa = 0.75, .theta = 0.03, .sigma = 0.01, .r0 = 0.028};
auto z0 = fin::rates::vasicek_zcb(2.0, vp.r0, vp);

auto vfit = fin::calibration::calibrate_vasicek(
    std::vector<fin::calibration::RateQuote>{{.maturity = 2.0, .zcb_price = z0, .weight = 1.0}},
    vp,
    fin::calibration::VasicekCalibOptions{});
```

### Expected output

- `vfit.objective` finite and `vfit.params` near curve-consistent region.

## Recipe 13: Kalman / EKF / UKF

### API Signature

- `flt::kalman_filter`
- `flt::ekf_filter`
- `flt::ukf_filter`

### Notes

Use linear model first (KF), then nonlinear model (EKF/UKF) when needed.

### Inputs

- observations + model definitions

### Output

- filtered states and log-likelihood

### Options

- `UkfOptions` (`alpha`, `beta`, `kappa`, `jitter`)

### Examples

13A) Linear KF baseline

```cpp
auto y = pluss::full({40}, 1.0, cfg);

fin::filtering::KalmanModel km;
km.transition = pluss::full({1,1}, 1.0, cfg);
km.observation = pluss::full({1,1}, 1.0, cfg);
km.process_cov = pluss::full({1,1}, 0.01, cfg);
km.observation_cov = pluss::full({1,1}, 0.05, cfg);
km.initial_mean = pluss::full({1}, 0.0, cfg);
km.initial_cov = pluss::full({1,1}, 1.0, cfg);

auto kf = fin::filtering::kalman_filter(y, km, cfg);
```

13B) EKF with explicit Jacobians

```cpp
fin::filtering::EkfModel ekm;
ekm.transition = [](const pluss::Series& x, std::int64_t) { return x.mul(0.96).add(0.02); };
ekm.observation = [](const pluss::Series& x, std::int64_t) { return x.square(); };
ekm.transition_jacobian = [](const pluss::Series& x, std::int64_t) {
  return pluss::full({1,1}, 0.96, x.options());
};
ekm.observation_jacobian = [](const pluss::Series& x, std::int64_t) {
  return x.mul(2.0).reshape({1,1});
};
ekm.process_cov = pluss::full({1,1}, 0.01, cfg);
ekm.observation_cov = pluss::full({1,1}, 0.05, cfg);
ekm.initial_mean = pluss::full({1}, 0.8, cfg);
ekm.initial_cov = pluss::full({1,1}, 0.4, cfg);

auto ekf = fin::filtering::ekf_filter(y, ekm, cfg);
```

13C) UKF derivative-free nonlinear filter

```cpp
fin::filtering::UkfModel ukm;
ukm.transition = [](const pluss::Series& x, std::int64_t) { return x.mul(0.95); };
ukm.observation = [](const pluss::Series& x, std::int64_t) { return x.square(); };
ukm.process_cov = pluss::full({1,1}, 0.01, cfg);
ukm.observation_cov = pluss::full({1,1}, 0.05, cfg);
ukm.initial_mean = pluss::full({1}, 1.0, cfg);
ukm.initial_cov = pluss::full({1,1}, 0.5, cfg);
ukm.options = fin::filtering::UkfOptions{.alpha = 1e-3, .beta = 2.0, .kappa = 0.0, .jitter = 1e-9};

auto ukf = fin::filtering::ukf_filter(y, ukm, cfg);
```

### Expected output

- `kf.filtered_mean`, `ekf.filtered_mean`, and `ukf.filtered_mean` defined.
- `kf.loglik`, `ekf.loglik`, and `ukf.loglik` finite.
- `ukf.options` controls sigma-point spread and numerical stability.

## Recipe 14: Particle Filter + SV Particle Filter

### API Signature

- `particle_filter(observations, ParticleFilterModel, cfg) -> ParticleFilterResult`
- `sv_particle_filter(log_returns, SvParams, ...) -> ParticleFilterResult`

### Notes

Use generic particle filter for custom state spaces; use `sv_particle_filter` for fast stochastic-vol return filtering.

### Inputs

- transition/log-likelihood callbacks or SV params

### Output

- filtered means, ESS trajectory, weights, optional particle storage

### Options

- resampling method/trigger/threshold and particle count

### Examples

14A) Generic particle filter (custom transition + log-likelihood)

```cpp
fin::filtering::ParticleFilterModel pm;
pm.initial_particles = pluss::randn({1000, 1}, cfg, 20260711ULL);
pm.options.ess_threshold = 0.6;
pm.options.resample = true;
pm.options.resample_method = fin::filtering::ParticleResampleMethod::Stratified;
pm.options.resample_trigger = fin::filtering::ParticleResampleTrigger::Ratio;
pm.options.resample_seed = 20260712ULL;

pm.transition = [=](const pluss::Series& particles, std::int64_t t) {
  return particles.mul(0.98).add(pluss::randn({particles.sizes()[0], 1}, cfg, 1000 + static_cast<std::uint64_t>(t)).mul(0.1));
};

pm.loglik = [](const pluss::Series& particles, const pluss::Series& obs, std::int64_t) {
  const auto resid = obs.reshape({1,1}).sub(particles);
  return resid.square().mul(-0.5).reshape({particles.sizes()[0]});
};

auto pf = fin::filtering::particle_filter(pluss::full({40}, 0.0, cfg), pm, cfg);
```

14B) Specialized SV particle filter convenience

```cpp
auto log_returns = pluss::randn({252}, cfg, 20260721ULL).mul(0.01);

auto svpf = fin::filtering::sv_particle_filter(
    log_returns,
    fin::volatility::SvParams{},
    4000,      // num_particles
    1.0 / 252.0,
    0.55,      // ess_threshold
    cfg,
    20260722ULL);
```

### Expected output

- `pf.ess` defined with one ESS value per observation step.
- `pf.filtered_mean` and `pf.weights` remain defined for all steps.
- `svpf.ess` and `svpf.filtered_mean` expose volatility-state diagnostics.

## Recipe 15: Delta-Hedge Trajectory

### API Signature

- `hdg::delta_hedge(...) -> DeltaHedgeResult`

### Notes

Use to evaluate discrete-time hedge replication error under assumed volatility.

### Inputs

- realized spot path and contract terms

### Output

- portfolio and hedge-error time series + terminal PnL

### Options

- increasing path resolution reduces rebalance discretization artifacts

### Examples

```cpp
auto spot_path = pluss::linspace(95.0, 110.0, 80, cfg);
auto hedged = fin::hedging::delta_hedge(
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

- finite `hedged.terminal_pnl` and defined `hedged.hedge_error` path.

## Recipe 16: Vol Bridge Joint Objective (Returns + Options)

### API Signature

- `weighted_joint_loglik(...)`
- `weighted_joint_nll(...)`
- `returns_metrics(...)`
- `pricing_metrics(...)`

### Notes

Use this bridge to combine time-series evidence and option-pricing evidence in one calibration criterion.

### Inputs

- return log-likelihood and option log-likelihood components

### Output

- weighted joint criterion

### Options

- `JointLikelihoodWeights` sets relative influence

### Examples

```cpp
fin::calibration::JointLikelihoodWeights w{.returns_weight = 1.0, .options_weight = 2.5};

double joint_ll = fin::calibration::weighted_joint_loglik(-1200.0, -380.0, w);
double joint_nll = fin::calibration::weighted_joint_nll(-1200.0, -380.0, w);
```

### Expected output

- finite scalar objective values usable directly in optimization loops.

## Practical Production Notes

### API Signature

- applies across all recipes

### Notes

Finance pipelines are sensitive to numeric settings and shape contracts.

### Inputs

- config + options + data contracts

### Output

- stable, reproducible outputs and clearer failure diagnosis

### Options

1. Prefer `Float64` for calibration/filtering and Greeks.
2. Set seeds explicitly in every stochastic option struct.
3. Start with smaller MC budgets for smoke checks, then scale paths/steps.
4. Validate input shape compatibility before expensive calibration runs.

### Examples

```cpp
// Smoke-first then scale pattern:
auto mc_small = fin::pricing::MonteCarloOptions{.paths = 5000, .steps = 64, .seed = 42};
auto mc_full  = fin::pricing::MonteCarloOptions{.paths = 50000, .steps = 252, .seed = 42};
```

### Expected output

- faster iteration during development and stable full-scale production runs.
