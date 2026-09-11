<!-- pluss-hand-authored -->
# Note::surround

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::surround(std::string_view left, std::string_view right) const
```

## Wrap Every Cell In A Chosen Prefix And Suffix

`surround(left, right)` returns a new `Note`, same shape as the input, with
each cell rewritten as `left + cell + right`. It is the general form of
[`quote`](quote.md) (`surround("\"", "\"")`): use it for Markdown emphasis
(`**...**`), bracketed spans (`[...]`), XML-ish tags, or any fixed
open/close pair.

The wrap is **unconditional and literal** — `left` and `right` are always
added, verbatim, whether or not the cell already begins or ends with them (so
`surround` is not idempotent), and nothing inside the cell is escaped. When
you want "add it only if missing", use
[`ensure_prefix`](ensure_prefix.md) / [`ensure_suffix`](ensure_suffix.md)
instead.

`left` and `right` may differ or be equal, and either may be empty (an empty
`right` makes `surround` a pure prefix add). An empty cell becomes
`left + right`. The operation is shape-preserving, cell-independent, and
returns a fresh `Note`.

## Example 1: Brackets

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"x", "", "a]"}).surround("[", "]"))
    << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["[x]", "[]", "[a]]"]
```

The empty cell becomes `"[]"`; `"a]"` becomes `"[a]]"` — the existing `]` is
not deduplicated.

## Example 2: The Same String On Both Sides

```cpp
const pluss::Note n = pluss::make_note({"mid"}).surround("**", "**");
std::cout << n << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["**mid**"]
```

## Example 3: Shape Is Preserved

```cpp
std::cout << pluss::note::print(
    pluss::make_note({{"a", "b"}}).surround("<", ">"))
    << "\n";
```

### Output

```text
Note(shape=[1, 2], dim=2, numel=2, encoding=utf8)
[
  ["<a>", "<b>"]
]
```

## See Also

- [`Note::quote`](quote.md): `surround("\"", "\"")` with its own name.
- [`Note::ensure_prefix`](ensure_prefix.md) /
  [`Note::ensure_suffix`](ensure_suffix.md): conditional, idempotent adds.
- [`Note::unquote`](unquote.md): remove one matching pair of surrounding
  quote characters.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="strip_zero_width.html">
    <span class="label">Previous</span>
    <span class="title">strip_zero_width</span>
  </a>
  <a class="next-link" href="swapcase.html">
    <span class="label">Next</span>
    <span class="title">swapcase</span>
  </a>
</div>
