<!-- generated from optima_05_working_set_and_interior.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Optima 5: The Working-Set and Interior Methods

Chapter 4 opened sequential QP. This chapter opens the other two quadratic-model algorithms. They share its machinery (a model of the problem, a quadratic subproblem, a line search) but differ in how they treat inequality constraints, and that difference decides what each one is good for. After it you can read a trace of either, name what its columns mean, and choose between the three.

Every program is complete and its output is the output of that program.

## The mental model

An inequality constraint is either pushing on the answer (active) or irrelevant (inactive). The three algorithms handle that fact differently.

| Method | What it keeps | How it treats an inequality |
|---|---|---|
| `SequentialQP` | every constraint, linearized | all rows enter the quadratic subproblem; the subproblem decides which bind |
| `WorkingSet` | a list of the constraints it currently treats as active | only rows in the list are enforced in the step; a row is added when the step would violate it and dropped when its multiplier turns the wrong way |
| `InternalProtocol` | every constraint, through a barrier | each inequality gets a slack variable kept strictly positive, and a barrier term of strength `μ` keeps iterates away from the boundary; `μ` is driven toward zero |

Two consequences matter in practice. The interior method never lets a bound or a linear row become active until the very end, so every iterate satisfies them strictly. The working-set method *does* let constraints become active, and it ends with an explicit active set that you can read off the multipliers.

## A guided tour: the working-set method

- [The working-set method](optima_05_working_set_and_interior/the_working_set_method.md)
- [The interior-point method](optima_05_working_set_and_interior/the_interior_point_method.md)

## Decisions

| Situation | Choice |
|---|---|
| Smooth problem, a handful of constraints | `SequentialQP`, the default |
| You want the active set and the multipliers explicitly | `WorkingSet`, then read `multipliers` |
| Thousands of inequality rows | `SequentialQP` needed the fewest iterations in Program 2 (2 against 4 and 14 at 2000 rows); all three are workable |
| Objective undefined outside bounds or the feasible region | `InternalProtocol` |
| A start on a curved constraint boundary | `InternalProtocol` with the default independent slacks |

## Pitfalls

- Choosing the working-set method to need fewer iterations. On the problems here it follows sequential QP's path in as many or more iterations; it is not a cheaper variant.
- Starting the interior method with a tiny barrier parameter hoping to need fewer iterations. It needs more (Program 5).

## Where next

Chapter 6 turns to what all three share: the curvature model (BFGS, limited memory, exact Hessians), the linear algebra of each step, and scaling.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

optima_05_working_set_and_interior/the_working_set_method
optima_05_working_set_and_interior/the_interior_point_method
```
<!-- apiref:toctree:end -->
