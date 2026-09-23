<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 912efca41ceb -->

# Stream::as_series

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream::as_series() const -> const pluss::Series&
```

## What It Does

Returns the stream's underlying `pluss::Series` payload by `const` reference — a raw escape
hatch, only valid when the stream is `Series`-kind. Requires no copy; unlike
[`to_thread()`](to_thread.md), this doesn't round-trip through `Thread` — it just exposes
the raw typed payload the `Stream` already wraps.

```{important} Kind-specific — throws if the stream isn't Series-kind
Calling `as_series()` on a `Note`/`Timeline`/`TimeIndex` stream throws `"Stream payload is
not Series"`. Check [`kind()`](kind.md) first if you're not certain of the stream's
payload kind, or use [`cast()`](cast.md) to convert it first.
```

## Parameters

`None`

## Returns

`const pluss::Series&`
    The raw `Series` payload, by reference — no copy is made.

## Examples

Example 1. Access the raw `Series` payload of a numeric stream.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

std::cout << stream.as_series();
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[10, 20, 30]
```

Example 2. Calling it on a non-`Series` stream throws.

```cpp
auto note = pluss::kindle::Stream(pluss::make_note({"a", "b"}));

try {
  auto out = note.as_series();
} catch (const std::exception& e) {
  std::cout << std::string_view(e.what()).substr(0, 26);
}
```

### Output

```text
Stream payload is not Seri
```

## Notes

- `Series`-only — throws on every other kind; see the important callout above.
- Returns by `const&`, not a copy — the reference is only valid as long as the source
  `Stream` is alive.
- The printed representation of a raw `pluss::Series` (`pluss::Series(shape=..., dtype=...,
  ...)`) is distinct from `Stream`'s own boxed-table rendering — this is the tensor-level
  type's own `operator<<`, not `Stream`'s.
- The thrown exception's `.what()` appends a native stack trace after the message text
  shown above; examples here show only the leading message for readability.

## See Also

- [`Stream::as_note`](as_note.md) / [`Stream::as_timeline`](as_timeline.md) /
  [`Stream::as_timeindex`](as_timeindex.md): the equivalent escape hatch for the other
  three payload kinds.
- [`Stream::to_thread`](to_thread.md): converts to a `Thread` instead of exposing the raw
  tensor-level payload.
- [`Stream::kind`](kind.md): check the payload kind before calling this method.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="as_note.html">
    <span class="label">Previous</span>
    <span class="title">as_note</span>
  </a>
  <a class="next-link" href="as_timeindex.html">
    <span class="label">Next</span>
    <span class="title">as_timeindex</span>
  </a>
</div>
