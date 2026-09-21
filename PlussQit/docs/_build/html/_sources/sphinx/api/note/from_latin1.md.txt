<!-- pluss-hand-authored -->
# Note::from_latin1

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::from_latin1() const
```

## Decode Latin-1 Bytes Into UTF-8 Text

`from_latin1()` reads each cell's bytes as **ISO-8859-1 (Latin-1)** — where
every byte value `0x00`–`0xFF` is the code point with that same number — and
returns a UTF-8 **text**-mode `Note` holding the decoded characters. Reach for
it when a [bytes-mode](is_bytes.md) `Note` came from a legacy file, protocol,
or database column that stores Western-European text as one byte per
character.

Latin-1 has no undefined byte values, so the decode **never fails and never
substitutes**: every input byte becomes exactly one Unicode scalar, and the
character count of the result equals the byte length of the input. Byte
`0xE9` becomes `é` (U+00E9), `0xA9` becomes `©` (U+00A9), and the ASCII range
`0x00`–`0x7F` is unchanged.

The operation is shape-preserving and cell-independent. The result is tagged
UTF-8 text regardless of the input mode; calling it on a `Note` that is
already text re-interprets that text's *bytes* as Latin-1, which is only
meaningful if the text was mis-tagged. The inverse is
[`to_latin1_lossy`](to_latin1_lossy.md); for the Windows superset that maps
`0x80`–`0x9F` to smart quotes, dashes, and the euro sign, use
[`from_windows1252`](from_windows1252.md).

## Example 1: High Bytes Become Their Latin-1 Characters

```cpp
const pluss::Note raw = pluss::note::bytes({std::string("caf") + '\xE9' + " " + '\xA9'});
std::cout << pluss::note::print(raw.from_latin1()) << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["café ©"]
```

`0xE9` → `é`, `0xA9` → `©`; the result is UTF-8 text.

## Example 2: Round Trip Through `to_latin1_lossy`

```cpp
std::cout << pluss::make_note({std::string("caf") + "é"})
                 .to_latin1_lossy()
                 .from_latin1()
          << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"café"
```

Any text whose characters are all U+0000–U+00FF survives
`to_latin1_lossy().from_latin1()` unchanged.

## See Also

- [`Note::to_latin1_lossy`](to_latin1_lossy.md): the inverse — UTF-8 text
  back to Latin-1 bytes.
- [`Note::from_windows1252`](from_windows1252.md): the Windows-1252 superset
  of Latin-1.
- [`Note::to_utf8_strict`](to_utf8_strict.md) /
  [`Note::to_utf8_lossy`](to_utf8_lossy.md): promote bytes that are *already*
  UTF-8 to text mode.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="for_each_flat.html">
    <span class="label">Previous</span>
    <span class="title">for_each_flat</span>
  </a>
  <a class="next-link" href="from_windows1252.html">
    <span class="label">Next</span>
    <span class="title">from_windows1252</span>
  </a>
</div>
