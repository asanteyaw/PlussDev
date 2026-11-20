<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e339005afd46 -->

# AtlasTsAccessor::fill_gaps

[Back to AtlasTsAccessor API Reference](../../atlas_ts_accessor_api_reference)

## Signature

```cpp
Atlas AtlasTsAccessor::fill_gaps(pluss::subsp::stats::ts::GapFillMode mode = pluss::subsp::stats::ts::GapFillMode::Forward, std::optional<double> constant = std::nullopt) const
```

## What It Does

Fills missing (`NULL`) values in every bound feature, in **every frame of the
stack**, according to `mode` — the same transform
[`DataFrameTsAccessor::fill_gaps`](../dataframe-ts/fill_gaps.md) applies to a single
frame, broadcast across the whole `Atlas` via [`Atlas::map_frames()`](../atlas/map_frames.md).
`Forward`/`Backward` carry the nearest neighbor within that frame; `Zero`/`Constant`
fill every gap unconditionally.

## Parameters

**mode** (`GapFillMode`, default `Forward`)
    `Forward`, `Backward`, `Zero`, or `Constant` — see
    [`DataFrameTsAccessor::fill_gaps`](../dataframe-ts/fill_gaps.md) for the full
    behavior of each.

**constant** (`std::optional<double>`, default `std::nullopt`)
    The fill value for `GapFillMode::Constant`. Required (and only meaningful) when
    `mode == Constant`.

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's bound
    feature's gaps filled according to `mode`.

## Examples

Example 1. Forward-fill a gapped feature stacked across two frames.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({{"load", Shelf(pluss::make_series({10.0, 0.0, 0.0, 30.0}), pluss::make_series(std::vector<bool>{true, false, false, true}))}});
auto q2 = DataFrame({{"load", Shelf(pluss::make_series({12.0, 0.0, 14.0, 0.0}), pluss::make_series(std::vector<bool>{true, false, true, false}))}});
auto atlas = Atlas({q1, q2});

auto out = atlas.ts().fill_gaps();
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
===========
     | load
-----------
0    | 10  
1    | 10  
2    | 10  
3    | 30  
-----------
[4 cs x 1 feat]
===========
     | load
-----------
0    | 12  
1    | 12  
2    | 14  
3    | 14  
-----------
[4 cs x 1 feat]
```

Each frame's gaps are carried forward using only that frame's own values —
`frame(1)`'s row `3` carries forward `14` from `frame(1)`'s own row `2`, never a
value from `frame(0)`.

```{note} How the gapped columns are built
`pluss::make_series({10.0, std::nullopt, ...})` does not compile in this build (`DataReader`
has no `std::optional<double>` element-type support), so the example builds each column from
its values plus a validity mask: the `0.0` under a missing cell is only a placeholder. A
`NaN` would not do here, because a `NaN` is a value and not a gap (see
[Missing, NaN and infinity](../../guide/missing_nan_and_inf.md)).
```

## Notes

- `Forward`/`Backward` gaps with no qualifying neighbor within that frame (leading
  nulls under `Forward`, trailing nulls under `Backward`) remain `NULL`.
- `Constant` requires `constant` to be supplied; calling it without one throws.
- Each frame's gap-filling is independent — a gap in one frame is never filled from
  another frame's values.

## See Also

- [`DataFrameTsAccessor::fill_gaps`](../dataframe-ts/fill_gaps.md): the single-frame
  operation this method broadcasts across the stack, with the full set of
  `Forward`/`Backward`/`Zero`/`Constant` worked examples.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="expanding.html">
    <span class="label">Previous</span>
    <span class="title">expanding</span>
  </a>
  <a class="next-link" href="lag.html">
    <span class="label">Next</span>
    <span class="title">lag</span>
  </a>
</div>
