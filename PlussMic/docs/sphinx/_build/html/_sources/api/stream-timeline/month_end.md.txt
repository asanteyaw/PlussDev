<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3777782553c4 -->

# StreamTimelineAccessor::month_end

[Back to StreamTimelineAccessor API Reference](../../stream_timeline_accessor_api_reference)

## Signature

```cpp
Stream StreamTimelineAccessor::month_end() const
```

## What It Does

Flags whether the bound stream's timestamp falls on the last day of its
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
    A new `Bool` `Series` stream: `true` where the row's day-of-month is that
    month's last valid day, `false` otherwise.

## Examples

Example 1. Both sample timestamps are the last day of their respective month.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}));
auto out = stream.tl().month_end();
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
1            | true                                                                                                     
========================================================================================================================
```

`"2024-01-31"` is January's last day; `"2024-02-29"` is February's last day
in this leap year — see [`leap()`](leap.md), since the correct "last day" for
February depends on whether the year is a leap year.

## Notes

- Correctly accounts for each month's actual length, including leap-year
  February — see the important callout on [`leap()`](leap.md).
- Only checks the date component; the time-of-day portion doesn't affect the
  result.
- Reads local calendar fields relative to whatever timezone the stream
  currently carries (`"UTC"` unless changed via [`tz()`](tz.md)/
  [`localize()`](localize.md)).
- See the important callout above.

## See Also

- [`StreamTimelineAccessor::month_start`](month_start.md): the mirror check
  at the beginning of the month.
- [`StreamTimelineAccessor::leap`](leap.md): where leap-year status actually
  changes February's last valid day.
- [`Stream::tl`](../stream/tl.md): the entry point this accessor is reached from.
- [`CatalogTimelineAccessor::month_end`](../catalog-timeline/month_end.md):
  the whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="month.html">
    <span class="label">Previous</span>
    <span class="title">month</span>
  </a>
  <a class="next-link" href="month_name.html">
    <span class="label">Next</span>
    <span class="title">month_name</span>
  </a>
</div>
