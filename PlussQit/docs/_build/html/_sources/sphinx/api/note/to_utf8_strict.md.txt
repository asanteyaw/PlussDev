<!-- pluss-hand-authored -->
# Note::to_utf8_strict

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::to_utf8_strict() const
```

## Promote Bytes To Text, Rejecting Anything Not Valid UTF-8

`to_utf8_strict()` re-tags a [bytes-mode](is_bytes.md) `Note` as **UTF-8
text** without changing a single byte — but only after checking that *every*
cell already contains well-formed UTF-8. If any cell does not, the call
raises `pluss::Error` and no result is produced. Use it at a trust boundary,
when you have a bytes payload that is *supposed* to be UTF-8 and you want a
hard failure rather than silent corruption if it is not.

### What "valid" means

Each cell is scanned as a sequence of UTF-8 code units: a lead byte in the
`0x00–0x7F`, `0xC0–0xDF`, `0xE0–0xEF`, or `0xF0–0xF7` range followed by the
right number of `0x80–0xBF` continuation bytes. A stray continuation byte, a
truncated multi-byte sequence, or a `0xFF` produces the rejection. Validation
runs over the whole `Note` **before** any result is built, so the operation
is all-or-nothing.

Called on a `Note` that is already text, it validates and returns an
equivalent text `Note`. The result is shape-preserving and shares the input's
byte content exactly.

For a version that never fails — substituting the replacement character for
each bad sequence — use [`to_utf8_lossy`](to_utf8_lossy.md). To *interpret*
non-UTF-8 bytes in a known legacy encoding instead of rejecting them, use
[`from_latin1`](from_latin1.md) or [`from_windows1252`](from_windows1252.md).

## Example 1: Clean Bytes Become Text

```cpp
std::cout << pluss::note::print(
    pluss::note::bytes({"café", "abc"}).to_utf8_strict())
    << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["café", "abc"]
```

The bytes are unchanged; only the encoding tag flips from `bytes` to `utf8`.

## Example 2: Invalid Bytes Are Rejected

```cpp
try {
  (void)pluss::note::bytes({std::string("bad\xFF\xFE") + "bytes"}).to_utf8_strict();
} catch (const pluss::Error& e) {
  const std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Note::to_utf8_strict found invalid UTF-8 payload
```

## See Also

- [`Note::to_utf8_lossy`](to_utf8_lossy.md): the non-failing counterpart.
- [`Note::from_latin1`](from_latin1.md) /
  [`Note::from_windows1252`](from_windows1252.md): decode legacy single-byte
  encodings into text.
- [`Note::is_utf8_valid`](is_utf8_valid.md): test validity per cell instead
  of raising.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_utf8_lossy.html">
    <span class="label">Previous</span>
    <span class="title">to_utf8_lossy</span>
  </a>
  <a class="next-link" href="to_windows1252_lossy.html">
    <span class="label">Next</span>
    <span class="title">to_windows1252_lossy</span>
  </a>
</div>
