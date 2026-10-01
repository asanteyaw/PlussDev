<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6580ddee3204 -->

# StreamTimelineAccessor::weekday_name

[Back to StreamTimelineAccessor API Reference](../../stream_timeline_accessor_api_reference)

## Signature

```cpp
Stream StreamTimelineAccessor::weekday_name() const
```

## What It Does

Renders the bound stream's timestamp as its full English weekday name,
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
    A new `Note` stream holding each row's full weekday name
    (`"Monday"`-`"Sunday"`).

## Examples

Example 1. Render the weekday name for a Wednesday and a Thursday.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}));
auto out = stream.tl().weekday_name();
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
0            | Wednesday                                                                                                
1            | Thursday                                                                                                 
========================================================================================================================
```

## Notes

- Always the full English name (`"Monday"`, not `"Mon"`); use
  [`fmt("%a")`](fmt.md) for the abbreviated three-letter form.
- Changes the stream's kind from `Timeline` to `Note`.
- See the important callout above.

## See Also

- [`StreamTimelineAccessor::weekday`](weekday.md): the same information as a
  `0`-`6` number instead of a name.
- [`StreamTimelineAccessor::month_name`](month_name.md): the equivalent for
  months.
- [`Stream::tl`](../stream/tl.md): the entry point this accessor is reached from.
- [`CatalogTimelineAccessor::weekday_name`](../catalog-timeline/weekday_name.md):
  the whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="weekday.html">
    <span class="label">Previous</span>
    <span class="title">weekday</span>
  </a>
  <a class="next-link" href="weekend.html">
    <span class="label">Next</span>
    <span class="title">weekend</span>
  </a>
</div>
