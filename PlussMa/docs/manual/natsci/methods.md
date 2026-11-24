# Methods

The methods shared by the biology, chemistry and physics models: how to tell whether a steady state attracts, how steady states appear and split when a parameter changes, how to fit a model to measurements and judge what the measurements can determine, and how to carry uncertainties through calculations. Each chapter applies the method to models from the other guides.

1. **Does a steady state attract, and where does that change?** Counting unstable directions, and following equilibria through a parameter past folds, oscillation onsets and branch points.
2. **How well do the data determine the parameters?** Least-squares fits with intervals, profile likelihood, and sensitivities that tell what to measure.
3. **What is the uncertainty of the result?** Measurements with standard uncertainties, correlated measurements, and the drift of an invariant along a simulated path.
4. **What happens when there are only a few molecules?** Gillespie ensembles for networks, epidemics and populations.
5. **How does a distribution over size evolve?** Growth, nucleation, aggregation and breakage.
6. **How do you integrate fast and slow processes together?** A stiff integrator and the state at chosen times.
7. **How do you differentiate and solve on a grid?** Periodic operators, boundary rules and Poisson solves.

Chapters, in reading order:

- [Methods 1: Stability and Branches](methods/meth_01_stability.md)
- [Methods 2: Fitting and Identifiability](methods/meth_02_fitting.md)
- [Methods 3: Measurement and Uncertainty](methods/meth_03_uncertainty.md)
- [Methods 4: Stochastic Kinetics](methods/meth_04_stochastic_kinetics.md)
- [Methods 5: Population Balances](methods/meth_05_population_balances.md)
- [Methods 6: Stiff Systems](methods/meth_06_stiff_systems.md)
- [Methods 7: Grids and Operators](methods/meth_07_grids_and_operators.md)

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

methods/meth_01_stability
methods/meth_02_fitting
methods/meth_03_uncertainty
methods/meth_04_stochastic_kinetics
methods/meth_05_population_balances
methods/meth_06_stiff_systems
methods/meth_07_grids_and_operators
```
<!-- apiref:toctree:end -->
