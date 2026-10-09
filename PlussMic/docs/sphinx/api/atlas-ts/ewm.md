<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ab783581639f -->

# AtlasTsAccessor::ewm

[Back to AtlasTsAccessor API Reference](../../atlas_ts_accessor_api_reference)

## Signature

```cpp
AtlasTsAccessor::ewm(double alpha, WindowStatistic statistic = WindowStatistic::Mean, bool adjusted = true, bool bias_corrected = true, std::int64_t min_count = 1, bool ignore_nulls = false) const -> Atlas
```

## What It Does

An exponentially weighted moving statistic over every bound feature, in **every
frame of the stack** — the same transform
[`DataFrameTsAccessor::ewm`](../dataframe-ts/ewm.md) applies to a single frame,
broadcast across the whole `Atlas` via [`Atlas::map_frames()`](../atlas/map_frames.md).
Each row's output blends every prior row within that same frame (and itself), with
weight decaying geometrically the further back a row is. Only
`WindowStatistic::Mean`, `Var`, and `Std` are supported.

## Parameters

**alpha** (`double`)
    Smoothing factor in `(0, 1]`. Larger values weight recent observations more
    heavily.

**statistic** (`WindowStatistic`, default `Mean`)
    `Mean`, `Var`, or `Std` — any other value throws.

**adjusted** (`bool`, default `true`)
    Normalized-weighted-average form (`true`) versus the classic recursive form
    (`false`) — see
    [`DataFrameTsAccessor::ewm`](../dataframe-ts/ewm.md) for the exact formulas.
    `adjusted = false` requires the bound feature to have no nulls anywhere in that
    frame.

**bias_corrected** (`bool`, default `true`)
    For `Var`/`Std` only: whether to apply a finite-sample bias correction.

**min_count** (`std::int64_t`, default `1`)
    Minimum non-null observations seen so far, within that frame, required for a
    row's output to be non-`NULL`.

**ignore_nulls** (`bool`, default `false`)
    When `true`, a null observation is skipped entirely when computing decay weights.

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's bound
    feature replaced by its exponentially weighted statistic.

## Examples

Example 1. The default exponentially weighted mean over a numeric feature, stacked
across two frames.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({{"load", Slate(pluss::make_series({10.0, 20.0, 15.0, 30.0}))}});
auto q2 = DataFrame({{"load", Slate(pluss::make_series({12.0, 18.0, 14.0, 28.0}))}});
auto atlas = Atlas({q1, q2});

auto out = atlas.ts().ewm(0.5);
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
==================
     | load       
------------------
0    | 10         
1    | 16.66666667
2    | 15.71428571
3    | 23.33333333
------------------
[4 cs x 1 feat]
==================
     | load       
------------------
0    | 12         
1    | 16         
2    | 14.85714286
3    | 21.86666667
------------------
[4 cs x 1 feat]
```

Each frame's decay weighting restarts at that frame's own row `0` — `frame(1)`'s row
`0` output is just `12`, unaffected by `frame(0)`'s values.

## Notes

- `adjusted = false` requires the bound feature to have no nulls anywhere within
  that frame — it throws otherwise, same as the single-frame method.
- `alpha = 1.0` degenerates to returning the input unchanged.
- `bias_corrected` only affects `Var`/`Std`.
- Each frame's decay weighting is independent — no row ever blends values from
  another frame.

## See Also

- [`DataFrameTsAccessor::ewm`](../dataframe-ts/ewm.md): the single-frame operation
  this method broadcasts across the stack, with the full set of `adjusted`/
  `bias_corrected`/`ignore_nulls` worked examples.
- [`AtlasTsAccessor::expanding`](expanding.md): an equally-weighted "everything so
  far" statistic, as opposed to this method's exponentially decaying weights.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="diff.html">
    <span class="label">Previous</span>
    <span class="title">diff</span>
  </a>
  <a class="next-link" href="expanding.html">
    <span class="label">Next</span>
    <span class="title">expanding</span>
  </a>
</div>
