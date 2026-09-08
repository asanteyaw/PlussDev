<!-- pluss-hand-authored -->
# Note::base64_decode

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::base64_decode() const
```

## Decode Every Cell From Base64

`base64_decode()` is the inverse of [`base64_encode`](base64_encode.md): it
reads each cell as standard Base64 (RFC 4648 alphabet, `=` padding) and
replaces it with the decoded bytes. Use it to unpack values that were carried
through an ASCII-only channel.

The decoded bytes are placed into a UTF-8-mode `Note` as-is; if the original
data was text they read back as text, and if it was binary the cell simply
holds those bytes. For anything that produced a `Note` with
`base64_encode()`, this is an exact round trip.

Decoding is **lenient**: characters outside the Base64 alphabet are skipped
rather than raising an error, and a cell that is not Base64 at all comes back
best-effort (often unchanged). Do not rely on `base64_decode()` to *validate*
input — check the shape of the data yourself if it matters. The operation is
shape-preserving, cell-independent, and returns a fresh `Note`; an empty cell
decodes to an empty cell.

## Example 1: Decode A Batch

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"SGVsbG8=", "Y2Fmw6k=", ""}).base64_decode())
    << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["Hello", "café", ""]
```

`"Y2Fmw6k="` decodes to the five UTF-8 bytes of `"café"`.

## Example 2: Exact Round Trip

```cpp
const pluss::Note original = pluss::make_note({"payload", "café"});
std::cout << original.base64_encode().base64_decode() << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["payload", "café"]
```

## Example 3: Lenient On Non-Base64 Input

```cpp
pluss::note::print(std::cout,
                   pluss::make_note({"not!!base64"}).base64_decode())
    << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["not!!base64"]
```

No exception — the input is returned best-effort.

## See Also

- [`Note::base64_encode`](base64_encode.md): the inverse.
- [`Note::hex_decode`](hex_decode.md) / [`Note::url_decode`](url_decode.md):
  the other transport decoders.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../../note_api_reference.html">
    <span class="label">Previous</span>
    <span class="title">Note API Reference</span>
  </a>
  <a class="next-link" href="base64_encode.html">
    <span class="label">Next</span>
    <span class="title">base64_encode</span>
  </a>
</div>
