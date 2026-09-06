<!-- pluss-hand-authored -->

# Series::amin

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::amin() const -> Series
Series::amin(std::int64_t dim) const -> Series
Series::amin(std::int64_t dim, bool keepdim) const -> Series
```

## What It Does

`amin()` reduces by value and keeps only minima.

Like [`amax()`](amax.md), this is a reduction-only surface. It does not switch
into pairwise comparison mode the way [`min()`](min.md) can when you pass
another `Series`.

That makes `amin()` the straight "smallest value along this lane" method.

## Overload Map

**`amin()`**

Reduce the whole `Series` to one overall minimum.

**`amin(dim)`**

Reduce one axis and remove it from the result.

**`amin(dim, keepdim)`**

Reduce one axis and optionally keep it as size `1`.

## Parameters

**dim** (`std::int64_t`)

The axis to reduce.

- positive dims count from the front
- negative dims count from the back
- the dim must be valid after normalization

**keepdim** (`bool`)

Shape toggle for axis-wise reductions.

- `false`: remove the reduced axis
- `true`: keep it as a singleton axis

## Returns

`Series`
    A reduced `Series` containing minima.

## Gradient

`amin()` mirrors [`amax()`](amax.md) under minimum instead of maximum.

- a unique minimum receives the whole incoming sensitivity
- tied minima split it evenly

So `amin()` is also tie-sharing in reverse-mode.

## Examples

Example 1. With no arguments, `amin()` reduces the whole `Series` to one
overall minimum.

```cpp
auto values = pluss::make_series({
    {1.0, 7.0, 3.0},
    {4.0, 7.0, 6.0},
});

std::cout << values.amin();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=1
```

Example 2. `amin(0)` reduces down columns.

```cpp
pluss::series::print(std::cout, values.amin(0));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.0000, 7.0000, 3.0000]
```

The middle column has values `7` and `7`, so its minimum is also `7`.

Example 3. `keepdim=true` preserves the reduced axis.

```cpp
std::cout << values.amin(1, true);
```

### Output

```text
pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[1],
  [4]]
```

Example 4. Negative dims reduce from the back.

```cpp
std::cout << values.amin(-1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 4]
```

Example 5. Tied minima share the backward sensitivity evenly.

```cpp
auto amin_grad = pluss::make_series({-2.0, -2.0, 3.0});
amin_grad.tracks_slope_(true);
amin_grad.amin().reverse();

std::cout << amin_grad.sensitivity();
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.5, 0.5, 0]
```

This is the minimum-side version of the same tie-sharing rule from `amax()`.
The objective is `L = min(-2, -2, 3) = -2`, and the incoming sensitivity `1`
is split across the two tied winners.

Example 6. Invalid dims fail through the shared reduction checks.

```cpp
try {
  std::cout << values.amin(3) << "\n";
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

- `amin()` is reduction-only.
- `keepdim` preserves geometry, not values.
- Tied minima split backward sensitivity evenly.
- Invalid dims fail directly.
- `NaN` propagates: a single `NaN` in a reduction lane makes that lane's result
  `NaN`. Filter it out first if you want the minimum of the finite values.

## Summary

Use `amin()` when the public meaning should stay strictly "smallest value per
reduction lane."

- `amin()` gives one overall minimum
- `amin(dim)` gives one minimum per lane
- `keepdim=true` keeps the reduced axis
- tied minima share reverse-mode sensitivity

## See Also

- [`Series::amax`](amax.md): maximum-value companion.
- [`Series::min`](min.md): broader surface that also includes elementwise
  pairwise minima.
- [`Series::argmin`](argmin.md): positions of minima instead of minimum values.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="amax.html">
    <span class="label">Previous</span>
    <span class="title">amax</span>
  </a>
  <a class="next-link" href="aminmax.html">
    <span class="label">Next</span>
    <span class="title">aminmax</span>
  </a>
</div>
