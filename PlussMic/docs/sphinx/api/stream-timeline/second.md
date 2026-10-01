<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c667d14120b1 -->

# StreamTimelineAccessor::second

[Back to StreamTimelineAccessor API Reference](../../stream_timeline_accessor_api_reference)

## Signature

```cpp
Stream StreamTimelineAccessor::second() const
```

## What It Does

Extracts the second-of-minute (`0`-`59`) of the bound stream's timestamp,
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
    A new `Int32` `Series` stream of second numbers.

## Examples

Example 1. Extract the second component from two timestamps.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}));
auto out = stream.tl().second();
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
0            | 30                                                                                                       
1            | 0                                                                                                        
========================================================================================================================
```

## Notes

- Truncates any sub-second precision the underlying timestamp carries — it
  reports whole seconds only.
- Reads local calendar fields relative to whatever timezone the stream
  currently carries (`"UTC"` unless changed via [`tz()`](tz.md)/
  [`localize()`](localize.md)).
- See the important callout above.

## See Also

- [`StreamTimelineAccessor::hour`](hour.md) / [`minute()`](minute.md): the
  neighboring time-of-day components.
- [`StreamTimelineAccessor::time`](time.md): the whole time-of-day portion at
  once.
- [`Stream::tl`](../stream/tl.md): the entry point this accessor is reached from.
- [`CatalogTimelineAccessor::second`](../catalog-timeline/second.md): the
  whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="round.html">
    <span class="label">Previous</span>
    <span class="title">round</span>
  </a>
  <a class="next-link" href="shift.html">
    <span class="label">Next</span>
    <span class="title">shift</span>
  </a>
</div>
