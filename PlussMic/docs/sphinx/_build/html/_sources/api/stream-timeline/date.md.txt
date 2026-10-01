<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 88e566b24ef3 -->

# StreamTimelineAccessor::date

[Back to StreamTimelineAccessor API Reference](../../stream_timeline_accessor_api_reference)

## Signature

```cpp
Stream StreamTimelineAccessor::date() const
```

## What It Does

Strips the time-of-day from the bound stream's timestamp (dropped to
midnight), replacing its values in place. Equivalent to
[`floor("D")`](floor.md).

```{important} StreamTimelineAccessor requires the bound stream to be Timeline-kind
Same precondition as every method on this accessor — see
[`StreamTimelineAccessor::isnat`](isnat.md)'s own important callout for the
full explanation and the exact throw message.
```

## Parameters

`None`

## Returns

`Stream`
    A new stream with its time-of-day portion zeroed out.

## Examples

Example 1. Strip the time-of-day from two timestamps.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}));
auto out = stream.tl().date();
auto fmted = out.tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << fmted.as_note().at(0) << " | " << fmted.as_note().at(1);
```

### Output

```text
2024-01-31 00:00:00 | 2024-02-29 00:00:00
```

## Notes

- See the important callout above.

## See Also

- [`StreamTimelineAccessor::time`](time.md): the mirror-image operation,
  keeping the time-of-day and zeroing the date.
- [`StreamTimelineAccessor::floor`](floor.md): the general form `date()` is a
  fixed `"D"` unit of.
- [`Stream::tl`](../stream/tl.md): the entry point this accessor is reached from.
- [`CatalogTimelineAccessor::date`](../catalog-timeline/date.md): the
  whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ceil.html">
    <span class="label">Previous</span>
    <span class="title">ceil</span>
  </a>
  <a class="next-link" href="day.html">
    <span class="label">Next</span>
    <span class="title">day</span>
  </a>
</div>
