<!-- pluss-hand-authored -->
# NoteTexAccessor::strip_cites

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note NoteTexAccessor::strip_cites() const
```

## Remove Complete Citation Calls

`strip_cites()` removes complete citation-family calls whose control-word name
begins with `cite`, including starred forms and variants such as `\citep` and
`\citet`. Any number of balanced optional groups may precede the required
braced key group. The whole call is removed; citation keys and notes are not
left in the prose.

Matching is lexical and exact at the command boundary. Escaped commands and
calls without a complete required group remain unchanged. Nested braces in the
key group and brackets inside optional notes are balanced. Use
[`citations`](citations.md) when citation arguments should be extracted rather
than discarded, and [`strip_bib`](strip_bib.md) for bibliography declarations
and reference-list environments.

Whitespace is collapsed and trimmed after removal. The returned Text Note is a
new, shape-preserving allocation; the receiver is unchanged. Undefined input
remains undefined and non-Text dtypes reject. Work is linear in source bytes.

## Example 1: Notes, Stars, Nested Keys, And Malformed Calls

```cpp
const auto source = pluss::make_note({
    R"(A \citep[see][p.~2]{alpha,{beta}} B \cite*{gamma} C)",
    R"(\\cite{escaped} \cite{unfinished)"});
std::cout << source.tex().strip_cites() << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["A B C", "\\\\cite{escaped} \\cite{unfinished"]
```

## Example 2: Variants Preserve A Matrix's Structure

```cpp
const auto grid = pluss::note::make(
    std::vector<std::string>{R"(Lead \citet{a} tail)", R"(\citeauthor*{b})",
                             "uncited prose", ""},
    std::array<std::int64_t, 2>{2, 2});
const auto clean = grid.tex().strip_cites();
std::cout << clean << '\n';
std::cout << "shape=[" << clean.size(0) << ", " << clean.size(1) << "]\n";
std::cout << std::boolalpha
          << "idempotent=" << ((clean == clean.tex().strip_cites()).all().bool_item())
          << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["Lead tail", ""], 
  ["uncited prose", ""]
]
shape=[2, 2]
idempotent=true
```

The lexical family rule includes `\citet` and `\citeauthor*`; a cell that
contains only a citation becomes an empty string without changing matrix
position.

## Example 3: Undefined And Invalid Input

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().strip_cites().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1}).tex().strip_cites();
} catch (const pluss::Error&) {
  std::cout << "strip cites Int64: rejected\n";
}
```

### Output

```text
false
strip cites Int64: rejected
```

## See Also

- [`citations`](citations.md): extract citation arguments with lane ownership.
- [`bibliography_keys`](bibliography_keys.md): split individual citation and bibliography keys.
- [`strip_bib`](strip_bib.md): remove bibliography machinery.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="strip_bib.html"><span class="label">Previous</span><span class="title">strip_bib</span></a>
  <a class="next-link" href="strip_cmds.html"><span class="label">Next</span><span class="title">strip_cmds</span></a>
</div>
