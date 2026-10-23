<!-- pluss-hand-authored -->

# Series::CalcAccessor::jac

[Back to Series::CalcAccessor API Reference](../../series_calc_accessor_api_reference.md)

## Signature

```cpp
template<class Fn>
Series Series::CalcAccessor::jac(Fn&& fn) const
```

## What It Computes

`jac()` computes the complete first derivative of a callback with arbitrary
output shape. If `fn(x)` has shape `Y` and `x` has shape `X`, the Jacobian has
shape `Y + X`: output axes first, input axes second.

This convention treats every callback output lane as one row of derivatives
without flattening either side. For a vector-to-vector map it gives the familiar
matrix. For a matrix-to-matrix map it gives a rank-four tensor whose first two
axes locate an output cell and whose final two axes locate an input cell.

`jac(fn)` is the named first-order form of `diff(fn, 1)`. Unlike
[`grad`](grad.md), it does not require a scalar output and does not reduce one
on the caller's behalf.

## Examples

Example 1. The first two output lanes are squares; the third is a coupled sum.
Rows therefore contain `[4,0]`, `[0,6]`, and `[1,1]` at input `[2,3]`.

```cpp
auto x = pluss::make_series({2.0, 3.0});
auto jacobian = x.calc.jac([](const pluss::Series& z) {
  return z.square().unite({z.sum().reshape({1})}, 0);
});

std::cout << jacobian << "\n";
```

### Output

```text
pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[4, 0],
  [0, 6],
  [1, 1]]
```

Example 2. Higher-rank axes remain explicit. Transposition only reroutes each
output cell to one input cell, so the rank-four Jacobian contains a permutation
pattern of ones.

```cpp
auto x = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
auto jacobian = x.calc.jac(
    [](const pluss::Series& z) { return z.transpose(0, 1); });

std::cout << "shape=[";
for (auto extent : jacobian.sizes()) std::cout << extent << ' ';
std::cout << "]\nflat=" << jacobian.flatten() << "\n";
```

### Output

```text
shape=[2 2 2 2 ]
flat=pluss::Series(shape=[16], strides=[1], dtype=float64, platform=cpu, numel=16, contiguous=true)
data=[1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1]
```

Example 3. A scalar output is valid. With no output axes to prepend, the
Jacobian shape is simply the input shape, matching `grad()`.

```cpp
auto x = pluss::make_series({2.0, 3.0});
auto objective = [](const pluss::Series& z) { return z.square().sum(); };

std::cout << "jac=" << x.calc.jac(objective) << "\n";
std::cout << "grad=" << x.calc.grad(objective) << "\n";
```

### Output

```text
jac=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[4, 6]
grad=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[4, 6]
```

Example 4. Elementwise `diff()` is preferable when a large lane-local map only
needs its diagonal. Both calls below describe the same diagonal values, but the
full Jacobian also stores all off-diagonal zeros.

```cpp
auto x = pluss::make_series({0.5, 1.0, 1.5});
auto fn = [](const pluss::Series& z) { return z.sin(); };

pluss::Series::CalcAccessor::CalcOptions options;
options.elementwise = true;

std::cout << "jac_shape=" << x.calc.jac(fn).sizes().size() << " axes\n";
std::cout << "diagonal=" << x.calc(options).diff(fn) << "\n";
```

### Output

```text
jac_shape=2 axes
diagonal=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.87758256, 0.54030231, 0.070737202]
```

## Cost

The output contains `fn(x).numel() * x.numel()` entries. This is the requested
mathematical object, not temporary overhead, so large maps can consume
substantial memory. Reduce or select callback outputs before `jac()` when only a
subset is needed. Use elementwise `diff()` only when off-diagonal derivatives
are known to be irrelevant by construction.

## Options And Failure Modes

`strict_dependency` is honored. `order` and `elementwise` do not alter
`jac()`'s fixed full first-order shape. The callback must return a defined
`Series`, and all operations on its path must provide a valid reverse rule.
Strict mode rejects output lanes disconnected from the receiver instead of
materializing their zero rows.

## See Also

- [`Series::CalcAccessor::grad`](grad.md): scalar-only first derivative.
- [`Series::CalcAccessor::hess`](hess.md): scalar second derivative.
- [`Series::CalcAccessor::diff`](diff.md): arbitrary order and an elementwise
  storage-saving mode.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="hess.html"><span class="label">Previous</span><span class="title">hess</span></a>
  <a class="next-link" href="options.html"><span class="label">Next</span><span class="title">options</span></a>
</div>
