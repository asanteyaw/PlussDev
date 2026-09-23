<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5fc2cd31120d -->

# StreamTimelineAccessor::round

[Back to StreamTimelineAccessor API Reference](../../stream_timeline_accessor_api_reference)

## Signature

```cpp
Stream StreamTimelineAccessor::round(std::string_view unit) const
```

## What It Does

Rounds the bound stream's timestamp to the *nearest* `unit` boundary,
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
    A new stream with its timestamp rounded to the nearest `unit`.

## Examples

Example 1. Round two timestamps to the nearest hour.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}));
auto out = stream.tl().round("H");
auto fmted = out.tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << fmted.as_note().at(0) << " | " << fmted.as_note().at(1);
```

### Output

```text
2024-01-31 09:00:00 | 2024-03-01 00:00:00
```

`09:15:30` is closer to `09:00:00` than `10:00:00`, so it rounds down;
`23:45:00` is closer to the next day's `00:00:00` than back to `23:00:00`, so
it rounds up and rolls into `2024-03-01`.

## Notes

- A value exactly halfway between two boundaries resolves **up**, never
  down.
- `unit` cannot be empty, and only accepts the fixed clock units listed
  above — an unrecognized unit throws `"Timeline: unsupported unit (expected
  S/min/H/D)"` (see [`floor()`](floor.md) for the throw example).
- See the important callout above.

## See Also

- [`StreamTimelineAccessor::floor`](floor.md), [`ceil()`](ceil.md): always
  down, or always up, instead of nearest.
- [`Stream::tl`](../stream/tl.md): the entry point this accessor is reached from.
- [`CatalogTimelineAccessor::round`](../catalog-timeline/round.md): the
  whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="quarter.html">
    <span class="label">Previous</span>
    <span class="title">quarter</span>
  </a>
  <a class="next-link" href="second.html">
    <span class="label">Next</span>
    <span class="title">second</span>
  </a>
</div>
