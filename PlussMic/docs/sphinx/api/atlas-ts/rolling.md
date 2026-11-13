<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4e78008a0a95 -->

# AtlasTsAccessor::rolling

[Back to AtlasTsAccessor API Reference](../../atlas_ts_accessor_api_reference)

## Signatures

```cpp
Atlas AtlasTsAccessor::rolling(std::int64_t window, WindowStatistic statistic = WindowStatistic::Mean, const WindowOptions& options = {}) const
Atlas AtlasTsAccessor::rolling(std::string_view duration, WindowStatistic statistic = WindowStatistic::Mean, const StatisticOptions& options = {}, WindowClosure closure = WindowClosure::Right) const
```

## What It Does

Computes a trailing rolling window statistic over every bound feature, in **every
frame of the stack** — the same transform
[`DataFrameTsAccessor::rolling`](../dataframe-ts/rolling.md) applies to a single
frame, broadcast across the whole `Atlas` via [`Atlas::map_frames()`](../atlas/map_frames.md).

```{important} No structured-specs overload
Unlike [`DataFrameTsAccessor::rolling`](../dataframe-ts/rolling.md), which has a
third overload accepting `pluss::RangeView<DataFrame::WindowSpec>` for computing
several named rolling columns at once, `AtlasTsAccessor::rolling` only has the two
overloads shown above — a fixed row-count window, or a calendar-duration window.
```

- **Fixed row count** (`rolling(window, statistic, options)`): each row's window is
  the `window` rows ending at (and including) that row, within that frame. The first
  `window - 1` rows use a growing partial window rather than becoming `NULL`, unless
  `options.min_count` is raised to `window`.
- **Calendar duration** (`rolling(duration, statistic, options, closure)`): the same
  trailing-window idea, but each row's window spans a fixed span of time. Requires
  that frame's case labels to already be `Timeline`/`TimeIndex`.

## Parameters

**window** (`std::int64_t`, fixed-window overload)
    The trailing window size, in rows. Must be positive.

**statistic** (`WindowStatistic`, default `Mean`)
    The statistic computed over each window: `Sum`, `Mean`, `Minimum`, `Maximum`,
    `Var`, `Std`, `Sem`, `Skew`, or `Kurtosis`.

**options** (`const WindowOptions&`, fixed-window overload)
    `min_count` (default `1`), `ddof` (default `1`), `centered` (default `false`),
    `closure` (default `Right`) — see
    [`DataFrameTsAccessor::rolling`](../dataframe-ts/rolling.md) for the full field
    breakdown.

**duration** (`std::string_view`, calendar-duration overload)
    A duration string (for example `"2D"`) defining each row's time-based window
    width, within that frame's own time axis.

**closure** (`WindowClosure`, calendar-duration overload, default `Right`)
    Which end of the time window is inclusive.

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's bound
    feature replaced by its rolling statistic.

## Examples

Example 1. A fixed-size trailing rolling mean, stacked across two frames.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({{"load", Slate(pluss::make_series({10.0, 20.0, 15.0, 30.0}))}});
auto q2 = DataFrame({{"load", Slate(pluss::make_series({12.0, 18.0, 14.0, 28.0}))}});
auto atlas = Atlas({q1, q2});

auto out = atlas.ts().rolling(2);
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
===========
     | load
-----------
0    | 10  
1    | 15  
2    | 17.5
3    | 22.5
-----------
[4 cs x 1 feat]
===========
     | load
-----------
0    | 12  
1    | 15  
2    | 16  
3    | 21  
-----------
[4 cs x 1 feat]
```

With the default `min_count = 1`, row `0` in each frame doesn't wait for a full
2-row window — its output is just itself.

Example 2. The calendar-duration overload, using each frame's own daily `Timeline`
case labels.

```cpp
auto d1 = DataFrame(
    {{"value", Slate(pluss::make_series({10.0, 20.0, 30.0, 5.0}))}},
    Slate(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z",
        "2024-01-03T00:00:00Z", "2024-01-04T00:00:00Z"})));
auto d2 = DataFrame(
    {{"value", Slate(pluss::make_series({1.0, 2.0, 3.0, 4.0}))}},
    Slate(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z",
        "2024-01-03T00:00:00Z", "2024-01-04T00:00:00Z"})));
auto dur_atlas = Atlas({d1, d2});

auto dur_out = dur_atlas.ts().rolling("2D");
std::cout << dur_out.frame(0) << "\n" << dur_out.frame(1);
```

### Output

```text
==================
           | value
------------------
2024-01-01 | 10   
2024-01-02 | 15   
2024-01-03 | 25   
2024-01-04 | 17.5 
------------------
[4 cs x 1 feat]
==================
           | value
------------------
2024-01-01 | 1    
2024-01-02 | 1.5  
2024-01-03 | 2.5  
2024-01-04 | 3.5  
------------------
[4 cs x 1 feat]
```

Each frame's `"2D"` window looks back two calendar days along that frame's own time
axis, independently of the other frame's dates.

## Notes

- See the important callout above: unlike the single-frame accessor, there is no
  structured-specs overload here.
- The fixed-window overload's default options mean the first `window - 1` rows use
  a growing partial window — set `min_count = window` to require a full window
  everywhere instead.
- The calendar-duration overload requires that frame's case labels to already be
  `Timeline`/`TimeIndex` (or `Atlas::ts()` was built with an explicit axis).
- Each frame's rolling computation is independent — no window ever spans across
  frames.

## See Also

- [`DataFrameTsAccessor::rolling`](../dataframe-ts/rolling.md): the single-frame
  operation this method broadcasts across the stack, including the structured-specs
  overload this accessor lacks.
- [`AtlasTsAccessor::expanding`](expanding.md): the same engine with the window
  growing from the start of each frame's series instead of a fixed size.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="returns.html">
    <span class="label">Previous</span>
    <span class="title">returns</span>
  </a>
  <a class="next-link" href="sort.html">
    <span class="label">Next</span>
    <span class="title">sort</span>
  </a>
</div>
