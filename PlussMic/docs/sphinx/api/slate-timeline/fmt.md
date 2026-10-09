<!-- pluss-hand-authored -->

# SlateTimelineAccessor::fmt

[Back to SlateTimelineAccessor API Reference](../../slate_timeline_accessor_api_reference)

## Signature

```cpp
SlateTimelineAccessor::fmt(std::string_view pattern) const -> Slate
```

## What It Does

Replaces the bound slate with a custom text representation built from
`pattern`, as a `Note` `Series`.

## Parameters

**pattern** (`std::string_view`)

A `strftime`-style format string. Recognized tokens: `%Y` (4-digit year),
`%m`/`%d` (2-digit month/day), `%H`/`%M`/`%S` (2-digit hour/minute/second),
`%f` (9-digit zero-padded nanoseconds), `%z` (UTC offset as `+HH:MM`/`-HH:MM` —
never a bare `Z`), `%a`/`%A` (abbreviated/full weekday name), `%b`/`%B`
(abbreviated/full month name), and `%%` (a literal `%`). Any other `%`
sequence, or any character outside a `%` token, is copied through literally.
Cannot be empty.

## Returns

`Slate`
    A new `Note`-kind slate, same length as the bound slate, holding each
    row's text rendered per `pattern`.

## Examples

Example 1. Format every row with a custom pattern.

```cpp
using namespace pluss::table;

auto events = Slate(pluss::make_timeline({
    "2024-01-31T09:15:30Z",
    "2024-02-29T23:45:00Z",
    "2024-07-06T14:00:00Z",
}));
auto out = events.tl().fmt("%d %b %Y");
std::cout << out;
```

### Output

```text
0  31 Jan 2024
1  29 Feb 2024
2  06 Jul 2024

ptype: Note
```

Example 2. Combine `%f` and `%z` to reproduce [`iso()`](iso.md)'s pattern by
hand.

```cpp
auto sub = Slate(pluss::make_timeline({"2024-01-31T09:15:30.123456789Z"}));
auto out2 = sub.tl().fmt("%Y-%m-%dT%H:%M:%S.%f%z");
std::cout << out2;
```

### Output

```text
0  2024-01-31T09:15:30.123456789+00:00

ptype: Note
```

`%f` emits all 9 nanosecond digits (`123456789`, not a truncated `123`), and
`%z` renders the UTC offset as the signed `"+00:00"`, never a bare `"Z"`.

Example 3. An empty pattern throws.

```cpp
try {
  auto bad = events.tl().fmt("");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Slate::tl().fmt: pattern cannot be empty
```

## Notes

- `pattern` cannot be empty.
- `%f` always emits all 9 nanosecond digits, zero-padded — there's no way to
  ask for fewer digits (e.g. milliseconds only) directly; slice the resulting
  text with [`SlateNoteAccessor`](../slate-note/chars.md) methods if fewer
  digits are needed.
- `%z` always renders as a signed `HH:MM` offset (e.g. `"+00:00"`), never the
  bare `"Z"` suffix some ISO-8601 tooling uses for UTC.
- `.tl()` requires the bound slate to already be `Timeline`-kind —
  the accessor's constructor throws otherwise.
- A `NaT` row (or one already null on the underlying slate) displays as
  `"NULL"`, regardless of `pattern`.

## See Also

- [`SlateTimelineAccessor::iso`](iso.md): a fixed ISO-8601-with-offset
  pattern, equivalent to `fmt("%Y-%m-%dT%H:%M:%S.%f%z")`.
- [`SlateTimelineAccessor::weekday_name`](weekday_name.md),
  [`SlateTimelineAccessor::month_name`](month_name.md): pre-built
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
