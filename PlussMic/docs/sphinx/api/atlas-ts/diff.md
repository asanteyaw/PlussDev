<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3fdba2dcb05d -->

# AtlasTsAccessor::diff

[Back to AtlasTsAccessor API Reference](../../atlas_ts_accessor_api_reference)

## Signature

```cpp
AtlasTsAccessor::diff(std::int64_t periods = 1) const -> Atlas
```

## What It Does

Computes the absolute period-over-period change of every bound feature in **every
frame of the stack**: `out[i] = x[i] - base[i]`, where `base` is `x` shifted by
`periods` within that same frame — the same transform
[`DataFrameTsAccessor::diff`](../dataframe-ts/diff.md) applies to a single frame,
broadcast across the whole `Atlas` via [`Atlas::map_frames()`](../atlas/map_frames.md).
A cell is `NULL` wherever `x[i]` or `base[i]` is missing. `periods = 0` produces all
zeros rather than throwing.

## Parameters

**periods** (`std::int64_t`, default `1`)
    The shift distance used to build the comparison base, within each frame
    independently.

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's bound
    feature replaced by its period-over-period absolute difference.

## Examples

Example 1. Compute the one-period difference of a numeric feature stacked across two
frames.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({{"load", Slate(pluss::make_series({10.0, 20.0, 15.0, 30.0}))}});
auto q2 = DataFrame({{"load", Slate(pluss::make_series({12.0, 18.0, 14.0, 28.0}))}});
auto atlas = Atlas({q1, q2});

auto out = atlas.ts().diff();
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
===========
     | load
-----------
0    |  nan
1    |  10 
2    | -5  
3    |  15 
-----------
[4 cs x 1 feat]
===========
     | load
-----------
0    |  nan
1    |  6  
2    | -4  
3    |  14 
-----------
[4 cs x 1 feat]
```

## Notes

- Unlike [`shift()`](../dataframe/shift.md)-based approaches you might build by hand,
  `diff()` handles `periods = 0` explicitly by returning all zeros rather than a
  frame of `NULL`.
- Each frame's difference is computed independently — no frame's computation crosses
  into another's data.
- `Atlas::ts()` auto-binds the numeric `Series` features on each frame, same as
  [`DataFrame::ts()`](../dataframe/ts.md) does for a single frame.

## See Also

- [`DataFrameTsAccessor::diff`](../dataframe-ts/diff.md): the single-frame operation
  this method broadcasts across the stack.
- [`AtlasTsAccessor::returns`](returns.md), [`AtlasTsAccessor::log_returns`](log_returns.md):
  the fractional and logarithmic counterparts to this absolute difference.
- [`AtlasTsAccessor::lag`](lag.md), [`AtlasTsAccessor::lead`](lead.md): the underlying
  shift this method's comparison base is built from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="asfreq.html">
    <span class="label">Previous</span>
    <span class="title">asfreq</span>
  </a>
  <a class="next-link" href="ewm.html">
    <span class="label">Next</span>
    <span class="title">ewm</span>
  </a>
</div>
