<!-- pluss-hand-authored -->
# NoteTexAccessor::macro_defs

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
TokenBatch NoteTexAccessor::macro_defs() const
```

## Inventory Complete Local Macro Declarations

`macro_defs()` extracts the control word introduced by each complete local
macro declaration. It recognizes `\newcommand`, `\renewcommand`, and
`\providecommand`, including their starred declaration forms, plus primitive
`\def`. Each token includes its leading backslash, such as `\RR`; this makes a
definition inventory directly comparable with [`macro_use`](macro_use.md).

For the command-style declarations, both `{\name}` and direct `\name` spellings
are accepted. Optional arity and default-value groups may follow the name, and
their brackets are balanced before the required replacement body is read. For
`\def`, parameter markers between the name and body are accepted. A declaration
is emitted only after its complete balanced body has been found; a name alone
does not count as a usable definition.

The extractor reports declarations, not their bodies, arity, scope, or final
meaning after redefinition. It preserves repeated declarations and source
order because either can matter to a later semantic pass. It does not expand
commands, follow included files, or attempt TeX category-code interpretation.
An escaped declaration command and malformed declaration contribute no token.

The result is ragged. `tokens()` owns a flat Text Note, and `offsets()` owns an
Int64 boundary vector whose range for cell `i` is
`[offsets[i], offsets[i + 1])`. Matrix and higher-rank receivers are traversed
in row-major lane order. Undefined input yields undefined tokens and offsets
`[0]`; any defined non-Text dtype is rejected.

Scanning is linear in source bytes and uses balanced delimiter scans rather
than a document parser. Token strings are copied into result-owned storage.

## Example 1: Command-Style And Primitive Declarations

```cpp
const auto source = pluss::make_note({
    R"(\newcommand{\RR}{\mathbb{R}} \renewcommand*\vecx[1]{\mathbf{#1}})",
    R"(\providecommand{\unit}[1][kg]{#1} \def\pair#1#2{#1,#2})",
    R"(\\newcommand{\escaped}{x} \newcommand{\broken})"});
const auto result = source.tex().macro_defs();

std::cout << result.tokens() << '\n';
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["\\RR", "\\vecx", "\\unit", "\\pair"]
```

The optional arity/default groups and `\def` parameter text do not change the
reported name. The escaped declaration and declaration without a replacement
body are omitted.

## Example 2: Definitions Remain Attached To Their Cells

```cpp
std::cout << "offsets=[";
const auto offsets = result.offsets().integers();
for (std::size_t i = 0; i < offsets.size(); ++i)
  std::cout << (i ? ", " : "") << offsets[i];
std::cout << "]\n";
```

### Output

```text
offsets=[0, 2, 4, 4]
```

The first and second cells each own two declarations. The malformed-only cell
owns none.

## Example 3: Undefined And Invalid Input

```cpp
const auto absent = pluss::Note::undefined().tex().macro_defs();
std::cout << std::boolalpha << absent.tokens().defined() << " offsets=["
          << absent.offsets().integers()[0] << "]\n";
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2})
      .tex().macro_defs();
} catch (const pluss::Error&) {
  std::cout << "macro defs Int64: rejected\n";
}
```

### Output

```text
false offsets=[0]
macro defs Int64: rejected
```

## See Also

- [`macro_use`](macro_use.md): inventory distinct control words used per cell.
- [`macro_subs`](macro_subs.md): substitute supported local definitions at use sites.
- [`collapse_macros`](collapse_macros.md): remove local declarations after source analysis.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="line_norm.html"><span class="label">Previous</span><span class="title">line_norm</span></a>
  <a class="next-link" href="macro_subs.html"><span class="label">Next</span><span class="title">macro_subs</span></a>
</div>
