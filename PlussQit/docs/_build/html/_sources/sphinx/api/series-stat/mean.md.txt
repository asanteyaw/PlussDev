<!-- pluss-hand-authored -->

# SeriesStatAccessor::mean

[Back to SeriesStatAccessor API Reference](../../series_stat_accessor_api_reference.md)

## Signatures

```cpp
Series SeriesStatAccessor::mean() const
Series SeriesStatAccessor::mean(std::int64_t dim, bool keepdim = false) const
Series SeriesStatProxy::mean() const
Series SeriesStatProxy::mean(std::int64_t dim, bool keepdim = false) const
```

## What It Does

`mean()` replaces a collection of values with its arithmetic average. With no
axis it summarizes the entire owner. With `dim`, it independently averages
each lane along that axis. `x.stat.mean(...)` and `x.stat().mean(...)` are
equivalent; the latter merely creates a proxy before invoking the operation.

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

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
std::cout << x.stat.mean() << "\n";
std::cout << x.stat.mean(0) << "\n";
std::cout << x.stat().mean(1, true) << "\n";
```

### Output

```text
data=3.5
data=[2.5, 3.5, 4.5]
data=[[2],
  [5]]
```

The three results answer different questions: one average for the full table,
one per column, and one per row while preserving broadcast alignment.

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
