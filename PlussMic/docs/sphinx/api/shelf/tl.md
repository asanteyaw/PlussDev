<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 618abb10c279 -->

# Shelf::tl

[Back to Shelf API Reference](../../shelf_api_reference)

## Signature

```cpp
ShelfTimelineAccessor Shelf::tl() const
```

## What It Does

Enters the dedicated timestamp accessor for this shelf — calendar-component
extraction (`year`, `month`, `weekday`, ...), formatting, timezone conversion,
flooring/ceiling/rounding to a unit, spans between two timestamps, and more, one
method per operation. Unlike [`nt()`](nt.md), `tl()` validates **eagerly**: the
shelf must already be defined and must already be `Timeline`- or `TimeIndex`-kind,
or the `.tl()` call itself throws immediately, before any accessor method runs.

Unlike [`DataFrame::tl()`](../dataframe/tl.md), this accessor is bound to a single
shelf — there's no equivalent of `.features(...)` narrowing to do first.

This page previews the accessor's most common entry points. Every one of its methods
has its own dedicated, example-rich reference page — start at the
[ShelfTimelineAccessor API Reference](../../shelf_timeline_accessor_api_reference)
and use its left sidebar to jump straight to any method (`year`, `month`, `fmt`,
`floor`, `tz`, `span`, and more).

## Parameters

`None`

## Returns

`ShelfTimelineAccessor`
    A lightweight accessor bound to this (already-validated) `Timeline`/`TimeIndex`
    shelf. Nearly every method on it returns a new `Shelf`; the source shelf is
    never mutated.

## Examples

Example 1. Pull calendar components off a `Timeline` shelf.

```cpp
using namespace pluss::table;

auto dates = Shelf(pluss::make_timeline({
    "2024-01-15T00:00:00Z",
    "2024-06-30T00:00:00Z",
    "2024-12-25T00:00:00Z",
}));

auto years = dates.tl().year();
std::cout << years;
```

### Output

```text
0  2024
1  2024
2  2024

ptype: Series
```

```cpp
auto weekdays = dates.tl().weekday_name();
std::cout << weekdays;
```

### Output

```text
0  Monday
1  Sunday
2  Wednesday

ptype: Note
```

`.year()` returns an `Int64` `Series`-kind shelf; `.weekday_name()` returns a
`Note`-kind shelf of full weekday names.

Example 2. `tl()` throws immediately on a non-`Timeline`/`TimeIndex` shelf — unlike
[`nt()`](nt.md), the check happens at the `.tl()` call itself, not on first method use.

```cpp
auto numbers = Shelf(pluss::make_series({10.0, 20.0, 30.0}));
try {
  auto bad = numbers.tl();
} catch (const pluss::Error& e) {
  std::cout << e.message();
}
```

### Output

```text
Shelf::tl: expected Timeline payload
```

## Notes

- `tl()` validates eagerly: the shelf must be defined and must already be
  `Timeline`- or `TimeIndex`-kind, or `.tl()` itself throws
  (`"Shelf::tl: Shelf is undefined"` / `"Shelf::tl: expected Timeline or
  TimeIndex payload"`) before any accessor method runs.
- Compare [`nt()`](nt.md), whose `Note`-kind check is lazy (deferred to the first
  method call on the accessor) rather than eager.
- No `options`/narrowing step is needed before calling `.tl()`, unlike
  [`DataFrame::tl()`](../dataframe/tl.md) — a `Shelf` is already a single payload.

## See Also

- [`Shelf::nt`](nt.md): the equivalent dedicated accessor for `Note` text payloads,
  which validates its payload kind lazily instead of eagerly.
- [`Shelf::label`](label.md): the dedicated accessor for the case-label axis, which
  works regardless of the shelf's own payload kind.
- [`DataFrame::tl`](../dataframe/tl.md): the frame-level equivalent, which requires
  every selected feature to already be `Timeline`/`TimeIndex`-kind.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="titlecase.html">
    <span class="label">Previous</span>
    <span class="title">titlecase</span>
  </a>
  <a class="next-link" href="to_string.html">
    <span class="label">Next</span>
    <span class="title">to_string</span>
  </a>
</div>
