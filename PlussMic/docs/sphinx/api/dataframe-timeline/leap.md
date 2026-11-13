<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d09cad67ae01 -->

# DataFrameTimelineAccessor::leap

[Back to DataFrameTimelineAccessor API Reference](../../dataframe_timeline_accessor_api_reference)

## Signature

```cpp
DataFrame DataFrameTimelineAccessor::leap() const
```

## What It Does

Replaces every selected `Timeline` feature with a boolean `Series`
flagging whether the row's calendar year is a leap year — same feature name,
same row order.

## Parameters

`None`

## Returns

`DataFrame`
    A new frame with each selected time feature replaced by a `Bool` Series:
    `true` when the row's year is a leap year, `false` otherwise.

## Examples

Example 1. Flag leap-year rows in a `Timeline` feature.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Slate(pluss::make_timeline({
          "2024-01-31T09:15:30Z",
          "2024-02-29T23:45:00Z",
          "2024-07-06T14:00:00Z",
          "2023-03-01T00:00:00Z",
      }))}});
std::cout << events;

auto out = events.tl().leap();
std::cout << out;
```

### Output (before)

```text
==========================
     | ts                 
--------------------------
0    | 2024-01-31 09:15:30
1    | 2024-02-29 23:45:00
2    | 2024-07-06 14:00:00
3    | 2023-03-01 00:00:00
--------------------------
[4 cs x 1 feat]
```

### Output (after)

```text
============
     | ts   
------------
0    | true 
1    | true 
2    | true 
3    | false
------------
[4 cs x 1 feat]
```

Every 2024 row flags `true`; the 2023 row flags `false`.

## Notes

- Reflects the *row's year*, not whether the row's exact date is February
  29th — a `"2024-06-15"` row also flags `true`, since 2024 as a whole is a
  leap year.
- `DataFrame::tl()` requires **every** feature on the calling frame to already
  be `Timeline`-kind — narrow with
  [`features()`](../dataframe/features.md) first if needed.
- A `NaT` row (or one already null on the underlying `Slate`) displays as
  `"NULL"`, not `false`.

## See Also

- [`DataFrameTimelineAccessor::month_end`](month_end.md): where leap-year
  status actually changes February's last valid day.
- [`DataFrameTimelineAccessor::yearday`](yearday.md): where leap years shift
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
