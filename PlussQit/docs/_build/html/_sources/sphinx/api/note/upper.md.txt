<!-- pluss-hand-authored -->
# Note::upper

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::upper() const
```

## Case-Fold Every Cell To Uppercase

`upper()` returns a new `Note`, same shape as the input, with every text cell
uppercased. It is the mirror of [`lower`](lower.md): reach for it when the
downstream consumer expects capitals — a code or country field, a display
header, a case-insensitive key you happen to normalize upward.

Like `lower`, the mapping is **full Unicode uppercase**, not ASCII-only.
Accented Latin letters (`é → É`), Greek (`α → Α`), and Cyrillic all fold, and
locale-independent special cases apply: German `ß` uppercases to `SS`, so a
cell can grow in length. `upper()` is therefore shape-preserving but **not**
byte-length-preserving. It changes case only — no trimming, no punctuation
removal, no Unicode-form normalization — and is idempotent.

For a caseless *comparison* key, prefer [`casefold`](casefold.md), which is
designed for that and handles a few more edge cases; `upper()` is for output
that should read as capitals.

## Example 1: A One-Dimensional Batch

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"  Ada lovelace  ", "grace HOPPER", "café"}).upper())
    << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["  ADA LOVELACE  ", "GRACE HOPPER", "CAFÉ"]
```

Case changes; the leading and trailing spaces are untouched.

## Example 2: A Scalar

```cpp
const pluss::Note n = pluss::make_note("HeLLo, world").upper();
std::cout << n << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"HELLO, WORLD"
```

## Example 3: Shape Is Preserved

```cpp
pluss::note::print(std::cout,
                   pluss::make_note({{"ab", "cD"}, {"Ef", "gh"}}).upper())
    << "\n";
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["AB", "CD"], 
  ["EF", "GH"]
]
```

## Example 4: Full Unicode, Including `ß → SS`

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"café", "αβγ", "ß"}).upper())
    << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["CAFÉ", "ΑΒΓ", "SS"]
```

`ß` has no single uppercase form, so it expands to `SS` and that cell grows
by one byte.

## Example 5: Idempotent

```cpp
std::cout << pluss::note::print(pluss::make_note({"ALREADY"}).upper().upper())
          << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["ALREADY"]
```

## See Also

- [`Note::lower`](lower.md): the lowercase counterpart.
- [`Note::casefold`](casefold.md): aggressive folding for caseless matching
  keys.
- [`Note::title`](title.md) / [`Note::capitalize`](capitalize.md) /
  [`Note::swapcase`](swapcase.md): partial-case transforms (ASCII-only).

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="unsqueeze.html">
    <span class="label">Previous</span>
    <span class="title">unsqueeze</span>
  </a>
  <a class="next-link" href="url_decode.html">
    <span class="label">Next</span>
    <span class="title">url_decode</span>
  </a>
</div>
