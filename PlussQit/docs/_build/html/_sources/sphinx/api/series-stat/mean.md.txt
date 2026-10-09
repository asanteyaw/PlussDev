<!-- pluss-hand-authored -->

# Series::StatAccessor::mean

[Back to Series::StatAccessor API Reference](../../series_stat_accessor_api_reference.md)

## Signatures

```cpp
Series Series::StatAccessor::mean() const
Series Series::StatAccessor::mean(std::int64_t dim, bool keepdim = false) const
Series SeriesStatProxy::mean() const
Series SeriesStatProxy::mean(std::int64_t dim, bool keepdim = false) const
```

## What It Does

`mean()` replaces a collection of values with its arithmetic average. With no
axis it summarizes the entire owner. With `dim`, it independently averages
each lane along that axis. `x.stat.mean(...)` and `x.stat().mean(...)` are
equivalent; the latter merely creates a proxy before invoking the operation.

This is a reduction, not a shape-preserving transform. The values belonging to
each selected lane are replaced by one floating-point result. Use
[`center`](center.md) when the goal is instead to subtract those means and keep
every input position.

## Parameters

**dim** (`std::int64_t`)
    Selects the axis whose lanes are averaged. On shape `[2,3]`, `dim=0`
    groups the two values in each column; `dim=1` groups the three values in
    each row. Negative dimensions count from the back, so `-1` selects the
    final axis.

**keepdim** (`bool`, default `false`)
    If `false`, the reduced axis disappears. If `true`, it remains with size
    one. This changes only geometry, not the calculated means.

## Returns

A floating-point `Series`. Global reduction has shape `[]`; dimensional
reduction removes `dim` unless `keepdim=true`.

## Reduction Geometry

For shape `[2,3]`, `dim=0` averages the two rows and returns three column means;
`dim=1` averages the three columns and returns two row means. `keepdim=true`
retains the reduced axis at size one, which is useful when the result will be
broadcast back over the input.

## Gradient

Each input in a reduced lane of length `N` receives `1/N` of that lane's
upstream sensitivity. Unlike `sum()`, gradient magnitude therefore does not
grow merely because a batch or sequence has more elements.

## Examples

Example 1. A global mean treats the entire table as one population.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
std::cout << x.stat.mean() << "\n";
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=3.5
```

Example 2. Reducing `dim=0` computes one mean for each column.

```cpp
std::cout << x.stat.mean(0) << "\n";
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2.5, 3.5, 4.5]
```

Example 3. Reducing rows while retaining the axis produces shape `[2,1]`,
which can broadcast back across the three columns.

```cpp
std::cout << x.stat().mean(-1, true) << "\n";
```

### Output

```text
pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[2],
  [5]]
```

The proxy spelling in Example 3 changes only how the operation is reached.
`x.stat.mean(...)` and `x.stat().mean(...)` share the same owner and semantics.

## Gotchas And Failure Modes

- Negative dimensions count from the back; `-1` is the last axis.
- Integral inputs promote to floating output because averages can be fractional.
- `keepdim` changes shape, not values.
- Invalid dimensions fail through the shared reduction contract.

## See Also

- [`Series::mean`](../series/mean.md): the same operation on the root surface.
- [`movmean`](movmean.md): one trailing average at every position.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="logpdf.html"><span class="label">Previous</span><span class="title">logpdf</span></a>
  <a class="next-link" href="movmax.html"><span class="label">Next</span><span class="title">movmax</span></a>
</div>
