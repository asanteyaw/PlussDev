<!-- pluss-hand-authored -->

# Series::sort_pair

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
std::pair<Series, Series> Series::sort_pair(
    std::int64_t dim = -1,
    bool descending = false,
    bool stable = false) const
```

## Ordering Values Without Losing Their Origin

`sort_pair()` orders every lane of one axis and returns the ordered values
together with the source positions that supplied them. `.first` is the value
Series; `.second` is an `int64` Series of local indices. For every output
coordinate `q`, the defining relationship is:

```text
result.first[q] == self gathered at result.second[q] along dim
```

Use this form when an ordering must also be applied to aligned data, or when a
later step needs to explain where an ordered value came from. Use
[`sort()`](sort.md) when values alone are sufficient and
[`argsort()`](argsort.md) when only the permutation is needed.

The two outputs come from one ordering kernel. This matters for equal values:
running a value sort and an independent index sort could otherwise choose
different valid orders when stability is disabled.

## Parameters And Shape

| Parameter | Meaning | Default |
| --- | --- | --- |
| `dim` | Axis ordered independently; negative axes count from the end. | `-1` |
| `descending` | `false` orders low to high; `true` orders high to low. | `false` |
| `stable` | Preserve source order among equivalent values. | `false` |

Both outputs retain the receiver's shape and platform. Values retain its
dtype; indices are `int64`. The receiver is not mutated. Ordering a non-final
axis may return strided, non-contiguous outputs because the kernel moves that
axis temporarily and then restores the original axis order.

With `stable=false`, equivalent values have no promised relative order. With
`stable=true`, their indices remain in source order. NaN values sort after
finite values in ascending order and before them in descending order.
Complex values have no total ordering and are rejected.

## Reverse Mode And Cost

The value channel is differentiable. Reverse mode scatters each incoming
sensitivity back through the exact index in `.second`; the integer index
channel is discrete. CPU execution performs one sort per lane and writes both
outputs in that pass. A stable sort can require more work than an unstable
sort, so request it only when equal-value provenance is part of the contract.

## Examples

### 1. Order Rows And Keep Their Column Positions

```cpp
auto table = pluss::make_series({
    {4.0, 1.0, 4.0, 2.0},
    {3.0, 5.0, 0.0, 5.0},
});
auto result = table.sort_pair();
std::cout << "values=" << result.first << "\nindices=" << result.second;
```

### Output

```text
values=pluss::Series(shape=[2,4], strides=[4,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[1, 2, 4, 4],
  [0, 3, 5, 5]]
indices=pluss::Series(shape=[2,4], strides=[4,1], dtype=int64, platform=cpu, numel=8, contiguous=true)
data=[[1, 3, 0, 2],
  [2, 0, 1, 3]]
```

The first row's ordered values came from columns `1, 3, 0, 2`.

### 2. Order Down Columns, Descending And Stable

```cpp
auto result = table.sort_pair(0, true, true);
std::cout << "values=" << result.first << "\nindices=" << result.second;
```

### Output

```text
values=pluss::Series(shape=[2,4], strides=[1,2], dtype=float64, platform=cpu, numel=8, contiguous=false)
data=[[4, 5, 4, 5],
  [3, 1, 0, 2]]
indices=pluss::Series(shape=[2,4], strides=[1,2], dtype=int64, platform=cpu, numel=8, contiguous=false)
data=[[0, 1, 0, 1],
  [1, 0, 1, 0]]
```

Each column is an independent two-element lane. The restored layout is
strided because axis `0`, rather than the final axis, was ordered.

### 3. Reverse Mode Uses The Returned Permutation

```cpp
auto x = pluss::make_series({3.0, 1.0, 2.0});
x.tracks_slope_(true);
auto result = x.sort_pair();
(result.first * pluss::make_series({10.0, 20.0, 30.0})).sum().reverse();
std::cout << "indices=" << result.second << "\ndx=" << x.sensitivity();
```

### Output

```text
indices=pluss::Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[1, 2, 0]
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[30, 10, 20]
```

The weight attached to each sorted rank returns to the source position named
by the paired index.

An undefined receiver, rank-zero receiver, out-of-range axis, or complex dtype
is rejected. Empty lanes are valid for sorting and produce empty outputs.

## See Also

- [`Series::sort`](sort.md): ordered values only.
- [`Series::argsort`](argsort.md): source permutation only.
- [`Series::topk_pair`](topk_pair.md): retain only the best `k` values and positions.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sort.html"><span class="label">Previous</span><span class="title">sort</span></a>
  <a class="next-link" href="split.html"><span class="label">Next</span><span class="title">split</span></a>
</div>
