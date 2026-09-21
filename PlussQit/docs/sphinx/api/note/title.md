<!-- pluss-hand-authored -->
# Note::title

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::title() const
```

## Title-Case Every Cell

`title()` returns a new `Note`, same shape as the input, with each cell
title-cased: the first letter of every word is put in its title form and the
rest of the word is lowercased. Use it for display labels, headings, and
proper-noun-ish columns that should read as "Words Look Like This".

The transform is **Unicode-aware** — it uses the Unicode default title-case
algorithm, so accented Latin, Greek, and Cyrillic words are handled, and a
letter with no distinct title form (most of them) simply uppercases. Words
are separated by whitespace and by most punctuation; a couple of boundary
details are worth knowing and are shown below:

- A digit followed by a letter starts a fresh word, so `"3rd"` becomes
  `"3Rd"`.
- `_` does **not** start a new word — `"snake_case_name"` title-cases only
  its very first letter.

`title()` changes case only — no trimming, no punctuation removal — and is
idempotent. It is shape-preserving and returns a fresh `Note`. Because some
title/lowercase mappings change length, the operation is not
byte-length-preserving.

## Example 1: A Sentence

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"the quick brown fox", "ALL CAPS HERE", "café au lait"}).title())
    << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["The Quick Brown Fox", "All Caps Here", "Café Au Lait"]
```

`ALL CAPS HERE` is lowercased and re-title-cased; `café` keeps its `é` and
gets a capital `C`.

## Example 2: Digit And Underscore Boundaries

```cpp
const pluss::Note n =
    pluss::make_note({"3rd place", "snake_case_name"}).title();
std::cout << n << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["3Rd Place", "Snake_case_name"]
```

The letter after the digit in `3rd` is title-cased; `_` is not a word
boundary, so only `snake_case_name`'s first letter changes.

## Example 3: A Scalar

```cpp
pluss::note::print(std::cout, pluss::make_note("hello world").title()) << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"Hello World"
```

## See Also

- [`Note::capitalize`](capitalize.md): uppercase only the first character of
  the whole cell (ASCII).
- [`Note::upper`](upper.md) / [`Note::lower`](lower.md): full-cell case
  folding.
- [`Note::swapcase`](swapcase.md): invert the case of every ASCII letter.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tex.html">
    <span class="label">Previous</span>
    <span class="title">tex</span>
  </a>
  <a class="next-link" href="to_ascii.html">
    <span class="label">Next</span>
    <span class="title">to_ascii</span>
  </a>
</div>
