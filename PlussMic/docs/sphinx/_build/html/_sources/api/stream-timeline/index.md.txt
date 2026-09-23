<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d2bf02f29566 -->

# StreamTimelineAccessor::index

[Back to StreamTimelineAccessor API Reference](../../stream_timeline_accessor_api_reference)

## Signature

```cpp
Stream StreamTimelineAccessor::index(bool require_sorted = true) const
```

## What It Does

Converts the bound stream from `Timeline`-kind to `TimeIndex`-kind.
Idempotent: calling `index()` on a stream that's already `TimeIndex`-kind
succeeds and leaves its values unchanged.

```{important} StreamTimelineAccessor requires the bound stream to be Timeline- or TimeIndex-kind
Same precondition as every method on this accessor — see
[`StreamTimelineAccessor::isnat`](isnat.md)'s own important callout for the
full explanation and the exact throw message.
```

## Parameters

**require_sorted** (`bool`, defaults to `true`)
    When `true` (the default), every value in the stream must be
    non-decreasing (and non-`NaT`), or the call throws. When `false`, the
    conversion happens without checking order or `NaT`.

## Returns

`Stream`
    A new stream converted to `TimeIndex`-kind.

## Examples

Example 1. Convert a sorted `Timeline` stream to `TimeIndex`.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeline({"2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z"}));
auto out = stream.tl().index();
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=TimeIndex  dtype=TimeIndex  records=2
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 2024-01-01T00:00:00.000000000+00:00                                                                      
1            | 2024-01-02T00:00:00.000000000+00:00                                                                      
========================================================================================================================
```

Example 2 (edge case). An out-of-order stream throws under the default
`require_sorted = true`.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeline({"2024-01-02T00:00:00Z", "2024-01-01T00:00:00Z"}));
try {
  auto out = stream.tl().index();
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
TimeIndex must be sorted (non-decreasing)
```

## Notes

- `require_sorted = false` skips the check entirely — use it only when the
  stream's order doesn't matter for what comes next.
- Idempotent on an already-`TimeIndex`-kind stream — see the important
  callout on [`isnat()`](isnat.md), which explains that `tl()` accepts both
  `Timeline`- and `TimeIndex`-kind streams uniformly.
- See the important callout above.

## See Also

- [`Stream::cast`](../stream/cast.md): the general-purpose kind-conversion
  entry point, of which this method is a specialized shorthand.
- [`Stream::tl`](../stream/tl.md): the entry point this accessor is reached from.
- [`CatalogTimelineAccessor::index`](../catalog-timeline/index.md): the
  whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="hour.html">
    <span class="label">Previous</span>
    <span class="title">hour</span>
  </a>
  <a class="next-link" href="isnat.html">
    <span class="label">Next</span>
    <span class="title">isnat</span>
  </a>
</div>
