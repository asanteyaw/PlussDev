<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 33cd99b3b66e -->

# StreamTimelineAccessor::floor

[Back to StreamTimelineAccessor API Reference](../../stream_timeline_accessor_api_reference)

## Signature

```cpp
Stream StreamTimelineAccessor::floor(std::string_view unit) const
```

## What It Does

Rounds the bound stream's timestamp *down* to the nearest `unit` boundary,
replacing its values in place (the stream stays `Timeline`- or
`TimeIndex`-kind, whichever it already was).

```{important} StreamTimelineAccessor requires the bound stream to be Timeline- or TimeIndex-kind
Same precondition as every method on this accessor — see
[`StreamTimelineAccessor::isnat`](isnat.md)'s own important callout for the
full explanation and the exact throw message.
```

## Parameters

**unit** (`std::string_view`)
    One of `"S"`/`"s"`/`"sec"`/`"second"`, `"min"`/`"m"`/`"minute"`,
    `"H"`/`"h"`/`"hour"`, or `"D"`/`"d"`/`"day"`. Cannot be empty. An
    unsupported unit throws.

## Returns

`Stream`
    A new stream with its timestamp floored to `unit`.

## Examples

Example 1. Floor two timestamps to the nearest hour.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}));
auto out = stream.tl().floor("H");
auto fmted = out.tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << fmted.as_note().at(0) << " | " << fmted.as_note().at(1);
```

### Output

```text
2024-01-31 09:00:00 | 2024-02-29 23:00:00
```

`09:15:30` drops to `09:00:00`; `23:45:00` was already exactly on the hour in
terms of its floor target, so it stays at `23:00:00`.

Example 2 (edge case). An unsupported unit throws.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z"}));
try {
  auto out = stream.tl().floor("bogus");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Timeline: unsupported unit (expected S/min/H/D)
```

## Notes

- `unit` cannot be empty, and only accepts the fixed clock units listed
  above — an unrecognized unit throws, per Example 2.
- See the important callout above.

## See Also

- [`StreamTimelineAccessor::ceil`](ceil.md) / [`round()`](round.md): round up,
  or to the nearest boundary, instead of always down.
- [`StreamTimelineAccessor::date`](date.md): shorthand for `floor("D")`.
- [`Stream::tl`](../stream/tl.md): the entry point this accessor is reached from.
- [`CatalogTimelineAccessor::floor`](../catalog-timeline/floor.md): the
  whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="epoch.html">
    <span class="label">Previous</span>
    <span class="title">epoch</span>
  </a>
  <a class="next-link" href="fmt.html">
    <span class="label">Next</span>
    <span class="title">fmt</span>
  </a>
</div>
