<!-- pluss-hand-authored -->
# Note::to_nfkd

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::to_nfkd() const
```

## Compatibility-Decompose Every Cell To Unicode Normalization Form KD

`to_nfkd()` returns a new `Note`, same shape as the input, with each cell
converted to **NFKD** — full compatibility decomposition with no
recomposition. It applies the same compatibility folds as
[`to_nfkc`](to_nfkc.md) (`ﬁ` → `fi`, `²` → `2`, full-width → plain) but
leaves accented letters *decomposed*, as base letter plus combining marks.

Reach for NFKD when you want both the compatibility folding **and** the
marks exposed for a follow-up pass such as
[`strip_accents`](strip_accents.md). Like NFKC it is **lossy** — do not use
it where the compatibility distinctions carry meaning.

This is the same operation as `normalize(3)`. `to_nfkd` is shape-preserving,
cell-independent, idempotent, and returns a fresh `Note`. Requires the
Unicode backend.

## Example 1: Compatibility Folds Applied, Accents Left Decomposed

```cpp
const std::string in = std::string("\xEF\xAC\x81") + "le " + "\xC2\xB2";  // ﬁ + "le " + ²
std::cout << pluss::note::print(
    pluss::make_note({in, "plain"}).to_nfkd())
    << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["file 2", "plain"]
```

`"ﬁle ²"` becomes `"file 2"` (this input has no accents to leave
decomposed).

## See Also

- [`Note::to_nfkc`](to_nfkc.md): the composed compatibility form.
- [`Note::to_nfd`](to_nfd.md): canonical decomposition without the
  compatibility folds.
- [`Note::strip_accents`](strip_accents.md): chain after `to_nfkd` to fold
  accents to ASCII too.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_nfkc.html">
    <span class="label">Previous</span>
    <span class="title">to_nfkc</span>
  </a>
  <a class="next-link" href="to_utf8_lossy.html">
    <span class="label">Next</span>
    <span class="title">to_utf8_lossy</span>
  </a>
</div>
