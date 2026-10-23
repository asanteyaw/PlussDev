<!-- generated from stochastic_04_simulating_sdes.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Stochastic 4: Simulating and Differentiating SDEs

A stochastic differential equation is rarely solvable in closed form, so you simulate it and ask two questions: how close is the simulation, and how does an answer change with a parameter? This chapter answers both with the library's solver, `fluctua`, and its pathwise adjoint. After it you can choose a scheme with a reason, read its convergence from a table, and compute the sensitivity of a payoff to a parameter in one backward sweep.

## The mental model

`fluctua(mu, sigma, y0, t0, t1, theta, options)` advances `dY = μ(t, Y, θ) dt + σ(t, Y, θ) dW` from `y0` over `options.steps` steps and returns the state at `t1`, with the realized `W(t1) − W(t0)`. `y0` has the batch shape: every entry is an independent path, and the whole batch is stepped in one call.

| Scheme | Reads | Strong order | Cost per step |
|---|---|---|---|
| `EulerMaruyama` | `μ`, `σ` | ½ | one evaluation |
| `Milstein` | `μ`, `σ`, and `σσ'` (by autodiff) | 1 | one evaluation plus a derivative |
| `StratonovichHeun` | the Stratonovich drift and `σ` | 1 | two evaluations |

Strong order measures the error of the path itself, `E|X_T − X̂_T|`, rather than of an average over paths. Halving the step divides an order-½ error by about 1.4 and an order-1 error by 2. `StratonovichHeun` solves the Stratonovich form of the equation, so it must be given the converted drift; chapter 2 shows the conversion.

The adjoint, `fluctua_adjoint`, runs the simulation, stores the realized noise, and sweeps backward through the same steps with the noise held fixed. Holding the noise fixed is what makes the result a pathwise derivative: `d loss / d y0` for every path, in one sweep, however many parameters there are.

## A guided tour

- [Convergence on geometric Brownian motion](stochastic_04_simulating_sdes/convergence_on_geometric_brownian_motion.md)
- [Pathwise delta](stochastic_04_simulating_sdes/pathwise_delta.md)

## What to remember

- Pick the scheme by what you can afford: Euler-Maruyama needs only the coefficients; Milstein needs `σσ'` too and converges at order 1 rather than ½; Heun solves the Stratonovich form.
- Measure convergence on a case with a closed form before trusting a scheme on one without.
- The adjoint holds the noise fixed, so it gives the derivative of the path, which is right for smooth payoffs and silent for payoffs with jumps. Chapter 5 shows what to do about those.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

stochastic_04_simulating_sdes/convergence_on_geometric_brownian_motion
stochastic_04_simulating_sdes/pathwise_delta
```
<!-- apiref:toctree:end -->
