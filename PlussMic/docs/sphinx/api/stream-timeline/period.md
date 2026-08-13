<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f51221f63929 -->

# StreamTimelineAccessor::period

[Back to StreamTimelineAccessor API Reference](../../stream_timeline_accessor_api_reference)

## Signature

```cpp
Stream StreamTimelineAccessor::period(std::string_view freq) const
```

## What It Does

Replaces the bound stream's timestamp with the *start* of the `freq`-sized
period it falls within, replacing its values in place. For the fixed clock
units, this is equivalent to [`floor(freq)`](floor.md); for the calendar
units it accepts, it snaps to the start of that calendar period (e.g. the
first of the month).

```{important} StreamTimelineAccessor requires the bound stream to be Timeline- or TimeIndex-kind
Same precondition as every method on this accessor — see
[`StreamTimelineAccessor::isnat`](isnat.md)'s own important callout for the
full explanation and the exact throw message.
```

## Parameters

**freq** (`std::string_view`)
    One of `"S"`/`"s"`/`"sec"`/`"second"`, `"min"`/`"m"`/`"minute"`,
    `"H"`/`"h"`/`"hour"`, or `"D"`/`"d"`/`"day"`. Cannot be empty. An
    unsupported frequency throws.

## Returns

`Stream`
    A new stream with its timestamp replaced by its period start.

## Examples

Example 1. Snap a mid-day timestamp to its containing day's start.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z"}));
auto out = stream.tl().period("D");
auto fmted = out.tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << fmted.as_note().at(0);
```

### Output

```text
2024-01-31 00:00:00
```

## Notes

- For the fixed units this method accepts, `period(freq)` behaves the same
  as [`floor(freq)`](floor.md) — see that page for the unrecognized-unit
  throw text.
- See the important callout above.

## See Also

- [`StreamTimelineAccessor::floor`](floor.md): the underlying rounding
  behavior this method shares for fixed units.
- [`StreamTimelineAccessor::shift`](shift.md): move the timestamp by a fixed
  offset instead of snapping to a period boundary.
- [`Stream::tl`](../stream/tl.md): the entry point this accessor is reached from.
- [`CatalogTimelineAccessor::period`](../catalog-timeline/period.md): the
  whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="month_start.html">
    <span class="label">Previous</span>
    <span class="title">month_start</span>
  </a>
  <a class="next-link" href="quarter.html">
    <span class="label">Next</span>
    <span class="title">quarter</span>
  </a>
</div>
