<!-- pluss-hand-authored -->
# Note::TexAccessor::strip_figs

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note Note::TexAccessor::strip_figs() const
```

## Remove Figure Regions, Not Graphic Calls

`strip_figs()` removes complete `figure`, `figure*`, `wrapfigure`, and
`subfigure` environments. It exists for source whose figure bodies, captions,
labels, and placement machinery should disappear as a unit. A standalone
`\includegraphics{...}` call is intentionally retained; choose
[`strip_graphics()`](strip_graphics.md) when both figure containers and direct
graphic inclusions must be removed.

Each supported environment is matched by its exact name. Same-name nested
regions are depth balanced, so an inner `figure` cannot close an outer one.
Starred and unstarred names are distinct. Escaped openers and unterminated
regions remain unchanged, while complete regions elsewhere in the same cell
are still removed.

The operation collapses ASCII whitespace around removed regions, allocates a
new Text Note, and preserves rank, shape, and cell order. It is idempotent and
does not inspect image files or resolve paths. Undefined input stays undefined;
all defined non-Text dtypes reject. Runtime is linear for each fixed set of
four environment names.

## Example 1: Nested And Variant Figure Regions

```cpp
const auto source = pluss::make_note({
    R"(A \begin{figure}outer \begin{figure}inner\end{figure} tail\end{figure} B)",
    R"(C \begin{figure*}wide\end{figure*} D \begin{wrapfigure}wrap\end{wrapfigure} E)",
    R"(\includegraphics{kept.png} \begin{figure}unfinished)"});
const auto clean = source.tex().strip_figs();
std::cout << clean << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["A B", "C D E", "\\includegraphics{kept.png} \\begin{figure}unfinished"]
```

## Example 2: A Scalar `subfigure`

```cpp
std::cout << pluss::make_note(
    R"(A \begin{subfigure}part\end{subfigure} B)").tex().strip_figs() << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"A B"
```

A scalar Note remains rank zero. The method transforms cells; it does not
promote scalar input into a collection.

## Example 3: Idempotence, Undefined State, And Dtype

```cpp
const auto once = pluss::make_note(
    R"(A \begin{figure}body\end{figure} B)").tex().strip_figs();
std::cout << std::boolalpha
          << "idempotent=" << ((once == once.tex().strip_figs()).all().bool_item())
          << " undefined="
          << pluss::Note::undefined().tex().strip_figs().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1}).tex().strip_figs();
} catch (const pluss::Error&) {
  std::cout << "strip figs Int64: rejected\n";
}
```

### Output

```text
idempotent=true undefined=false
strip figs Int64: rejected
```

## See Also

- [`figures`](figures.md): extract figure regions instead of deleting them.
- [`strip_graphics`](strip_graphics.md): remove direct graphic calls and figure regions.
- [`strip_env`](strip_env.md): remove one exact environment selected by name.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="strip_envs.html"><span class="label">Previous</span><span class="title">strip_envs</span></a>
  <a class="next-link" href="strip_fmt.html"><span class="label">Next</span><span class="title">strip_fmt</span></a>
</div>
