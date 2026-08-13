<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5f8f0b7fe1df -->

# StreamTimelineAccessor::tz

[Back to StreamTimelineAccessor API Reference](../../stream_timeline_accessor_api_reference)

## Signature

```cpp
Stream StreamTimelineAccessor::tz(std::string_view name) const
```

## What It Does

Relabels the bound stream's timestamp *display* timezone to `name`, without
changing the underlying instant — the same operation as
[`Thread::tl().tz()`](../thread-timeline/tz.md)/
[`DataFrame::tl().tz()`](../dataframe-timeline/tz.md) at the table layer.

The relabeling survives being read back through any further `.tl()` call
(`fmt()`, `hour()`, another `tz()`, anything) — see Example 1.

```{important} StreamTimelineAccessor requires the bound stream to be Timeline- or TimeIndex-kind
Same precondition as every method on this accessor — see
[`StreamTimelineAccessor::isnat`](isnat.md)'s own important callout for the
full explanation and the exact throw message.
```

## Parameters

**name** (`std::string_view`)
    The target timezone. Only `"UTC"` (also the empty string),
    `"America/Edmonton"`, `"America/New_York"`, and `"Europe/London"` are
    recognized (case-insensitive); any other name throws.

## Returns

`Stream`
    A new stream with its timestamp relabeled to `name`. Reading the result
    back through any further `.tl()` call reflects the new display timezone.

## Examples

Example 1. `tz()` relabels the display timezone, and the relabeling is still
in effect on the very next `.tl()` read.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z"}));
auto shifted = stream.tl().tz("America/New_York");
auto fmted = shifted.tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << fmted.as_note().at(0);
```

### Output

```text
2024-01-31 04:15:30
```

`04:15:30` is the `"America/New_York"`-relative reading of the same instant
(a `-5:00` offset in January) — matching what
[`Thread::tl().tz()`](../thread-timeline/tz.md) produces on the same input.

## Notes

- Only four timezone names are recognized: `"UTC"` (or empty string),
  `"America/Edmonton"`, `"America/New_York"`, and `"Europe/London"` — any
  other name throws.
- The relabeling round-trips correctly through the `kindle::Stream` boundary
  every further `.tl()` call goes through.
- See the important callout above.

## See Also

- [`StreamTimelineAccessor::utc`](utc.md): shorthand for `tz("UTC")`.
- [`StreamTimelineAccessor::localize`](localize.md): changes the underlying
  instant (which *does* persist) rather than just the display label.
- [`Stream::tl`](../stream/tl.md): the entry point this accessor is reached from.
- [`CatalogTimelineAccessor::tz`](../catalog-timeline/tz.md): the
  whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="time.html">
    <span class="label">Previous</span>
    <span class="title">time</span>
  </a>
  <a class="next-link" href="utc.html">
    <span class="label">Next</span>
    <span class="title">utc</span>
  </a>
</div>
