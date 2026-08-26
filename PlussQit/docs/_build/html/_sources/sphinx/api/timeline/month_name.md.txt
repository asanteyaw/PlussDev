# Timeline::month_name

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline::month_name() -> Note
```

Returns the full month label for each timestamp.

This is the semantic month-label surface. Use it when the payload you need is
the month name itself, not a mixed format string or a numeric month extraction.

## Parameters

This method takes no parameters.

The practical choices nearby are:

- `month_name()` for full text labels such as `"November"`
- [`strftime(...)`](strftime.md) for abbreviated or mixed labels
- [`month()`](year.md) when you want numeric month components

## Returns

`Note`
    Full month names, preserving the source shape.

## Notes

- This is the semantic convenience equivalent of formatting with `%B`.
- It is useful for plots, monthly summaries, grouped labels, and reports where
  the final artifact should already contain readable month names.
- If you need the numeric month for grouping or arithmetic, use the date-part
  extraction family documented on [`year`](year.md).

## Examples

Example 1. Convert a small timeline into full month labels.

```cpp
auto labels = base.month_name();
std::cout << pluss::print_note(labels) << "\n";
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["November", "November", "December", "January"]
```

Example 2. Apply the same conversion to a calendar-oriented test set that spans
several months.

```cpp
auto labels = calendar.month_name();
std::cout << pluss::print_note(labels) << "\n";
```

### Output

```text
Note(shape=[8], dim=1, numel=8, encoding=utf8)
["February", "March", "March", "April", "December", "January", "November", "November"]
```

Example 3. Compare full month names with numeric month extraction.

```cpp
auto labels = base.month_name();
auto values = base.month();
std::cout << pluss::print_note(labels) << "\n";
std::cout << pluss::series::print(values) << "\n";
```

### Output

Full month names:

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["November", "November", "December", "January"]
```

Numeric month values:

```text
Series(shape=[4], strides=[1], dtype=int32, platform=cpu, numel=4, contiguous=true)
data=[11, 11, 12, 1]
```

## See Also

- [`Timeline::weekday_name`](weekday_name.md)
- [`Timeline::strftime`](strftime.md)
- [`Timeline::year`](year.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="weekday_name.html">
    <span class="label">Previous</span>
    <span class="title">weekday_name</span>
  </a>
  <a class="next-link" href="floor.html">
    <span class="label">Next</span>
    <span class="title">floor</span>
  </a>
</div>
