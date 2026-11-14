<!-- generated from finance_03_heston_and_bates.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Finance 3: Heston and Bates in the field

Stochastic-volatility models explain the smile that Black-Scholes cannot. This chapter prices European options under Heston and its jump extension Bates by the two Fourier engines of the `Field`, the Carr-Madan FFT and the COS series, cross-checks them against each other, and shows how to price under any model whose characteristic function is known. Monte Carlo pricing of the same models is the next chapter.

Every program is complete, and its output is the output of that program.

## The calls

```cpp
instrument::Vanilla instrument::make(pricing::OptionType type, double strike, double maturity, Config cfg = f64_default());
market::State market::make(double spot, double rate, double dividend_yield, Config cfg = f64_default());
module::Model model::make(model::Kind kind, std::initializer_list<double> params, Config cfg = f64_default());
module::Field::Result module::Field::price(Config cfg = {}) const;
module::Engine module::Controls::engine = module::Engine::Analytic;
```

Heston takes five parameters and Bates three more, and the engine is `Cos` or `Fft`. Assembled:

```cpp
mod::Field field{.instrument = fin::instrument::make(prc::Call, strike, maturity),
                 .market = fin::market::make(spot, rate, dividend_yield),
                 .model = fin::model::make(fin::model::Kind::Heston, {kappa, theta, sigma, rho, v0}),     // or Kind::Bates with three jump parameters more
                 .controls = {.engine = mod::Engine::Cos}};                                               // or Engine::Fft
field.price().price;
```

| Model | Engines of this chapter | Controls |
|---|---|---|
| Heston | `Cos`, `Fft` | `field.controls.cos`, `field.controls.fft` |
| Bates | `Cos`, `Fft` | the same |
| any characteristic function | `option_price_cos`, `option_price_fft`, `option_price_fft_grid` (free functions) | `CosOptions`, `HestonFftOptions` |

`fin` is `pluss::subsp::math::finance`, `mod` is `fin::module`, `prc` is `fin::pricing`. The Heston and Bates field models take their parameters in the documented order; the engine is a switch, so the same field priced twice with different engines is the cross-check used throughout.

## Programs

- [The Heston model](finance_03_heston_and_bates/the_heston_model.md)
- [The FFT pricer](finance_03_heston_and_bates/the_fft_pricer.md)
- [The COS pricer](finance_03_heston_and_bates/the_cos_pricer.md)
- [Jumps: the Bates model](finance_03_heston_and_bates/jumps_the_bates_model.md)
- [Generic characteristic functions](finance_03_heston_and_bates/generic_characteristic_functions.md)
- [Smiles from Heston](finance_03_heston_and_bates/smiles_from_heston.md)

## Checklist

1. Check any pricer by put-call parity and by the Black-Scholes limit (a small volatility of variance).
2. Prefer COS for accuracy and for batches; use the FFT when a whole strike grid is wanted; price the same field with both engines as the independent check.
3. Leave `controls.fft.mode` at `LittleTrap`: the original form is refused for long maturities.
4. The FFT is accurate only if the frequency range *N* Δu covers the decay of the characteristic function; raise `du` (not just `num_fft`) if the answer moves.
5. A supplied characteristic function must be the plain one, of ln S_T, with a risk-neutral drift; both generic pricers apply the discounting.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

finance_03_heston_and_bates/generic_characteristic_functions
finance_03_heston_and_bates/jumps_the_bates_model
finance_03_heston_and_bates/smiles_from_heston
finance_03_heston_and_bates/the_cos_pricer
finance_03_heston_and_bates/the_fft_pricer
finance_03_heston_and_bates/the_heston_model
```
<!-- apiref:toctree:end -->
