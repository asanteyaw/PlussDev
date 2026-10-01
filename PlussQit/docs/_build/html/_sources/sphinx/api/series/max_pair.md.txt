<!-- pluss-hand-authored -->

# Series::max_pair

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
std::pair<Series, Series> Series::max_pair(
    std::int64_t dim,
    bool keepdim = false) const
```

## Why This Method Exists

`max_pair()` answers two inseparable questions about every lane of an axis:
what is its largest value, and which source position supplied that value? It
returns those answers together so the value and its provenance cannot drift
apart through a second, independently written reduction.

The first member of the pair contains values. The second contains local
`int64` indices measured along `dim`. For a rank-2 input reduced across columns,
for example, each index is a column number within its row; it is not a flat
offset into storage.

Use `max_pair()` when later work needs both channels, such as retaining the
winning score while gathering an aligned label from the same position. Use
[`max()`](max.md) when only values matter, and [`argmax()`](argmax.md) when only
positions matter.

## Parameters

| Parameter | Meaning | Default |
| --- | --- | --- |
| `dim` | Axis whose lanes compete. Negative axes count backward from the final axis. | Required |
| `keepdim` | Keep the reduced axis with extent `1` in both returned Series. | `false` |

`dim` names exactly one axis; there is no whole-Series overload. For rank-2
shape `[R, C]`, reducing `dim=0` returns one winner per column, while reducing
`dim=1` or `dim=-1` returns one winner per row.

## Return Contract

The return value is `std::pair<Series, Series>`:

| Member | Dtype | Shape | Meaning |
| --- | --- | --- | --- |
| `.first` | Source dtype | Source shape with `dim` removed, or retained as `1` | Winning values |
| `.second` | `int64` | Exactly the same shape as `.first` | Each winner's local source index along `dim` |

Both members stay on the source platform. The source is not mutated. A scalar
source accepts `dim=0` or `dim=-1` and returns scalar value `self` paired with
scalar index `0`.

## Ties, NaN, And Reverse Mode

If several positions contain the same maximum, the pair selects the first
position along the reduced axis. The value and index are produced from that
same selection, so they always describe one another.

That provenance also defines reverse mode. The value member is differentiable,
and its incoming sensitivity is scattered back only to the reported index.
The integer index member is discrete and does not carry sensitivity. This is a
meaningful distinction from value-only `max(dim)`: that reduction has no index
to privilege and shares sensitivity across equal winners, whereas
`max_pair(dim).first` follows the one winner named by `.second`.

NaN participates in the ordered-extremum kernel rather than being silently
discarded. When NaN wins a lane, the paired index identifies the selected NaN
position. Use a NaN-specific reduction when omission is the intended policy.

## Execution Model

The CPU kernel scans each lane once and writes the winning value and its index
together, matching the two-output reduction model. Reverse mode reuses that
index as its scatter route rather than searching for the winner again.
Returned Series own their result buffers; neither is a view of the receiver.

## Examples

Example 1. Structured binding gives names to the value and provenance channels.

```cpp
auto scores = pluss::make_series({
    {1.0, 7.0, 3.0},
    {4.0, 7.0, 6.0},
});

const auto [values, indices] = scores.max_pair(1);
std::cout << "values=" << values << "\n";
std::cout << "indices=" << indices;
```

### Output

```text
values=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[7, 7]
indices=pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[1, 1]
```

Each row contributes one winner. The indices are column positions, so both
rows report `1` even though they occupy different flat storage ranges.

Example 2. A negative axis is normalized before reduction, and `keepdim=true`
retains that axis in both members for later broadcasting.

```cpp
const auto kept = scores.max_pair(-1, true);
std::cout << "values=" << kept.first << "\n";
std::cout << "indices=" << kept.second;
```

### Output

```text
values=pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[7],
  [7]]
indices=pluss::Series(shape=[2,1], strides=[1,1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[[1],
  [1]]
```

Example 3. Ties choose the first position, and reverse mode follows precisely
the index that the pair reports.

```cpp
auto tied = pluss::make_series({
    {5.0, 5.0, 1.0},
    {2.0, 4.0, 4.0},
});
tied.tracks_slope_(true);

const auto result = tied.max_pair(1);
result.first.sum().reverse();

std::cout << "values=" << result.first << "\n";
std::cout << "indices=" << result.second << "\n";
std::cout << "dx=" << tied.sensitivity();
```

### Output

```text
values=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[5, 4]
indices=pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[0, 1]
dx=pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 0, 0],
  [0, 1, 0]]
```

The second equal maximum in each row receives zero sensitivity because it is
not the position named by the paired index.

Example 4. A scalar still has one valid reduction position.

```cpp
const auto result = pluss::make_series(42.0).max_pair(0);
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

Example 5. A lane with no candidates cannot produce either a value or an
index, so reduction over an empty axis fails before allocating a result.

```cpp
try {
  std::cout << pluss::series::zeros({2, 0}).max_pair(1).first;
} catch (const std::exception& error) {
  std::string message = error.what();
  std::cout << message.substr(0, message.find('\n'));
}
```

### Output

```text
max_pair: cannot reduce an empty dimension
```

An undefined receiver, an out-of-range axis, complex ordered input, and an
empty reduced axis are rejected. Empty non-reduced axes are valid because they
contain no lanes requiring a winner.

## See Also

- [`Series::max`](max.md): value-only reduction with tie-sharing reverse mode.
- [`Series::argmax`](argmax.md): index-only ordered reduction.
- [`Series::min_pair`](min_pair.md): paired minimum and source position.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="max.html">
    <span class="label">Previous</span>
    <span class="title">max</span>
  </a>
  <a class="next-link" href="maximum.html">
    <span class="label">Next</span>
    <span class="title">maximum</span>
  </a>
</div>
