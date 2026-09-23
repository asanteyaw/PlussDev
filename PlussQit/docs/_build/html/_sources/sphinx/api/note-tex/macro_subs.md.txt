<!-- pluss-hand-authored -->
# NoteTexAccessor::macro_subs

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note NoteTexAccessor::macro_subs(std::int64_t max_passes = 1) const
```

## Substitute Local Zero-Argument Macros

`macro_subs()` collects simple zero-argument macro declarations from each cell,
removes those declarations, and substitutes matching control-word uses in the
remaining source. It supports the local `\newcommand`, `\renewcommand`, and
`\def` forms represented by the TeX cleanup kernel. Each cell owns its own
macro map; a declaration in one lane never changes another lane.

One pass performs one substitution level. If `\A` expands to `\B` and `\B`
expands to `done`, the default result still contains `\B`; passing `2` reaches
`done`. This round-based rule is deterministic and independent of declaration
or hash-table iteration order. Substitution stops early when a round makes no
change, so a generous bound does not force needless scans after convergence.

The scanner respects control-word boundaries. Declaring `\A` does not rewrite
the prefix of `\Alpha`, and a doubled backslash is retained as a linebreak
rather than treated as a macro use. Parameterized macros, conditionals, TeX
category-code changes, and file-level scope are outside this focused operation;
use it for explicit local textual substitutions rather than as a TeX engine.

`max_passes` must be positive. The bound also prevents an endlessly recursive
definition from running forever: self- or mutually recursive macros stop after
the requested number of rounds. The operation returns a new shape-preserving
Text Note, leaves the receiver unchanged, preserves undefined input, and
rejects non-text dtypes. Each round is a single lexical pass over each cell.

## Example 1: Default One-Level Substitution

```cpp
const auto source = pluss::make_note(
    R"(\newcommand{\A}{\B}\newcommand{\B}{done}Value: \A; prefix: \Alpha.)");
std::cout << source.tex().macro_subs() << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"Value: \\B; prefix: \\Alpha."
```

Declarations are removed, `\A` advances one level, and the longer `\Alpha`
control word remains intact.

## Example 2: Bound Recursive Expansion Explicitly

```cpp
std::cout << source.tex().macro_subs(2) << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"Value: done; prefix: \\Alpha."
```

## Example 3: Macro Maps Are Cell-Local

```cpp
const auto lanes = pluss::make_note({
    R"(\newcommand{\unit}{kg}mass: \unit)",
    R"(\newcommand{\unit}{m}length: \unit)"});
std::cout << lanes.tex().macro_subs() << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["mass: kg", "length: m"]
```

## Example 4: Reject A Meaningless Pass Bound

```cpp
try {
  (void)source.tex().macro_subs(0);
} catch (const pluss::Error&) {
  std::cout << "macro_subs(0): rejected\n";
}
```

### Output

```text
macro_subs(0): rejected
```

Negative values are rejected by the same rule.

## Example 5: Undefined And Invalid Dtypes

```cpp
std::cout << std::boolalpha
          << pluss::Note::undefined().tex().macro_subs().defined() << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1, 2})
      .tex().macro_subs();
} catch (const pluss::Error&) {
  std::cout << "macro substitutions Int64: rejected\n";
}
```

### Output

```text
false
macro substitutions Int64: rejected
```

## See Also

- `collapse_macros`: remove declarations and canonicalize built-in spellings
  without substituting local uses.
- `set_macro`: install or replace one declaration.
- [`strip_macro`](strip_macro.md): remove one declaration without expanding uses.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="macro_defs.html"><span class="label">Previous</span><span class="title">macro_defs</span></a>
  <a class="next-link" href="macro_use.html"><span class="label">Next</span><span class="title">macro_use</span></a>
</div>
