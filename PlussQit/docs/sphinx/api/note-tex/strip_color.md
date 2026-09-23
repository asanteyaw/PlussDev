<!-- pluss-hand-authored -->
# NoteTexAccessor::strip_color

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note NoteTexAccessor::strip_color() const
```

## Remove Color Styling While Retaining Visible Content

`strip_color()` unwraps `\textcolor{color}{content}`, `\colorbox{color}{content}`,
and `\fcolorbox{frame}{background}{content}` to their visible content. Optional
color-model groups are accepted. Declaration commands `\color{...}` and
`\pagecolor{...}` are removed because they carry no visible body.

All required and optional groups are balanced, so nested braces in visible
content survive. Exact control-word matching prevents longer or escaped
commands from being rewritten. An incomplete call remains unchanged rather
than exposing only a fragment of its arguments.

Whitespace is collapsed and trimmed after rewriting. The method allocates a
new Text Note with the receiver's rank and shape and does not mutate its input.
Undefined state propagates; non-Text dtypes reject. Runtime is linear in source
and retained-content bytes.

## Example 1: Wrappers, Declarations, And Boundaries

```cpp
const auto source = pluss::make_note({
    R"(A \textcolor{red}{bold {nested}} B \colorbox{blue}{box} C)",
    R"(\fcolorbox[rgb]{0,0,0}{1,1,1}{framed} \color{green} tail)",
    R"(\\textcolor{red}{escaped} \textcolor{red}{unfinished)"});
std::cout << source.tex().strip_color() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["A bold {nested} B box C", "framed tail", "\\\\textcolor{red}{escaped} \\textcolor{red}{unfinished"]
```

## Example 2: Declarations, Color Models, And Shape

```cpp
const auto grid = pluss::note::make(
    std::vector<std::string>{R"(\pagecolor{black} white)",
                             R"(\textcolor[rgb]{1,0,0}{red})", "plain", ""},
    std::array<std::int64_t, 2>{2, 2});
const auto clean = grid.tex().strip_color();
std::cout << clean << '\n';
std::cout << "shape=[" << clean.size(0) << ", " << clean.size(1) << "]\n";
std::cout << std::boolalpha
          << "receiver unchanged=" << (grid.at(0).find("pagecolor") != std::string_view::npos)
          << " idempotent=" << ((clean == clean.tex().strip_color()).all().bool_item())
          << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["white", "red"], 
  ["plain", ""]
]
shape=[2, 2]
receiver unchanged=true idempotent=true
```

The optional `[rgb]` model describes the color argument, not visible content.
It disappears with that argument, while the final body remains in place.

## Example 3: Undefined And Invalid Input

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().strip_color().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1}).tex().strip_color();
} catch (const pluss::Error&) {
  std::cout << "strip color Int64: rejected\n";
}
```

### Output

```text
false
strip color Int64: rejected
```

## See Also

- [`strip_fmt`](strip_fmt.md): unwrap recognized font-formatting commands.
- [`strip_cmds`](strip_cmds.md): discard complete command calls and their content.
- [`deaccent`](deaccent.md): remove accent styling while retaining characters.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="strip_cmds.html"><span class="label">Previous</span><span class="title">strip_cmds</span></a>
  <a class="next-link" href="strip_comments.html"><span class="label">Next</span><span class="title">strip_comments</span></a>
</div>
