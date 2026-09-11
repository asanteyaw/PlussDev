<!-- pluss-hand-authored -->
# Note::from_windows1252

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::from_windows1252() const
```

## Decode Windows-1252 Bytes Into UTF-8 Text

`from_windows1252()` reads each cell's bytes as **Windows-1252** (the
"ANSI" code page long used by Windows and still common in exported CSVs,
email, and older web pages) and returns a UTF-8 **text**-mode `Note`. It is
the natural choice over [`from_latin1`](from_latin1.md) when the source is
Windows software, because Windows-1252 fills the `0x80`–`0x9F` range — which
Latin-1 leaves as control codes — with printable characters: the euro sign
`0x80` → `€`, the smart quotes `0x91`–`0x94` → `‘ ’ “ ”`, the dashes `0x96`
`0x97` → `– —`, the bullet `0x95` → `•`, and so on.

Bytes `0x00`–`0x7F` and `0xA0`–`0xFF` decode exactly as in Latin-1. Five
positions in the `0x80`–`0x9F` block are unassigned in Windows-1252 (`0x81`,
`0x8D`, `0x8F`, `0x90`, `0x9D`); each of those becomes a literal `?`. Every
other byte maps to exactly one character, so the decode never raises.

The operation is shape-preserving and cell-independent, and the result is
tagged UTF-8 text regardless of input mode. The inverse is
[`to_windows1252_lossy`](to_windows1252_lossy.md).

## Example 1: Smart Punctuation And The Euro Sign

```cpp
const pluss::Note raw = pluss::note::bytes(
    {std::string("\x93") + "quote" + "\x94" + " " + "\x97" + " bullet " +
     "\x95" + " euro " + "\x80"});
std::cout << pluss::note::print(raw.from_windows1252()) << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["“quote” — bullet • euro €"]
```

## Example 2: Unassigned Bytes Become `?`

```cpp
std::cout << pluss::note::bytes({std::string("x\x81y\x8d") + "z"})
                 .from_windows1252()
          << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["x?y?z"]
```

`0x81` and `0x8D` have no Windows-1252 character, so each decodes to `?`.

## See Also

- [`Note::to_windows1252_lossy`](to_windows1252_lossy.md): the inverse.
- [`Note::from_latin1`](from_latin1.md): the plain ISO-8859-1 decode, without
  the `0x80`–`0x9F` characters.
- [`Note::to_utf8_lossy`](to_utf8_lossy.md): use instead when the bytes are
  already UTF-8.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="from_latin1.html">
    <span class="label">Previous</span>
    <span class="title">from_latin1</span>
  </a>
  <a class="next-link" href="hex_decode.html">
    <span class="label">Next</span>
    <span class="title">hex_decode</span>
  </a>
</div>
