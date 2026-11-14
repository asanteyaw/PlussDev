<!-- generated from finance_10_volatility_bridge.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Finance 10: The Volatility Bridge

Returns say what volatility has been and options say what it is expected to be. This chapter connects the two with GARCH-type models. After it you can turn a fitted GARCH, GJR or EGARCH model into conditional variances and a forecast over the life of an option, test whether an asymmetric model is needed, work with the NGARCH-in-mean model that carries a risk premium and a leverage effect, check it with its standardised innovations, and combine a returns likelihood with an options likelihood in one objective.

Every program is complete and its output is the output of that program.

## The mental model

- **A GARCH variance remembers.** Today's variance depends on yesterday's return and yesterday's variance, so it can be inferred from the returns and forecast, and it reverts to a long-run level at the rate of the persistence.
- **Asymmetry matters for equities.** GJR and EGARCH raise the variance more after a fall; the likelihood-ratio test tells whether the data need it.
- **The forecast over an option's life is an average.** Averaging the daily variance forecasts gives the volatility that prices an option of that maturity.
- **A pricing model carries a premium.** The NGARCH-in-mean model adds a risk premium {math}`\lambda` and a leverage {math}`\gamma`; its likelihood identifies them, and an options likelihood can be added to it.

| Entry point | Purpose |
|---|---|
| `bridge::returns_metrics`, `bridge::variance_forecast` | likelihood, conditional variances and a variance forecast for a fitted GARCH, GJR or EGARCH model |
| `volatility::unconditional_variance`, `to_pricing_params` | the long-run variance of a model, and a fitted GARCH as pricing parameters |
| `pricing::pricing_metrics`, `pricing_variance_forecast` | likelihood, variances, innovations and forecasts of the NGARCH-in-mean model |
| `bridge::weighted_joint_loglik`, `weighted_joint_nll` | a weighted combination of a returns and an options likelihood |

## Programs

- [From returns to variance forecasts](finance_10_volatility_bridge/returns_and_variance_forecasts.md)
- [The NGARCH model for pricing](finance_10_volatility_bridge/ngarch_for_pricing.md)

## Checklist

1. Keep the units straight: per cent for a fit, fractions for the NGARCH, variances per period.
2. Average the variance forecast over the life of the option.
3. Check a model with the mean and standard deviation of its standardised innovations.
4. Use an options likelihood when the returns cannot identify the risk premium.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

finance_10_volatility_bridge/returns_and_variance_forecasts
finance_10_volatility_bridge/ngarch_for_pricing
```
<!-- apiref:toctree:end -->
