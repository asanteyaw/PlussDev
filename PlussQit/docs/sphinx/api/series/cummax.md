<!-- pluss-hand-authored -->

# Series::cummax

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::cummax() const
Series Series::cummax(std::int64_t dim) const
```

## What It Does

`cummax()` computes a running maximum along one axis.

Like [`cumsum()`](cumsum.md) it is a scan, not a reduction: the axis keeps its
full length, and each position is replaced by the largest value seen from the
start of the axis up to and including that position.

For a row `[1, 3, 2, 5, 4]` the running maximum is `[1, 3, 3, 5, 5]` — it never
decreases, and it "latches" onto each new record until a larger one appears.

The no-argument form scans axis `0`. Pass `dim` to scan a different axis;
negative values count from the back.

## Parameters

**dim** (`std::int64_t`, optional)

The axis to scan. Omitted: axis `0`. Negative values count from the back. The
result keeps the same shape, because a scan does not remove an axis.

## Returns

`Series`
    The running maximum along `dim`, with the same shape and dtype as the
    input (an integer series stays integer).

## Gradient

`cummax()` is differentiable.

Its backward routes the sensitivity of each output position to the single input
position that held the running maximum there. Formally, `d/dx_i cummax(x)[t]`
is `1` when `i` is the position of the running max at `t` and `0` otherwise, so
a value that stays the record for a long stretch collects the gradient of every
output in that stretch. On a tie the earliest position keeps the record,
matching the forward.

## Examples

The examples build up from the basic scan to the axis argument and the
backward.

Example 1. The running maximum of a 1-D row.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({1.0, 3.0, 2.0, 5.0, 4.0}).cummax(0));
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1.0000, 3.0000, 3.0000, 5.0000, 5.0000]
```

The `2` and the `4` never appear in the output — the running maximum has
already passed them.

Example 2. `dim` selects the axis; each lane is scanned independently.

```cpp
auto m = pluss::make_series({
    {1.0, 4.0, 2.0},
    {3.0, 2.0, 5.0},
});

std::cout << "dim 0:\n" << m.cummax(0);
std::cout << "\ndim 1:\n" << m.cummax(1);
```

### Output

```text
dim 0:
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 4, 2],
  [3, 4, 5]]
dim 1:
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 4, 4],
  [3, 3, 5]]
```

Down the columns the running max compares row `0` with row `1`. Across the rows
it walks left to right.

Example 3. Integer input keeps its dtype.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({1, 3, 2, 5}, pluss::ScalarType::Int64).cummax(0));
```

### Output

```text
Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[1, 3, 3, 5]
```

Example 4. A `NaN` never becomes the running maximum, so the scan carries the
last real record past it.

```cpp
double nan = std::nan("");

pluss::series::print(
    std::cout,
    pluss::make_series({1.0, nan, 3.0, 2.0}).cummax(0));
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1.0000, 1.0000, 3.0000, 3.0000]
```

Example 5. The backward sends each output's sensitivity to the position that
held the record.

```cpp
auto x = pluss::make_series({1.0, 3.0, 2.0, 5.0, 4.0});
x.tracks_slope_(true);

x.cummax(0).sum().reverse();

std::cout << "dx=" << x.sensitivity();
```

### Output

```text
dx=pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1, 2, 0, 2, 0]
```

Position `1` held the record at outputs `1` and `2`, so it collects `2`.
Position `3` held it at outputs `3` and `4`, so it also collects `2`. Positions
`2` and `4` were never the record and get `0`.

Example 6. On a tie the earliest position keeps the record and so keeps the
gradient.

```cpp
auto x = pluss::make_series({3.0, 3.0, 2.0, 4.0});
x.tracks_slope_(true);

x.cummax(0).sum().reverse();

std::cout << "dx=" << x.sensitivity();
```

### Output

```text
dx=pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[3, 0, 0, 1]
```

The second `3` never strictly exceeds the first, so position `0` stays the
record through outputs `0`, `1`, and `2`.

## Gotchas And Failure Modes

- `cummax()` is a scan: the output keeps the full axis length. To collapse the
  axis to a single maximum, use [`max()`](max.md).
- A `NaN` is skipped by the running comparison rather than propagated. If you
  need `NaN`-aware behavior, screen the input first.
- Ties keep the earliest position, both in the forward output and in the
  gradient.

## Summary

`cummax(x)` is the running maximum along an axis:

- shape- and dtype-preserving scan; default axis is `0`
- monotonically non-decreasing along the scanned axis
- differentiable: each output's sensitivity flows to the position that held the
  record, ties to the earliest

## See Also

- [`Series::cummin`](cummin.md): the running minimum.
- [`Series::max`](max.md): the whole-axis maximum, which removes the axis.
- [`Series::cumsum`](cumsum.md): the additive prefix scan.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cross.html">
    <span class="label">Previous</span>
    <span class="title">cross</span>
  </a>
  <a class="next-link" href="cummin.html">
    <span class="label">Next</span>
    <span class="title">cummin</span>
  </a>
</div>
