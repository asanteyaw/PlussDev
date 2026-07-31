<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f597075c945d -->

# Atlas::tl

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
Atlas::tl() const -> AtlasTimelineAccessor
```

## What It Does

The entry point into the calendar/timestamp-operations accessor, applied across every
frame in the atlas — extracting year, month, day, weekday, timezone conversion,
flooring/ceiling to a unit, and more, one method per operation. `tl()` binds the current
atlas and hands back an `AtlasTimelineAccessor`, documented in full on its own pages (see
[`AtlasTimelineAccessor` API Reference](../../atlas_timeline_accessor_api_reference)).

```{important} tl() itself never validates — the per-feature check happens lazily, per frame
Unlike [`DataFrame::tl()`](../dataframe/tl.md) (which throws immediately if any feature on
the frame isn't `Timeline`/`TimeIndex`-kind), `Atlas::tl()` never throws — it just binds.
The validation happens later, independently per frame, the first time you actually call a
method on the returned accessor (e.g. `.year()`). If any frame in the atlas has a
non-timeline feature at that point, that frame's own `DataFrame::tl()` throws the same
message [`DataFrame::tl`](../dataframe/tl.md) documents (pointing at
`df.features({"date"}).tl()` as the fix) — but only when an accessor method is actually
invoked, not at `tl()`'s own call site.
```

## Parameters

`None`

## Returns

`AtlasTimelineAccessor`
    A binder for the current atlas — call one of its methods to actually perform a
    calendar/timestamp operation across every frame.

## Examples

Example 1. `tl()` binds without validating; the check happens when a method is called.

```cpp
auto f0 = pluss::table::DataFrame({
    {"ts", pluss::table::Thread(pluss::make_timeline({"2024-01-01T00:00:00Z"}))},
});
auto atlas = pluss::table::Atlas({f0});

auto out = atlas.tl().year();
std::cout << out.dloc(0);
```

### Output

```text
===========
     | ts  
-----------
0    | 2024
-----------
[1 cs x 1 feat]
```

## Notes

- `tl()` alone does no validation — see the important callout above. Narrow to timeline
  feature(s) first with [`features()`](features.md) if the atlas has other feature kinds
  present.
- The full accessor surface is documented separately — see
  [`AtlasTimelineAccessor` API Reference](../../atlas_timeline_accessor_api_reference).

## See Also

- [`DataFrame::tl`](../dataframe/tl.md): the single-frame delegate, whose eager validation
  this method's lazy, per-frame validation ultimately relies on.
- [`Atlas::features`](features.md): narrow to timeline feature(s) before calling `tl()`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tail.html">
    <span class="label">Previous</span>
    <span class="title">tail</span>
  </a>
  <a class="next-link" href="to_string.html">
    <span class="label">Next</span>
    <span class="title">to_string</span>
  </a>
</div>
