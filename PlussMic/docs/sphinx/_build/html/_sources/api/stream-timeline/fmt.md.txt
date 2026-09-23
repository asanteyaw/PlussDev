<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d4c1db2ff900 -->

# StreamTimelineAccessor::fmt

[Back to StreamTimelineAccessor API Reference](../../stream_timeline_accessor_api_reference)

## Signature

```cpp
Stream StreamTimelineAccessor::fmt(std::string_view pattern) const
```

## What It Does

Renders the bound stream's timestamp as custom text built from `pattern`,
replacing it with a `Note` (text) stream.

```{important} StreamTimelineAccessor requires the bound stream to be Timeline- or TimeIndex-kind
Same precondition as every method on this accessor — see
[`StreamTimelineAccessor::isnat`](isnat.md)'s own important callout for the
full explanation and the exact throw message.
```

## Parameters

**pattern** (`std::string_view`)
    A `strftime`-style format string. Recognized tokens: `%Y` (4-digit year),
    `%m`/`%d` (2-digit month/day), `%H`/`%M`/`%S` (2-digit hour/minute/second),
    `%f` (9-digit zero-padded nanoseconds), `%z` (UTC offset as
    `+HH:MM`/`-HH:MM`, never a bare `Z`), `%a`/`%A` (abbreviated/full weekday
    name), `%b`/`%B` (abbreviated/full month name), and `%%` (a literal `%`).
    Cannot be empty.

## Returns

`Stream`
    A new `Note` stream holding each row's text rendered per `pattern`.

## Examples

Example 1. Format two timestamps with a custom day/month-name/year pattern.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}));
auto out = stream.tl().fmt("%d %b %Y");
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Note  dtype=Note  records=2
label=no  mask=yes  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 31 Jan 2024                                                                                              
1            | 29 Feb 2024                                                                                              
========================================================================================================================
```

## Notes

- `pattern` cannot be empty — an empty pattern throws.
- `%f` always emits all 9 nanosecond digits, zero-padded; `%z` always renders
  as a signed `HH:MM` offset, never a bare `"Z"`.
- Changes the stream's kind from `Timeline`/`TimeIndex` to `Note`.
- See the important callout above.

## See Also

- [`StreamTimelineAccessor::iso`](iso.md): a fixed ISO-8601-with-offset
  pattern, equivalent to `fmt("%Y-%m-%dT%H:%M:%S.%f%z")`.
- [`StreamTimelineAccessor::weekday_name`](weekday_name.md) /
  [`month_name()`](month_name.md): dedicated shorthands for the full-name
  tokens.
- [`Stream::tl`](../stream/tl.md): the entry point this accessor is reached from.
- [`CatalogTimelineAccessor::fmt`](../catalog-timeline/fmt.md): the
  whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="floor.html">
    <span class="label">Previous</span>
    <span class="title">floor</span>
  </a>
  <a class="next-link" href="hour.html">
    <span class="label">Next</span>
    <span class="title">hour</span>
  </a>
</div>
