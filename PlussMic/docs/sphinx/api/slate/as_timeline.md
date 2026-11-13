<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0f875c7d19f5 -->

# Slate::as_timeline

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
const pluss::Timeline& Slate::as_timeline() const
```

## What It Does

Exposes the underlying `pluss::Timeline` payload directly, by reference — the temporal
escape hatch for calendar-component extraction, rounding, localization, and other
`Timeline`-specific operations that live below the `Slate` surface (many of which are
already re-exposed more conveniently via [`tl()`](tl.md), the `SlateTimelineAccessor`).
Requires the slate to be `Timeline`-kind; every other payload kind throws — including
`TimeIndex`, even though the two are closely related.

Same reference/no-rewrap contract as [`as_series()`](as_series.md) — see that page's
important callout for the full explanation.

## Parameters

`None`

## Returns

`const pluss::Timeline&`
    A reference to the underlying timeline payload, valid as long as the `Slate` itself
    stays alive.

## Examples

Example 1. Drop to `Timeline` to extract the calendar year, then wrap the result back into
a `Slate`.

```cpp
using namespace pluss::table;

auto stamps = Slate(pluss::make_timeline({
    "2024-01-01T09:10:00Z",
    "2024-02-01T11:00:00Z",
    "2024-03-05T15:30:00Z",
}));

auto out = Slate(stamps.as_timeline().year());
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
Slate-surface accessor — this example calls the underlying payload's method
directly through the reference `as_timeline()` returns.

Example 2. Calling `as_timeline()` on a non-`Timeline` slate throws — including on a
`TimeIndex`-kind slate.

```cpp
auto indexed = Slate(pluss::timeindex::make({"2024-01-01T00:00:00Z"}, true));
try {
  const auto& bad = indexed.as_timeline();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slate: payload is not Timeline
```

## Notes

- Returns a reference into the slate's own storage — no copy, no allocation.
- The result does not automatically become a `Slate` again — wrap it explicitly with
  `Slate(...)` when a `Slate`-only API needs it back.

## See Also

- [`Slate::as_series`](as_series.md): the numeric escape hatch, including the full
  reference/no-rewrap explanation.
- [`Slate::tl`](tl.md): the `Slate`-surface temporal accessor — prefer this in most
  application code; reach for `as_timeline()` only when you need a `Timeline`-only method
  `tl()` doesn't re-expose.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="astype.html">
    <span class="label">Next</span>
    <span class="title">astype</span>
  </a>
</div>
