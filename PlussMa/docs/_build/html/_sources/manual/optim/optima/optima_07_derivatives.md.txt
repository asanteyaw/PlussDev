<!-- generated from optima_07_derivatives.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Optima 7: Derivatives

Every algorithm in this guide follows gradients. The objective's gradient says which way is downhill, the constraints' Jacobians say how the constraints move, and the exact curvature models need second derivatives. This chapter is about where those derivatives come from. After it you can supply them in the form that suits your function, check a hand-written derivative before it costs you a wrong answer, call the derivative tools on their own, count what each choice costs, and choose `fwd_objective` for the long loops where a tape is a burden.

Every program is complete and its output is the output of that program.

## The mental model

You write the function; the library supplies the derivative, or you supply it yourself. There are three ways to hand over an objective, and the constraints have the same choices.

| Form | You write | The derivative comes from | Use it when |
|---|---|---|---|
| `autodiff` | the function, on `Series` values | reverse mode, recorded on a tape; differentiated again for exact curvature | the default: any function, any number of variables |
| `analytic` | the value and the gradient | you | the derivative is simple, or already known |
| `fwd_objective` | the function, on `Fwd<N>` values | forward mode in plain doubles | a long scalar loop with few parameters |

The exact curvature models of chapter 6 differentiate the `autodiff` function a second time, so the one function serves both gradients and curvature. `analytic` and `fwd_objective` fill the same slot, `objective.analytic`; when `analytic` is set it takes precedence over `autodiff` for the gradient.

All of these describe the function and its derivative at a point. None of them changes what the algorithm does with them, so a correct derivative in any form gives the same run.

## Supplying derivatives

- [Automatic, analytic and audited derivatives](optima_07_derivatives/automatic_analytic_and_audited_derivatives.md)
- [Derivative tools, call counts and long loops](optima_07_derivatives/derivative_tools_and_costs.md)

## What to remember

- Three forms hand over an objective: `autodiff`, `analytic`, `fwd_objective`. The algorithms do not care which; the same derivative gives the same run.
- Write `autodiff` once, on plain `Series`; exact curvature needs no second callback.
- Audit every hand-written derivative with `audit_optima_derivatives`. A wrong gradient does not fail loudly: it shows up as `NoProgress`.
- An analytic constraint returns Jacobian blocks of shape `[constraints, block size]`, and may leave a residual kind it does not have undefined.
- `grad`, `jacrev`, `jacfwd`, `jvp` and `hvp` are available on their own.
- Exact curvature costs one Hessian-vector product per variable per Hessian.
- A long scalar loop with few parameters: `Fwd<N>` avoids the tape. Many parameters: use `autodiff`.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

optima_07_derivatives/automatic_analytic_and_audited_derivatives
optima_07_derivatives/derivative_tools_and_costs
```
<!-- apiref:toctree:end -->
