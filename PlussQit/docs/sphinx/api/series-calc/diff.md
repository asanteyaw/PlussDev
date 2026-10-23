<!-- pluss-hand-authored -->

# Series::CalcAccessor::diff

[Back to Series::CalcAccessor API Reference](../../series_calc_accessor_api_reference.md)

## Related Accessor Methods

`x.calc.diff` is also an accessor with named methods for stochastic calculus:
`ito`, `strat`, `marcus`, `malliavin` and `skorokhod`. They are documented on their
own pages, linked under See Also. The call form on this page is unchanged.

## Signatures

```cpp
template<class Fn>
Series Series::CalcAccessor::diff(Fn&& fn) const

template<class Fn>
Series Series::CalcAccessor::diff(Fn&& fn, std::int64_t order) const
```

## What It Computes

`diff()` differentiates a callable at the owning `Series`. It evaluates `fn`
with a fresh differentiable copy of the receiver, follows the graph made by
ordinary `Series` operations, and returns the requested derivative. It does not
estimate a slope by perturbing input values, so there is no step-size option or
finite-difference truncation error.

The default order is one. Order zero evaluates `fn(x)` and returns its value.
For order greater than one, each reverse pass retains a differentiable graph for
the next pass. An operation on the path must therefore support the requested
derivative depth, not merely a first reverse rule.

This method is unrelated to root [`Series::diff()`](../series/diff.md), which
subtracts adjacent stored elements.

## Full Derivative Geometry

Let `fn(x)` have shape `Y` and `x` have shape `X`. In the default mode, order
`n` returns shape

```text
Y + X + X + ... + X
    \_____ n copies _____/
```

Output axes come first. Every derivative axis block uses the receiver's full
shape, not a flattened public shape. For vector-to-vector first derivatives,
this is the familiar Jacobian. For a scalar output at order two, it is the
Hessian. Higher-rank input and output axes remain visible, allowing callers to
contract or rearrange them without reconstructing indices from a flat matrix.

## Elementwise Geometry

Set `elementwise=true` when every output lane depends only on its corresponding
input lane and only the diagonal derivative is needed. The callback must return
the receiver's exact shape. The result keeps that shape at every order:

```cpp
pluss::Series::CalcAccessor::CalcOptions options;
options.elementwise = true;
auto diagonal = x.calc(options).diff(fn, 4);
```

This costs one reverse pass per order rather than materializing all combinations
of input lanes. Do not use it for coupled functions merely because input and
output shapes happen to match; off-diagonal dependencies would be deliberately
discarded.

## Dependency Policy

With the default `strict_dependency=false`, a callback result that genuinely
does not depend on the receiver contributes zeros. With
`strict_dependency=true`, a disconnected path throws and names the derivative
order at which dependency disappeared.

The higher-order engine separately tracks whether reverse results remain
differentiable. If an operation supports first order but detaches its reverse
result, a later order fails closed rather than being mistaken for zero.

## Examples

Example 1. A product of transcendental functions needs no hand-written product
or chain rule at the call site. The graph composes both rules through fourth
order.

```cpp
auto x = pluss::make_series(0.5);
auto fn = [](const pluss::Series& z) {
  return z.log().mul(z.asin());
};

std::cout << std::setprecision(10);
std::cout << "value=" << x.calc.diff(fn, 0).item<double>() << "\n";
std::cout << "first=" << x.calc.diff(fn, 1).item<double>() << "\n";
std::cout << "fourth=" << x.calc.diff(fn, 4).item<double>() << "\n";
```

### Output

```text
value=-0.3629310151
first=0.2468201286
fourth=19.8335025
```

Example 2. The default first derivative of a vector-valued function is the
full Jacobian. `sin()` is lane-local, so only the diagonal is nonzero.

```cpp
auto x = pluss::make_series({0.5, 1.0, 1.5});
auto jacobian = x.calc.diff(
    [](const pluss::Series& z) { return z.sin(); });

std::cout << jacobian << "\n";
```

### Output

```text
pluss::Series(shape=[3,3], strides=[3,1], dtype=float64, platform=cpu, numel=9, contiguous=true)
data=[[0.87758256, 0, 0],
  [0, 0.54030231, 0],
  [0, 0, 0.070737202]]
```

Example 3. Elementwise mode extracts that diagonal directly and keeps the
receiver shape. A fourth derivative of sine returns sine again.

```cpp
pluss::Series::CalcAccessor::CalcOptions options;
options.elementwise = true;

auto x = pluss::make_series({0.5, 1.0, 1.5});
auto sine = [](const pluss::Series& z) { return z.sin(); };

std::cout << "first=" << x.calc(options).diff(sine) << "\n";
std::cout << "fourth=" << x.calc(options).diff(sine, 4) << "\n";
```

### Output

```text
first=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.87758256, 0.54030231, 0.070737202]
fourth=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.47942554, 0.84147098, 0.99749499]
```

Example 4. Elementwise differentiation is rank-polymorphic. For `z^3`, the
second derivative is `6z` lane by lane.

```cpp
pluss::Series::CalcAccessor::CalcOptions options;
options.elementwise = true;

auto x = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
auto result = x.calc(options).diff(
    [](const pluss::Series& z) { return z.pow(3.0); }, 2);

std::cout << result << "\n";
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[6, 12],
  [18, 24]]
```

Example 5. A detached constant returns zeros under the default policy.

```cpp
pluss::Series::CalcAccessor::CalcOptions options;
options.elementwise = true;

auto x = pluss::make_series({0.5, 1.0, 1.5});
auto constant = [](const pluss::Series& z) {
  return z.mul(0.0).add(7.0).detach();
};

std::cout << x.calc(options).diff(constant) << "\n";
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0, 0]
```

Example 6. Strict mode turns the same missing dependency into a diagnostic,
which is useful when a detached path is more likely to be a model bug than an
intentional constant.

```cpp
pluss::Series::CalcAccessor::CalcOptions options;
options.elementwise = true;
options.strict_dependency = true;

auto x = pluss::make_series({0.5, 1.0, 1.5});
auto constant = [](const pluss::Series& z) {
  return z.mul(0.0).add(7.0).detach();
};

try {
  (void)x.calc(options).diff(constant);
} catch (const std::exception& error) {
  std::string message = error.what();
  std::cout << message.substr(0, message.find('\n')) << "\n";
}
```

### Output

```text
Series.calc.diff: the function does not depend on x at derivative order 1
```

## Receiver State And Platform

The receiver need not be a leaf and need not have tracking enabled. `diff()`
creates its own leaf and leaves the receiver's tracking flag and stored
sensitivity unchanged. The derivative remains on the receiver's platform; no
CPU transfer is part of the calculus contract.

The callback should keep its result on that platform. Host extraction inside
the callback breaks graph composition and prevents batched accelerator work.

## Performance

Full higher-order derivative tensors grow as the product of all output lanes
and one complete input lane set per order. That geometry is inherently large:
a length-`N` vector-to-vector `n`th derivative has `N^(n+1)` entries. Prefer:

- `elementwise=true` for genuinely lane-local functions;
- [`grad`](grad.md) for scalar objectives when only first order is needed;
- [`hess`](hess.md) for a scalar second derivative with an explicit contract;
- contractions inside the callback when the full output geometry is not
  required.

## Failure Modes

- `order` must be non-negative.
- The callback must return a defined `Series`.
- Elementwise mode requires the callback output shape to equal the input shape.
- Strict mode rejects a disconnected derivative path.
- A higher-order request rejects reverse paths that are not themselves
  differentiable.
- Shape growth and allocation limits still apply to full derivative tensors.

## See Also

- [`Series::CalcAccessor::grad`](grad.md): scalar-objective first derivative.
- [`Series::CalcAccessor::jac`](jac.md): named first full derivative.
- [`Series::CalcAccessor::hess`](hess.md): scalar-objective second derivative.
- [`Series::diff`](../series/diff.md): adjacent-sample forward differences.
- Stochastic differentials: [`diff.ito`](diff_ito.md), [`diff.strat`](diff_strat.md),
  [`diff.marcus`](diff_marcus.md), [`diff.malliavin`](diff_malliavin.md).

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="antidiff.html"><span class="label">Previous</span><span class="title">antidiff</span></a>
  <a class="next-link" href="grad.html"><span class="label">Next</span><span class="title">grad</span></a>
</div>
