<!-- generated from calculus_06_worked_problems.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Calculus 6: Worked Problems

The first five chapters introduced the pieces. This chapter uses them together on problems whose answers can be checked, and ends with the two properties that matter in production code: precision and concurrency. Each problem is a complete program, built almost entirely from the operators of Chapters 2 to 5.

Every program is complete and its output is the output of that program.

## The problems

| Problem | Operators used | Check |
|---|---|---|
| Cumulants of seven distributions from their generating functions | `differential` of order 1 to 4 | the known cumulants |
| A Taylor polynomial and its error | `differential` of orders 1 to 6 | the known series of exp(sin x) |
| The sensitivity of a probability to its standard deviation | `integral`, `differential` | the error function and its derivative |
| Precision and concurrency | `differential`, evaluation on `Float32` and from threads | a `Float64` reference and a direct computation |

## Programs

- [Cumulants from a generating function](calculus_06_worked_problems/cumulants_from_a_generating_function.md)
- [A Taylor polynomial and its error](calculus_06_worked_problems/taylor_polynomials_from_derivatives.md)
- [The sensitivity of a probability](calculus_06_worked_problems/sensitivity_of_a_probability.md)
- [Precision and concurrency](calculus_06_worked_problems/precision_and_concurrency.md)

## Checklist

1. Write the model once as a `Function`, and get every derived quantity (cumulants, coefficients, sensitivities) from operators.
2. Check a derived quantity against a known case before trusting it on an unknown one.
3. Give model parameters as variables, so a family of models is one call.
4. Evaluate in the precision the application needs; compare `Float32` against `Float64` when it matters.
5. Share one `Function` among threads freely; do not copy it to avoid locks.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

calculus_06_worked_problems/cumulants_from_a_generating_function
calculus_06_worked_problems/taylor_polynomials_from_derivatives
calculus_06_worked_problems/sensitivity_of_a_probability
calculus_06_worked_problems/precision_and_concurrency
```
<!-- apiref:toctree:end -->
