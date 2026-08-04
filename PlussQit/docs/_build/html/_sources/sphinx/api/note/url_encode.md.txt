# Note::url_encode

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note::url_encode() const -> Note
```

## What It Does

Percent-encodes each text cell for URL-safe transport.

This is the right method when your text must move through a URL or query-string
style channel where spaces and reserved characters need escaping.

Like the Base64 surface, this is transport-oriented rather than NLP-oriented.
Use it when the output must survive URL parsing rules, not when the goal is to
clean or tokenize natural language.

## Parameters

This method takes no parameters.

## Returns

`Note`
    A shape-preserving `Note` whose cells are URL-encoded.

## Examples

Example 1. Encode a string containing a space and a plus sign.

```cpp
auto note = pluss::make_note("a b+c");
auto encoded = note.url_encode();
std::cout << "url_encode:\n";
for (std::int64_t i = 0; i < encoded.numel(); ++i) {
  std::cout << "  [" << i << "] " << encoded.at(i) << "\n";
}
```

### Output

```text
url_encode:
  [0] a%20b%2Bc
```

Example 2. Decode it back to the original text.

```cpp
auto note = pluss::make_note("a b+c");
auto encoded = note.url_encode();
auto roundtrip = encoded.url_decode();
std::cout << "url_roundtrip:\n";
for (std::int64_t i = 0; i < roundtrip.numel(); ++i) {
  std::cout << "  [" << i << "] " << roundtrip.at(i) << "\n";
}
```

### Output

```text
url_roundtrip:
  [0] a b+c
```

Example 3. Encode a batch of cells for query-safe transport.

```cpp
auto note = pluss::make_note({"a b+c", "x/y"});
auto encoded = note.url_encode();
std::cout << "url_encode_batch:\n";
for (std::int64_t i = 0; i < encoded.numel(); ++i) {
  std::cout << "  [" << i << "] " << encoded.at(i) << "\n";
}
```

### Output

```text
url_encode_batch:
  [0] a%20b%2Bc
  [1] x%2Fy
```

## Notes

- `url_encode()` is about transport-safe serialization, not NLP cleanup.
- The example is useful because `+` is encoded too, which makes it clear the
  output is percent-encoded text rather than a whitespace-only rewrite.
- For binary-safe or compact interchange, Base64 may be a better fit than URL
  encoding.

## See Also

- [`Note::base64_encode`](base64_encode.md): alternate reversible transport
  encoding.
- `Note::url_decode()`: reverse the percent encoding.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="base64_encode.html">
    <span class="label">Previous</span>
    <span class="title">base64_encode</span>
  </a>
  <a class="next-link" href="../../note_api_reference.html">
    <span class="label">Next</span>
    <span class="title">Note API Reference</span>
  </a>
</div>
