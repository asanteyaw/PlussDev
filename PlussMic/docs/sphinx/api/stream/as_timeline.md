<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0f875c7d19f5 -->

# Stream::as_timeline

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream::as_timeline() const -> const pluss::Timeline&
```

## What It Does

Returns the stream's underlying `pluss::Timeline` payload by `const` reference — a raw
escape hatch, only valid when the stream is `Timeline`-kind. No copy is made.

```{important} Kind-specific — throws if the stream isn't Timeline-kind
Calling `as_timeline()` on a `Series`/`Note`/`TimeIndex` stream throws `"Stream payload is
not Timeline"`. Check [`kind()`](kind.md) first, or use [`cast()`](cast.md) to convert.
```

## Parameters

`None`

## Returns

`const pluss::Timeline&`
    The raw `Timeline` payload, by reference — no copy is made.

## Examples

Example 1. Access the raw `Timeline` payload of a timestamp stream.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeline(
    {"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z", "2024-01-01T10:05:00Z"}));

std::cout << stream.as_timeline();
```

### Output

```text
Timeline(shape=[3], tz=UTC)
[ 2024-01-01 09:10:00, 2024-01-01 09:40:00, 2024-01-01 10:05:00 ]
```

Example 2. Calling it on a non-`Timeline` stream throws.

```cpp
auto series = pluss::kindle::Stream(pluss::make_series({1.0, 2.0}));

try {
  auto out = series.as_timeline();
} catch (const std::exception& e) {
  std::cout << std::string_view(e.what()).substr(0, 28);
}
```

### Output

```text
Stream payload is not Timeli
```

## Notes

- `Timeline`-only — throws on every other kind; see the important callout above.
- Returns by `const&`, not a copy — the reference is only valid as long as the source
  `Stream` is alive.
- The printed representation of a raw `pluss::Timeline` (`Timeline(shape=..., tz=...)`) is
  distinct from `Stream`'s own boxed-table rendering.
- The thrown exception's `.what()` appends a native stack trace after the message text
  shown above; examples here show only the leading message for readability.

## See Also

- [`Stream::as_series`](as_series.md) / [`Stream::as_note`](as_note.md): the equivalent escape hatch for the other
  three payload kinds.
- [`Stream::kind`](kind.md): check the payload kind before calling this method.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="astype.html">
    <span class="label">Next</span>
    <span class="title">astype</span>
  </a>
</div>
