<!-- pluss-hand-authored -->

# Series::minimum

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::minimum(const Series& rhs) const -> Series
Series::minimum(double rhs) const -> Series
```

## What It Does

Elementwise minimum: `out[i] = min(self[i], rhs[i])` (or `min(self[i], rhs)` for the
scalar overload) — the mirror image of [`maximum`](maximum.md).

## Parameters

**rhs** (`const Series&` or `double`)

The other operand. The `Series` overload broadcasts against `self`; the `double`
overload compares every element against the same scalar.

## Returns

`Series`
    A new `Series` holding the elementwise minimum, promoted to the wider of the two
    operand dtypes for the `Series` overload.

## Gradient

`minimum` is differentiable in both operands.

```{note} Tie-breaking convention
At a tie (`self[i] == rhs[i]`), the full gradient again goes to `self` — the same
"ties favor self" rule as [`maximum`](maximum.md), not a mirrored "ties favor rhs"
rule. Keeping one tie-breaking direction across both ops (rather than each picking
its own) is what makes `relu()`, `clamp_min()`/`clamp_max()`, and `maximum()`/`minimum()`
compose into a single consistent derivative at their shared boundaries.
```

Formally: `d(out)/d(self[i]) = 1` where `self[i] <= rhs[i]`, else `0`.
`d(out)/d(rhs[i]) = 1` where `self[i] > rhs[i]`, else `0`.

## Examples

Example 1. Elementwise minimum between two same-shaped tensors.

```cpp
auto x = pluss::make_series(std::vector<double>{1, 5, 3}, {3});
auto y = pluss::make_series(std::vector<double>{4, 2, 3}, {3});
std::cout << x.minimum(y);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 2, 3]
```

Example 2. The scalar overload caps every element at one bound from above.

```cpp
auto x = pluss::make_series(std::vector<double>{1, 5, 3}, {3});
std::cout << x.minimum(2.5);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 2.5, 2.5]
```

Indices 1 and 2 (`5` and `3`, both `> 2.5`) are pulled down to the bound; index 0
(`1 < 2.5`) already satisfies it and passes through unchanged. This scalar overload
is exactly what [`clamp_max`](clamp_max.md) does with a friendlier name for the
"cap from above" use case.

Example 3. Gradient split at a tie — same operands as `maximum`'s Example 3, to
show the two ops attribute the tied position identically.

```cpp
auto a = pluss::make_series(std::vector<double>{1, 5, 2}, {3});
auto b = pluss::make_series(std::vector<double>{4, 2, 2}, {3});
a.tracks_slope_(true);
b.tracks_slope_(true);

auto y = a.minimum(b);
y.sum().reverse();
std::cout << "y=" << y << "\n";
std::cout << "da=" << a.sensitivity() << "\n";
std::cout << "db=" << b.sensitivity() << "\n";
```

### Output

```text
y=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 2, 2]
da=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 0, 1]
db=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 1, 0]
```

Index 0 (`a=1 < b=4`): `minimum` picks `a`, so the gradient flows to `a`. Index 1
(`a=5 > b=2`): `minimum` picks `b`, gradient flows to `b`. Index 2 is the tie
(`a=2 == b=2`): gradient goes to `a` (`da=1`, `db=0`) — the same direction as
`maximum`'s tie in its own Example 3, not the opposite.

## Notes

- `minimum` is a plain elementwise comparison, not a reduction — for "the smallest
  element across a dimension," see the reduction surface (`amin`) instead.
- The `Series`-arg overload broadcasts; the `double`-arg overload does not need to.

## See Also

- [`Series::maximum`](maximum.md): the elementwise counterpart, with the same
  tie-breaking convention (not a mirrored one).
- [`Series::clamp_max`](clamp_max.md), [`Series::clamp_min`](clamp_min.md):
  one-sided bounds built from the same underlying comparison.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="maximum.html">
    <span class="label">Previous</span>
    <span class="title">maximum</span>
  </a>
</div>
