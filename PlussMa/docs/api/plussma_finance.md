<!-- generated from plussma_finance.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Finance API Reference

The finance tools of PlussMa live in `pluss::subsp::math::finance`. They price options by closed form, transforms, simulation, trees and PDEs through one pricing field, invert prices into implied volatilities, simulate rates and stochastic-volatility models, delta-hedge, and filter hidden states, always over whole `Series` of spots, strikes and maturities so that a surface is one call.

```cpp
#include <pluss/ma.h>

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
```

Every page of this reference documents one function, enumeration or options structure: what it is, when to call it, every parameter and field, what it returns and what it refuses, and programs with real output that cover each overload and each option. Looking for the mathematics behind the models? The {guide}`finance guide <finance>` develops it.

## The families

| Family | What it covers |
|---|---|
| [Implied volatility](finance/implied.md) | Inverting a price into a Black-Scholes volatility, with a choice of algorithm |
| [The pricing field](finance/field.md) | The `Field` object: pricing, Greeks, losses and objectives, simulated paths; engines and controls |
| [Several assets](finance/chord.md) | The `Chord`: named Fields priced together, with assets that can share a call merged into one |
| [Models](finance/model.md) | The six model families, their parameters and the `Model` value |
| [Instruments and market](finance/instrument.md) | Vanilla, American and Bermudan instruments; the market state |
| [Pricing engines](finance/engines.md) | Analytic, FFT, COS, Monte Carlo, lattice, spectral and PDE engines with every setting |
| [Interest-rate models](finance/rates.md) | Vasicek, CIR, Hull-White and HJM simulation; closed-form zero-coupon bonds |
| [Delta hedging](finance/hedging.md) | Replicating an option along a spot path and measuring the hedging error |
| [Filtering](finance/filtering.md) | Kalman, extended, unscented and particle filters; a stochastic-volatility particle filter |
| [The volatility bridge](finance/volbridge.md) | Fitted GARCH models to pricing dynamics: likelihoods, variances, forecasts, NGARCH, joint likelihood |

Pricing under stochastic-volatility models goes through the pricing field and its engines (FFT, COS, Monte Carlo, PDE, lattice and so on); the functions behind those engines (`heston_price_cos` and the like) are the implementation layer and are not a separate interface.

## Conventions

- Rates and dividend yields are annual and continuously compounded; maturities are in years; volatility is annual.
- A `Series` argument is a batch: two `Series` arguments are paired element by element, not combined into a grid.
- Examples print with `pluss::tils::OutputEngine`; a `Series` is read with `.items<double>()`.
- Invalid inputs throw `pluss::Error` naming the operation and the problem; a quantity that cannot exist (an implied volatility of an unattainable price) is NaN.

```{toctree}
:maxdepth: 3
:hidden:

finance/implied
finance/field
finance/chord
finance/model
finance/instrument
finance/engines
finance/rates
finance/hedging
finance/filtering
finance/volbridge
```
