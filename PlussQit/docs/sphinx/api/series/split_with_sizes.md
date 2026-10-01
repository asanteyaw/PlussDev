<!-- pluss-hand-authored -->

# Series::split_with_sizes

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
std::vector<Series> Series::split_with_sizes(
    pluss::msharp::RangeView::Int split_sizes,
    std::int64_t dim = 0) const
```

## Partition An Axis By Exact Lengths

`split_with_sizes()` partitions one axis into consecutive pieces whose lengths
are supplied explicitly. It exists for boundaries known from a schema, packed
record, or earlier size calculation. Unlike [`split()`](split.md), no final
remainder is inferred; the requested lengths must account for the axis exactly.

The return type is `std::vector<Series>` because the number of pieces is known
only at runtime. Every element is a zero-copy slice view over the receiver.
Pieces preserve dtype, platform, untouched dimensions, and the receiver's
strides; splitting columns can therefore produce valid non-dense views.

## Parameters And Validation

| Parameter | Meaning | Default |
| --- | --- | --- |
| `split_sizes` | Non-empty sequence of non-negative piece lengths. Their sum must equal `size(dim)`. | Required |
| `dim` | Axis partitioned; negative values count from the end. | `0` |

A zero length is meaningful and produces an empty piece at that boundary.
Negative lengths, an empty size list, a wrong total, rank-zero input, and an
out-of-range axis are rejected. The checks occur before returning a partial
result.

Because pieces alias the receiver, materialize with [`clone()`](clone.md) when
independent ownership is required. Reverse mode uses slice provenance: a loss
through one piece contributes only to the source region covered by that piece.

## Examples

### 1. Three Explicit One-Dimensional Regions

```cpp
auto x = pluss::make_series({0LL, 1LL, 2LL, 3LL, 4LL, 5LL});
auto parts = x.split_with_sizes({2, 1, 3});
std::cout << "count=" << parts.size() << "\n";
for (std::size_t i = 0; i < parts.size(); ++i)
  std::cout << "part[" << i << "]=" << parts[i] << "\n";
```

### Output

```text
count=3
part[0]=pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[0, 1]
part[1]=pluss::Series(shape=[1], strides=[1], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=[2]
part[2]=pluss::Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[3, 4, 5]
```

### 2. Preserve An Empty Region

```cpp
auto parts = x.split_with_sizes({2, 0, 4});
std::cout << "count=" << parts.size() << "\n";
for (std::size_t i = 0; i < parts.size(); ++i)
  std::cout << "part[" << i << "]=" << parts[i] << "\n";
```

### Output

```text
count=3
part[0]=pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[0, 1]
part[1]=pluss::Series(shape=[0], strides=[1], dtype=int64, platform=cpu, numel=0, contiguous=true)
[]
part[2]=pluss::Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[2, 3, 4, 5]
```

### 3. Partition Columns With A Negative Axis

```cpp
auto table = pluss::make_series({
    {4.0, 1.0, 4.0, 2.0},
    {3.0, 5.0, 0.0, 5.0},
});
auto parts = table.split_with_sizes({1, 3}, -1);
std::cout << parts[0] << "\n" << parts[1];
```

### Output

```text
pluss::Series(shape=[2,1], strides=[4,1], dtype=float64, platform=cpu, numel=2, contiguous=false)
data=[[4],
  [3]]
pluss::Series(shape=[2,3], strides=[4,1], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[1, 4, 2],
  [5, 0, 5]]
```

### 4. Reverse Through Only One Piece

```cpp
auto x = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0, 5.0});
x.tracks_slope_(true);
auto parts = x.split_with_sizes({2, 1, 3});
parts[2].sum().reverse();
std::cout << x.sensitivity();
```

### Output

```text
pluss::Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[0, 0, 0, 1, 1, 1]
```

### 5. Require An Exact Total

```cpp
try { (void)x.split_with_sizes({2, 2}); }
catch (const std::exception& error) {
  std::string message = error.what();
  std::cout << message.substr(0, message.find('\n'));
}
```

### Output

```text
split_with_sizes: sizes must sum to size(dim) (4 != 6)
```

## See Also

- [`Series::split`](split.md): fixed-size pieces with an inferred remainder.
- [`Series::chunk`](chunk.md): request a target piece count.
- [`Series::tensor_split`](tensor_split.md): split by count or cut positions.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="split.html"><span class="label">Previous</span><span class="title">split</span></a>
  <a class="next-link" href="sparse_resize.html"><span class="label">Next</span><span class="title">sparse_resize</span></a>
</div>
