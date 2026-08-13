<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4c0e8cdf5997 -->

# StreamTimelineAccessor::year

[Back to StreamTimelineAccessor API Reference](../../stream_timeline_accessor_api_reference)

## Signature

```cpp
Stream StreamTimelineAccessor::year() const
```

## What It Does

Extracts the calendar year of the bound stream's timestamp, replacing it with
an `Int32` `Series` stream.

```{important} StreamTimelineAccessor requires the bound stream to be Timeline- or TimeIndex-kind
Same precondition as every method on this accessor — see
[`StreamTimelineAccessor::isnat`](isnat.md)'s own important callout for the
full explanation and the exact throw message.
```

## Parameters

`None`

## Returns

`Stream`
    A new `Int32` `Series` stream of calendar years.

## Examples

Example 1. Extract the year from two timestamps in the same year.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}));
auto out = stream.tl().year();
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
0            | 2024                                                                                                     
1            | 2024                                                                                                     
========================================================================================================================
```

## Notes

- Reads local calendar fields relative to whatever timezone the stream
  currently carries (`"UTC"` unless changed via [`tz()`](tz.md)/
  [`localize()`](localize.md)).
- See the important callout above.

## See Also

- [`StreamTimelineAccessor::month`](month.md) / [`day()`](day.md): the other
  calendar-date components, extracted the same way.
- [`StreamTimelineAccessor::leap`](leap.md): whether the extracted year is a
  leap year.
- [`Stream::tl`](../stream/tl.md): the entry point this accessor is reached from.
- [`CatalogTimelineAccessor::year`](../catalog-timeline/year.md): the
  whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="weekend.html">
    <span class="label">Previous</span>
    <span class="title">weekend</span>
  </a>
  <a class="next-link" href="yearday.html">
    <span class="label">Next</span>
    <span class="title">yearday</span>
  </a>
</div>
