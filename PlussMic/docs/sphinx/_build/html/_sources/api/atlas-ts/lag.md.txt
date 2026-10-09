<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 44ac0d705b39 -->

# AtlasTsAccessor::lag

[Back to AtlasTsAccessor API Reference](../../atlas_ts_accessor_api_reference)

## Signature

```cpp
AtlasTsAccessor::lag(std::int64_t periods = 1) const -> Atlas
```

## What It Does

Shifts every bound feature `periods` rows forward, within **every frame of the
stack** independently: `out[i] = x[i - periods]` where that index is in bounds,
`NULL` otherwise — the same transform
[`DataFrameTsAccessor::lag`](../dataframe-ts/lag.md) applies to a single frame,
broadcast across the whole `Atlas` via [`Atlas::map_frames()`](../atlas/map_frames.md).
A negative `periods` delegates to [`lead()`](lead.md) with the sign flipped.

## Parameters

**periods** (`std::int64_t`, default `1`)
    The number of rows to shift forward, within each frame. Negative values behave
    as [`lead(-periods)`](lead.md) instead.

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's bound
    feature lagged by `periods` rows.

## Examples

Example 1. Lag a numeric feature by one row, stacked across two frames.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({{"load", Slate(pluss::make_series({10.0, 20.0, 15.0, 30.0}))}});
auto q2 = DataFrame({{"load", Slate(pluss::make_series({12.0, 18.0, 14.0, 28.0}))}});
auto atlas = Atlas({q1, q2});

auto out = atlas.ts().lag();
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
===========
     | load
-----------
0    | nan 
1    | 10  
2    | 20  
3    | 15  
-----------
[4 cs x 1 feat]
===========
     | load
-----------
0    | nan 
1    | 12  
2    | 18  
3    | 14  
-----------
[4 cs x 1 feat]
```

Each frame shifts independently — `frame(1)`'s row `1` shows `frame(1)`'s own row
`0` value (`12`), never a value borrowed from `frame(0)`.

## Notes

- `lag(periods)` with a negative `periods` is exactly `lead(-periods)` — the two
  methods share one implementation, differing only in shift direction.
- Each frame's shift never crosses into another frame's data.
- `Atlas::ts()` auto-binds the numeric `Series` features on each frame, same as
  [`DataFrame::ts()`](../dataframe/ts.md) does for a single frame.

## See Also

- [`DataFrameTsAccessor::lag`](../dataframe-ts/lag.md): the single-frame operation
  this method broadcasts across the stack.
- [`AtlasTsAccessor::lead`](lead.md): the mirror-image shift.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fill_gaps.html">
    <span class="label">Previous</span>
    <span class="title">fill_gaps</span>
  </a>
  <a class="next-link" href="lead.html">
    <span class="label">Next</span>
    <span class="title">lead</span>
  </a>
</div>
