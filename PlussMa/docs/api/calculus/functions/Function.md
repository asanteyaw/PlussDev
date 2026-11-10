<!-- generated from Function.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `math::Function`

A formula in named variables, recorded once and evaluated on any `Series` data.

## Signature

```cpp
math::Function::Function() noexcept;
math::Function::Function(const math::Function &) noexcept;
math::Function::Function(math::Function &&) noexcept;
```

## Construction

A `Function` is created by [`math::function`](function_factory.md) (from a callable, or from a constant), by [`math::stack`](stack.md) (from several functions), or as the result of applying an [`operators::Operator`](../operators/Operator.md). The default constructor makes an *undefined* function that holds nothing; test for it with [`defined`](Function/defined.md). Copies share the same recorded formula and are cheap.

## State

A function is immutable. Every method that "changes" it, such as `assume`, `add` or `exp`, returns a new `Function` and leaves the receiver untouched.

## Methods

| Method | What it does |
|---|---|
| [`abs`](Function/abs.md) | The absolute value of a real function, or the modulus of a complex one. |
| [`add`](Function/add.md) | The sum of two functions, or of a function and a number, as a new function. |
| [`angle`](Function/angle.md) | The argument (phase angle) of a complex function, in radians in `(−π, π]`. |
| [`arg`](Function/arg.md) | Returns one of the function's own variables as an expression, so it can be combined with the function after the function was built. |
| [`arity`](Function/arity.md) | The number of variables the function takes. |
| [`assume`](Function/assume.md) | Declares what is known about a variable, and returns a new function simplified by that knowledge. |
| [`assumption`](Function/assumption.md) | Reads back what has been declared about one variable with `assume`. |
| [`call`](Function/call.md) | The call operator, `f(...)`: evaluates the function with a scalar, with a list of `Series` in declaration order, or with named bindings in any order. |
| [`compose`](Function/compose.md) | Substitutes functions for the variables: `f.compose({g, h})` is `f(g, h)`. |
| [`conj`](Function/conj.md) | The complex conjugate of a function: the same real part and the opposite imaginary part. |
| [`contract`](Function/contract.md) | Sums the product of two tensor functions over chosen pairs of output axes. |
| [`cos`](Function/cos.md) | The cosine of a function (in radians), as a new function. |
| [`cosh`](Function/cosh.md) | The hyperbolic cosine of a function, as a new function. |
| [`defined`](Function/defined.md) | Reports whether a `Function` holds a formula, or is the empty default-constructed value. |
| [`det`](Function/det.md) | The determinant of a square matrix function, as a scalar function. |
| [`div`](Function/div.md) | The quotient of two functions, or of a function and a number, as a new function. |
| [`exp`](Function/exp.md) | The exponential `eˣ` of a function, as a new function. |
| [`expm1`](Function/expm1.md) | `eˣ − 1` of a function, computed accurately when `x` is small. |
| [`imag`](Function/imag.md) | The imaginary part of a function. |
| [`inverse`](Function/inverse.md) | The matrix inverse of a square matrix function. |
| [`log`](Function/log.md) | The natural logarithm of a function, as a new function. |
| [`log1p`](Function/log1p.md) | `ln(1 + x)` of a function, computed accurately when `x` is small. |
| [`matmul`](Function/matmul.md) | The matrix product: contracts the last output axis of the receiver with the first output axis of the argument. |
| [`mul`](Function/mul.md) | The product of two functions, or of a function and a number, as a new function. |
| [`negate`](Function/negate.md) | The unary minus, `operator-`: the negative of a function, `−f`. |
| [`pow`](Function/pow.md) | Raises a function to a power, where the power is a number or another function. |
| [`provenance`](Function/provenance.md) | Reports how a function's formula is known: exactly, as an unopened callable, through a lazy integral, or through automatic differentiation. |
| [`rank`](Function/rank.md) | The number of output axes of one evaluation: 0 for a scalar, 1 for a vector, 2 for a matrix, and so on. |
| [`real`](Function/real.md) | The real part of a function. |
| [`select`](Function/select.md) | Picks one index along an output axis, removing that axis. |
| [`shape`](Function/shape.md) | The length of each output axis of one evaluation, as a list of integers. |
| [`sin`](Function/sin.md) | The sine of a function (in radians), as a new function. |
| [`sinh`](Function/sinh.md) | The hyperbolic sine of a function, as a new function. |
| [`sqrt`](Function/sqrt.md) | The square root of a function, as a new function. |
| [`sub`](Function/sub.md) | The difference of two functions, or of a function and a number, as a new function. |
| [`tan`](Function/tan.md) | The tangent of a function (in radians), as a new function. |
| [`tanh`](Function/tanh.md) | The hyperbolic tangent of a function, as a new function. |
| [`trace`](Function/trace.md) | The trace of a square matrix function: the sum of its diagonal, as a scalar function. |
| [`transpose`](Function/transpose.md) | Swaps two output axes: the last two by default, or any two you name. |
| [`variables`](Function/variables.md) | The names of the function's variables, in the order a by-position call binds them. |

## Changelog

- **0.1**: initial documented surface.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Function/Introduction
Function/abs
Function/add
Function/angle
Function/arg
Function/arity
Function/assume
Function/assumption
Function/call
Function/compose
Function/conj
Function/contract
Function/cos
Function/cosh
Function/defined
Function/det
Function/div
Function/exp
Function/expm1
Function/imag
Function/inverse
Function/log
Function/log1p
Function/matmul
Function/mul
Function/negate
Function/pow
Function/provenance
Function/rank
Function/real
Function/select
Function/shape
Function/sin
Function/sinh
Function/sqrt
Function/sub
Function/tan
Function/tanh
Function/trace
Function/transpose
Function/variables
```
<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="function_factory.html">
    <span class="label">Next</span>
    <span class="title">function</span>
  </a>
</div>
<!-- apiref:nav:end -->
