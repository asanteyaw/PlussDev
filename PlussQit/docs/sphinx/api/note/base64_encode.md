<!-- pluss-hand-authored -->
# Note::base64_encode

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::base64_encode() const
```

## Encode Every Cell As Standard Base64

`base64_encode()` replaces each text cell with its Base64 transfer encoding,
using the standard alphabet (`A-Z`, `a-z`, `0-9`, `+`, `/`) and `=` padding
(RFC 4648 §4). It is the method to reach for when a `Note` has to travel
through a channel that only carries printable ASCII — a JSON string, a URL
fragment budget permitting, an HTTP header, a log line, a config value.

The input is treated as a byte sequence, not as characters: a cell holding
`"café"` is encoded from its four UTF-8 bytes (`63 61 66 C3 A9` — five bytes),
so the result depends on the exact bytes, not on how many characters they
render as. This is why the encoded length grows in blocks of four output
characters per three input bytes, with `=` padding when the final block is
short.

The operation is **shape-preserving** and cell-independent: the result has
the same rank and extents as the input, each cell encoded on its own, and a
new allocation is returned. The result is a UTF-8-mode `Note` (the Base64
alphabet is ASCII). An empty cell encodes to an empty cell.

[`base64_decode`](../../note_api_reference.md) is the exact inverse for any
`Note` produced here.

## Example 1: A Scalar

```cpp
std::cout << pluss::note::print(pluss::make_note("Hello, Note").base64_encode())
          << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"SGVsbG8sIE5vdGU="
```

## Example 2: Padding Follows The Input Length

```cpp
const pluss::Note n =
    pluss::make_note({"abc", "ab", "a", ""}).base64_encode();
std::cout << n << "\n";
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["YWJj", "YWI=", "YQ==", ""]
```

Three input bytes give a full four-character block with no padding; two bytes
give one `=`; one byte gives two `=`. An empty cell stays empty.

## Example 3: Multi-Byte UTF-8 Is Encoded From Its Bytes

```cpp
pluss::note::print(std::cout,
                   pluss::make_note({"café", "€"}).base64_encode())
    << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["Y2Fmw6k=", "4oKs"]
```

`"café"` is five bytes (`é` is two), and `"€"` is three bytes, so they encode
to eight and four Base64 characters respectively.

## Example 4: Shape Is Preserved

```cpp
std::cout << pluss::note::print(
    pluss::make_note({{"one", "two"}, {"three", "four"}}).base64_encode())
    << "\n";
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["b25l", "dHdv"], 
  ["dGhyZWU=", "Zm91cg=="]
]
```

## Example 5: Round-Trips Through `base64_decode`

```cpp
const pluss::Note original = pluss::make_note({"payload", "café"});
std::cout << original.base64_encode().base64_decode() << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["payload", "café"]
```

`base64_decode()` recovers the exact original bytes.

## See Also

- `pluss::coden::Note::base64_decode`: the inverse.
- [`Note::url_encode`](url_encode.md): percent-encoding for the same
  "make it ASCII-safe" goal, keyed to URL syntax instead of transfer size.
- [`NoteTexAccessor::escape`](../note-tex/escape.md): escaping for a specific consumer
  (TeX) rather than a general binary-safe encoding.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="base64_decode.html">
    <span class="label">Previous</span>
    <span class="title">base64_decode</span>
  </a>
  <a class="next-link" href="begin.html">
    <span class="label">Next</span>
    <span class="title">begin</span>
  </a>
</div>
