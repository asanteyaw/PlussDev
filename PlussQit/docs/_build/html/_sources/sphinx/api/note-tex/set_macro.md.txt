<!-- pluss-hand-authored -->
# Note::TexAccessor::set_macro

[Back to Note TeX Accessor API Reference](../../note_tex_accessor_api_reference)

## Signature

```cpp
Note Note::TexAccessor::set_macro(std::string_view name,
                               std::string_view body) const
```

## Define One Cell-Local Control Word

`set_macro()` returns TeX source whose first declaration defines `name` with
`body`. It is useful when every cell must carry a self-contained declaration
before later expansion, rendering, storage, or transport. The method does not
expand uses of the macro; [`macro_subs()`](macro_subs.md) performs that separate
job. It also differs from [`cmd_sub()`](cmd_sub.md), which replaces occurrences
lexically without creating a declaration.

Before inserting the declaration, the method removes every complete existing
definition of the same name written with `\newcommand`, `\renewcommand`,
`\providecommand`, or `\def`. Definitions of other names and ordinary uses are
untouched. The canonical inserted spelling is
`\newcommand{\name}{body}` followed by one newline and the remaining source.
Consequently, applying the method again changes the definition rather than
accumulating duplicates.

`name` may be written as `unit` or `\unit`. After the optional leading slash it
must be a non-empty TeX control word containing letters or `@`; a starred name,
spaces, punctuation, and multiple leading slashes reject. `body` is inserted
literally and may contain nested groups and control words. The declaration is
zero-argument, so an unescaped `#` parameter marker rejects; braces must also
balance, with escaped braces ignored by the balance check. Both arguments are
validated before the receiver is read.

The operation is out-of-place and cell-wise. It returns a Text Note with the
receiver's rank and shape, including scalar and empty cells. Undefined state
propagates after argument validation; defined non-Text Notes reject. Processing
is linear in source bytes and allocates one output string per cell.

## Example 1: Replace Or Add A Definition

```cpp
const auto source = pluss::make_note({
    R"(\renewcommand*{\unit}{old}
value=\unit)",
    "plain source",
    R"(\newcommand{\unit}{cm}\unit + \unitless)"});
std::cout << source.tex().set_macro("unit", R"(\mathrm{kg})") << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["\\newcommand{\\unit}{\\mathrm{kg}}\nvalue=\\unit", "\\newcommand{\\unit}{\\mathrm{kg}}\nplain source", "\\newcommand{\\unit}{\\mathrm{kg}}\n\\unit + \\unitless"]
```

The longer `\unitless` use is not mistaken for `\unit`; only declarations are
removed, not uses.

## Example 2: Both Name Spellings Are Equivalent

```cpp
const auto source = pluss::make_note("value=\\unit");
const auto bare = source.tex().set_macro("unit", "m");
const auto slashed = source.tex().set_macro(R"(\unit)", "m");
std::cout << std::boolalpha
          << "same=" << ((bare == slashed).all().bool_item()) << '\n';
```

### Output

```text
same=true
```

## Example 3: Higher-Rank Shape Is Preserved

```cpp
const auto grid = pluss::make_note(
    std::vector<std::string>{"a", "b", "", R"(\unit)"},
    std::array<std::int64_t, 2>{2, 2});
std::cout << grid.tex().set_macro("unit", "s") << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["\\newcommand{\\unit}{s}\na", "\\newcommand{\\unit}{s}\nb"],
  ["\\newcommand{\\unit}{s}\n", "\\newcommand{\\unit}{s}\n\\unit"]
]
```

## Example 4: Invalid Definitions Reject Before Receiver Access

```cpp
for (const auto bad_body : {false, true}) {
  try {
    (void)(bad_body
        ? pluss::Note::undefined().tex().set_macro("unit", "{x")
        : pluss::Note::undefined().tex().set_macro("bad name", "x"));
  } catch (const pluss::Error&) {
    std::cout << (bad_body ? "unbalanced body" : "bad name")
              << ": rejected\n";
  }
}
```

### Output

```text
bad name: rejected
unbalanced body: rejected
```

## Example 5: Undefined State And Dtype

```cpp
std::cout << std::boolalpha << "undefined="
          << pluss::Note::undefined().tex().set_macro("unit", "m").defined()
          << '\n';
try {
  (void)pluss::note::make(std::vector<std::int64_t>{1}).tex()
      .set_macro("unit", "m");
} catch (const pluss::Error&) {
  std::cout << "Int64: rejected\n";
}
```

### Output

```text
undefined=false
Int64: rejected
```

## See Also

- [`macro_subs`](macro_subs.md): expand simple cell-local definitions.
- [`strip_macro`](strip_macro.md): remove one definition without replacing it.
- [`cmd_sub`](cmd_sub.md): replace one exact control word directly.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sections.html"><span class="label">Previous</span><span class="title">sections</span></a>
  <a class="next-link" href="simplify_fractions.html"><span class="label">Next</span><span class="title">simplify_fractions</span></a>
</div>
