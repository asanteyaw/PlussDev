<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f4647676e9a3 -->

# StreamTimelineAccessor::hour

[Back to StreamTimelineAccessor API Reference](../../stream_timeline_accessor_api_reference)

## Signature

```cpp
Stream StreamTimelineAccessor::hour() const
```

## What It Does

Extracts the hour-of-day (`0`-`23`) of the bound stream's timestamp,
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
    A new `Int32` `Series` stream of hour numbers.

## Examples

Example 1. Extract the hour from a morning and a late-night timestamp.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}));
auto out = stream.tl().hour();
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
0            | 9                                                                                                        
1            | 23                                                                                                       
========================================================================================================================
```

## Notes

- 24-hour clock: `0` is midnight, `23` is 11 PM.
- Reads local calendar fields relative to whatever timezone the stream
  currently carries (`"UTC"` unless changed via [`tz()`](tz.md)/
  [`localize()`](localize.md)).
- See the important callout above.

## See Also

- [`StreamTimelineAccessor::minute`](minute.md) / [`second()`](second.md): the
  finer-grained time-of-day components.
- [`StreamTimelineAccessor::time`](time.md): the whole time-of-day portion at
  once.
- [`Stream::tl`](../stream/tl.md): the entry point this accessor is reached from.
- [`CatalogTimelineAccessor::hour`](../catalog-timeline/hour.md): the
  whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fmt.html">
    <span class="label">Previous</span>
    <span class="title">fmt</span>
  </a>
  <a class="next-link" href="index.html">
    <span class="label">Next</span>
    <span class="title">index</span>
  </a>
</div>
