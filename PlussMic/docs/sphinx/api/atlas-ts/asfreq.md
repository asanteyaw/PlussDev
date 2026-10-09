<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 765db046f092 -->

# AtlasTsAccessor::asfreq

[Back to AtlasTsAccessor API Reference](../../atlas_ts_accessor_api_reference)

## Signature

```cpp
AtlasTsAccessor::asfreq(std::string_view freq, std::int64_t step = 1, pluss::subsp::stats::ts::GapFillMode fill_mode = pluss::subsp::stats::ts::GapFillMode::Forward, std::optional<double> constant = std::nullopt, pluss::subsp::stats::ts::ResampleOp op = pluss::subsp::stats::ts::ResampleOp::Last) const -> Atlas
```

## What It Does

[`resample()`](resample.md)s every bound feature, in **every frame of the stack**,
onto the regular grid implied by `freq` using `op` to combine any bucket with more
than one source observation, then immediately runs [`fill_gaps()`](fill_gaps.md)
with `fill_mode`/`constant` over the resampled result — the same transform
[`DataFrameTsAccessor::asfreq`](../dataframe-ts/asfreq.md) applies to a single
frame, broadcast across the whole `Atlas` via [`Atlas::map_frames()`](../atlas/map_frames.md).

## Parameters

**freq** (`std::string_view`)
    The bucket frequency, exactly as in [`resample()`](resample.md).

**step** (`std::int64_t`, default `1`)
    A frequency multiplier, exactly as in [`resample()`](resample.md).

**fill_mode** (`GapFillMode`, default `Forward`)
    The [`fill_gaps()`](fill_gaps.md) strategy applied to empty buckets after
    resampling, within each frame.

**constant** (`std::optional<double>`, default `std::nullopt`)
    The fill value when `fill_mode == GapFillMode::Constant`.

**op** (`ResampleOp`, default `Last`)
    How a bucket with more than one source observation is combined — `Mean`, `Sum`,
    `Min`, `Max`, `First`, `Last`, or `Count`.

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's bound
    features resampled onto the regular grid and gaps filled according to
    `fill_mode`.

## Examples

Example 1. Resample two frames to a daily grid with the default `op = Last`,
forward-filling each frame's empty day.

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

auto out = atlas.ts().asfreq("D");
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
==================
           | value
------------------
2024-01-01 | 20   
2024-01-02 | 20   
2024-01-03 | 5    
------------------
[3 cs x 1 feat]
==================
           | value
------------------
2024-01-01 | 2    
2024-01-02 | 2    
2024-01-03 | 4    
------------------
[3 cs x 1 feat]
```

Each frame's `2024-01-01` bucket keeps its own last observation (`20` for
`frame(0)`, `2` for `frame(1)`); each frame's empty `2024-01-02` bucket forward-fills
from that frame's own `2024-01-01`, never the other frame's value.

## Notes

- `asfreq()` is exactly `resample(freq, step)` followed by
  `.mean()`/`.sum()`/etc.-equivalent bucketing via `op`, then
  `fill_gaps(fill_mode, constant)`, run independently on every frame.
- The default `op = Last` matches the common "most recent observation stands for
  the period" convention, same as the single-frame method's default.
- Requires each frame's own case labels to already be `Timeline`/`TimeIndex`.

## See Also

- [`DataFrameTsAccessor::asfreq`](../dataframe-ts/asfreq.md): the single-frame
  operation this method broadcasts across the stack.
- [`AtlasTsAccessor::resample`](resample.md): the bucketing step this method runs
  before filling gaps.
- [`AtlasTsAccessor::fill_gaps`](fill_gaps.md): the gap-fill step this method runs
  after resampling.
- [`AtlasTsResampler::asfreq`](../atlas-ts-resample/asfreq.md): a related but
  distinct operation — a plain reindex (no aggregation), reached via
  [`resample()`](resample.md)'s chainable form.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="diff.html">
    <span class="label">Next</span>
    <span class="title">diff</span>
  </a>
</div>
