<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 618abb10c279 -->

# Slate::tl

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
SlateTimelineAccessor Slate::tl() const
```

## What It Does

Enters the dedicated timestamp accessor for this slate — calendar-component
extraction (`year`, `month`, `weekday`, ...), formatting, timezone conversion,
flooring/ceiling/rounding to a unit, spans between two timestamps, and more, one
method per operation. Unlike [`nt()`](nt.md), `tl()` validates **eagerly**: the
slate must already be defined and must already be `Timeline`- or `TimeIndex`-kind,
or the `.tl()` call itself throws immediately, before any accessor method runs.

Unlike [`DataFrame::tl()`](../dataframe/tl.md), this accessor is bound to a single
slate — there's no equivalent of `.features(...)` narrowing to do first.

This page previews the accessor's most common entry points. Every one of its methods
has its own dedicated, example-rich reference page — start at the
[SlateTimelineAccessor API Reference](../../slate_timeline_accessor_api_reference)
and use its left sidebar to jump straight to any method (`year`, `month`, `fmt`,
`floor`, `tz`, `span`, and more).

## Parameters

`None`

## Returns

`SlateTimelineAccessor`
    A lightweight accessor bound to this (already-validated) `Timeline`/`TimeIndex`
    slate. Nearly every method on it returns a new `Slate`; the source slate is
    never mutated.

## Examples

Example 1. Pull calendar components off a `Timeline` slate.

```cpp
using namespace pluss::table;

auto dates = Slate(pluss::make_timeline({
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

`.year()` returns an `Int64` `Series`-kind slate; `.weekday_name()` returns a
`Note`-kind slate of full weekday names.

Example 2. `tl()` throws immediately on a non-`Timeline`/`TimeIndex` slate — unlike
[`nt()`](nt.md), the check happens at the `.tl()` call itself, not on first method use.

```cpp
auto numbers = Slate(pluss::make_series({10.0, 20.0, 30.0}));
try {
  auto bad = numbers.tl();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slate::tl: expected Timeline or TimeIndex payload
```

## Notes

- `tl()` validates eagerly: the slate must be defined and must already be
  `Timeline`- or `TimeIndex`-kind, or `.tl()` itself throws
  (`"Slate::tl: Slate is undefined"` / `"Slate::tl: expected Timeline or
  TimeIndex payload"`) before any accessor method runs.
- Compare [`nt()`](nt.md), whose `Note`-kind check is lazy (deferred to the first
  method call on the accessor) rather than eager.
- No `options`/narrowing step is needed before calling `.tl()`, unlike
  [`DataFrame::tl()`](../dataframe/tl.md) — a `Slate` is already a single payload.

## See Also

- [`Slate::nt`](nt.md): the equivalent dedicated accessor for `Note` text payloads,
  which validates its payload kind lazily instead of eagerly.
- [`Slate::label`](label.md): the dedicated accessor for the case-label axis, which
  works regardless of the slate's own payload kind.
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
