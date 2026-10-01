<!-- pluss-hand-authored -->

# Series::topk_pair

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
std::pair<Series, Series> Series::topk_pair(
    std::int64_t k,
    std::int64_t dim = -1,
    bool largest = true,
    bool sorted = true) const
```

## Select Values And Provenance Together

`topk_pair()` selects `k` entries from every lane of one axis and returns both
their values and their local source positions. It is the provenance-preserving
selection operation: `.first` contains values and `.second` contains `int64`
indices suitable for gathering aligned data along the same axis.

Unlike a full sort, selection need not establish an order among entries that
cannot enter the result. Use it when only an extreme subset matters. Use
[`topk()`](topk.md) for values alone and [`sort_pair()`](sort_pair.md) when all
entries and their positions are required.

## Parameters And Contract

| Parameter | Meaning | Default |
| --- | --- | --- |
| `k` | Number selected from each lane. Must satisfy `0 <= k <= size(dim)`. | Required |
| `dim` | Selection axis; negative values count from the end. | `-1` |
| `largest` | Select largest values when `true`, smallest when `false`. | `true` |
| `sorted` | Put selected values in rank order when `true`; leave their order unspecified when `false`. | `true` |

Both outputs have the receiver's rank. Their extent on `dim` becomes `k`; all
other extents are unchanged. The value dtype and platform are preserved, and
indices are `int64` on that platform. `k=0` is valid and returns matching empty
outputs. A negative or oversized `k` is an error, not a request to clamp.

Equal boundary values may be represented by any qualifying source position;
there is no stability option. When `sorted=false`, only membership is
contractual: consumers must not infer rank from output position.

## Reverse Mode And Execution

Selection writes values and indices in one kernel. On CPU, `sorted=true` uses
partial ordering rather than sorting the entire lane. `sorted=false` can use
partitioning and may be cheaper. The value output is differentiable and
scatters sensitivity to the exact returned positions; indices do not carry
sensitivity.

## Examples

### 1. Two Largest Values Per Row

```cpp
auto table = pluss::make_series({
    {4.0, 1.0, 4.0, 2.0},
    {3.0, 5.0, 0.0, 5.0},
});
auto result = table.topk_pair(2, 1);
std::cout << "values=" << result.first << "\nindices=" << result.second;
```

### Output

```text
values=pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[4, 4],
  [5, 5]]
indices=pluss::Series(shape=[2,2], strides=[2,1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[[2, 0],
  [3, 1]]
```

Both ties qualify. Their relative order is deliberately unspecified.

### 2. Select The Smallest Values With A Negative Axis

```cpp
auto result = table.topk_pair(2, -1, false, true);
std::cout << "values=" << result.first << "\nindices=" << result.second;
```

### Output

```text
values=pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 2],
  [0, 3]]
indices=pluss::Series(shape=[2,2], strides=[2,1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[[1, 3],
  [2, 0]]
```

### 3. Request Membership Without Ordering

```cpp
auto x = pluss::make_series({9LL, 1LL, 7LL, 3LL, 5LL});
auto result = x.topk_pair(3, 0, true, false);
std::cout << "values=" << result.first << "\nindices=" << result.second;
```

### Output

```text
values=pluss::Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[9, 7, 5]
indices=pluss::Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[0, 2, 4]
```

This run happens to appear ordered; `sorted=false` does not promise that
order, so portable code treats these three entries as an unordered set.

### 4. Select Nothing Without Special-Case Control Flow

```cpp
auto result = table.topk_pair(0, 1);
std::cout << "values=" << result.first << "\nindices=" << result.second;
```

### Output

```text
values=pluss::Series(shape=[2,0], strides=[0,1], dtype=float64, platform=cpu, numel=0, contiguous=true)
[]
indices=pluss::Series(shape=[2,0], strides=[0,1], dtype=int64, platform=cpu, numel=0, contiguous=true)
[]
```

### 5. Reject An Oversized Selection

```cpp
try { (void)table.topk_pair(5, 1); }
catch (const std::exception& error) {
  std::string message = error.what();
  std::cout << message.substr(0, message.find('\n'));
}
```

### Output

```text
topk_pair: k must be <= size(dim)
```

Invalid axes, negative `k`, complex ordered input, and undefined receivers are
also rejected.

## See Also

- [`Series::topk`](topk.md): selected values only.
- [`Series::sort_pair`](sort_pair.md): complete ordering with provenance.
- [`Series::max_pair`](max_pair.md): one maximum and one source index per lane.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="topk.html"><span class="label">Previous</span><span class="title">topk</span></a>
  <a class="next-link" href="trace.html"><span class="label">Next</span><span class="title">trace</span></a>
</div>
