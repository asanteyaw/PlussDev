<!-- pluss-hand-authored -->
# Note::hex_encode

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::hex_encode() const
```

## Encode Every Cell As Hexadecimal

`hex_encode()` replaces each cell with the lowercase hexadecimal
representation of its bytes: every input byte becomes exactly two hex digits,
with no separators and no `0x` prefix. Use it when you need a fully
unambiguous, fixed-ratio ASCII rendering of arbitrary bytes — a hash-like
identifier, a debug dump, a value for a system that rejects Base64's `+`/`/`.

Compared with [`base64_encode`](base64_encode.md), hex is simpler to read and
byte-aligned (2 characters per byte, always), at the cost of being twice the
size rather than four-thirds. Like Base64 it operates on the **bytes** of the
cell, so `"café"` encodes its five UTF-8 bytes.

The result is a UTF-8-mode `Note` (hex digits are ASCII). The operation is
shape-preserving, cell-independent, and returns a fresh `Note`; an empty cell
encodes to an empty cell. [`hex_decode`](hex_decode.md) is the exact inverse.

## Example 1: A Batch

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"Hi!", "café", ""}).hex_encode())
    << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["486921", "636166c3a9", ""]
```

`"Hi!"` is three bytes → six hex digits; `"café"` is five bytes →
`636166c3a9`.

## Example 2: Control Bytes

```cpp
const pluss::Note n = pluss::make_note("\n\t").hex_encode();
std::cout << n << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"0a09"
```

Newline (`0a`) and tab (`09`).

## Example 3: Round-Trips Through `hex_decode`

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"payload", "€"}).hex_encode().hex_decode())
    << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["payload", "€"]
```

## See Also

- [`Note::hex_decode`](hex_decode.md): the inverse.
- [`Note::base64_encode`](base64_encode.md): more compact ASCII-safe
  encoding.
- [`Note::url_encode`](url_encode.md): percent-encoding for URL contexts.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="hex_decode.html">
    <span class="label">Previous</span>
    <span class="title">hex_decode</span>
  </a>
  <a class="next-link" href="host.html">
    <span class="label">Next</span>
    <span class="title">host</span>
  </a>
</div>
