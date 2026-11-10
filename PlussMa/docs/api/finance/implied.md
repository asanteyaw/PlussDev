<!-- generated from implied.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Implied volatility

The volatility that makes the Black-Scholes price equal an observed option price. [`Field::implied_vol`](field/Field/implied_vol.md) inverts the price by a bracketed root-finder for a whole strip of quotes, returning NaN where no volatility can reproduce the price; [`ImpliedVolOptions`](implied/ImpliedVolOptions.md) and [`ImpliedVolMethod`](implied/ImpliedVolMethod.md) choose the algorithm and its limits.

## The Surface

| Symbol | What it is |
|---|---|
| [`ImpliedVolMethod`](implied/ImpliedVolMethod.md) | The root-finding algorithm `Field::implied_vol` uses. |
| [`ImpliedVolOptions`](implied/ImpliedVolOptions.md) | The algorithm, the volatility bracket, the tolerance and the iteration limit of `Field::implied_vol`. |

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

implied/ImpliedVolMethod
implied/ImpliedVolOptions
```
<!-- apiref:toctree:end -->
