<!-- pluss-hand-authored -->
# NoteTexAccessor::strip_graphics

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note NoteTexAccessor::strip_graphics() const
```

## Remove Direct Graphics And Figure Containers

`strip_graphics()` removes complete `\includegraphics` calls and complete
`figure` or `figure*` environments. It is the broad image cleanup operation:
direct inclusions disappear wherever they occur, and a figure container is
removed with its caption, labels, and body. Use [`strip_figs()`](strip_figs.md)
when standalone graphic calls must survive, or `graphics_tags()` when a
graphic call should become an explicit text marker rather than disappear.

An `\includegraphics` call may have one balanced optional group followed by
one balanced required path group. Nested braces in either group are respected.
The command is removed only when the complete required group exists. Figure
regions are same-name depth balanced and starred names are handled separately.
Escaped commands, longer control words, incomplete calls, and unclosed figure
regions remain unchanged.

Removal is cell-local, out-of-place, and shape preserving. ASCII whitespace is
collapsed and cells are trimmed; a cell containing only a graphic becomes an
empty string, not an absent cell. Undefined input propagates and defined
non-Text Notes reject. Runtime is linear in source bytes and does not access
the filesystem or decode image content.

## Example 1: Options, Nested Paths, Figures, And Malformed Calls

```cpp
const auto source = pluss::make_note({
    R"(A \includegraphics[width={0.5\textwidth}]{img/{plot}.pdf} B)",
    R"(C \begin{figure}body\end{figure} D \begin{figure*}wide\end{figure*} E)",
    R"(\\includegraphics{escaped} \includegraphics{unfinished)"});
std::cout << source.tex().strip_graphics() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["A B", "C D E", "\\\\includegraphics{escaped} \\includegraphics{unfinished"]
```

## Example 2: Empty Cells Retain Matrix Position

```cpp
const auto grid = pluss::note::make(
    std::vector<std::string>{R"(\includegraphics{a})", "plain",
                             R"(\begin{figure}b\end{figure})", ""},
    std::array<std::int64_t, 2>{2, 2});
const auto clean = grid.tex().strip_graphics();
std::cout << clean << '\n';
std::cout << "shape=[" << clean.size(0) << ", " << clean.size(1) << "]\n";
std::cout << std::boolalpha
          << "idempotent=" << ((clean == clean.tex().strip_graphics()).all().bool_item())
          << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["", "plain"], 
  ["", ""]
]
shape=[2, 2]
idempotent=true
```

## Example 3: Undefined And Invalid Dtype

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().strip_graphics().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1}).tex().strip_graphics();
} catch (const pluss::Error&) {
  std::cout << "strip graphics Int64: rejected\n";
}
```

### Output

```text
false
strip graphics Int64: rejected
```

## See Also

- [`strip_figs`](strip_figs.md): remove figure-family environments only.
- [`includes`](includes.md): extract graphic and source include targets.
- `graphics_tags`: retain a textual marker containing each graphic path.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="strip_fonts.html"><span class="label">Previous</span><span class="title">strip_fonts</span></a>
  <a class="next-link" href="strip_labels.html"><span class="label">Next</span><span class="title">strip_labels</span></a>
</div>
