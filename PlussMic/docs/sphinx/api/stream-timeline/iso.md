<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 344e4207b92c -->

# StreamTimelineAccessor::iso

[Back to StreamTimelineAccessor API Reference](../../stream_timeline_accessor_api_reference)

## Signature

```cpp
Stream StreamTimelineAccessor::iso() const
```

## What It Does

Renders the bound stream's timestamp as a fixed ISO-8601-with-offset text
representation, replacing it with a `Note` (text) stream. Equivalent to
[`fmt("%Y-%m-%dT%H:%M:%S.%f%z")`](fmt.md).

```{important} StreamTimelineAccessor requires the bound stream to be Timeline- or TimeIndex-kind
Same precondition as every method on this accessor — see
[`StreamTimelineAccessor::isnat`](isnat.md)'s own important callout for the
full explanation and the exact throw message.
```

## Parameters

`None`

## Returns

`Stream`
    A new `Note` stream holding each row as
    `"YYYY-MM-DDTHH:MM:SS.fffffffff+HH:MM"`.

## Examples

Example 1. Render a timestamp as ISO-8601 text.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z"}));
auto out = stream.tl().iso();
std::cout << out.as_note().at(0);
```

### Output

```text
2024-01-31T09:15:30.000000000+00:00
```

## Notes

- Always includes the full 9-digit nanosecond fraction and a signed `%z`
  offset — use [`fmt()`](fmt.md) with your own pattern for a more compact
  string.
- Changes the stream's kind from `Timeline`/`TimeIndex` to `Note`.
- See the important callout above.

## See Also

- [`StreamTimelineAccessor::fmt`](fmt.md): the general form this method is a
  fixed pattern of.
- [`Stream::tl`](../stream/tl.md): the entry point this accessor is reached from.
- [`CatalogTimelineAccessor::iso`](../catalog-timeline/iso.md): the
  whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="isnat.html">
    <span class="label">Previous</span>
    <span class="title">isnat</span>
  </a>
  <a class="next-link" href="leap.html">
    <span class="label">Next</span>
    <span class="title">leap</span>
  </a>
</div>
