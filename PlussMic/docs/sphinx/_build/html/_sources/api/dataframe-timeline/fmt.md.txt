<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a604ad15463e -->

# DataFrameTimelineAccessor::fmt

[Back to DataFrameTimelineAccessor API Reference](../../dataframe_timeline_accessor_api_reference)

## Signature

```cpp
DataFrameTimelineAccessor::fmt(std::string_view pattern) const -> DataFrame
```

## What It Does

Replaces every selected `Timeline`/`TimeIndex` feature with a custom text
representation built from `pattern`, as a `Note` `Series` — same feature
name, same row order.

## Parameters

**pattern** (`std::string_view`)

A `strftime`-style format string. Recognized tokens: `%Y` (4-digit year),
`%m`/`%d` (2-digit month/day), `%H`/`%M`/`%S` (2-digit hour/minute/second),
`%f` (9-digit zero-padded nanoseconds), `%z` (UTC offset as `+HH:MM`/`-HH:MM`
— never a bare `Z`), `%a`/`%A` (abbreviated/full weekday name), `%b`/`%B`
(abbreviated/full month name), and `%%` (a literal `%`). Any other `%`
sequence, or any character outside a `%` token, is copied through literally.

## Returns

`DataFrame`
    A new frame with each selected time feature replaced by a `Note` Series
    holding each row's text rendered per `pattern`.

## Examples

Example 1. Format every row of a `Timeline` feature with a custom pattern.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Thread(pluss::make_timeline({
          "2024-01-31T09:15:30Z",
          "2024-02-29T23:45:00Z",
          "2024-07-06T14:00:00Z",
          "2023-03-01T00:00:00Z",
      }))}});
std::cout << events;

auto out = events.tl().fmt("%d %b %Y");
std::cout << out;
```

### Output (before)

```text
==========================
     | ts                 
--------------------------
0    | 2024-01-31 09:15:30
1    | 2024-02-29 23:45:00
2    | 2024-07-06 14:00:00
3    | 2023-03-01 00:00:00
--------------------------
[4 cs x 1 feat]
```

### Output (after)

```text
==================
     | ts         
------------------
0    | 31 Jan 2024
1    | 29 Feb 2024
2    | 06 Jul 2024
3    | 01 Mar 2023
------------------
[4 cs x 1 feat]
```

The `"ts"` column keeps its name but is now `Note`-kind, holding each row
rendered as `"DD Mon YYYY"`.

Example 2. Combine `%f` and `%z` to reproduce [`iso()`](iso.md)'s pattern by
hand, confirming both tokens' documented behavior.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Thread(pluss::make_timeline({"2024-01-31T09:15:30.123456789Z"}))}});
std::cout << events;

auto out = events.tl().fmt("%Y-%m-%dT%H:%M:%S.%f%z");
std::cout << out;
```

### Output (before)

```text
=============================
     | ts                    
-----------------------------
0    | 2024-01-31 09:15:30.1~
-----------------------------
[1 cs x 1 feat]
```

### Output (after)

```text
=============================
     | ts                    
-----------------------------
0    | 2024-01-31T09:15:30.1~
-----------------------------
[1 cs x 1 feat]
```

The full (unclipped) rendered text is
`"2024-01-31T09:15:30.123456789+00:00"` — `%f` emits all 9 nanosecond digits
(`123456789`, not a truncated `123`), and `%z` renders the UTC offset as the
signed `"+00:00"`, never a bare `"Z"`.

Example 3. Pass an empty pattern to see the call throw.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Thread(pluss::make_timeline({"2024-01-31T09:15:30Z"}))}});

try {
  auto out = events.tl().fmt("");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Thread::tl().fmt: pattern cannot be empty
```

An empty `pattern` has no tokens to render anything with, so it's rejected
outright rather than silently producing an empty string for every row.

## Notes

- `pattern` cannot be empty — an empty pattern throws.
- `%f` always emits all 9 nanosecond digits, zero-padded — there's no way to
  ask for fewer digits (e.g. milliseconds only) directly; slice the resulting
  text with [`note()`](../dataframe/note.md) accessor methods if you need
  fewer digits.
- `%z` always renders as a signed `HH:MM` offset (e.g. `"+00:00"`), never the
  bare `"Z"` suffix some ISO-8601 tooling uses for UTC.
- `DataFrame::tl()` requires **every** feature on the calling frame to already
  be `Timeline`/`TimeIndex`-kind — narrow with
  [`features()`](../dataframe/features.md) first if needed.
- A `NaT` row (or one already null on the underlying `Thread`) displays as
  `"NULL"`, regardless of `pattern`.

## See Also

- [`DataFrameTimelineAccessor::iso`](iso.md): a fixed ISO-8601-with-offset
  pattern, equivalent to `fmt("%Y-%m-%dT%H:%M:%S.%f%z")`.
- [`DataFrameTimelineAccessor::weekday_name`](weekday_name.md),
  [`DataFrameTimelineAccessor::month_name`](month_name.md): pre-built
  single-field text conveniences.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="floor.html">
    <span class="label">Previous</span>
    <span class="title">floor</span>
  </a>
  <a class="next-link" href="hour.html">
    <span class="label">Next</span>
    <span class="title">hour</span>
  </a>
</div>
