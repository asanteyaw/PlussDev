<!-- pluss-hand-authored -->
# Note::phonetic_soundex

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::phonetic_soundex() const
```

## Reduce Each Word To Its Soundex Code

`phonetic_soundex()` replaces each cell with the Soundex codes of the words it
contains, so that names that *sound* alike collapse to the same key.
`Robert` and `Rupert` both become `R163`. Use it for fuzzy name matching,
deduplicating a contact list, or building a "sounds-like" index.

### The Soundex code

Each word is reduced to a **letter followed by three digits**:

1. Keep the first letter, upper-cased.
2. Map the rest to digits — `B F P V` → `1`, `C G J K Q S X Z` → `2`,
   `D T` → `3`, `L` → `4`, `M N` → `5`, `R` → `6`; vowels and `Y` produce no
   digit.
3. Drop a digit that repeats the one before it (including a first digit that
   repeats the first letter's own code).
4. `H` and `W` are **transparent**: two same-coded consonants separated only
   by `H`/`W` still collapse to one digit (`Ashcroft` → `A261`). A vowel or
   `Y` between them does *not* collapse them.
5. Pad with `0` or truncate so the code is exactly four characters.

This is the NARA / U.S. Census rule set.

### Per cell

The cell is tokenized into words (letters and digits; punctuation is a
separator), each word is coded, and the codes are joined with a single space.
`"John Smith"` → `"J500 S530"`. A cell with no word yields an empty string; a
word that is a single letter codes as that letter plus `000`.

The operation is shape-preserving and cell-independent, works on UTF-8 text
(a bytes-mode `Note` is rejected with `Note::phonetic_soundex requires a
UTF-8 text Note (not bytes)`), and returns a fresh UTF-8 `Note`. Non-ASCII
letters are not decoded — run [`transliterate_ascii`](transliterate_ascii.md)
first if the input has accents.

## Example 1: Similar Names Share A Code

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"Robert", "Rupert", "Rubin", "Ashcraft", "Tymczak"})
        .phonetic_soundex())
    << "\n";
```

### Output

```text
Note(shape=[5], dim=1, numel=5, encoding=utf8)
["R163", "R163", "R150", "A261", "T522"]
```

`Robert` and `Rupert` collapse to `R163`; `Rubin` differs at the third digit.

## Example 2: `H` And `W` Are Transparent

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"Ashcroft", "Pfister"}).phonetic_soundex())
    << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["A261", "P236"]
```

In `Ashcroft` the `H` between `s` and `c` (both coded `2`) is skipped without
breaking the run, so the two `2`s become one. In `Pfister` the leading `f`
repeats `P`'s own code (`1`) and is dropped.

## Example 3: Multi-Word Cells

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"John Smith", "the quick brown fox"}).phonetic_soundex())
    << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["J500 S530", "T000 Q200 B650 F200"]
```

## Example 4: Short Words And Empty Cells

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"Lee", "A", ""}).phonetic_soundex())
    << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["L000", "A000", ""]
```

## Example 5: Punctuation Is A Separator

```cpp
std::cout << pluss::make_note({"O'Brien, Jr."}).phonetic_soundex() << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["O165 J600"]
```

`O'Brien` splits at the apostrophe into `O` and `Brien`, coded together as
`O165`; `Jr` becomes `J600`.

## See Also

- [`Note::phonetic_metaphone`](phonetic_metaphone.md): a consonant-skeleton
  key that keeps more of the word's shape than a four-character Soundex code.
- [`Note::transliterate_ascii`](transliterate_ascii.md): fold accents to
  ASCII before coding.
- [`Note::tokenize_words`](tokenize_words.md): the word split used per cell.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="phonetic_metaphone.html">
    <span class="label">Previous</span>
    <span class="title">phonetic_metaphone</span>
  </a>
  <a class="next-link" href="quote.html">
    <span class="label">Next</span>
    <span class="title">quote</span>
  </a>
</div>
