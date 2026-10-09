<!-- pluss-hand-authored -->
# Note::TexAccessor::strip_preamble

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note Note::TexAccessor::strip_preamble() const
```

## Isolate A Complete Document Body

`strip_preamble()` returns the source between the first complete exact
`\begin{document}` and the next complete exact `\end{document}`. Bytes before
the opener and after the closer are discarded. Use it when a full document must
be reduced to its authored body before command extraction or conversion. Use
[`strip_pkgs()`](strip_pkgs.md) when only package and document-class calls should
be removed while all other source remains, or [`strip_metadata()`](strip_metadata.md)
for title-page declarations alone.

Whitespace is permitted between `begin` or `end` and the balanced
`{document}` group, and around `document` inside that group. Matching is exact,
case-sensitive, and lexical: escaped tags and longer control words do not form
boundaries. Both boundaries are required. If either is absent or malformed, the
entire cell is returned unchanged rather than guessing where authored content
ends. The extracted body is trimmed at its outer ASCII whitespace; internal
line endings and all nested environments remain byte-for-byte intact.

The operation is out-of-place and shape preserving. A document with an empty
body yields an empty Text cell, not an absent lane. Undefined state propagates;
defined non-Text input rejects. The implementation makes a linear scan and does
not parse, compile, or validate the document body.

## Example 1: Extract A Body And Retain Nested Source

```cpp
const auto source = pluss::make_note(
    R"(\documentclass{article}
\begin { document }
Body \begin{figure}x\end{figure}
\end{document}
trailer)");
std::cout << source.tex().strip_preamble() << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"Body \\begin{figure}x\\end{figure}"
```

## Example 2: Incomplete And Escaped Boundaries Are Loss-Preserving

```cpp
const auto source = pluss::make_note({
    R"(head \begin{document}body)",
    R"(\\begin{document}escaped\\end{document})",
    "plain body"});
std::cout << source.tex().strip_preamble() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["head \\begin{document}body", "\\\\begin{document}escaped\\\\end{document}", "plain body"]
```

## Example 3: Empty Bodies And Matrix Shape

```cpp
const auto grid = pluss::make_note(
    std::vector<std::string>{R"(h\begin{document}A\end{document}t)",
                             "plain", R"(\begin{document}\end{document})", ""},
    std::array<std::int64_t, 2>{2, 2});
std::cout << grid.tex().strip_preamble() << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["A", "plain"],
  ["", ""]
]
```

## Example 4: Idempotence And Type Boundaries

```cpp
const auto once = pluss::make_note(
    R"(head\begin{document}body\end{document}tail)")
    .tex().strip_preamble();
std::cout << std::boolalpha
          << "idempotent="
          << ((once == once.tex().strip_preamble()).all().bool_item())
          << " undefined="
          << pluss::Note::undefined().tex().strip_preamble().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1}).tex().strip_preamble();
} catch (const pluss::Error&) {
  std::cout << "Int64: rejected\n";
}
```

### Output

```text
idempotent=true undefined=false
Int64: rejected
```

## See Also

- [`strip_metadata`](strip_metadata.md): remove title-page declarations only.
- [`strip_pkgs`](strip_pkgs.md): remove package and document-class calls.
- [`strip_comments`](strip_comments.md): remove comments while retaining lines.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="strip_pkgs.html"><span class="label">Previous</span><span class="title">strip_pkgs</span></a>
  <a class="next-link" href="strip_refs.html"><span class="label">Next</span><span class="title">strip_refs</span></a>
</div>
