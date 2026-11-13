<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1ce95b95829f -->

# Stream::size

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
std::int64_t Stream::size() const
```

## What It Does

Returns the number of rows in the stream — its single dimension, since a `Stream` is
always one column. Identical in value to [`nrecords()`](nrecords.md); the two exist as
synonyms so call sites can use whichever reads better in context.

## Parameters

`None`

## Returns

`std::int64_t`
    The row count.

## Examples

Example 1. Size of a plain numeric stream.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

std::cout << stream.size();
```

### Output

```text
3
```

Example 2. `size()` and `nrecords()` always agree — they report the same count.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}));

std::cout << "size: " << stream.size() << "  nrecords: " << stream.nrecords();
```

### Output

```text
size: 5  nrecords: 5
```

Example 3. Calling `size()` on an undefined stream throws.

```cpp
pluss::kindle::Stream undef;

try {
  undef.size();
} catch (const std::exception& e) {
  std::cout << std::string_view(e.what()).substr(0, 24);
}
```

### Output

```text
Stream: undefined handle
```

## Notes

- Requires a defined stream ([`defined()`](defined.md)); throws `"Stream: undefined
  handle"` on a default-constructed `Stream()` — see Example 3.
- Always equal to [`nrecords()`](nrecords.md) and to [`shape()`](shape.md)'s first element.

## See Also

- [`Stream::nrecords`](nrecords.md): the exact synonym for this method.
- [`Stream::shape`](shape.md): returns `(size(), 1)` as a pair.
- [`Stream::empty`](empty.md): `size() == 0`, as a direct boolean check.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="show.html">
    <span class="label">Previous</span>
    <span class="title">show</span>
  </a>
  <a class="next-link" href="slice.html">
    <span class="label">Next</span>
    <span class="title">slice</span>
  </a>
</div>
