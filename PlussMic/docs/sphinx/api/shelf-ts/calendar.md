<!-- pluss-hand-authored -->

# TimeSeriesAccessor::calendar

[Back to TimeSeriesAccessor API Reference](../../shelf_ts_accessor_api_reference)

## Signature

```cpp
DataFrame TimeSeriesAccessor::calendar(const Shelf& axis) const
```

## What It Does

Extracts calendar components from a time axis into their own columns: year, month, day, hour, and a
formatted weekday string. The values of the bound shelf are not used. Only `axis`, which must be a
`Timeline` shelf, is read, and the result has one row per instant on that axis.

## Parameters

**axis** (`const Shelf&`)

The `Timeline` shelf whose instants are broken into components.

## Returns

`DataFrame`
    A new frame with the five columns `axis_year`, `axis_month`, `axis_day`, `axis_hour` and
    `axis_weekday`, labelled by the axis instants.

## Examples

Example 1. Break two instants into calendar components.

```cpp
using namespace pluss::table;

auto days = Shelf(pluss::make_timeline({"2024-03-14T09:30:00Z", "2024-12-25T00:00:00Z"}));
auto values = Shelf(pluss::make_series({1.0, 2.0}));

std::cout << values.ts().calendar(days);
```

### Output

```text
==============================================================================
                    | axis_year  axis_month  axis_day  axis_hour  axis_weekday
------------------------------------------------------------------------------
2024-03-14 09:30:00 | 2024       3           14        9          Thursday    
2024-12-25 00:00:00 | 2024       12          25        0          Wednesday   
------------------------------------------------------------------------------
[2 cs x 5 feat]
```

Example 2. A `Timeline` shelf can supply its own axis.

```cpp
auto days = Shelf(pluss::make_timeline({"2024-03-14T09:30:00Z", "2024-12-25T00:00:00Z"}));

std::cout << days.ts().calendar(days);
```

### Output

```text
==============================================================================
                    | axis_year  axis_month  axis_day  axis_hour  axis_weekday
------------------------------------------------------------------------------
2024-03-14 09:30:00 | 2024       3           14        9          Thursday    
2024-12-25 00:00:00 | 2024       12          25        0          Wednesday   
------------------------------------------------------------------------------
[2 cs x 5 feat]
```

## Notes

- The same components are available for a frame's own axis through
  [`DataFrameTsAccessor::calendar`](../dataframe-ts/calendar.md).

## See Also

- [`Shelf::ts`](../shelf/ts.md): enters the accessor.
- [`TimeSeriesAccessor::sort`](sort.md): order values by a time axis.
