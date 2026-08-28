# Timeline::weekday_name

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline::weekday_name() -> Note
```

Returns the full weekday label for each timestamp.

Use this when the semantics are "give me the weekday names" rather than "apply
an arbitrary format string". It is the direct temporal-label surface for day
names.

## Parameters

This method takes no parameters.

Your main practical choice is between:

- `weekday_name()` when you want the semantic weekday label
- [`strftime(...)`](strftime.md) when you want abbreviated weekday names or a
  mixed date-time label that includes weekday text

## Returns

`Note`
    Full weekday names, preserving the shape of the source timeline.

## Notes

- This is the semantic convenience equivalent of formatting with `%A`.
- It is often the clearest surface for plots, summaries, schedule tables, and
  demo code where the weekday label itself is the payload.
- If you need Monday/Tuesday as boolean predicates rather than strings, use the
  weekday predicate family documented on [`is_monday`](is_monday.md).

## Examples

Example 1. Convert a small intraday timeline into weekday labels.

```cpp
auto labels = base.weekday_name();
std::cout << pluss::note::print(labels) << "\n";
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["Friday", "Friday", "Tuesday", "Wednesday"]
```

Example 2. Apply the same conversion to a calendar-oriented test set that spans
multiple weekdays.

```cpp
auto labels = calendar.weekday_name();
std::cout << pluss::note::print(labels) << "\n";
```

### Output

```text
Note(shape=[8], dim=1, numel=8, encoding=utf8)
["Thursday", "Friday", "Sunday", "Monday", "Tuesday", "Wednesday", "Saturday", "Monday"]
```

Example 3. Compare semantic weekday labeling with a custom formatted label.

```cpp
auto names = base.weekday_name();
auto labels = base.strftime("%A, %b %d");
std::cout << pluss::note::print(names) << "\n";
std::cout << pluss::note::print(labels) << "\n";
```

### Output

Weekday names alone:

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["Friday", "Friday", "Tuesday", "Wednesday"]
```

Weekday embedded inside a larger custom label:

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["Friday, Nov 01", "Friday, Nov 01", "Tuesday, Dec 31", "Wednesday, Jan 01"]
```

## See Also

- [`Timeline::month_name`](month_name.md)
- [`Timeline::strftime`](strftime.md)
- [`Timeline::is_monday`](is_monday.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_note_family.html">
    <span class="label">Previous</span>
    <span class="title">to_note_family</span>
  </a>
  <a class="next-link" href="month_name.html">
    <span class="label">Next</span>
    <span class="title">month_name</span>
  </a>
</div>
