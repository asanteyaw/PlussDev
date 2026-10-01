<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b2e0b4de55a2 -->

# StreamTimelineAccessor::month_name

[Back to StreamTimelineAccessor API Reference](../../stream_timeline_accessor_api_reference)

## Signature

```cpp
Stream StreamTimelineAccessor::month_name() const
```

## What It Does

Renders the bound stream's timestamp as its full English month name,
replacing it with a `Note` (text) stream.

```{important} StreamTimelineAccessor requires the bound stream to be Timeline-kind
Same precondition as every method on this accessor — see
[`StreamTimelineAccessor::isnat`](isnat.md)'s own important callout for the
full explanation and the exact throw message.
```

## Parameters

`None`

## Returns

`Stream`
    A new `Note` stream holding each row's full month name
    (`"January"`-`"December"`).

## Examples

Example 1. Render the month name for a January and a February timestamp.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}));
auto out = stream.tl().month_name();
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
0            | January                                                                                                  
1            | February                                                                                                 
========================================================================================================================
```

## Notes

- Always the full English name (`"January"`, not `"Jan"`); use
  [`fmt("%b")`](fmt.md) for the abbreviated three-letter form.
- Changes the stream's kind from `Timeline` to `Note`.
- See the important callout above.

## See Also

- [`StreamTimelineAccessor::month`](month.md): the same information as a
  `1`-`12` number instead of a name.
- [`StreamTimelineAccessor::weekday_name`](weekday_name.md): the equivalent
  for weekdays.
- [`Stream::tl`](../stream/tl.md): the entry point this accessor is reached from.
- [`CatalogTimelineAccessor::month_name`](../catalog-timeline/month_name.md):
  the whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="month_end.html">
    <span class="label">Previous</span>
    <span class="title">month_end</span>
  </a>
  <a class="next-link" href="month_start.html">
    <span class="label">Next</span>
    <span class="title">month_start</span>
  </a>
</div>
