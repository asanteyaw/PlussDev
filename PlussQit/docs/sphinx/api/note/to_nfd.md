<!-- pluss-hand-authored -->
# Note::to_nfd

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::to_nfd() const
```

## Decompose Every Cell To Unicode Normalization Form D

`to_nfd()` returns a new `Note`, same shape as the input, with each cell
converted to **NFD** — the canonical decomposed form. Every precomposed
accented character is split into its base letter followed by its combining
marks: the single code point `é` (U+00E9) becomes `e` + U+0301 (combining
acute).

NFD is the useful form when you want to *operate on the marks* — for
example [`strip_accents`](strip_accents.md) only removes standalone
combining marks and the common Latin-1 precomposed letters, so
`to_nfd().strip_accents()` folds a much wider range of accented text to
ASCII. It does not change how the text renders, only its bytes; pair it with
[`lengths_chars`](lengths_chars.md) to see the change.

`to_nfd` is shape-preserving,
cell-independent, idempotent, and returns a fresh `Note`. Requires the
Unicode backend.

## Example 1: A Precomposed Accent Is Split

```cpp
const auto n = pluss::make_note({"café", "abc"}).to_nfd();
std::cout << pluss::note::print(n) << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["café", "abc"]
```

The render is unchanged.

## Example 2: The Character Count Shows The Change

```cpp
const auto out = pluss::make_note({"café", "abc"}).to_nfd();
const auto bytes = out.lengths_bytes();
const auto chars = out.lengths_chars();
std::cout << "bytes=[" << bytes[0] << ", " << bytes[1] << "] chars=["
          << chars[0] << ", " << chars[1] << "]\n";
```

### Output

```text
bytes=[6, 3] chars=[5, 3]
```

`"café"` is now 5 code points (`c a f e ◌́`) in 6 bytes; the ASCII cell is
untouched.

## See Also

- [`Note::to_nfc`](to_nfc.md): the opposite direction — canonical
  composition.
- [`Note::strip_accents`](strip_accents.md): chain after `to_nfd` to fold
  accents to ASCII.
- [`Note::to_nfkd`](to_nfkd.md): decompose **and** apply compatibility
  folds.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_nfc.html">
    <span class="label">Previous</span>
    <span class="title">to_nfc</span>
  </a>
  <a class="next-link" href="to_nfkc.html">
    <span class="label">Next</span>
    <span class="title">to_nfkc</span>
  </a>
</div>
