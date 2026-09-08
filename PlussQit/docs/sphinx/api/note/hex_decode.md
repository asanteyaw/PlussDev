<!-- pluss-hand-authored -->
# Note::hex_decode

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::hex_decode() const
```

## Decode Every Cell From Hexadecimal

`hex_decode()` is the inverse of [`hex_encode`](hex_encode.md): it reads each
cell as a run of hex digit pairs and replaces it with the decoded bytes. Both
lowercase and uppercase digits are accepted. Use it to unpack a value that
was carried as hex.

The decoded bytes go into a UTF-8-mode `Note` as-is. For any `Note` produced
by `hex_encode()` this is an exact round trip. Decoding is lenient: it does
not raise on stray non-hex characters or an odd digit count — it decodes what
it can. Do not use it as a validator.

The operation is shape-preserving, cell-independent, and returns a fresh
`Note`; an empty cell decodes to an empty cell.

## Example 1: Decode A Batch

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"486921", "636166c3a9"}).hex_decode())
    << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["Hi!", "café"]
```

## Example 2: Exact Round Trip

```cpp
const pluss::Note original = pluss::make_note({"Hi!", "café"});
std::cout << original.hex_encode().hex_decode() << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["Hi!", "café"]
```

## See Also

- [`Note::hex_encode`](hex_encode.md): the inverse.
- [`Note::base64_decode`](base64_decode.md) /
  [`Note::url_decode`](url_decode.md): the other transport decoders.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ends_with.html">
    <span class="label">Previous</span>
    <span class="title">ends_with</span>
  </a>
  <a class="next-link" href="hex_encode.html">
    <span class="label">Next</span>
    <span class="title">hex_encode</span>
  </a>
</div>
