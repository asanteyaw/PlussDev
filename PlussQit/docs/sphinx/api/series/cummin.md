<!-- pluss-hand-authored -->

# Series::cummin

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::cummin() const
Series Series::cummin(std::int64_t dim) const
```

## What It Does

`cummin()` computes a running minimum along one axis — the mirror of
[`cummax()`](cummax.md).

It is a scan, not a reduction: the axis keeps its full length and each position
is replaced by the smallest value seen from the start of the axis up to and
including that position.

For a row `[5, 3, 4, 1, 2]` the running minimum is `[5, 3, 3, 1, 1]` — it never
increases, and it latches onto each new low until a smaller one appears.

The no-argument form scans axis `0`. Pass `dim` to scan a different axis;
negative values count from the back.

## Parameters

**dim** (`std::int64_t`, optional)

The axis to scan. Omitted: axis `0`. Negative values count from the back. The
result keeps the same shape.

## Returns

`Series`
    The running minimum along `dim`, same shape and dtype as the input (an
    integer series stays integer).

## Gradient

`cummin()` is differentiable.

Its backward routes the sensitivity of each output position to the single input
position that held the running minimum there, so a value that stays the low for
a long stretch collects the gradient of every output in that stretch. On a tie
the earliest position keeps the record, matching the forward.

## Examples

Example 1. The running minimum of a 1-D row.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({5.0, 3.0, 4.0, 1.0, 2.0}).cummin(0));
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[5.0000, 3.0000, 3.0000, 1.0000, 1.0000]
```

Example 2. `dim` selects the axis.

```cpp
auto m = pluss::make_series({
    {5.0, 2.0, 4.0},
    {3.0, 6.0, 1.0},
});

std::cout << "dim 0:\n" << m.cummin(0);
std::cout << "\ndim 1:\n" << m.cummin(1);
```

### Output

```text
dim 0:
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[5, 2, 4],
  [3, 2, 1]]
dim 1:
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[5, 2, 2],
  [3, 3, 1]]
```

Example 3. Integer input keeps its dtype.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({4, 2, 3, 1}, pluss::ScalarType::Int64).cummin(0));
```

### Output

```text
Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[4, 2, 2, 1]
```

Example 4. The backward sends each output's sensitivity to the position that
held the low.

```cpp
auto x = pluss::make_series({5.0, 3.0, 4.0, 1.0, 2.0});
x.tracks_slope_(true);

x.cummin(0).sum().reverse();

std::cout << "dx=" << x.sensitivity();
```

### Output

```text
dx=pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1, 2, 0, 2, 0]
```

Position `1` held the low at outputs `1` and `2`; position `3` held it at
outputs `3` and `4`. Positions `2` and `4` were never the low and get `0`.

## Gotchas And Failure Modes

- `cummin()` is a scan, not a reduction. To collapse the axis to a single
  minimum, use [`min()`](min.md).
- A `NaN` is skipped by the running comparison rather than propagated.
- Ties keep the earliest position, in both the output and the gradient.

## Summary

`cummin(x)` is the running minimum along an axis:

- shape- and dtype-preserving scan; default axis is `0`
- monotonically non-increasing along the scanned axis
- differentiable: each output's sensitivity flows to the position that held the
  record, ties to the earliest

## See Also

- [`Series::cummax`](cummax.md): the running maximum.
- [`Series::min`](min.md): the whole-axis minimum, which removes the axis.
- [`Series::cumsum`](cumsum.md): the additive prefix scan.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cummax.html">
    <span class="label">Previous</span>
    <span class="title">cummax</span>
  </a>
  <a class="next-link" href="cumprod.html">
    <span class="label">Next</span>
    <span class="title">cumprod</span>
  </a>
</div>
