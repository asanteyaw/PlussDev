<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0e6c6f8b3f6a -->

# AtlasTsAccessor::expanding

[Back to AtlasTsAccessor API Reference](../../atlas_ts_accessor_api_reference)

## Signature

```cpp
Atlas AtlasTsAccessor::expanding(WindowStatistic statistic = WindowStatistic::Mean, const StatisticOptions& options = {}) const
```

## What It Does

The same trailing-window engine as [`rolling()`](rolling.md), except each frame's
window always starts at that frame's row `0` and grows to include every row up to
and including the current one — the same transform
[`DataFrameTsAccessor::expanding`](../dataframe-ts/expanding.md) applies to a single
frame, broadcast across the whole `Atlas` via [`Atlas::map_frames()`](../atlas/map_frames.md).

## Parameters

**statistic** (`WindowStatistic`, default `Mean`)
    The statistic computed over each row's growing window — see
    [`rolling()`](rolling.md) for the full set.

**options** (`const StatisticOptions&`)
    `ddof` (default `1`), `min_count` (default `1`).

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's bound
    feature replaced by its expanding statistic.

## Examples

Example 1. An expanding mean over a numeric feature, stacked across two frames.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({{"load", Slate(pluss::make_series({10.0, 20.0, 15.0, 30.0}))}});
auto q2 = DataFrame({{"load", Slate(pluss::make_series({12.0, 18.0, 14.0, 28.0}))}});
auto atlas = Atlas({q1, q2});

auto out = atlas.ts().expanding();
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
============
     | load 
------------
0    | 10   
1    | 15   
2    | 15   
3    | 18.75
------------
[4 cs x 1 feat]
==================
     | load       
------------------
0    | 12         
1    | 15         
2    | 14.66666667
3    | 18         
------------------
[4 cs x 1 feat]
```

Each frame's window grows from its own row `0` — `frame(1)`'s row `3` averages only
`frame(1)`'s own four values (`12, 18, 14, 28` → `18`), never mixing in `frame(0)`'s
data.

## Notes

- `expanding()` is exactly [`rolling()`](rolling.md) with a window as large as each
  frame's whole series.
- `Var`/`Std` require the growing window's observation count to exceed `ddof`
  (default `1`), same as [`rolling()`](rolling.md).
- Each frame's expanding computation is independent — no window ever spans across
  frames.

## See Also

- [`DataFrameTsAccessor::expanding`](../dataframe-ts/expanding.md): the single-frame
  operation this method broadcasts across the stack.
- [`AtlasTsAccessor::rolling`](rolling.md): the fixed-size trailing window this
  method is a special case of.
- [`AtlasTsAccessor::ewm`](ewm.md): a different "whole history so far" statistic,
  weighted by exponential decay instead of equally.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ewm.html">
    <span class="label">Previous</span>
    <span class="title">ewm</span>
  </a>
  <a class="next-link" href="fill_gaps.html">
    <span class="label">Next</span>
    <span class="title">fill_gaps</span>
  </a>
</div>
