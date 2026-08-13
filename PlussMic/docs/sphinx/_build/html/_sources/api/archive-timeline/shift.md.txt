<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4f3be2252c0a -->

# ArchiveTimelineAccessor::shift

[Back to ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference)

## Signature

```cpp
Archive ArchiveTimelineAccessor::shift(std::int64_t periods, std::string_view freq) const
```

## What It Does

Shifts each bound key's timestamp forward (or backward, if `periods` is
negative) by `periods * freq`, replacing each bound key's values in place.
Month/year shifts are calendar-aware: a day-of-month that doesn't exist in the
target month is clamped down to that month's last valid day. Applied to **every catalog** in the stack, by delegating to that catalog's own
[`CatalogTimelineAccessor::shift`](../catalog-timeline/shift.md).

```{important} Binding and kind-check rules are inherited from Archive::tl()
This method's key-binding (from `catalog(0)`) and lazy Timeline/TimeIndex-kind check
follow exactly the rules documented on [`Archive::tl`](../archive/tl.md) — see that
page's two important callouts for the full explanation. `ArchiveTimelineAccessor`
requires every bound key to be `Timeline`- or `TimeIndex`-kind, checked the moment any
method (including this one) is called.
```

## Parameters

**periods** (`std::int64_t`)
    How many `freq` units to shift by. Positive moves forward in time, negative
    moves backward.

**freq** (`std::string_view`)
    One of `"y"`/`"yr"`/`"year"`/`"years"`/`"annual"`,
    `"mo"`/`"mth"`/`"month"`/`"months"`, `"d"`/`"day"`/`"days"`,
    `"h"`/`"hr"`/`"hour"`/`"hours"`, `"t"`/`"min"`/`"minute"`/`"minutes"`, or
    `"s"`/`"sec"`/`"second"`/`"seconds"` (case-insensitive). Cannot be empty.

## Returns

`Archive`
    A new archive of the same depth, with every bound key's timestamps shifted by
    `periods * freq`, in every catalog.

## Examples

Example 1. Shift two timestamps forward one calendar month, showing
day-of-month clamping on the January value. Delegation to `CatalogTimelineAccessor::shift` is exact — the same
input catalog produces the identical per-catalog output shown on
[`CatalogTimelineAccessor::shift`](../catalog-timeline/shift.md)'s own page.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"})),
});
auto archive = pluss::kindle::Archive({c0});
auto out = archive.tl().shift(1, "months");
auto fmted = out.tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << fmted.catalog(0).keys("ts").as_note().at(0) << " | " << fmted.catalog(0).keys("ts").as_note().at(1);
```

### Output

```text
2024-02-29 09:15:30 | 2024-03-29 23:45:00
```

## Notes

- See the important callout above — binding and kind-check rules come from
  [`Archive::tl`](../archive/tl.md).
- This method's per-catalog result is identical to calling
  [`CatalogTimelineAccessor::shift`](../catalog-timeline/shift.md) directly on each
  catalog in the stack.

## See Also

- [`CatalogTimelineAccessor::shift`](../catalog-timeline/shift.md): the single-catalog
  delegate this method applies to every catalog.
- [`Archive::tl`](../archive/tl.md): the entry point this accessor is reached from.
- [ArchiveTimelineAccessor API Reference](../../archive_timeline_accessor_api_reference):
  the full list of 32 methods this accessor exposes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="second.html">
    <span class="label">Previous</span>
    <span class="title">second</span>
  </a>
  <a class="next-link" href="time.html">
    <span class="label">Next</span>
    <span class="title">time</span>
  </a>
</div>
