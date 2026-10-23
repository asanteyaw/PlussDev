<!-- generated from septima_04_lsr1_trust_region.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Septima 4: The L-SR1 Trust-Region Optimizer

Adam and L-BFGS, the two optimizers of `septima`, both take a step and then see how it went. A trust-region method works the other way round: it first decides how far it is willing to trust a model of the function, then takes the best step inside that distance, and finally compares what the model predicted with what the function did. The third optimizer of the family, `l_sr1_trust_region`, is a trust-region method built on a limited-memory SR1 model of the curvature. After this chapter you know what it does that L-BFGS cannot, where it is the better choice, what its settings mean, and what it cannot do.

Every program is complete and its output is the output of that program.

## The mental model

Near a point, a smooth function looks like a quadratic: its value, its gradient, and a matrix of curvature. L-BFGS builds the curvature matrix from recent steps in a way that keeps it **positive-definite**, so its model is always a bowl that curves up in every direction. That is right near a minimum, and wrong near a saddle, where the function curves down in some direction. An SR1 model drops the restriction. It can represent a matrix with negative curvature, so its model knows that there is a direction in which the function falls, and a trust-region step can follow it.

The trust region is the safety. The model is trusted only within a ball whose radius the method adjusts. If the function decreased as much as the model predicted, the ball grows; if it decreased much less, or increased, the ball shrinks and the step is retried. The method therefore never needs a line search, and a bad model cannot send it far.

| | Adam | L-BFGS | L-SR1 trust region |
|---|---|---|---|
| Call | `septima` | `septima` | `l_sr1_trust_region` |
| Uses curvature | no | yes, positive-definite | yes, can be indefinite |
| Step control | learning rate | learning rate or line search | trust-region radius |
| Constraints and bounds | yes (chapter 2) | yes (chapter 2) | **no** |
| Complex parameters | yes | yes | yes |

The call takes a `Trace` objective and a starting `Series`, and returns an `Lsr1Result`:

```cpp
auto r = optim::l_sr1_trust_region(objective, x0);          // one Series
auto [params, r] = optim::l_sr1_trust_region(objective_of_blocks, blocks);  // a list of blocks
```

## Programs

- [A first run, and a comparison](septima_04_lsr1_trust_region/a_first_run_and_a_comparison.md)
- [Saddles](septima_04_lsr1_trust_region/saddles.md)
- [The settings that matter](septima_04_lsr1_trust_region/the_settings_that_matter.md)
- [Several blocks, and a complex parameter](septima_04_lsr1_trust_region/several_blocks_and_a_complex_parameter.md)

## Settings

| Setting | Meaning | Default |
|---|---|---|
| `step.memory_size` | step pairs kept in the curvature model (never more than the number of variables) | 10 |
| `step.initial_radius`, `step.max_radius` | starting and largest trust-region radius | 1, 1e8 |
| `step.initial_gamma`, `step.gamma_shrink` | the initial scale of the curvature model, and how conservatively it is rescaled | 1, 0.9 |
| `step.sr1_skip_tolerance` | a step pair that would make the update unstable is skipped | 1e-8 |
| `step.eta_1` ... `step.eta_4`, `step.tau_2`, `step.tau_3` | the thresholds on how well the model predicted the decrease, which decide when the radius shrinks or grows | 0.25, 0.5, 0.75, 2 and 0.1, 0.75 |
| `step.wolfe_c1`, `step.max_line_search_iterations` | sufficient decrease on the accepted step, and the cap on shortenings | 1e-4, 20 |
| `max_iterations` | the budget | 200 |
| `gradient_tolerance` | convergence on the 2-norm of the gradient | 1e-6 |
| `step_tolerance`, `max_no_progress_iterations` | the test for `NoProgress` | 1e-12, 25 |
| `progress_hook` | called each iteration with the objective, gradient norm, radius and step length; return `false` to stop | none |

The exit flags are the same as Septima's: `Converged`, `MaxIterations`, `Stopped`, `NoProgress`, `NonFiniteEvaluation`, `InvalidProblem`. Leave everything but `memory_size`, `initial_radius` and the tolerances alone unless you have measured a reason.

## What it cannot do

L-SR1 has no way to state a bound, an equality or an inequality: the call takes only an objective and a starting point. If your problem has constraints, use `septima` with Adam, or L-BFGS with the strong-Wolfe line search (chapter 2), or Optima. If you need an unconstrained answer and the parameters are bounded in practice only by the objective itself, L-SR1 is the best of the three at accuracy.

## What to remember

- L-SR1 is a trust-region optimizer: it chooses how far to trust a quadratic model, steps inside that distance, and adapts the distance to how well the model predicted.
- Its curvature model can be indefinite, so it can leave a saddle where L-BFGS stalls beside it.
- On smooth unconstrained problems it reaches a more accurate answer than L-BFGS, at more function evaluations per iteration.
- It needs a memory of about 10 or more. A radius that keeps shrinking points at too small a memory.
- A first-order test cannot tell a saddle from a minimum. Run from more than one start and compare objective values.
- It handles blocks, complex parameters and all the dtypes Septima does. It does not handle constraints or bounds.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

septima_04_lsr1_trust_region/a_first_run_and_a_comparison
septima_04_lsr1_trust_region/saddles
septima_04_lsr1_trust_region/the_settings_that_matter
septima_04_lsr1_trust_region/several_blocks_and_a_complex_parameter
```
<!-- apiref:toctree:end -->
