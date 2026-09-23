<!-- pluss-hand-authored -->
# NoteTexAccessor::to_html

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note NoteTexAccessor::to_html() const
```

## Convert A Recognized TeX Subset To HTML Markup

`to_html()` rewrites a fixed set of common TeX commands to their HTML
equivalents — headings, bold, italic/emphasis, a linebreak, and list items
— unwraps color and hyperlink wrappers down to their visible content, and
removes labels, references, citations, packages, bibliography commands,
spacing/delimiter commands, and font-size declarations. Use it for a quick,
best-effort HTML preview of simple TeX prose; it is not a full document
converter.

The recognized rewrites, applied in this order:

| TeX | HTML |
| --- | --- |
| `\section{...}` / `\section*{...}` | `<h1>...</h1>` |
| `\subsection{...}` / `\subsection*{...}` | `<h2>...</h2>` |
| `\subsubsection{...}` / `\subsubsection*{...}` | `<h3>...</h3>` |
| `\textbf{...}` | `<strong>...</strong>` |
| `\emph{...}` / `\textit{...}` | `<em>...</em>` |
| `\\` (a TeX linebreak) | `<br/>` |
| `\item` | `<li>` |

`\textcolor{...}{...}`/`\colorbox{...}{...}` and `\href{url}{label}`/
hyperref commands are then unwrapped down to their visible content — the
color name and the URL are both discarded, same as on
[`to_markdown`](to_markdown.md); `to_html()` does not emit an HTML anchor
for a hyperlink, it only keeps the label text.

There is no `mode` parameter — unlike [`to_text`](to_text.md) and
`to_markdown`'s own `mode`, `to_html()` has exactly one behavior.

The operation is shape-preserving, cell-independent, and returns a fresh
`Note`. Undefined input remains undefined; Bool, Int64, Float64, and Bytes
receivers are rejected because TeX source is Text.

## Example 1: Headings, Inline Formatting, Linebreak, Item, Link, And Color

```cpp
std::cout << pluss::make_note({
    R"(\section{Intro}\subsection{Detail}\subsubsection{Fine})",
    R"(\textbf{Bold} and \emph{emphasis}\\next \item first)",
    R"(See \href{https://example.test}{label} \label{x} \ref{x}.)",
    R"(\textcolor{red}{warn})"})
    .tex().to_html()
    << '\n';
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["<h1>Intro</h1><h2>Detail</h2><h3>Fine</h3>", "<strong>Bold</strong> and <em>emphasis</em><br/>next <li> first", "See label .", "warn"]
```

The third cell shows the link unwrapped to its bare `label` — the URL is
discarded, not turned into an `<a href="...">` tag — alongside the two
recognized removals, `\label{x}` and `\ref{x}`. The fourth cell shows
`\textcolor{red}{warn}` reduced to its bare content the same way.

## Example 2: A 2-D Grid, Shape Preserved

```cpp
const auto grid = pluss::make_note(
    std::vector<std::string>{R"(\section{A})", R"(\textbf{B})", "plain", ""},
    std::array<std::int64_t, 2>{2, 2});
std::cout << grid.tex().to_html() << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["<h1>A</h1>", "<strong>B</strong>"], 
  ["plain", ""]
]
```

## Example 3: Undefined And Non-Text Input

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().to_html().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2}).tex().to_html();
} catch (const pluss::Error&) {
  std::cout << "to html Int64: rejected\n";
}
```

### Output

```text
false
to html Int64: rejected
```

## See Also

- [`to_markdown`](to_markdown.md): the equivalent conversion to Markdown,
  which shares the same color/hyperlink removal and adds a `mode` for
  case-folding that `to_html()` has no counterpart for.
- [`to_text`](to_text.md): drop TeX structure entirely instead of
  converting it to another markup language.
- [`render`](render.md): a readable Unicode projection instead of markup.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="text_only.html"><span class="label">Previous</span><span class="title">text_only</span></a>
  <a class="next-link" href="to_latex.html"><span class="label">Next</span><span class="title">to_latex</span></a>
</div>
