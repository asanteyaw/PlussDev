<!-- pluss-hand-authored -->

# Series::CalcAccessor::hess

[Back to Series::CalcAccessor API Reference](../../series_calc_accessor_api_reference.md)

## Signature

```cpp
template<class Fn>
Series Series::CalcAccessor::hess(Fn&& fn) const
```

## What It Computes

`hess()` computes all second partial derivatives of a scalar-valued callback.
For receiver shape `X`, the result shape is `X + X`. The first input-axis block
identifies the gradient component being differentiated; the second identifies
the variable used for that derivative.

For a vector receiver, this is the usual square Hessian matrix. For a matrix
receiver of shape `[r,c]`, the result has shape `[r,c,r,c]`; axes are not
silently flattened. This makes contractions and axis-aware analysis explicit.

`hess(fn)` is the scalar-constrained equivalent of `diff(fn, 2)`. It runs
higher-order reverse mode, not finite differencing of a previously rounded
gradient. The callback path therefore needs reverse rules that remain
differentiable through second order.

## Reading The Result

For vector `x`, entry `[i,j]` is

```text
partial^2 fn(x) / (partial x[i] partial x[j])
```

Diagonal entries measure curvature along one variable. Off-diagonal entries
measure coupling: they are zero for a sum of lane-local terms and nonzero when
variables interact. Numerically equal cross-partials are expected for a smooth
function, but `hess()` does not post-process or symmetrize the computed tensor.

## Examples

Example 1. A sum of cubes has diagonal curvature `6x` and no cross coupling.

```cpp
auto x = pluss::make_series({2.0, 3.0});
auto hessian = x.calc.hess(
    [](const pluss::Series& z) { return z.pow(3.0).sum(); });

std::cout << hessian << "\n";
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[12, 0],
  [0, 18]]
```

Example 2. Mixed terms appear off the diagonal. At `(a,b)=(2,3)`,
`fn(a,b)=a^2*b+b^3` has cross-partials `2a=4`.

```cpp
auto x = pluss::make_series({2.0, 3.0});
auto hessian = x.calc.hess([](const pluss::Series& z) {
  auto a = z.select(0, 0);
  auto b = z.select(0, 1);
  return a.square().mul(b).add(b.pow(3.0));
});

std::cout << hessian << "\n";
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[6, 4],
  [4, 18]]
```

Example 3. A scalar receiver produces a rank-zero Hessian because each copy of
an empty input shape contributes no axis.

```cpp
auto x = pluss::make_series(0.5);
auto curvature = x.calc.hess(
    [](const pluss::Series& z) { return z.sin(); });

std::cout << std::setprecision(10);
std::cout << curvature.item<double>() << "\n";
```

### Output

```text
-0.4794255386
```

Example 4. `hess()` requires an actual scalar callback result. It never sums a
vector implicitly because doing so would change the requested function.

```cpp
auto x = pluss::make_series({2.0, 3.0});
try {
  (void)x.calc.hess(
      [](const pluss::Series& z) { return z.square(); });
} catch (const std::exception& error) {
  std::string message = error.what();
  std::cout << message.substr(0, message.find('\n')) << "\n";
}
```

### Output

```text
Series.calc.hess: function must return a rank-zero scalar Series
```

## Cost And Storage

For `N` input lanes, the Hessian contains `N^2` values. Keeping higher-rank
axes does not change that count; it only preserves their meaning. Avoid
materializing the full Hessian when an algorithm needs only a directional
curvature or matrix-vector product. `hess()` is appropriate when every entry is
actually consumed or when `N` is modest.

## Options And Failure Modes

`strict_dependency` is honored. `order` and `elementwise` do not change the
fixed scalar, second-order contract. Failures include an undefined callback
result, a non-scalar result, a disconnected strict path, or an operation whose
reverse rule does not support differentiation through the second pass.

## See Also

- [`Series::CalcAccessor::grad`](grad.md): scalar first derivative.
- [`Series::CalcAccessor::diff`](diff.md): arbitrary order and output shape.
- [`Series::CalcAccessor::jac`](jac.md): first derivative of any output.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="grad.html"><span class="label">Previous</span><span class="title">grad</span></a>
  <a class="next-link" href="jac.html"><span class="label">Next</span><span class="title">jac</span></a>
</div>
