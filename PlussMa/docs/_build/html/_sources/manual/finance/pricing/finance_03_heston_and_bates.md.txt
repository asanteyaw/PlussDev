<!-- generated from finance_03_heston_and_bates.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Finance 3: Heston and Bates

Stochastic-volatility models explain the smile that Black-Scholes cannot, and the Heston model does it with a one-dimensional integral. This chapter prices European options under Heston and its jump extension Bates by three semi-analytic methods, numerical integration, the Carr-Madan FFT and the COS series, and shows how to price under any model whose characteristic function is known. Monte Carlo pricing of the same models is the next chapter.

Every program is complete, and its output is the output of that program.

## The calls

| Model | Integration | FFT | COS |
|---|---|---|---|
| Heston | `heston_price` | `heston_price_fft`, `heston_price_fft_grid` | `heston_price_cos` (scalar and `Series`) |
| Bates | | `bates_price_fft`, `bates_price_fft_grid` | `bates_price_cos` |
| any characteristic function | | `option_price_fft`, `option_price_fft_grid` | `option_price_cos` (scalar and `Series`) |

All are in `pluss::subsp::math::finance::pricing` (alias `prc`), with the parameter structs `HestonParams` and `BatesParams` in `...::volatility` (alias `vol`). The controls are `HestonIntegrationOptions`, `HestonFftOptions` and `CosOptions`.

## Programs

- [The Heston model](finance_03_heston_and_bates/the_heston_model.md)
- [The FFT pricer](finance_03_heston_and_bates/the_fft_pricer.md)
- [The COS pricer](finance_03_heston_and_bates/the_cos_pricer.md)
- [Jumps: the Bates model](finance_03_heston_and_bates/jumps_the_bates_model.md)
- [Generic characteristic functions](finance_03_heston_and_bates/generic_characteristic_functions.md)
- [Smiles from Heston](finance_03_heston_and_bates/smiles_from_heston.md)

## Checklist

1. Check any pricer by put-call parity and by the Black-Scholes limit (a small volatility of variance).
2. Prefer COS for accuracy and for batches; use the FFT when a whole strike grid is wanted; use integration as the independent reference.
3. Leave `mode` at `LittleTrap`: the original form is wrong for long maturities.
4. The FFT is accurate only if the frequency range *N* Δu covers the decay of the characteristic function; raise Δu (not just *N*) if the answer moves.
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
