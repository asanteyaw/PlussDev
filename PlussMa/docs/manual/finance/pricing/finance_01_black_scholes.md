<!-- generated from finance_01_black_scholes.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Finance 1: Black-Scholes in the field

The Black-Scholes model prices a European option on an asset with constant volatility. It is the reference that every richer model in this guide reduces to, and the basis of the quoting convention of implied volatility. After this chapter you can build a `Field` for a Black-Scholes option, price calls and puts, read and check the five Greeks, price whole strips with `Series` inputs, and know exactly which inputs are rejected.

Every program is complete, and its output is the output of that program.

## The object

```cpp
instrument::Vanilla instrument::make(pricing::OptionType type, double strike, double maturity, Config cfg = f64_default());
market::State market::make(double spot, double rate, double dividend_yield, Config cfg = f64_default());
module::Model model::make(model::Kind kind, std::initializer_list<double> params, Config cfg = f64_default());
module::Field::Result module::Field::price(Config cfg = {}) const;
```

The three factories build the instrument, the market and the model, and `price` prices the field. Assembled into a `Field`:

```cpp
mod::Field field{.instrument = fin::instrument::make(prc::Call, strike, maturity),
                 .market = fin::market::make(spot, rate, dividend_yield),
                 .model = fin::model::make(fin::model::Kind::BlackScholes, {volatility}),
                 .controls = {.engine = mod::Engine::Analytic, .risk = mod::Risk::All}};
mod::Result result = field.price();     // result.price, result.greeks.{delta,gamma,vega,theta,rho}
```

`fin` is `pluss::subsp::math::finance`, `mod` is `fin::module` and `prc` is `fin::pricing`. A `Field` is **instrument + market + model + controls**: what is bought, the market it is bought in, the stochastic model that values it, and the engine that computes the value. Changing the model or the engine changes nothing else: the later chapters price the same instrument under Heston, Bates and SABR, and by Fourier transform, simulation and finite differences, by editing one line.

| Input | Constraint |
|---|---|
| `spot`, `strike`, `volatility` | finite and positive |
| `maturity` | finite and non-negative; zero gives the intrinsic value |
| `rate`, `dividend_yield` | finite; may be negative |

## Programs

- [Prices and put-call parity](finance_01_black_scholes/prices_and_parity.md)
- [The Greeks](finance_01_black_scholes/the_greeks.md)
- [Batches of spots and maturities](finance_01_black_scholes/batches_of_spots_and_maturities.md)
- [Edge cases and errors](finance_01_black_scholes/edge_cases_and_errors.md)

## Checklist

1. Quote `vega` and `rho` per unit, `theta` per year: scale them before comparing with a market convention.
2. A call minus a put must equal `S e^(−qT) − K e^(−rT)` whatever the volatility; use it to test any pricer.
3. `Series` inputs of length *n* are paired element by element, not combined into a grid; a single value applies to all.
4. A zero volatility is an error: use a small positive one, or the discounted forward intrinsic value.
5. A field reports every invalid input at once, naming each.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

finance_01_black_scholes/prices_and_parity
finance_01_black_scholes/the_greeks
finance_01_black_scholes/batches_of_spots_and_maturities
finance_01_black_scholes/edge_cases_and_errors
```
<!-- apiref:toctree:end -->
