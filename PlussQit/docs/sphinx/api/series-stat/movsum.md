<!-- pluss-hand-authored -->

# Series::StatAccessor::movsum

[Back to Series::StatAccessor API Reference](../../series_stat_accessor_api_reference.md)

## Signatures

```cpp
Series Series::StatAccessor::movsum() const
Series Series::StatAccessor::movsum(std::int64_t window) const
Series Series::StatAccessor::movsum(std::int64_t window, std::int64_t dim) const
Series SeriesStatProxy::movsum() const
Series SeriesStatProxy::movsum(std::int64_t window) const
Series SeriesStatProxy::movsum(std::int64_t window, std::int64_t dim) const
```

## What It Does

`movsum()` returns a trailing sum at every input position. The output has the
same shape as the owner. The current value is included, and the window reaches
backward at most `window-1` positions along the selected axis.

No padding is invented at the beginning. Before a full window exists, the
method sums the available prefix. The no-argument form uses `window=3` on the
last axis.

## Window Boundaries

At position `i`, the contributing interval is
`[max(0, i-window+1), i]`. For `[1,2,3,4]` and window three, the intervals are
`[1]`, `[1,2]`, `[1,2,3]`, and `[2,3,4]`.

## Parameters And Return Shape

`window` is the maximum number of positions included and must be positive.
`dim` selects the axis that carries order; negative dimensions count from the
back. Omitting `dim` selects the last axis. The output always has the same
shape as the owner because every input position receives one trailing result.

## Examples

Example 1. The default window is three.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
std::cout << x.stat.movsum() << "\n";
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1, 3, 6, 9]
```

The first two outputs are expanding prefixes. Only at index two does the
window reach its requested width.

Example 2. A two-position window forgets `1` before producing the third total.

```cpp
std::cout << x.stat().movsum(2) << "\n";
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1, 3, 5, 7]
```

Example 3. Axis selection applies windows independently to every surviving lane.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
std::cout << x.stat.movsum(2, 0) << "\n";
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 2, 3],
  [5, 7, 9]]
```

Here `dim=0` runs downward through each column. The first row has no predecessor,
while the second row adds the corresponding value from the first row.

## Gradient

An input receives sensitivity from every output window that contains it. Thus
for `movsum(3).sum()` on four values, early inputs receive more contributions
than the final input because they appear in more trailing windows.

## Gotchas And Failure Modes

- `window` must be positive.
- A window larger than the axis is legal; it behaves as an expanding prefix.
- Omitted `dim` means the last axis, not a flattened input.

## See Also

- [`movmean`](movmean.md): divide each trailing sum by its actual window length.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="movstd.html"><span class="label">Previous</span><span class="title">movstd</span></a>
  <a class="next-link" href="movvar.html"><span class="label">Next</span><span class="title">movvar</span></a>
</div>
