<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: aa704954abcd -->

# StreamTimelineAccessor::month

[Back to StreamTimelineAccessor API Reference](../../stream_timeline_accessor_api_reference)

## Signature

```cpp
Stream StreamTimelineAccessor::month() const
```

## What It Does

Extracts the calendar month (`1`-`12`) of the bound stream's timestamp,
replacing it with an `Int32` `Series` stream.

```{important} StreamTimelineAccessor requires the bound stream to be Timeline- or TimeIndex-kind
Same precondition as every method on this accessor — see
[`StreamTimelineAccessor::isnat`](isnat.md)'s own important callout for the
full explanation and the exact throw message.
```

## Parameters

`None`

## Returns

`Stream`
    A new `Int32` `Series` stream of month numbers.

## Examples

Example 1. Extract the month from a January and a February timestamp.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}));
auto out = stream.tl().month();
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
0            | 1                                                                                                        
1            | 2                                                                                                        
========================================================================================================================
```

## Notes

- `1` is January, `12` is December.
- Reads local calendar fields relative to whatever timezone the stream
  currently carries (`"UTC"` unless changed via [`tz()`](tz.md)/
  [`localize()`](localize.md)).
- See the important callout above.

## See Also

- [`StreamTimelineAccessor::month_name`](month_name.md): the same
  information spelled out as a full English name.
- [`StreamTimelineAccessor::year`](year.md) / [`day()`](day.md): the other
  calendar-date components.
- [`StreamTimelineAccessor::month_start`](month_start.md) /
  [`month_end()`](month_end.md): whether the day is the first or last day of
  this month.
- [`Stream::tl`](../stream/tl.md): the entry point this accessor is reached from.
- [`CatalogTimelineAccessor::month`](../catalog-timeline/month.md): the
  whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="minute.html">
    <span class="label">Previous</span>
    <span class="title">minute</span>
  </a>
  <a class="next-link" href="month_end.html">
    <span class="label">Next</span>
    <span class="title">month_end</span>
  </a>
</div>
