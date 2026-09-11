<!-- pluss-hand-authored -->
# Note::transliterate_ascii

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::transliterate_ascii() const
```

## Force Text Down To Plain ASCII

`transliterate_ascii()` rewrites each cell so it contains only 7-bit ASCII: it
folds the accented Latin letters to their base form and replaces anything it
cannot represent with a literal `?`. Use it when a downstream consumer — a
legacy identifier scheme, a filename rule, a fixed-width field — accepts only
ASCII and an approximate rendering is better than a hard failure.

### How it folds

1. First the cell runs through the same accent fold as
   [`strip_accents`](strip_accents.md): a precomposed Latin-1 letter
   (`é`, `ñ`, `Ø`, `Æ`) becomes its base (`e`, `n`, `O`, `AE`), and a
   standalone combining diacritic is dropped.
2. Then **every remaining byte with the high bit set** (`≥ 0x80`) is replaced
   by one `?` per byte.

Step 1 only knows the Latin-1 block, so a letter from Latin Extended-A
(`ā`, `ĥ`, `ǔ`) survives step 1 unchanged and is turned into `??` (two bytes,
two question marks) by step 2. To widen the fold, decompose first:
[`to_nfd`](to_nfd.md)`().transliterate_ascii()` splits `ā` into `a` + a
combining macron, and step 1 drops the macron — so the result is `a`. That
still cannot rescue a non-Latin script; use
[`transliterate_greek`](transliterate_greek.md) /
[`transliterate_cyrillic`](transliterate_cyrillic.md) for those.

The operation is shape-preserving and cell-independent, works on UTF-8 text
(a bytes-mode `Note` is rejected with `Note::transliterate_ascii requires a
UTF-8 text Note (not bytes)`), and returns a fresh UTF-8 `Note`. A cell that
is already ASCII is returned unchanged.

`transliterate_latin()` is an exact synonym for this method — the Latin
transliteration *is* the ASCII fold — so this page covers both.

## Example 1: Latin-1 Accents Fold To Their Base

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"café", "señor", "Ærø", "naïve"}).transliterate_ascii())
    << "\n";
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["cafe", "senor", "AEro", "naive"]
```

## Example 2: Beyond Latin-1 Becomes `?`

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"ĥ", "Œuvre", "π ≈ 3"}).transliterate_ascii())
    << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["??", "OEuvre", "?? ??? 3"]
```

`Œ` is in the Latin-1 range so it folds to `OE`; `ĥ` (Latin Extended-A), the
Greek `π`, and the `≈` sign are each two or three high bytes and become that
many `?`.

## Example 3: Decompose First To Widen The Fold

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"ā", "ǔ", "Ω"}).to_nfd().transliterate_ascii())
    << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["a", "u", "??"]
```

`ā` and `ǔ` decompose to a base letter plus a combining mark, which the accent
fold then removes. `Ω` is a Greek letter, not an accented Latin one — NFD does
not change it, so it still becomes `??`.

## Example 4: Already-ASCII And Higher-Rank Input

```cpp
std::cout << pluss::make_note({"plain text 123"}).transliterate_ascii() << "\n";
std::cout << pluss::note::print(
    pluss::make_note({{"àb", "c"}}).transliterate_ascii())
    << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["plain text 123"]
Note(shape=[1, 2], dim=2, numel=2, encoding=utf8)
[
  ["ab", "c"]
]
```

## See Also

- [`Note::strip_accents`](strip_accents.md): fold accents but keep the text
  as UTF-8 (no `?` substitution).
- [`Note::to_ascii`](to_ascii.md): the plain cleanup that replaces non-ASCII
  with `?` **without** the accent fold.
- [`Note::transliterate_greek`](transliterate_greek.md) /
  [`Note::transliterate_cyrillic`](transliterate_cyrillic.md): romanize a
  non-Latin script.
- [`Note::to_latin1_lossy`](to_latin1_lossy.md): keep the Latin-1 characters
  as bytes instead of folding to ASCII.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tokenize_words.html">
    <span class="label">Previous</span>
    <span class="title">tokenize_words</span>
  </a>
  <a class="next-link" href="transliterate_cyrillic.html">
    <span class="label">Next</span>
    <span class="title">transliterate_cyrillic</span>
  </a>
</div>
