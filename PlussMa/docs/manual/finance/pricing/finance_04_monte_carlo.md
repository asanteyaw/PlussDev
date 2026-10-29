<!-- generated from finance_04_monte_carlo.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Finance 4: Monte Carlo

Monte Carlo pricing simulates paths of the asset under the pricing measure and averages the discounted payoff. It is slower than the semi-analytic methods of the previous chapter for a European option under Heston, but it needs only a way of simulating the model, so it applies unchanged to models that have no characteristic function and to payoffs that depend on the whole path. This chapter simulates paths, prices by simulation, measures the error, and reduces it.

Every program is complete, and its output is the output of that program.

## The calls

| | Function |
|---|---|
| paths | `gbm_paths`, `heston_paths`, `bates_paths`, `sv_paths`, `heston_nandi_paths`, `sabr_paths`, `slv_paths`, `rough_heston_paths`, `rough_bergomi_paths` (namespace `volatility`, alias `vol`) |
| prices | `heston_price_mc`, `bates_price_mc`, `sv_price_mc`, `heston_nandi_price_mc`, `sabr_price_mc`, `slv_price_mc`, `rough_heston_price_mc`, `rough_bergomi_price_mc` (namespace `pricing`, alias `prc`) |
| controls | `vol::SimOptions` for paths, `prc::MonteCarloOptions` for prices |

## Programs

- [Simulated paths](finance_04_monte_carlo/simulated_paths.md)
- [Pricing by simulation](finance_04_monte_carlo/pricing_by_simulation.md)
- [Variance reduction](finance_04_monte_carlo/variance_reduction.md)
- [Quasi-random sequences](finance_04_monte_carlo/quasi_random_sequences.md)
- [Options and errors](finance_04_monte_carlo/options_and_errors.md)

## Checklist

1. Check a generator before using it: the mean of the terminal spot must be S₀ e^(μT).
2. A Monte Carlo price has no error bar of its own. Repeat with different seeds to measure one, and compare the spread with the bias from the number of steps.
3. The error falls as 1/√N: ten times the paths for three times the accuracy. Use variance reduction before more paths.
4. Importance sampling helps for options far from the money, and hurts if the shift points the wrong way or is too large.
5. Quasi-random sequences gain most with few steps; randomise them to get an error bar.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

finance_04_monte_carlo/options_and_errors
finance_04_monte_carlo/pricing_by_simulation
finance_04_monte_carlo/quasi_random_sequences
finance_04_monte_carlo/simulated_paths
finance_04_monte_carlo/variance_reduction
```
<!-- apiref:toctree:end -->
