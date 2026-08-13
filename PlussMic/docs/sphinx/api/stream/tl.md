<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 187acd714ea9 -->

# Stream::tl

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
StreamTimelineAccessor Stream::tl() const
```

## What It Does

The entry point into timestamp-feature extraction on the stream. `tl()` binds
the current stream and hands back a `StreamTimelineAccessor`, which exposes 32
methods: calendar field extraction (`year`, `month`, `day`, `hour`, ...), boolean
predicates (`isnat`, `valid`, `weekend`, `leap`), formatting/conversion (`fmt`,
`iso`, `epoch`, `date`, `time`), rounding (`floor`, `ceil`, `round`), and
timezone/shift operations (`shift`, `period`, `tz`, `localize`, `utc`, `index`) —
see the [StreamTimelineAccessor API Reference](../../stream_timeline_accessor_api_reference)
for every method's own dedicated page.

```{important} Requires the stream itself to be Timeline- or TimeIndex-kind
Same pattern as [`Catalog::tl()`](../catalog/tl.md) — `tl()` accepts both
`Timeline`-kind and `TimeIndex`-kind streams uniformly. Calling any method on a
`Series`/`Note` stream throws `"Thread::tl: expected Timeline or TimeIndex
payload"`. Cast first with [`Stream::cast(ThreadCastTarget::Timeline)`](cast.md)
if the stream isn't already a timestamp.
```

## Parameters

`None`

## Returns

`StreamTimelineAccessor`
    A binder for the current stream — call one of its 32 methods to extract a
    calendar feature, reformat, or shift the stream.

## Examples

Example 1. `tl().year()` extracts the calendar year from a `Timeline`-kind
stream — see [`StreamTimelineAccessor::year`](../stream-timeline/year.md) for
this specific method's own page.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeline({"2024-01-01T09:10:00Z"}));
auto out = stream.tl().year();
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[int32]  records=1
label=no  mask=yes  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 2024                                                                                                     
========================================================================================================================
```

Example 2 (edge case). A non-`Timeline`/`TimeIndex` stream throws — see the
important callout above.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({1.0}));
try {
  auto out = stream.tl().year();
} catch (const pluss::Error& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Thread::tl: expected Timeline or TimeIndex payload
```

## Notes

- Not `noexcept`; the returned accessor itself does no work, but every method it
  exposes eagerly re-checks the bound stream's kind.
- See the important callout above — every one of the 32 methods on the returned
  accessor shares this Timeline-or-TimeIndex-required precondition.
- Calendar-field-extraction methods (`year`, `month`, `day`, ...) return a
  `Series`-kind stream in place of the original `Timeline`-kind stream, per the
  output above.
- Requires a defined stream; throws `"Stream: undefined handle"` otherwise.

## See Also

- [StreamTimelineAccessor API Reference](../../stream_timeline_accessor_api_reference):
  the full list of 32 methods this accessor exposes.
- [`Stream::nt`](nt.md) / [`Stream::ts`](ts.md): the sibling accessors for
  text-processing and time-series operations.
- [`Stream::cast`](cast.md): convert a non-`Timeline`/`TimeIndex` stream first.
- [`Catalog::tl`](../catalog/tl.md): the whole-catalog equivalent, which binds
  every key of a multi-key collection instead of one stream.
- [`CatalogTimelineAccessor::year`](../catalog-timeline/year.md): the exact same
  underlying calendar engine, applied across a catalog's keys.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="take.html">
    <span class="label">Previous</span>
    <span class="title">take</span>
  </a>
  <a class="next-link" href="to_json.html">
    <span class="label">Next</span>
    <span class="title">to_json</span>
  </a>
</div>
