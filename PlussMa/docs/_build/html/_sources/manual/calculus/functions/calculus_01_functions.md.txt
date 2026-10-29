<!-- generated from calculus_01_functions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Calculus 1: Functions

A formula is the thing calculus acts on, and in most code a formula is trapped inside a C++ function that can only be called. `pluss::math::Function` makes it a value: you write the formula once, Pluss records its structure, and from then on the same object can be evaluated on any data, differentiated, integrated, combined with others, transformed by operators, and saved. After this chapter you can build a function of several variables, evaluate it by position or by name on scalars, vectors and grids, state what you know about its variables so that it simplifies, predict which inputs it rejects, and wrap existing `Series` code when the formula cannot be restated.

Every program is complete and its output is the output of that program. Values are printed with `OutputEngine` and shown to the digits that are meaningful for the point being made.

## The mental model

The header is `<pluss/ma.h>`; the types are `pluss::math::Function` and, from Chapter 5, `pluss::operators::Operator`. The programs use the aliases `namespace math = pluss::math;` and `namespace operators = pluss::operators;`.

- **A `Function` is an immutable value.** Copies are cheap and share the same recorded formula; nothing you do to a copy, or to the arrays you evaluated it on, changes it. It can be shared between threads.
- **It is built by tracing.** `math::function({"x", "y"}, callable)` runs `callable` once with placeholder `Function`s for `x` and `y`, and records the operations it performs. The formula is the graph of those operations, not the C++ code.
- **It is evaluated on `Series`.** `f({x, y})` or `f({{"y", y}, {"x", x}})` replays the graph with real arrays. Shapes broadcast, the floating-point precision and the platform of the arguments are kept, and the result is a `Series`.
- **It preserves real or complex Series values.** Real inputs receive strict real-domain checks; complex inputs use principal branches and reject their singular points.

| Entry point | What it does |
|---|---|
| `math::function(variables, callable)` | traces a callable into a `Function` |
| `math::function(double)`, `math::function(Series)` | a constant |
| `f(...)` | evaluates by position or by name |
| `f.arity()`, `f.variables()`, `f.rank()`, `f.shape()` | describes the function |
| `f.arg("x")` | the coordinate `x` of `f` as a function |
| `f.assume("x", assumption)`, `f.assumption("x")` | states and reads what is known about a variable |
| `f.provenance()` | how the function was obtained |
| `+`, `-`, `*`, `/`, `add`, `sub`, `mul`, `div`, `pow`, `exp`, `log`, `log1p`, `expm1`, `sqrt`, `sin`, `cos`, `tan`, `sinh`, `cosh`, `tanh`, `abs`, unary `-` | the expression vocabulary |

## Programs

- [Building and evaluating a function](calculus_01_functions/building_and_evaluating.md)
- [The expression vocabulary](calculus_01_functions/the_expression_vocabulary.md)
- [Simplification and assumptions](calculus_01_functions/simplification_and_assumptions.md)
- [Domains and errors](calculus_01_functions/domains_and_errors.md)
- [Functions from Series code](calculus_01_functions/functions_from_series_code.md)

## Checklist

1. Write the formula with `Function` arithmetic inside the callable; do not branch on values, because there are none while tracing.
2. Name each variable once; evaluate with every variable, by position or by name.
3. Name a sub-expression you use twice and reuse it: it is simplified and evaluated once.
4. Use `assume` only for facts that are true of the problem. It is a promise and the data is not checked against it.
5. Expect an error, not a `NaN`, from `log`, `log1p`, `sqrt` and division outside their domains; do not expect one from `pow`.
6. Use the `Function` vocabulary when you can. An opaque `Series` callable works but has no exact derivative and cannot be saved.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

calculus_01_functions/building_and_evaluating
calculus_01_functions/the_expression_vocabulary
calculus_01_functions/simplification_and_assumptions
calculus_01_functions/domains_and_errors
calculus_01_functions/functions_from_series_code
```
<!-- apiref:toctree:end -->
