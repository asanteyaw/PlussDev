<!-- pluss-hand-authored -->

# Series::scatter_reduce_amax

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::scatter_reduce_amax(
    std::int64_t dim,
    const Series& index,
    const Series& src,
    bool include_self) const -> Series

Series::scatter_reduce_amax(
    std::int64_t dim,
    const Series& index,
    const Series& src,
    bool include_self,
    double initial_scalar) const -> Series
```

## What It Does

`scatter_reduce_amax()` is the typed maximum-reduction wrapper around
[`scatter_reduce()`](scatter_reduce.md).

It answers one question repeatedly along the indexed destinations:

```text
Which value is the largest among the competing contributors here?
```

Because the reduction is fixed to maximum, this wrapper is clearer than passing
the string `"amax"` by hand and it avoids a stringly-typed call site.

## Parameters

**dim**, **index**, and **src**

These behave exactly as on [`scatter_reduce()`](scatter_reduce.md).

**include_self** (`bool`)

Whether the current destination lane in `self` competes in the maximum.

If it is `true`, the old value can survive when it is already larger than every
incoming scattered contribution.

**initial_scalar** (`double`)

An explicit seed that participates in the maximum when this overload is used.

## Returns

`Series`
    A new `Series` containing the indexed maximum reduction.

## Gradient

Reverse-mode routes sensitivity only through the winning maximum contributors.
If several contributors tie for the same winning value, the sensitivity of that
destination lane is shared across them.

## Examples

Example 1. When `include_self = true`, the existing destination value can win
the maximum race.

```cpp
auto base = pluss::make_series({10.0, 20.0, 30.0, 40.0});
auto index = pluss::make_series({1LL, 1LL});
auto src = pluss::make_series({1.0, 3.0});

std::cout << base.scatter_reduce_amax(0, index, src, true);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[10, 20, 30, 40]
```

Destination `1` compares `20`, `1`, and `3`, so the base lane stays in place.

Example 2. When `include_self = false`, only the scattered contributors
compete.

```cpp
std::cout << base.scatter_reduce_amax(0, index, src, false);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[10, 3, 30, 40]
```

Now destination `1` chooses between `1` and `3` only, so the maximum is `3`.

Example 3. Reverse-mode splits sensitivity across equal maxima.

```cpp
auto grad_base = pluss::make_series({2.0, 5.0, 5.0, 1.0});
auto grad_src = pluss::make_series({5.0, 4.0});
auto grad_index = pluss::make_series({1LL, 3LL});

grad_base.tracks_slope_(true);
grad_src.tracks_slope_(true);

auto out = grad_base.scatter_reduce(0, grad_index, grad_src, "amax", true);
out.sum().reverse();

std::cout << "out=" << out << "\n";
std::cout << "dbase=" << grad_base.sensitivity() << "\n";
std::cout << "dsrc=" << grad_src.sensitivity() << "\n";
```

### Output

```text
out=pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[2, 5, 5, 4]
dbase=pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1, 0.5, 1, 0]
dsrc=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.5, 1]
```

At destination `1`, both the base lane and the first source lane equal the
winning maximum `5`, so the lane's upstream sensitivity is split equally.

## Gotchas And Failure Modes

- `include_self` determines whether the old destination value is a candidate
  maximum.
- Ties are real and matter for gradients.
- If you need a different reduction, use [`scatter_reduce()`](scatter_reduce.md)
  or one of its other typed wrappers.

## Summary

Use `scatter_reduce_amax()` when repeated indexed writes should keep the
largest contributor at each destination.

## See Also

- [`Series::scatter_reduce`](scatter_reduce.md): general form.
- [`Series::scatter_reduce_amin`](scatter_reduce_amin.md): minimum counterpart.
- [`Series::scatter_reduce_sum`](scatter_reduce_sum.md): additive counterpart.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="scatter_reduce_.html">
    <span class="label">Previous</span>
    <span class="title">scatter_reduce_</span>
  </a>
  <a class="next-link" href="scatter_reduce_amin.html">
    <span class="label">Next</span>
    <span class="title">scatter_reduce_amin</span>
  </a>
</div>
