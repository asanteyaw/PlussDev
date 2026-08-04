# Note::tex_to_text

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note::tex_to_text() const -> Note
Note::tex_to_text(std::int64_t mode) const -> Note
```

## What It Does

Reduces TeX-authored content into plain text.

This is the method to reach for when your input is already TeX or LaTeX-like
markup and you want a text-oriented downstream surface rather than escaped TeX
syntax.

In practical terms, this method strips away much of the TeX structure and keeps
the readable text payload that remains useful for indexing, summarization, or
plain-text export.

So `tex_to_text()` goes in the opposite direction from [`tex_escape`](tex_escape.md):
it consumes authored TeX-like markup and tries to recover readable plain text.

## Parameters

**mode**

Optional conversion mode for alternative TeX-to-text policies.

Use the zero-argument form when you want the default library behavior. Reach
for the explicit `mode` overload only when you already know the implementation
mode contract you intend to target.

## Returns

`Note`
    A shape-preserving `Note` where TeX markup has been reduced toward plain
    text.

## Examples

Example 1. Collapse a small TeX document into plain text.

```cpp
auto out = pluss::make_note(
    "\\documentclass{article}\n"
    "% comment\n"
    "\\begin{document}\n"
    "Text \\textbf{bold} and $x+y$.\n"
    "\\label{l1}\\ref{l1}\\cite{k}\\href{https://x.y}{link}\n"
    "\\end{document}\n")
    .tex_to_text();
std::cout << "tex_to_text:\n";
for (std::int64_t i = 0; i < out.numel(); ++i) {
  std::cout << "  [" << i << "] " << out.at(i) << "\n";
}
```

### Output

```text
tex_to_text:
  [0] Text bold and . link
```

Example 2. Use the same surface on smaller inline markup when you only need the
readable text payload.

```cpp
auto out = pluss::make_note("\\textit{Hello}").tex_to_text();
std::cout << "tex_to_text_inline:\n";
for (std::int64_t i = 0; i < out.numel(); ++i) {
  std::cout << "  [" << i << "] " << out.at(i) << "\n";
}
```

### Output

```text
tex_to_text_inline:
  [0] Hello
```

## Notes

- The current default behavior keeps readable text and drops much of the TeX
  control structure.
- The example shows an important practical point: math markup is not preserved
  as literal source text by default here.
- If the goal is to protect plain text before inserting it into TeX, use
  [`tex_escape`](tex_escape.md) instead. `tex_to_text()` moves in the opposite
  direction.

## See Also

- [`Note::tex_escape`](tex_escape.md): escape plain text for TeX output rather
  than reducing TeX into plain text.
- [`Note::nlp_remove_urls`](nlp_remove_urls.md): additional cleanup after
  textualization if you still want to strip noisy link-like remnants.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tex_escape.html">
    <span class="label">Previous</span>
    <span class="title">tex_escape</span>
  </a>
  <a class="next-link" href="remove_all.html">
    <span class="label">Next</span>
    <span class="title">remove_all</span>
  </a>
</div>
