<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 00f8fe5cc6f1 -->

# Stream::as_note

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
const pluss::Note& Stream::as_note() const
```

## What It Does

Returns the stream's underlying `pluss::Note` payload by `const` reference — a raw escape
hatch, only valid when the stream is `Note`-kind. No copy is made.

```{important} Kind-specific — throws if the stream isn't Note-kind
Calling `as_note()` on a `Series`/`Timeline`/`TimeIndex` stream throws `"Stream payload is
not Note"`. Check [`kind()`](kind.md) first, or use [`cast()`](cast.md) to convert.
```

## Parameters

`None`

## Returns

`const pluss::Note&`
    The raw `Note` payload, by reference — no copy is made.

## Examples

Example 1. Access the raw `Note` payload of a text stream.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}));

std::cout << stream.as_note();
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["Accra", "Tema", "Kumasi"]
```

Example 2. Calling it on a non-`Note` stream throws.

```cpp
auto series = pluss::kindle::Stream(pluss::make_series({1.0, 2.0}));

try {
  auto out = series.as_note();
} catch (const std::exception& e) {
  std::cout << std::string_view(e.what()).substr(0, 24);
}
```

### Output

```text
Stream payload is not No
```

## Notes

- `Note`-only — throws on every other kind; see the important callout above.
- Returns by `const&`, not a copy — the reference is only valid as long as the source
  `Stream` is alive.
- The printed representation of a raw `pluss::Note` (`Note(shape=..., dim=..., ...)`) is
  distinct from `Stream`'s own boxed-table rendering.
- The thrown exception's `.what()` appends a native stack trace after the message text
  shown above; examples here show only the leading message for readability.

## See Also

- [`Stream::as_series`](as_series.md) / [`Stream::as_timeline`](as_timeline.md): the equivalent escape hatch for the other
  three payload kinds.
- [`Stream::kind`](kind.md): check the payload kind before calling this method.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="abs.html">
    <span class="label">Previous</span>
    <span class="title">abs</span>
  </a>
  <a class="next-link" href="as_series.html">
    <span class="label">Next</span>
    <span class="title">as_series</span>
  </a>
</div>
