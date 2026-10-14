<!-- pluss-hand-authored -->

# SlateTimelineAccessor::weekend

[Back to SlateTimelineAccessor API Reference](../../slate_timeline_accessor_api_reference)

## Signature

```cpp
SlateTimelineAccessor::weekend() const -> Slate
```

## What It Does

Replaces the bound slate with a boolean `Series` flagging whether each row
falls on Saturday or Sunday.

## Parameters

`None`

## Returns

`Slate`
    A new `Bool`-typed `Series` slate, same length as the bound slate:
    `true` on Saturday/Sunday, `false` otherwise.

## Examples

Example 1. Flag weekend rows.

```cpp
using namespace pluss::table;

auto events = Slate(pluss::make_timeline({
    "2024-01-31T09:15:30Z",
    "2024-07-06T14:00:00Z",
    "2023-03-01T00:00:00Z",
}));
auto out = events.tl().weekend();
std::cout << out;
```

### Output

```text
0  false
1  true
2  false

ptype: Series
```

Only `"2024-07-06"` (a Saturday) is a weekend row.

## Notes

- `true` covers Saturday and Sunday only — there's no way to include Friday or
  exclude one of the two via this method; build a custom mask from
  [`weekday()`](weekday.md) if a different day set is needed.
- `.tl()` requires the bound slate to already be `Timeline`-kind —
  the accessor's constructor throws otherwise.
- A `NaT` row (or one already null on the underlying slate) displays as
  `"NULL"`, not `false`.

## See Also

- [`SlateTimelineAccessor::weekday`](weekday.md): the underlying
  day-of-week number `weekend()` is derived from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="weekday_name.html">
    <span class="label">Previous</span>
    <span class="title">weekday_name</span>
  </a>
  <a class="next-link" href="year.html">
    <span class="label">Next</span>
    <span class="title">year</span>
  </a>
</div>
