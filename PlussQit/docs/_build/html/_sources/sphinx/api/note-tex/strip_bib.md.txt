<!-- pluss-hand-authored -->
# Note::TexAccessor::strip_bib

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note Note::TexAccessor::strip_bib() const
```

## Remove Bibliography Machinery As Complete Units

`strip_bib()` removes complete `\bibliography{...}` and
`\addbibresource{...}` calls, `\printbibliography` with or without options,
and complete `thebibliography` environments. It is intended for source views
that retain surrounding prose but must exclude bibliography configuration and
authored reference-list bodies.

Required and optional groups are balanced, so braces inside a resource path or
option do not terminate a call early. Same-name bibliography environments are
depth balanced and removed as one outer region. Escaped commands, longer
control words, incomplete required groups, and unclosed environments remain
unchanged rather than causing partial deletion.

After removal, ASCII whitespace is collapsed and the cell is trimmed. This
prevents holes left by deleted regions, but means the operation is not a
byte-preserving splice. It is lane-wise, out-of-place, and shape preserving;
empty results remain empty cells. Undefined input stays undefined and defined
non-Text Notes are rejected. The balanced scan is linear in source bytes and
allocates one result string per cell.

## Example 1: Commands, Options, And Nested Environments

```cpp
const auto source = pluss::make_note({
    R"(Before \bibliography{refs/{main}} middle \printbibliography[heading={none}] after)",
    R"(A \begin{thebibliography}{9} one \begin{thebibliography}{1} nested \end{thebibliography} two \end{thebibliography} Z)",
    R"(\\bibliography{escaped} \bibliography{unfinished)"});
std::cout << source.tex().strip_bib() << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["Before middle after", "A Z", "\\\\bibliography{escaped} \\bibliography{unfinished"]
```

The first two lanes remove complete structures. The escaped and malformed
forms in the third lane are preserved.

## Example 2: Higher-Rank Input, Allocation, And Idempotence

```cpp
const auto grid = pluss::note::make(
    std::vector<std::string>{R"(A \addbibresource{refs/a.bib} B)",
                             R"(\printbibliography)", "plain", ""},
    std::array<std::int64_t, 2>{2, 2});
const auto clean = grid.tex().strip_bib();
std::cout << clean << '\n';
std::cout << "shape=[" << clean.size(0) << ", " << clean.size(1) << "]\n";
std::cout << std::boolalpha
          << "receiver unchanged="
          << (grid.at(0).find("addbibresource") != std::string_view::npos)
          << " idempotent=" << ((clean == clean.tex().strip_bib()).all().bool_item())
          << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["A B", ""], 
  ["plain", ""]
]
shape=[2, 2]
receiver unchanged=true idempotent=true
```

The empty result from `\printbibliography` remains a cell; it is not removed
from the tensor. The unchanged receiver and idempotent result make the method
safe to place in a repeatable cleanup pipeline.

## Example 3: Undefined And Invalid Input

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().strip_bib().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1}).tex().strip_bib();
} catch (const pluss::Error&) {
  std::cout << "strip bib Int64: rejected\n";
}
```

### Output

```text
false
strip bib Int64: rejected
```

## See Also

- [`bibliography_keys`](bibliography_keys.md): retain bibliography identifiers instead of removing source.
- [`strip_env`](strip_env.md): remove one caller-selected environment kind.
- [`strip_pkgs`](strip_pkgs.md): remove package and document-class declarations.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sqrt_pow.html"><span class="label">Previous</span><span class="title">sqrt_pow</span></a>
  <a class="next-link" href="strip_cites.html"><span class="label">Next</span><span class="title">strip_cites</span></a>
</div>
