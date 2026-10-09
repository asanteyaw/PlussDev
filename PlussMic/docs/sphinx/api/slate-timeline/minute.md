<!-- pluss-hand-authored -->

# SlateTimelineAccessor::minute

[Back to SlateTimelineAccessor API Reference](../../slate_timeline_accessor_api_reference)

## Signature

```cpp
SlateTimelineAccessor::minute() const -> Slate
```

## What It Does

Replaces the bound slate with its minute-of-hour (`0`-`59`), as a numeric
`Series`.

## Parameters

`None`

## Returns

`Slate`
    A new `Series`-kind (`Int64`) slate, same length as the bound slate,
    holding each row's minute-of-hour.

## Examples

Example 1. Extract the minute from every row.

```cpp
using namespace pluss::table;

auto events = Slate(pluss::make_timeline({
    "2024-01-31T09:15:30Z",
    "2024-02-29T23:45:00Z",
    "2024-07-06T14:00:00Z",
}));
auto out = events.tl().minute();
std::cout << out;
```

### Output

```text
0  15
1  45
2  0

ptype: Series
```

## Notes

- `.tl()` requires the bound slate to already be `Timeline`-kind —
  the accessor's constructor throws otherwise.
- A `NaT` row (or one already null on the underlying slate) displays as
  `"NULL"`.
- Reads the local minute relative to the slate's current timezone.

## See Also

- [`SlateTimelineAccessor::hour`](hour.md),
  [`SlateTimelineAccessor::second`](second.md): the neighboring clock-time
  components.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="localize.html">
    <span class="label">Previous</span>
    <span class="title">localize</span>
  </a>
  <a class="next-link" href="month.html">
    <span class="label">Next</span>
    <span class="title">month</span>
  </a>
</div>
