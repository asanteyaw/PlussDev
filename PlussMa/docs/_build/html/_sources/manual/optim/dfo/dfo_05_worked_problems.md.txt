<!-- generated from dfo_05_worked_problems.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Derivative-Free Optimization 5: Worked Problems and Checking Answers

The earlier chapters introduced the pieces. This one puts them together on two problems and ends with a routine for deciding whether an answer can be trusted, which matters more here than with gradient methods because a derivative-free solver gives no optimality certificate.

Every program is complete and its output is the output of that program.

## Programs

- [A fitted curve, and how to check an answer](dfo_05_worked_problems/a_fit_and_how_to_check_it.md)

## A short guide to the whole section

- Gradient available and the function smooth: use the gradient-based solvers; they will need far fewer evaluations.
- Smooth, few variables, no constraints: `Auto` (Nelder–Mead), then polish with `OrthoMads` or `PatternSearch`.
- Many local minima: a population method (`DifferentialEvolutionPrmm`, `ParticleSwarm`), then polish locally.
- Constraints: `PatternSearch`; equalities only there.
- Failures, several objectives, or an expensive deterministic function: `blackbox` and `MadsAdvanced` with the cache on.
- Always: count evaluations, fix the seed when reporting, and verify the answer from outside.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

dfo_05_worked_problems/a_fit_and_how_to_check_it
```
<!-- apiref:toctree:end -->
