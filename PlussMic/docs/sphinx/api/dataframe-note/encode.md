<!-- pluss-hand-authored -->

# DataFrameNoteAccessor::encode

[Back to DataFrameNoteAccessor API Reference](../../dataframe_note_accessor_api_reference)

## Signature

```cpp
DataFrameNoteAccessor::encode(std::string_view encoding = "base64") const -> DataFrame
```

## What It Does

Replaces every selected text feature's cells with an encoded form of their
raw bytes, choosing between three schemes via the `encoding` argument:
`"base64"` (the default), `"hex"`, or `"url"`. Unlike most methods on this
accessor, `encode()` doesn't validate its argument against an enum — any
other string throws.

- **`"base64"`**: standard Base64 (RFC 4648), using the classic alphabet
  (`A`-`Z`, `a`-`z`, `0`-`9`, `+`, `/`) with `=` padding to a multiple of 4
  output characters.
- **`"hex"`**: lowercase hexadecimal, two characters per input byte (so the
  output is always exactly twice as long as the input, in bytes).
- **`"url"`**: percent-encoding (RFC 3986) — letters, digits, and
  `-`, `_`, `.`, `~` pass through unchanged; every other byte becomes
  `%XX` with two **uppercase** hex digits.

```{note} Encode only — no decode() on this accessor
This accessor exposes `encode()` but no corresponding `decode()`. There's
no built-in way to reverse the transform through `DataFrameNoteAccessor`
itself.
```

## Parameters

**encoding** (`std::string_view`, default `"base64"`)
    Which scheme to apply: `"base64"`, `"hex"`, or `"url"`. Any other value
    throws `"Thread::encode: encoding must be base64, hex, or url"`.

## Returns

`DataFrame`
    A new frame with every selected feature's text replaced by its encoded
    form.

## Examples

The examples below share one input frame containing a mix of plain text,
punctuation, and a space:

```cpp
using namespace pluss::table;

auto payloads = DataFrame({
    {"text", Thread(pluss::make_note(
        {"Hi", "Pluss!", "a b/c", "x"}),
        pluss::make_series(std::vector<bool>{true, true, true, false}))},
});
std::cout << payloads;
```

### Output (before, shared by every example)

```text
=============
     | text  
-------------
0    | Hi    
1    | Pluss!
2    | a b/c 
3    | NULL  
-------------
[4 cs x 1 feat]
```

Example 1. The default: `encode()` with no argument uses Base64.

```cpp
auto out1 = payloads.note().encode();
std::cout << out1;
```

### Output

```text
===============
     | text    
---------------
0    | SGk=    
1    | UGx1c3Mh
2    | YSBiL2M=
3    | NULL    
---------------
[4 cs x 1 feat]
```

`"Hi"` is 2 bytes, so it needs 2 bytes of `=` padding to round up to a full
4-character block: `"SGk="`. `"Pluss!"` is 6 bytes — an exact multiple of
3 — so it encodes to 8 characters with no padding. `"a b/c"` is 5 bytes, one
short of a multiple of 3, so it gets a single `=`.

Example 2. `"hex"`: two lowercase hex digits per input byte.

```cpp
auto out2 = payloads.note().encode("hex");
std::cout << out2;
```

### Output

```text
===================
     | text        
-------------------
0    | 4869        
1    | 506c75737321
2    | 6120622f63  
3    | NULL        
-------------------
[4 cs x 1 feat]
```

`"Hi"` (2 bytes: `H`=0x48, `i`=0x69) becomes `"4869"` — always exactly
twice as many characters as input bytes, since every byte becomes exactly
two hex digits.

Example 3. `"url"`: percent-encode everything outside the unreserved set,
using uppercase hex digits after the `%`.

```cpp
auto out3 = payloads.note().encode("url");
std::cout << out3;
```

### Output

```text
================
     | text     
----------------
0    | Hi       
1    | Pluss%21 
2    | a%20b%2Fc
3    | NULL     
----------------
[4 cs x 1 feat]
```

`"Hi"` is unchanged — every character in it (`H`, `i`) is in the unreserved
set (letters, digits, `-`, `_`, `.`, `~`). `"Pluss!"`'s `!` becomes `%21`.
`"a b/c"`'s space becomes `%20` (not `+`, the older `application/x-www-form-urlencoded`
convention — this is plain RFC 3986 percent-encoding) and its `/` becomes
`%2F`, both with uppercase hex digits.

Example 4. An unrecognized `encoding` argument throws immediately.

```cpp
try {
  auto bad = payloads.note().encode("base32");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread::encode: encoding must be base64, hex, or url
```

## Notes

- Encodes raw bytes, not Unicode codepoints — for ASCII text, "bytes" and
  "characters" coincide, but multi-byte UTF-8 characters encode as their
  full multi-byte sequence, not as one unit.
- `"hex"` output is always lowercase; `"url"` percent-escapes use uppercase
  hex digits — the two schemes intentionally differ in this convention,
  matching common real-world usage of each format.
- Changes the feature's payload kind but stays `Note` (text in, text out) —
  unlike [`length()`](length.md), which switches to `Series`.
- A null cell on input stays null on output.
- No matching `decode()` exists on this accessor — see the note above.

## See Also

- [`DataFrameNoteAccessor::length`](length.md): count characters instead of
  encoding them.
- [`DataFrameNoteAccessor::replace`](replace.md): a more general text
  transform, useful for ad-hoc escaping needs `encode()` doesn't cover.

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
