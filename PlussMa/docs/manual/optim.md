# Optimization

Constrained and unconstrained optimization, linear and mixed-integer programming, derivative-free search and root finding. Looking up a particular function or option? See the {reference}`optimization API reference <optim>`.

## Choosing a solver

**A root of one equation.** [`roots`](optim/roots.md) finds where a scalar function crosses zero, by bracketing methods that cannot fail once a sign change is known, or by Newton-type steps from a start.

**A quadratic objective with linear constraints.** [`qp`](optim/qp.md) solves convex quadratic programs for one problem or a whole batch, with warm starts and a prepared solver for repeated solves.

**A linear objective with linear constraints.** [`linprog`](optim/linprog.md) solves linear programs by an interior-point method or the exact simplex, for one problem or thousands at once, and returns certificates: dual values, a proof of infeasibility, or a ray of unboundedness.

**Integer decisions.** [`milp`](optim/milp.md) adds integrality to a linear program by branch-and-bound on the batched solver.

**A smooth nonlinear objective with any mix of constraints.** [`optima`](optim/optima.md) takes an objective written with ordinary `Series` arithmetic, differentiated automatically (or supplied with its gradient), with bounds, linear rows and nonlinear equality and inequality constraints, and runs one of four algorithms.

**A smooth objective, first-order or quasi-Newton methods.** [`septima`](optim/septima.md) minimises with Adam or limited-memory BFGS, with bounds, equalities and inequalities through an augmented Lagrangian; `l_sr1_trust_region` adds limited-memory SR1 with a trust region, which follows negative curvature past saddles. The Adam step is also available on its own, for training loops you write yourself.

**No derivative, or one you cannot trust.** [`dfo`](optim/dfo.md) minimises with objective values only: the front door `solve_dfo` / `DFO` chooses a method from the problem, or you name a Nelder-Mead simplex, pattern search, differential evolution, a particle swarm or a mesh-adaptive search. Pattern search and the mesh-adaptive methods take bounds and nonlinear constraints, and the black-box form handles failed evaluations, several objectives, a cache and a surrogate. Use it when the objective is non-smooth, noisy, a simulation, or has local minima that a gradient method would settle in.

## The chapters

- [optima](optim/optima.md)
- [septima](optim/septima.md)
- [linprog](optim/linprog.md)
- [milp](optim/milp.md)
- [dfo](optim/dfo.md)
- [roots](optim/roots.md)
- [qp](optim/qp.md)

```{toctree}
:maxdepth: 3
:hidden:

optim/optima
optim/septima
optim/linprog
optim/milp
optim/dfo
optim/roots
optim/qp
```
