<!-- pluss-hand-authored -->

# SlateTimelineAccessor::leap

[Back to SlateTimelineAccessor API Reference](../../slate_timeline_accessor_api_reference)

## Signature

```cpp
SlateTimelineAccessor::leap() const -> Slate
```

## What It Does

Replaces the bound slate with a boolean `Series` flagging whether each row's
calendar year is a leap year.

## Parameters

`None`

## Returns

`Slate`
    A new `Bool`-typed `Series` slate, same length as the bound slate:
    `true` when the row's year is a leap year, `false` otherwise.

## Examples

Example 1. Flag leap-year rows.

```cpp
using namespace pluss::table;

auto events = Slate(pluss::make_timeline({
    "2024-01-31T09:15:30Z",
    "2024-07-06T14:00:00Z",
    "2023-03-01T00:00:00Z",
}));
auto out = events.tl().leap();
std::cout << out;
```

### Output

```text
0  true
1  true
2  false

ptype: Series
```

Every 2024 row flags `true`; the 2023 row flags `false`.

## Notes

- Reflects the *row's year*, not whether the row's exact date is February
  29th — a `"2024-06-15"` row also flags `true`, since 2024 as a whole is a
  leap year.
- `.tl()` requires the bound slate to already be `Timeline`-kind —
  the accessor's constructor throws otherwise.
- A `NaT` row (or one already null on the underlying slate) displays as
  `"NULL"`, not `false`.

## See Also

- [`SlateTimelineAccessor::month_end`](month_end.md): where leap-year status
  actually changes February's last valid day.
- [`SlateTimelineAccessor::yearday`](yearday.md): where leap years shift
  every day-of-year number from March onward.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="iso.html">
    <span class="label">Previous</span>
    <span class="title">iso</span>
  </a>
  <a class="next-link" href="localize.html">
    <span class="label">Next</span>
    <span class="title">localize</span>
  </a>
</div>
