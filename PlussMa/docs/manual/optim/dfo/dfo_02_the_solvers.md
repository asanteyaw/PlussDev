<!-- generated from dfo_02_the_solvers.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Derivative-Free Optimization 2: Choosing a Solver

There is no best derivative-free method; there is a best method for a problem's shape. After this chapter you can say what each of the seven main solvers does, read a comparison table for what it shows and what it does not, and use restarts or a population method when a problem has many local minima.

Every program is complete and its output is the output of that program. Counts are evaluations of the objective; no running times are claimed.

## The methods in one paragraph each

**Nelder–Mead** (`NelderMead`) keeps a simplex of n+1 points and replaces the worst by reflecting it through the others, stretching when that helps, shrinking when nothing does. Few evaluations on smooth low-dimensional problems; a local method, and unreliable beyond a dozen variables or on noisy functions.

**Pattern search** (`PatternSearch`) polls the 2n points ±step along a set of directions around the best point, moves if one improves, and halves the step if none does. It is the one method here that also handles nonlinear constraints (Chapter 3).

**Mesh adaptive direct search** (`OrthoMads`, `LtMads`, `MadsAdvanced`) is pattern search with a changing set of poll directions. `OrthoMads` uses a different orthogonal basis at each iteration, so over time the polls cover every direction rather than only the coordinate axes; `LtMads` builds random lower-triangular bases. `MadsAdvanced` adds a search step, a cache, a surrogate, constraint handling and multiple objectives, and is what `Auto` chooses for black boxes (Chapter 4).

**Differential evolution** (`DifferentialEvolution`, `DifferentialEvolutionPrmm`) keeps a population; each member is challenged by a trial point built from differences of other members, and replaced if the trial is better. The PRMM variant adapts its mutation and crossover rates as it runs and keeps a small elite of best members. Good on multimodal functions; spends many evaluations.

**Particle swarm** (`ParticleSwarm`, `ParticleSwarmDv`) moves a swarm, each particle pulled toward its own best and the swarm's best. The `Dv` variant lets the inertia decay over the run, mixes in crossover between particles, and gives up on a particle that stagnates for a set number of iterations. Scales to higher dimensions; also spends many evaluations.

`NelderMead` is the classic simplex algorithm: with no bounds it runs unmodified, with the usual starting simplex (5% steps, 0.00025 for zero entries), tolerances of 1e-4 and budgets of 200 times the number of variables. Program 2 below calls it with no bounds (Program 1 gives every method a box, so that all seven can be compared on equal terms). Bounds are an optional addition: when you supply them, every point is clamped into the box before the objective is evaluated. Nonlinear constraints are not supported by this method and are rejected with an error.

## Programs

- [Comparing the methods](dfo_02_the_solvers/comparing_the_methods.md)

## Choosing

| If the problem is... | Start with |
|---|---|
| smooth, few variables, cheap to evaluate | `NelderMead` |
| smooth, you need a certificate of local convergence on a mesh | `PatternSearch` or `OrthoMads` |
| non-smooth or with plateaus | `PatternSearch` or `OrthoMads` |
| multimodal and a global answer matters | `DifferentialEvolutionPrmm`, or `ParticleSwarm` for many variables |
| bounded, moderate dimension, unknown shape | `DifferentialEvolution` |
| constrained | `PatternSearch` (Chapter 3) |
| a black box that can fail or has several outputs | `MadsAdvanced` (Chapter 4) |

If you can differentiate the function, use the gradient-based solvers of the earlier chapters; they will take far fewer evaluations than anything in this chapter.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

dfo_02_the_solvers/comparing_the_methods
```
<!-- apiref:toctree:end -->
