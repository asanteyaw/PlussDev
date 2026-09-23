<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 937aae15068f -->

# AtlasTimelineAccessor::valid

[Back to AtlasTimelineAccessor API Reference](../../atlas_timeline_accessor_api_reference)

## Signature

```cpp
AtlasTimelineAccessor::valid() const -> Atlas
```

## What It Does

Replaces every `Timeline`/`TimeIndex` feature in **every frame of the stack** with a
boolean `Series` flagging whether the row is a real (non-`NaT`) timestamp — the logical
inverse of [`isnat()`](isnat.md) — the same transform
[`DataFrame::tl().valid()`](../dataframe-timeline/valid.md) applies to a single frame,
broadcast across the whole `Atlas` via [`map_frames()`](../atlas/map_frames.md).

```{important} valid()'s own output is never masked to NULL — same as the single-frame method
Like [`DataFrameTimelineAccessor::valid`](../dataframe-timeline/valid.md), this is one of
the few `.tl()` methods whose result is never itself displayed as `"NULL"`: it exists
precisely to answer "is this row real," so the `NaT` row reports `false`, not `NULL`.
Nearly every other calendar-field method on this accessor shows `"NULL"` for a `NaT` row
instead.
```

## Parameters

`None`

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's time
    feature replaced by a `Bool` Series: `true` where the row is a real timestamp,
    `false` where it's `NaT`.

## Examples

Example 1. Flag real timestamps across a two-frame stack where only one frame has any
`NaT` rows.

```cpp
using namespace pluss::table;

auto valid_tl = pluss::make_timeline({
    "2024-01-31T09:15:30Z",
    "2024-02-29T23:45:00Z",
    "2024-06-15T12:00:00Z",
});
auto ticks = DataFrame({{"ts", Thread(valid_tl)}})
                 .tl().epoch().features("ts").as_series().to_vec<std::int64_t>();
ticks[2] = pluss::codeh::Timeline::kNaT;
std::vector<std::int64_t> sizes{(std::int64_t)ticks.size()};
auto tl_with_nat = pluss::timeline::from_epoch(
    pluss::msharp::RangeView::Of<std::int64_t>(ticks.data(), ticks.size()),
    pluss::msharp::RangeView::Of<std::int64_t>(sizes.data(), sizes.size()));

auto q1 = DataFrame({{"ts", Thread(tl_with_nat)}});
auto q2 = DataFrame({{"ts", Thread(pluss::make_timeline({
    "2024-07-06T14:00:00Z",
    "2023-03-01T00:00:00Z",
    "2023-12-31T23:59:59Z",
}))}});
auto atlas = Atlas({q1, q2});

auto out = atlas.tl().valid();
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
============
     | ts   
------------
0    | true 
1    | true 
2    | false
------------
[3 cs x 1 feat]
===========
     | ts  
-----------
0    | true
1    | true
2    | true
-----------
[3 cs x 1 feat]
```

`frame(0)`'s row `2` — the only genuinely `NaT` row across the stack — flags `false`;
every other row in either frame, including all of `frame(1)`, flags `true`.

```{important} make_timeline({..., "", ...}) currently throws instead of producing NaT
The commonly-shown pattern of dropping an empty string into `make_timeline({...})` to
build a `NaT` row currently throws `"moment: failed to parse datetime"` instead (a
confirmed library issue). The example above works around it by building a real
`Timeline` first, then splicing `pluss::codeh::Timeline::kNaT` into its raw epoch
nanoseconds via `pluss::from_epoch_nanos` — a genuine `NaT` value, not a masked-null row.
```

## Notes

- See the important callouts above: `valid()` never masks its own output to `NULL`, and
  (for now) real `NaT` values must be built via the epoch-nanoseconds splice shown above
  rather than an empty-string literal.
- `Atlas::tl()` requires **every** feature on each stacked frame to already be
  `Timeline`/`TimeIndex`-kind, checked lazily per frame the first time a method like this
  one is actually called — see [`Atlas::tl`](../atlas/tl.md).
- Each frame is transformed independently — a `NaT` row in one frame has no effect on any
  other frame's result.

## See Also

- [`DataFrameTimelineAccessor::valid`](../dataframe-timeline/valid.md): the single-frame
  operation this method broadcasts across the stack.
- [`AtlasTimelineAccessor::isnat`](isnat.md): the logical inverse, with the same
  never-`NULL` behavior.
- [`Atlas::map_frames`](../atlas/map_frames.md): the general per-frame transform this
  accessor is built on.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="utc.html">
    <span class="label">Previous</span>
    <span class="title">utc</span>
  </a>
  <a class="next-link" href="week.html">
    <span class="label">Next</span>
    <span class="title">week</span>
  </a>
</div>
