<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 668d28368f6a -->

# StreamTimelineAccessor::isnat

[Back to StreamTimelineAccessor API Reference](../../stream_timeline_accessor_api_reference)

## Signature

```cpp
Stream StreamTimelineAccessor::isnat() const
```

## What It Does

Flags whether the bound stream's timestamp is `NaT` (missing/"Not a Time"),
replacing it with a `Bool` `Series` stream.

```{important} StreamTimelineAccessor requires the bound stream to be Timeline-kind
Every method on this accessor — not just `isnat()` — routes through
`Slate::tl()` for the bound stream. `tl()` requires the stream to be
`Timeline`-kind; if it is not, the call throws
`"Slate::tl: expected Timeline payload"`. Cast first with
[`Stream::cast(SlateCastTarget::Timeline)`](../stream/cast.md) if the stream
isn't already a timestamp.
```

`isnat()`'s own result is never itself `NaT` — it always answers `true` or
`false`, even for the row it's flagging.

## Parameters

`None`

## Returns

`Stream`
    A new `Bool` `Series` stream: `true` where the value is `NaT`, `false`
    otherwise.

## Examples

Example 1. Flag a `NaT` timestamp alongside a valid one. An empty string
passed to [`pluss::make_timeline`](../stream/tl.md) produces `NaT` rather than
throwing.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeline({"", "2024-01-01T00:00:00Z"}));
auto out = stream.tl().isnat();
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
0            | true                                                                                                     
1            | false                                                                                                    
========================================================================================================================
```

Example 2 (edge case). A non-`Timeline` stream throws — see the
important callout above.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({1, 2, 3}));
try {
  auto out = stream.tl().isnat();
} catch (const pluss::Error& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Slate::tl: expected Timeline payload
```

## Notes

- See the important callout above — this precondition applies to every method
  on `StreamTimelineAccessor`, not only `isnat()`.
- Unlike almost every other method on this accessor, `isnat()`'s output is
  never itself masked — it directly answers the missingness question.

## See Also

- [`StreamTimelineAccessor::valid`](valid.md): the logical inverse.
- [`Stream::tl`](../stream/tl.md): the entry point this accessor is reached from.
- [`CatalogTimelineAccessor::isnat`](../catalog-timeline/isnat.md): the
  whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="hour.html">
    <span class="label">Previous</span>
    <span class="title">hour</span>
  </a>
  <a class="next-link" href="iso.html">
    <span class="label">Next</span>
    <span class="title">iso</span>
  </a>
</div>
