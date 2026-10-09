<!-- pluss-hand-authored -->
# Note::TexAccessor::collapse_macros

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note Note::TexAccessor::collapse_macros() const
```

## Remove Local Declarations Without Expanding Use Sites

`collapse_macros()` reduces declaration-heavy fragments to compact canonical
source. For every cell it removes complete `\newcommand`, `\renewcommand`,
`\providecommand`, and `\def` declarations; applies the built-in spelling
normalizations from [`canonicalize_macros()`](canonicalize_macros.md); then
collapses ASCII whitespace and trims both ends.

The operation intentionally does **not** substitute a removed definition at
its use sites. If a cell declares `\RR` and later contains `\RR`, the use
remains `\RR`. This makes `collapse_macros()` appropriate when declarations
are metadata to discard but symbolic source should remain symbolic. Use
`macro_subs()` when the body must replace every use before the declaration
is removed.

Declaration parsing is brace-aware. Nested bodies such as
`\newcommand{\RR}{\mathbb{R}}` and parameterized definitions such as
`\def\pair#1#2{{#1,#2}}` are removed as complete units rather than leaving a
suffix behind. Malformed or unterminated declarations are retained because
there is no safe declaration boundary to erase.

Whitespace normalization is consequential: spaces, tabs, line feeds,
carriage returns, form feeds, and vertical tabs become one ordinary space,
including whitespace that may be meaningful to a downstream TeX formatter.
The method is therefore a compact-source transform, not merely a declaration
filter. It does not remove comments or external definitions.

The result owns fresh Text storage and preserves rank, shape, and cell order.
Undefined propagates; empty and declaration-only cells become empty strings.
Non-Text Notes reject. Parsing and whitespace normalization are linear in
source size, aside from the bounded lookup of the twelve canonical built-ins.

## Example 1: Nested `newcommand` Body And Surviving Use Site

```cpp
const auto documents = pluss::make_note({
    "  \\newcommand{\\RR}{\\mathbb{R}}\n"
    "  \\dfrac{1}{2}   in   \\RR  ",
    R"(\def\pair#1#2{{#1,#2}}
        \tfrac{x}{y})"});
std::cout << documents.tex().collapse_macros() << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["\\frac{1}{2} in \\RR", "\\frac{x}{y}"]
```

Both declarations disappear, fraction spellings become canonical, and the
first cell's `\RR` use remains unresolved by design.

## Example 2: Native Integer Cells Reject

```cpp
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2})
      .tex().collapse_macros();
} catch (const pluss::Error&) {
  std::cout << "collapse Int64: rejected\n";
}
```

### Output

```text
collapse Int64: rejected
```

## See Also

- [`canonicalize_macros`](canonicalize_macros.md): normalize built-ins without removing declarations.
- [`macro_subs`](macro_subs.md): substitute locally defined bodies before discarding declarations.
- [`Note::collapse_whitespace`](../note/collapse_whitespace.md): whitespace-only normalization on the root text surface.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cmd_sub.html"><span class="label">Previous</span><span class="title">cmd_sub</span></a>
  <a class="next-link" href="command_names.html"><span class="label">Next</span><span class="title">command_names</span></a>
</div>
