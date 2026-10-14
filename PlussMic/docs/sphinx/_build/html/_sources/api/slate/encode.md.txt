<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5187620e430e -->

# Slate::encode

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
Slate::encode(std::string_view encoding = "base64") const -> Slate
```

## What It Does

Replaces every cell of a `Note` (string) slate with an encoded form of its raw
bytes, choosing between three schemes via the `encoding` argument: `"base64"` (the
default), `"hex"`, or `"url"`. Requires the slate to be `Note`-kind. Unlike most
methods in this family, `encode()` doesn't validate its argument against an enum —
any other string throws.

- **`"base64"`**: standard Base64 (RFC 4648), using the classic alphabet (`A`-`Z`,
  `a`-`z`, `0`-`9`, `+`, `/`) with `=` padding to a multiple of 4 output characters.
- **`"hex"`**: lowercase hexadecimal, two characters per input byte (so the output is
  always exactly twice as long as the input, in bytes).
- **`"url"`**: percent-encoding (RFC 3986) — letters, digits, and `-`, `_`, `.`, `~`
  pass through unchanged; every other byte becomes `%XX` with two **uppercase** hex
  digits.

```{note} Encode only — no decode()
`Slate` exposes `encode()` but no corresponding `decode()`. There's no built-in way
to reverse the transform on a `Slate`.
```

## Parameters

**encoding** (`std::string_view`, default `"base64"`)
    Which scheme to apply: `"base64"`, `"hex"`, or `"url"`. Any other value throws
    `"Slate::encode: encoding must be base64, hex, or url"`.

## Returns

`Slate`
    A new `Note`-kind slate, same length as `*this`, holding the encoded text.

## Examples

The examples below share one input slate containing a mix of plain text,
punctuation, and a space:

```cpp
using namespace pluss::table;

auto payloads = Slate(pluss::make_note({"Accra Mall", "50% off!", "path/to/file", "x"}),
    pluss::make_series(std::vector<bool>{true, true, true, false}));
std::cout << payloads;
```

### Output (before, shared by every example)

```text
0  Accra Mall
1  50% off!
2  path/to/file
3  NULL

ptype: Note
```

Example 1. The default: `encode()` with no argument uses Base64.

```cpp
auto out1 = payloads.encode();
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

`"Accra Mall"` is 10 bytes, needing 2 bytes of `=` padding to round up to a full
16-character block. `"50% off!"` is 8 bytes, one short of a multiple of 3, so it gets
a single `=`. `"path/to/file"` is 12 bytes — an exact multiple of 3 — so it encodes
cleanly with no padding at all.

Example 2. `"hex"`: two lowercase hex digits per input byte.

```cpp
auto out2 = payloads.encode("hex");
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

`"50% off!"` (8 bytes) becomes a 16-character hex string — always exactly twice as
many characters as input bytes, since every byte becomes exactly two hex digits.

Example 3. `"url"`: percent-encode everything outside the unreserved set, using
uppercase hex digits after the `%`.

```cpp
auto out3 = payloads.encode("url");
std::cout << out3;
```

### Output

```text
0  Accra%20Mall
1  50%25%20off%21
2  path%2Fto%2Ffile
3  NULL

ptype: Note
```

`"Accra Mall"`'s space becomes `%20`. `"50% off!"`'s literal `%` becomes `%25` (it
has to be escaped too, since `%` itself isn't in the unreserved set), its space
becomes `%20`, and its `!` becomes `%21`. `"path/to/file"`'s `/` characters each
become `%2F`, all with uppercase hex digits.

Example 4. An unrecognized `encoding` argument throws immediately.

```cpp
try {
  auto bad = payloads.encode("base32");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slate::encode: encoding must be base64, hex, or url
```

Example 5. A non-`Note` slate throws.

```cpp
auto numbers = Slate(pluss::make_series({1.0, 2.0, 3.0}));
try {
  auto bad = numbers.encode();
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

- Requires the calling slate to be `Note`-kind.
- Encodes raw bytes, not Unicode codepoints — for ASCII text, "bytes" and
  "characters" coincide, but multi-byte UTF-8 characters encode as their full
  multi-byte sequence, not as one unit.
- `"hex"` output is always lowercase; `"url"` percent-escapes use uppercase hex
  digits — the two schemes intentionally differ in this convention, matching common
  real-world usage of each format.
- Stays `Note`-kind (text in, text out) — unlike [`length()`](length.md), which
  switches to `Series`.
- A null cell on input stays null on output.
- No matching `decode()` exists — see the note above.

## See Also

- [`Slate::length`](length.md): count characters instead of encoding them.
- [`Slate::replace`](replace.md): a more general text transform, useful for ad-hoc
  escaping needs `encode()` doesn't cover.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="duplicated.html">
    <span class="label">Previous</span>
    <span class="title">duplicated</span>
  </a>
  <a class="next-link" href="ends.html">
    <span class="label">Next</span>
    <span class="title">ends</span>
  </a>
</div>
