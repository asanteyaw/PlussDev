<!-- pluss-hand-authored -->
# Note::to_windows1252_lossy

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::to_windows1252_lossy() const
```

## Encode Text Down To Windows-1252 Bytes

`to_windows1252_lossy()` is the inverse of
[`from_windows1252`](from_windows1252.md): it takes UTF-8 text and produces a
[bytes-mode](is_bytes.md) `Note` encoded in **Windows-1252**. It is the
choice over [`to_latin1_lossy`](to_latin1_lossy.md) when the text contains
typographic punctuation, because Windows-1252 can represent the smart quotes
`‘ ’ “ ”`, the en/em dashes `– —`, the bullet `•`, the euro sign `€`, and a
few other symbols in its `0x80`–`0x9F` block.

Each character is encoded as:

- its own byte if it is `U+0000`–`U+007F` or `U+00A0`–`U+00FF` (the shared
  Latin-1 range), or
- the corresponding `0x80`–`0x9F` byte if it is one of the Windows-1252
  special characters, or
- a literal `?` (`0x3F`) otherwise.

The input is first passed through [`to_utf8_lossy`](to_utf8_lossy.md), so
malformed spans are cleaned before encoding. The **result is tagged `bytes`**
because the payload is no longer UTF-8. Shape is preserved.

## Example 1: Smart Quotes, Dash, And Euro

```cpp
std::cout << pluss::note::print(
    pluss::make_note({std::string("“") + "q" + "”" + "—" + "€"}).to_windows1252_lossy())
    << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=bytes)
b"\x93q\x94\x97\x80"
```

`“` → `0x93`, `”` → `0x94`, `—` → `0x97`, `€` → `0x80`.

## Example 2: Unrepresentable Characters Become `?`

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"a中b"}).to_windows1252_lossy())
    << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=bytes)
[b"a?b"]
```

`中` (U+4E2D) has no Windows-1252 byte, so it becomes `?`.

## See Also

- [`Note::from_windows1252`](from_windows1252.md): the inverse.
- [`Note::to_latin1_lossy`](to_latin1_lossy.md): plain ISO-8859-1, without
  the `0x80`–`0x9F` characters.
- [`Note::to_ascii`](to_ascii.md): reduce all the way to 7-bit ASCII.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_utf8_strict.html">
    <span class="label">Previous</span>
    <span class="title">to_utf8_strict</span>
  </a>
  <a class="next-link" href="tokenize_words.html">
    <span class="label">Next</span>
    <span class="title">tokenize_words</span>
  </a>
</div>
