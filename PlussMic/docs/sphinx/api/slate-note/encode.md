<!-- pluss-hand-authored -->

# SlateNoteAccessor::encode

[Back to SlateNoteAccessor API Reference](../../slate_note_accessor_api_reference)

## Signature

```cpp
SlateNoteAccessor::encode(std::string_view encoding = "base64") const -> Slate
```

## What It Does

Replaces every cell of the bound slate with an encoded form of its raw bytes,
choosing between three schemes via the `encoding` argument: `"base64"` (the
default), `"hex"`, or `"url"` — a direct forward to
[`Slate::encode()`](../slate/encode.md). Requires the bound slate to be
`Note`-kind. Unlike most methods in this family, `encode()` doesn't validate its
argument against an enum — any other string throws.

- **`"base64"`**: standard Base64 (RFC 4648), using the classic alphabet (`A`-`Z`,
  `a`-`z`, `0`-`9`, `+`, `/`) with `=` padding to a multiple of 4 output characters.
- **`"hex"`**: lowercase hexadecimal, two characters per input byte.
- **`"url"`**: percent-encoding (RFC 3986) — letters, digits, and `-`, `_`, `.`, `~`
  pass through unchanged; every other byte becomes `%XX` with two **uppercase** hex
  digits.

```{note} Encode only — no decode()
There's no built-in way to reverse the transform through this accessor.
```

## Parameters

**encoding** (`std::string_view`, default `"base64"`)

Which scheme to apply: `"base64"`, `"hex"`, or `"url"`. Any other value throws.

## Returns

`Slate`
    A new `Note`-kind slate, same length as the bound slate, holding the encoded
    text.

## Examples

Example 1. The default: `encode()` with no argument uses Base64.

```cpp
using namespace pluss::table;

auto payloads = Slate(pluss::make_note({"Accra Mall", "50% off!", "path/to/file", "x"}),
    pluss::make_series(std::vector<bool>{true, true, true, false}));
auto out1 = payloads.nt().encode();
std::cout << out1;
```

### Output

```text
0  QWNjcmEgTWFsbA==
1  NTAlIG9mZiE=
2  cGF0aC90by9maWxl
3  NULL

ptype: Note
```

Example 2. `"hex"`: two lowercase hex digits per input byte.

```cpp
auto out2 = payloads.nt().encode("hex");
std::cout << out2;
```

### Output

```text
0  4163637261204d616c6c
1  353025206f666621
2  706174682f746f2f66696c65
3  NULL

ptype: Note
```

Example 3. An unrecognized `encoding` argument throws immediately.

```cpp
try {
  auto bad = payloads.nt().encode("base32");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slate::encode: encoding must be base64, hex, or url
```

Example 4. A non-`Note` slate throws — the same message
[`Slate::encode()`](../slate/encode.md) produces directly.

```cpp
auto numbers = Slate(pluss::make_series({1.0, 2.0, 3.0}));
try {
  auto bad = numbers.nt().encode();
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Slate::encode: expected Note payload
```

## Notes

- `SlateNoteAccessor::encode()` is a one-line forward to
  [`Slate::encode()`](../slate/encode.md) — identical behavior and throws.
- Encodes raw bytes, not Unicode codepoints.
- `"hex"` output is always lowercase; `"url"` percent-escapes use uppercase hex
  digits.
- Stays `Note`-kind (text in, text out).
- A null cell on input stays null on output.

## See Also

- [`Slate::encode`](../slate/encode.md): the direct, non-accessor form this method
  forwards to.
- [`SlateNoteAccessor::length`](length.md): count characters instead of encoding
  them.
- [`SlateNoteAccessor::replace`](replace.md): a more general text transform.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="count.html">
    <span class="label">Previous</span>
    <span class="title">count</span>
  </a>
  <a class="next-link" href="ends.html">
    <span class="label">Next</span>
    <span class="title">ends</span>
  </a>
</div>
