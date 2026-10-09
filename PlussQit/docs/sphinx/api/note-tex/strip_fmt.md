<!-- pluss-hand-authored -->
# Note::TexAccessor::strip_fmt

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note Note::TexAccessor::strip_fmt() const
```

## Unwrap Formatting While Keeping Its Text

`strip_fmt()` removes the wrappers `\textbf`, `\textit`, `\emph`,
`\underline`, `\texttt`, `\mathrm`, `\mathbf`, `\mathit`, and `\mathsf`,
but retains each command's required argument. It answers a different question
from [`strip_cmds()`](strip_cmds.md): formatting is presentation around useful
content, so this method promotes the visible body instead of deleting the
entire call.

Required groups are brace balanced and nested supported wrappers are processed
recursively. Exact command matching leaves longer names unchanged. Escaped
commands and commands without a complete required group remain untouched; an
incomplete wrapper is not partially unwrapped. Font declaration commands such
as `\bfseries` are outside this narrower contract and remain in the result;
[`strip_fonts()`](strip_fonts.md) removes those as well.

Whitespace is normalized after unwrapping. The operation is lane-wise,
out-of-place, shape preserving, and idempotent. Undefined input propagates and
defined non-Text input rejects. Its balanced scan is linear in source plus
retained body bytes; recursive memory follows formatting nesting depth.

## Example 1: Nested Text And Math Formatting

```cpp
const auto source = pluss::make_note({
    R"(A \textbf{bold \emph{and nested}} B)",
    R"(\underline{under} \mathrm{x} \texttt{code})",
    R"(\\textbf{escaped} \textbf{unfinished)"});
std::cout << source.tex().strip_fmt() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["A bold and nested B", "under x code", "\\\\textbf{escaped} \\textbf{unfinished"]
```

## Example 2: Formatting A Matrix Does Not Flatten It

```cpp
const auto grid = pluss::note::make(
    std::vector<std::string>{R"(\mathbf{x})", "plain", R"(\mathsf{y})", ""},
    std::array<std::int64_t, 2>{2, 2});
const auto clean = grid.tex().strip_fmt();
std::cout << clean << '\n';
std::cout << "shape=[" << clean.size(0) << ", " << clean.size(1) << "]\n";
std::cout << std::boolalpha
          << "idempotent=" << ((clean == clean.tex().strip_fmt()).all().bool_item())
          << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["x", "plain"], 
  ["y", ""]
]
shape=[2, 2]
idempotent=true
```

## Example 3: Undefined And Invalid Dtype

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().strip_fmt().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1}).tex().strip_fmt();
} catch (const pluss::Error&) {
  std::cout << "strip fmt Int64: rejected\n";
}
```

### Output

```text
false
strip fmt Int64: rejected
```

## See Also

- [`strip_fonts`](strip_fonts.md): also remove declaration-style font switches.
- [`strip_color`](strip_color.md): unwrap color presentation while retaining content.
- [`strip_cmds`](strip_cmds.md): discard commands together with attached groups.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="strip_figs.html"><span class="label">Previous</span><span class="title">strip_figs</span></a>
  <a class="next-link" href="strip_fonts.html"><span class="label">Next</span><span class="title">strip_fonts</span></a>
</div>
