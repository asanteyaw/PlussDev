<!-- pluss-hand-authored -->
# NoteTexAccessor::strip_env

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note NoteTexAccessor::strip_env(std::string_view name) const
```

## Remove One Exact Environment Kind

`strip_env(name)` removes every complete `\begin{name}...\end{name}` region
from each Text cell. The environment name is exact and may contain ASCII
letters, `@`, and one terminal `*`; a leading backslash, empty name, spaces,
and punctuation are rejected before any cell is transformed.

Same-name nesting is depth balanced, so an inner region cannot terminate its
outer region early. Other environment kinds inside the selected region are
ordinary body content and disappear with it. Escaped openers and closers do
not participate. An opener without a matching closer remains unchanged,
preventing accidental deletion through the rest of a cell.

Whitespace is collapsed and trimmed after complete regions are removed. The
operation is shape preserving and out-of-place. Undefined input remains
undefined after name validation; non-Text input rejects. Runtime is linear in
the source bytes for a fixed name, and memory is proportional to retained text.

## Example 1: Nested, Escaped, And Unclosed Regions

```cpp
const auto source = pluss::make_note({
    R"(A \begin{proof}outer \begin{proof}inner\end{proof} tail\end{proof} B)",
    R"(\\begin{proof}escaped\end{proof})",
    R"(\begin{proof}unfinished)"});
std::cout << source.tex().strip_env("proof") << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["A B", "\\\\begin{proof}escaped\\end{proof}", "\\begin{proof}unfinished"]
```

## Example 2: Starred And Unstarred Names Are Distinct

```cpp
const auto source = pluss::make_note({
    R"(A \begin{figure*}wide\end{figure*} B)",
    R"(C \begin{figure}plain\end{figure} D)"});
std::cout << source.tex().strip_env("figure*") << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["A B", "C \\begin{figure}plain\\end{figure} D"]
```

The terminal star is part of the environment name. Selecting `figure*` does
not also select `figure`.

## Example 3: Invalid Names Fail Before Transformation

```cpp
try {
  (void)source.tex().strip_env("bad name");
} catch (const pluss::Error&) {
  std::cout << "strip_env(bad name): rejected\n";
}
```

### Output

```text
strip_env(bad name): rejected
```

## Example 4: Undefined And Invalid Dtype

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().strip_env("proof").defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1})
      .tex().strip_env("proof");
} catch (const pluss::Error&) {
  std::cout << "strip env Int64: rejected\n";
}
```

### Output

```text
false
strip env Int64: rejected
```

## See Also

- [`env`](env.md): extract one exact environment kind instead.
- [`envs`](envs.md): extract all complete environments with offsets.
- [`strip_envs`](strip_envs.md): remove every complete environment kind.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="strip_delims.html"><span class="label">Previous</span><span class="title">strip_delims</span></a>
  <a class="next-link" href="strip_envs.html"><span class="label">Next</span><span class="title">strip_envs</span></a>
</div>
