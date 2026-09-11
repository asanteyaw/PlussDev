<!-- pluss-hand-authored -->
# Note::lower

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::lower() const
```

## Case-Fold Every Cell To Lowercase

`lower()` returns a new `Note`, same shape as the input, with every text cell
lowercased. It is the standard case-normalization step before a
case-insensitive comparison, de-duplication, grouping key, or substring test
([`contains`](contains.md) is byte-exact, so folding case first is how you
make it case-insensitive).

The mapping is **full Unicode lowercase**, not ASCII-only. Latin letters with
diacritics (`É → é`), Greek (`Α → α`), and Cyrillic (`Д → д`) all fold, and
locale-independent special cases apply: German `ẞ` (capital sharp s) becomes
`ß`, and Turkish dotted capital `İ` becomes `i` followed by a combining dot
(`i̇`), the Unicode default mapping. Because a few of these mappings change
length, the output cell can have a different byte length than the input — do
not assume `lower()` is length-preserving even though it is shape-preserving.

`lower()` changes **case only**. It does not trim whitespace, strip
punctuation, collapse spaces, or normalize Unicode form. That narrow contract
is useful: if the output still has padding or odd spacing, `lower()` is not
the stage that introduced or failed to remove it — pair it with
[`strip`](strip.md) when you want both. The operation is idempotent
(`lower().lower()` equals `lower()`), allocates a fresh `Note`, and does not
touch the input.

## Example 1: A One-Dimensional Batch

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"  Ada LOVELACE  ", "\tGRACE Hopper\t", "Linus"}).lower())
    << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["  ada lovelace  ", "\tgrace hopper\t", "linus"]
```

Case changes; the leading spaces and the tab characters are untouched.

## Example 2: A Scalar

```cpp
const pluss::Note n = pluss::make_note("HeLLo, WORLD").lower();
std::cout << n << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"hello, world"
```

## Example 3: Shape Is Preserved

```cpp
pluss::note::print(std::cout,
                   pluss::make_note({{"AB", "cD"}, {"Ef", "GH"}}).lower())
    << "\n";
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["ab", "cd"], 
  ["ef", "gh"]
]
```

## Example 4: Full Unicode, Including Special Cases

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"CAFÉ", "ΑΒΓ ДЕЁ", "İ", "ẞ"}).lower())
    << "\n";
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["café", "αβγ деё", "i̇", "ß"]
```

`É`, `Α`–`Γ`, and `Д`–`Ё` fold normally; `İ` maps to `i` plus a combining
dot; `ẞ` maps to `ß`.

## Example 5: Idempotent

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"already lower"}).lower().lower())
    << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["already lower"]
```

## Example 6: Paired With `strip` For Full Normalization

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"  Ada LOVELACE  ", "\tGRACE Hopper\t"}).strip().lower())
    << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["ada lovelace", "grace hopper"]
```

## See Also

- `pluss::coden::Note::upper`: the uppercase counterpart.
- `pluss::coden::Note::casefold`: aggressive case folding for
  caseless matching (handles more edge cases than `lower` for comparison
  keys).
- [`Note::strip`](strip.md): remove boundary whitespace; commonly chained
  with `lower`.
- [`Note::contains`](contains.md): the byte-exact predicate that `lower`
  makes case-insensitive.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lengths_graphemes.html">
    <span class="label">Previous</span>
    <span class="title">lengths_graphemes</span>
  </a>
  <a class="next-link" href="lstrip.html">
    <span class="label">Next</span>
    <span class="title">lstrip</span>
  </a>
</div>
