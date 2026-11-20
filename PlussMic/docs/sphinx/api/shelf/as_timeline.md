<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0f875c7d19f5 -->

# Shelf::as_timeline

[Back to Shelf API Reference](../../shelf_api_reference)

## Signature

```cpp
const pluss::Timeline& Shelf::as_timeline() const
```

## What It Does

Exposes the underlying `pluss::Timeline` payload directly, by reference — the temporal
escape hatch for calendar-component extraction, rounding, localization, and other
`Timeline`-specific operations that live below the `Shelf` surface (many of which are
already re-exposed more conveniently via [`tl()`](tl.md), the `ShelfTimelineAccessor`).
Requires the shelf to be `Timeline`-kind; every other payload kind throws — including
`TimeIndex`, even though the two are closely related.

Same reference/no-rewrap contract as [`as_series()`](as_series.md) — see that page's
important callout for the full explanation.

## Parameters

`None`

## Returns

`const pluss::Timeline&`
    A reference to the underlying timeline payload, valid as long as the `Shelf` itself
    stays alive.

## Examples

Example 1. Drop to `Timeline` to extract the calendar year, then wrap the result back into
a `Shelf`.

```cpp
using namespace pluss::table;

auto stamps = Shelf(pluss::make_timeline({
    "2024-01-01T09:10:00Z",
    "2024-02-01T11:00:00Z",
    "2024-03-05T15:30:00Z",
}));

auto out = Shelf(stamps.as_timeline().year());
std::cout << out;
```

### Output

```text
0  2024
1  2024
2  2024
ptype: Series
```

`year()` here is `pluss::Timeline`'s own method, not the `tl().year()`
Shelf-surface accessor — this example calls the underlying payload's method
directly through the reference `as_timeline()` returns.

Example 2. Calling `as_timeline()` on a non-`Timeline` shelf throws — including on a
`TimeIndex`-kind shelf.

```cpp
auto words = Shelf(pluss::make_note({"2024-01-01"}));
try {
  const auto& bad = words.as_timeline();
} catch (const pluss::Error& e) {
  std::cout << e.message();
}
```

### Output

```text
Shelf: payload is not Timeline
```

## Notes

- Returns a reference into the shelf's own storage — no copy, no allocation.
- The result does not automatically become a `Shelf` again — wrap it explicitly with
  `Shelf(...)` when a `Shelf`-only API needs it back.

## See Also

- [`Shelf::as_series`](as_series.md): the numeric escape hatch, including the full
  reference/no-rewrap explanation.
- [`Shelf::tl`](tl.md): the `Shelf`-surface temporal accessor — prefer this in most
  application code; reach for `as_timeline()` only when you need a `Timeline`-only method
  `tl()` doesn't re-expose.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="as_series.html">
    <span class="label">Previous</span>
    <span class="title">as_series</span>
  </a>
  <a class="next-link" href="astype.html">
    <span class="label">Next</span>
    <span class="title">astype</span>
  </a>
</div>
