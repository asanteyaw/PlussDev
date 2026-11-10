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

| Operation | Purpose |
|---|---|
| [`returns`](../atlas-ts/returns.md), [`log_returns`](../atlas-ts/log_returns.md), [`diff`](../atlas-ts/diff.md) | Compute changes between observations. |
| [`lag`](../atlas-ts/lag.md), [`lead`](../atlas-ts/lead.md) | Shift observations backward or forward. |
| [`rolling`](../atlas-ts/rolling.md), [`expanding`](../atlas-ts/expanding.md), [`ewm`](../atlas-ts/ewm.md) | Calculate windowed statistics. |
| [`fill_gaps`](../atlas-ts/fill_gaps.md), [`sort`](../atlas-ts/sort.md) | Fill missing values or order observations. |
| [`resample`](../atlas-ts/resample.md), [`asfreq`](../atlas-ts/asfreq.md) | Change the time frequency. |

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
