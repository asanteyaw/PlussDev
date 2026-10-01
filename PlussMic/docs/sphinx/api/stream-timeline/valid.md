<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: eff36b95bf9a -->

# StreamTimelineAccessor::valid

[Back to StreamTimelineAccessor API Reference](../../stream_timeline_accessor_api_reference)

## Signature

```cpp
Stream StreamTimelineAccessor::valid() const
```

## What It Does

Flags whether the bound stream's timestamp is a real, present timestamp (not
`NaT`), replacing it with a `Bool` `Series` stream. Exactly the logical
inverse of [`isnat()`](isnat.md).

```{important} StreamTimelineAccessor requires the bound stream to be Timeline-kind
Same precondition as every method on this accessor — see
[`StreamTimelineAccessor::isnat`](isnat.md)'s own important callout for the
full explanation and the exact throw message.
```

## Parameters

`None`

## Returns

`Stream`
    A new `Bool` `Series` stream: `true` where the value is a real timestamp,
    `false` where it's `NaT`.

## Examples

Example 1. Flag a valid timestamp alongside a `NaT` one.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeline({"", "2024-01-01T00:00:00Z"}));
auto out = stream.tl().valid();
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[bool]  records=2
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | false                                                                                                    
1            | true                                                                                                     
========================================================================================================================
```

## Notes

- Exactly `!isnat()`, provided for readability at call sites that want to
  filter *for* present timestamps rather than *for* missing ones.
- See the important callout above.

## See Also

- [`StreamTimelineAccessor::isnat`](isnat.md): the logical inverse.
- [`Stream::tl`](../stream/tl.md): the entry point this accessor is reached from.
- [`CatalogTimelineAccessor::valid`](../catalog-timeline/valid.md): the
  whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="utc.html">
    <span class="label">Previous</span>
    <span class="title">utc</span>
  </a>
  <a class="next-link" href="week.html">
    <span class="label">Next</span>
    <span class="title">week</span>
  </a>
</div>
