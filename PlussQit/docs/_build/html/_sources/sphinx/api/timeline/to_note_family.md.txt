# Timeline::to_note_* Family

[Back to Timeline API Reference](../../timeline_api_reference)

## Representative Signatures

```cpp
Timeline::to_note_date() -> Note
Timeline::to_note_time() -> Note
Timeline::to_note_datetime() -> Note
Timeline::to_note_rfc3339() -> Note
```

## What It Does

The `to_note_*` family provides named convenience formats on top of
`strftime(...)`.

Use this family when you want a common text layout without remembering or
rewriting the underlying format directives each time.

The current surface includes many more members than the four shown above:

- date-only helpers
- time-only helpers
- datetime helpers
- RFC-style helpers
- delimiter variants
- compact variants
- timezone-bearing variants
- weekday/month-name variants

This page documents the usage pattern with representative methods. The naming
contract is direct: the method name tells you the intended output form.

## Family Breakdown

These helpers are parameterless, but they still form several distinct usage
groups:

- date-oriented helpers
  Methods such as `to_note_date()` drop the clock portion and emphasize the
  calendar identity of each timestamp.
- time-oriented helpers
  Methods such as `to_note_time()` keep the clock portion and suppress the
  calendar portion.
- datetime helpers
  Methods such as `to_note_datetime()` keep both date and time but choose a
  presentation-oriented layout instead of canonical ISO precision.
- interchange-oriented helpers
  Methods such as `to_note_rfc3339()` keep explicit offset information in a
  layout that is practical for APIs, logs, and wire-format exports.

The right helper depends on what the text will do next:

- use the date helper for grouping labels and calendar-facing displays
- use the time helper for intraday schedules and clock-only comparisons
- use the datetime helper for reports and dashboards
- use the RFC helper when the text will leave Pluss and needs timezone-explicit
  transport semantics

## Parameters

This family takes no explicit parameters.

The practical toggle is the helper you choose:

- `to_note_date()`
  Calendar-only output.
- `to_note_time()`
  Clock-only output.
- `to_note_datetime()`
  Human-readable combined date-time output.
- `to_note_rfc3339()`
  Combined date-time output with explicit offset semantics.

## Examples

Example 1. Extract date-only text.

```cpp
auto out = base.to_note_date();
std::cout << pluss::note::print(out) << "\n";
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["2024-11-01", "2024-11-01", "2024-12-31", "2025-01-01"]
```

Example 2. Extract time-only text.

```cpp
auto out = base.to_note_time();
std::cout << pluss::note::print(out) << "\n";
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["09:00:00", "10:15:30", "23:59:59", "00:00:00"]
```

Example 3. Extract a plain datetime layout.

```cpp
auto out = base.to_note_datetime();
std::cout << pluss::note::print(out) << "\n";
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["2024-11-01 09:00:00", "2024-11-01 10:15:30", "2024-12-31 23:59:59", "2025-01-01 00:00:00"]
```

Example 4. Extract RFC-style text with explicit offset.

```cpp
auto out = base.to_note_rfc3339();
std::cout << pluss::note::print(out) << "\n";
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["2024-11-01T09:00:00+00:00", "2024-11-01T10:15:30+00:00", "2024-12-31T23:59:59+00:00", "2025-01-01T00:00:00+00:00"]
```

## Notes

- Prefer the named helper when it already expresses your intent clearly.
- Drop back to [`strftime`](strftime.md) when the exact format is custom or
  domain-specific.
- The larger convenience family is intentionally broad so common export layouts
  stay one method call away.
- The helper name is the contract. These methods are for stable common layouts,
  not for user-defined formatting logic.

## See Also

- [`Timeline::strftime`](strftime.md)
- [`Timeline::to_note_iso`](to_note_iso.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_note_iso.html">
    <span class="label">Previous</span>
    <span class="title">to_note_iso</span>
  </a>
  <a class="next-link" href="weekday_name.html">
    <span class="label">Next</span>
    <span class="title">weekday_name</span>
  </a>
</div>
