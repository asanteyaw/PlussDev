<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f1d063783034 -->

# Stream::to_json

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
std::string Stream::to_json() const
```

## What It Does

Serializes the stream to a JSON string describing its structure — kind, dtype, size,
values, and a validity mask when the stream has one. This is the same per-stream JSON
shape that appears nested inside each entry of
[`Catalog::to_json()`](../catalog/to_json.md)'s `"streams"` array.

## Parameters

`None`

## Returns

`std::string`
    A JSON document describing the stream's kind, dtype, size, values, and (if present)
    validity mask.

## Examples

Example 1. Serialize a plain numeric stream, no validity mask.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}));

std::cout << stream.to_json();
```

### Output

```text
{
  "_type": "Stream",
  "defined": true,
  "kind": "Series",
  "dtype": "Series[float64]",
  "size": 3,
  "values": [
    1,
    2,
    3
  ]
}
```

Example 2. A stream with a validity mask gets an extra `"validity"` array — invalid cells
still show their underlying stored value in `"values"`, not JSON `null`.

```cpp
auto mask = pluss::make_series(std::vector<bool>{true, false, true});
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}), mask);

std::cout << stream.to_json();
```

### Output

```text
{
  "_type": "Stream",
  "defined": true,
  "kind": "Series",
  "dtype": "Series[float64]",
  "size": 3,
  "values": [
    1,
    2,
    3
  ],
  "validity": [
    true,
    false,
    true
  ]
}
```

Row `1` is invalid (per `"validity"`), but `"values"` still shows `2` at that position —
check `"validity"` to know which entries actually matter.

## Notes

- `"validity"` only appears when the stream actually has a validity mask; a stream with no
  `NULL`s at all omits the key entirely (see Example 1).
- Invalid cells keep their underlying stored value in `"values"` — only `"validity"` marks
  them as not meaningful.
- The same per-stream shape as the nested `"value"` objects inside
  [`Catalog::to_json()`](../catalog/to_json.md)'s `"streams"` array.

## See Also

- [`Stream::to_string`](to_string.md): the plain ASCII-table rendering, a distinct text
  representation from this structural JSON dump.
- [`Catalog::to_json`](../catalog/to_json.md): the whole-catalog structural dump this
  method's output nests inside, one per stream.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="take.html">
    <span class="label">Previous</span>
    <span class="title">take</span>
  </a>
  <a class="next-link" href="to_string.html">
    <span class="label">Next</span>
    <span class="title">to_string</span>
  </a>
</div>
