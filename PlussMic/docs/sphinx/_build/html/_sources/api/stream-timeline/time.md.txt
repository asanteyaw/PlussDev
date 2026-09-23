<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7159de73730c -->

# StreamTimelineAccessor::time

[Back to StreamTimelineAccessor API Reference](../../stream_timeline_accessor_api_reference)

## Signature

```cpp
Stream StreamTimelineAccessor::time() const
```

## What It Does

Keeps only the time-of-day portion of the bound stream's timestamp — the
date is reset to the Unix epoch day (`1970-01-01`) — replacing its values in
place.

```{important} StreamTimelineAccessor requires the bound stream to be Timeline- or TimeIndex-kind
Same precondition as every method on this accessor — see
[`StreamTimelineAccessor::isnat`](isnat.md)'s own important callout for the
full explanation and the exact throw message.
```

## Parameters

`None`

## Returns

`Stream`
    A new stream with its date portion reset to `1970-01-01`, keeping only
    the time-of-day.

## Examples

Example 1. Keep only the time-of-day from two timestamps.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}));
auto out = stream.tl().time();
auto fmted = out.tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << fmted.as_note().at(0) << " | " << fmted.as_note().at(1);
```

### Output

```text
1970-01-01 09:15:30 | 1970-01-01 23:45:00
```

Every row's date is now `1970-01-01`; each original time-of-day is preserved
exactly.

## Notes

- The result is still `Timeline`/`TimeIndex`-kind with a fixed date
  component — it's meant for comparing or grouping by time-of-day, not as a
  display format. Use [`fmt("%H:%M:%S")`](fmt.md) for a bare text time.
- See the important callout above.

## See Also

- [`StreamTimelineAccessor::date`](date.md): the mirror-image operation,
  keeping the date and zeroing the time-of-day.
- [`Stream::tl`](../stream/tl.md): the entry point this accessor is reached from.
- [`CatalogTimelineAccessor::time`](../catalog-timeline/time.md): the
  whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="shift.html">
    <span class="label">Previous</span>
    <span class="title">shift</span>
  </a>
  <a class="next-link" href="tz.html">
    <span class="label">Next</span>
    <span class="title">tz</span>
  </a>
</div>
