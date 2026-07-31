<!-- pluss-hand-authored -->

# Series::maximum

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::maximum(const Series& rhs) const -> Series
Series::maximum(double rhs) const -> Series
```

## What It Does

Elementwise maximum: `out[i] = max(self[i], rhs[i])` (or `max(self[i], rhs)` for the
scalar overload). Unlike [`clamp_min`](clamp_min.md), both sides are ordinary
values — there is no "bound" role here, just two operands compared elementwise.

`self[i] == rhs[i]` (a tie) simply returns that shared value; ties only matter once
you look at the **Gradient** section below, because a tie is the one case where
`self` and `rhs` can't both keep the full derivative.

## Parameters

**rhs** (`const Series&` or `double`)

The other operand. The `Series` overload broadcasts against `self` using ordinary
broadcasting rules; the `double` overload compares every element against the same
scalar.

## Returns

`Series`
    A new `Series` holding the elementwise maximum, promoted to the wider of the two
    operand dtypes for the `Series` overload.

## Gradient

`maximum` is differentiable in both operands.

```{note} Tie-breaking convention
At a tie (`self[i] == rhs[i]`), the full gradient is attributed to `self` — `rhs`
gets zero at that position. This matches [`clamp_min`](clamp_min.md)/[`clamp_max`](clamp_max.md)'s
own boundary convention (both are inclusive toward `self`), so a value composed from
`relu()`/`clamp_min()`/`maximum()` in sequence has one consistent boundary rule
throughout, rather than each op picking its own.
```

Formally: `d(out)/d(self[i]) = 1` where `self[i] >= rhs[i]`, else `0`.
`d(out)/d(rhs[i]) = 1` where `self[i] < rhs[i]`, else `0`.

## Examples

Example 1. Elementwise maximum between two same-shaped tensors.

```cpp
auto x = pluss::make_series(std::vector<double>{1, 5, 3}, {3});
auto y = pluss::make_series(std::vector<double>{4, 2, 3}, {3});
std::cout << x.maximum(y);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[4, 5, 3]
```

Index 2 is a tie (`3 == 3`): the output is `3` either way, but see the gradient
example below for what a tie means for *which operand* that `3` is attributed to.

Example 2. The scalar overload compares every element against one bound.

```cpp
auto x = pluss::make_series(std::vector<double>{1, 5, 3}, {3});
std::cout << x.maximum(2.5);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2.5, 5, 3]
```

Only index 0 (`1 < 2.5`) is raised to the bound; the other two elements already
exceed it and pass through unchanged.

Example 3. Gradient split at a tie.

```cpp
auto a = pluss::make_series(std::vector<double>{1, 5, 2}, {3});
auto b = pluss::make_series(std::vector<double>{4, 2, 2}, {3});
a.tracks_slope_(true);
b.tracks_slope_(true);

auto y = a.maximum(b);
y.sum().reverse();
std::cout << "y=" << y << "\n";
std::cout << "da=" << a.sensitivity() << "\n";
std::cout << "db=" << b.sensitivity() << "\n";
```

### Output

```text
y=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[4, 5, 2]
da=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 1, 1]
db=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 0, 0]
```

Index 0 (`a=1 < b=4`): gradient flows entirely to `b` (`da=0`, `db=1`). Index 1
(`a=5 > b=2`): gradient flows entirely to `a`. Index 2 is the tie (`a=2 == b=2`):
the gradient goes to `a` (`da=1`, `db=0`), per the tie-breaking convention above —
`b` gets nothing even though its value equally determined the output.

## Notes

- `maximum` is a plain elementwise comparison, not a reduction — for "the largest
  element across a dimension," see the reduction surface (`amax`) instead.
- The `Series`-arg overload broadcasts; the `double`-arg overload does not need to,
  since a scalar compares against every element uniformly.
- See [`clamp_min`](clamp_min.md) for the common case of maximizing against a fixed
  scalar floor phrased as a one-sided clamp instead.

## See Also

- [`Series::minimum`](minimum.md): the elementwise counterpart, with the mirrored
  tie-breaking convention.
- [`Series::clamp_min`](clamp_min.md), [`Series::clamp_max`](clamp_max.md):
  one-sided bounds built from the same underlying comparison.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="index_select.html">
    <span class="label">Previous</span>
    <span class="title">index_select</span>
  </a>
  <a class="next-link" href="minimum.html">
    <span class="label">Next</span>
    <span class="title">minimum</span>
  </a>
</div>
