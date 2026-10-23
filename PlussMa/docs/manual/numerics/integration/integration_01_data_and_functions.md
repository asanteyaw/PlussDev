<!-- generated from integration_01_data_and_functions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Integration 1: Data and Functions

An integral adds up a quantity over an interval. Pluss computes it two ways: from values you already have (`trapezoid`), and from a function you can call (`quadra`). After this chapter you can integrate sampled data, integrate a function to a stated accuracy, choose among the rules, and tell a converged answer from one that is not.

Every program is complete and its output is the output of that program. Evaluation counts are shown because they are the same on every machine; no running times are claimed.

## The mental model

The module is `pluss::subsp::math::integ`. Its two ideas:

- **The integrand takes a Series of points and returns a Series of values** (`IntegrandFn`, a `std::function<Series(const Series&)>`). The library calls it with a whole grid of nodes at once, never point by point, so the integrand is ordinary array arithmetic and runs on the fast backends.
- **Integrals are differentiable.** They are built from ordinary differentiable operations, so a parameter the integrand captures receives its derivative (Chapter 2).

| Entry point | Integrates |
|---|---|
| `integ::trapezoid(y, x)`, `trapezoid(y, dx)` | samples `y` at points `x` (or equal spacing `dx`); last axis by default |
| `integ::cumulative_trapezoid(y, x)` | the running integral of samples |
| `integ::quadra(f, a, b, options)` | a function over `[a, b]` by a composite rule |

## Programs

- [Integrating data](integration_01_data_and_functions/integrating_data.md)
- [Integrating a function](integration_01_data_and_functions/integrating_a_function.md)
- [The rules](integration_01_data_and_functions/the_rules.md)
- [Tolerance and honest failure](integration_01_data_and_functions/tolerance_and_honest_failure.md)

## Checklist

1. Data you have: `trapezoid`. A function you can call: `quadra`.
2. Write the integrand as array arithmetic on its argument; do not loop over points.
3. Leave the rule at its default for smooth integrands; use `GaussLegendre` for the fewest evaluations.
4. Split the interval at kinks and jumps.
5. Always read `converged` and `error_estimate`.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

integration_01_data_and_functions/integrating_data
integration_01_data_and_functions/integrating_a_function
integration_01_data_and_functions/the_rules
integration_01_data_and_functions/tolerance_and_honest_failure
```
<!-- apiref:toctree:end -->
