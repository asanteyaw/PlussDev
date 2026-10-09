<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3a3b14b6311d -->

# Atlas::ts

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
Atlas::ts() const -> AtlasTsAccessor
```

## What It Does

The entry point into the time-series operations accessor, applied across every frame in
the atlas. `ts()` itself binds the current atlas and returns an `AtlasTsAccessor`:

```cpp
class AtlasTsAccessor {
public:
  Atlas returns(std::int64_t periods = 1) const;
  Atlas log_returns(std::int64_t periods = 1) const;
  Atlas diff(std::int64_t periods = 1) const;
  Atlas lag(std::int64_t periods = 1) const;
  Atlas lead(std::int64_t periods = 1) const;
  Atlas rolling(std::int64_t window, WindowStatistic statistic = WindowStatistic::Mean,
                const WindowOptions& options = {}) const;
  Atlas rolling(std::string_view duration, WindowStatistic statistic = WindowStatistic::Mean,
                const StatisticOptions& options = {},
                WindowClosure closure = WindowClosure::Right) const;
  Atlas expanding(WindowStatistic statistic = WindowStatistic::Mean,
                  const StatisticOptions& options = {}) const;
  Atlas ewm(double alpha, WindowStatistic statistic = WindowStatistic::Mean,
            bool adjusted = true, bool bias_corrected = true,
            std::int64_t min_count = 1, bool ignore_nulls = false) const;
  Atlas fill_gaps(GapFillMode mode = GapFillMode::Forward,
                  std::optional<double> constant = std::nullopt) const;
  Atlas sort(bool ascending = true, bool nulls_last = true) const;
  AtlasTsResampler resample(std::string_view freq, std::int64_t step = 1) const;
  Atlas asfreq(std::string_view freq, std::int64_t step = 1, /* ... */) const;
};
```

This is the same operation family as [`DataFrame::ts()`](../dataframe/ts.md)/
[`DataFrameTsAccessor`](../../dataframe_ts_accessor_api_reference), applied per-frame across
the whole atlas. Every method on the returned `AtlasTsAccessor` has its own page under
[`AtlasTsAccessor` API Reference](../../atlas_ts_accessor_api_reference), including
[`resample()`](../atlas-ts/resample.md)'s chainable
[`AtlasTsResampler`](../../atlas_ts_resampler_api_reference) return type.

## Parameters

`None`

## Returns

`AtlasTsAccessor`
    A binder for the current atlas — call one of its methods to perform a time-series
    operation across every frame.

## Examples

Example 1. Compute period-over-period returns across every frame.

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Slate(pluss::make_series({1.0, 2.0, 3.0}))},
});
auto atlas = pluss::table::Atlas({f0});

auto out = atlas.ts().returns();
std::cout << out.dloc(0);
```

### Output

```text
============
     | sales
------------
0    | nan  
1    | 1    
2    | 0.5  
------------
[3 cs x 1 feat]
```

The first row shows `nan` — there's no prior period to compute a return against. The cell
is genuinely `NULL` (confirmed via `dloc(0).isnull()`, which reports `true` for that row);
it just renders here as `nan` rather than the `NULL` text
[`DataFrame::ts()`](../dataframe/ts.md)`.returns()`'s own docs show for the identical
case — a display-only difference between the two paths, not a difference in validity.

## Notes

- `ts()` alone does no work — it only binds the atlas.
- Every method runs independently per frame — see the individual
  [`AtlasTsAccessor`](../../atlas_ts_accessor_api_reference) method pages for exactly how
  each one treats the depth stack.

## See Also

- [`AtlasTsAccessor` API Reference](../../atlas_ts_accessor_api_reference): every method
  on the accessor this call returns.
- [`DataFrame::ts`](../dataframe/ts.md): the single-frame delegate each `AtlasTsAccessor`
  method broadcasts.
- [`Atlas::tl`](tl.md): the calendar/timestamp-field accessor, fully documented on its own
  pages, as distinct from this time-series accessor.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_string.html">
    <span class="label">Previous</span>
    <span class="title">to_string</span>
  </a>
  <a class="next-link" href="unique.html">
    <span class="label">Next</span>
    <span class="title">unique</span>
  </a>
</div>
