<!-- pluss-hand-authored -->
# Note::collapse_whitespace

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::collapse_whitespace() const
```

## Trim And Fold Every Run Of Whitespace To One Space

`collapse_whitespace()` returns a new `Note`, same shape as the input, in
which each cell has its leading and trailing whitespace removed and every
interior run of one or more whitespace characters replaced by a **single
space**. It is the one-call "tidy the spacing" step for text pulled from
HTML, PDFs, wrapped source, or copy-paste.

Whitespace is the ASCII set space, `\t`, `\n`, `\r`, `\f`, `\v`. A tab
becomes a space; a run of blank lines becomes a single space, so multi-line
content is flattened to one line — use [`collapse_newlines`](collapse_newlines.md)
instead when line breaks must survive, or [`dedent`](dedent.md) to strip only
common indentation.

`collapse_whitespace` changes only spacing — no letters, punctuation, or
non-ASCII are touched. It is shape-preserving, cell-independent, and returns
a fresh `Note`.

## Example 1: A Batch

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"  hello   world  ", "a\t\tb\nc", "clean"}).collapse_whitespace())
    << "\n";
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["hello world", "a b c", "clean"]
```

The doubled internal spaces, the tabs, and the newline all collapse to one
space each; the ends are trimmed.

## Example 2: Shape Is Preserved

```cpp
const pluss::Note n = pluss::make_note({{"  a  b  ", " c "}}).collapse_whitespace();
std::cout << n << "\n";
```

### Output

```text
Note(shape=[1, 2], dim=2, numel=2, encoding=utf8)
[
  ["a b", "c"]
]
```

## See Also

- [`Note::collapse_spaces`](collapse_spaces.md): fold space/tab runs but
  leave newlines and the cell edges alone.
- [`Note::collapse_newlines`](collapse_newlines.md): fold only runs of line
  breaks.
- [`Note::strip`](strip.md): trim the edges without touching interior
  spacing.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="collapse_spaces.html">
    <span class="label">Previous</span>
    <span class="title">collapse_spaces</span>
  </a>
  <a class="next-link" href="contains.html">
    <span class="label">Next</span>
    <span class="title">contains</span>
  </a>
</div>
