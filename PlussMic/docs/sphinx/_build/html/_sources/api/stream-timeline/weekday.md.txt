<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 56e852e91978 -->

# StreamTimelineAccessor::weekday

[Back to StreamTimelineAccessor API Reference](../../stream_timeline_accessor_api_reference)

## Signature

```cpp
Stream StreamTimelineAccessor::weekday() const
```

## What It Does

Extracts the weekday number (`0` = Monday through `6` = Sunday) of the bound
stream's timestamp, replacing it with an `Int32` `Series` stream.

```{important} StreamTimelineAccessor requires the bound stream to be Timeline- or TimeIndex-kind
Same precondition as every method on this accessor — see
[`StreamTimelineAccessor::isnat`](isnat.md)'s own important callout for the
full explanation and the exact throw message.
```

## Parameters

`None`

## Returns

`Stream`
    A new `Int32` `Series` stream of weekday numbers.

## Examples

Example 1. Extract the weekday number from a Wednesday and a Thursday.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}));
auto out = stream.tl().weekday();
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
0            | 2                                                                                                        
1            | 3                                                                                                        
========================================================================================================================
```

`"2024-01-31"` is a Wednesday (`2`), `"2024-02-29"` is a Thursday (`3`) — see
[`weekday_name()`](weekday_name.md) for the same information spelled out.

## Notes

- `0` is Monday, `6` is Sunday (ISO weekday numbering).
- Reads local calendar fields relative to whatever timezone the stream
  currently carries (`"UTC"` unless changed via [`tz()`](tz.md)/
  [`localize()`](localize.md)).
- See the important callout above.

## See Also

- [`StreamTimelineAccessor::weekday_name`](weekday_name.md): the same
  information as a full English name instead of a number.
- [`StreamTimelineAccessor::weekend`](weekend.md): whether the weekday falls
  on a weekend.
- [`StreamTimelineAccessor::week`](week.md): the ISO week-of-year this
  weekday belongs to.
- [`Stream::tl`](../stream/tl.md): the entry point this accessor is reached from.
- [`CatalogTimelineAccessor::weekday`](../catalog-timeline/weekday.md): the
  whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="week.html">
    <span class="label">Previous</span>
    <span class="title">week</span>
  </a>
  <a class="next-link" href="weekday_name.html">
    <span class="label">Next</span>
    <span class="title">weekday_name</span>
  </a>
</div>
