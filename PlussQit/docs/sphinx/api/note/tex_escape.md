# Note::tex_escape

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note::tex_escape() const -> Note
```

## What It Does

Escapes TeX-sensitive characters so ordinary text can be inserted more safely
into LaTeX-oriented content.

This is the first TeX-facing method many users need. It is useful when you have
plain text containing `%`, `$`, backslashes, or similar special characters and
you want to move that text into a TeX document without it being interpreted as
raw TeX syntax.

It is not the right method when the input is already authored LaTeX that you
intend to preserve as live commands. In that case, escaping would destroy the
meaning of the source.

## Parameters

This method takes no parameters.

## Returns

`Note`
    A new `Note` with TeX-special characters escaped.

## Examples

Example 1. Escape percent signs, dollar signs, and literal backslash-led names.

```cpp
auto tex = pluss::make_note({
    "Price is 50% of $x+y$",
    R"(Use \alpha and \beta.)",
});

auto out = tex.tex_escape();
std::cout << "tex_escape:\n";
for (std::int64_t i = 0; i < out.numel(); ++i) {
  std::cout << "  [" << i << "] " << out.at(i) << "\n";
}
```

### Output

```text
tex_escape:
  [0] Price is 50\% of \$x+y\$
  [1] Use \textbackslash{}alpha and \textbackslash{}beta.
```

Example 2. Plain text that has no TeX-sensitive characters stays readable after
the pass.

```cpp
auto out = pluss::make_note({"chapter title", "plain text"}).tex_escape();
std::cout << "tex_escape_plain:\n";
for (std::int64_t i = 0; i < out.numel(); ++i) {
  std::cout << "  [" << i << "] " << out.at(i) << "\n";
}
```

### Output

```text
tex_escape_plain:
  [0] chapter title
  [1] plain text
```

## Notes

- `tex_escape()` is for protecting plain text, not for preserving live TeX
  commands.
- If your input is already authored TeX and you want to inspect or simplify it,
  use the broader TeX method family instead of escaping it again.
- This method preserves shape and works cell by cell like the rest of the `Note`
  transform family.

## See Also

- [`Note::replace`](replace.md): manual literal substitution if you only need a
  tiny custom escape rule.
- [`Note::nlp_remove_urls`](nlp_remove_urls.md): remove unwanted link text
  before escaping for TeX export.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="strip.html">
    <span class="label">Previous</span>
    <span class="title">strip</span>
  </a>
  <a class="next-link" href="tex_to_text.html">
    <span class="label">Next</span>
    <span class="title">tex_to_text</span>
  </a>
</div>
