<!-- pluss-hand-authored -->
# pluss::note::bytes

[Back to Note Factory API Reference](../../note_factory_api_reference)

## Signatures

```cpp
Note pluss::note::bytes(text_source values)
Note pluss::note::bytes(string_range flat, integer_range sizes)
Note pluss::note::bytes(byte_range raw)
```

## Raw Byte Content, No UTF-8 Assumption

`bytes()` builds a `Note` in **bytes mode** (`encoding=bytes`). It is the
right constructor whenever the content is not guaranteed to be well-formed
text: binary blobs, protocol frames, hashes, encrypted payloads, arbitrary
file contents, or text in an unknown or mixed encoding.

The difference from [`make`](make.md) is a contract, not a storage change.
Both keep the exact bytes you give them. But a UTF-8-mode `Note` promises its
cells are text and lets text methods (character length, case mapping, regex,
tokenization) act on them; a bytes-mode `Note` makes no such promise, and its
printer shows the bytes rather than pretending they render as characters.
Convert a bytes `Note` to text explicitly once you know the encoding is safe.

`bytes()` accepts the same call-site spellings as `make()` — a scalar, a
braced list, a nested block up to rank `3`, or a contiguous range of strings
— plus one form `make()` does not have:

- **`byte_range raw`** — a contiguous range of `std::byte` read as **one
  opaque scalar cell**. Use this for a buffer that is genuinely a single
  blob, including one that contains embedded `NUL` bytes.

The `(flat, sizes)` overload works exactly as it does for `make()`: the flat
cell count must equal the product of `sizes`.

### Printing bytes

`pluss::note::print` shows a bytes-mode `Note` with a `b"..."` marker by
default (`NotePrintBytesMode::Escaped`). Printable ASCII is shown as-is;
every other byte — controls, `DEL`, and everything `>= 0x80` — is written as
a `\xNN` escape, so the escaped view is always plain ASCII and safe to paste
into text. Set `opt.bytes_mode = pluss::NotePrintBytesMode::Hex` for a
`hex[..]` byte grid instead — the same information without the `\x` noise
when almost nothing is printable.

### A C++ string-literal caveat

`bytes({"\x00\x01\x02"})` does **not** store three bytes: a C string literal
ends at its first `NUL`, so `"\x00..."` is an empty string before it ever
reaches `bytes()`. To carry embedded `NUL`s, use the `byte_range` overload
(Example 5) or a `std::string` with an explicit length. This is a property of
C++ literals, not of `bytes()`.

## Example 1: A Scalar Of ASCII Bytes

```cpp
std::cout << pluss::note::print(pluss::note::bytes("plain ascii bytes")) << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=bytes)
b"plain ascii bytes"
```

Same bytes as `pluss::note::make("plain ascii bytes")` would hold — but
`encoding=bytes` and the `b"..."` marker signal "not promised to be text".

## Example 2: A One-Dimensional Batch (Default Escaped Print)

```cpp
const pluss::Note n = pluss::note::bytes({"AB", "\xDE\xAD\xBE\xEF"});
std::cout << n << "\n";  // operator<< == print() with default options
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=bytes)
[b"AB", b"\xDE\xAD\xBE\xEF"]
```

`AB` is printable so it shows literally; `DE AD BE EF` are non-ASCII bytes,
so the escaped printer writes each as a `\xNN` escape rather than emitting
raw bytes. The next example shows the same value as a hex grid.

## Example 3: The Same Value, Hex Dump

```cpp
pluss::NotePrintOptions hex;
hex.bytes_mode = pluss::NotePrintBytesMode::Hex;
std::cout << pluss::note::print(pluss::note::bytes({"AB", "\xDE\xAD\xBE\xEF"}), hex)
          << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=bytes)
[hex[41 42], hex[de ad be ef]]
```

`hex[..]` shows every byte unambiguously. Prefer it whenever the content is
not mostly printable.

## Example 4: Flat Storage Plus An Explicit Shape

```cpp
std::vector<std::string> flat{"one", "two", "three", "four"};
std::array<std::int64_t, 2> sizes{2, 2};
const pluss::Note n = pluss::note::bytes(
    pluss::msharp::RangeView::Of<std::string>(flat),
    pluss::msharp::RangeView::Of<std::int64_t>(sizes));
pluss::note::print(std::cout, n) << "\n";  // print(os, n) returns the stream
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=bytes)
[
  [b"one", b"two"], 
  [b"three", b"four"]
]
```

## Example 5: A Raw `std::byte` Buffer As One Scalar

```cpp
std::array<std::byte, 4> raw{std::byte{0x48}, std::byte{0x69},
                             std::byte{0x21}, std::byte{0x00}};
const pluss::Note n = pluss::note::bytes(
    pluss::msharp::RangeView::Of<std::byte>(raw));
std::cout << n << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=bytes)
b"Hi!\x00"
```

The whole range is one cell, and the trailing `NUL` is kept — unlike a C
string literal, which would have stopped at it. The same value as a hex dump:

```cpp
std::cout << pluss::note::print(n, hex) << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=bytes)
hex[48 69 21 00]
```

## Example 6: The String-Literal `NUL` Caveat, Shown

```cpp
std::cout << pluss::note::print(pluss::note::bytes({"\x00\x01\x02", "kept"}), hex)
          << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=bytes)
[hex[], hex[6b 65 70 74]]
```

The first cell is **empty** — `"\x00\x01\x02"` is a zero-length C literal.
The second cell is fine (`kept` = `6b 65 70 74`). Reach for the `byte_range`
overload when `NUL`s must survive.

## Example 7: Escaped Vs Hex On The Same Cells

```cpp
const pluss::Note n = pluss::note::bytes({"\xC3\xA9", "\xFF"});
std::cout << pluss::note::print(n) << "\n";        // default: escaped
std::cout << pluss::note::print(n, hex) << "\n";   // hex
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=bytes)
[b"\xC3\xA9", b"\xFF"]
Note(shape=[2], dim=1, numel=2, encoding=bytes)
[hex[c3 a9], hex[ff]]
```

In a **bytes**-mode `Note` every non-ASCII byte is escaped, so `C3 A9` (which
would be `é` as UTF-8 text) and the lone `FF` both show as `\x` escapes —
the printer does not try to decode them. The hex view is the same bytes in
lower-case without the `\x`. Contrast [`make`](make.md), where a UTF-8-mode
`Note` holding those same `C3 A9` bytes prints as `é`.

## See Also

- [`pluss::note::make`](make.md): the UTF-8-mode constructor — use it when
  the content is known-good text.
- [`pluss::note::full_bytes_like`](full_bytes_like.md) /
  [`pluss::note::empty_bytes_like`](empty_bytes_like.md): shape-first
  bytes-mode construction from an existing `Note`.
- [`pluss::note::print`](print.md): the renderer and its `bytes_mode`
  option.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="arange.html">
    <span class="label">Previous</span>
    <span class="title">arange</span>
  </a>
  <a class="next-link" href="empty.html">
    <span class="label">Next</span>
    <span class="title">empty</span>
  </a>
</div>
