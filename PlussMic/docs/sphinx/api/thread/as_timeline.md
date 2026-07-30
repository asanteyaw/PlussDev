<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0f875c7d19f5 -->

# Thread::as_timeline

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::as_timeline() const -> const pluss::Timeline&
```

## What It Does

Exposes the underlying `pluss::Timeline` payload directly, by reference — the temporal
escape hatch for calendar-component extraction, rounding, localization, and other
`Timeline`-specific operations that live below the `Thread` surface (many of which are
already re-exposed more conveniently via [`tl()`](tl.md), the `ThreadTimelineAccessor`).
Requires the thread to be `Timeline`-kind; every other payload kind throws — including
`TimeIndex`, even though the two are closely related.

Same reference/no-rewrap contract as [`as_series()`](as_series.md) — see that page's
important callout for the full explanation.

## Parameters

`None`

## Returns

`const pluss::Timeline&`
    A reference to the underlying timeline payload, valid as long as the `Thread` itself
    stays alive.

## Examples

Example 1. Drop to `Timeline` to extract the calendar year, then wrap the result back into
a `Thread`.

```cpp
using namespace pluss::table;

auto stamps = Thread(pluss::make_timeline({
    "2024-01-01T09:10:00Z",
    "2024-02-01T11:00:00Z",
    "2024-03-05T15:30:00Z",
}));

auto out = Thread(stamps.as_timeline().year());
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
Thread-surface accessor — this example calls the underlying payload's method
directly through the reference `as_timeline()` returns.

Example 2. Calling `as_timeline()` on a non-`Timeline` thread throws — including on a
`TimeIndex`-kind thread.

```cpp
auto indexed = Thread(pluss::make_timeindex({"2024-01-01T00:00:00Z"}, true));
try {
  const auto& bad = indexed.as_timeline();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread: payload is not Timeline
```

## Notes

- Returns a reference into the thread's own storage — no copy, no allocation.
- Requires `Timeline`-kind specifically; a `TimeIndex`-kind thread does **not** satisfy
  this, even though the two payload kinds are closely related — see
  [`as_timeindex()`](as_timeindex.md) for the indexed counterpart.
- The result does not automatically become a `Thread` again — wrap it explicitly with
  `Thread(...)` when a `Thread`-only API needs it back.

## See Also

- [`Thread::as_timeindex`](as_timeindex.md): the escape hatch for the indexed temporal
  payload kind, which additionally supports resampling.
- [`Thread::as_series`](as_series.md): the numeric escape hatch, including the full
  reference/no-rewrap explanation.
- [`Thread::tl`](tl.md): the `Thread`-surface temporal accessor — prefer this in most
  application code; reach for `as_timeline()` only when you need a `Timeline`-only method
  `tl()` doesn't re-expose.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="as_timeindex.html">
    <span class="label">Previous</span>
    <span class="title">as_timeindex</span>
  </a>
  <a class="next-link" href="astype.html">
    <span class="label">Next</span>
    <span class="title">astype</span>
  </a>
</div>
