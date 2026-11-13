<!-- pluss-hand-authored -->

# Series::sort

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::sort() const
Series Series::sort(std::int64_t dim) const
Series Series::sort(std::int64_t dim, bool descending) const
Series Series::sort(std::int64_t dim, bool descending, bool stable) const
```

## What It Does

`sort()` returns values in order and leaves the source unchanged. It is a
within-axis transformation: it never combines values across unrelated lanes.
By default it orders the final axis ascending. `dim` chooses another axis,
`descending` changes direction, and `stable` controls the relative order of
equal values.

Use [`sort_pair()`](sort_pair.md) when sorted values and their original
positions must be returned together.

## Parameters

**dim** (`std::int64_t`): axis to order. Negative dimensions count from the
end.

**descending** (`bool`): `false` gives smallest-to-largest; `true` gives
largest-to-smallest.

**stable** (`bool`): preserve equal values' source order when `true`. With
`false`, their order has no public guarantee.

## Returns

`sort()` returns ordered values with the source shape and dtype.

## Gradient

`sort()` moves values without altering them, so its **value channel is
differentiable**: in backpropagation each output's gradient is routed back to
the position that value held in the input. What is *not* differentiable is the
ordering itself — which input position maps to which rank is a discrete
function of the data, held fixed during the backward pass, so no gradient flows
through a rank change when two values cross.

## Examples

Example 1. The default sorts the final axis ascending.

```cpp
auto scores = pluss::make_series({3.0, 1.0, 4.0, 1.0, 5.0});
pluss::series::print(std::cout, scores.sort());
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1.0000, 1.0000, 3.0000, 4.0000, 5.0000]
```

Example 2. Descending order selects the opposite ordering direction.

```cpp
std::cout << scores.sort(0, true);
```

### Output

```text
pluss::Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[5, 4, 3, 1, 1]
```

Example 3. Axis choice changes which values compete. `dim=0` orders down each
column; `dim=-1` orders across each row.

```cpp
auto table = pluss::make_series({{4.0, 1.0, 3.0}, {2.0, 5.0, 1.0}});
std::cout << table.sort(0) << "\n";
std::cout << table.sort(-1);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[1,2], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[2, 1, 1],
  [4, 5, 3]]
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 3, 4],
  [1, 2, 5]]
```

The first result is non-contiguous because the ordered axis is not final. That
is a layout detail, not a different value ordering.

Example 4. An absent dimension is rejected rather than interpreted as a flat
sort.

```cpp
try { std::cout << scores.sort(1); }
catch (const std::exception& error) { std::cout << error.what(); }
```

### Output

```text
sort: dim out of range
```

Example 5. The value channel is differentiable — a weighted sum of the sorted
values sends each weight back to the position that supplied that rank.

```cpp
auto x = pluss::make_series({3.0, 1.0, 4.0, 1.0, 5.0});
x.tracks_slope_(true);

auto weights = pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0});
(x.sort() * weights).sum().reverse();

pluss::series::print(std::cout, x.sensitivity());
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[30.0000, 10.0000, 40.0000, 20.0000, 50.0000]
```

The ascending order is `[1, 1, 3, 4, 5]`, drawn from input positions
`[1, 3, 0, 2, 4]`, so weight `30` (the third) lands on position `0`, weight
`40` on position `2`, and so on.

## Gotchas And Failure Modes

- `sort()` returns values; [`argsort()`](argsort.md) returns the indices needed
  to recreate the order.
- Equal-value ordering is a contract only with `stable=true`.
- Sorting non-final axes can produce a non-contiguous result. Call
  [`contiguous()`](contiguous.md) only when a downstream operation requires
  contiguous layout.
- The value channel is differentiable; the ordering decision is not.
- Complex-valued inputs are rejected.

## Summary

Use `sort()` for a complete ordered value sequence and `sort_pair()` whenever
the source positions must travel with that sequence.

## See Also

- [`Series::argsort`](argsort.md)
- [`Series::topk`](topk.md)
- [`Series::kthvalue`](kthvalue.md)


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="softplus.html">
    <span class="label">Previous</span>
    <span class="title">softplus</span>
  </a>
  <a class="next-link" href="sort_pair.html">
    <span class="label">Next</span>
    <span class="title">sort_pair</span>
  </a>
</div>
