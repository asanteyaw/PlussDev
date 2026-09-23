<!-- pluss-hand-authored -->

# ThreadTimelineAccessor::month_name

[Back to ThreadTimelineAccessor API Reference](../../thread_timeline_accessor_api_reference)

## Signature

```cpp
ThreadTimelineAccessor::month_name() const -> Thread
```

## What It Does

Replaces the bound thread with its full English month name, as a `Note`
(text) `Series`.

## Parameters

`None`

## Returns

`Thread`
    A new `Note`-kind thread, same length as the bound thread, holding each
    row's full month name (`"January"`-`"December"`).

## Examples

Example 1. Extract the month name from every row.

```cpp
using namespace pluss::table;

auto events = Thread(pluss::make_timeline({
    "2024-01-31T09:15:30Z",
    "2024-02-29T23:45:00Z",
    "2024-07-06T14:00:00Z",
}));
auto out = events.tl().month_name();
std::cout << out;
```

### Output

```text
0  January
1  February
2  July

ptype: Note
```

## Notes

- Always the full English name (`"January"`, not `"Jan"`); use
  [`fmt("%b")`](fmt.md) for the abbreviated three-letter form.
- `.tl()` requires the bound thread to already be `Timeline`/`TimeIndex`-kind —
  the accessor's constructor throws otherwise.
- A `NaT` row (or one already null on the underlying thread) displays as
  `"NULL"`, not a text placeholder.

## See Also

- [`ThreadTimelineAccessor::month`](month.md): the same information as a
  `1`-`12` number instead of a name.
- [`ThreadTimelineAccessor::weekday_name`](weekday_name.md): the equivalent for
  weekdays.
- [`ThreadTimelineAccessor::fmt`](fmt.md): build any other text representation
  via a custom pattern.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="month_end.html">
    <span class="label">Previous</span>
    <span class="title">month_end</span>
  </a>
  <a class="next-link" href="month_start.html">
    <span class="label">Next</span>
    <span class="title">month_start</span>
  </a>
</div>
