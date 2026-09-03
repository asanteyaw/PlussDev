<!-- pluss-hand-authored -->

# Series::norm

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::norm(double p) const -> Series
Series::norm(double p, std::int64_t dim, bool keepdim=false) const -> Series
```

## What It Does

`norm()` computes a p-norm from absolute values.

For a flat vector `x`, the core rule is:

- `p = 2`: Euclidean norm, `sqrt(sum |xᵢ|²)`
- `p = 1`: Manhattan norm, `sum |xᵢ|`
- `p = inf`: maximum absolute value
- general finite `p`: `(sum |xᵢ|ᵖ)^(1/p)`

The method uses the same absolute-value idea for signed data, so negative
entries contribute by magnitude, not by sign.

There are two public geometries:

- `norm(p)` reduces the whole `Series`
- `norm(p, dim, keepdim)` reduces one axis at a time

## Parameters

**p** (`double`)

The norm order.

- `1.0` gives the sum of absolute values
- `2.0` gives the Euclidean norm
- `infinity` gives the maximum absolute value
- other finite values use the general p-norm formula

Changing `p` changes the geometry of "size." A larger `p` puts more weight on
the largest magnitudes in the lane.

**dim** (`std::int64_t`)

The axis to reduce in the axis-wise overload.

- positive dims count from the front
- negative dims count from the back
- the dim must be valid after normalization

**keepdim** (`bool`)

Whether to preserve the reduced axis as size `1`.

This affects alignment and broadcasting, not the underlying norm formula.

## Returns

`Series`
    A reduced `Series` containing norm values.

On the current verified root surface:

- `norm(1.0)`, `norm(2.0)`, and `norm(inf)` over the whole input return a
  scalar-shaped result
- the general zero-argument finite-`p` path currently returns a one-value
  `Series` with shape `[1]`

The contained numeric value is still the norm in both cases.

## Gradient

`norm()` is differentiable on its smooth branches, with the usual caution that
non-smooth points exist at zero or at max-style ties depending on `p`.

For the Euclidean case,

`||x||₂ = sqrt(x₁² + x₂² + ... + xₙ²)`

so the derivative with respect to each entry is:

`d||x||₂/dxᵢ = xᵢ / ||x||₂`

That is why the backward of the `3-4-5` example produces `0.6` and `0.8`:

- `d||x||₂/dx₁ = 3 / 5 = 0.6`
- `d||x||₂/dx₂ = 4 / 5 = 0.8`

## Examples

Example 1. The Euclidean norm of `{3, 4}` is `5`.

```cpp
auto vec = pluss::make_series({3.0, 4.0});
std::cout << vec.norm(2.0);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=5
```

Example 2. The `L1` norm adds magnitudes, not signed values.

```cpp
auto signed_vec = pluss::make_series({-1.0, 2.0, -3.0});
pluss::series::print(std::cout, signed_vec.norm(1.0));
```

### Output

```text
Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=6.0000
```

The calculation is `|-1| + |2| + |-3| = 1 + 2 + 3 = 6`.

Example 3. The infinity norm keeps only the largest absolute magnitude.

```cpp
std::cout << signed_vec.norm(std::numeric_limits<double>::infinity());
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=3
```

So the sign is irrelevant here too; only the largest magnitude matters.

Example 4. A general finite `p` uses the full p-norm formula.

```cpp
auto general_vec = pluss::make_series({1.0, 2.0, 2.0});
std::cout << general_vec.norm(3.0);
```

### Output

```text
pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[2.5712816]
```

Here the value is

`( |1|^3 + |2|^3 + |2|^3 )^(1/3) = (1 + 8 + 8)^(1/3) = 17^(1/3) ≈ 2.5712816`.

Example 5. `norm(p, dim)` reduces one axis at a time.

```cpp
auto norm_mat = pluss::make_series({
    {3.0, 4.0},
    {5.0, 12.0},
});

std::cout << norm_mat.norm(2.0, 1, false);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[5, 13]
```

So each row is reduced independently:

- first row: `sqrt(3^2 + 4^2) = 5`
- second row: `sqrt(5^2 + 12^2) = 13`

Example 6. `keepdim=true` preserves the reduced axis as size `1`.

```cpp
pluss::series::print(std::cout, norm_mat.norm(2.0, 1, true));
```

### Output

```text
Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=
⎡  5.0000 ⎤
⎣ 13.0000 ⎦
```

Example 7. Negative dims count from the back.

```cpp
std::cout << norm_mat.norm(2.0, -1, false);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[5, 13]
```

For a rank-2 `Series`, `-1` means the last axis, so this matches Example 5.

Example 8. The Euclidean backward sends each entry its fraction of the overall
length.

```cpp
auto norm_grad = pluss::make_series({3.0, 4.0});
norm_grad.tracks_slope_(true);
norm_grad.norm(2.0).reverse();

std::cout << "y=" << norm_grad.norm(2.0) << "\n";
std::cout << "dx=" << norm_grad.sensitivity();
```

### Output

```text
y=pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=5
dx=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.6, 0.8]
```

This is exactly the `xᵢ / ||x||₂` rule:

- `3 / 5 = 0.6`
- `4 / 5 = 0.8`

Example 9. Invalid dims fail through the shared reduction checks.

```cpp
try {
  std::cout << norm_mat.norm(2.0, 2, false) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
reduce: dim out of range
```

## Gotchas And Failure Modes

- `norm()` uses magnitudes, so signs do not cancel.
- Different `p` values express different ideas of size.
- `keepdim` changes shape only.
- The infinity norm is really a max-of-magnitudes reduction.
- On the current verified surface, the zero-argument general finite-`p` path
  returns shape `[1]` rather than scalar shape `[]`.

## Summary

Use `norm()` when the question is "how large is this lane by a chosen norm?"

- `p = 1` sums magnitudes
- `p = 2` gives Euclidean length
- `p = inf` gives the largest magnitude
- `norm(p, dim, keepdim)` applies the same idea lane by lane

## See Also

- [`Series::amax`](amax.md): infinity-norm building block through maximum
  absolute value.
- [`Series::abs`](abs.md): the magnitude step that underlies the norm family.
- [`Series::sum`](sum.md): additive reduction without the p-norm geometry.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nonzero.html">
    <span class="label">Previous</span>
    <span class="title">nonzero</span>
  </a>
  <a class="next-link" href="permute.html">
    <span class="label">Next</span>
    <span class="title">permute</span>
  </a>
</div>
