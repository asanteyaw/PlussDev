<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d8b8b290bc67 -->

# StreamTimelineAccessor::ceil

[Back to StreamTimelineAccessor API Reference](../../stream_timeline_accessor_api_reference)

## Signature

```cpp
Stream StreamTimelineAccessor::ceil(std::string_view unit) const
```

## What It Does

Rounds the bound stream's timestamp *up* to the nearest `unit` boundary,
replacing its values in place (the stream stays `Timeline`-kind).

```{important} StreamTimelineAccessor requires the bound stream to be Timeline-kind
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
    A new stream with its timestamp ceiled to `unit`.

## Examples

Example 1. Ceil two timestamps to the nearest hour.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}));
auto out = stream.tl().ceil("H");
auto fmted = out.tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << fmted.as_note().at(0) << " | " << fmted.as_note().at(1);
```

### Output

```text
2024-01-31 10:00:00 | 2024-03-01 00:00:00
```

Both rows advance to the start of their *next* hour — `09:15:30` to
`10:00:00`, and `23:45:00` rolling over the day boundary into
`2024-03-01 00:00:00`.

## Notes

- A row already exactly on a `unit` boundary is left unchanged, not advanced
  to the next boundary.
- `unit` cannot be empty, and only accepts the fixed clock units listed
  above — an unrecognized unit throws `"Timeline: unsupported unit (expected
  S/min/H/D)"` (see [`floor()`](floor.md) for the throw example).
- See the important callout above.

## See Also

- [`StreamTimelineAccessor::floor`](floor.md) / [`round()`](round.md): round
  down, or to the nearest boundary, instead of always up.
- [`Stream::tl`](../stream/tl.md): the entry point this accessor is reached from.
- [`CatalogTimelineAccessor::ceil`](../catalog-timeline/ceil.md): the
  whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="date.html">
    <span class="label">Next</span>
    <span class="title">date</span>
  </a>
</div>
