<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9bce8919b4eb -->

# StreamTimelineAccessor::leap

[Back to StreamTimelineAccessor API Reference](../../stream_timeline_accessor_api_reference)

## Signature

```cpp
Stream StreamTimelineAccessor::leap() const
```

## What It Does

Flags whether the bound stream's timestamp's calendar year is a leap year,
replacing it with a `Bool` `Series` stream.

```{important} StreamTimelineAccessor requires the bound stream to be Timeline-kind
Same precondition as every method on this accessor — see
[`StreamTimelineAccessor::isnat`](isnat.md)'s own important callout for the
full explanation and the exact throw message.
```

```{important} Reflects the row's YEAR, not whether the date is February 29th
`leap()` answers "is this row's *year* a leap year," not "is this row
February 29th." Any row dated anywhere in 2024 flags `true`, since 2024 as a
whole is a leap year — see Example 1, where a January date and a
February-29th date both flag `true` for the same reason.
```

## Parameters

`None`

## Returns

`Stream`
    A new `Bool` `Series` stream: `true` when the row's calendar year is a
    leap year, `false` otherwise.

## Examples

Example 1. Two timestamps from the same leap year both flag `true`, even
though only one falls on February 29th.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}));
auto out = stream.tl().leap();
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

## Notes

- See the important callouts above — reflects the row's year, not its exact
  date, and requires the bound stream to be `Timeline`-kind.
- Reads local calendar fields relative to whatever timezone the stream
  currently carries (`"UTC"` unless changed via [`tz()`](tz.md)/
  [`localize()`](localize.md)).

## See Also

- [`StreamTimelineAccessor::month_end`](month_end.md): where leap-year status
  actually changes February's last valid day.
- [`StreamTimelineAccessor::yearday`](yearday.md): where leap years shift
  every day-of-year number from March onward.
- [`Stream::tl`](../stream/tl.md): the entry point this accessor is reached from.
- [`CatalogTimelineAccessor::leap`](../catalog-timeline/leap.md): the
  whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="iso.html">
    <span class="label">Previous</span>
    <span class="title">iso</span>
  </a>
  <a class="next-link" href="localize.html">
    <span class="label">Next</span>
    <span class="title">localize</span>
  </a>
</div>
