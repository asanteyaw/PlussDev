<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c01aae104312 -->

# StreamTimelineAccessor::yearday

[Back to StreamTimelineAccessor API Reference](../../stream_timeline_accessor_api_reference)

## Signature

```cpp
Stream StreamTimelineAccessor::yearday() const
```

## What It Does

Extracts the day-of-year (`1`-`366`) of the bound stream's timestamp,
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
    A new `Int32` `Series` stream of day-of-year numbers.

## Examples

Example 1. Extract the day-of-year from a late-January and a late-February
timestamp.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}));
auto out = stream.tl().yearday();
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
0            | 31                                                                                                       
1            | 60                                                                                                       
========================================================================================================================
```

`"2024-01-31"` is the 31st day of the year; `"2024-02-29"` is the 60th — the
extra leap day pushes every date from March onward one day later than it
would fall in a non-leap year.

## Notes

- `1` is January 1st; the maximum is `366` in a leap year, `365` otherwise —
  see [`leap()`](leap.md).
- Reads local calendar fields relative to whatever timezone the stream
  currently carries (`"UTC"` unless changed via [`tz()`](tz.md)/
  [`localize()`](localize.md)).
- See the important callout above.

## See Also

- [`StreamTimelineAccessor::day`](day.md): the day-of-*month* number instead
  of day-of-year.
- [`StreamTimelineAccessor::week`](week.md): the ISO week-of-year, a
  coarser-grained position within the year.
- [`StreamTimelineAccessor::leap`](leap.md): whether the row's year has the
  extra day that shifts this number.
- [`Stream::tl`](../stream/tl.md): the entry point this accessor is reached from.
- [`CatalogTimelineAccessor::yearday`](../catalog-timeline/yearday.md): the
  whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="year.html">
    <span class="label">Previous</span>
    <span class="title">year</span>
  </a>
</div>
