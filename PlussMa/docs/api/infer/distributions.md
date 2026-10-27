<!-- generated from distributions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# infer: distributions

Tests of a whole distribution: Kolmogorov–Smirnov (one and two samples) and Jarque–Bera. Each page documents one function: its signature, what it computes, the parameters, the fields of the result, the errors, and examples with their output.

## The Surface

| Function | What it does |
|---|---|
| [`kstest`](distributions/kstest.md) | One-sample Kolmogorov–Smirnov test against a completely specified normal distribution. |
| [`kstest2`](distributions/kstest2.md) | Two-sample Kolmogorov–Smirnov test: do two samples come from the same distribution? |
| [`jbtest`](distributions/jbtest.md) | Jarque–Bera test of normality, from the skewness and kurtosis. |

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

distributions/kstest
distributions/kstest2
distributions/jbtest
```
<!-- apiref:toctree:end -->
