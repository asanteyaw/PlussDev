<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3685d9f2e68b -->

# Stream::defined

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream::defined() const noexcept -> bool
```

## What It Does

Reports whether the stream owns a concrete payload. A default-constructed `Stream()` has
none — `defined()` returns `false`, and calling most other methods on it (e.g.
[`size()`](size.md)) throws `"Stream: undefined handle"`. Every `Stream` built from a
`Series`/`Note`/`Timeline`/`TimeIndex` constructor, or from
[`from_thread()`](from_thread.md), is defined immediately.

## Parameters

`None`

## Returns

`bool`
    `true` if the stream has a concrete payload; `false` for a default-constructed
    `Stream()`.

## Examples

Example 1. A stream built from data is defined.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

std::cout << stream.defined();
```

### Output

```text
1
```

Example 2. A default-constructed stream is not — and calling other methods on it throws.

```cpp
pluss::kindle::Stream undef;
std::cout << "defined: " << undef.defined() << "\n";

try {
  undef.size();
} catch (const std::exception& e) {
  std::cout << std::string_view(e.what()).substr(0, 24);
}
```

### Output

```text
defined: 0
Stream: undefined handle
```

## Notes

- `noexcept` and safe to call on any `Stream`, including a default-constructed one — it's
  the one method that never throws regardless of state.
- Guard with `defined()` before calling size/shape/kind-family methods on a stream that
  might be default-constructed (e.g. a member variable before assignment).
- The thrown exception's `.what()` appends a native stack trace after the message text
  shown above; examples here show only the leading message for readability.

## See Also

- [`Stream::empty`](empty.md): a different check — a *defined* stream with zero rows.
- [`Stream::size`](size.md) / [`Stream::shape`](shape.md): throw the same
  `"Stream: undefined handle"` message this page describes when called on an undefined
  stream.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="clip.html">
    <span class="label">Previous</span>
    <span class="title">clip</span>
  </a>
  <a class="next-link" href="distinct.html">
    <span class="label">Next</span>
    <span class="title">distinct</span>
  </a>
</div>
