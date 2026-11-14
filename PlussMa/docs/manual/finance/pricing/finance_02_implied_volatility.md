<!-- generated from finance_02_implied_volatility.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Finance 2: Implied Volatility

The implied volatility of an option is the volatility that makes the Black-Scholes price equal the observed price. It is the common language in which option prices are quoted and compared, and the quantity that richer models are calibrated to reproduce. After this chapter you can invert a quote or a whole strip of them, choose and control the solver, and know exactly when the answer is `NaN`.

Every program is complete, and its output is the output of that program.

## The call

```cpp
Series module::Field::implied_vol(const pricing::ImpliedVolOptions &options = {}, Config cfg = {}) const;                         // implied by the field's own model prices
Series module::Field::implied_vol(const Series &quotes, const pricing::ImpliedVolOptions &options = {}, Config cfg = {}) const;   // implied by market quotes
```

`implied_vol` is a method of the `Field` of chapter 1. The field supplies the instrument and the market, one Black-Scholes price is inverted per option, and the answer is a `Series`. Without `quotes` it inverts the field's own prices, so the same call turns a Heston or SABR field into its smile.

| `ImpliedVolOptions` | Meaning | Default |
|---|---|---|
| `method` | `Auto` (= `Brent`), `Newton`, `Secant`, `Bisection`, `Brent` | `Auto` |
| `lower_vol`, `upper_vol` | the search bracket | 1e-8, 10 |
| `tolerance` | accepted price error, relative to the time value | 1e-8 |
| `max_iterations` | iteration cap | 128 |
| `vega_floor` | where the vega is below this, Newton takes a bisection step | 1e-12 |

## Programs

- [Recovering a volatility](finance_02_implied_volatility/recovering_a_volatility.md)
- [Controls and the no-arbitrage bounds](finance_02_implied_volatility/controls_and_the_no_arbitrage_bounds.md)

## Checklist

1. The result is `NaN`, not an error, when no volatility in the bracket reproduces the price: filter with `isnan`, and look at the quote.
2. Every price must lie strictly between the discounted intrinsic value and the discounted spot (for a call).
3. A solver cut off at `max_iterations` gives `NaN`, never a half-converged number.
4. The bracket is a plausibility filter as well as a search range.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

finance_02_implied_volatility/recovering_a_volatility
finance_02_implied_volatility/controls_and_the_no_arbitrage_bounds
```
<!-- apiref:toctree:end -->
