<!-- pluss-hand-authored -->
# Note::to_latin1_lossy

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::to_latin1_lossy() const
```

## Encode Text Down To Latin-1 Bytes

`to_latin1_lossy()` is the inverse of [`from_latin1`](from_latin1.md): it
takes UTF-8 text and produces a [bytes-mode](is_bytes.md) `Note` in which
each character `U+0000`–`U+00FF` is written as its single ISO-8859-1 byte.
Any character **outside** that range has no Latin-1 representation and is
replaced by a literal `?` (`0x3F`). Use it to hand text to a component that
only accepts Latin-1.

The input is first run through [`to_utf8_lossy`](to_utf8_lossy.md), so a
malformed cell is cleaned (bad spans become `` first, which is itself
non-Latin-1 and therefore ends up as `?`). The **result is tagged `bytes`**,
not text — printing shows the `b"…"` form with `\xNN` for the high bytes —
because the payload is no longer UTF-8. Shape is preserved.

The mapping is exact and reversible for the characters it can represent:
`from_latin1()` on the output recovers every character that was not turned
into `?`.

## Example 1: A Representable Character Round-Trips

```cpp
std::cout << pluss::note::print(
    pluss::make_note({std::string("caf") + "é"}).to_latin1_lossy())
    << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=bytes)
b"caf\xE9"
```

`é` (U+00E9) becomes the one byte `0xE9`; the `Note` is now in bytes mode.

## Example 2: Out-Of-Range Characters Become `?`

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"a€b中c"}).to_latin1_lossy())
    << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=bytes)
[b"a?b?c"]
```

`€` (U+20AC) and `中` (U+4E2D) are both beyond `U+00FF`, so each is one `?`.

## Example 3: Reverse With `from_latin1`

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

## See Also

- [`Note::from_latin1`](from_latin1.md): the inverse — Latin-1 bytes to text.
- [`Note::to_windows1252_lossy`](to_windows1252_lossy.md): the Windows
  superset, which can also represent smart quotes, dashes, and the euro sign.
- [`Note::to_ascii`](to_ascii.md): a harsher reduction, to 7-bit ASCII only.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_ascii.html">
    <span class="label">Previous</span>
    <span class="title">to_ascii</span>
  </a>
  <a class="next-link" href="to_nfc.html">
    <span class="label">Next</span>
    <span class="title">to_nfc</span>
  </a>
</div>
