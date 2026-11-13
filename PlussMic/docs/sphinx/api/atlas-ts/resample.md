<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2b7b63087bee -->

# AtlasTsAccessor::resample

[Back to AtlasTsAccessor API Reference](../../atlas_ts_accessor_api_reference)

## Signature

```cpp
AtlasTsResampler AtlasTsAccessor::resample(std::string_view freq, std::int64_t step = 1) const
```

## What It Does

Buckets every bound feature, in **every frame of the stack**, into regular
calendar/duration periods along that frame's own time axis, and returns a lazy,
chainable [`AtlasTsResampler`](../../atlas_ts_resampler_api_reference) — nothing is
aggregated until one of its own methods (`.mean()`, `.sum()`, `.min()`, `.max()`,
`.first()`, `.last()`, `.count()`, `.asfreq()`, `.ffill()`, `.bfill()`,
`.nearest()`) is called.

```{important} No op-taking overload
Unlike [`DataFrameTsAccessor::resample`](../dataframe-ts/resample.md), which has
overloads that take an explicit `ResampleOp` and return the aggregated `DataFrame`
directly in one call, `AtlasTsAccessor::resample` only has this single
chainable-form overload — always call one of the returned
[`AtlasTsResampler`](../../atlas_ts_resampler_api_reference)'s own methods to actually
aggregate.
```

## Parameters

**freq** (`std::string_view`)
    The bucket frequency — fixed units (`S`/`min`/`H`/`D`, optionally prefixed with
    a multiplier), calendar-aware `W`/`M`/`Y` buckets.

**step** (`std::int64_t`, default `1`)
    A frequency multiplier — for example `step = 6` with `freq = "H"` produces
    6-hour buckets.

## Returns

`AtlasTsResampler`
    A chainable accessor bound to every frame in the stack; nothing is aggregated
    until one of its own methods is called.

## Examples

Example 1. Daily buckets across two frames sharing the same date range, aggregated
with `.mean()`.

```cpp
using namespace pluss::table;

auto q1 = DataFrame(
    {{"value", Slate(pluss::make_series({10.0, 20.0, 30.0, 5.0}))}},
    Slate(pluss::make_timeline({
        "2024-01-01T08:00:00Z", "2024-01-01T20:00:00Z",
        "2024-01-03T10:00:00Z", "2024-01-03T15:00:00Z"})));
auto q2 = DataFrame(
    {{"value", Slate(pluss::make_series({1.0, 2.0, 3.0, 4.0}))}},
    Slate(pluss::make_timeline({
        "2024-01-01T08:00:00Z", "2024-01-01T20:00:00Z",
        "2024-01-03T10:00:00Z", "2024-01-03T15:00:00Z"})));
auto atlas = Atlas({q1, q2});

auto resampled = atlas.ts().resample("D");
auto out = resampled.mean();
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
==================
           | value
------------------
2024-01-01 | 15   
2024-01-02 | nan  
2024-01-03 | 17.5 
------------------
[3 cs x 1 feat]
==================
           | value
------------------
2024-01-01 | 1.5  
2024-01-02 | nan  
2024-01-03 | 3.5  
------------------
[3 cs x 1 feat]
```

Both frames build the same 3-tick daily grid (spanning each frame's own min/max
tick), but `frame(1)`'s empty `2024-01-02` bucket and populated buckets are computed
purely from `frame(1)`'s own four values — never `frame(0)`'s.

## Notes

- See the important callout above: unlike the single-frame accessor, there is no
  op-taking overload that returns the aggregated `Atlas` in one call — always chain
  through the returned resampler.
- Each frame builds its own regular grid from its own minimum/maximum tick — the
  grids for two frames need not line up at all if their date ranges differ.
- Requires each frame's own case labels to already be `Timeline`/`TimeIndex`.

## See Also

- [`DataFrameTsAccessor::resample`](../dataframe-ts/resample.md): the single-frame
  operation this method broadcasts across the stack, including the op-taking
  overloads this accessor lacks.
- [`AtlasTsResampler`](../../atlas_ts_resampler_api_reference): the chainable accessor
  this method returns.
- [`AtlasTsAccessor::asfreq`](asfreq.md): resample-then-fill-gaps in one call.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="log_returns.html">
    <span class="label">Previous</span>
    <span class="title">log_returns</span>
  </a>
  <a class="next-link" href="returns.html">
    <span class="label">Next</span>
    <span class="title">returns</span>
  </a>
</div>
