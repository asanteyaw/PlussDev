<!-- generated from plussma_distr.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Distributions API Reference

The distributions of PlussMa live in `pluss::subsp::stats::distr`. Fifty-seven families are represented by `dst::Kind`; `GaussianMixture` adds one object-only family, for fifty-eight families overall. Every kind is reachable through the same functional surface and through the generic `distr::Distribution` object. Twenty-four frequently used families also have dedicated typed wrappers.

## Two Ways In

**Kind-based functions** work for every family. `dst::Kind` is an enumeration with one value per family, and the free functions `distr::pdf`, `distr::logpdf`, `distr::cdf`, `distr::logcdf` and `distr::sample` take the kind, the points (or the shape of the draws) and a `std::vector<double>` of parameters in the order the family's page gives. Every function evaluates a whole `Series` at once.

**Objects** (the [object based distributions](distr/object_based.md)) bind a family and its parameters. `distr::Distribution{kind, params}` works for every `dst::Kind` and delegates evaluation and sampling to the kind-based functions. Dedicated wrappers such as `distr::Normal{mean, stddev}` and `distr::Gamma{shape, scale, location}` add strongly named parameters and exact family-specific operations. The object surface includes `pdf`, `cdf`, `logpdf`, `logcdf`, `sample`, `sample_like`, the cumulant generating function `cgf`, `cumulant(order)`, and maximum-likelihood fitting where supported. An object can take a `Series` for each parameter so one call evaluates a different distribution at every point.

The examples print with plain `std::cout`: a `Series` is exported through
`host<double>()`, and its contiguous row-major values are read from
`.array().data`.

## The Families and the Object Forms

| Group | Families | What they cover |
|---|---|---|
| [Continuous, whole real line](distr/continuous_real_line.md) | 10 | Families whose values range over the whole real line: the normal and its heavy-tailed and skewed relatives, and the mixtures built from them. |
| [Continuous, positive](distr/continuous_positive.md) | 23 | Families for positive quantities: lifetimes, waiting times, sizes, amplitudes and the distributions of test statistics. |
| [Continuous, bounded](distr/continuous_bounded.md) | 13 | Families on a bounded interval or a circle: proportions, bounded shapes and directions. |
| [Discrete](distr/discrete.md) | 11 | Families for counts and other integer-valued variables. |
| [Object based distributions](distr/object_based.md) | 25 object types | The generic `Distribution` plus 24 dedicated wrappers: parameters held in an object, with density, distribution function, draws, cumulants and fits as methods. |

## Conventions

- Parameters outside their valid range, or a parameter list of the wrong length, are refused with an error that names the requirement.
- A point outside the support has density zero (mass zero for a discrete family), distribution value 0 or 1, and `logpdf` equal to `-inf`.
- `sample` takes a seed: the same seed always gives the same draws, and a seed of 0 is a fixed default, not a random one.
- Every family page follows the same pattern: the density, the distribution function and the log-density on a grid; a check that the density, the distribution function and the sampler describe the same distribution; how the shape changes with the parameters; and, where they exist, the object form, the fit, a closed form, and a relationship to another family.

```{toctree}
:maxdepth: 3
:hidden:

distr/continuous_real_line
distr/continuous_positive
distr/continuous_bounded
distr/discrete
distr/object_based
```
