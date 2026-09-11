<!-- pluss-hand-authored -->
# Note::casefold

[Back to Note API Reference](../../note_api_reference)

## Signatures

```cpp
Note Note::casefold() const
Note Note::casefold(std::string_view locale) const
```

## Fold Case For Caseless Comparison

`casefold()` returns a new `Note`, same shape as the input, with each cell
reduced to its Unicode case-folded form. Case folding is stronger than
[`lower`](lower.md): it is designed so that two strings that differ only in
case fold to the *same* bytes, which makes it the right key for caseless
grouping, de-duplication, and membership checks.

The practical difference from `lower()` shows up on characters whose casing
is not a simple 1:1 lowercase. The most common example is the German sharp s:
`lower("straße")` stays `"straße"`, but `casefold("straße")` also folds `ß`
so that it matches a `"STRASSE"` that was folded the same way. Use `casefold`
when you are building a comparison key; use `lower` when you want readable
lowercase output.

### `locale`

`casefold(locale)` applies language-specific folding rules. Pass a language
tag such as `"de"`, `"tr"`, or `"lt"`; an empty string (the no-argument
form) uses the language-independent default. The main effect is on
characters that fold differently by language — for example, with `"de"` the
capital sharp s `ẞ` folds to `ss`.

`casefold()` changes case only, is idempotent, shape-preserving, and returns
a fresh `Note`. Some foldings change length, so it is not
byte-length-preserving. Requires the Unicode backend.

## Example 1: A Batch

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"HeLLo", "ΑΒΓ", "STRASSE"}).casefold())
    << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["hello", "αβγ", "strasse"]
```

## Example 2: `ß` Folds Where `lower` Leaves It

```cpp
const pluss::Note n = pluss::make_note({"straße"}).casefold();
std::cout << n << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["strasse"]
```

`casefold` turns `ß` into `ss`, so this now matches a case-folded
`"STRASSE"`. `lower("straße")` would keep the `ß`.

## Example 3: Language-Specific Folding

```cpp
pluss::note::print(std::cout,
                   pluss::make_note({"STRASSE ẞ"}).casefold("de")) << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["strasse ss"]
```

With the `"de"` locale the capital sharp s `ẞ` folds to `ss`.

## Example 4: A Scalar

```cpp
std::cout << pluss::note::print(pluss::make_note("MiXeD").casefold()) << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"mixed"
```

## See Also

- [`Note::lower`](lower.md): readable lowercase output (1:1 mappings only).
- [`Note::contains`](contains.md): the byte-exact predicate that a shared
  `casefold` makes caseless.
- [`Note::to_nfc`](to_nfc.md): Unicode normalization, the other half of a
  robust comparison key.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="capitalize.html">
    <span class="label">Previous</span>
    <span class="title">capitalize</span>
  </a>
  <a class="next-link" href="cell.html">
    <span class="label">Next</span>
    <span class="title">cell</span>
  </a>
</div>
