# Finance Object Framework Blueprint

## Goal

Build a Pluss-native object framework equivalent to MATLAB `fininstrument` + `finmodel` + `finpricer` + `price`, with stronger typing, better extensibility, and reusable pricing kernels (especially generic FFT/FRFT).

## MATLAB Inspiration (read + mapped)

- Local files:
  - `/Applications/MATLAB_R2025b.app/toolbox/fininst/fininst/fininstrument.m`
  - `/Applications/MATLAB_R2025b.app/toolbox/fininst/fininst/finmodel.m`
  - `/Applications/MATLAB_R2025b.app/toolbox/fininst/fininst/finpricer.m`
  - `/Applications/MATLAB_R2025b.app/toolbox/fininst/fininst/+fininstrument/FinInstrument.m`
  - `/Applications/MATLAB_R2025b.app/toolbox/fininst/fininst/+finmodel/FinModel.m`
  - `/Applications/MATLAB_R2025b.app/toolbox/fininst/fininst/+finpricer/FinPricer.m`
  - `/Applications/MATLAB_R2025b.app/toolbox/fininst/fininst/+finpricer/FFT.m`
  - `/Applications/MATLAB_R2025b.app/toolbox/fininst/fininst/optByHestonFFT.m`
  - `/Applications/MATLAB_R2025b.app/toolbox/fininst/fininst/optByBatesFFT.m`
  - `/Applications/MATLAB_R2025b.app/toolbox/fininst/fininst/optSensByFFT.m`
  - `/Applications/MATLAB_R2025b.app/toolbox/fininst/fininst/priceresult.m`
- Web docs:
  - `https://www.mathworks.com/help/fininst/create-credit-derivative-instruments-objects.html`
  - `https://www.mathworks.com/help/fininst/choosing-instruments-models-and-pricers.html`
  - `https://www.mathworks.com/help/fininst/finpricer.fft.html`

## Target Design (Pluss)

### Namespaces

- `pluss::subsp::math::finance::instrument`
- `pluss::subsp::math::finance::model`
- `pluss::subsp::math::finance::pricer`
- `pluss::subsp::math::finance::portfolio`
- `pluss::subsp::math::finance::market`

### Core Objects

- `InstrumentBase` (typed derivatives: `Vanilla`, `Barrier`, `CDS`, `Swaption`, ...)
- `ModelBase` (typed models: `BlackScholes`, `Heston`, `Bates`, `SABR`, `HullWhite`, ...)
- `PricerBase` (typed engines: `Analytic`, `FFT`, `Tree`, `MC`, `FD`, `Credit`)
- `MarketState` (curves, spot/forward, dividend/borrow, calendars/daycount, today date)
- `PriceRequest` (instrument + model + pricer + market + risk flags)
- `PriceResult`:
  - scalar/vector price
  - optional greeks table
  - diagnostics (grid/scheme/iterations/convergence/errors)
  - optional metadata payload

### Capability Contracts (no string dispatch)

- `SupportsCF` (model exposes characteristic function)
- `SupportsPathSim` (model exposes transition/path generator)
- `SupportsTree` (model exposes tree step coefficients)
- `SupportsPDE` (model exposes local generator coefficients)
- `SupportsClosedForm` (model/instrument combination has analytic formula)

Pricers compile against capability concepts; invalid model-pricer-instrument mixes fail early with explicit errors.

## Generic FFT/FRFT Engine (high-impact kernel)

### Required Inputs

- Characteristic function callback `phi(u, T, state, params)`
- Carr-Madan damping `alpha`
- `num_fft`, `du` (`CharacteristicFcnStep`)
- optional `dk` (`LogStrikeStep`), else `dk = 2*pi/(num_fft*du)`
- quadrature rule (`simpson` or `trapezoidal`)
- `spot/forward`, `r`, `q`, maturity

### Behavior

- If `dk * du == 2*pi/num_fft` (within tolerance): use FFT.
- Else: use FRFT.
- Compute full strike grid when strike input empty.
- Interpolate safely for requested strikes.
- Return greeks if characteristic derivatives are available; else fallback to robust bumping with shared kernels.

### Why this is better

- One kernel for Heston/Bates/Merton and future affine/Levy CF models.
- Clean separation: model owns CF, pricer owns transform numerics.
- Same engine supports price + sensitivities.

## Object API Shape (ergonomic + explicit)

```cpp
namespace fin = pluss::subsp::math::finance;

auto inst = fin::instrument::Vanilla{/* strike, maturity, call/put */};
auto mdl  = fin::model::Heston{/* params */};
auto mkt  = fin::market::State{/* curves, spot, q */};
auto eng  = fin::pricer::Fft{/* alpha, num_fft, du, dk, quadrature, little_trap */};

auto res = eng.price(inst, mdl, mkt, fin::pricer::RiskSpec::All);
// res.price, res.greeks, res.diagnostics
```

## Execution Plan

### Phase 1: Framework skeleton

- Add base objects (`InstrumentBase`, `ModelBase`, `PricerBase`, `PriceResult`, `MarketState`).
- Add strict validation/errors and compatibility checks.
- Add `price(...)` dispatcher with typed overloads (not string maps).

### Phase 2: Generic FFT stack

- Extract/standardize characteristic-function interface.
- Add reusable `fft_kernel` + `frft_kernel` + quadrature module.
- Wire Heston/Bates/Merton through same kernel.
- Add strike-grid mode and greeks mode.

### Phase 3: Object migration

- Wrap existing function APIs under object methods without behavior regression.
- Keep free-function APIs for power users; objects call same kernels.

### Phase 4: Portfolio + batch

- `Portfolio` object with vectorized pricing over mixed instruments.
- Shared market snapshot + parallel engine execution.
- Result aggregation table (MATLAB `priceresult` equivalent, but typed).

### Phase 5: Model-pricer matrix expansion

- Add documented compatibility matrix (`instrument x model x pricer`) in code and docs.
- Fill high-impact gaps by asset class (equity/IR/credit/inflation).

## Testing Strategy

- Kernel parity tests against existing trusted paths (Heston/Bates FFT currently cross-referenced).
- Deterministic regression fixtures for prices/greeks on fixed grids.
- Stress tests for:
  - deep ITM/OTM
  - very short/long maturities
  - extreme vol/jump parameters
  - FRFT/FFT boundary (`dk*du` close to `2*pi/N`)
- Catch2 benchmark tests for vectorized/batch throughput.

## Non-Goals

- No global string factory routing in hot path.
- No duplicated pricing logic per model when generic kernel can serve all.
- No forced monolithic include; keep module-level includes and clean dependency boundaries.
