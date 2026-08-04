# Note::base64_encode

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note::base64_encode() const -> Note
```

## What It Does

Encodes each text cell into Base64.

This is a transport-oriented method rather than a linguistic one. It is useful
when text must be embedded into a channel that expects Base64-safe content or
when you need a deterministic reversible representation for interchange.

The method transforms the cell contents, not the shape. If you start with one
cell, you get one encoded cell back. If you start with a 1D batch, every batch
member is encoded independently.

## Parameters

This method takes no parameters.

## Returns

`Note`
    A shape-preserving `Note` whose cell contents are Base64-encoded.

## Examples

Example 1. Encode a short text cell into Base64.

```cpp
auto note = pluss::make_note("Hi!");
auto encoded = note.base64_encode();
std::cout << "base64_encode:\n";
for (std::int64_t i = 0; i < encoded.numel(); ++i) {
  std::cout << "  [" << i << "] " << encoded.at(i) << "\n";
}
```

### Output

```text
base64_encode:
  [0] SGkh
```

Example 2. Decode the encoded text back to the original payload.

```cpp
auto note = pluss::make_note("Hi!");
auto encoded = note.base64_encode();
auto roundtrip = encoded.base64_decode();
std::cout << "base64_roundtrip:\n";
for (std::int64_t i = 0; i < roundtrip.numel(); ++i) {
  std::cout << "  [" << i << "] " << roundtrip.at(i) << "\n";
}
```

### Output

```text
base64_roundtrip:
  [0] Hi!
```

Example 3. Encode a batch of text cells, not just a scalar note.

```cpp
auto note = pluss::make_note({"Hi!", "Pluss"});
auto encoded = note.base64_encode();
std::cout << "base64_encode_batch:\n";
for (std::int64_t i = 0; i < encoded.numel(); ++i) {
  std::cout << "  [" << i << "] " << encoded.at(i) << "\n";
}
```

### Output

```text
base64_encode_batch:
  [0] SGkh
  [1] UGx1c3M=
```

## Notes

- `base64_encode()` is reversible when paired with `base64_decode()`.
- This is a content transport transform, not a text-cleaning transform.
- It preserves shape like the rest of the `Note` cellwise transform family.

## See Also

- `Note::base64_decode()`: reverse the encoding.
- [`Note::url_encode`](url_encode.md): use URL-safe percent encoding when the
  target transport expects URL escaping rather than Base64.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nlp_mask_pii.html">
    <span class="label">Previous</span>
    <span class="title">nlp_mask_pii</span>
  </a>
  <a class="next-link" href="url_encode.html">
    <span class="label">Next</span>
    <span class="title">url_encode</span>
  </a>
</div>
