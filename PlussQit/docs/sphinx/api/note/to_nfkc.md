<!-- pluss-hand-authored -->
# Note::to_nfkc

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::to_nfkc() const
```

## Compatibility-Fold Every Cell To Unicode Normalization Form KC

`to_nfkc()` returns a new `Note`, same shape as the input, with each cell
converted to **NFKC** — compatibility decomposition followed by canonical
composition. On top of what [`to_nfc`](to_nfc.md) does, NFKC also replaces
*compatibility* characters with their ordinary equivalents: the `ﬁ` ligature
becomes `fi`, the superscript `²` becomes `2`, full-width and stylised
letter variants become their plain forms, non-breaking spaces become normal
spaces.

Use it to build a search or matching key that should treat those variants as
the same text — but note it is **lossy**: `x²` and `x2` become identical, so
never NFKC-normalise content where the distinction matters (mathematical
notation, code). For a lossless normalisation use [`to_nfc`](to_nfc.md).

`to_nfkc` is shape-preserving,
cell-independent, idempotent, and returns a fresh `Note`. Requires the
Unicode backend.

## Example 1: Ligatures And Superscripts Are Folded

```cpp
const std::string in = std::string("\xEF\xAC\x81") + "le " + "\xC2\xB2";  // ﬁ + "le " + ²
std::cout << pluss::note::print(
    pluss::make_note({in, "plain"}).to_nfkc())
    << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["file 2", "plain"]
```

`"ﬁle ²"` becomes `"file 2"`; the ASCII cell is unchanged.

## See Also

- [`Note::to_nfc`](to_nfc.md): canonical composition only — lossless.
- [`Note::to_nfkd`](to_nfkd.md): the decomposed compatibility form.
- [`Note::casefold`](casefold.md): combine with a normal form for a robust
  caseless key.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_nfd.html">
    <span class="label">Previous</span>
    <span class="title">to_nfd</span>
  </a>
  <a class="next-link" href="to_nfkd.html">
    <span class="label">Next</span>
    <span class="title">to_nfkd</span>
  </a>
</div>
