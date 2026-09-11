<!-- pluss-hand-authored -->
# Note::to_utf8_lossy

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::to_utf8_lossy() const
```

## Promote Bytes To Text, Replacing Anything Not Valid UTF-8

`to_utf8_lossy()` converts a [bytes-mode](is_bytes.md) `Note` to **UTF-8
text**, walking each cell as UTF-8 and substituting the Unicode replacement
character `�` (U+FFFD) for every byte or partial sequence that is not
well-formed. Unlike [`to_utf8_strict`](to_utf8_strict.md) it never raises —
reach for it when you would rather carry on with a marked-up best effort than
fail on a malformed payload (log lines, scraped text, mixed-encoding dumps).

Well-formed sequences are copied through byte-for-byte; only the invalid
spans are rewritten, and each maladjusted byte contributes one `�` (so
`\xFF\x80` becomes two replacement characters). A cell that is already valid
UTF-8 is unchanged apart from the encoding tag. The operation is
shape-preserving and cell-independent.

Because it consumes anything, `to_utf8_lossy()` is also the front half of
[`to_latin1_lossy`](to_latin1_lossy.md) and
[`to_windows1252_lossy`](to_windows1252_lossy.md): they normalize to clean
UTF-8 with this routine before encoding down to the single-byte target.

## Example 1: Invalid Bytes Become The Replacement Character

```cpp
std::cout << pluss::note::print(
    pluss::note::bytes({std::string("ok-") + '\xFF' + "-" + '\x80' + "-end"})
        .to_utf8_lossy())
    << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["ok-�-�-end"]
```

`\xFF` and `\x80` are each replaced by one `�` (U+FFFD); the ASCII around
them is untouched.

## Example 2: Valid UTF-8 Is Unchanged

```cpp
std::cout << pluss::note::bytes({"café"}).to_utf8_lossy() << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["café"]
```

## See Also

- [`Note::to_utf8_strict`](to_utf8_strict.md): raise instead of substituting.
- [`Note::from_latin1`](from_latin1.md) /
  [`Note::from_windows1252`](from_windows1252.md): reinterpret the bytes in a
  legacy encoding rather than replacing them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_nfkd.html">
    <span class="label">Previous</span>
    <span class="title">to_nfkd</span>
  </a>
  <a class="next-link" href="to_utf8_strict.html">
    <span class="label">Next</span>
    <span class="title">to_utf8_strict</span>
  </a>
</div>
