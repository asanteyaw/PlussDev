<!-- generated from functions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Functions

A `math::Function` is a formula written once and evaluated on any data. This family documents the value type itself, the two functions that build one, the arithmetic and tensor operations on it, and the accessors that report what a function is.

## The Surface

| Symbol | What it is |
|---|---|
| [`Function`](functions/Function.md) | A formula in named variables, recorded once and evaluated on any `Series` data. |
| [`function`](functions/function_factory.md) | Creates a `Function`: from a callable (which is traced once), or from a constant. |
| [`operator_add`](functions/operator_add.md) | The infix sum: a function plus a function, a real number or a complex number, in either order. |
| [`operator_div`](functions/operator_div.md) | The infix quotient: a function divided by a function, a real number or a complex number, or a number divided by a function. |
| [`operator_mul`](functions/operator_mul.md) | The infix product: a function times a function, a real number or a complex number, in either order. |
| [`operator_sub`](functions/operator_sub.md) | The infix difference: a function minus a function, a real number or a complex number, in either order. |
| [`stack`](functions/stack.md) | Assembles a vector- or matrix-valued function from several functions, adding a trailing output axis. |

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

functions/Function
functions/function_factory
functions/operator_add
functions/operator_div
functions/operator_mul
functions/operator_sub
functions/stack
```
<!-- apiref:toctree:end -->
