<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b4cef985e588 -->

# StreamTimelineAccessor::month_start

[Back to StreamTimelineAccessor API Reference](../../stream_timeline_accessor_api_reference)

## Signature

```cpp
Stream StreamTimelineAccessor::month_start() const
```

## What It Does

Flags whether the bound stream's timestamp falls on the first day of its
calendar month, replacing it with a `Bool` `Series` stream.

```{important} StreamTimelineAccessor requires the bound stream to be Timeline- or TimeIndex-kind
Same precondition as every method on this accessor — see
[`StreamTimelineAccessor::isnat`](isnat.md)'s own important callout for the
full explanation and the exact throw message.
```

## Parameters

`None`

## Returns

`Stream`
    A new `Bool` `Series` stream: `true` where the row's day-of-month is `1`,
    `false` otherwise.

## Examples

Example 1. Neither sample timestamp falls on the 1st of its month.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}));
auto out = stream.tl().month_start();
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
0            | false                                                                                                    
1            | false                                                                                                    
========================================================================================================================
```

Both sample dates are the *last* day of their month, not the first — see
[`month_end()`](month_end.md), which flags `true` for exactly these rows.

## Notes

- Only checks the day-of-month component (`day() == 1`); the time-of-day
  portion doesn't affect the result.
- Reads local calendar fields relative to whatever timezone the stream
  currently carries (`"UTC"` unless changed via [`tz()`](tz.md)/
  [`localize()`](localize.md)).
- See the important callout above.

## See Also

- [`StreamTimelineAccessor::month_end`](month_end.md): the mirror check at
  the end of the month.
- [`StreamTimelineAccessor::day`](day.md): the underlying day-of-month number
  this method is derived from.
- [`Stream::tl`](../stream/tl.md): the entry point this accessor is reached from.
- [`CatalogTimelineAccessor::month_start`](../catalog-timeline/month_start.md):
  the whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="month_name.html">
    <span class="label">Previous</span>
    <span class="title">month_name</span>
  </a>
  <a class="next-link" href="period.html">
    <span class="label">Next</span>
    <span class="title">period</span>
  </a>
</div>
