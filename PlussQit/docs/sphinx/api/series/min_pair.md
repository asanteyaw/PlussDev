<!-- pluss-hand-authored -->

# Series::min_pair

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
std::pair<Series, Series> Series::min_pair(
    std::int64_t dim,
    bool keepdim = false) const
```

## Why This Method Exists

`min_pair()` reduces each lane of one axis while preserving the provenance of
the selected minimum. Its first result member is the minimum value; its second
is the local `int64` position along `dim` from which that value came.

This paired contract matters whenever the position is part of the answer. A
minimum price without the period that produced it, or a minimum distance
without the candidate that attained it, is often incomplete. Keeping both
channels aligned also avoids independently evaluating [`min()`](min.md) and
[`argmin()`](argmin.md).

## Parameters

| Parameter | Meaning | Default |
| --- | --- | --- |
| `dim` | Axis reduced into one selected minimum per lane. Negative axes count from the end. | Required |
| `keepdim` | Retain `dim` with extent `1` in both result members. | `false` |

There is no whole-Series overload. `dim` always names the axis within which
positions are reported. On shape `[R, C]`, `dim=1` produces column indices for
each row, while `dim=0` produces row indices for each column.

## Return Contract

The method returns `std::pair<Series, Series>` with synchronized members:

| Member | Dtype | Shape | Meaning |
| --- | --- | --- | --- |
| `.first` | Source dtype | Reduced shape | Selected minimum values |
| `.second` | `int64` | Same as `.first` | Local source indices along `dim` |

With `keepdim=false`, the selected axis is removed. With `keepdim=true`, it
remains with extent `1`, which is useful when the value result will be
broadcast against the source. Both members stay on the source platform and own
their result storage.

## Selection And Gradient Rules

Equal minima are resolved by selecting the first position along the reduced
axis. The CPU kernel writes the value and index in one lane scan; reverse mode
sends incoming sensitivity only to the index stored in `.second`. The index
channel is integer-valued and non-differentiable.

This differs deliberately from value-only `min(dim)`: without an accompanying
index, that reduction shares sensitivity across equal minima. If downstream
logic must agree on one concrete source position, use `min_pair()` and keep the
two members together.

NaN is not omitted. If the ordered reduction selects a NaN, `.first` contains
that NaN and `.second` identifies its selected position. An empty reduced axis
has no valid minimum and is rejected.

## Examples

Example 1. Destructuring exposes one minimum and one local position per row.

```cpp
auto values = pluss::make_series({
    {4.0, 1.0, 3.0},
    {2.0, 1.0, 6.0},
});

const auto [mins, indices] = values.min_pair(1);
std::cout << "values=" << mins << "\n";
std::cout << "indices=" << indices;
```

### Output

```text
values=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 1]
indices=pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[1, 1]
```

Example 2. `-1` selects the final axis, `keepdim=true` preserves it, and ties
route sensitivity to the first selected position.

```cpp
auto tied = pluss::make_series({
    {1.0, 1.0, 5.0},
    {-4.0, -4.0, 2.0},
});
tied.tracks_slope_(true);

const auto result = tied.min_pair(-1, true);
result.first.sum().reverse();

std::cout << "values=" << result.first << "\n";
std::cout << "indices=" << result.second << "\n";
std::cout << "dx=" << tied.sensitivity();
```

### Output

```text
values=pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[1],
  [-4]]
indices=pluss::Series(shape=[2,1], strides=[1,1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[[0],
  [0]]
dx=pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 0, 0],
  [1, 0, 0]]
```

Example 3. A scalar has one value and therefore one valid local position.

```cpp
const auto result = pluss::make_series(42.0).min_pair(-1);
std::cout << "value=" << result.first << "\n";
std::cout << "index=" << result.second;
```

### Output

```text
value=pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=42
index=pluss::Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=0
```

Example 4. Reducing an empty axis fails because no lane has a candidate value.

```cpp
try {
  std::cout << pluss::series::zeros({2, 0}).min_pair(1).first;
} catch (const std::exception& error) {
  std::string message = error.what();
  std::cout << message.substr(0, message.find('\n'));
}
```

### Output

```text
min_pair: cannot reduce an empty dimension
```

Undefined input, an out-of-range axis, complex ordered input, and an empty
reduced axis are errors. An empty non-reduced axis is valid and yields an empty
collection of lanes.

## See Also

- [`Series::min`](min.md): value-only reduction with tie-sharing reverse mode.
- [`Series::argmin`](argmin.md): index-only minimum reduction.
- [`Series::max_pair`](max_pair.md): paired maximum and source position.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="min.html">
    <span class="label">Previous</span>
    <span class="title">min</span>
  </a>
  <a class="next-link" href="minimum.html">
    <span class="label">Next</span>
    <span class="title">minimum</span>
  </a>
</div>
