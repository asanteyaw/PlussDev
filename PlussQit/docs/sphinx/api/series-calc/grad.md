<!-- pluss-hand-authored -->

# Series::CalcAccessor::grad

[Back to Series::CalcAccessor API Reference](../../series_calc_accessor_api_reference.md)

## Signature

```cpp
template<class Fn>
Series Series::CalcAccessor::grad(Fn&& fn) const
```

## What It Computes

`grad()` computes the first derivative of a scalar-valued callable with respect
to every lane of the owning `Series`. If `x` has shape `X`, `fn(x)` must be a
rank-zero `Series` and the result has shape `X`:

```text
gradient[i] = partial fn(x) / partial x[i]
```

For higher-rank receivers, `i` is a multi-index. The gradient retains the
original axes rather than flattening them into a vector, so its entries remain
aligned with the variables they describe.

`grad()` delegates to the same first-order transform as `diff(fn, 1)`. Its
additional value is the scalar-objective check and the resulting shape promise.
Use it for losses, energies, likelihoods, and other scalar summaries. Use
[`jac`](jac.md) when the callback returns multiple values.

## Options And State

`strict_dependency` is honored. Other derivative options do not alter
`grad()`'s fixed first-order, full-gradient geometry: `order` is ignored and
`elementwise` does not turn a vector objective into a gradient.

The receiver does not need `tracks_slope_()` and does not receive a stored
sensitivity. `grad()` creates a temporary differentiable leaf, returns a new
`Series`, and preserves the receiver's dtype-compatible platform.

## Examples

Example 1. The gradient of a sum of squares is `2x`.

```cpp
auto x = pluss::make_series({2.0, 3.0});
auto gradient = x.calc.grad(
    [](const pluss::Series& z) { return z.square().sum(); });

std::cout << gradient << "\n";
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[4, 6]
```

Example 2. Matrix variables remain a matrix. For
`fn(z) = 1/2 * sum(z^2)`, the gradient equals `z` lane by lane.

```cpp
auto x = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
auto gradient = x.calc.grad([](const pluss::Series& z) {
  return z.square().mul(0.5).sum();
});

std::cout << gradient << "\n";
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 2],
  [3, 4]]
```

Example 3. `grad()` and first-order `diff()` agree for a scalar objective, but
`grad()` communicates the intended geometry directly.

```cpp
auto x = pluss::make_series({2.0, 3.0});
auto objective = [](const pluss::Series& z) { return z.square().sum(); };

std::cout << "grad=" << x.calc.grad(objective) << "\n";
std::cout << "diff=" << x.calc.diff(objective, 1) << "\n";
```

### Output

```text
grad=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[4, 6]
diff=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[4, 6]
```

Example 4. Returning one value per input lane is not a scalar objective, even
when summing it later would have been easy. `grad()` refuses to guess that
reduction.

```cpp
auto x = pluss::make_series({2.0, 3.0});
try {
  (void)x.calc.grad(
      [](const pluss::Series& z) { return z.square(); });
} catch (const std::exception& error) {
  std::string message = error.what();
  std::cout << message.substr(0, message.find('\n')) << "\n";
}
```

### Output

```text
Series.calc.grad: function must return a rank-zero scalar Series
```

## Gradient Flow

The returned gradient remains differentiable when every reverse operation on
the callback path supports graph construction. This is how higher-order
transforms can differentiate a gradient. If only the second derivative is
needed, [`hess`](hess.md) expresses that intent and validates the scalar
objective in one call.

## Failure Modes

- The receiver and callback result must be defined.
- The callback result must have rank zero; shape `[1]` is not scalar.
- Strict dependency mode rejects an objective disconnected from the receiver.
- Unsupported differentiable paths fail according to the centralized
  higher-order transform contract.

## See Also

- [`Series::CalcAccessor::jac`](jac.md): first derivative of a non-scalar
  output.
- [`Series::CalcAccessor::hess`](hess.md): second derivative of a scalar
  objective.
- [`Series::reverse`](../series/reverse.md): reverse an already-built graph and
  store sensitivities on tracked leaves.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="diff.html"><span class="label">Previous</span><span class="title">diff</span></a>
  <a class="next-link" href="hess.html"><span class="label">Next</span><span class="title">hess</span></a>
</div>
