# Timeline::strftime

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Note Timeline::strftime(std::string_view format) const
```

## What It Does

Formats every timestamp in the timeline into text using a strftime-style format
string.

This is the main bridge from temporal data into labels, report text, export
strings, and human-facing calendar representations.

## Parameters

**fmt**

The format string to apply to each timestamp.

Common directives include:

- `%Y`
  Four-digit year.
- `%m`
  Zero-padded month number.
- `%d`
  Zero-padded day of month.
- `%H`
  Zero-padded hour.
- `%M`
  Zero-padded minute.
- `%S`
  Zero-padded second.
- `%F`
  ISO date, equivalent to `%Y-%m-%d`.
- `%T`
  Whole-second clock time, equivalent to `%H:%M:%S`.
- `%f`
  Fractional seconds.
- `%z`
  Numeric timezone offset.
- `%a`
  Abbreviated weekday name.
- `%A`
  Full weekday name.
- `%b`
  Abbreviated month name.
- `%B`
  Full month name.

## Returns

`Note`
    A text object with the same shape as the input timeline.

## Examples

Example 1. Build compact report labels with year, month, day, hour, and minute.

```cpp
auto out = base.strftime("%Y-%m-%d %H:%M");
std::cout << pluss::note::print(out) << "\n";
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["2024-11-01 09:00", "2024-11-01 10:15", "2024-12-31 23:59", "2025-01-01 00:00"]
```

Example 2. Switch the format string to produce weekday and abbreviated month
labels instead.

```cpp
auto out = base.strftime("%A, %b %d");
std::cout << pluss::note::print(out) << "\n";
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["Friday, Nov 01", "Friday, Nov 01", "Tuesday, Dec 31", "Wednesday, Jan 01"]
```

## Notes

- `strftime()` is the flexible formatting surface.
- If you always want canonical ISO output, [`to_note_iso`](to_note_iso.md) is
  shorter and more explicit.
- The output lives in `Note`, so it naturally composes with the text-processing
  surfaces in PlussQit and PlussAI.
- `Note::vector()` contains non-owning `string_view` values. Keep the formatted
  `Note` alive while using that vector:

  ```cpp
  const auto formatted = timeline.strftime("%F");
  const auto dates = formatted.vector(); // views remain valid with formatted alive
  ```

  Do not store the result of `timeline.strftime("%F").vector()`; the temporary
  owning `Note` is destroyed at the end of that statement.

## See Also

- [`Timeline::to_note_iso`](to_note_iso.md)
- [`Timeline::weekday_name`](weekday_name.md)
- [`Timeline::month_name`](month_name.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="span.html">
    <span class="label">Previous</span>
    <span class="title">span</span>
  </a>
  <a class="next-link" href="to_note_iso.html">
    <span class="label">Next</span>
    <span class="title">to_note_iso</span>
  </a>
</div>
