<!-- pluss-hand-authored -->
# NoteNlpAccessor::metaphone

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note NoteNlpAccessor::metaphone() const
```

## Reduce Each Word To Its Consonant Skeleton

`metaphone()` replaces each cell with a compact pronunciation key for
every word it contains: the first letter, then the remaining consonants with
vowels and near-silent letters removed. `programming` → `PRGRMNG`. It keeps
more of a word's shape than the four-character [`soundex`](soundex.md)
code, so it is a better fuzzy-match key for longer words.

### The key

Each word is upper-cased and reduced as follows:

1. Keep the first letter.
2. From the rest, drop every vowel (`A E I O U`), every `H`, `W`, and `Y`,
   and every non-letter.
3. Drop a consonant that immediately repeats the previous kept letter.
4. Truncate to at most eight characters.

This is a lightweight **consonant-skeleton** key. It is *not* the full
Metaphone algorithm: it does not apply letter-context rules such as `PH` to
`F`, `CK` to `K`, or silent `GN`, so `Pfister` keeps both `P` and `F` as
`PFSTR`. A separate double-Metaphone operation is not exposed because the
implementation does not produce the alternate key that would make that
contract meaningfully different.

### Per cell

The cell is tokenized into words (letters and digits; punctuation is a
separator), each word is reduced, and the keys are joined with a single
space. A cell with no word yields an empty string.

The operation is shape-preserving and cell-independent, works on UTF-8 text
(a bytes-mode `Note` is rejected with `NoteNlpAccessor::metaphone requires a
UTF-8 text Note (not bytes)`), and returns a fresh UTF-8 `Note`. Non-ASCII
letters are not decoded — run [`to_ascii`](to_ascii.md)
first if the input has accents.

## Example 1: Names

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"Thompson", "knight", "programming", "Pfister"})
        .nlp().metaphone())
    << "\n";
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["TMPSN", "KNGT", "PRGRMNG", "PFSTR"]
```

`Thompson` keeps `T` then `MPSN` (the `H` and both `O`s drop); `knight` keeps
`K` then `NGT` (`I` drops, and this key does not silence the `K` or `GH`).

## Example 2: Multi-Word Cells

```cpp
std::cout << pluss::make_note({"quick brown fox"}).nlp().metaphone() << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["QCK BRN FX"]
```

## See Also

- [`NoteNlpAccessor::soundex`](soundex.md): a fixed four-character
  "sounds-like" code.
- [`NoteNlpAccessor::to_ascii`](to_ascii.md): fold accents to
  ASCII before coding.
- [`NoteNlpAccessor::tokens`](tokens.md): the word split used per cell.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mentions.html"><span class="label">Previous</span><span class="title">mentions</span></a>
  <a class="next-link" href="min_length.html"><span class="label">Next</span><span class="title">min_length</span></a>
</div>
