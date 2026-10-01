<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5bf093929305 -->

# StreamTimelineAccessor::day

[Back to StreamTimelineAccessor API Reference](../../stream_timeline_accessor_api_reference)

## Signature

```cpp
Stream StreamTimelineAccessor::day() const
```

## What It Does

Extracts the day-of-month (`1`-`31`) of the bound stream's timestamp,
replacing it with an `Int32` `Series` stream.

```{important} StreamTimelineAccessor requires the bound stream to be Timeline-kind
Same precondition as every method on this accessor — see
[`StreamTimelineAccessor::isnat`](isnat.md)'s own important callout for the
full explanation and the exact throw message.
```

## Parameters

`None`

## Returns

`Stream`
    A new `Int32` `Series` stream of day-of-month numbers.

## Examples

Example 1. Extract the day-of-month from two end-of-month timestamps.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}));
auto out = stream.tl().day();
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[int32]  records=2
label=no  mask=yes  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 31                                                                                                       
1            | 29                                                                                                       
========================================================================================================================
```

`2024-02-29` is valid — 2024 is a leap year, so February has 29 days that
year; see [`leap()`](leap.md).

## Notes

- Reads local calendar fields relative to whatever timezone the stream
  currently carries (`"UTC"` unless changed via [`tz()`](tz.md)/
  [`localize()`](localize.md)).
- See the important callout above.

## See Also

- [`StreamTimelineAccessor::year`](year.md) / [`month()`](month.md): the
  other calendar-date components.
- [`StreamTimelineAccessor::yearday`](yearday.md): the day-of-*year* number
  instead of day-of-month.
- [`StreamTimelineAccessor::month_end`](month_end.md): whether this day is
  the last day of its month.
- [`Stream::tl`](../stream/tl.md): the entry point this accessor is reached from.
- [`CatalogTimelineAccessor::day`](../catalog-timeline/day.md): the
  whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="date.html">
    <span class="label">Previous</span>
    <span class="title">date</span>
  </a>
  <a class="next-link" href="epoch.html">
    <span class="label">Next</span>
    <span class="title">epoch</span>
  </a>
</div>
