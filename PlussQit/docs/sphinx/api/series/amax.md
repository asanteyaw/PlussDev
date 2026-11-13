<!-- pluss-hand-authored -->

# Series::amax

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::amax() const
Series Series::amax(std::int64_t dim) const
Series Series::amax(std::int64_t dim, bool keepdim) const
```

## What It Does

`amax()` reduces by value and keeps only maxima.

This is the reduction-only companion to [`max()`](max.md). That difference is
important:

- `amax()` is always a reduction
- `max(other)` has a second elementwise comparison family that `amax()` does
  not have

So `amax()` is the better choice when the public meaning should stay
unambiguous: "take maxima along this reduction lane."

## Overload Map

**`amax()`**

Reduce the whole `Series` to one overall maximum.

**`amax(dim)`**

Reduce one axis and drop that axis from the result.

**`amax(dim, keepdim)`**

Reduce one axis and optionally retain it as size `1`.

## Parameters

**dim** (`std::int64_t`)

The axis to reduce.

- positive dims count from the front
- negative dims count from the back
- the dim must land on a real axis after normalization

Changing `dim` changes which values compete with each other.

**keepdim** (`bool`)

Controls result shape after axis reduction.

- `false`: remove the reduced axis
- `true`: keep it as size `1`

This is mostly a shape-management toggle. The maxima themselves do not change;
only how the result can be aligned back against the original shape.

## Returns

`Series`
    A reduced `Series` containing maxima.

- `amax()` returns one scalar value for the whole input
- `amax(dim)` returns one maximum per surviving lane
- `keepdim=true` preserves the reduced axis as a singleton

## Gradient

`amax()` is differentiable almost everywhere, with one important tie rule.

If one value is the unique winner in a reduction lane, it receives the whole
incoming sensitivity. If multiple values tie for the maximum, the incoming
sensitivity is split evenly across those tied winners.

So for a lane `[2, 5, 5]`, the backward of `amax()` does not choose one winner
arbitrarily. Each `5` receives half of the incoming sensitivity.

## Examples

Example 1. With no arguments, `amax()` reduces the whole `Series` to one
overall maximum.

```cpp
auto values = pluss::make_series({
    {1.0, 7.0, 3.0},
    {4.0, 7.0, 6.0},
});

std::cout << values.amax();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=7
```

Example 2. `amax(0)` reduces down columns, so each column produces one winner.

```cpp
pluss::series::print(std::cout, values.amax(0));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[4.0000, 7.0000, 6.0000]
```

Example 3. `keepdim=true` preserves the reduced axis as size `1`.

```cpp
std::cout << values.amax(1, true);
```

### Output

```text
pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[7],
  [7]]
```

The maxima match the row-wise reduction you would expect from `dim = 1`; the
only difference is that the row axis stays present as a singleton.

Example 4. Negative dims count from the back.

```cpp
std::cout << values.amax(-1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[7, 7]
```

On a rank-2 `Series`, `-1` means the last axis, so this is the same reduction
geometry as `amax(1)`.

Example 5. Tied maxima share the backward sensitivity evenly.

```cpp
auto amax_grad = pluss::make_series({2.0, 5.0, 5.0});
amax_grad.tracks_slope_(true);
amax_grad.amax().reverse();

pluss::series::print(std::cout, amax_grad.sensitivity());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.0000, 0.5000, 0.5000]
```

The scalar objective here is `L = max(2, 5, 5) = 5`. Because the last two
entries tie for the winning value, the incoming scalar sensitivity `1` is split
as `1/2` and `1/2`.

Example 6. Out-of-range dims fail through the shared reduction checks.

```cpp
try {
  std::cout << values.amax(2) << "\n";
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

- `amax()` is reduction-only. If you want elementwise pairwise maxima, use
  [`max(other)`](max.md) or [`maximum()`](maximum.md).
- `keepdim` changes shape only.
- The tie rule matters in reverse-mode: equal winners share sensitivity.
- Invalid dims fail; they do not clamp to the nearest valid axis.
- `NaN` propagates: a single `NaN` in a reduction lane makes that lane's result
  `NaN`. Filter it out first if you want the maximum of the finite values.

## Summary

Use `amax()` when the goal is "largest value per reduction lane" and nothing
else.

- `amax()` gives one overall maximum
- `amax(dim)` gives one maximum per surviving lane
- `keepdim=true` keeps the reduced axis for later broadcasting
- tied winners split backward sensitivity evenly

## See Also

- [`Series::amin`](amin.md): minimum-value companion.
- [`Series::max`](max.md): broader surface that also includes elementwise
  pairwise maxima.
- [`Series::argmax`](argmax.md): positions of maxima instead of maximum values.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="allclose.html">
    <span class="label">Previous</span>
    <span class="title">allclose</span>
  </a>
  <a class="next-link" href="amin.html">
    <span class="label">Next</span>
    <span class="title">amin</span>
  </a>
</div>
