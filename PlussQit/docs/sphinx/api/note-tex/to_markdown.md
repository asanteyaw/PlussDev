<!-- pluss-hand-authored -->
# NoteTexAccessor::to_markdown

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signatures

```cpp
Note NoteTexAccessor::to_markdown() const
Note NoteTexAccessor::to_markdown(std::int64_t mode) const
```

## Convert A Recognized TeX Subset To Markdown

`to_markdown()` rewrites a fixed set of common TeX commands to their
Markdown equivalents — headings, bold, italic/emphasis, and list items —
and removes color wrappers, hyperlink commands, labels, references,
citations, packages, bibliography commands, spacing/delimiter commands, and
font commands. Use it for a quick, best-effort Markdown preview of simple
TeX prose; it is not a full document converter.

The recognized rewrites, applied in this order:

| TeX | Markdown |
| --- | --- |
| `\section{...}` / `\section*{...}` | `\n# ...\n` |
| `\subsection{...}` / `\subsection*{...}` | `\n## ...\n` |
| `\subsubsection{...}` / `\subsubsection*{...}` | `\n### ...\n` |
| `\textbf{...}` | `**...**` |
| `\emph{...}` / `\textit{...}` | `*...*` |
| `\item` | `- ` |

`to_markdown()` and [`to_html`](to_html.md) both remove `\textcolor`/
`\colorbox` wrappers (keeping their content) and unwrap `\href`/hyperref
commands (keeping the visible label) — the two converters agree on that
point. They still differ on one rewrite: `to_html` maps a TeX linebreak
(`\\`) to `<br/>`, but `to_markdown` has no rewrite for it and leaves it as
a literal `\\` in the output.

### `mode`

`mode = 0` (the default) applies the rewrite table and removals above with
no further change. `mode >= 1` additionally case-folds the resulting
Markdown (ASCII lowercase) — the same contract as
[`to_text`](to_text.md)'s own `mode` — folding after every rewrite has
already run, so Markdown punctuation such as `#` and `*` is unaffected (it
has no case) while headings and prose text fold. Negative values behave
like `0`. `to_html` has no `mode` parameter at all.

The operation is shape-preserving, cell-independent, and returns a fresh
`Note`. Undefined input remains undefined; Bool, Int64, Float64, and Bytes
receivers are rejected because TeX source is Text.

## Example 1: Headings, Inline Formatting, Item, Color, And A Link

```cpp
std::cout << pluss::make_note({
    R"(\section{Intro}\subsection{Detail}\subsubsection{Fine})",
    R"(\textbf{Bold} and \emph{emphasis} \item first)",
    R"(\textcolor{red}{warn} \href{https://example.test}{link} \label{x} \ref{x}.)"})
    .tex().to_markdown()
    << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["# Intro ## Detail ### Fine", "**Bold** and *emphasis* - first", "warn link ."]
```

The third cell shows both removals: `\textcolor{red}{warn}` becomes bare
`warn`, and `\href{...}{link}` becomes bare `link` — both unwrapped rather
than passed through — while `\label{x}` and `\ref{x}` are removed
entirely. `to_html` treats this exact cell identically.

## Example 2: `mode` >= 1 Case-Folds

```cpp
const auto source = pluss::make_note({
    R"(\section{Intro}\subsection{Detail}\subsubsection{Fine})",
    R"(\textbf{Bold} and \emph{emphasis} \item first)",
    R"(\textcolor{red}{warn} \href{https://example.test}{link} \label{x} \ref{x}.)"});
std::cout << source.tex().to_markdown(1) << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["# intro ## detail ### fine", "**bold** and *emphasis* - first", "warn link ."]
```

Every letter folds to lowercase; the Markdown punctuation `#`, `**`, and
`-` is unchanged since punctuation has no case.

## Example 3: A 2-D Grid, Shape Preserved

```cpp
const auto grid = pluss::make_note(
    std::vector<std::string>{R"(\section{A})", R"(\textbf{B})", "plain", ""},
    std::array<std::int64_t, 2>{2, 2});
std::cout << grid.tex().to_markdown() << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["# A", "**B**"], 
  ["plain", ""]
]
```

## Example 4: Undefined And Non-Text Input

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().to_markdown().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2}).tex().to_markdown();
} catch (const pluss::Error&) {
  std::cout << "to markdown Int64: rejected\n";
}
```

### Output

```text
false
to markdown Int64: rejected
```

## See Also

- [`to_html`](to_html.md): the equivalent conversion to HTML, which shares
  the same color/hyperlink removal but has no `mode` parameter and maps a
  TeX linebreak to `<br/>` instead of leaving it literal.
- [`to_text`](to_text.md): drop TeX structure entirely instead of
  converting it to another markup language.
- [`render`](render.md): a readable Unicode projection instead of markup.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_latex.html"><span class="label">Previous</span><span class="title">to_latex</span></a>
  <a class="next-link" href="to_text.html"><span class="label">Next</span><span class="title">to_text</span></a>
</div>
