<!-- pluss-hand-authored -->

# ThreadTimelineAccessor::weekday_name

[Back to ThreadTimelineAccessor API Reference](../../thread_timeline_accessor_api_reference)

## Signature

```cpp
ThreadTimelineAccessor::weekday_name() const -> Thread
```

## What It Does

Replaces the bound thread with its full English weekday name, as a `Note`
(text) `Series`.

## Parameters

`None`

## Returns

`Thread`
    A new `Note`-kind thread, same length as the bound thread, holding each
    row's full weekday name (`"Monday"`-`"Sunday"`).

## Examples

Example 1. Extract the weekday name from every row.

```cpp
using namespace pluss::table;

auto events = Thread(pluss::make_timeline({
    "2024-01-31T09:15:30Z",
    "2024-02-29T23:45:00Z",
    "2024-07-06T14:00:00Z",
}));
auto out = events.tl().weekday_name();
std::cout << out;
```

### Output

```text
0  Wednesday
1  Thursday
2  Saturday

ptype: Note
```

## Notes

- Always the full English name (`"Monday"`, not `"Mon"`); use
  [`fmt("%a")`](fmt.md) for the abbreviated three-letter form.
- `.tl()` requires the bound thread to already be `Timeline`-kind —
  the accessor's constructor throws otherwise.
- A `NaT` row (or one already null on the underlying thread) displays as
  `"NULL"`, not a text placeholder like `"NaT"`.

## See Also

- [`ThreadTimelineAccessor::weekday`](weekday.md): the same information as a
  `0`-`6` number instead of a name.
- [`ThreadTimelineAccessor::month_name`](month_name.md): the equivalent for
  months.
- [`ThreadTimelineAccessor::fmt`](fmt.md): build any other text representation
  via a custom pattern.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="weekday.html">
    <span class="label">Previous</span>
    <span class="title">weekday</span>
  </a>
  <a class="next-link" href="weekend.html">
    <span class="label">Next</span>
    <span class="title">weekend</span>
  </a>
</div>
