<!-- pluss-hand-authored -->
# Note::TexAccessor::strip_links

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note Note::TexAccessor::strip_links() const
```

## Remove Destinations Without Losing Labels

`strip_links()` rewrites three TeX link forms according to what a reader can
see. `\href{destination}{label}` and `\hyperref[target]{label}` become their
labels; `\url{destination}` disappears because the destination is also its
entire visible payload. This single contract replaces the former duplicate URL
and hyperlink removers.

Destination, target, and label groups are balanced. Nested braces in a URL or
label do not truncate a match, and link commands nested inside visible labels
are processed recursively. Other TeX in a retained label is left untouched:
for example, `\textbf{here}` remains formatting source until `strip_fmt()` or
another text projection is requested. Exact control-word matching protects
longer names. Escaped and incomplete calls remain byte-for-byte present.

After rewriting, ASCII whitespace is collapsed and each cell is trimmed. The
receiver is not mutated; the result preserves rank, shape, and dtype Text.
Repeated application is stable. Undefined input stays undefined and defined
non-Text input rejects. The balanced scanner is linear in source and retained
label bytes and performs no URL validation or network access.

## Example 1: Visible Labels, Bare URLs, And Boundaries

```cpp
const auto source = pluss::make_note({
    R"(A \href{https://x.test/{a}}{visible {label}} B)",
    R"(C \hyperref[target]{jump \textbf{here}} D \url{https://y.test} E)",
    R"(\\href{escaped}{kept} \href{broken}{unfinished)"});
std::cout << source.tex().strip_links() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["A visible {label} B", "C jump \\textbf{here} D E", "\\\\href{escaped}{kept} \\href{broken}{unfinished"]
```

The label survives because it carries authored prose. The bare `\url` has no
separate label, so no destination text is promoted into the result.

## Example 2: Shape, Allocation, And Empty Labels

```cpp
const auto grid = pluss::note::make(
    std::vector<std::string>{R"(\href{u}{one})", R"(\url{u})", "plain", ""},
    std::array<std::int64_t, 2>{2, 2});
const auto clean = grid.tex().strip_links();
std::cout << clean << '\n';
std::cout << "shape=[" << clean.size(0) << ", " << clean.size(1) << "]\n";
std::cout << std::boolalpha
          << "receiver unchanged=" << (grid.at(0).find("href") != std::string_view::npos)
          << " idempotent=" << ((clean == clean.tex().strip_links()).all().bool_item())
          << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["one", ""], 
  ["plain", ""]
]
shape=[2, 2]
receiver unchanged=true idempotent=true
```

## Example 3: Undefined And Invalid Dtype

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().strip_links().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1}).tex().strip_links();
} catch (const pluss::Error&) {
  std::cout << "strip links Int64: rejected\n";
}
```

### Output

```text
false
strip links Int64: rejected
```

## See Also

- [`strip_fmt`](strip_fmt.md): unwrap formatting retained inside link labels.
- [`strip_cmds`](strip_cmds.md): remove calls together with their attached content.
- [`to_text`](to_text.md): apply a broader readable-text projection.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="strip_labels.html"><span class="label">Previous</span><span class="title">strip_labels</span></a>
  <a class="next-link" href="strip_lr.html"><span class="label">Next</span><span class="title">strip_lr</span></a>
</div>
