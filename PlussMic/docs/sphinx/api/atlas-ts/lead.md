<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5ecdf2fdabd5 -->

# AtlasTsAccessor::lead

[Back to AtlasTsAccessor API Reference](../../atlas_ts_accessor_api_reference)

## Signature

```cpp
Atlas AtlasTsAccessor::lead(std::int64_t periods = 1) const
```

## What It Does

Shifts every bound feature `periods` rows backward, within **every frame of the
stack** independently: `out[i] = x[i + periods]` where that index is in bounds,
`NULL` otherwise — the same transform
[`DataFrameTsAccessor::lead`](../dataframe-ts/lead.md) applies to a single frame,
broadcast across the whole `Atlas` via [`Atlas::map_frames()`](../atlas/map_frames.md).
A negative `periods` delegates to [`lag()`](lag.md) with the sign flipped.

## Parameters

**periods** (`std::int64_t`, default `1`)
    The number of rows to shift backward, within each frame. Negative values behave
    as [`lag(-periods)`](lag.md) instead.

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's bound
    feature led by `periods` rows.

## Examples

Example 1. Lead a numeric feature by one row, stacked across two frames.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({{"load", Slate(pluss::make_series({10.0, 20.0, 15.0, 30.0}))}});
auto q2 = DataFrame({{"load", Slate(pluss::make_series({12.0, 18.0, 14.0, 28.0}))}});
auto atlas = Atlas({q1, q2});

auto out = atlas.ts().lead();
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
===========
     | load
-----------
0    | 20  
1    | 15  
2    | 30  
3    | nan 
-----------
[4 cs x 1 feat]
===========
     | load
-----------
0    | 18  
1    | 14  
2    | 28  
3    | nan 
-----------
[4 cs x 1 feat]
```

Each frame's last row has no successor within *that* frame, so it's `NULL` — the two
frames never borrow rows from each other.

## Notes

- `lead(periods)` with a negative `periods` is exactly `lag(-periods)`.
- Each frame's shift never crosses into another frame's data.
- `Atlas::ts()` auto-binds the numeric `Series` features on each frame, same as
  [`DataFrame::ts()`](../dataframe/ts.md) does for a single frame.

## See Also

- [`DataFrameTsAccessor::lead`](../dataframe-ts/lead.md): the single-frame operation
  this method broadcasts across the stack.
- [`AtlasTsAccessor::lag`](lag.md): the mirror-image shift.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lag.html">
    <span class="label">Previous</span>
    <span class="title">lag</span>
  </a>
  <a class="next-link" href="log_returns.html">
    <span class="label">Next</span>
    <span class="title">log_returns</span>
  </a>
</div>
