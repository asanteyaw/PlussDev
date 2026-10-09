<!-- pluss-hand-authored -->
# Note::TexAccessor::strip_macro

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note Note::TexAccessor::strip_macro(std::string_view name) const
```

## Remove One Macro's Declarations

`strip_macro(name)` removes complete declarations of one control-word name
from each cell. It recognizes `\newcommand`, `\renewcommand`,
`\providecommand`, and primitive `\def` declarations, including parameter
specifications and balanced nested bodies. Use it when a local definition must
be discarded while declarations of other macros and all ordinary source remain.

`name` accepts `RR` or `\RR`; both identify the same control word. ASCII
letters and `@` are valid, with an optional terminal `*`. Empty names, spaces,
punctuation, and embedded backslashes reject before any cell is transformed.
Matching is exact and case-sensitive. The declaration disappears, but use
sites such as `\RR{x}` remain unchanged: this is declaration removal, not
macro expansion. Use [`macro_subs()`](macro_subs.md) to expand local definitions
before discarding them.

Malformed declarations are retained. Complete declarations of other names are
also retained. Removed regions are replaced by spacing, then ASCII whitespace
is collapsed and cells are trimmed. The result is out-of-place, shape
preserving, and idempotent for one name. Undefined state propagates only after
the name passes validation; defined non-Text input rejects. Runtime is linear
in source bytes, with balanced-group memory proportional to nesting depth.

## Example 1: Declaration Families, Parameters, And Use Sites

```cpp
const auto source = pluss::make_note({
    R"(A \newcommand{\RR}[1]{\mathbf{R}_{#1}} B \RR{x} C)",
    R"(\def\RR#1{{#1,#1}} \renewcommand{\ZZ}{Z} \RR{q})",
    R"(\\newcommand{\RR}{escaped} \newcommand{\RR}{unfinished)"});
const auto clean = source.tex().strip_macro("RR");
std::cout << clean << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["A B \\RR{x} C", "\\renewcommand{\\ZZ}{Z} \\RR{q}", "\\\\newcommand{\\RR}{escaped} \\newcommand{\\RR}{unfinished"]
```

The `RR` declarations disappear; `RR` use sites and the complete `ZZ`
declaration remain. Escaped and incomplete declaration text is preserved.

## Example 2: A Leading Backslash Is An Equivalent Name Spelling

```cpp
const auto source = pluss::make_note({
    R"(A \newcommand{\RR}[1]{\mathbf{R}_{#1}} B \RR{x} C)",
    R"(\def\RR#1{{#1,#1}} \renewcommand{\ZZ}{Z} \RR{q})",
    R"(\\newcommand{\RR}{escaped} \newcommand{\RR}{unfinished)"});
std::cout << source.tex().strip_macro(R"(\RR)") << '\n';
std::cout << std::boolalpha
          << "same="
          << ((source.tex().strip_macro("RR") ==
               source.tex().strip_macro(R"(\RR)")).all().bool_item())
          << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["A B \\RR{x} C", "\\renewcommand{\\ZZ}{Z} \\RR{q}", "\\\\newcommand{\\RR}{escaped} \\newcommand{\\RR}{unfinished"]
same=true
```

## Example 3: Invalid Names Reject Before Reading Input

```cpp
try {
  (void)pluss::Note::undefined().tex().strip_macro("bad name");
} catch (const pluss::Error&) {
  std::cout << "strip_macro(bad name): rejected\n";
}
```

### Output

```text
strip_macro(bad name): rejected
```

## Example 4: Undefined State, Dtype, And Idempotence

```cpp
const auto source = pluss::make_note(
    R"(A \newcommand{\RR}{\mathbb{R}} B \RR C)");
const auto once = source.tex().strip_macro("RR");
std::cout << std::boolalpha
          << "idempotent=" << ((once == once.tex().strip_macro("RR")).all().bool_item())
          << " undefined="
          << pluss::Note::undefined().tex().strip_macro("RR").defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1})
      .tex().strip_macro("RR");
} catch (const pluss::Error&) {
  std::cout << "strip macro Int64: rejected\n";
}
```

### Output

```text
idempotent=true undefined=false
strip macro Int64: rejected
```

## See Also

- [`macro_defs`](macro_defs.md): extract declared macro names.
- [`macro_subs`](macro_subs.md): expand cell-local simple macros.
- [`collapse_macros`](collapse_macros.md): remove all local declarations and normalize built-ins.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="strip_lr.html"><span class="label">Previous</span><span class="title">strip_lr</span></a>
  <a class="next-link" href="strip_metadata.html"><span class="label">Next</span><span class="title">strip_metadata</span></a>
</div>
