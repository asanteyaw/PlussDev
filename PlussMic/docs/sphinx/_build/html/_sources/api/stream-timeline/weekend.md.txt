<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 08482e3bbd90 -->

# StreamTimelineAccessor::weekend

[Back to StreamTimelineAccessor API Reference](../../stream_timeline_accessor_api_reference)

## Signature

```cpp
Stream StreamTimelineAccessor::weekend() const
```

## What It Does

Flags whether the bound stream's timestamp falls on a Saturday or Sunday,
replacing it with a `Bool` `Series` stream.

```{important} StreamTimelineAccessor requires the bound stream to be Timeline-kind
Same precondition as every method on this accessor — see
[`StreamTimelineAccessor::isnat`](isnat.md)'s own important callout for the
full explanation and the exact throw message.
```

## Parameters

`None`

## Returns

`Stream`
    A new `Bool` `Series` stream: `true` where the row falls on a weekend,
    `false` otherwise.

## Examples

Example 1. Flag a Saturday and a Wednesday.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeline({"2024-07-06T14:00:00Z", "2023-03-01T00:00:00Z"}));
auto out = stream.tl().weekend();
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[bool]  records=2
label=no  mask=yes  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | true                                                                                                     
1            | false                                                                                                    
========================================================================================================================
```

`"2024-07-06"` is a Saturday; `"2023-03-01"` is a Wednesday.

## Notes

- Saturday and Sunday only — there's no configurable weekend definition on
  this method.
- Reads local calendar fields relative to whatever timezone the stream
  currently carries (`"UTC"` unless changed via [`tz()`](tz.md)/
  [`localize()`](localize.md)).
- See the important callout above.

## See Also

- [`StreamTimelineAccessor::weekday`](weekday.md): the underlying weekday
  number this method is derived from.
- [`StreamTimelineAccessor::weekday_name`](weekday_name.md): the weekday
  spelled out as a full English name.
- [`Stream::tl`](../stream/tl.md): the entry point this accessor is reached from.
- [`CatalogTimelineAccessor::weekend`](../catalog-timeline/weekend.md): the
  whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="weekday_name.html">
    <span class="label">Previous</span>
    <span class="title">weekday_name</span>
  </a>
  <a class="next-link" href="year.html">
    <span class="label">Next</span>
    <span class="title">year</span>
  </a>
</div>
