<!-- generated from stochastic.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Stochastic calculus

Operators for stochastic integrals along a sampled path, the quadratic variation and covariation of paths, and the exact formulas of Itô and Stratonovich calculus: generators, the change-of-variables formula, conversion between conventions, and Malliavin calculus on a discretised Wiener path.

## The Surface

| Symbol | What it is |
|---|---|
| [`IntegralOptions`](stochastic/IntegralOptions.md) | Options of the stochastic integral: the convention, `θ`, whether to keep the running integral, and the path axis. |
| [`PathOptions`](stochastic/PathOptions.md) | Options of the path-variation operators: whether to keep the running value and which axis is the path. |
| [`convention_change`](stochastic/convention_change.md) | Rewrites a drift so that the same process is described in another convention. |
| [`covariation`](stochastic/covariation.md) | The quadratic covariation `[X, Y]` of two sampled paths: the sum of products of their increments. |
| [`generator`](stochastic/generator.md) | The infinitesimal generator of a diffusion: `L g = μ·∇g + ½ tr(σσᵀ ∇²g)`. |
| [`malliavin_derivative`](stochastic/malliavin_derivative.md) | The Malliavin derivative of a functional of a discretised Wiener path, as a vector of sensitivities to each increment. |
| [`quadratic_variation`](stochastic/quadratic_variation.md) | The quadratic variation `[X]` of a sampled path: the sum of squared increments. |
| [`skorokhod_integral`](stochastic/skorokhod_integral.md) | The Skorokhod integral of a vector process on a discretised Wiener path: an Itô integral that also accepts anticipating integrands. |
| [`stochastic_differential`](stochastic/stochastic_differential.md) | Itô's formula (and its Stratonovich and θ versions): the drift and diffusion of `g(X)` for `dX = μ dt + σ ∘_θ dW`. |
| [`stochastic_integral`](stochastic/stochastic_integral.md) | The stochastic integral of a function of a sampled path against a noise path, in the Itô, Stratonovich, Klimontovich or general θ convention. |
| [`theta_of`](stochastic/theta_of.md) | The `θ` of a convention: where in each step the integrand is read. |

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

stochastic/IntegralOptions
stochastic/PathOptions
stochastic/convention_change
stochastic/covariation
stochastic/generator
stochastic/malliavin_derivative
stochastic/quadratic_variation
stochastic/skorokhod_integral
stochastic/stochastic_differential
stochastic/stochastic_integral
stochastic/theta_of
```
<!-- apiref:toctree:end -->
