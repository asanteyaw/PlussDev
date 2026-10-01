<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3b1f666e1289 -->

# StreamTimelineAccessor::utc

[Back to StreamTimelineAccessor API Reference](../../stream_timeline_accessor_api_reference)

## Signature

```cpp
Stream StreamTimelineAccessor::utc() const
```

## What It Does

Shorthand for [`tz("UTC")`](tz.md) — relabels the bound stream's display
timezone back to `"UTC"` without changing the underlying instant.

```{important} StreamTimelineAccessor requires the bound stream to be Timeline-kind
Same precondition as every method on this accessor — see
[`StreamTimelineAccessor::isnat`](isnat.md)'s own important callout for the
full explanation and the exact throw message.
```

## Parameters

`None`

## Returns

`Stream`
    A new stream relabeled to `"UTC"` display. Reading the result back
    through any further `.tl()` call reflects the `"UTC"` relabeling.

## Examples

Example 1. `tz("America/New_York")` relabels the display timezone (visible
on the first `fmt()` read below); a follow-up `utc()` relabels it back to
`"UTC"`, and that too is visible on the next read — the instant itself never
changes across either call.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z"}));
auto ny = stream.tl().tz("America/New_York");
std::cout << "NY-relabeled: "
          << ny.tl().fmt("%Y-%m-%d %H:%M:%S").as_note().at(0) << "\n";

auto out = ny.tl().utc();
std::cout << "UTC-relabeled: "
          << out.tl().fmt("%Y-%m-%d %H:%M:%S").as_note().at(0);
```

### Output

```text
NY-relabeled: 2024-01-31 04:15:30
UTC-relabeled: 2024-01-31 09:15:30
```

## Notes

- The relabeling round-trips correctly through the `kindle::Stream` boundary
  every further `.tl()` call goes through, the same as [`tz()`](tz.md).
- See the important callout above.

## See Also

- [`StreamTimelineAccessor::tz`](tz.md): the general form this method is a
  fixed `"UTC"` shorthand for.
- [`StreamTimelineAccessor::localize`](localize.md): the operation that
  actually changes the underlying instant.
- [`Stream::tl`](../stream/tl.md): the entry point this accessor is reached from.
- [`CatalogTimelineAccessor::utc`](../catalog-timeline/utc.md): the
  whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tz.html">
    <span class="label">Previous</span>
    <span class="title">tz</span>
  </a>
  <a class="next-link" href="valid.html">
    <span class="label">Next</span>
    <span class="title">valid</span>
  </a>
</div>
