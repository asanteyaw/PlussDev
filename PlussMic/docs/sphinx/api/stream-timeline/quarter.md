<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6d9c87c7cfaf -->

# StreamTimelineAccessor::quarter

[Back to StreamTimelineAccessor API Reference](../../stream_timeline_accessor_api_reference)

## Signature

```cpp
Stream StreamTimelineAccessor::quarter() const
```

## What It Does

Extracts the calendar quarter (`1`-`4`) of the bound stream's timestamp,
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
    A new `Int32` `Series` stream of quarter numbers.

## Examples

Example 1. Extract the quarter from a July and a March timestamp.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeline({"2024-07-06T14:00:00Z", "2023-03-01T00:00:00Z"}));
auto out = stream.tl().quarter();
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
0            | 3                                                                                                        
1            | 1                                                                                                        
========================================================================================================================
```

July falls in Q3 (months 7-9); March falls in Q1 (months 1-3).

## Notes

- `1` covers January-March, `2` April-June, `3` July-September, `4`
  October-December.
- Reads local calendar fields relative to whatever timezone the stream
  currently carries (`"UTC"` unless changed via [`tz()`](tz.md)/
  [`localize()`](localize.md)).
- See the important callout above.

## See Also

- [`StreamTimelineAccessor::month`](month.md): the finer-grained component
  `quarter()` is derived from.
- [`StreamTimelineAccessor::period`](period.md): group timestamps into
  arbitrary frequency-based periods, not just quarters.
- [`Stream::tl`](../stream/tl.md): the entry point this accessor is reached from.
- [`CatalogTimelineAccessor::quarter`](../catalog-timeline/quarter.md): the
  whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="period.html">
    <span class="label">Previous</span>
    <span class="title">period</span>
  </a>
  <a class="next-link" href="round.html">
    <span class="label">Next</span>
    <span class="title">round</span>
  </a>
</div>
