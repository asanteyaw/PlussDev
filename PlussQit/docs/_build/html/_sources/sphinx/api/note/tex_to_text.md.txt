<!-- pluss-hand-authored -->
# Note::tex_to_text

[Back to Note API Reference](../../note_api_reference)

## Signatures

```cpp
Note Note::tex_to_text() const
Note Note::tex_to_text(std::int64_t mode) const
```

## Reduce Authored TeX To Readable Plain Text

`tex_to_text()` returns a new `Note`, same shape as the input, with each cell
stripped down from TeX/LaTeX markup to the prose it carries. Use it when the
input is already `.tex`-style content and the downstream step wants plain
text — full-text indexing, search snippets, embeddings, summarization, a
plain-text export column.

It is the opposite direction from [`tex_escape`](tex_escape.md): `tex_escape`
protects text so TeX will typeset it verbatim; `tex_to_text` throws the TeX
scaffolding away and keeps the words.

### What is removed and what is kept

In order, each cell is passed through:

1. **Comments** — everything from an unescaped `%` to end of line.
2. **Preamble** — content before `\begin{document}` and after
   `\end{document}` (when those markers are present).
3. **Math** — inline (`$…$`), display (`\[…\]`, `$$…$$`), and
   `\begin{equation}`-style environments are dropped entirely, contents and
   all.
4. **Text-wrapping commands** — `\text{…}`, `\textbf{…}`, `\textit{…}`,
   `\emph{…}`, `\underline{…}`, `\texttt{…}`, `\mathrm{…}`, `\mathbf{…}`,
   `\mathit{…}`, `\mathsf{…}`, `\operatorname{…}` are replaced by their
   braced content.
5. **Every other `\command`** — removed. A symbol macro such as `\alpha`
   leaves nothing behind; it is not transliterated to `α` or `alpha`.
6. Remaining `{` and `}` become spaces; runs of whitespace collapse to one;
   the cell is trimmed.

Because math and symbol macros are dropped, a cell that is *only* math
becomes empty, and a round trip through `tex_escape` is lossy (the escapes it
adds read as commands here).

### `mode`

`mode = 0` (the default) applies the pipeline above. `mode >= 1` additionally
case-folds the recovered text (ASCII lowercase), matching the "sanitize"
convention used elsewhere in the library — handy when the output feeds a
case-insensitive index or key. Negative values behave like `0`.

The operation is shape-preserving, cell-independent, and returns a fresh
`Note`.

## Example 1: Commands And Math

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"Text \\textbf{bold} and $x + y$.", "$only math$"}).tex_to_text())
    << "\n";
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["Text bold and .", ""]
```

`\textbf{bold}` becomes `bold`; `$x + y$` is dropped (the trailing `.` is
prose and stays, and the gap collapses). A math-only cell reduces to empty.

## Example 2: Comments And Preamble

```cpp
const pluss::Note n = pluss::make_note(
    {"\\documentclass{article}\n% note\n\\begin{document}\nBody \\emph{here}\n\\end{document}"})
    .tex_to_text();
std::cout << n << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["Body here"]
```

The class line and the `% note` comment are gone; only the document body
survives, with `\emph{here}` unwrapped.

## Example 3: `mode` >= 1 Also Case-Folds

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"\\textbf{Hello} World"}).tex_to_text(1))
    << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["hello world"]
```

## Example 4: `mode` 0 Is The Default

```cpp
std::cout << pluss::note::print(
    pluss::make_note({"\\textbf{Hello} World"}).tex_to_text(0))
    << "\n";
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["Hello World"]
```

## Example 5: A Scalar — Symbol Macros Vanish

```cpp
pluss::note::print(std::cout,
                   pluss::make_note("\\alpha kept? \\textit{no}").tex_to_text())
    << "\n";
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"kept? no"
```

`\alpha` leaves nothing; `\textit{no}` unwraps to `no`.

## See Also

- [`Note::tex_escape`](tex_escape.md): the opposite direction — protect text
  for TeX.
- `pluss::coden::Note::tex_strip_comments` / `tex_strip_preamble`: run just
  one stage of the pipeline.
- [`Note::nlp_mask_pii`](nlp_mask_pii.md): a different "clean this text"
  pass, keyed to identifiers rather than markup.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tex_escape.html">
    <span class="label">Previous</span>
    <span class="title">tex_escape</span>
  </a>
  <a class="next-link" href="title.html">
    <span class="label">Next</span>
    <span class="title">title</span>
  </a>
</div>
