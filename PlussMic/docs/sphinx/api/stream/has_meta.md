<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3d8ccc0761df -->

# Stream::has_meta

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
bool Stream::has_meta(std::string_view key) const
```

## What It Does

Reports whether `key` exists in the stream's free-form metadata map — an
extensible `std::unordered_map<std::string, std::string>` of arbitrary string key/value
pairs attached to the stream (units, provenance, descriptions, anything caller-defined),
distinct from the row-label axis (see [`lbl()`](lbl.md)) and from the payload data itself.

## Parameters

**key** (`std::string_view`)
    The metadata key to check for.

## Returns

`bool`
    `true` if `key` is present in the stream's metadata map; `false` otherwise.

## Examples

Example 1. A fresh stream has no metadata.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

std::cout << stream.has_meta("units");
```

### Output

```text
0
```

Example 2. After [`with_meta()`](with_meta.md), the key is present.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}))
                  .with_meta("units", "kg");

std::cout << stream.has_meta("units");
```

### Output

```text
1
```

## Notes

- Checking an unknown key returns `false`; it never throws for a missing key.
- Requires a defined stream; throws `"Stream: undefined handle"` on a default-constructed
  `Stream()`.

## See Also

- [`Stream::meta`](meta.md): retrieve the value for a key this method confirms exists.
- [`Stream::with_meta`](with_meta.md) / [`Stream::with_meta_`](with_meta_.md): set the key
  this method checks for.
- [`Stream::drop_meta`](drop_meta.md) / [`Stream::drop_meta_`](drop_meta_.md): remove a key.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="has_label.html">
    <span class="label">Previous</span>
    <span class="title">has_label</span>
  </a>
  <a class="next-link" href="head.html">
    <span class="label">Next</span>
    <span class="title">head</span>
  </a>
</div>
