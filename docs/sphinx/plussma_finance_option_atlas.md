# PlussMa Finance Option Atlas

This atlas is a compact map of major option combinations across PlussMa Finance.  
Use it to decide model/engine/options quickly before coding.

## Include And Namespace

```cpp
#include <pluss/ma.h>
namespace fin = pluss::ma::finance;
```

## 1) Instrument x Market x Model x Engine Matrix

| Use case | Instrument | Model | Engine | Primary API | Output |
|---|---|---|---|---|---|
| Vanilla European, baseline | `instrument::Vanilla` | BS implicit (vol input) | Analytic | `pricing::black_scholes_price` | `double/Series` |
| Vanilla + Greeks | `instrument::Vanilla` | BS implicit | Analytic | `pricing::black_scholes_greeks` | struct |
| Recover implied vol | `instrument::Vanilla` | BS implicit | Root-find | `pricing::implied_vol` | `double/Series` |
| Heston point price | `instrument::Vanilla` | `volatility::HestonParams` | Quadrature | `pricing::heston_price` | `double` |
| Heston fast grid | `instrument::Vanilla` | `volatility::HestonParams` | FFT | `pricing::heston_price_fft_grid` | `HestonFftResult` |
| Heston stable point | `instrument::Vanilla` | `volatility::HestonParams` | COS | `pricing::heston_price_cos` | `double` |
| Heston Monte Carlo | `instrument::Vanilla` | `volatility::HestonParams` | MC | `pricing::heston_price_mc` | `double` |
| Bates jump model | `instrument::Vanilla` | `volatility::BatesParams` | FFT/COS/MC | `pricing::bates_price_*` | `double` |
| Heston-Nandi GARCH | `instrument::Vanilla` | `volatility::HestonNandiParams` | Semi-analytic/MC | `pricing::heston_nandi_price*` | `double` |
| SABR normal/black vol | `instrument::Vanilla` | `volatility::SabrParams` | Closed-form approx | `pricing::sabr_*` | `double` |
| Rough vol studies | `instrument::Vanilla` | rough params | MC | `pricing::rough_*_price_mc` | `double` |
| Structured risk/loss workflow | `module::Field` | any in `model::make(...)` | configurable | `module::price/risk/objective` | `Series` |

## 2) Functional Pricing Atlas

### 2.1 Black-Scholes Family

| Goal | API | Key options |
|---|---|---|
| Scalar price | `pricing::black_scholes_price(OptionType, double, ...)` | none |
| Vectorized price | `pricing::black_scholes_price(OptionType, Series, ...)` | `Config` |
| Scalar Greeks | `pricing::black_scholes_greeks(OptionType, double, ...)` | none |
| Vectorized Greeks | `pricing::black_scholes_greeks(OptionType, Series, ...)` | `Config` |
| Implied vol scalar | `pricing::implied_vol(..., ImpliedVolOptions)` | method, bounds, tol, max_iter |
| Implied vol vectorized | `pricing::implied_vol(...Series..., ImpliedVolOptions, Config)` | same + platform |

### 2.2 Heston/Bates/HN/SABR/Rough

| Model | Fastest typical | Most robust typical | Notes |
|---|---|---|---|
| Heston | FFT | COS | FFT for surfaces; COS for stable spot checks |
| Bates | FFT/COS | COS | include jump params sanity checks |
| Heston-Nandi | semi-analytic | MC | useful for daily-return-linked dynamics |
| SABR | approx | MC | approx for calibration loops, MC for validation |
| Rough Heston/Bergomi | MC | MC | expensive; tune `paths/steps` carefully |

## 3) Volatility Process Atlas

| Task | API | Output |
|---|---|---|
| Simulate Heston variance | `volatility::simulate_heston_paths` | `Series` paths |
| Simulate Bates variance | `volatility::simulate_bates_paths` | `Series` paths |
| Simulate SABR | `volatility::simulate_sabr_paths` | `Series` paths |
| Simulate SV baseline | `volatility::simulate_sv_paths` | `Series` paths |
| Simulate SLV | `volatility::simulate_slv_paths` | `Series` paths |
| Simulate rough Heston | `volatility::simulate_rough_heston_paths` | `Series` paths |
| Simulate rough Bergomi | `volatility::simulate_rough_bergomi_paths` | `Series` paths |

Recommended first-pass settings:
- `steps_per_year = 252`
- `paths >= 10000` for directional diagnostics
- MC for production calibration: `paths >= 50000` and repeated seeds

## 4) Rates Atlas

| Goal | API | Notes |
|---|---|---|
| Vasicek short-rate paths | `rates::simulate_vasicek_paths` | closed-form friendly Gaussian model |
| CIR short-rate paths | `rates::simulate_cir_paths` | positivity-sensitive; check Feller condition |
| Hull-White paths | `rates::simulate_hull_white_paths` | time-dependent mean-reversion context |
| Zero-coupon bond pricing | `rates::zero_coupon_bond_price` | scalar closed-form family |
| Rate parameter calibration | `rates::calibrate_*_from_curve` | curve-to-model fit |

## 5) Calibration Atlas

### 5.1 Option Calibration APIs

| API | Model | Input market | Output |
|---|---|---|---|
| `calibration::calibrate_heston` | Heston | spot, rate/dividend, maturities, strikes, market prices | `CalibrationResult<HestonParams>` |
| `calibration::calibrate_bates` | Bates | same | `CalibrationResult<BatesParams>` |
| `calibration::calibrate_heston_nandi` | HN | same | `CalibrationResult<HestonNandiParams>` |
| `calibration::calibrate_sabr` | SABR | same | `CalibrationResult<SabrParams>` |
| `calibration::calibrate_sv` | SV | same | `CalibrationResult<SvParams>` |
| `calibration::calibrate_slv` | SLV | same | `CalibrationResult<SlvParams>` |
| `calibration::calibrate_rough_heston` | rough Heston | same | `CalibrationResult<RoughHestonParams>` |
| `calibration::calibrate_rough_bergomi` | rough Bergomi | same | `CalibrationResult<RoughBergomiParams>` |

### 5.2 Calibration Option Controls

| Option field | Meaning | Typical start |
|---|---|---|
| `max_iterations` | optimization budget | 500-2000 |
| `tolerance` | stopping tolerance | `1e-6` |
| `seed` / deterministic | reproducibility | fixed seed + deterministic |
| bounds/projections | parameter admissibility | model-specific hard constraints |

## 6) Object Surface Atlas

### 6.1 Instruments

| Type | Builder | Key fields |
|---|---|---|
| Vanilla | `instrument::Vanilla{...}` | `is_call`, `strike`, `maturity` |
| Digital | `instrument::Digital{...}` | payout style, barrier-ish flags if applicable |
| Barrier | `instrument::Barrier{...}` | barrier level/type, rebate |
| Asian | `instrument::Asian{...}` | averaging setup |

### 6.2 Market State

| Field | Meaning |
|---|---|
| `spot` | underlying spot level(s) |
| `rate` | risk-free rate(s) |
| `dividend_yield` | carry/dividend yield(s) |

### 6.3 Model Build

| API | Purpose |
|---|---|
| `model::make(...)` | create parameterized model packs |
| `models::*` | OOP model classes (`params()`, `calibrate_inplace`) |

### 6.4 Pricer Front

| API | Purpose |
|---|---|
| `pricer::price(instrument, market, model, controls)` | direct price call |
| `pricer::greeks(...)` | risk sensitivities |

## 7) module::Field Atlas (Workflow Layer)

`module::Field` combines instrument + market + model + controls and exposes a compact compute stack.

| Method | What it returns |
|---|---|
| `module::price(field)` | modeled price(s) |
| `module::risk(field)` | risk output configured by `controls.risk` |
| `module::residual(field, observed)` | model minus observed |
| `module::loss(field, observed)` | scalar/series loss |
| `module::objective(field, observed)` | objective for calibration loops |

### Canonical initialization

```cpp
fin::module::Field wf{
    .instrument = fin::instrument::Vanilla{
        .type = fin::pricing::OptionType::Auto,
        .is_call = is_call_series,
        .strike = strike_series,
        .maturity = maturity_series,
    },
    .market = fin::market::State{
        .spot = spot_series,
        .rate = rate_series,
        .dividend_yield = div_series,
    },
    .model = fin::model::make(1.5, 0.04, 0.3, -0.7, 0.04, 0.12, -0.08, 0.20, cfg),
    .controls = fin::module::Controls{
        .engine = fin::module::Engine::Cos,
        .risk = fin::module::Risk::PriceOnly,
        .loss = fin::module::Loss::MSE,
    },
};
```

## 8) Filtering Atlas

| Family | API prefix | Typical use |
|---|---|---|
| Linear Gaussian | `filtering::kalman_*` | latent state extraction in linear state-space |
| Nonlinear Gaussian | `filtering::ekf_*`, `filtering::ukf_*` | nonlinear observation/state maps |
| Non-Gaussian | `filtering::particle_*` | multimodal/heavy-tail latent inference |

## 9) Hedging Atlas

| Task | API |
|---|---|
| Delta hedge simulation | `hedging::delta_hedge_*` |
| PnL path diagnostics | `hedging::hedge_pnl_*` |

Use fixed transaction-cost assumptions and rebalance grid sensitivity sweeps when comparing strategies.

## 10) Vol Bridge Atlas

| Goal | API |
|---|---|
| Bring Stats vol estimates into pricing stack | `vol_bridge::*` |
| Joint objective with market prices + time-series residuals | bridge objective helpers |

## 11) Selection Quick Rules

- Need maximum speed on vanilla surfaces: Heston FFT.
- Need robust point checks: COS.
- Need path-dependent behaviors: Monte Carlo.
- Need strict reproducibility: deterministic seeds + explicit engine and scheme.
- Need calibration with structural constraints: enforce model-specific bounds and inspect residual slices by maturity/strike.

## 12) Minimal Decision Flow

1. Choose instrument family.
2. Choose model family (BS/Heston/Bates/HN/SABR/rough).
3. Choose engine (analytic/FFT/COS/MC).
4. Choose API style (functional vs object vs `module::Field`).
5. Pick calibration/filtering/hedging overlays as needed.
6. Lock reproducibility controls (dtype, platform, seed).

