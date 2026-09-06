<!-- pluss-hand-authored -->

# SeriesStatAccessor::movmean

[Back to SeriesStatAccessor API Reference](../../series_stat_accessor_api_reference.md)

## Signatures

```cpp
Series SeriesStatAccessor::movmean() const
Series SeriesStatAccessor::movmean(std::int64_t window) const
Series SeriesStatAccessor::movmean(std::int64_t window, std::int64_t dim) const
Series SeriesStatProxy::movmean() const
Series SeriesStatProxy::movmean(std::int64_t window) const
Series SeriesStatProxy::movmean(std::int64_t window, std::int64_t dim) const
```

## What It Does

`movmean()` computes a trailing arithmetic mean at every position while
preserving shape. The default is a three-value window on the last axis.

Startup positions use only values that exist. Their divisor is the actual
partial-window length, not the requested full window. This avoids implicit
zero padding and makes the prefix statistically meaningful.

## Parameters And Return Shape

`window` is the maximum trailing width and must be greater than zero. `dim`
selects the ordered axis; omitted `dim` means the last axis, and negative dims
count from the back. One mean is produced per input position, so shape is
preserved.

## Examples

Example 1. The default window is three, with divisors `1`, `2`, `3`, and `3`.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
std::cout << x.stat.movmean() << "\n";
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1, 1.5, 2, 3]
```

Example 2. Reducing the width to two makes every full result the mean of the
current value and its immediate predecessor.

```cpp
std::cout << x.stat().movmean(2) << "\n";
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1, 1.5, 2.5, 3.5]
```

For window three, the third result is `(1+2+3)/3`; the fourth slides forward
to `(2+3+4)/3`.

Example 3. On a matrix, choose which direction represents time or order.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
std::cout << x.stat.movmean(2, 1) << "\n";
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 1.5, 2.5],
  [4, 4.5, 5.5]]
```

The rows do not mix: `dim=1` applies a separate trailing calculation to each
row.

Example 4. Reverse mode reveals how overlapping windows reuse inputs.

```cpp
auto tracked = pluss::make_series({1.0, 2.0, 3.0, 4.0});
tracked.tracks_slope_(true);
tracked.stat.movmean(3).sum().reverse();
std::cout << tracked.sensitivity() << "\n";
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1.8333333, 1.1666667, 0.66666667, 0.33333333]
```

## Gradient

Each output distributes its sensitivity equally across its actual window.
The nonuniform result is correct: early values participate in more outputs,
but each participation is weighted by that output's actual window length.

## Gotchas And Failure Modes

- The method is trailing, not centered or forward-looking.
- Partial prefixes divide by `1,2,...` until the full window is available.
- `window<=0` and invalid dimensions fail explicitly.

## See Also

- [`mean`](mean.md): reduce a lane to one mean.
- [`movsum`](movsum.md): trailing totals without count normalization.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="movmax.html"><span class="label">Previous</span><span class="title">movmax</span></a>
  <a class="next-link" href="movmin.html"><span class="label">Next</span><span class="title">movmin</span></a>
</div>
