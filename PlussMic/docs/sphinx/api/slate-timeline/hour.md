<!-- pluss-hand-authored -->

# SlateTimelineAccessor::hour

[Back to SlateTimelineAccessor API Reference](../../slate_timeline_accessor_api_reference)

## Signature

```cpp
Slate SlateTimelineAccessor::hour() const
```

## What It Does

Replaces the bound slate with its hour-of-day (`0`-`23`), as a numeric
`Series`.

## Parameters

`None`

## Returns

`Slate`
    A new `Series`-kind (`Int64`) slate, same length as the bound slate,
    holding each row's hour-of-day.

## Examples

Example 1. Extract the hour from every row.

```cpp
using namespace pluss::table;

auto events = Slate(pluss::make_timeline({
    "2024-01-31T09:15:30Z",
    "2024-02-29T23:45:00Z",
    "2024-07-06T14:00:00Z",
}));
auto out = events.tl().hour();
std::cout << out;
```

### Output

```text
0  9
1  23
2  14

ptype: Series
```

## Notes

- `.tl()` requires the bound slate to already be `Timeline`-kind —
  the accessor's constructor throws otherwise.
- A `NaT` row (or one already null on the underlying slate) displays as
  `"NULL"`.
- Reads the local hour relative to the slate's current timezone; changing
  timezone with [`tz()`](tz.md) changes what `hour()` reports for the same
  underlying instant.

## See Also

- [`SlateTimelineAccessor::minute`](minute.md),
  [`SlateTimelineAccessor::second`](second.md): the finer clock-time
  components.
- [`SlateTimelineAccessor::time`](time.md): the full clock-time portion as a
  `Timeline`, instead of one numeric field at a time.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fmt.html">
    <span class="label">Previous</span>
    <span class="title">fmt</span>
  </a>
  <a class="next-link" href="isnat.html">
    <span class="label">Next</span>
    <span class="title">isnat</span>
  </a>
</div>
