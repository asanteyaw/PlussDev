<!-- generated from septima_05_choosing_and_worked_problems.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Septima 5: Choosing a Solver, and Worked Problems

Four chapters introduced Septima's two optimizers, its constraint handling and the L-SR1 trust-region optimizer, and chapter 2 showed one problem on which Septima and Optima agree but differ greatly in cost. This chapter puts them side by side on the problems each is good for. After it you can choose between Adam, L-BFGS, L-SR1 and Optima for a problem you have not seen, and you have three complete examples to start from: a model fitted to data, a constrained least-squares problem checked against Optima, and one function minimized by every solver.

Every program is complete and its output is the output of that program.

## The mental model

The choice rests on four questions about the problem, in this order.

1. **Does it have constraints other than bounds?** Bounds are enforced exactly by Septima and by Optima. Equalities and inequalities are enforced by both, and the answers agree (chapter 2), but Optima holds them to a far tighter precision (its violation ended at 2e-10 where Septima's ends near its tolerance of 1e-6) and in far fewer evaluations. If the constraints are nonlinear and must hold to high precision, or the problem is small, use Optima.
2. **Is it smooth?** For a smooth objective, a method that uses curvature (L-BFGS, L-SR1, Optima) needs far fewer iterations than Adam.
3. **Could there be saddles, and do you need a very accurate answer?** L-SR1 handles indefinite curvature (chapter 4) and reaches the smallest gradient of the three first-order methods.
4. **How large is it?** Optima's default BFGS curvature model is a dense matrix (chapter 6), so its memory grows with the square of the number of parameters, and each step solves a quadratic program. Septima's methods store a few vectors per parameter and take a step from gradients alone. Optima's limited-memory model avoids the dense matrix too.

## Programs

- [Choosing a solver on worked problems](septima_05_choosing_and_worked_problems/choosing_a_solver.md)

## Choosing

| Your problem | Use | Why (chapter) |
|---|---|---|
| Few variables, nonlinear equalities or inequalities that must hold to high precision | **Optima** | 6 evaluations and a violation of 2e-10, against 242 to 854 evaluations and violations near 1e-7 (chapter 2) |
| Convex objective with bounds and a linear equality or two | Septima (or Optima) | the same answer as Optima (Program 2) |
| Nonlinear constraints, many parameters or only gradients available | Septima with L-BFGS | the same solution as Optima on the non-convex problem of chapter 2 |
| Smooth, unconstrained, a very accurate answer needed | **L-SR1** | smallest gradient of the first-order methods (Programs 1 and 3, chapter 4) |
| Smooth, unconstrained, a good answer is enough | **L-BFGS** | few iterations, no tuning (chapter 3) |
| A saddle could trap the run | **L-SR1** | leaves a saddle that L-BFGS stalls beside (chapter 4) |
| A noisy gradient, or nothing else converges | **Adam**, with a tuned learning rate | needs only a gradient (chapter 3) |
| Bounds only | Adam or L-BFGS | bounds are enforced exactly by projection (chapter 2) |
| Complex parameters | any of the three first-order methods | the real and imaginary parts are two coordinates (chapters 3 and 4) |

## A routine for a new problem

1. Write the objective on `Series`. If it has an undefined region, add bounds that keep the iterates inside it (chapter 3).
2. If it has nonlinear constraints that must hold accurately, use Optima and stop here.
3. Otherwise try L-BFGS first, then L-SR1 if you need more accuracy or suspect a saddle, then Adam with a tuned learning rate if the others misbehave.
4. Read the result by `objective_value`, `stationarity`, `step_inf_norm` and `constraint_violation`, not by the exit flag alone.
5. Check the answer by something the solver did not use: a closed form, a second optimizer, a second start (the routine of Optima chapter 8 applies unchanged).

## What to remember

- Constraints decide the solver. Nonlinear constraints that must hold accurately go to Optima; bounds and simple linear constraints suit Septima.
- Among the first-order methods, L-BFGS is the default, L-SR1 is the accurate and saddle-aware one, and Adam is the one that needs only a gradient.
- On a smooth, convex fit to data, the curvature methods converge in a fraction of the iterations Adam needs (7 and 33 against 190 in Program 1), and they all find the same answer.
- Compare solvers by evaluations and by the accuracy of the answer, not by iterations alone: L-SR1 and the line searches spend more than one evaluation per iteration.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

septima_05_choosing_and_worked_problems/choosing_a_solver
```
<!-- apiref:toctree:end -->
