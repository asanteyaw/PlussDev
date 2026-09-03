<!-- pluss-hand-authored -->

# Series::scatter_reduce_amin

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::scatter_reduce_amin(
    std::int64_t dim,
    const Series& index,
    const Series& src,
    bool include_self) const -> Series

Series::scatter_reduce_amin(
    std::int64_t dim,
    const Series& index,
    const Series& src,
    bool include_self,
    double initial_scalar) const -> Series
```

## What It Does

`scatter_reduce_amin()` is the typed minimum-reduction wrapper around
[`scatter_reduce()`](scatter_reduce.md).

At each indexed destination it asks the mirror-image question of
`scatter_reduce_amax()`:

```text
Which value is the smallest among the competing contributors here?
```

Use it when repeated indexed writes should keep the lowest contributor instead
of summing or overwriting.

## Parameters

**dim**, **index**, and **src**

These have the same meaning as on [`scatter_reduce()`](scatter_reduce.md).

**include_self** (`bool`)

Whether the old destination value in `self` participates in the minimum.

**initial_scalar** (`double`)

An explicit seed that joins the minimum comparison when this overload is used.

## Returns

`Series`
    A new `Series` containing the indexed minimum reduction.

## Gradient

Reverse-mode routes sensitivity only through the winning minima. Equal winners
share the incoming sensitivity of that destination lane.

## Examples

Example 1. With `include_self = false`, only the scattered contributors can win
the minimum.

```cpp
auto base = pluss::make_series({10.0, 20.0, 30.0, 40.0});
auto index = pluss::make_series({1LL, 1LL});
auto src = pluss::make_series({1.0, 3.0});

std::cout << base.scatter_reduce_amin(0, index, src, false);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[10, 1, 30, 40]
```

Destination `1` sees only `1` and `3`, so the minimum is `1`.

Example 2. With `include_self = true`, a smaller base lane can remain in place.

```cpp
auto keep_base = pluss::make_series({10.0, 0.0, 30.0, 40.0});

std::cout << keep_base.scatter_reduce_amin(0, index, src, true);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[10, 0, 30, 40]
```

The base lane `0` is smaller than both incoming contributors, so it wins.

Example 3. Reverse-mode follows the winning minima.

```cpp
auto grad_base = pluss::make_series({1.0, 2.0});
auto grad_src = pluss::make_series({-3.0, 0.5});
auto grad_index = pluss::make_series({0LL, 1LL});

grad_base.tracks_slope_(true);
grad_src.tracks_slope_(true);

auto out = grad_base.scatter_reduce(0, grad_index, grad_src, "amin", false);
out.sum().reverse();

std::cout << "out=" << out << "\n";
std::cout << "dbase=" << grad_base.sensitivity() << "\n";
std::cout << "dsrc=" << grad_src.sensitivity() << "\n";
```

### Output

```text
out=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[-3, 0.5]
dbase=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0, 0]
dsrc=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 1]
```

Because `include_self = false`, the minimum at each destination is chosen only
from the scattered values, so the base lanes receive no sensitivity.

## Gotchas And Failure Modes

- `include_self` determines whether the old destination value competes in the
  minimum.
- Like `amax`, equal winners share sensitivity.
- If you need a sum or a maximum instead, choose the corresponding wrapper.

## Summary

Use `scatter_reduce_amin()` when repeated indexed writes should keep the
smallest contributor at each destination.

## See Also

- [`Series::scatter_reduce`](scatter_reduce.md): general form.
- [`Series::scatter_reduce_amax`](scatter_reduce_amax.md): maximum counterpart.
- [`Series::scatter_reduce_sum`](scatter_reduce_sum.md): additive counterpart.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="scatter_reduce_amax.html">
    <span class="label">Previous</span>
    <span class="title">scatter_reduce_amax</span>
  </a>
  <a class="next-link" href="scatter_reduce_sum.html">
    <span class="label">Next</span>
    <span class="title">scatter_reduce_sum</span>
  </a>
</div>
