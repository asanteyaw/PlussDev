<!-- pluss-hand-authored -->
# Note::strip_accents

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::strip_accents() const
```

## Fold Accented Letters To Their Base ASCII Form

`strip_accents()` returns a new `Note`, same shape as the input, with
accent marks removed from each cell: `café` becomes `cafe`, `señor` becomes
`senor`, `naïve` becomes `naive`. Use it to build a loose, ASCII-only key
for search or matching where accented and unaccented spellings should count
as the same.

### What it covers

Two things are folded:

- **Standalone combining diacritics** anywhere in the text (U+0300–U+036F
  and similar) are dropped, so any text already in decomposed form
  ([`to_nfd`](to_nfd.md)) is handled fully.
- **Precomposed Latin-1 accented letters** (`À`–`ÿ`: the common
  European set — `é è ê ë`, `ñ`, `ç`, `ü`, …) are mapped to their base
  letter.

Precomposed letters outside that Latin-1 set — Latin Extended-A and beyond
(`ā ē ĥ ă ķ ő ...`) — are **not** in the table and pass through unchanged.
To fold those too, decompose first: `to_nfd().strip_accents()`.

`strip_accents` is shape-preserving, cell-independent, and returns a fresh
`Note`.

## Example 1: Common Accents Are Folded

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"café résumé", "naïve", "señor"}).strip_accents())
    << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["cafe resume", "naive", "senor"]
```

## Example 2: The Latin-Extended Limit

```cpp
const pluss::Note n = pluss::make_note({"ĥăçķ ō"}).strip_accents();
std::cout << n << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["ĥăcķ ō"]
```

Only `ç` (Latin-1) is folded; `ĥ`, `ă`, `ķ`, `ō` (Latin Extended-A) are not.

## Example 3: Decompose First To Fold Everything

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"ĥăçķ ō"}).to_nfd().strip_accents())
    << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["hack o"]
```

After [`to_nfd`](to_nfd.md) the accents are standalone combining marks, which
`strip_accents` removes.

## See Also

- [`Note::to_nfd`](to_nfd.md): decompose accents so `strip_accents` can
  reach all of them.
- `pluss::coden::Note::transliterate_ascii`: a broader ASCII fold that also
  handles non-Latin scripts.
- [`Note::to_ascii`](to_ascii.md): replace what is left with `?` after
  folding.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="strip.html">
    <span class="label">Previous</span>
    <span class="title">strip</span>
  </a>
  <a class="next-link" href="strip_bom.html">
    <span class="label">Next</span>
    <span class="title">strip_bom</span>
  </a>
</div>
