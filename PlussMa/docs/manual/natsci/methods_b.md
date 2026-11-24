# Methods: Simulation and Grids

The methods that sit underneath the field and reaction models: simulating chance one event at a time, following size distributions, integrating stiff systems, and differentiating and solving on grids. This landing page lists the chapters; the lead merges them into the Methods page.

1. **What happens when there are only a few molecules?** Gillespie ensembles for reaction networks, epidemics and populations, run as thousands of replicates in one call.
2. **How does a distribution over size evolve?** Growth, nucleation, outflow, aggregation and breakage, with the number and the mass tracked exactly.
3. **How do I integrate a system with fast and slow processes?** A stiff integrator with a step set by accuracy, and the state at a list of times.
4. **How do I differentiate and solve on a grid?** Gradients, divergences, curls and Laplacians on periodic grids and boxes, boundary rules, and Poisson solves.

Chapters, in reading order:

- [Methods 11: Stochastic Kinetics](methods/meth_11_stochastic_kinetics.md)
- [Methods 12: Population Balances](methods/meth_12_population_balances.md)
- [Methods 13: Stiff Systems](methods/meth_13_stiff_systems.md)
- [Methods 14: Grids and Operators](methods/meth_14_grids_and_operators.md)

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

methods/meth_11_stochastic_kinetics
methods/meth_12_population_balances
methods/meth_13_stiff_systems
methods/meth_14_grids_and_operators
```
<!-- apiref:toctree:end -->
